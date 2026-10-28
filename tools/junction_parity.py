#!/usr/bin/env python3
"""Compare raw native/WASI049 trajectories with explicit field tolerances."""
import argparse,csv,hashlib,io,json,subprocess
from pathlib import Path
import numpy as np
from junction_mesh import fixture,native_input

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['native','wasm','node','out']:p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    native_hash=hashlib.sha256(a.native.read_bytes()).hexdigest();wasm_hash=hashlib.sha256(a.wasm.read_bytes()).hexdigest()
    cases=[('x',8,'quiet',1000000,900,0,0,.00015,0),('y',8,'pulse',2000000,900,0,1,.0015,0),
           ('h',8,'pulse',2000000,300,0,1,.0015,0),('area',8,'pulse',2000000,900,1,1,.0015,0),
           ('y',16,'flow',2000000,900,0,1000,.00015,0),('x',8,'reversal',1000000,900,0,1000,.0005,0),
           ('straight',4,'translation',512000,900,0,0,.0005,-15),('straight',2,'stability',256000,900,0,1,.001,15)]
    rows=[]
    for k,n,mode,rate,T,source,amplitude,duration,U in cases:
        name=f'{k}-{n}-{mode}';text=native_input(fixture(k,n),mode,rate,T,source,amplitude,duration,U);(a.out/(name+'.input')).write_text(text)
        outputs=[]
        for label,command in [('native',[str(a.native)]),('wasm',[str(a.node),'tools/bench_wasm.mjs',str(a.wasm)])]:
            r=subprocess.run(command,input=text,text=True,capture_output=True,timeout=600)
            (a.out/(name+'-'+label+'.csv')).write_text(r.stdout);(a.out/(name+'-'+label+'.stderr')).write_text(r.stderr)
            if r.returncode or '"status":0' not in r.stderr:raise ValueError((name,label,r.returncode,r.stderr[-300:]))
            data=list(csv.DictReader(io.StringIO(r.stdout)));outputs.append({key:np.array([float(row[key]) for row in data]) for key in data[0]})
        errors={};passed=True
        for key in outputs[0]:
            x,y=outputs[0][key],outputs[1][key]
            error=float(np.max(abs(x-y)));errors[key]=error
            atol=1e-7 if 'delta_p' in key else 1e-8 if key=='energy_J' or 'u_m_s' in key else 1e-12 if key.endswith('_kg') else 1e-14
            passed &= bool(np.allclose(x,y,rtol=5e-10,atol=atol))
        rows.append(dict(name=name,maximum_absolute_differences=errors,pass_=passed));print(name,passed,max(errors.values()),flush=True)
    bound=hashlib.sha256(a.native.read_bytes()).hexdigest()==native_hash and hashlib.sha256(a.wasm.read_bytes()).hexdigest()==wasm_hash
    summary=dict(cases=rows,native_sha256=native_hash,wasm_sha256=wasm_hash,pass_=bool(bound and all(row['pass_'] for row in rows)));(a.out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if not summary['pass_']:raise SystemExit('Native/WASI trajectory mismatch')

if __name__=='__main__':main()
