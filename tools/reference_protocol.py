"""Session-locked reference ledger and stereo-safe measurement/audition controls.

Raw decoder output is never normalized. This tool cannot certify microphone
calibration, RPM/load or hardware from an MP3 or its spectrum.
"""
import argparse
import base64
import hashlib
import json
import re
from pathlib import Path
import struct
import subprocess
import tempfile

import numpy as np
from audio_wav import read_wav
from checkpoint_sections import publish_section

ROOT=Path(__file__).resolve().parents[1]
RATE=48000
BANDS=((20,500),(500,2000),(2000,8000),(8000,16000))


def digest(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda:f.read(1<<20),b''):h.update(block)
    return h.hexdigest()


def validate(lock):
    if type(lock['schema_version']) is not int or lock['schema_version']!=1:raise ValueError('Unsupported reference lock')
    ids=set();groups={};fingerprints={};clips=set()
    for s in lock['sessions']:
        if re.fullmatch(r'[a-z][a-z0-9-]{0,95}',s['id']) is None:raise ValueError('Unsafe session ID')
        if s['id'] in ids:raise ValueError('Duplicate session')
        ids.add(s['id']);part=s['partition']
        if part not in ('development','validation','quarantine'):raise ValueError('Unknown partition')
        if s['used_before_lock'] and part=='validation':raise ValueError('Previously used audio cannot become held out')
        for key,book in ((s['family'],groups),(s['source_sha256'],fingerprints)):
            if key in book and book[key]!=part:raise ValueError('Session/fingerprint leakage')
            book[key]=part
        if len(s['source_sha256'])!=64 or any(c not in '0123456789abcdef' for c in s['source_sha256']):raise ValueError('Invalid fingerprint')
        for q in s['intervals']:
            if re.fullmatch(r'[a-z][a-z0-9-]{0,95}',q['id']) is None:raise ValueError('Unsafe interval ID')
            if q['id'] in clips:raise ValueError('Duplicate interval')
            clips.add(q['id'])
            a,b=q['start_frame'],q['end_frame']
            if type(a) is not int or a<0 or (b is not None and (type(b) is not int or b<=a)):raise ValueError('Invalid sample interval')
        if s['claims']['calibrated_spl'] or s['claims'].get('matched_engine_response'):
            raise ValueError('Version1 does not implement calibrated SPL or matched-engine qualification')
    return lock


