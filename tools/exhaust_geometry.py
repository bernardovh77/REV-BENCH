#!/usr/bin/env python3
"""044 bounded canonical-build compiler and native geometry-control preparation.

No runtime engine adapter or qualified component-loss model is supplied here.
The native recipe validates the descriptor; its inventories are observed once
at preparation. This compiler constructs new grids, port incidence, paths and
an executable B0 plan, rather than reusing the production cell connectivity.
"""
import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import subprocess

from build_contract import dumps, loads, number, require, validate_build
from check_build_contract import native, prepare_build

EXHAUST=('header','exhaust','center','link','muffler','tailpipe')


@dataclass(frozen=True)
class PreparedGeometry:
    """Immutable serialized plan; consumers get independent decoded copies."""
    payload: str

    def manifest(self):
        return loads(self.payload)


def _basis(overrides,identity,quantity):
    entry=overrides.get(identity,{}).get(quantity)
    if entry is None:
        return dict(basis='model-estimate',source='validated legacy recipe; not surveyed hardware',uncertainty=None)
    require(type(entry) is dict and set(entry)=={'basis','source','uncertainty'},'Dimension provenance fields')
    require(entry['basis'] in ('measured','published','model-estimate') and
            type(entry['source']) is str and 0<len(entry['source'])<=1024,'Dimension provenance basis/source')
    require(entry['uncertainty'] is None or number(entry['uncertainty'])>=0,'Dimension uncertainty')
    return dict(entry)


