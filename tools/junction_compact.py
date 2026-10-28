#!/usr/bin/env python3
"""Measure compact-node validity fairly: retain geometry's stubs and H bridge."""
import argparse,json
from pathlib import Path
import numpy as np
from junction_mesh import fixture,helmholtz,compact_transfer
from check_junction import compare

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    frequencies=[1,10,25,50,100,200,500,1000,2000,4000];rows=[]
    for kind in ['area','y','x','h']:
        for T,c in [(300,347.24387326840287),(900,580.1001671698209)]:
            mesh=fixture(kind,32);reference=helmholtz(mesh,frequencies,c)
            for finite in [False,True]:
                compact=compact_transfer(mesh,frequencies,c,finite);comparisons=[compare(x[None,:,:],y[None,:,:]) for x,y in zip(compact,reference)]
                rows.append(dict(kind=kind,T=T,finite_volume=finite,frequencies=frequencies,comparisons=comparisons,
                                 tested_valid_frequencies=[f for f,r in zip(frequencies,comparisons) if r['pass_']]))
                np.savez_compressed(a.out/f'{kind}-{T}-{finite}.npz',frequencies=frequencies,reference=reference,compact=compact)
            print(kind,T,rows[-1]['tested_valid_frequencies'],flush=True)
    summary=dict(cases=rows,low_frequency_limit_pass=all(r['comparisons'][0]['matrix_relative_error']<=.02 for r in rows))
    (a.out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if not summary['low_frequency_limit_pass']:raise SystemExit('Compact reference fails its analytic low-frequency limit')

if __name__=='__main__':main()
