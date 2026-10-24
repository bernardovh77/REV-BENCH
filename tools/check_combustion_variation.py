"""Stage041 seeded native map; evidence/validity, never automatic realism approval."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import subprocess
import time

ROOT=Path(__file__).resolve().parents[1]
SEEDS=(17,29,61)
FLAGS=('phi-range','temperature-range','pressure-range','dilution-range','speed-floor',
       'dilution-floor','demand-limit','invalid-input','no-reactants','capped-policy','spark-limit')


def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def validate(row):
    def finite(value):
        if isinstance(value,dict):
            for v in value.values():finite(v)
        elif isinstance(value,list):
            for v in value:finite(v)
        elif isinstance(value,float) and not math.isfinite(value):
            raise ValueError('Nonfinite map observation')
    finite(row)
    if not row['ok'] or row['warm_stalled_steps'] or row['early_heat_steps'] or row['invalid_events'] or row['dropped_events']:
        raise ValueError('Native operating/accounting/event gate failed')
    for name,scale,tol in [('mass_error_kg','mass_scale_kg',2e-11),
                           ('species_error_kg','mass_scale_kg',2e-11),
                           ('energy_error_J','energy_scale_J',2e-9),
                           ('crank_error_J','crank_scale_J',1e-9)]:
        if not math.isfinite(row[name]) or not abs(row[name])<tol*row[scale]:
            raise ValueError('Accounting tolerance failed: '+name)
    if row['diagnostic_invalid'] or row['flags'][7]:
        raise ValueError('Invalid numerical observation or charge input')
    for name in ('invalid-flow-reset','species-repair','energy-repair','temperature-floor',
                 'radiation-velocity-reset','invalid-wall-heat-reset'):
        if row['guard_counts'].get(name,0):
            raise ValueError('Numerical repair in operating map: '+name)
    for c in row['cylinders']:
        net,gross,pump=(c[k] for k in ('net_imep_Pa','gross_imep_Pa','pumping_mep_Pa'))
        if net['n']<2 or net['n']!=gross['n'] or net['n']!=pump['n']:
            raise ValueError('Incomplete or mismatched cylinder work windows')
        if not math.isclose(gross['mean'],net['mean']+pump['mean'],rel_tol=2e-12,abs_tol=1e-6):
            raise ValueError('Gross/net/pumping books disagree')
    return row


def commands(probe):
    jobs=[]
    builds=json.loads((ROOT/'contracts/examples/builds-v1.json').read_text())
    limits={b['build_id']:b['parameters']['rev_limit'][0]*30/math.pi for b in builds}
    limits.update({'i6':8500,'v8-cross':7500,'v8-flat':9000})
    for preset,limit in limits.items():
        for seed in SEEDS:
            base=[str(probe),'--preset',preset,'--seed',str(seed)]
            jobs.append((f'{preset}-idle-{seed}',base+['--mode','idle','--seconds','30','--warmup','6']))
            for rpm in (1000,3000,round(.9*limit)):
                for throttle in (.1,1):
                    key=f'{preset}-held-{rpm}-throttle{throttle:g}-seed{seed}'
                    jobs.append((key,base+['--rpm',str(rpm),'--throttle',str(throttle)]))
            jobs.append((f'{preset}-zero-3000-{seed}',base+['--variation','0']))
    return jobs


def execute(probe,out,workers=2):
    out.mkdir(parents=True,exist_ok=True);identity=digest(probe)
    jobs=commands(probe)
    def one(job):
        key,command=job;path=out/(key+'.json')
        if path.exists():
            old=json.loads(path.read_text())
            if old.get('probe_sha256')==identity and old.get('command')==command:
                validate(old['result']);return old
        start=time.monotonic()
        run=subprocess.run(command,text=True,capture_output=True,timeout=600)
        (out/(key+'.log')).write_text(run.stderr)
        try:row=json.loads(run.stdout)
        except ValueError:
            path.with_suffix('.failed-output.txt').write_text(run.stdout)
            raise ValueError(f'Probe output failed for {key}: {run.returncode}')
        record=dict(key=key,command=command,probe_sha256=identity,elapsed_s=time.monotonic()-start,
                    returncode=run.returncode,result=row)
        path.write_text(json.dumps(record,indent=2)+'\n')
        if run.returncode:raise ValueError('Probe rejected '+key)
        validate(row);print(key,'pass',flush=True);return record
    with ThreadPoolExecutor(max_workers=workers) as pool:records=list(pool.map(one,jobs))
    rows=[r['result'] for r in records]
    lookup={r['key']:r['result'] for r in records}
    controls=[]
    for preset in {r['preset'] for r in rows}:
        for seed in SEEDS:
            a=lookup[f'{preset}-zero-3000-{seed}'];b=lookup[f'{preset}-held-3000-throttle1-seed{seed}']
            change=100*(b['brake_torque_Nm']['mean']/a['brake_torque_Nm']['mean']-1)
            controls.append(dict(preset=preset,seed=seed,mean_torque_change_pct=change,
                                 legacy_under_one_pct=abs(change)<1))
    summary=dict(scope='Seeded native consistency and validity map, not matched-engine calibration or sound acceptance.',
        residual_fraction_basis='Burned-product mass fraction only; retained reactants remain in actual phi/species. No total gas-origin tracer.',
        heat_ca50_basis='Integrated chemical heat reaches half the charge eligible to burn; incomplete charges may have no crossing.',
        probe_sha256=identity,runs=len(rows),seeds=SEEDS,idle_runs=sum(r['mode']=='idle' for r in rows),
        all_native_checks_pass=True,flags={name:sum(r['flags'][i] for r in rows) for i,name in enumerate(FLAGS)},
        mean_torque_controls=controls,
        all_mean_torque_controls_under_one_pct=all(c['legacy_under_one_pct'] for c in controls),
        real_engine_cov_qualified=False,s1_complete=False,records=records)
    (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    return summary


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--probe',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--workers',type=int,default=2);args=p.parse_args()
    if not 1<=args.workers<=4:p.error('workers must be1..4')
    result=execute(args.probe.resolve(),args.out,args.workers)
    print(f"{result['runs']} seeded maps pass; matched real COV and S1 remain unqualified")


if __name__=='__main__':main()