def compile_exhaust(build,native_probe,*,cell_m=.025,cell_capacity=8192,path_capacity=4096,
                    require_spatial=False,provenance=None,capability='geometry-control'):
    require(capability=='geometry-control','Full component impedances/thermal/flow/radiation await049–051; unsupported capability')
    validate_build(build,require_spatial=require_spatial)
    require(0<number(cell_m)<=1,'Positive requested cell size <=1 m')
    require(type(cell_capacity) is int and 1<=cell_capacity<=65536,'Gas-cell capacity')
    require(type(path_capacity) is int and 1<=path_capacity<=65536,'Path capacity')
    build=prepare_build(native_probe,build)
    text=''.join(f'{name} {i} {v:.17g}\n' for name,values in build['parameters'].items() for i,v in enumerate(values))
    raw=native(native_probe,'import',build['build_id'],text)
    inventory=raw['geometry_inventory']
    native_regions={(p['kind'],p['number']):p for p in inventory['pipes']}
    kinds=('runner','header','exhaust','tailpipe','muffler','snorkel','center','link')
    parameters=build['parameters'];ns=build['build_id'];components=[];node_map={};ports=[];losses=[]
    logical_nodes={n['id']:n['kind'] for n in build['geometry']['nodes']}
    provenance={} if provenance is None else provenance
    require(type(provenance) is dict,'Provenance mapping required')
    declared={c['id'] for c in build['geometry']['components'] if c['kind'] in EXHAUST}
    declared|={n for n,kind in logical_nodes.items() if kind in ('collector','junction')}
    require(set(provenance)<=declared,'Unknown provenance identity')
    for identity,entries in provenance.items():
        require(type(entries) is dict and set(entries)<={'length_m','area_m2','volume_m3'},'Unknown dimension provenance quantity')
    cell_start=0
    for c in build['geometry']['components']:
        if c['kind'] not in EXHAUST: continue
        role=int(c['id'].rsplit('-',1)[1]);observation=native_regions[kinds.index(c['kind']),role]
        for value,observed in ((c['length_m'],observation['length_m']),(c['area_m2'],observation['area_m2'])):
            require(math.isclose(value,observed,rel_tol=1e-10,abs_tol=1e-12),'Native physical dimensions differ')
        volume=c['length_m']*c['area_m2']
        require(math.isclose(volume,observation['volume_m3'],rel_tol=1e-10,abs_tol=1e-12),'Native storage volume differs')
        count=max(4,math.ceil(c['length_m']/cell_m))
        require(count<=65536 and cell_start+count<=cell_capacity,'Gas-cell capacity exceeded; no coarsening')
        ends=[]
        for end,port in enumerate(c['ports']):
            kind=logical_nodes[port['node_id']]
            # Ambient is an environmental label, never an inter-outlet duct.
            node_id=c['id']+'/outlet' if kind=='ambient-exhaust' else port['node_id']
            if node_id not in node_map:
                v=0
                if kind=='collector':v=inventory['collectors'][int(node_id.rsplit('-',1)[1])]
                if kind=='junction':v=inventory['junction_volume_m3']
                node_map[node_id]=dict(id=node_id,logical_id=port['node_id'],kind=kind,
                    volume_m3=v,volume_basis=_basis(provenance,node_id,'volume_m3'),ports=[],position_m=None)
            node=node_map[node_id];node['ports'].append(port['id']);ends.append(node_id)
            point=None if c['centerline_m'] is None else c['centerline_m'][0 if end==0 else -1]
            if point is not None:
                require(node['position_m'] is None or math.dist(point,node['position_m'])<=1e-10,
                        'Connected port positions disagree')
                node['position_m']=point
            ports.append(dict(id=port['id'],node_id=node_id,component_id=c['id'],axis_sign=port['axis_sign'],
                              gas_cell=cell_start+(0 if end==0 else count-1)))
        component=dict(id=c['id'],kind=c['kind'],nodes=ends,ports=[p['id'] for p in c['ports']],
            length_m=c['length_m'],area_m2=c['area_m2'],volume_m3=volume,
            cells=count,cell_start=cell_start,dx_m=c['length_m']/count,centerline_m=c['centerline_m'],
            length_basis=_basis(provenance,c['id'],'length_m'),area_basis=_basis(provenance,c['id'],'area_m2'),
            native_loss_metadata={k:observation[k] for k in ('packing_R','packing_tau','end_laminar','end_exit','friction_factor','wall_temperature_K')})
        components.append(component);cell_start+=count
        if c['kind']=='exhaust':
            losses.append(dict(id=c['id']+'/catalyst',kind='catalyst-interface',port_id=c['ports'][1]['id'],
                effective_area_m2=parameters['catalyst_area'][0],laminar_Pa_s_m=parameters['catalyst_laminar'][0],
                axial_length_m=None,volume_m3=0,dimension_basis='effective-model-estimate'))
        if c['kind']=='tailpipe' and parameters['muffler_type'][0]==2:
            losses.append(dict(id=f'{ns}/muffler-{role}/baffles',kind='baffle-interface',port_id=c['ports'][0]['id'],
                effective_area_m2=parameters['muffler_area'][0],laminar_Pa_s_m=observation['end_laminar'][0],
                axial_length_m=None,volume_m3=0,dimension_basis='effective-model-estimate'))
        if c['kind']=='muffler' and parameters['muffler_type'][0]==1:
            losses.append(dict(id=c['id']+'/packing',kind='distributed-packing',component_id=c['id'],
                               resistance_Pa_s_m2=observation['packing_R'],settle_s=observation['packing_tau'],dimension_basis='effective-model-estimate'))
        if c['kind']=='center' and parameters['center_packing'][0]>0:
            losses.append(dict(id=c['id']+'/packing',kind='distributed-packing',component_id=c['id'],
                resistance_Pa_s_m2=observation['packing_R'],settle_s=observation['packing_tau'],dimension_basis='effective-model-estimate'))
    nodes=list(node_map.values());indices={n['id']:i for i,n in enumerate(nodes)}
    require(len(nodes)<=32 and len(components)<=48 and all(len(n['ports'])<=8 for n in nodes),'B0 node/edge/degree capacity')
    sources=[dict(id=f'{ns}/cylinder-{int(c["id"].rsplit("-",1)[1])}/exhaust-valve',
                  node_id=c['nodes'][0],port_id=c['ports'][0]) for c in components if c['kind']=='header']
    outlets=[dict(id=c['id']+'/radiation',node_id=c['nodes'][1],port_id=c['ports'][1])
             for c in components if c['kind']=='tailpipe']
    adjacency={n['id']:[] for n in nodes}
    for c in components:
        for a,b,sign in ((c['nodes'][0],c['nodes'][1],1),(c['nodes'][1],c['nodes'][0],-1)):
            adjacency[a].append((b,c,sign))
    paths=[]
    def routes(start,target):
        found=[];stack=[(start,frozenset((start,)),[])]
        while stack:
            node,visited,path=stack.pop()
            if node==target:
                require(len(paths)+len(found)<path_capacity,'Source/outlet path capacity')
                found.append(path);continue
            for other,c,sign in adjacency[node]:
                if other not in visited:
                    stack.append((other,visited|{other},path+[dict(component_id=c['id'],direction=sign,
                        length_m=c['length_m'],area_m2=c['area_m2'])]))
        return found
    # Include every physical component port as a possible future located jet;
    # the compiler does not claim those noise sources are already implemented.
    for source in sources+[dict(id=p['id'],node_id=p['node_id']) for p in ports]:
        for outlet in outlets:
            route_list=routes(source['node_id'],outlet['node_id'])
            for route in route_list:paths.append(dict(source_id=source['id'],outlet_id=outlet['id'],
                segments=route,length_m=sum(r['length_m'] for r in route)))
    pairs=[dict(source_id=s['id'],outlet_id=o['id'],connected=any(
        p['source_id']==s['id'] and p['outlet_id']==o['id'] for p in paths)) for s in sources for o in outlets]
    result=dict(schema_version=1,kind='prepared-exhaust-geometry',capability=capability,
        build_id=ns,generation=build['generation'],revision=build['revision'],
        build_sha256=hashlib.sha256(dumps(build).encode()).hexdigest(),
        components=components,nodes=nodes,ports=ports,interfaces=losses,sources=sources,outlets=outlets,
        source_outlet_pairs=pairs,paths=paths,gas_cells=cell_start,
        volume_m3=sum(c['volume_m3'] for c in components)+sum(n['volume_m3'] for n in nodes),
        spatial_resolved=all(c['centerline_m'] is not None for c in components) and build['geometry']['engine_to_vehicle'] is not None,
        engine_to_vehicle=build['geometry']['engine_to_vehicle'],
        unqualified_physics=['valve-load','mean-flow','finite-junction-transfer','catalyst-impedance',
                             'packing','friction','wall-thermal','radiation'],
        backend_nodes=[dict(index=indices[n['id']],kind=1 if n['kind'] in ('exhaust-valve','ambient-exhaust') else 0,
                            value=0 if n['kind'] in ('exhaust-valve','ambient-exhaust') else n['volume_m3']) for n in nodes])
    return PreparedGeometry(json.dumps(result,sort_keys=True,allow_nan=False,separators=(',',':')))


