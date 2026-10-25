#!/usr/bin/env python3
"""048 independent material-front/scattering/reversal/wall evidence matrix."""
import argparse,csv,hashlib,io,itertools,json,math,re,subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import numpy as np


def run(probe,out,name,mode='profile',n=768,rate=768000,U=0,kind=1,front=.7,length=2,amplitude=1,frequency=0,duration=None):
    # Independent reference speed from the declared initial state (not fitted travel).
    if duration is None:duration=.002
    command=[str(probe),mode,*map(str,(n,rate,U,kind,front,length,amplitude,frequency,duration))]
    r=subprocess.run(command,capture_output=True,text=True,timeout=300)
    (out/(name+'.csv')).write_text(r.stdout);(out/(name+'.stderr')).write_text(r.stderr)
    lines=[x[13:] for x in r.stderr.splitlines() if x.startswith('RB_MT_RESULT ')]
    if r.returncode or len(lines)!=1:raise ValueError((name,r.returncode,r.stderr[-300:]))
    report=json.loads(re.sub(r'\b(?:-?nan|-?inf)\b','null',lines[0]))
    report.update(command=command,mode=mode,n=n,rate=rate,U=U,kind=kind,front=front,length=length,amplitude=amplitude,frequency=frequency)
    report['probe_sha256']=hashlib.sha256(probe.read_bytes()).hexdigest()
    if not report['status']:
        scale=101325*.003*length+abs(report['initial_E_J'])
        report['energy_relative']=abs(report['energy_balance_J'])/scale
        report['momentum_relative']=abs(report['momentum_balance'])/(max(report['rho_left'],report['rho_right'])*.003*length*max(report['c_left'],report['c_right']))
        report['book_pass']=report['energy_relative']<=1e-10 and report['momentum_relative']<=1e-10
    (out/(name+'.json')).write_text(json.dumps(report,indent=2)+'\n')
    rows=list(csv.DictReader(io.StringIO(r.stdout))) if r.stdout.strip() else []
    data={k:np.array([float(x[k]) for x in rows]) for k in rows[0]} if rows else {}
    return report,data


def reference(x,dx,r):
    c1,c2=r['c_left'],r['c_right'];Z1,Z2=r['rho_left']*c1,r['rho_right']*c2
    reflection=(Z2-Z1)/(Z2+Z1);transmission=2*Z2/(Z1+Z2)
    center=.2;width=.04;t=r['time_s'];front=r['front'];U=r['U'];f=r['frequency'];amp=r['amplitude']
    def wave(x):
        d=(x-center)/width;return amp*np.exp(-d*d)*np.cos(2*np.pi*f*(x-center)/c1)
    nodes,weights=np.polynomial.legendre.leggauss(8)
    xx=np.asarray(x)[:,None]+np.asarray(dx)[:,None]*nodes/2-U*t
    p=np.where(xx<front,wave(xx-c1*t)+reflection*wave(2*front-xx-c1*t),transmission*wave(front+c1/c2*(xx-front)-c1*t))
    return np.sum(p*weights/2,axis=1)


