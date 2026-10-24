"""Stage042 native fueling/source evidence; no automatic acoustic acceptance."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import subprocess

PRESETS=('i4','i6','v8-cross','v8-flat','boxer4')
SEEDS=(17,29,61)
SCENARIOS=('loaded','startup-restart','afr-toggle','dfco-resume',
           'hard-cut-resume','soft-cut','between-ivo-ivc')


def validate(row):
    def finite(x):
        if isinstance(x,dict):
            for v in x.values():finite(v)
        elif isinstance(x,float) and not math.isfinite(x):raise ValueError('Nonfinite observation')
    finite(row)
    if not row['ok'] or any(row[k] for k in ('invalid_events','missing_click_intervals',
            'phantom_click_intervals','early_heat_steps','dropped_events','diagnostic_invalid')):
        raise ValueError('Native event/clock/causality failure')
    if row['positive_doses']<=0 or row['positive_doses']!=row['clicks']:
        raise ValueError('Injector excitation is detached from positive doses')
    for k,tol in (('step_mass_error_kg',1e-15),('step_energy_error_J',1e-9)):
        if abs(row[k])>tol:raise ValueError('Per-step source bookkeeping: '+k)
    for a,b,tol in (('event_fuel_kg','fuel_in_kg',1e-15),('heard_fuel_kg','fuel_in_kg',1e-15),
                    ('event_sensible_J','fuel_sensible_J',1e-9)):
        if not math.isclose(row[a],row[b],rel_tol=0,abs_tol=tol):raise ValueError('Cumulative source bookkeeping')
    for key,scale,tol in (('mass_error_kg','mass_scale_kg',2e-11),('species_error_kg','mass_scale_kg',2e-11),
                          ('energy_error_J','energy_scale_J',2e-9),('crank_error_J','crank_scale_J',1e-9)):
        if not abs(row[key])<tol*row[scale]:raise ValueError('Conservative books: '+key)
    for name in ('invalid-flow-reset','species-repair','energy-repair','temperature-floor',
                 'radiation-velocity-reset','invalid-wall-heat-reset'):
        if row['guard_counts'].get(name,0):raise ValueError('Numerical repair: '+name)
    if row['scenario']=='dfco-resume' and not (row['fuel_cut_steps']>0 and row['cut_resumes']>0
            and row['fuel_cut_clicks']==0 and row['kinds']['dfco']>0):
        raise ValueError('Missing or invalid DFCO/cut/resume coverage')
    return row


def commands(probe):
    return [(f'{p}-{s}-seed{seed}',[str(probe),'--preset',p,'--scenario',s,'--seed',str(seed)])
            for p in PRESETS for seed in SEEDS for s in SCENARIOS]


def execute(probe,out,workers):
    out.mkdir(parents=True,exist_ok=True)
    identity=hashlib.sha256(probe.read_bytes()).hexdigest()
    def one(job):
        key,cmd=job;path=out/(key+'.json')
        old=json.loads(path.read_text()) if path.exists() else {}
        if old.get('probe_sha256')==identity and old.get('command')==cmd:
            validate(old['result']);return old
        run=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
        (out/(key+'.log')).write_text(run.stderr)
        try:row=json.loads(run.stdout)
        except ValueError:
            path.with_suffix('.failed-output.txt').write_text(run.stdout);raise
        record=dict(probe_sha256=identity,command=cmd,returncode=run.returncode,result=row)
        path.write_text(json.dumps(record,indent=2)+'\n')
        if run.returncode:raise ValueError('Probe rejected '+key)
        validate(row);print(key,'pass',flush=True);return record
    with ThreadPoolExecutor(max_workers=workers) as pool:records=list(pool.map(one,commands(probe)))
    result=dict(runs=len(records),presets=PRESETS,seeds=SEEDS,scenarios=SCENARIOS,
        all_native_checks_pass=True,real_injector_calibrated=False,s1_complete=False,records=records)
    (out/'summary.json').write_text(json.dumps(result,indent=2)+'\n');return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--workers',type=int,default=2);a=p.parse_args()
    if not 1<=a.workers<=4:p.error('workers must be 1..4')
    r=execute(a.probe.resolve(),a.out,a.workers);print(r['runs'],'source scenarios pass; calibration remains open')


if __name__=='__main__':main()
