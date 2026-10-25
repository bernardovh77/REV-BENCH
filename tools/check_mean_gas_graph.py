#!/usr/bin/env python3
"""046 frozen signed-flow continuum, port timing, contact and work gates."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import itertools
import json
import math
from pathlib import Path
import numpy as np
from check_gas_graph import run,balance
from gas_graph_reference import SimpleWave


def experiment(probe,out,long=True,baseline=None):
    out.mkdir(parents=True,exist_ok=True);eos={};c0={}
    for T,mix in itertools.product((300,900),(0,1)):
        e,_=run(probe,out,f'eos-{T}-{mix}',mode='eos',T=T,mix=mix)
        eos[T,mix]=e;c0[T,mix]=SimpleWave(e,T).c0
    tasks=[]
    for U,T,mix,cut in itertools.product((-70,0,70),(300,900),(0,1),(.313,.557)):
        c=c0[T,mix];n=math.floor(cut*96000/c)
        for amp in (1e-5,.001,.01):
            tasks.append((f'pulse-{U}-{T}-{mix}-{cut}-{amp}',dict(mean_flow=U,T=T,mix=mix,cut=cut,n=n,amplitude=amp,duration=1/(c+U))))
        for f in (200,2000,4000):
            tasks.append((f'carrier-{U}-{T}-{mix}-{cut}-{f}',dict(mean_flow=U,T=T,mix=mix,cut=cut,n=n,amplitude=1e-5,frequency=f,duration=1/(c+U))))
    for U,rate,cut in itertools.product((-70,70),(96000,384000),(.313,.557)):
        tasks.append((f'clock-{U}-{rate}-{cut}',dict(mean_flow=U,rate=rate,cut=cut,n=math.floor(cut*96000/c0[300,0]),duration=1/(c0[300,0]+U))))
    for U,factor in itertools.product((-70,70),(.5,1.5)):
        tasks.append((f'spatial-{U}-{factor}',dict(mean_flow=U,n=round(86*factor),duration=1/(c0[300,0]+U))))
    def evaluate(task):
        name,args=task;r,d=run(probe,out,name,**args)
        if not r['status']:
            wave=SimpleWave(eos[r['temperature'],r['mixture']],r['temperature'],amplitude=r['amplitude'],frequency=r['frequency'],mean_flow=r['mean_flow'])
            p=wave.pressure_of_average(d['x_m'],d['dx_m'],r['time_s'])-101325;measured=d['p_Pa']-101325
            r['waveform_l2']=float(np.sqrt(np.sum(d['dx_m']*(measured-p)**2)/np.sum(d['dx_m']*p*p)))
            r['relative_balance']=balance(r);r['waveform_pass']=r['waveform_l2']<=.02
            if r['frequency']:
                carrier=np.exp(-2j*np.pi*r['frequency']*d['x_m']/(wave.c0+wave.U))
                ratio=np.sum(measured*carrier*d['dx_m'])/np.sum(p*carrier*d['dx_m'])
                r['magnitude_db']=float(20*np.log10(abs(ratio)));r['phase_deg']=float(np.angle(ratio)*180/np.pi)
                r['carrier_pass']=abs(r['magnitude_db'])<=1 and abs(r['phase_deg'])<=10
            if not r['mean_flow'] and baseline:
                old,od=run(baseline,out,name+'-045',**{k:v for k,v in args.items() if k!='mean_flow'})
                r['zero_flow_max_pressure_difference_Pa']=float(np.max(abs(od['p_Pa']-d['p_Pa'])))
                r['zero_flow_parity_pass']=not old['status'] and r['zero_flow_max_pressure_difference_Pa']<1e-7
            np.savez_compressed(out/(name+'-reference.npz'),x=d['x_m'],reference=p,measured=measured)
        return r
    with ThreadPoolExecutor(max_workers=2) as pool:profiles=list(pool.map(evaluate,tasks))
    returns=[]
    for U,cut,R in itertools.product((-70,0,70),(.313,.557),(-1,0,1)):
        n=math.floor(cut*96000/c0[300,0]);r,d=run(probe,out,f'return-{U}-{cut}-{R}',mode='return',mean_flow=U,cut=cut,n=n,amplitude=1e-5,duration=.014,reflection=R)
        if not r['status']:
            c=r['c'];x=(math.floor(.15/(cut/n))+.5)*cut/n
            forward=(r['length']-.1)/(c+U);backward=forward+(r['length']-x)/(c-U)
            window=abs(d['time_s']-forward)<3*.035/(c+U)
            k=np.argmax(abs(d['terminal_incident_Pa'][window]));r['forward_tick_error']=float((d['time_s'][window][k]-forward)*r['rate'])
            window=abs(d['time_s']-backward)<3*.035/(c-U);values=d['probe_pressure_Pa'][window];times=d['time_s'][window];k=np.argmax(abs(values))
            r['return_peak_Pa']=float(values[k]);r['return_tick_error']=float((times[k]-backward)*r['rate'])
            r['return_pass']=bool(abs(r['forward_tick_error'])<=5 and (abs(values[k])<.01 if R==0 else values[k]*R>0 and abs(r['return_tick_error'])<=5))
            r['relative_balance']=balance(r)
        returns.append(r)
    contacts=[]
    for U,T,cut in itertools.product((-70,70),(300,900),(.313,.557)):
        n=math.floor(cut*96000/c0[T,1]);r,d=run(probe,out,f'contact-{U}-{T}-{cut}',mode='advect',mean_flow=U,T=T,mix=1,cut=cut,n=n,amplitude=0,duration=.0006)
        if not r['status']:
            passed=np.clip(abs(U)*r['time_s']-.02,0,.04);expected=np.sign(U)*.003*passed*(np.array(r['patch_state'])[:3]-np.array(r['base_state'])[:3])
            excess=np.array(r['interface_transfer'])[:3]-.003*r['time_s']*U*np.array(r['base_state'])[:3]
            r['contrast_transfer_relative_error']=float(np.max(abs(excess-expected)/abs(expected)))
            r['pressure_drift_Pa']=float(np.max(abs(d['p_Pa']-101325)))
            r['velocity_drift_m_s']=float(np.max(abs(d['u_m_s']-U)))
            r['relative_balance']=balance(r)
            r['contact_pass']=r['contrast_transfer_relative_error']<=.05 and r['pressure_drift_Pa']<=1 and r['velocity_drift_m_s']<=.001
        contacts.append(r)
    stable=[]
    if long:
        for U,R in itertools.product((-70,70),(-1,0,1)):
            r,_=run(probe,out,f'stability-{U}-{R}',mode='stability',mean_flow=U,length=.065317,cut=.01831,n=8,amplitude=1e-5,duration=1,reflection=R)
            if not r['status']:
                r['maximum_ratio']=r['maximum_perturbation_J']/r['initial_perturbation_J'];r['final_ratio']=r['final_perturbation_J']/r['initial_perturbation_J']
                r['maximum_ratio_limit']=1.10*max(1,R*R*(r['c']-U)/(r['c']+U))
                r['stability_pass']=r['maximum_ratio']<=r['maximum_ratio_limit'] and r['final_ratio']<.001
                r['relative_balance']=balance(r)
            stable.append(r)
    allrows=profiles+returns+contacts+stable
    summary=dict(schema=1,profiles=profiles,returns=returns,contacts=contacts,stability=stable)
    summary.update(gaussian_pass=all(r.get('waveform_pass',False) for r in profiles if not r['frequency']),
                   carrier_pass=all(r.get('carrier_pass',False) for r in profiles if r['frequency']),
                   return_pass=all(r.get('return_pass',False) for r in returns),contact_pass=all(r.get('contact_pass',False) for r in contacts),
                   stability_pass=long and all(r.get('stability_pass',False) for r in stable),
                   zero_flow_parity_pass=bool(baseline) and all(r.get('zero_flow_parity_pass',False) for r in profiles if not r.get('mean_flow')),
                   conservation_pass=all(not r['status'] and r['relative_balance']<=1e-10 and r['clock_error']==0 and abs(r['graph_work_residual_J'])<=1e-10*max(1e-20,r['initial_perturbation_J']) for r in allrows))
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');return summary

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--baseline',type=Path);p.add_argument('--skip-long',action='store_true');a=p.parse_args()
    s=experiment(a.probe,a.out,not a.skip_long,a.baseline);print(json.dumps({k:v for k,v in s.items() if k.endswith('_pass')},indent=2))
