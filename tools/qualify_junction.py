#!/usr/bin/env python3
"""049 reproducible all-port reference matrix; retain primary and finer results."""
import argparse,concurrent.futures,hashlib,json
from pathlib import Path
import numpy as np
from check_junction import acoustic_case
from junction_mesh import fixture,helmholtz

def qualify(probe,out,kind,T,reuse=False):
    target=out/f'{kind}-{T}';target.mkdir(parents=True,exist_ok=True)
    c=347.24387326840287 if T==300 else 580.1001671698209
    frequencies=[200,500,1000,2000]+([4000] if T==900 else [])
    coarse=helmholtz(fixture(kind,24),frequencies,c)
    reference=helmholtz(fixture(kind,32),frequencies,c)
    uncertainty=float(np.linalg.norm(reference-coarse)/np.linalg.norm(reference))
    np.savez_compressed(target/'independent-reference.npz',frequencies=frequencies,n24=coarse,n32=reference)
    if uncertainty>.02:raise ValueError((kind,T,'independent reference unconverged',uncertainty))
    candidates=[]
    for n,rate in [(16,2000000),(24,4000000),(32,4000000)]:
        result=acoustic_case(probe,target,kind,n,T,frequencies,reference,rate=rate,reuse=reuse)
        candidates.append(result)
        print(kind,T,n,'PASS' if result['pass'] else 'FAIL',result.get('matrix_relative_error'),flush=True)
        if result['pass']:break
    # Show the actual compact approximation error without selecting it as truth.
    areas=np.array([p['area_m2'] for p in fixture(kind,8)['ports']])
    compact=np.ones((len(areas),1))*(2*areas/areas.sum())[None,:]-np.eye(len(areas))
    compact_errors=[float(np.linalg.norm(r-compact)/np.linalg.norm(r)) for r in reference]
    result=dict(kind=kind,T=T,frequencies=frequencies,reference_refinement_relative=uncertainty,
                fully_collapsed_zero_volume_relative_errors=compact_errors,candidates=candidates,
                selected_n=candidates[-1]['n'],pass_=bool(candidates[-1]['pass']))
    (target/'qualification.json').write_text(json.dumps(result,indent=2)+'\n')
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True);p.add_argument('--workers',type=int,default=2)
    p.add_argument('--resume',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    jobs=[(k,T) for k in ['straight','area','y','x','h'] for T in [300,900]]
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:
        futures=[pool.submit(qualify,a.probe,a.out,k,T,a.resume) for k,T in jobs]
        results=[f.result() for f in futures]
    summary=dict(probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),cases=results,
                 pass_=all(r['pass_'] for r in results))
    (a.out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if not summary['pass_']:raise SystemExit('049 matrix has failed gates')

if __name__=='__main__':main()