def experiment(probe,out):
    out.mkdir(parents=True,exist_ok=True);speeds={}
    for kind in (0,1):
        r,_=run(probe,out,f'base-{kind}',mode='contact',n=32,kind=kind,duration=.00001)
        assert not r['status'];speeds[kind]=r['c_left']
    tasks=[]
    for kind,U,front,f in itertools.product((0,1),(-70,0,70),(.7,1.1),(0,200,1000,2000,4000)):
        duration=(front-.2)/speeds[kind]+.0006
        tasks.append((f'wave-{kind}-{U}-{front}-{f}',dict(kind=kind,U=U,front=front,frequency=f,duration=duration)))
    for n,rate in ((192,192000),(384,384000),(384,768000),(768,1536000)):
        for kind in (0,1):
            tasks.append((f'grid-{n}-{rate}-{kind}',dict(n=n,rate=rate,kind=kind,U=70,front=.7,frequency=2000,duration=(.7-.2)/speeds[kind]+.0006)))
    def evaluate(task):
        name,args=task;r,d=run(probe,out,name,**args)
        if not r['status']:
            p=reference(d['x_m'],d['dx_m'],r);measured=d['p_Pa']-101325
            r['waveform_l2']=float(np.sqrt(np.sum(d['dx_m']*(p-measured)**2)/np.sum(d['dx_m']*p*p)))
            r['gaussian_pass']=r['waveform_l2']<=.05
            if r['frequency']:
                packets=[]
                for side in (0,1):
                    at=d['x_m']-r['U']*r['time_s'];window=(at<r['front']) if side==0 else (at>=r['front'])
                    k=2*np.pi*r['frequency']/(r['c_right'] if side else -r['c_left'])
                    carrier=np.exp(-1j*k*d['x_m'][window]);weight=d['dx_m'][window]
                    ratio=np.sum(measured[window]*carrier*weight)/np.sum(p[window]*carrier*weight)
                    packets.append(dict(magnitude_db=float(20*np.log10(abs(ratio))),phase_deg=float(np.angle(ratio)*180/np.pi)))
                r['packets']=packets;r['carrier_pass']=all(abs(q['magnitude_db'])<=1 and abs(q['phase_deg'])<=10 for q in packets)
            np.savez_compressed(out/(name+'-reference.npz'),x=d['x_m'],reference=p,measured=measured)
        return r
    with ThreadPoolExecutor(max_workers=2) as pool:profiles=list(pool.map(evaluate,tasks))
    contacts=[]
    for kind,U,front in itertools.product((0,1),(-70,0,70),(.7,1.1)):
        r,d=run(probe,out,f'contact-{kind}-{U}-{front}',mode='contact',n=96,U=U,kind=kind,front=front,duration=.0006)
        if not r['status']:
            displacement=U*r['time_s'];donor=0 if U>0 else 1
            which=kind if donor==0 else 1-kind;y=np.array([1,0,0]) if which==0 else np.array([.1,.001,.899])
            density=r['rho_left'] if donor==0 else r['rho_right'];expected=-.003*displacement*density*y
            change=np.array(r['left_species'])-np.array(r['initial_left_species'])
            r['species_transfer_error_kg']=float(np.max(abs(change-expected)))
            r['pressure_drift_Pa']=float(np.max(abs(d['p_Pa']-101325)));r['velocity_drift_m_s']=float(np.max(abs(d['u_m_s']-U)))
            r['contact_pass']=r['species_transfer_error_kg']<=1e-10+1e-2*float(np.max(abs(expected))) and r['pressure_drift_Pa']<=1 and r['velocity_drift_m_s']<=.001
            # Actual partial-cell energy and measured moving-left pressure work,
            # compared to independently tabulated initial donor enthalpy.
            initial_e=(r['initial_left_internal_J']/sum(r['initial_left_species'])) if donor==0 else None
            if donor==1:
                # Fixed initial right material can be decoded independently from the
                # final invariant T and composition coefficients in047 reference.
                from gas_graph_reference import SimpleWave
                coeff_path=out.parent.parent/'stage-047/matrix-final/coefficients.csv'
                rows=list(csv.DictReader(coeff_path.open()));row=rows[0 if which==0 else 3]
                eos=dict(R=float(row['R']),k_lo=float(row['k_lo']),k_hi=float(row['k_hi']),lo=[float(row['l'+str(j)]) for j in range(5)],hi=[float(row['h'+str(j)]) for j in range(5)])
                initial_e=float(SimpleWave(eos,300 if which==0 else 900).energy(np.array(300 if which==0 else 900)))
            Rgas=101325/(density*(300 if which==0 else 900));h=initial_e+Rgas*(300 if which==0 else 900)
            expected_h=.003*displacement*density*(h+.5*U*U)
            dE=r['left_internal_J']-r['initial_left_internal_J']+.5*U*U*float(np.sum(change))
            actual_h=.003*101325*displacement-dE
            r['enthalpy_transfer_error_J']=abs(actual_h-expected_h)
            r['contact_pass'] &= r['enthalpy_transfer_error_J']<=1e-8+.01*abs(expected_h)
        contacts.append(r)
    specials=[]
    for mode,U,duration in (('reversal',70,.02),('outlet',0,.02)):
        r,d=run(probe,out,mode,mode=mode,n=192,rate=384000,U=U,amplitude=0,duration=duration)
        if not r['status']:
            if mode=='reversal':r['trajectory_error_m']=float(np.max(abs(d['contact_x_m']-(.7+70*np.sin(2*np.pi*100*d['time_s'])/(2*np.pi*100)))))
            r['reversal_pass']=r['port_velocity_min']<0 and r['port_velocity_max']>0 and r['ambient_inward_kg']>1e-12 and r.get('trajectory_error_m',0)<1e-5
        specials.append(r)
    positive,_=run(probe,out,"outlet-positive",mode="outlet-positive",n=192,rate=384000,U=0,amplitude=0,duration=.02)
    walls=[]
    for n,rate in ((96,384000),(192,384000),(384,384000),(384,768000)):
        r,d=run(probe,out,f'wall-{n}-{rate}',mode='wall',n=n,rate=rate,U=70,amplitude=0,duration=.002)
        walls.append(r)
    stable=[]
    for U in (0,70):
        r,d=run(probe,out,f'stability-{U}',mode='stability',n=64,rate=384000,U=U,kind=1,front=.23,length=.4,amplitude=1,frequency=4000,duration=1)
        if not r['status']:
            r['maximum_ratio']=r['maximum_PE']/r['initial_PE'];r['final_ratio']=r['final_PE']/r['initial_PE']
            r['stability_pass']=r['maximum_ratio']<=1.02 and r['final_ratio']<.001
        stable.append(r)
    unsupported,_=run(probe,out,'unsupported-clock',n=768,rate=192000,duration=.00001)
    primary=profiles[:60];allrows=profiles+contacts+specials+walls+stable+[positive]
    result=dict(schema=1,profiles=profiles,contacts=contacts,reversals=specials,walls=walls,stability=stable,unsupported=unsupported,positive_outlet_control=positive)
    result.update(primary_pass=all(not r['status'] and (r.get('carrier_pass',False) if r['frequency'] else r.get('gaussian_pass',False)) for r in primary),
                  contact_pass=all(r.get('contact_pass',False) for r in contacts),reversal_pass=all(r.get('reversal_pass',False) for r in specials),
                  wall_pass=all(not r['status'] and r['book_pass'] for r in walls) and len({r['wall_capacity'] for r in walls})==1,
                  stability_pass=all(r.get('stability_pass',False) for r in stable),
                  conservation_pass=all(not r['status'] and r['book_pass'] for r in allrows),guard_pass=unsupported['status']!=0)
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    r=experiment(a.probe,a.out);print({k:v for k,v in r.items() if k.endswith('_pass')})
