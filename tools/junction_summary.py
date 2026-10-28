#!/usr/bin/env python3
"""Join049 selected gates without erasing rejected coarse or prototype results."""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from refine_junction_flow import compare_rows,load_trace
from junction_mesh import fixture,helmholtz

def load(p):return json.loads(p.read_text())

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--evidence',type=Path,required=True);p.add_argument('--candidate',choices=['v2','v3'],default='v3');a=p.parse_args();root=a.evidence
    matrix=root/f'matrix-{a.candidate}';control_root=root/f'controls-{a.candidate}';flow_root=root/f'flow-{a.candidate}';qualified_root=root/f'qualified-{a.candidate}'
    acoustic=load(matrix/'summary.json');controls=load(control_root/'summary.json');compact=load(root/'compact/summary.json')
    parity=load(root/f'parity-{a.candidate}/summary.json');costs=load(root/f'costs-{a.candidate}/summary.json')
    flow=[];rows=[]
    for path in sorted(flow_root.glob('*/summary.json')):
        primary=load(path);refinement=load(path.with_name('refinement.json')) if path.with_name('refinement.json').exists() else None
        selected=primary['rows'][-1];qualified=primary['pass_'];n=16
        rows.extend(primary['rows'])
        if not qualified and refinement:
            rows.extend(r['row'] for r in refinement['refinements']);selected=refinement['refinements'][-1]['row'];n=refinement['selected_n'];qualified=refinement['pass_']
        source=selected['source'];lo=selected['port_min_velocity'][source];hi=selected['port_max_velocity'][source]
        direction=(lo<0<hi) if primary['mode']=='reversal' else (lo<0 if primary['amplitude']>0 else hi>0)
        flow.append(dict(kind=primary['kind'],mode=primary['mode'],amplitude=primary['amplitude'],primary=primary,refinement=refinement,selected_n=n,selected_transport_n=refinement.get('selected_transport_n') if refinement else None,selected=selected,
                         actual_source_velocity_range=[lo,hi],direction_pass=bool(direction),pass_=bool(qualified and direction)))
    if len(flow)!=12:raise ValueError('Missing signed branch controls')
    for case in acoustic['cases']:rows.extend(r for c in case['candidates'] for r in c['rows'])
    for case in controls['cases']:rows.extend(case['rows'])
    for case in costs['cases']:rows.extend(case['rows'])
    checks={}
    for name,kind in [('clock-y','y'),('capture-h','h')]:
        d=np.load(control_root/name/'transfer.npz');base=np.load(matrix/f'{kind}-900/{kind}-16-900-2000000-matrix.npz')['candidate'][:,:,0]
        error=float(np.linalg.norm(d['measured']-base)/np.linalg.norm(base));checks[name]=dict(relative_error=error,pass_=error<=.01)
    for name,kind,mode,A in [('flow-clock-y','y','flow',1000),('flow-clock-area-reversal','area','reversal',1000)]:
        n=8 if kind=='y' else 16;case=flow_root/f'{kind}-{mode}-{A}';base=load(case/f'{kind}-{mode}-{A}-{n}.json')
        r=load(control_root/name/'summary.json')['rows'][0]
        check=compare_rows(base,r,load_trace(case/f'{kind}-{mode}-{A}-{n}.csv'),load_trace(control_root/name/(name+'.csv')))
        check['pass_']=bool(check['pass_'] and max(check['pressure_l2'])<=.01 and check['transported_mass_relative']<=.01)
        if name=='flow-clock-area-reversal' and not check['pass_']:
            fine_name=name+'-fine';fine=load(control_root/fine_name/'summary.json')['rows'][0]
            finer=compare_rows(r,fine,load_trace(control_root/name/(name+'.csv')),load_trace(control_root/fine_name/(fine_name+'.csv')))
            finer['pass_']=bool(finer['pass_'] and max(finer['pressure_l2'])<=.01 and finer['transported_mass_relative']<=.01 and finer['transported_mass_relative']<check['transported_mass_relative'])
            checks[name]=dict(primary=check,refined=finer,selected_clock=8000000,pass_=finer['pass_'])
        else:checks[name]=check
    edit=np.load(control_root/'edit-x/transfer.npz')['measured'];base=np.load(matrix/'x-900/x-16-900-2000000-matrix.npz')['candidate'][:,:,0]
    change=float(np.max(abs(edit[:,0]-base[:,0])));checks['actual-upstream-geometry-response']=dict(maximum_complex_return_change=change,pass_=change>=.05)
    reference=np.load(control_root/'edit-x/transfer.npz')['reference']
    coarse=helmholtz(fixture('x',24,stub_ratio=2),[200,500,1000,2000,4000],580.1001671698209)[:,:,0]
    uncertainty=float(np.linalg.norm(coarse-reference)/np.linalg.norm(reference))
    checks['edited-geometry-independent-reference']=dict(relative_refinement_change=uncertainty,pass_=uncertainty<=.02)
    for kind in ['y','h']:
        coarse=load(control_root/f'coarse-{kind}/summary.json')['matrix_relative_error']
        fine=next(c for c in acoustic['cases'] if c['kind']==kind and c['T']==900)['candidates'][-1]['matrix_relative_error']
        checks[f'{kind}-spatial-convergence']=dict(coarse_error=coarse,fine_error=fine,pass_=fine<coarse)
    frozen=load(qualified_root/'sources.json')
    code=[k for k in frozen if k.startswith('experiments/') or k=='tools/junction_probe.c']
    source_match=all(hashlib.sha256(Path(k).read_bytes()).hexdigest()==frozen[k] for k in code)
    probe=hashlib.sha256((qualified_root/'junction_probe').read_bytes()).hexdigest()
    trajectory_binding=all(r['probe_sha256']==probe for r in rows)
    physical_gate=all(not r['status'] and r.get('book_pass',False) and r['max_mach']<=.05 for r in rows)
    summary=dict(schema=1,candidate=a.candidate,acoustic=acoustic,flow=flow,controls=controls,independent_checks=checks,compact=compact,parity=parity,costs=costs,
                 source_match=source_match,trajectory_binding=trajectory_binding,physical_gate=physical_gate,probe_sha256=probe,
                 maximum_mach=max(r['max_mach'] for r in rows),maximum_cfl=max(r['max_cfl'] for r in rows),
                 maximum_whole_face_fallbacks=max(r['reconstruction_fallbacks'] for r in rows),
                 maximum_trace_face_predictions=max(r['trace_face_limits'] for r in rows),
                 maximum_normalized_book=max(max(r.get('mass_relative',float('inf')),r.get('energy_relative',float('inf')),r.get('momentum_relative',float('inf'))) for r in rows),
                 pass_=bool(source_match and trajectory_binding and physical_gate and acoustic['pass_'] and controls['pass_'] and parity['pass_'] and parity.get('native_sha256')==probe and costs['pass_'] and compact['low_frequency_limit_pass'] and all(c['pass_'] for c in flow) and all(c['pass_'] for c in checks.values())))
    (root/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(dict(pass_=summary['pass_'],physical_books=summary['maximum_normalized_book'],checks=checks,selected_grids={f"{c['kind']}-{c['mode']}-{c['amplitude']}":c['selected_n'] for c in flow}),indent=2))
    if not summary['pass_']:raise SystemExit('049 retains unresolved qualification gates')

if __name__=='__main__':main()
