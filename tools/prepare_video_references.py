"""Prepare supplied video references and append three auditions to the one result.

All measurements use the versioned protocol's unfiltered original channels.
Video views and audio microphone positions are separate evidence. No engine
parameter or reference calibration is inferred from the recordings.
"""
import argparse
import base64
import hashlib
from html import escape
import json
from pathlib import Path
import subprocess
import tempfile

import numpy as np
from audio_wav import read_wav
from checkpoint_sections import checked, load, publish_section
from reference_protocol import ROOT, RATE, digest, prepare, validate, wav_payload

BEGIN = '<!-- VIDEO REFERENCES BEGIN -->'
END = '<!-- VIDEO REFERENCES END -->'
NEW_IDS = ('user-gr86-mountain', 'user-gr86-awe', 'user-e46-valvetronic')


def checked_lock(path):
    lock = validate(json.loads(path.read_text()))
    parent = lock['parent_lock']
    if digest(ROOT/parent['path']) != parent['sha256']:
        raise ValueError('Frozen parent lock changed')
    original = json.loads((ROOT/parent['path']).read_text())
    if lock['sessions'][:len(original['sessions'])] != original['sessions']:
        raise ValueError('Earlier source assignments/intervals changed')
    added = lock['sessions'][len(original['sessions']):]
    if tuple(s['id'] for s in added) != NEW_IDS or any(s['partition']!='development' for s in added):
        raise ValueError('These reviewed video sources must remain development references')
    for s in added:
        if s['preset']=='boxer4' and s['metadata']['preset_relationship']!='related-fa24-not-fb20':
            raise ValueError('GR86 is not an exact FB20 reference')
    return lock, added


def screen(added, evidence):
    """Check each selected interval against both channels of the recorded screen."""
    reports = {r['video']:r for r in json.loads((evidence/'speech-screen.json').read_text())}
    rows=[]
    for s in added:
        vid=s['source_page'].split('v=')[-1]
        report=reports[vid]
        if report['source_sha256']!=s['source_sha256'] or report['probabilities_sha256']!=digest(evidence/(vid+'-vad.npy')):
            raise ValueError('Speech screen is not bound to this source')
        prob=np.load(evidence/(vid+'-vad.npy'),allow_pickle=False)
        if prob.ndim!=2 or prob.shape[1]!=2 or not np.isfinite(prob).all():
            raise ValueError('Expected a finite per-channel speech screen')
        for q in s['intervals']:
            start=max(0,int(np.floor((q['start_frame']/RATE-.55)/.032)))
            end=min(len(prob),int(np.ceil((q['end_frame']/RATE+.55)/.032)))
            peak=float(prob[start:end].max())
            if peak>=.35:
                raise ValueError('Speech-risk interval: '+q['id'])
            rows.append(dict(id=q['id'],peak_speech_probability=peak,padding_s=.55,
                             screen_sha256=digest(evidence/(vid+'-vad.npy')),
                             model_sha256=reports[vid]['model_sha256']))
    return rows


