#!/usr/bin/env python3
"""049 signed hot-front and exact-translation controls, with fixed physical books."""
import argparse,concurrent.futures,json
from pathlib import Path
import numpy as np
from check_junction import run
from junction_mesh import fixture

def flow_case(probe,out,kind,mode,amplitude,reuse=False):
    folder=out/f'{kind}-{mode}-{amplitude}';folder.mkdir(parents=True,exist_ok=True)
    results=[];traces=[]
    for n,rate in [(8,1000000),(16,2000000)]:
        r,d=run(probe,folder,f'{kind}-{mode}-{amplitude}-{n}',fixture(kind,n),mode=mode,rate=rate,T=900,amplitude=amplitude,duration=.006,reuse=reuse)
        results.append(r);traces.append(d)
        print(kind,mode,amplitude,n,r['status'],r.get('book_pass'),flush=True)
        if r['status']:break
    if len(results)==2 and all(not r['status'] for r in results):
        # Compare complete physical boundary pressure histories; no delay/gain fit.
        a,b=traces;errors=[]
        for p in range(results[0]['ports']):
            key=f'port_{p}_delta_p_Pa';fine=np.interp(a['time_s'],b['time_s'],b[key])
            errors.append(float(np.linalg.norm(a[key]-fine)/max(np.linalg.norm(fine),1e-12)))
        mass_changes=[np.array(r['final_mass'])-r['initial_mass'] for r in results]
        # Identical geometric initial inventories; normalize the *transported*
        # material, rather than hiding differences under the full gas inventory.
        mass_error=float(np.linalg.norm(mass_changes[0]-mass_changes[1])/max(np.linalg.norm(mass_changes[1]),1e-30))
        result=dict(kind=kind,mode=mode,amplitude=amplitude,rows=results,pressure_l2=errors,transported_mass_relative=mass_error,
                    pass_=bool(all(r['book_pass'] and r['max_mach']<=.05 for r in results) and max(errors)<=.05 and mass_error<=.05))
    else:result=dict(kind=kind,mode=mode,amplitude=amplitude,rows=results,pass_=False)
    (folder/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--workers',type=int,default=2);p.add_argument('--refine',action='store_true');p.add_argument('--resume',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    jobs=[(k,m,A) for k in ['area','y','x','h'] for m,A in [('flow',1000),('flow',-1000),('reversal',1000)]]
    def execute(job):
        r=flow_case(a.probe,a.out,*job,reuse=a.resume)
        if not a.refine:return r
        from refine_junction_flow import refine
        followup=refine(a.probe,a.out/f'{job[0]}-{job[1]}-{job[2]}') if not r['pass_'] else None
        return dict(primary=r,refinement=followup,pass_=bool(r['pass_'] or (followup and followup['pass_'])))
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:
        futures=[pool.submit(execute,job) for job in jobs];results=[f.result() for f in futures]
    summary=dict(cases=results,pass_=all(r['pass_'] for r in results));(a.out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if not summary['pass_']:raise SystemExit('Hot-flow refinement gate failed; retain and inspect')

if __name__=='__main__':main()
