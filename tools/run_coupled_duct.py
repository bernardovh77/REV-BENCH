#!/usr/bin/env python3
"""Measure the bounded open-port coupling proof; never changes engine sound."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RATE, C, RHO, GAMMA = 192000., 343., 1.2, 1.4
P0 = RHO*C*C/GAMMA


def read(path):
    with path.open() as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"Empty measurement: {path}")
    data = {key: np.array([float(row[key]) for row in rows]) for key in rows[0]}
    if not all(np.isfinite(value).all() for value in data.values()):
        raise ValueError("Nonfinite coupled measurement")
    return data


def initial(x, length, amplitude):
    pressure = P0*(1+amplitude*np.exp(-((x/length-.15)/.04)**2))
    density = RHO*(pressure/P0)**(1/GAMMA)
    velocity = 2*(np.sqrt(GAMMA*pressure/density)-C)/(GAMMA-1)
    return pressure, velocity


def exact_pressure(x, length, amplitude, elapsed):
    """Independent pre-shock right simple-wave characteristic solution."""
    foot = x-C*elapsed
    for _ in range(80):
        _, velocity = initial(foot, length, amplitude)
        updated = x-(C+(GAMMA+1)*velocity/2)*elapsed
        if np.max(abs(updated-foot)) < 1e-14:
            foot = updated
            break
        foot = updated
    else:
        raise ValueError("Exact simple-wave characteristic did not converge")
    return initial(foot, length, amplitude)[0]


def assess(rows):
    if len({r['id'] for r in rows}) != len(rows) or not rows:
        raise ValueError("Missing or duplicate coupled cases")
    if any(r['status'] or r['failures'] or r['failed_stage'] for r in rows):
        raise ValueError("Coupled solver failed; do not interpret partial results")
    for row in rows:
        if any(isinstance(value,(float,int)) and not np.isfinite(value) for value in row.values()):
            raise ValueError("Nonfinite coupled scalar measurement")
        if any(len(row[side])!=4 for side in ('left_balance','right_balance')):
            raise ValueError("Incomplete conservative balance")
        if row.get('cells')!=560 or row.get('rate_Hz')!=192000:
            raise ValueError("Incorrect geometry or clock in proof matrix")
        if abs(row.get('length_m',0)-560*C/RATE)>1e-12:
            raise ValueError("Incorrect physical path length")
    expected_cases={(g,a,cut,steps,r) for g,a,cut,steps,r in
        [('simple-wave',a,cut,448,0.) for a in (1e-5,.001,.01) for cut in (168,280,560)] +
        [(g,.001,168,steps,r) for g,steps in (('reflection',1120),('growth',192000)) for r in (-1.,0.,1.)]}
    if {(r['group'],r.get('amplitude'),r.get('cut'),r.get('steps'),r.get('reflection')) for r in rows} != expected_cases:
        raise ValueError("Incorrect or incomplete proof trajectories")
    if not all(np.isfinite(x) for r in rows for side in ('left_balance','right_balance') for x in r[side]):
        raise ValueError("Nonfinite balance")
    wave = [r for r in rows if r['group']=='simple-wave']
    reflection = [r for r in rows if r['group']=='reflection']
    growth = [r for r in rows if r['group']=='growth']
    expected = {(a, cut) for a in (1e-5,.001,.01) for cut in (168,280,560)}
    if {(r['amplitude'],r['cut']) for r in wave} != expected or len(wave)!=9:
        raise ValueError("Incomplete simple-wave matrix")
    if {r['reflection'] for r in reflection} != {-1.,0.,1.} or len(reflection)!=3:
        raise ValueError("Incomplete reflection controls")
    if {r['reflection'] for r in growth} != {-1.,0.,1.} or len(growth)!=3 or len(rows)!=15:
        raise ValueError("Incomplete growth controls")
    conservation = max(abs(value) for r in rows for side in ('left_balance','right_balance') for value in r[side])
    # Do not compare kg, N*s and J to one dimensionful number. Normalize each
    # physical inventory separately; a zero-size all-FV right region must close
    # absolutely. This also makes a one-second roundoff budget meaningful.
    normalized=[]
    for row in rows:
        for side,count in (('left_balance',row['cut']),('right_balance',row['cells']-row['cut'])):
            if not count:
                if any(abs(value)>1e-14 for value in row[side]):
                    raise ValueError("Nonzero transfer in an absent subdomain")
                continue
            volume=.003*row['length_m']*count/row['cells']
            scales=(RHO*volume,RHO*C*volume,P0/(GAMMA-1)*volume,RHO*volume)
            normalized.extend(abs(value)/scale for value,scale in zip(row[side],scales))
    conservation_relative=max(normalized)
    proof = [r for r in wave if r['cut']!=560]
    timing = max(abs(r['echo_error_ticks']) for r in reflection if r['reflection'])
    signs = all(r['return_peak_Pa']*r['reflection']>0 for r in reflection if r['reflection'])
    muted = next(r for r in reflection if r['reflection']==0)
    partitions = {str(a): max(r['relative_pressure_L2'] for r in proof if r['amplitude']==a) for a in (1e-5,.001,.01)}
    if any(not (r['initial_perturbation_J']>0) for r in growth):
        raise ValueError("Growth screen requires nonzero initial disturbance")
    if any(r['peak_perturbation_J']<r['initial_perturbation_J'] or r['final_perturbation_J']<0 for r in growth):
        raise ValueError("Invalid perturbation energy")
    growth_ratio=max(r['peak_perturbation_J']/r['initial_perturbation_J'] for r in growth)
    final_ratio=max(r['final_perturbation_J']/r['initial_perturbation_J'] for r in growth)
    gates = dict(both_region_conservation=conservation_relative<1e-10,
                 independent_simple_wave=max(partitions.values())<.02,
                 reflected_load_sign=signs, return_timing=timing<=5,
                 matched_echo=abs(muted['return_peak_Pa'])<.1,
                 source_free_growth=growth_ratio<=1.02 and final_ratio<.001)
    return dict(gates=gates, proof_pass=all(gates.values()),
                maximum_region_balance_absolute=conservation,
                maximum_region_balance_relative=conservation_relative,
                balance_scale_note='Each kg, N*s, J and tagged-kg residual divided by its own initial reference inventory; momentum scale rho*c*volume. Relative engineering screen1e-10; raw mixed-unit maximum retained only for provenance.',
                maximum_echo_error_ticks=timing, partition_pressure_error=partitions,
                maximum_source_free_energy_ratio=growth_ratio, maximum_final_energy_ratio=final_ratio,
                production_engine_validated=False, variable_mixture_EOS_validated=False,
                thermal_reversal_validated=False, real_time_engine_validated=False)


def run(args):
    directory = args.output.resolve(); directory.mkdir(parents=True, exist_ok=True)
    source_paths = [ROOT/'Makefile',ROOT/'tools/coupled_duct_probe.c',Path(__file__),
                    *ROOT.joinpath('experiments/acoustics').glob('*.[ch]')]
    manifest = dict(source_revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        working_tree_status=subprocess.check_output(['git','status','--short'],cwd=ROOT,text=True),
        sources={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths},
        method='One conserved spatial state; retained MC/RK2 gas region; downstream characteristic streaming plus distributed nonlinear/entropy correction. No added acoustic energy reservoir.',
        initial_background=dict(rate_Hz=RATE,c_m_s=C,rho_kg_m3=RHO,gamma=GAMMA,pressure_Pa=P0),cases=[])
    rows=[]
    matrix=[(f'wave-{a}-{cut}','simple-wave',cut,a,448,0.) for a in (1e-5,.001,.01) for cut in (168,280,560)]
    matrix += [(f'reflect-{r}','reflection',168,.001,1120,r) for r in (-1.,0.,1.)]
    matrix += [(f'growth-{r}','growth',168,.001,192000,r) for r in (-1.,0.,1.)]
    for key,group,cut,amplitude,steps,reflection in matrix:
        prefix=directory/key
        command=[str(args.probe.resolve()),'560',str(cut),str(amplitude),str(steps),str(reflection),str(prefix)]
        result=subprocess.run(command,capture_output=True,text=True,check=True)
        row=dict(json.loads(result.stdout),id=key,group=group)
        (directory/f'{key}.json').write_text(json.dumps(row,indent=2)+'\n')
        manifest['cases'].append(dict(id=key,command=command,stderr=result.stderr))
        if group=='simple-wave':
            field=read(Path(str(prefix)+'.field.csv'))
            target=exact_pressure(field['x_m'],row['length_m'],amplitude,steps/RATE)
            row['relative_pressure_L2']=float(np.linalg.norm(field['pressure_Pa']-target)/np.linalg.norm(target-P0))
        elif group=='reflection':
            trace=read(Path(str(prefix)+'.trace.csv'))
            a,b=int(1.6*560),int(1.8*560)
            index=a+int(np.argmax(abs(trace['gas_probe_Pa'][a:b])))
            position=(int(.15*560)+.5)*C/RATE
            expected_time=(2*row['length_m']-.15*row['length_m']-position)/C
            row.update(return_peak_Pa=float(trace['gas_probe_Pa'][index]),return_time_s=float(trace['time_s'][index]),
                expected_return_time_s=expected_time,echo_error_ticks=float((trace['time_s'][index]-expected_time)*RATE))
        rows.append(row)
    # Fixture throughput without trace recording/file output, interleaved controls.
    costs=[]
    for repeat in range(3):
        for cut in (168,280,560):
            command=[str(args.probe.resolve()),'560',str(cut),'.001','448','0','-']
            row=json.loads(subprocess.check_output(command,text=True))
            costs.append(dict(row,repeat=repeat)); manifest['cases'].append(dict(id=f'cost-{repeat}-{cut}',command=command))
    summary=assess(rows)
    summary['fixture_cost_median_seconds']={str(cut):float(np.median([r['seconds'] for r in costs if r['cut']==cut])) for cut in (168,280,560)}
    summary['fixture_cost_note']='Entire 560-cell coupling fixture, not graph-only, production engine, WASM or callback real-time acceptance; downstream correction grid remains.'
    (directory/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (directory/'measurements.json').write_text(json.dumps(rows,indent=2)+'\n')
    (directory/'costs.json').write_text(json.dumps(costs,indent=2)+'\n')
    (directory/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary,indent=2))
    if not summary['proof_pass']:
        raise SystemExit('Coupled proof rejected: inspect each gate')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe',type=Path,default=Path('/tmp/rev-bench-s2d-next/coupled_duct_probe'))
    parser.add_argument('--output',type=Path,default=ROOT/'renders/evidence/coupling-proof-2026-10-23')
    run(parser.parse_args())