def measurements(x):
    if x.ndim!=2 or len(x)<2 or not np.isfinite(x).all():raise ValueError('Finite multichannel samples required')
    powers=np.mean(x*x,axis=0)
    n=min(8192,len(x));hop=max(1,n//2);w=np.hanning(n)
    bands=np.zeros((x.shape[1],len(BANDS)));blocks=0
    f=np.fft.rfftfreq(n,1/RATE);weights=np.ones(len(f));weights[1:]=2
    if n%2==0:weights[-1]=1
    for start in range(0,len(x)-n+1,hop):
        spec=abs(np.fft.rfft(x[start:start+n]*w[:,None],axis=0))**2
        spec=spec*weights[:,None]/(n*np.sum(w*w))
        for j,(a,b) in enumerate(BANDS):bands[:,j]+=spec[(f>=a)&(f<b)].sum(axis=0)
        blocks+=1
    bands/=blocks
    return dict(channel_power=powers.tolist(),mean_channel_power=float(powers.mean()),
                channel_peak=np.max(abs(x),axis=0).tolist(),band_edges_hz=BANDS,
                channel_band_power=bands.tolist(),mean_channel_band_power=bands.mean(axis=0).tolist(),
                channel_rms_dbfs=(10*np.log10(np.maximum(powers,1e-300))).tolist(),
                folded_mono_power=float(np.mean(x.mean(axis=1)**2)))


def audition_gains(clips,mode,target_rms=.1,peak_cap=.8):
    powers=np.array([np.mean(x*x) for x in clips]);peaks=np.array([np.max(abs(x)) for x in clips])
    if not np.isfinite(powers).all() or not np.isfinite(peaks).all() or np.any(powers<=0):raise ValueError('Non-silent finite audition required')
    if mode=='common':g=np.full(len(clips),target_rms/np.sqrt(powers).max())
    elif mode=='level-matched':g=target_rms/np.sqrt(powers)
    else:raise ValueError('Unknown audition mode')
    g*=min(1,peak_cap/max(peaks*g))
    return g.tolist()


def wav_payload(x,tag=3):
    if x.ndim!=2 or not len(x) or not x.shape[1] or not np.isfinite(x).all():raise ValueError('Finite channel samples required')
    channels=x.shape[1]
    if tag==3:raw=np.asarray(x,dtype='<f4').tobytes();bits=32
    elif tag==1:
        if np.max(abs(x))>1:raise ValueError('Listening conversion would clip')
        raw=np.rint(x*32767).astype('<i2').tobytes();bits=16
    else:raise ValueError('Unsupported WAV format')
    align=channels*(bits//8)
    return struct.pack('<4sI4s4sIHHIIHH4sI',b'RIFF',36+len(raw),b'WAVE',b'fmt ',16,tag,channels,RATE,RATE*align,align,bits,b'data',len(raw))+raw


def prepare(lock,source_dir,out,ffmpeg,publish=False):
    validate(lock);sources={}
    if 'legacy_catalog' in lock:
        legacy=lock['legacy_catalog']
        if digest(ROOT/legacy['path'])!=legacy['sha256']:raise ValueError('Legacy catalog differs from lock')
    for s in lock['sessions']:
        locator=s['source']
        candidates=list(source_dir.glob('*'+locator['filename_contains']+'*.mp3')) if 'filename_contains' in locator else [ROOT/locator['path']]
        if len(candidates)!=1 or not candidates[0].is_file() or digest(candidates[0])!=s['source_sha256']:raise ValueError('Source absent or changed: '+s['id'])
        sources[s['id']]=candidates[0]
    decoder_version=subprocess.check_output([str(ffmpeg),'-version'],text=True).splitlines()[0]
    out.mkdir(parents=True,exist_ok=True);rows=[];development=[]
    for s in lock['sessions']:
        with tempfile.TemporaryDirectory(dir=out) as scratch:
            decoded=Path(scratch)/'decoded.wav'
            command=[str(ffmpeg),'-hide_banner','-nostdin','-v','error','-i',str(sources[s['id']]),'-ar',str(RATE),'-c:a','pcm_f32le',str(decoded)]
            subprocess.run(command,check=True)
            x,rate=read_wav(decoded)
            if rate!=RATE:raise ValueError('Wrong decoded rate')
            for interval in s['intervals']:
                a=interval['start_frame'];b=interval['end_frame'] if interval['end_frame'] is not None else len(x)
                if b>len(x) or a>=b:raise ValueError('Interval exceeds decode')
                y=x[a:b];path=out/'raw'/(interval['id']+'.wav');path.parent.mkdir(exist_ok=True)
                path.write_bytes(wav_payload(y));check,sr=read_wav(path)
                if sr!=RATE or not np.array_equal(y,check):raise ValueError('Raw float excerpt differs from source decode')
                rows.append(dict(id=interval['id'],session=s['id'],partition=s['partition'],source_sha256=s['source_sha256'],
                    start_frame=a,end_frame=b,source_seconds=[a/RATE,b/RATE],channels=x.shape[1],sha256=digest(path),
                    processing='FFmpeg decode/resample to48000Hz float32; sample slicing only; no channel mixing/gain/filter/fade',
                    decode_command=command,**measurements(y)))
                if s['partition']=='development' and len(development)<2:development.append((interval['id'],y[:RATE*4].copy()))
    controls=[];cards=[]
    for mode in (('common','level-matched') if len(development)==2 else ()):
        gains=audition_gains([x for _,x in development],mode)
        for i,((name,x),gain) in enumerate(zip(development,gains)):
            y=x*gain;preview=wav_payload(y,1);controls.append(dict(source=name,mode=mode,gain=gain,seconds=len(x)/RATE,preview_sha256=hashlib.sha256(preview).hexdigest()))
            title='Example '+str(i+1)+' — '+('Same gain' if mode=='common' else 'Matched average level')
            cards.append('<article><h3>'+title+'</h3><p>Previously used reference · '+format(len(x)/RATE,'.1f')+'s</p><audio controls preload="none" aria-label="'+title+'" src="data:audio/wav;base64,'+base64.b64encode(preview).decode()+'"></audio></article>')
    report=dict(schema_version=1,lock_sha256=hashlib.sha256(json.dumps(lock,sort_keys=True).encode()).hexdigest(),
        decoder_version=decoder_version,decoder_sha256=digest(ffmpeg),
        measurement_rate_hz=RATE,source_count=len(sources),clips=rows,audition_controls=controls,
        calibration_qualified=False,reason='No matched exhaust/RPM/load/calibrated microphone set; per-source unknowns stay in lock')
    (out/'report.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    if publish:
        fragment='<!-- REFERENCE PROTOCOL BEGIN --><section id="reference-protocol"><h2>Reference comparison controls</h2><p>Same gain preserves recorded level differences. Matched average level compares texture and removes those differences. These examples were used earlier. New recordings reserved for later checks stay separate. Recording levels do not establish real-world loudness; background sounds may remain.</p>'+''.join(cards)+'</section><!-- REFERENCE PROTOCOL END -->'
        publish_section(ROOT/'renders/RESULTS.html','reference-protocol',fragment)
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--lock',type=Path,default=ROOT/'tools/reference_sessions_v1.json');p.add_argument('--source-dir',type=Path,required=True);p.add_argument('--out',type=Path,default=ROOT/'renders/reference-protocol');p.add_argument('--ffmpeg',type=Path,required=True);p.add_argument('--publish',action='store_true');args=p.parse_args()
    result=prepare(json.loads(args.lock.read_text()),args.source_dir,args.out,args.ffmpeg,args.publish)
    print(len(result['clips']),'raw float references prepared; calibration remains unqualified')
