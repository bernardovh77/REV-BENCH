#!/usr/bin/env python3
"""114 independent sharp-interface references, fixed-port books and limits."""
import argparse,csv,hashlib,io,itertools,json,re,subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import numpy as np
from check_material import reference

def run(probe,out,name,mode='profile',n=2048,rate=2000000,U=0,kind=0,front=.7,length=2,amplitude=1,frequency=0,duration=.002):
    command=[str(probe),mode,*map(str,(n,rate,U,kind,front,length,amplitude,frequency,duration))]
    raw=subprocess.run(command,capture_output=True,text=True,timeout=600)
    (out/(name+'.csv')).write_text(raw.stdout);(out/(name+'.stderr')).write_text(raw.stderr)
    lines=[x[len('RB_FT_RESULT '):] for x in raw.stderr.splitlines() if x.startswith('RB_FT_RESULT ')]
    if raw.returncode or len(lines)!=1:raise ValueError((name,raw.returncode,raw.stderr[-500:]))
    r=json.loads(re.sub(r'\b(?:-?nan|-?inf)\b','null',lines[0]))
    r.update(command=command,mode=mode,n=n,rate=rate,U=U,kind=kind,front=front,length=length,amplitude=amplitude,frequency=frequency)
    r['probe_sha256']=hashlib.sha256(probe.read_bytes()).hexdigest()
    if not r['status']:
        mass=max(r['rho_left'],r['rho_right'])*.003*length
        Es=101325*.003*length+abs(r['initial_E_J']);Ps=mass*max(r['c_left'],r['c_right'])
        r['energy_relative']=max(abs(r['energy_balance_J']),abs(r['total_energy_balance_J']),abs(r['regional_energy_balance_J']))/Es
        r['momentum_relative']=abs(r['momentum_balance'])/Ps
        r['mass_relative']=max(map(abs,r['mass_balance']+r['regional_mass_balance']))/mass
        r['book_pass']=max(r['energy_relative'],r['momentum_relative'],r['mass_relative'])<=1e-10
    data={}
    rows=list(csv.DictReader(io.StringIO(raw.stdout))) if raw.stdout.strip() else []
    if rows:data={k:np.array([float(x[k]) for x in rows]) for k in rows[0]}
    return r,data

def wave(probe,out,name,args):
    r,d=run(probe,out,name,**args)
    if not r['status']:
        expected=reference(d['x_m'],d['dx_m'],r);actual=d['p_Pa']-101325;weights=d['dx_m']
        r['waveform_l2']=float(np.sqrt(np.sum(weights*(expected-actual)**2)/np.sum(weights*expected**2)))
        r['packet_pass']=r['waveform_l2']<=.05
        if r['frequency']:
            packets=[]
            for side in (0,1):
                at=d['x_m']-r['U']*r['time_s'];mask=at<r['front'] if side==0 else at>=r['front']
                k=2*np.pi*r['frequency']/(r['c_right'] if side else -r['c_left'])
                v=np.exp(-1j*k*d['x_m'][mask])*weights[mask]
                ratio=np.sum(actual[mask]*v)/np.sum(expected[mask]*v)
                packets.append({'magnitude_db':float(20*np.log10(abs(ratio))),'phase_deg':float(np.angle(ratio)*180/np.pi)})
            r['packets']=packets;r['carrier_pass']=all(abs(p['magnitude_db'])<=1 and abs(p['phase_deg'])<=10 for p in packets)
        else:r['carrier_pass']=True
        np.savez_compressed(out/(name+'-reference.npz'),x=d['x_m'],reference=expected,measured=actual)
    (out/(name+'.json')).write_text(json.dumps(r,indent=2)+'\n');return r

def qualify_enthalpy(result,properties):
    """Use the independently audited047 property export, not observed flux."""
    rows=list(csv.DictReader(properties.open()))
    h=[next(float(r['h']) for r in rows if int(r['mixture'])==k and float(r['T_K'])==T) for k,T in [(0,300),(3,900)]]
    for r in result['transport']:
        if r['status']:continue
        donor=0 if r['U']>0 else 1
        density=r['rho_left'] if donor==0 else r['rho_right']
        expected=.003*r['U']*density*(h[donor]+.5*r['U']**2)*r['time_s']
        actual=r['left_energy_J'] if donor==0 else r['right_energy_J']
        r['inlet_total_enthalpy_error']=abs(actual-expected)/abs(expected)
    result['enthalpy_gate']=all(not r['status'] and r['inlet_total_enthalpy_error']<=.01 for r in result['transport'])
    result['properties_reference']={'path':str(properties),'sha256':hashlib.sha256(properties.read_bytes()).hexdigest(),'enthalpy_J_kg':h}
    result['pass']&=result['enthalpy_gate']
    return result

