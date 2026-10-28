#!/usr/bin/env python3
"""Retain a failed049 coarse hot-flow gate and apply predeclared24/32 controls."""
import argparse,csv,json
from pathlib import Path
import numpy as np
from check_junction import run
from junction_mesh import fixture

def compare_rows(a,b,da,db):
    errors=[]
    for p in range(a['ports']):
        key=f'port_{p}_delta_p_Pa';fine=np.interp(da['time_s'],db['time_s'],db[key])
        errors.append(float(np.linalg.norm(da[key]-fine)/max(np.linalg.norm(fine),1e-12)))
    ma=np.array(a['final_mass'])-a['initial_mass'];mb=np.array(b['final_mass'])-b['initial_mass']
    error=float(np.linalg.norm(ma-mb)/max(np.linalg.norm(mb),1e-30))
    return dict(n_coarse=a['cells'],n_fine=b['cells'],pressure_l2=errors,transported_mass_relative=error,
                pass_=bool(a['book_pass'] and b['book_pass'] and max(errors)<=.05 and error<=.05 and max(a['max_mach'],b['max_mach'])<=.05))

def load_trace(path):
    with path.open() as stream:rows=list(csv.DictReader(stream))
    return {k:np.array([float(r[k]) for r in rows]) for k in rows[0]}

def refine(probe,case):
    initial=json.loads((case/'summary.json').read_text());kind=initial['kind'];mode=initial['mode'];A=initial['amplitude']
    name=f'{kind}-{mode}-{A}';previous=initial['rows'][-1];trace=load_trace(case/(name+'-16.csv'));refinements=[]
    previous_error=initial.get('transported_mass_relative',float('inf'))
    trials=[(24,4000000,None),(32,4000000,None)]
    trials+=([(32,4000000,40),(32,4000000,48)] if kind=='h' else [(40,8000000,None),(48,8000000,None)])
    for n,rate,transport_n in trials:
        mesh=fixture(kind,n,h_transport_n=transport_n)
        if len(mesh['cells'])>8192 or len(mesh['faces'])>65536:break
        label=name+f'-{n}'+(f'-transport-{transport_n}' if transport_n is not None else '')
        r,d=run(probe,case,label,mesh,mode=mode,rate=rate,T=900,amplitude=A,duration=.006,reuse=True)
        result=compare_rows(previous,r,trace,d) if not r['status'] else dict(pass_=False)
        result['convergence_decreasing']=result.get('transported_mass_relative',float('inf'))<previous_error
        result['pass_'] &= result['convergence_decreasing']
        result.update(n=n,transport_n=transport_n,row=r);refinements.append(result);print(name,n,transport_n,result.get('transported_mass_relative'),result['pass_'],flush=True)
        if result['pass_'] or r['status']:break
        previous,trace,previous_error=r,d,result.get('transported_mass_relative',float('inf'))
    final=dict(primary=initial,refinements=refinements,selected_n=refinements[-1]['n'],selected_transport_n=refinements[-1]['transport_n'],pass_=refinements[-1]['pass_'])
    (case/'refinement.json').write_text(json.dumps(final,indent=2)+'\n');return final

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--case',type=Path,required=True);a=p.parse_args()
    final=refine(a.probe,a.case)
    if not final['pass_']:raise SystemExit('Fine flow controls still fail')

if __name__=='__main__':main()
