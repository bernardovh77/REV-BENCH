#!/usr/bin/env python3
"""043 frozen numerical experiment; rejection is an explicit successful outcome.

Nonlinear reference uses the exact perfect-gas Riemann solution only inside
the inherited EOS's constant-cv (<200 K) extension. No fit to candidate output.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import csv
import io
import itertools
import json
import math
from pathlib import Path
import subprocess


def run(probe, mode='wave', n=128, rate=192000, temperature=300, flow=0,
        frequency=2000, amplitude=.1, mixture=0, duration=0, boundary=0):
    args=[str(probe),mode,*map(str,(n,rate,temperature,flow,frequency,amplitude,mixture,duration,boundary))]
    result=subprocess.run(args,text=True,capture_output=True,timeout=180)
    if result.returncode:
        raise ValueError(f'Probe failed {args}: {result.stderr[:300]}')
    if mode in ('sod','rarefaction'):
        return list(csv.DictReader(io.StringIO(result.stdout)))
    record=json.loads(result.stdout)
    record.update(mode=mode,temperature=temperature,flow=flow,frequency=frequency,
                  amplitude=amplitude,mixture=mixture,boundary=boundary)
    if not all(math.isfinite(v) for v in record.values() if type(v) is float):
        raise ValueError('Nonfinite evidence')
    scales=[max(abs(x),1e-3) for x in record['initial']]
    scales[3]=max(1,abs(record['initial'][0])*record['c'])
    scales[4]=max(1,abs(record['initial'][4]),abs(record['initial'][0])*record['c']**2)
    record['relative_balance']=[abs(x)/scale for x,scale in zip(record['balance'],scales)]
    record['conservation_pass']=max(record['relative_balance'])<=1e-10
    record['points_per_wavelength']=n/4 if mode=='wave' else None
    record['representation_pass']=mode!='wave' or n>=16
    record['acoustic_pass']=record['status']==0 and record['representation_pass'] and abs(record['magnitude_db'])<=1 and abs(record['phase_deg'])<=10
    return record


def exact_riemann(x,time,left,right,gamma):
    """Independent scalar pressure root, then analytic shock/fan sampling."""
    def wave(p,state):
        rho,u,p0=state;c=math.sqrt(gamma*p0/rho)
        if p>p0:
            return (p-p0)*math.sqrt(2/((gamma+1)*rho)/(p+(gamma-1)/(gamma+1)*p0))
        return 2*c/(gamma-1)*((p/p0)**((gamma-1)/(2*gamma))-1)
    lo,hi=1e-12,max(left[2],right[2])*10
    def eq(p): return wave(p,left)+wave(p,right)+right[1]-left[1]
    if eq(lo)>0: raise ValueError('Vacuum is outside reference domain')
    while eq(hi)<0: hi*=2
    for _ in range(100):
        mid=(lo+hi)/2
        if eq(mid)>0: hi=mid
        else: lo=mid
    ps=(lo+hi)/2;us=.5*(left[1]+right[1]+wave(ps,right)-wave(ps,left))
    side=-1 if x/time<us else 1
    rho,u,p=left if side<0 else right;c=math.sqrt(gamma*p/rho);xi=x/time
    if ps>p:
        speed=u+side*c*math.sqrt((gamma+1)/(2*gamma)*ps/p+(gamma-1)/(2*gamma))
        if side*(xi-speed)>=0: return rho,u,p
        ratio=ps/p;g=(gamma-1)/(gamma+1)
        return rho*(ratio+g)/(g*ratio+1),us,ps
    cs=c*(ps/p)**((gamma-1)/(2*gamma))
    head=u+side*c;tail=us+side*cs
    if side*(xi-head)>=0: return rho,u,p
    if side*(xi-tail)<=0: return rho*(ps/p)**(1/gamma),us,ps
    uf=2/(gamma+1)*(-side*c+(gamma-1)/2*u+xi)
    cf=2/(gamma+1)*(c+side*(gamma-1)/2*(xi-u))
    return rho*(cf/c)**(2/(gamma-1)),uf,p*(cf/c)**(2*gamma/(gamma-1))


def experiment(probe,out):
    out.mkdir(parents=True,exist_ok=True)
    # Thermodynamics is read from a zero-duration tiny wave, not fitted travel.
    tasks=[]
    for mix,T,U,f,dx,rate in itertools.product((0,1),(300,900),(-70,0,70),(200,1000,2000,4000),(.05,.025,.0125),(192000,384000)):
        # R/cv-based sound speed from the same declared EOS is the independent
        # continuum characteristic. Obtain it without any propagation steps.
        base=run(probe,n=8,rate=1e12,temperature=T,flow=U,frequency=f,mixture=mix,duration=1e-12)
        n=max(4,round((base['c']+U)*4/f/dx))
        tasks.append(dict(n=n,rate=rate,temperature=T,flow=U,frequency=f,mixture=mix))
    with ThreadPoolExecutor(max_workers=2) as pool:
        rows=list(pool.map(lambda args:run(probe,**args),tasks))
    (out/'waves.json').write_text(json.dumps(rows,indent=2)+'\n')
    contacts=[run(probe,mode='contact',n=n,rate=384000,temperature=300,flow=u,duration=.002,mixture=0)
              for n,u in itertools.product((80,160,320),(0,70,-70))]
    finite=[run(probe,n=128,rate=384000,frequency=2000,amplitude=a,duration=t)
            for a,t in itertools.product((100,1000),(.003,.03))]
    boundary=[run(probe,mode='contact',n=80,rate=384000,temperature=300,flow=20,duration=.001,boundary=b)
              for b in (1,2)]
    # Below200 K air has constant cv in production. Derive this declared gas
    # constant from species data (independent of any fitted propagation output).
    R=8.314462618/.0289603;t=.2
    cp=1026.1074+t*(-208.00753+t*(479.58521+t*(-36.262009+t*-120.8045)))
    gamma=cp/(cp-R)
    nonlinear=[]
    for mode in ('sod','rarefaction'):
        left=(100000/(R*(100 if mode=='sod' else 150)),0 if mode=='sod' else -70,100000)
        right=(.125*left[0],0,10000) if mode=='sod' else (left[0],70,100000)
        for n in (200,400,800):
            cells=run(probe,mode=mode,n=n,rate=384000,duration=.001,boundary=1)
            (out/f'{mode}-{n}.csv').write_text('x,rho,u,p,T,c\n'+''.join(','.join(c.values())+'\n' for c in cells))
            errors=[0.,0.,0.]
            for cell in cells:
                exact=exact_riemann(float(cell['x']),.001,left,right,gamma)
                for j,key in enumerate(('rho','u','p')): errors[j]+=abs(float(cell[key])-exact[j])/n
            normalized=[errors[0]/left[0],errors[1]/math.sqrt(gamma*left[2]/left[0]),errors[2]/left[2]]
            nonlinear.append(dict(mode=mode,n=n,l1_relative=normalized))
    nonlinear_pass=all(max(r['l1_relative'])<.04 for r in nonlinear) and all(
        max([r for r in nonlinear if r['mode']==m][-1]['l1_relative']) <
        max([r for r in nonlinear if r['mode']==m][0]['l1_relative']) for m in ('sod','rarefaction'))
    failures=[r for r in rows if r['status']]
    summary=dict(wave_rows=len(rows),excluded_cfl=len(failures),alias_controls=sum(not r['representation_pass'] for r in rows),acoustic_pass=sum(r['acoustic_pass'] for r in rows),
                 acoustic_fail=sum(not r['acoustic_pass'] for r in rows if r['representation_pass'] and not r['status']),
                 max_balance=max(max(r['relative_balance']) for r in rows+contacts+finite+boundary if not r['status']),
                 contacts=contacts,finite_amplitude=finite,boundaries=boundary,nonlinear=nonlinear,nonlinear_pass=nonlinear_pass,
                 full_screen_pass=all(r['acoustic_pass'] for r in rows) and all(r['pressure_drift_Pa']<=1 for r in contacts) and nonlinear_pass,
                 decision='Do not promote unless every declared physical/acoustic gate passes; retain failures.')
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    return summary


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe',type=Path,required=True);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();print(json.dumps(experiment(args.probe,args.out),indent=2))
