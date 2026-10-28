#!/usr/bin/env python3
"""049 independent clock/capture/geometry, finite pulse and long-run controls."""
import argparse,concurrent.futures,json
from pathlib import Path
import numpy as np
from check_junction import run,transfer,compare
from junction_mesh import fixture,helmholtz

def control(probe,out,name,kind,n,**kw):
    target=out/name;target.mkdir(parents=True,exist_ok=True)
    mesh=fixture(kind,n,**kw.pop('geometry',{}));r,d=run(probe,target,name,mesh,**kw)
    if r['status']:return dict(name=name,rows=[r],pass_=False)
    result=dict(name=name,rows=[r],pass_=r['book_pass'] and r['max_mach']<=.05)
    mode=kw.get('mode','pulse')
    if mode=='translation':
        rho=[1.1764249566561706,.39300733302833885];index=0 if kw['U']>0 else 1
        expected=np.zeros(2);expected[index]=rho[index]*r['volume_m3']
        result['final_material_relative']=float(np.linalg.norm(np.array(r['final_mass'])-expected)/np.linalg.norm(expected))
        result['pass_'] &= result['final_material_relative']<=.001 and r['max_pressure_fraction']<=1e-8
    if mode=='stability':
        result['max_PE_ratio']=r['maximum_PE']/r['initial_PE'];result['final_PE_ratio']=r['final_PE']/r['initial_PE']
        result['pass_'] &= result['max_PE_ratio']<=1.01 and result['final_PE_ratio']<=1e-5
    if mode=='pulse':
        frequencies=[200,500,1000,2000,4000];c=580.1001671698209
        mesh32=fixture(kind,32,stub_ratio=2 if name.startswith('edit') else 1)
        reference=helmholtz(mesh32,frequencies,c)[:,:,0]
        measured,calibration=transfer(r,d,frequencies)
        physical=result['pass_'];result.update(compare(measured,reference));result['accuracy_pass']=result['pass_']
        result['accuracy_required']=not name.startswith('coarse')
        result['calibration_error']=float(np.max(abs(calibration-1)))
        result['pass_']=bool(physical and (result['accuracy_pass'] or not result['accuracy_required']) and result['calibration_error']<=.001)
        np.savez_compressed(target/'transfer.npz',measured=measured,reference=reference,frequencies=frequencies)
    (target/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(name,'PASS' if result['pass_'] else 'FAIL',r['steps'],flush=True);return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--resume',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    jobs=[]
    for U in [-15,15]:
        for n,rate in [(4,512000),(8,1000000)]:jobs.append((f'translate-{U}-{n}','straight',n,dict(mode='translation',rate=rate,T=900,U=U,duration=.032)))
    for U in [0,15]:jobs.append((f'stability-{U}','straight',2,dict(mode='stability',rate=256000,T=900,U=U,amplitude=1,duration=1)))
    for kind in ['y','h']:
        for amplitude in [5,100]:jobs.append((f'finite-{kind}-{amplitude}',kind,16,dict(T=900,amplitude=amplitude)))
    for kind in ['y','h']:jobs.append((f'coarse-{kind}',kind,8,dict(T=900)))
    jobs.extend([('clock-y','y',16,dict(T=900,rate=4000000)),('capture-h','h',16,dict(T=900,duration=.006)),
                 ('edit-x','x',16,dict(T=900,geometry=dict(stub_ratio=2))),
                 ('flow-clock-y','y',8,dict(mode='flow',rate=2000000,T=900,amplitude=1000,duration=.006)),
                 ('flow-clock-area-reversal','area',16,dict(mode='reversal',rate=4000000,T=900,amplitude=1000,duration=.006)),
                 ('flow-clock-area-reversal-fine','area',16,dict(mode='reversal',rate=8000000,T=900,amplitude=1000,duration=.006))])
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        futures=[pool.submit(control,a.probe,a.out,name,k,n,reuse=a.resume,**kw) for name,k,n,kw in jobs];results=[f.result() for f in futures]
    summary=dict(cases=results,pass_=all(r['pass_'] for r in results));(a.out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if not summary['pass_']:raise SystemExit('049 additional control gates failed; retain and inspect')

if __name__=='__main__':main()
