#!/usr/bin/env python3
"""Reproduce 044 five native networks, inventories, paths and compiler faults."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from build_contract import ROOT,loads
from exhaust_geometry import compile_exhaust,execute,native_input

FAULTS={
 'ambient-shortcut':("node_id=c['id']+'/outlet' if kind=='ambient-exhaust' else port['node_id']","node_id=port['node_id']",'test_five_native_networks_all_sources_outlets_and_inventory'),
 'missing-second-link':("if c['kind'] not in EXHAUST: continue","if c['kind'] not in EXHAUST or c['id'].endswith('link-1'): continue",'test_five_native_networks_all_sources_outlets_and_inventory'),
 'altered-acoustic-length':("length_m=c['length_m'],area_m2=c['area_m2'],volume_m3=volume,","length_m=1.1*c['length_m'],area_m2=c['area_m2'],volume_m3=volume,",'test_five_native_networks_all_sources_outlets_and_inventory'),
 'altered-bore-area':("area_m2=c['area_m2'],volume_m3=volume,","area_m2=.9*c['area_m2'],volume_m3=volume,",'test_five_native_networks_all_sources_outlets_and_inventory'),
 'lost-collector-storage':("v=inventory['collectors'][int(node_id.rsplit('-',1)[1])]","v=.5*inventory['collectors'][int(node_id.rsplit('-',1)[1])]",'test_five_native_networks_all_sources_outlets_and_inventory'),
 'reversed-port-normal':("axis_sign=port['axis_sign'],","axis_sign=-port['axis_sign'],",'test_five_native_networks_all_sources_outlets_and_inventory'),
 'ignored-cell-capacity':("require(count<=65536 and cell_start+count<=cell_capacity,'Gas-cell capacity exceeded; no coarsening')","require(True,'faulty capacity bypass')",'test_capacity_capabilities_short_delays_and_provenance'),
 'unsupported-losses-accepted':("require(capability=='geometry-control',","require(True,",'test_capacity_capabilities_short_delays_and_provenance'),
}


def faults(contract,probe,out):
    source=(ROOT/'tools/exhaust_geometry.py').read_text();results=[]
    for name,(before,after,test) in FAULTS.items():
        if source.count(before)!=1:raise ValueError(f'{name}: expected one mutation location')
        with tempfile.TemporaryDirectory(prefix='rev-bench-geometry-fault-') as temp:
            (Path(temp)/'exhaust_geometry.py').write_text(source.replace(before,after))
            env=dict(os.environ,RB_GEOMETRY_CONTRACT_PROBE=str(contract),RB_GEOMETRY_PROBE=str(probe))
            code='import sys,unittest;sys.path[:0]=sys.argv[1:3];result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromName(sys.argv[3]));sys.exit(not result.wasSuccessful())'
            result=subprocess.run([str(Path(os.sys.executable)), '-c',code,temp,str(ROOT/'tools'),
                'test_exhaust_geometry.ExhaustGeometryTests.'+test],capture_output=True,text=True,env=env,timeout=180)
            (out/(name+'.log')).write_text(result.stdout+result.stderr)
            # An import/build failure cannot masquerade as caught behavior.
            caught=result.returncode!=0 and 'Ran 1 test' in result.stderr and 'FAILED' in result.stderr
            results.append(dict(fault=name,caught=caught,returncode=result.returncode))
    if not all(x['caught'] for x in results):raise ValueError(f'Uncaught or unexecuted compiler faults: {results}')
    return results


def check(contract,probe,out,mutations=False):
    out.mkdir(parents=True,exist_ok=True);records=[]
    for build in loads((ROOT/'contracts/examples/builds-v1.json').read_text()):
        start=time.perf_counter();prepared=compile_exhaust(build,contract);prepare_s=time.perf_counter()-start
        plan=prepared.manifest();result=execute(prepared,probe)
        folder=out/build['build_id'];folder.mkdir(exist_ok=True)
        (folder/'geometry.json').write_text(prepared.payload+'\n')
        (folder/'native-plan.txt').write_text(native_input(prepared))
        (folder/'native-control.json').write_text(json.dumps(result,indent=2)+'\n')
        records.append(dict(engine=build['build_id'],components=len(plan['components']),nodes=len(plan['nodes']),
            gas_cells=plan['gas_cells'],volume_m3=plan['volume_m3'],interfaces=len(plan['interfaces']),
            cylinder_pairs=len(plan['source_outlet_pairs']),connected_pairs=sum(p['connected'] for p in plan['source_outlet_pairs']),
            all_port_paths=len(plan['paths']),spatial_resolved=plan['spatial_resolved'],
            max_balance=result['max_balance'],failures=result['failures'],prepare_s=prepare_s,
            diagnostic_control_s=result['elapsed_s'],timing_kind=result['timing_kind'],plan_sha256=hashlib.sha256(prepared.payload.encode()).hexdigest()))
    report=dict(schema=1,scope='bounded lossless geometry control, not production component qualification',engines=records)
    if mutations:report['mutations']=faults(contract,probe,out)
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n');return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-contract',type=Path,required=True);parser.add_argument('--native-geometry',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True);parser.add_argument('--mutations',action='store_true')
    a=parser.parse_args();print(json.dumps(check(a.native_contract.resolve(),a.native_geometry.resolve(),a.out,a.mutations),indent=2))
