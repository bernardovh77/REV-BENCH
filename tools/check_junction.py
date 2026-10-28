#!/usr/bin/env python3
"""049 independent finite-junction geometry, complex transfer and flow checks."""
import argparse,csv,hashlib,io,json,re,subprocess
from pathlib import Path
from types import SimpleNamespace
import numpy as np
from junction_mesh import fixture,native_input,helmholtz

def run(probe,out,name,mesh,mode='pulse',rate=2000000,T=300,source=0,amplitude=1,duration=.003,U=0,reuse=False):
    binary_hash=hashlib.sha256(probe.read_bytes()).hexdigest()
    text=native_input(mesh,mode,rate,T,source,amplitude,duration,U)
    input_hash=hashlib.sha256(text.encode()).hexdigest();raw=None
    if reuse and all((out/(name+s)).exists() for s in ['.input','.csv','.stderr','.json']):
        old=json.loads((out/(name+'.json')).read_text())
        if old.get('probe_sha256')==binary_hash and old.get('input_sha256')==input_hash and hashlib.sha256((out/(name+'.input')).read_bytes()).hexdigest()==input_hash:
            raw=SimpleNamespace(returncode=0,stdout=(out/(name+'.csv')).read_text(),stderr=(out/(name+'.stderr')).read_text())
    (out/(name+'.input')).write_text(text)
    if raw is None:raw=subprocess.run([str(probe)],input=text,capture_output=True,text=True,timeout=7200)
    (out/(name+'.csv')).write_text(raw.stdout);(out/(name+'.stderr')).write_text(raw.stderr)
    if raw.returncode:raise ValueError((name,raw.returncode,raw.stderr[-600:]))
    lines=[x[len('RB_JV_RESULT '):] for x in raw.stderr.splitlines() if x.startswith('RB_JV_RESULT ')]
    if len(lines)!=1:raise ValueError((name,'missing result'))
    r=json.loads(re.sub(r'\b(?:-?nan|-?inf)\b','null',lines[0]));r.update(name=name,mode=mode,rate=rate,T=T,source=source,amplitude=amplitude,duration=duration,U=U,geometry_id=mesh['id'])
    if hashlib.sha256(probe.read_bytes()).hexdigest()!=binary_hash:raise ValueError('Probe changed during measurement')
    r['probe_sha256']=binary_hash;r['input_sha256']=hashlib.sha256(text.encode()).hexdigest()
    if not r['status']:
        mass=r['rho_reference']*r['volume_m3'];energy=101325*r['volume_m3']+abs(r['initial_E_J']);momentum=mass*r['c_reference']
        # Hot/cold controls use the larger density as a declared common scale.
        if mode in ['flow','translation','reversal']:mass=max(r['rho_reference'],1.1764249566561706)*r['volume_m3'];momentum=mass*max(r['c_reference'],580.1001671698209)
        r['mass_relative']=max(max(map(abs,r['mass_balance'])),r['regional_mass_error_kg'])/mass
        r['momentum_relative']=max(max(map(abs,r['momentum_balance'])),r['regional_momentum_error_Ns'])/momentum
        r['energy_relative']=max(abs(r['energy_balance_J']),r['regional_energy_error_J'])/energy
        r['book_pass']=max(r['mass_relative'],r['momentum_relative'],r['energy_relative'])<=1e-10
    rows=list(csv.DictReader(io.StringIO(raw.stdout))) if raw.stdout.strip() else []
    data={k:np.array([float(v[k]) for v in rows]) for k in rows[0]} if rows else {}
    (out/(name+'.json')).write_text(json.dumps(r,indent=2)+'\n');return r,data

def transfer(r,data,frequencies):
    if r['status']:raise ValueError('Rejected propagation')
    t=data['time_s'];driver=data['drive_Pa'];phase=np.exp(-2j*np.pi*np.array(frequencies)[:,None]*t)
    incoming=phase@driver;Z=r['rho_reference']*r['c_reference'];responses=[];calibration=[]
    for port in range(r['ports']):
        p=data[f'port_{port}_delta_p_Pa'];u=data[f'port_{port}_outward_u_m_s']
        responses.append((phase@(.5*(p+Z*u)))/incoming)
        if port==r['source']:calibration=(phase@(.5*(p-Z*u)))/incoming
    return np.array(responses).T,np.array(calibration)

def compare(candidate,reference):
    strong=np.abs(reference)>=.05;weak=~strong;ratio=np.ones(reference.shape,dtype=complex);ratio[strong]=candidate[strong]/reference[strong]
    magnitude=np.abs(20*np.log10(abs(ratio[strong])));phase=np.abs(np.angle(ratio[strong],deg=True))
    error=float(np.linalg.norm(candidate-reference)/np.linalg.norm(reference));small=float(np.max(abs(candidate[weak]-reference[weak]))) if np.any(weak) else 0.
    return dict(matrix_relative_error=error,max_magnitude_db=float(magnitude.max(initial=0)),max_phase_deg=float(phase.max(initial=0)),weak_absolute_error=small,
                pass_=bool(error<=.05 and magnitude.max(initial=0)<=1 and phase.max(initial=0)<=10 and small<=.01))

def acoustic_case(probe,out,kind,n,T,frequencies,reference,rate=2000000,duration=.003,sources=None,reuse=False):
    mesh=fixture(kind,n);(out/f'{kind}-{n}-geometry.json').write_text(json.dumps(mesh,indent=2)+'\n')
    sources=list(range(len(mesh['ports']))) if sources is None else list(sources)
    columns=[];rows=[]
    for source in sources:
        r,d=run(probe,out,f'{kind}-{n}-{T}-source-{source}-{rate}',mesh,rate=rate,T=T,source=source,duration=duration,reuse=reuse)
        if r['status']:return {'kind':kind,'n':n,'T':T,'rows':rows+[r],'pass':False}
        response,calibration=transfer(r,d,frequencies);r['calibration_error']=float(np.max(abs(calibration-1)))
        r['tail_relative']=float(np.linalg.norm(d[f'port_{source}_delta_p_Pa'][-max(8,len(d['time_s'])//10):])/max(np.linalg.norm(d[f'port_{source}_delta_p_Pa']),1e-30))
        r.update(compare(response,reference[:,:,source]));columns.append(response);rows.append(r)
    matrix=np.stack(columns,axis=-1);selected=reference[:,:,list(sources)];comparison=compare(matrix,selected)
    result=dict(kind=kind,n=n,T=T,frequencies=frequencies,rows=rows,**comparison)
    result['pass']=bool(comparison['pass_'] and all(r['book_pass'] and r['calibration_error']<=.001 for r in rows))
    np.savez_compressed(out/f'{kind}-{n}-{T}-{rate}-matrix.npz',candidate=matrix,reference=selected,frequencies=frequencies)
    (out/f'{kind}-{n}-{T}-{rate}-summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--kind',default='x');p.add_argument('--n',type=int,default=8);p.add_argument('--temperature',type=int,default=300);a=p.parse_args()
    a.out.mkdir(parents=True,exist_ok=True)
    # Audited047 property values, never a fit of the candidate's wave travel.
    c=347.24387326840287 if a.temperature==300 else 580.1001671698209
    frequencies=[200,500,1000,2000]+([4000] if a.temperature==900 else [])
    reference=helmholtz(fixture(a.kind,24),frequencies,c)
    r=acoustic_case(a.probe,a.out,a.kind,a.n,a.temperature,frequencies,reference)
    print({k:v for k,v in r.items() if k not in ['rows']})
