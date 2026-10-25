"""044 actual native preparation/propagation and independent geometry controls."""
import copy
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from build_contract import ROOT,from_native,loads
from check_build_contract import native
from exhaust_geometry import PreparedGeometry,compile_exhaust,execute,native_input


class ExhaustGeometryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(prefix='rev-bench-geometry-tests-')
        cls.contract=Path(os.environ.get('RB_GEOMETRY_CONTRACT_PROBE',Path(cls.temp.name)/'build_contract_probe'))
        cls.probe=Path(os.environ.get('RB_GEOMETRY_PROBE',Path(cls.temp.name)/'exhaust_geometry_probe'))
        if not cls.contract.is_file() or not cls.probe.is_file():
            subprocess.run(['make','-s','-j2',f'BUILD={cls.temp.name}','LTO=0',str(cls.contract),str(cls.probe)],
                           cwd=ROOT,check=True,capture_output=True,text=True,timeout=300)
        cls.builds=loads((ROOT/'contracts/examples/builds-v1.json').read_text())

    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()

    def compile(self,build=None,**kwargs): return compile_exhaust(build or self.builds[0],self.contract,**kwargs)

    def test_five_native_networks_all_sources_outlets_and_inventory(self):
        expected_pairs={'i4':4,'i6':6,'v8-cross':16,'v8-flat':8,'boxer4':8}
        for build in self.builds:
            with self.subTest(engine=build['build_id']):
                prepared=self.compile(build);plan=prepared.manifest();report=execute(prepared,self.probe)
                self.assertEqual(sum(p['connected'] for p in plan['source_outlet_pairs']),expected_pairs[build['build_id']])
                self.assertEqual(report['gas_cells'],plan['gas_cells'])
                original={c['id']:c for c in build['geometry']['components'] if c['kind'] not in ('runner','snorkel')}
                self.assertEqual({c['id'] for c in plan['components']},set(original))
                for c in plan['components']:
                    self.assertEqual(c['length_m'],original[c['id']]['length_m'])
                    self.assertEqual(c['area_m2'],original[c['id']]['area_m2'])
                    self.assertAlmostEqual(c['cells']*c['dx_m'],c['length_m'],places=12)
                    self.assertEqual(c['ports'],[p['id'] for p in original[c['id']]['ports']])
                    self.assertEqual(c['length_basis']['basis'],'model-estimate')
                raw=native(self.contract,'export',build['build_id'])['geometry_inventory']
                for n in plan['nodes']:
                    if n['kind']=='collector':self.assertEqual(n['volume_m3'],raw['collectors'][int(n['id'].rsplit('-',1)[1])])
                    if n['kind']=='junction':self.assertEqual(n['volume_m3'],raw['junction_volume_m3'])
                # Independent descriptor length sum: these base systems have
                # exactly one loop-free path per reachable source/outlet pair.
                p=build['parameters'];expected={}
                for i,L in enumerate(p['header_length']):
                    total=L+sum(p[k][0] for k in ('exhaust_pipe_length','center_length','link_length','muffler_length','tailpipe_length'))
                    expected[f'{build["build_id"]}/cylinder-{i}/exhaust-valve']=total
                cylinder_paths=[r for r in plan['paths'] if r['source_id'] in expected]
                self.assertEqual(len(cylinder_paths),expected_pairs[build['build_id']])
                for path in cylinder_paths:self.assertAlmostEqual(path['length_m'],expected[path['source_id']],places=10)
                for port in plan['ports']:
                    end=int(port['id'][-1]);self.assertEqual(port['axis_sign'],1 if end==0 else -1)
                for source in plan['sources']:
                    slot=int(source['id'].split('/cylinder-')[1].split('/')[0])
                    self.assertEqual(source['port_id'],f'{build["build_id"]}/header-{slot}:0')
                    self.assertEqual(source['node_id'],original[f'{build["build_id"]}/header-{slot}']['ports'][0]['node_id'])
                cats=[x for x in plan['interfaces'] if x['kind']=='catalyst-interface']
                self.assertEqual(len(cats),len(raw['collectors']))
                for x in cats:
                    self.assertEqual(x['effective_area_m2'],p['catalyst_area'][0]);self.assertEqual(x['laminar_Pa_s_m'],p['catalyst_laminar'][0])

    def test_unresolved_and_inconsistent_spatial_geometry_reject(self):
        for b in self.builds:
            with self.assertRaisesRegex(ValueError,'Unresolved'):self.compile(b,require_spatial=True)
        b=copy.deepcopy(self.builds[0]);b['geometry']['engine_to_vehicle']=dict(translation_m=[0,0,0],rotation_xyzw=[0,0,0,1])
        # Explicit connected bent paths with known lengths. Synthetic geometry
        # is a consistency fixture, not a routing suggestion for a real car.
        positions={n['id']:[i*.0001,0,0] for i,n in enumerate(b['geometry']['nodes'])}
        for c in b['geometry']['components']:
            a,z=[positions[p['node_id']] for p in c['ports']];distance=math.dist(a,z)
            midpoint=[(a[0]+z[0])/2,0,math.sqrt(c['length_m']**2-distance**2)/2]
            c['centerline_m']=[a,midpoint,z]
        plan=self.compile(b,require_spatial=True).manifest();self.assertTrue(plan['spatial_resolved'])
        for c in plan['components']:
            self.assertAlmostEqual(sum(math.dist(a,z) for a,z in zip(c['centerline_m'],c['centerline_m'][1:])),c['length_m'],places=10)
        bad=copy.deepcopy(b);bad['geometry']['components'][0]['centerline_m'][1][2]*=1.01
        with self.assertRaisesRegex(ValueError,'acoustic length'):self.compile(bad,require_spatial=True)
        # Translate the entire header without changing its own arc length:
        # each line still validates, but shared physical ports must disagree.
        bad=copy.deepcopy(b)
        c=next(c for c in bad['geometry']['components'] if c['kind']=='header')
        c['centerline_m']=[[p[0],p[1]+1,p[2]] for p in c['centerline_m']]
        with self.assertRaisesRegex(ValueError,'port positions'):self.compile(bad,require_spatial=True)

    def test_capacity_capabilities_short_delays_and_provenance(self):
        for kwargs in (dict(cell_capacity=4),dict(path_capacity=1),dict(capability='physical-exhaust'),dict(cell_m=0)):
            with self.assertRaises(ValueError):self.compile(**kwargs)
        b=copy.deepcopy(self.builds[0]);cid=next(c['id'] for c in b['geometry']['components'] if c['kind']=='header')
        provenance={cid:dict(length_m=dict(basis='measured',source='synthetic fixture ruler',uncertainty=.001))}
        c=next(c for c in self.compile(b,provenance=provenance).manifest()['components'] if c['id']==cid)
        self.assertEqual(c['length_basis']['basis'],'measured');self.assertEqual(c['length_basis']['uncertainty'],.001)
        with self.assertRaises(ValueError):self.compile(provenance={'unknown':{}})
        # Preserve physical dimensions and reject insufficient backend delays.
        b['parameters']['header_length'][0]=.05
        text=''.join(f'{k} {i} {v:.17g}\n' for k,values in b['parameters'].items() for i,v in enumerate(values))
        b=from_native(native(self.contract,'import','i4',text))
        with self.assertRaisesRegex(ValueError,'Native preparation'):execute(self.compile(b),self.probe,rate=8000,temperature=4000)

    def test_prepared_generation_edits_and_runner_identity(self):
        b=copy.deepcopy(self.builds[1]);prepared=self.compile(b);original=prepared.manifest()
        mutated=prepared.manifest();mutated['components'].clear();self.assertTrue(prepared.manifest()['components'])
        b['generation']='9007199254740993';b['revision']='2'
        after=self.compile(b).manifest();self.assertEqual(after['generation'],'9007199254740993')
        self.assertNotEqual(after['build_sha256'],original['build_sha256'])
        b=from_native(native(self.contract,'export','i6',no_runners=True))
        self.assertEqual(self.compile(b).manifest()['components'],original['components'])
        # Native regenerate a changed header: compiler must propagate the
        # dimension to actual cells and paths without renaming other regions.
        b=copy.deepcopy(self.builds[0]);p=b['parameters'];p['header_length'][0]+=.1
        text=''.join(f'{k} {i} {v:.17g}\n' for k,values in p.items() for i,v in enumerate(values))
        b=from_native(native(self.contract,'import','i4',text));plan=self.compile(b).manifest()
        baseline=self.compile().manifest()
        for old,new in zip(baseline['components'],plan['components']):
            self.assertEqual(old['id'],new['id'])
            self.assertAlmostEqual(new['length_m']-old['length_m'],.1 if old['id']=='i4/header-0' else 0,places=12)
        for r in plan['paths']:
            if r['source_id']=='i4/cylinder-0/exhaust-valve':
                old=next(x for x in baseline['paths'] if x['source_id']==r['source_id'] and x['outlet_id']==r['outlet_id'])
                self.assertAlmostEqual(r['length_m']-old['length_m'],.1,places=12)

    def test_native_consumer_rejects_bad_plan_and_work_faults(self):
        prepared=self.compile();text=native_input(prepared)
        for value in ('0 0 0 0 0 0 0\n',text+'junk\n',text.replace('192000','nan',1)):
            result=subprocess.run([str(self.probe)],input=value,text=True,capture_output=True,timeout=10)
            self.assertNotEqual(result.returncode,0)
        # A changed serialized topology must fail against canonical path
        # identities, reachability or volume before any qualified use.
        d=prepared.manifest();d['components'][0]['area_m2']*=2
        with self.assertRaisesRegex(ValueError,'volume'):execute(PreparedGeometry(json.dumps(d)),self.probe)
