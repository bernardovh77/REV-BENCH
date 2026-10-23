"""Public contract checks; fixtures are actual native five-preset descriptions."""
import copy
import json
import math
from pathlib import Path
import unittest

import build_contract as contract


def examples():
    return json.loads((contract.ROOT/'contracts/examples/builds-v1.json').read_text())


def snapshot(build):
    return dict(schema_version=1, build_id=build['build_id'], generation=build['generation'], build_revision=build['revision'],
                sequence='1', physics_tick='192000', physics_hz=192000, physics_origin='0',
                audio_frame='48000', audio_hz=48000, audio_origin='0',
                cycle_index='25', phase_rad=0., speed_rad_s=100*math.pi,
                cylinders=[dict(pressure_Pa=101325., temperature_K=300., burn_fraction=0.)
                           for _ in range(build['parameters']['cylinders'][0])])


class SharedContracts(unittest.TestCase):
    def test_five_native_descriptions_round_trip_without_losing_annotations(self):
        for build in examples():
            with self.subTest(engine=build['build_id']):
                build['extensions']['x-notes'] = {'perspective': 'unresolved'}
                self.assertEqual(contract.loads(contract.dumps(build)), build)

    def test_catalog_uses_si_including_angles_speed_and_controller_gains(self):
        fields = contract.catalog()
        i6 = examples()[1]['parameters']
        self.assertEqual(fields['rev_limit']['unit'], 'rad/s')
        self.assertEqual(fields['intake_duration']['unit'], 'rad')
        self.assertEqual(fields['idle_gain_p']['unit'], 's/rad')
        self.assertAlmostEqual(i6['rev_limit'][0], 8500*math.pi/30, places=12)
        self.assertEqual(i6['cylinders'], [6])

    def test_versions_features_and_unknown_topology_reject(self):
        edits = [lambda b: b.update(schema_version=2),
                 lambda b: b.update(required_features=['turbo']),
                 lambda b: b['geometry']['components'][0].update(kind='perforated-cross-pipe'),
                 lambda b: b['geometry'].update(frame='pixels'),
                 lambda b: b.update(silent_unknown=True)]
        for edit in edits:
            build = examples()[0]; edit(build)
            with self.assertRaises(ValueError):
                contract.validate_build(build)

    def test_duplicate_dangling_and_reoriented_ports_reject(self):
        edits = [lambda c: c['ports'][0].update(node_id='missing'),
                 lambda c: c['ports'][0].update(axis_sign=-1),
                 lambda c: c['ports'][1].update(id=c['ports'][0]['id'])]
        for edit in edits:
            build = examples()[0]; edit(build['geometry']['components'][0])
            with self.assertRaises(ValueError):
                contract.validate_build(build)

    def test_unknown_spatial_data_cannot_be_used_as_a_3d_route(self):
        with self.assertRaisesRegex(ValueError, 'placement'):
            contract.validate_build(examples()[0], require_spatial=True)
        build = examples()[0]
        build['geometry']['engine_to_vehicle'] = dict(translation_m=[0., 0., 0.], rotation_xyzw=[0., 0., 0., 1.])
        for c in build['geometry']['components']:
            c['centerline_m'] = [[0., 0., 0.], [0., c['length_m'], 0.]]
        contract.validate_build(build, require_spatial=True)
        build['geometry']['components'][0]['centerline_m'][1][1] += .1
        with self.assertRaisesRegex(ValueError, 'acoustic length'):
            contract.validate_build(build)

    def test_json_capacity_duplicates_nonfinite_and_integer_loss_reject(self):
        for text in ('{"x":1,"x":2}', '{"x":NaN}', ' '* (contract.MAX_JSON+1)):
            with self.assertRaises(ValueError): contract.loads(text)
        for generation in (9007199254740993, '18446744073709551616', '01'):
            build = examples()[0]; build['generation'] = generation
            with self.assertRaises(ValueError): contract.validate_build(build)
        build = examples()[0]; build['generation'] = '9007199254740993'
        self.assertEqual(contract.loads(contract.dumps(build))['generation'], '9007199254740993')

    def test_snapshot_has_one_generation_clock_and_complete_cylinder_set(self):
        build = examples()[1]
        good = snapshot(build)
        contract.validate_snapshot(good, build)
        for key, value in [('generation', '2'), ('build_revision', '2'), ('audio_frame', '48001'), ('phase_rad', 4*math.pi),
                           ('physics_hz', 96000), ('cylinders', [])]:
            bad = copy.deepcopy(good); bad[key] = value
            with self.assertRaises(ValueError): contract.validate_snapshot(bad, build)

    def test_wrapped_phase_preserves_unwrapped_cycle_and_snapshot_order(self):
        build = examples()[0]; first = snapshot(build)
        first.update(phase_rad=4*math.pi-.01, cycle_index='25')
        next_one = copy.deepcopy(first)
        next_one.update(sequence='2', physics_tick='192004', audio_frame='48001', phase_rad=.01, cycle_index='26')
        contract.validate_snapshot(next_one, build, first)
        next_one['cycle_index'] = '25'
        with self.assertRaisesRegex(ValueError, 'backwards'): contract.validate_snapshot(next_one, build, first)
        with self.assertRaisesRegex(ValueError, 'Stale'): contract.validate_snapshot(first, build, first)

    def test_transaction_failure_preserves_active_state_and_commit_invalidates_history(self):
        build = examples()[0]; session = contract.ReferenceSession(build)
        session.accept_snapshot(snapshot(build))
        candidate = copy.deepcopy(build); candidate['generation'] = '2'
        candidate['required_features'] = ['unsupported']
        with self.assertRaises(ValueError): session.commit(candidate, '1')
        self.assertEqual(session.active, build)
        self.assertIsNotNone(session.last_snapshot)
        candidate['required_features'] = []
        with self.assertRaisesRegex(ValueError, 'Stale'): session.commit(candidate, '0')
        session.commit(candidate, '1')
        self.assertIsNone(session.last_snapshot)
        with self.assertRaisesRegex(ValueError, 'generation'): session.accept_snapshot(snapshot(build))
        candidate['parameters']['bore'][0] = 123
        self.assertNotEqual(session.active['parameters']['bore'][0], 123)

    def test_actual_event_clock_keeps_command_discharge_and_flame_distinct(self):
        build = examples()[0]
        event = dict(schema_version=1, build_id='i4', generation='1', build_revision='1', sequence='1',
                     kind='spark-discharge', actor_id='i4/cylinder-0', cycle_id='1', physics_tick='100',
                     subtick=.25, cycle_index='0', phase_rad=1.)
        contract.validate_event(event, build)
        for key, value in [('kind', 'random-pop'), ('actor_id', 'i4/cylinder-99'),
                           ('generation', '2'), ('subtick', 1.)]:
            bad = dict(event); bad[key] = value
            with self.assertRaises(ValueError): contract.validate_event(bad, build)

    def test_build_revision_changes_on_descriptor_edit_but_not_cold_reset(self):
        build = examples()[0]; session = contract.ReferenceSession(build)
        edited = copy.deepcopy(build)
        edited['generation'] = '2'
        edited['parameters']['accessory_power'][0] += 1
        with self.assertRaisesRegex(ValueError, 'revision'): session.commit(edited, '1')
        edited['revision'] = '2'
        session.commit(edited, '1')
        reset = copy.deepcopy(edited); reset['generation'] = '3'
        session.commit(reset, '2')
        self.assertEqual(session.active['revision'], '2')

    def test_audible_arrival_separates_physical_and_digital_time(self):
        # 20 ms gas, 10 ms air, 3 ms DSP; device/queue already in timestamp.
        self.assertAlmostEqual(contract.audible_performance_time(1, .020, .010, .003, 1, 4), 4.033)
        # The same pulse stamped at the aperture is already 20 ms later.
        self.assertAlmostEqual(contract.audible_performance_time(1.020, 0, .010, .003, 1, 4), 4.033)


if __name__ == '__main__':
    unittest.main()
