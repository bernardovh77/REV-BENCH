#!/usr/bin/env python3
"""049 actual no-I/O native CPU cost; report fixtures, never full-engine speed."""
import argparse,json,statistics
from pathlib import Path
from check_junction import run
from junction_mesh import fixture

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    results=[]
    for kind,n,rate in [('straight',8,1000000),('y',16,2000000),('x',16,2000000),('h',16,2000000)]:
        for U in [0,1]:
            rows=[]
            for repeat in range(3):
                r,_=run(a.probe,a.out,f'{kind}-{n}-{U}-{repeat}',fixture(kind,n),mode='cost',rate=rate,T=900,U=U,duration=.0005)
                if r['status'] or not r['book_pass'] or r['timing_clock']!='process_cpu':raise ValueError('Invalid native timing/control')
                rows.append(r)
            median=statistics.median(r['elapsed_s'] for r in rows)
            result=dict(kind=kind,n=n,U=U,rows=rows,median_cpu_s=median,simulated_per_cpu=.0005/median,bytes=rows[0]['bytes'])
            results.append(result);print(kind,n,U,result['simulated_per_cpu'],result['bytes'],flush=True)
    (a.out/'summary.json').write_text(json.dumps(dict(cases=results,pass_=True,scope='single uniform-gas research fixture; preparation/allocation and diagnostic I/O excluded; not an engine benchmark'),indent=2)+'\n')

if __name__=='__main__':main()