def native_input(prepared,rate=192000,temperature=300,pressure=101325):
    require(8000<=number(rate)<=1000000 and 20<=number(temperature)<=4000 and number(pressure)>0,'Frozen environment')
    plan=prepared.manifest();indices={n['id']:i for i,n in enumerate(plan['nodes'])}
    require(plan['schema_version']==1 and plan['kind']=='prepared-exhaust-geometry' and
            plan['capability']=='geometry-control','Unsupported prepared geometry capability/version')
    lines=[f'{len(plan["nodes"])} {len(plan["components"])} {len(plan["sources"])} {len(plan["outlets"])} {rate:.17g} {temperature:.17g} {pressure:.17g}']
    lines.extend(f'{n["kind"]} {n["value"]:.17g}' for n in plan['backend_nodes'])
    lines.extend(f'{indices[c["nodes"][0]]} {indices[c["nodes"][1]]} {c["length_m"]:.17g} {c["area_m2"]:.17g} {c["cells"]}' for c in plan['components'])
    lines.extend(str(indices[s['node_id']]) for s in plan['sources'])
    lines.extend(str(indices[o['node_id']]) for o in plan['outlets'])
    return '\n'.join(lines)+'\n'


def execute(prepared,probe,**environment):
    r=subprocess.run([str(probe)],input=native_input(prepared,**environment),text=True,capture_output=True,timeout=120)
    require(r.returncode==0,f'Native preparation/control rejected ({r.returncode}): {r.stderr.strip()}')
    report=json.loads(r.stdout)
    plan=prepared.manifest()
    require(report['nodes']==len(plan['nodes']) and report['edges']==len(plan['components']) and
            report['gas_cells']==plan['gas_cells'],'Prepared counts differ')
    indices={n['id']:i for i,n in enumerate(plan['nodes'])}
    for index,(c,binding) in enumerate(zip(plan['components'],report['gas_bindings'],strict=True)):
        require(binding==dict(region=index,a=indices[c['nodes'][0]],b=indices[c['nodes'][1]],
            first_cell=c['cell_start'],last_cell=c['cell_start']+c['cells']-1),'Prepared gas port/cell binding differs')
    require(math.isclose(report['gas_volume_m3'],prepared.manifest()['volume_m3'],rel_tol=1e-10,abs_tol=1e-12),
            'Prepared gas volume differs')
    require(report['max_balance']<=1e-10 and report['failures']==0,'Compiled acoustic work/failure gate')
    pairs=prepared.manifest()['source_outlet_pairs']
    for index,(pair,observed) in enumerate(zip(pairs,report['responses'],strict=True)):
        require(observed['source']==index//len(plan['outlets']) and observed['outlet']==index%len(plan['outlets']),
                'Prepared source/outlet identity differs')
        require((observed['peak']>0)==pair['connected'],'Compiled all-source/all-outlet reachability differs')
        matching=[p for p in prepared.manifest()['paths'] if p['source_id']==pair['source_id'] and p['outlet_id']==pair['outlet_id']]
        if matching:
            earliest=min(sum(math.floor(s['length_m']*report['rate']/report['c']) for s in p['segments']) for p in matching)
            require(observed['first_tick']>=earliest,'Compiled response precedes physical path')
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build',type=Path);parser.add_argument('--native-contract',type=Path,required=True)
    parser.add_argument('--native-geometry',type=Path,required=True);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();prepared=compile_exhaust(loads(args.build.read_text()),args.native_contract)
    report=execute(prepared,args.native_geometry);args.out.mkdir(parents=True,exist_ok=True)
    (args.out/'geometry.json').write_text(prepared.payload+'\n');(args.out/'control.json').write_text(json.dumps(report,indent=2)+'\n')
