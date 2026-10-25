#!/usr/bin/env python3
"""048 supplemental packet convergence after the frozen tonal gates passed."""
import argparse,json
from pathlib import Path
import numpy as np
from check_material import run,reference


def evaluate(probe,out):
    out.mkdir(parents=True,exist_ok=True);rows=[]
    for kind in (0,1):
        for n,rate in ((768,768000),(1536,1536000),(3072,2000000)):
            # Same physical initial state/path; independent rate is explicit.
            base,_=run(probe,out,f'base-{kind}-{n}',mode='contact',n=32,kind=kind,duration=.00001)
            duration=.9/base['c_left']+.0006
            r,d=run(probe,out,f'packet-{kind}-{n}',n=n,rate=rate,U=70,kind=kind,front=1.1,frequency=4000,duration=duration)
            if not r['status']:
                p=reference(d['x_m'],d['dx_m'],r);measured=d['p_Pa']-101325
                r['waveform_l2']=float(np.sqrt(np.sum(d['dx_m']*(p-measured)**2)/np.sum(d['dx_m']*p*p)))
                np.savez_compressed(out/f'packet-{kind}-{n}-reference.npz',x=d['x_m'],reference=p,measured=measured)
            rows.append(r)
    result=dict(rows=rows,convergent=all(all(not r['status'] for r in rows if r['kind']==k) and all(a['waveform_l2']>b['waveform_l2'] for a,b in zip([r for r in rows if r['kind']==k],[r for r in rows if r['kind']==k][1:])) for k in (0,1)))
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    r=evaluate(a.probe,a.out);print([(x['kind'],x['n'],x['status'],x.get('waveform_l2')) for x in r['rows']])
