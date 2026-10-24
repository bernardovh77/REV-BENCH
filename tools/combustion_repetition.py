"""Stage041 three-seed stereo idle repetition; no reference fitting or mono fold."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import numpy as np
from audio_wav import read_wav,read_wav_bytes
from compare_real import BANDS,as_db,band,repeat
from reference_protocol import measurements
from check_combustion_variation import SEEDS


def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def analyze(path,start=6,end=30):
    if read_wav_bytes(path)[1]!=3:raise ValueError('Unfiltered float measurement source required')
    x,rate=read_wav(path)
    if rate!=48000 or x.shape[1]!=2 or len(x)<round(end*rate) or not np.isfinite(x).all():
        raise ValueError('Incomplete/nonfinite stereo48kHz source')
    x=x[round(start*rate):round(end*rate)]
    if not len(x) or np.max(abs(x))>=1:raise ValueError('Source needs finite unclipped headroom')
    channels=[]
    for ch in range(2):
        y=x[:,ch]-x[:,ch].mean()
        if not np.any(y):raise ValueError('Silent channel cannot qualify a repetition metric')
        low,cycle=repeat(band(y,rate,*BANDS[0]),rate,120/1600,120/450)
        values=[float(as_db(low))]
        values.extend(float(as_db(repeat(band(y,rate,*b),rate,.97*cycle,1.03*cycle)[0])) for b in BANDS[1:])
        if not np.isfinite(values).all():raise ValueError('Nonfinite repetition observation')
        channels.append(dict(channel=ch,repetition_db=values,selected_lag_s=cycle,
            legacy_low_band_target_met=0<=values[0]<=15))
    return dict(raw_sha256=digest(path),window_s=[start,end],channels=channels,
                raw_stereo_power=measurements(x),
                method='Original channels analyzed independently; legacy maximum-lag correlation, .5s frames. Selected lag may be a harmonic; not independent RPM. Bands are analysis filters only; no source EQ/gain/folding.',
                calibrated_realism=False,s1_complete=False)


def execute(cli,out,workers):
    out.mkdir(parents=True,exist_ok=True);cli_hash=digest(cli)
    jobs=[(preset,seed) for preset in ('i4','i6','v8-cross','v8-flat','boxer4') for seed in SEEDS]
    def one(job):
        preset,seed=job;key=f'{preset}-seed{seed}';wav=out/(key+'.wav');report=out/(key+'.json')
        cmd=[str(cli),'sound','--preset',preset,'--idle','--seconds','30','--quality','high',
             '--seed',str(seed),'--variation','1','--scene','asphalt','--gain','.5','--float','--out',str(wav)]
        old=json.loads(report.read_text()) if report.exists() else {}
        if not (old.get('cli_sha256')==cli_hash and old.get('command')==cmd and wav.exists()
                and old['analysis']['raw_sha256']==digest(wav)):
            run=subprocess.run(cmd,capture_output=True,text=True,timeout=600)
            log=run.stdout+run.stderr;(out/(key+'.log')).write_text(log)
            counts=re.findall(r'soft-limited samples (\d+), non-finite (\d+)',log)
            if run.returncode or counts!=[('0','0')]:raise ValueError('Audio render rejected: '+key)
            row=dict(preset=preset,seed=seed,cli_sha256=cli_hash,command=cmd,analysis=analyze(wav))
            report.write_text(json.dumps(row,indent=2)+'\n')
        else:row=old
        print(key,'repetition measured',flush=True);return row
    with ThreadPoolExecutor(max_workers=workers) as pool:rows=list(pool.map(one,jobs))
    result=dict(runs=len(rows),same_gain=.5,scene='asphalt',core_variation=1,window_s=[6,30],
        scope='Seeded per-channel descriptive engine-audio metrics; no real-engine calibration or S1 completion.',
        rows=rows,s1_complete=False)
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cli',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--workers',type=int,default=2);args=p.parse_args()
    if not 1<=args.workers<=4:p.error('workers must be1..4')
    execute(args.cli.resolve(),args.out,args.workers)


if __name__=='__main__':main()