def evaluate(probe,out,properties):
    out.mkdir(parents=True,exist_ok=True)
    bases={k:run(probe,out,f'base-{k}',mode='contact',n=16,rate=32000,kind=k,duration=1/32000)[0] for k in (0,1)}
    tasks=[]
    for kind,U,front,f in itertools.product((0,1),(-70,0,70),(.7,1.1),(0,200,1000,2000,4000)):
        tasks.append((f'wave-{kind}-{U}-{front}-{f}',dict(kind=kind,U=U,front=front,frequency=f,duration=(front-.2)/bases[kind]['c_left']+.0006)))
    with ThreadPoolExecutor(max_workers=2) as pool:
        profiles=list(pool.map(lambda t:wave(probe,out,*t),tasks))
    # Freeze/retain coarse failures; include both thermal orders and independent clocks.
    grids=[]
    for kind,n,rate in itertools.product((0,1),(512,1024),(1000000,2000000)):
        grids.append(wave(probe,out,f'grid-{kind}-{n}-{rate}',dict(kind=kind,n=n,rate=rate,U=70,front=1.1,frequency=4000,duration=.9/bases[kind]['c_left']+.0006)))
    fine=[]
    for r in profiles:
        if r.get('status') or not r.get('packet_pass') or not r.get('carrier_pass'):
            args={k:r[k] for k in ['U','kind','front','frequency']};args.update(n=4096,rate=4000000,duration=(r['front']-.2)/bases[r['kind']]['c_left']+.0006)
            fine.append(wave(probe,out,f"fine-{r['kind']}-{r['U']}-{r['front']}-{r['frequency']}",args))
    contacts=[]
    for kind,U in itertools.product((0,1),(-70,0,70)):
        r,d=run(probe,out,f'contact-{kind}-{U}',mode='contact',kind=kind,U=U,n=128,rate=256000,front=.5,length=1,amplitude=0,duration=.001)
        r['contact_pass']=not r['status'] and r['max_pressure_error']<=1 and r['max_velocity_error']<=.001 and r['book_pass'];contacts.append(r)
    transport=[]
    for U in (-70,70):
        for n in (64,128):
            r,d=run(probe,out,f'through-{U}-{n}',mode='throughflow',n=n,rate=256000,U=U,front=.5,length=1,amplitude=0,duration=4/70)
            if not r['status']:
                initial=np.array(r['initial_mass']);density=[r['rho_left'],r['rho_right']]
                expected=initial.copy()
                # Four residence times: donor fills the entire finite duct;
                # injected partial masses and final inventory provide independent tests.
                donor=0 if U>0 else 1
                expected[:]=0;expected[donor]=.003*density[donor]
                r['inventory_error']=float(np.max(abs(np.array(r['final_mass'])-expected))/(.003*max(density)))
                incoming=np.array(r['left_mass'] if U>0 else r['right_mass'])
                target=np.zeros(2);target[donor]=U*.003*density[donor]*r['time_s']
                r['inlet_mass_error']=float(np.max(abs(incoming-target)) / max(abs(target)))
                r['transport_pass']=max(r['inventory_error'],r['inlet_mass_error'])<=.01 and r['book_pass']
            transport.append(r)
    extras=[]
    for mode,n,rate,U,duration in [('outlet',128,256000,0,.02),('wall',64,256000,0,.002),('wall',128,256000,0,.002),('wall',128,512000,0,.002),('stability',64,256000,0,1),('stability',64,256000,70,1)]:
        r,d=run(probe,out,f'{mode}-{n}-{rate}-{U}',mode=mode,n=n,rate=rate,U=U,kind=1,front=.5,length=1,amplitude=.1,frequency=4000,duration=duration)
        if not r['status']:
            r['extra_pass']=r['book_pass']
            if mode=='outlet':r['extra_pass']&=r['port_mass_min']<0<r['port_mass_max'] and r['max_cold_kg']>1e-9
            if mode=='stability':
                r['peak_PE_ratio']=r['maximum_PE']/r['initial_PE'];r['final_PE_ratio']=r['final_PE']/r['initial_PE']
                r['extra_pass']&=r['peak_PE_ratio']<=1.02 and r['final_PE_ratio']<.001
        extras.append(r)
    bad,_=run(probe,out,'unsupported-cfl',mode='contact',n=2048,rate=192000,duration=1/192000)
    costs=[]
    for _ in range(3):
        for U in (0,70):
            r,_=run(probe,out,f'cost-{U}-{_}',mode='cost',n=256,rate=512000,U=U,kind=1,front=.5,length=1,amplitude=.1,frequency=4000,duration=.01);r['simulated_cpu_factor']=r['time_s']/r['cpu_s'];costs.append(r)
    selected=all(not r['status'] and r['book_pass'] and r['packet_pass'] and r['carrier_pass'] for r in profiles if not any(all(r[k]==f[k] for k in ['U','kind','front','frequency']) for f in fine)) and all(not f['status'] and f['book_pass'] and f['packet_pass'] and f['carrier_pass'] for f in fine)
    result={'profiles':profiles,'grids':grids,'fine':fine,'contacts':contacts,'transport':transport,'extras':extras,'costs':costs,'unsupported':bad,'wave_gate':selected,'contact_gate':all(r['contact_pass'] for r in contacts),'transport_gate':all(r.get('transport_pass',False) for r in transport),'extras_gate':all(r.get('extra_pass',False) for r in extras),'unsupported_gate':bad['status']!=0}
    result['pass']=all(result[k] for k in ['wave_gate','contact_gate','transport_gate','extras_gate','unsupported_gate'])
    qualify_enthalpy(result,properties)
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--properties',type=Path,required=True);a=p.parse_args()
    r=evaluate(a.probe,a.out,a.properties);print({k:v for k,v in r.items() if k.endswith('gate') or k=='pass'})