def auditions(added, out, ffmpeg):
    preview=out/'listening';preview.mkdir(exist_ok=True)
    rows=[]
    for s in added:
        pieces=[];timeline=[];at=0.
        for i,q in enumerate(s['intervals']):
            x,rate=read_wav(out/'raw'/(q['id']+'.wav'))
            if rate!=RATE or x.shape[1]!=2:
                raise ValueError('Expected original48kHz stereo')
            # Endpoint fades are listening-only; measurement samples stay intact.
            y=x.copy();n=min(480,len(y)//2);y[:n]*=np.linspace(0,1,n)[:,None];y[-n:]*=np.linspace(1,0,n)[:,None]
            if i:
                pieces.append(np.zeros((round(.7*RATE),2)));at+=.7
            pieces.append(y);duration=len(y)/RATE
            timeline.append(dict(id=q['id'],preview_seconds=[at,at+duration],
                source_seconds=[q['start_frame']/RATE,q['end_frame']/RATE],
                view=q['acoustic_group'],exhaust_valves=q.get('exhaust_valves')));at+=duration
        audio=np.concatenate(pieces);rms=float(np.sqrt(np.mean(audio*audio)));peak=float(np.max(abs(audio)))
        gain=min(.1/max(rms,1e-30),.65/max(peak,1e-30));audio*=gain
        path=preview/(s['id']+'.mp3')
        with tempfile.TemporaryDirectory(dir=preview) as tmp:
            wav=Path(tmp)/'listening.wav';wav.write_bytes(wav_payload(audio))
            command=[str(ffmpeg),'-hide_banner','-nostdin','-v','error','-i',str(wav),'-c:a','libmp3lame','-b:a','192k','-y',str(path)]
            subprocess.run(command,check=True)
            check=Path(tmp)/'check.wav'
            subprocess.run([str(ffmpeg),'-hide_banner','-nostdin','-v','error','-i',str(path),'-c:a','pcm_f32le',str(check)],check=True)
            y,_=read_wav(check);decoded_peak=float(np.max(abs(y)))
            if not np.isfinite(y).all() or decoded_peak>=1:
                raise ValueError('Listening encode has no headroom')
        rows.append(dict(id=s['id'],title=s['title'],file=str(path),sha256=digest(path),gain=gain,
            seconds=at,decoded_peak=decoded_peak,timeline=timeline,
            processing='Stereo;10ms endpoint fades per excerpt;0.7s gaps;one fixed gain per source;192kbpsMP3. No EQ, denoising, pitch or distance correction. Raw excerpts unchanged.'))
    return rows


def publish(added, rows):
    target=ROOT/'renders/RESULTS.html';renders=target.parent;manifest=load(renders)
    old=next(e for e in manifest['sections'] if e['id']=='user-references')
    fragment=checked(renders,old).read_text()
    if BEGIN in fragment:
        before,rest=fragment.split(BEGIN,1);fragment=before+rest.split(END,1)[1]
    cards=[]
    for s,row in zip(added,rows):
        timeline=' · '.join(f"{a['source_seconds'][0]:g}–{a['source_seconds'][1]:g}s ({a['view']}{', '+a['exhaust_valves'] if a['exhaust_valves'] else ''})" for a in row['timeline'])
        picture=''
        if row.get('perspective_image'):
            picture='<details><summary>Recording views</summary><img style="max-width:100%;height:auto" alt="Reviewed source camera viewpoints" src="data:image/jpeg;base64,'+base64.b64encode(Path(row['perspective_image']).read_bytes()).decode()+'"></details>'
        payload=base64.b64encode(Path(row['file']).read_bytes()).decode()
        cards.append('<article><h3>'+escape(s['title'])+'</h3><p>'+escape(s['metadata']['perspective'])+'</p><p>Source excerpts: '+escape(timeline)+'</p>'+picture+'<audio controls preload="none" aria-label="'+escape(s['title'],quote=True)+'" src="data:audio/mpeg;base64,'+payload+'"></audio></article>')
    addition=BEGIN+'<section id="video-references"><h2>GR86 and E46 video references</h2><p>GR86 driving excerpts are heard inside the cabin; the 2.4-litre FA24 differs from our FB20 preset. E46 excerpts separate rear-quarter revs, roadside pass-bys and views beside the tailpipe. Valve labels come from the video. Camera position does not verify the microphone position.</p><p>Introductions, car washing and presenter-led sections are excluded. Speech screening reduces dialogue risk; road, wind or other background sound may remain. GR86 sound-enhancement settings, microphone gain and precise RPM/load remain unknown.</p><p>One fixed listening gain per recording; 10 ms end fades and 0.7 s gaps. Different recordings are not loudness matched. Original-channel float excerpts remain the measurement sources.</p>'+''.join(cards)+'</section>'+END
    # The registered producer uses a newline between closing tags.
    marker='<!-- USER REFERENCES END -->';pos=fragment.rfind('</section>',0,fragment.rfind(marker))
    if pos<0:
        raise ValueError('Expected registered reference section')
    fragment=fragment[:pos]+addition+fragment[pos:]
    publish_section(target,'user-references',fragment)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--lock',type=Path,default=ROOT/'tools/reference_sessions_v2.json')
    p.add_argument('--source-dir',type=Path,required=True)
    p.add_argument('--ffmpeg',type=Path,required=True)
    p.add_argument('--out',type=Path,default=ROOT/'renders/reference-protocol-v2')
    p.add_argument('--evidence',type=Path,default=ROOT/'renders/evidence/video-references-2026-10-24')
    p.add_argument('--publish',action='store_true');args=p.parse_args()
    lock,added=checked_lock(args.lock);screens=screen(added,args.evidence)
    report=prepare(lock,args.source_dir,args.out,args.ffmpeg,publish=False)
    rows=auditions(added,args.out,args.ffmpeg)
    for row in rows:
        image=args.evidence/(row['id']+'-perspectives.jpg')
        if image.is_file():
            row['perspective_image']=str(image)
    if any(digest(ROOT/s['source']['path'])!=s['source_sha256'] for s in added):
        raise ValueError('Source changed during preparation')
    (args.out/'video-reference-report.json').write_text(json.dumps(dict(lock_revision=2,clips=rows,speech_checks=screens,
        source_originals_unchanged=True,calibration_qualified=False,source_count=report['source_count']),indent=2)+'\n')
    if args.publish:
        publish(added,rows)
    print(f"Prepared{len(screens)} screened stereo excerpts from3videos; prior sessions preserved; calibration remains open")


if __name__=='__main__':main()
