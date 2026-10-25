#!/usr/bin/env python3
"""045 independent simple-wave/phase/return/conservation and stability matrix."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import csv
import hashlib
import io
import itertools
import json
import math
from pathlib import Path
import subprocess
import numpy as np
from gas_graph_reference import SimpleWave


def run(probe,out,name,*,mode='profile',rate=192000,T=300,mix=0,length=1.50317,
        cut=.313,n=86,amplitude=.001,frequency=0,duration=.003,reflection=0,area=.003,mean_flow=None):
    args=[str(probe),mode,*map(str,(rate,T,mix,length,cut,n,amplitude,frequency,duration,reflection))]
    args.append(str(area))
    if mean_flow is not None:args.append(str(mean_flow))
    r=subprocess.run(args,text=True,capture_output=True,timeout=300)
    (out/(name+'.csv')).write_text(r.stdout)
    records=[line.removeprefix('RB_GG_RESULT ') for line in r.stderr.splitlines() if line.startswith('RB_GG_RESULT ')]
    if r.returncode or len(records)!=1:raise ValueError(f'Probe execution failed {args}: {r.stderr[-600:]}')
    report=json.loads(records[0]);report.update(command=args,mode=mode,temperature=T,mixture=mix,
        length=length,cut=cut,amplitude=amplitude,frequency=frequency,rate=rate,reflection=reflection)
    report['probe_sha256']=hashlib.sha256(Path(probe).read_bytes()).hexdigest()
    (out/(name+'.json')).write_text(json.dumps(report,indent=2)+'\n')
    values=list(csv.DictReader(io.StringIO(r.stdout))) if r.stdout.strip() else []
    data={key:np.array([float(row[key]) for row in values]) for key in values[0]} if values else {}
    return report,data


def balance(report):
    total=report['rho']*report.get('area_m2',.003)*report['length'];c=report['c']
    scales=[total,total,total,total*c,total*c*c]
    return max(abs(value)/scale for side in ('left_balance','right_balance') for value,scale in zip(report[side],scales))


def experiment(probe,out,long=True,native_contract=None):
    out.mkdir(parents=True,exist_ok=True);eos={};c0={}
    for T,mix in itertools.product((300,900),(0,1)):
        e,_=run(probe,out,f'eos-{T}-{mix}',mode='eos',T=T,mix=mix)
        eos[T,mix]=e;c0[T,mix]=SimpleWave(e,T).c0
    tasks=[]
    for T,mix,cut,amp in itertools.product((300,900),(0,1),(.313,.557),(1e-5,.001,.01)):
        n=max(4,math.floor(cut*96000/c0[T,mix]));name=f'pulse-{T}-{mix}-{cut}-{amp}'
        tasks.append((name,dict(T=T,mix=mix,cut=cut,n=n,amplitude=amp,duration=1/c0[T,mix])))
    for T,mix,cut,f in itertools.product((300,900),(0,1),(.313,.557),(200,1000,2000,4000)):
        n=max(4,math.floor(cut*96000/c0[T,mix]));name=f'carrier-{T}-{mix}-{cut}-{f}'
        tasks.append((name,dict(T=T,mix=mix,cut=cut,n=n,amplitude=1e-5,frequency=f,duration=1/c0[T,mix])))
    for rate in (96000,384000):
        for cut in (.313,.557):
            tasks.append((f'clock-{rate}-{cut}',dict(rate=rate,cut=cut,n=math.floor(cut*96000/c0[300,0]),duration=1/c0[300,0])))
    for factor in (.5,1.5):
        tasks.append((f'spatial-{factor}',dict(n=round(86*factor),duration=1/c0[300,0])))
    profiles=[]
    def evaluate(task):
        name,args=task;r,d=run(probe,out,name,**args)
        if not r['status']:
            reference=SimpleWave(eos[r['temperature'],r['mixture']],r['temperature'],amplitude=r['amplitude'],frequency=r['frequency'])
            p=reference.pressure_of_average(d['x_m'],d['dx_m'],r['time_s'])-101325
            measured=d['p_Pa']-101325
            norm=float(np.sum(d['dx_m']*p*p))
            r['waveform_l2']=float(np.sqrt(np.sum(d['dx_m']*(measured-p)**2)/norm))
            r['relative_balance']=balance(r);r['waveform_pass']=r['waveform_l2']<=.02
            if r['frequency']:
                carrier=np.exp(-2j*np.pi*r['frequency']*d['x_m']/reference.c0)
                ratio=np.sum(measured*carrier*d['dx_m'])/np.sum(p*carrier*d['dx_m'])
                r['magnitude_db']=float(20*np.log10(abs(ratio)));r['phase_deg']=float(np.angle(ratio)*180/np.pi)
                r['carrier_pass']=abs(r['magnitude_db'])<=1 and abs(r['phase_deg'])<=10
            np.savez_compressed(out/(name+'-reference.npz'),x=d['x_m'],reference=p,measured=measured)
        return r
    with ThreadPoolExecutor(max_workers=2) as pool:profiles=list(pool.map(evaluate,tasks))
    returns=[]
    for cut,R in itertools.product((.313,.557),(-1,0,1)):
        r,d=run(probe,out,f'return-{cut}-{R}',mode='return',cut=cut,n=math.floor(cut*96000/c0[300,0]),duration=.010,reflection=R)
        if not r['status']:
            probe_x=(math.floor(.15/(cut/math.floor(cut*96000/c0[300,0])))+.5)*(cut/math.floor(cut*96000/c0[300,0]))
            expected=(2*r['length']-.1-probe_x)/r['c'];window=abs(d['time_s']-expected)<3*.035/r['c']
            values=d['probe_pressure_Pa'][window];times=d['time_s'][window]
            peak=int(np.argmax(abs(values)));r['return_peak_Pa']=float(values[peak]);r['return_tick_error']=float((times[peak]-expected)*r['rate'])
            r['return_pass']=bool(abs(values[peak])<.1 if R==0 else values[peak]*R>0 and abs(r['return_tick_error'])<=5)
            r['relative_balance']=balance(r)
        returns.append(r)
    adverse=[]
    for amp in (.05,.1):
        r,_=run(probe,out,f'adverse-{amp}',amplitude=amp);adverse.append(r)
    contact,_=run(probe,out,'contact-pulse',mode='contact',mix=1,n=math.floor(.313*96000/c0[300,1]),duration=.004)
    if not contact['status']:contact['relative_balance']=balance(contact)
    stable=[]
    if long:
        for R in (-1,0,1):
            r,_=run(probe,out,f'stability-{R}',mode='stability',length=.065317,cut=.01831,n=8,
                    amplitude=1e-5,duration=1,reflection=R)
            if not r['status']:
                r['maximum_ratio']=r['maximum_perturbation_J']/r['initial_perturbation_J']
                r['final_ratio']=r['final_perturbation_J']/r['initial_perturbation_J']
                r['stability_pass']=r['maximum_ratio']<=1.02 and r['final_ratio']<.001;r['relative_balance']=balance(r)
            stable.append(r)
    components=[]
    if native_contract:
        from build_contract import ROOT,loads
        from exhaust_geometry import compile_exhaust
        for build in loads((ROOT/'contracts/examples/builds-v1.json').read_text()):
            plan=compile_exhaust(build,native_contract).manifest()
            for kind in ('exhaust','tailpipe'):
                component=next(c for c in plan['components'] if c['kind']==kind)
                r,d=run(probe,out,'component-'+component['id'].replace('/','-'),mode='return',mix=1,
                    length=.313+component['length_m'],cut=.313,n=math.floor(.313*96000/c0[300,1]),
                    area=component['area_m2'],reflection=1,duration=.012)
                r.update(component_id=component['id'],build_id=build['build_id'],
                         isolated_component_control=True,component_ports=component['ports'])
                if not r['status']:
                    r['relative_balance']=balance(r)
                    r['return_seen']=bool(np.max(abs(d['received_Pa']))>.1)
                components.append(r)
    summary=dict(schema=1,profiles=profiles,returns=returns,adverse=adverse,contact=contact,stability=stable,components=components)
    summary['physical_pass']=all(not r['status'] and r.get('waveform_pass') and r['relative_balance']<=1e-10 and r['clock_error']==0 for r in profiles)
    summary['carrier_pass']=all(r.get('carrier_pass',False) for r in profiles if r['frequency'])
    summary['return_pass']=all(r.get('return_pass',False) for r in returns)
    summary['stability_pass']=long and all(r.get('stability_pass',False) for r in stable)
    summary['component_pass']=bool(components) and all(not r['status'] and r['relative_balance']<=1e-10 and r['return_seen'] for r in components)
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');return summary


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--skip-long',action='store_true');p.add_argument('--native-contract',type=Path)
    a=p.parse_args();r=experiment(a.probe,a.out,not a.skip_long,a.native_contract)
    print(json.dumps({k:v for k,v in r.items() if k.endswith('_pass')},indent=2))
