#!/usr/bin/env python3
"""047 independently audit properties, actual fixed-wall operator and finite wall."""
import argparse,csv,io,json,math,subprocess,hashlib
from pathlib import Path
import numpy as np
from gas_graph_reference import SimpleWave


def nasa(T,mix):
    data=json.loads(Path(__file__).with_name('thermal_reference.json').read_text())['species']
    air={'O2':.2095,'N2':.7808,'Ar':.0093}
    fuel={'C8H18,isooctane':.4785,'C7H16,n-heptane':.15,'C7H8':.3715}
    oxygen=7.4785+13.985/4
    burned={'CO2':7.4785,'H2O':13.985/2,'N2':oxygen*.7808/.2095,'Ar':oxygen*.0093/.2095}
    mol=(air,fuel,burned)[mix];mass={'O2':.0319988,'N2':.0280134,'Ar':.039948,'CO2':.0440095,'H2O':.01801528,'C8H18,isooctane':.11422852,'C7H16,n-heptane':.10020194,'C7H8':.09213842}
    R=8.314462618*sum(mol.values())/sum(n*mass[k] for k,n in mol.items())
    cp=0
    for k,n in mol.items():
        row=data[k];a=row['coefficients'][0 if len(row['coefficients'])==1 or T<=row['ranges'][1] else 1]
        cp+=8.314462618*n*sum(a[i]*T**i for i in range(5))
    cp/=sum(n*mass[k] for k,n in mol.items())
    return cp,math.sqrt(cp/(cp-R)*R*T)


def evaluate(probe,out):
    out.mkdir(parents=True,exist_ok=True)
    def run(name,args):
        r=subprocess.run([str(probe),*map(str,args)],capture_output=True,text=True,check=True)
        (out/(name+'.csv')).write_text(r.stdout)
        return [{k:float(v) for k,v in row.items()} for row in csv.DictReader(io.StringIO(r.stdout))]
    props=run('properties',['properties']);coeff=run('coefficients',['coefficients']);fixed=run('fixed-wall',['fixed'])
    errors=[];comparisons=[];jumps=[]
    for r in props:
        cp,cv,R,T=r['cp'],r['cv'],r['R'],r['T_K'];assert cv>0
        assert abs(cp-cv-R)<1e-10 and abs(r['h']-r['u']-R*T)<1e-8
        err=max(abs(r[k]/T-1) for k in ('T_back_20','T_back_600','T_back_4000'));errors.append(err);assert err<=1e-8
        if 200<=T<=4000 and r['mixture']<3:
            ncp,nc=nasa(T,int(r['mixture']));comparisons.append(dict(mixture=int(r['mixture']),T=T,cp_error=cp/ncp-1,c_error=r['c']/nc-1))
    sound_errors=[]
    for k,row in enumerate(coeff):
        eos=dict(R=row['R'],k_lo=row['k_lo'],k_hi=row['k_hi'],lo=[row['l'+str(j)] for j in range(5)],hi=[row['h'+str(j)] for j in range(5)])
        cvlow=sum(eos['lo']);cvhigh=sum(eos['hi']);R=eos['R']
        jumps.append(dict(mixture=k,cp_fraction=(cvhigh-cvlow)/(cvlow+R),c_fraction=math.sqrt((1+R/cvhigh)/(1+R/cvlow))-1))
        for T in (300,600,900,1200,1800,2500,3500):
            wave=SimpleWave(eos,T,amplitude=1e-5);wave.width=1e9
            # Independently integrated constant-composition entropy curve.
            p,temperature,u=wave.initial(np.array([.1]));rho=p/(R*temperature);rho0=101325/(R*T)
            c=math.sqrt(float((p[0]-101325)/(rho[0]-rho0)))
            expected=float(wave.c0);sound_errors.append(abs(c/expected-1));assert sound_errors[-1]<=1e-5
    for r in fixed:
        assert abs(r['heat_J']-r['expected_heat_J'])<=1e-10*max(1,abs(r['heat_J']))
        assert abs(r['energy_residual_J'])<=1e-10*max(1,abs(r['heat_J'])) and r['wall_after_K']==r['wall_K']
    pairs=[];reference_errors=[]
    # Independent RK4 integration of Tg/Tw with exported polynomial cv.
    def cv_at(T,mix):
        row=coeff[3 if mix else 0];a=[row[('l' if T<=1000 else 'h')+str(j)] for j in range(5)]
        return sum(a[j]*(T/1000)**j for j in range(5))
    def reference(dt,duration,T,Tw,C,Hi,Ho,mix):
        def rhs(a,b):return Hi*(b-a)/(.003*cv_at(a,mix)),(Hi*(a-b)+Ho*(300-b))/C
        for _ in range(round(duration/dt)):
            a,b=rhs(T,Tw);c,d=rhs(T+dt*a/2,Tw+dt*b/2);e,f=rhs(T+dt*c/2,Tw+dt*d/2);g,h=rhs(T+dt*e,Tw+dt*f)
            T+=dt*(a+2*c+2*e+g)/6;Tw+=dt*(b+2*d+2*f+h)/6
        return T,Tw
    for mix in (0,1):
        C=math.pi*.001*.061*7800*500;Hi=200*math.pi*.06;Ho=40*math.pi*.062
        ref=reference(1e-6,.005,900,650,C,Hi,Ho,mix)
        for dt in (4e-6,2e-6,1e-6):
            name=f'pair-reference-{mix}-{dt}';d=run(name,['pair',900,650,C,Hi,Ho,dt,.005,mix]);last=d[-1]
            err=max(abs(last['gas_K']-ref[0]),abs(last['wall_K']-ref[1]));reference_errors.append(err)
            pairs.append(dict(name=name,error_K=err,dt=dt,max_balance=max(abs(r['balance_J']) for r in d)))
        assert pairs[-1]['error_K']<=.001 and pairs[-3]['error_K']>pairs[-2]['error_K']>pairs[-1]['error_K']
    sensitivities=[]
    for thickness in (.001,.002):
        for hi in (50,200):
            for ho in (10,40):
                C=math.pi*thickness*(.06+thickness)*7800*500;Hi=hi*math.pi*.06;Ho=ho*math.pi*(.06+2*thickness)
                d=run(f'wall-{thickness}-{hi}-{ho}',['pair',900,650,C,Hi,Ho,.01,10,1])
                sensitivities.append(dict(thickness_m=thickness,h_inner=hi,h_outer=ho,capacity_J_K=C,Bi=max(hi,ho)*thickness/16,gas_time_s=.003*cv_at(900,1)/Hi,wall_time_s=C/(Hi+Ho),final=d[-1],max_balance=max(abs(r['balance_J']) for r in d)))
                assert max(abs(r['balance_J']) for r in d)<=1e-10*C*900
    air=[r for r in comparisons if r['mixture']==0];burned=[r for r in comparisons if r['mixture']==2 and r['T']<=1000]
    result=dict(schema=1,property_rows=len(props),max_inversion_relative=max(errors),max_isentropic_speed_relative=max(sound_errors),joins=jumps,nasa=comparisons,fixed_wall_rows=len(fixed),pairs=pairs,sensitivities=sensitivities,
                air_reference_pass=max(abs(r['cp_error']) for r in air)<=.005 and max(abs(r['c_error']) for r in air)<=.002,
                low_burned_reference_pass=max(abs(r['cp_error']) for r in burned)<=.01 and max(abs(r['c_error']) for r in burned)<=.005,
                internal_pass=True,wall_pass=True,probe_sha256=hashlib.sha256(probe.read_bytes()).hexdigest())
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    r=evaluate(a.probe,a.out);print({k:v for k,v in r.items() if k.endswith('_pass')})
