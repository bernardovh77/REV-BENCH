#!/usr/bin/env python3
"""Stage 037 native five-preset round-trip and identity/adverse-case proof."""
import argparse
import copy
import json
import math
from pathlib import Path
import subprocess

from build_contract import ROOT, catalog, dumps, from_native, loads, require, validate_build


def native(probe, command, preset, text=None, no_runners=False):
    args = [str(probe), command, preset] + (['--no-runners'] if no_runners else [])
    result = subprocess.run(args, input=text, text=True, capture_output=True, timeout=30)
    require(result.returncode == 0, f'Native descriptor rejected: {result.returncode}')
    return loads(result.stdout)


def prepare_build(probe, build):
    """Native full-descriptor validity plus derived logical-topology agreement."""
    validate_build(build)
    require(build['build_id'] in ('i4', 'i6', 'v8-cross', 'v8-flat', 'boxer4'), 'Unsupported proof build namespace')
    text = ''.join(f'{name} {i} {v:.17g}\n' for name, values in build['parameters'].items()
                   for i, v in enumerate(values))
    raw = native(probe, 'import', build['build_id'], text)
    require(raw['catalog'] == catalog(), 'Native contract catalog differs')
    derived = from_native(raw, build['generation'])
    derived['revision'] = build['revision']
    # Spatial annotations cannot change the legacy logical gas topology.
    observed = copy.deepcopy(build['geometry'])
    observed['engine_to_vehicle'] = None
    for c in observed['components']:
        c['centerline_m'] = None
    require(observed == derived['geometry'] and build['sources'] == derived['sources'], 'Unsupported/inconsistent logical topology')
    for name, values in build['parameters'].items():
        for before, after in zip(values, raw['parameters'][name]):
            require(math.isclose(before, after, rel_tol=1e-14, abs_tol=1e-15), 'Descriptor round-trip differs')
    return derived


def check(probe):
    examples = loads((ROOT/'contracts/examples/builds-v1.json').read_text())
    records = []
    for build in examples:
        raw = native(probe, 'export', build['build_id'])
        require(raw['catalog'] == catalog(), 'Native field catalog differs')
        require(from_native(raw) == build, 'Committed example differs from current native preset')
        restored = prepare_build(probe, loads(dumps(build)))
        no_runners = from_native(native(probe, 'export', build['build_id'], no_runners=True))
        prepare_build(probe, no_runners)
        before = {c['id']: c for c in restored['geometry']['components'] if c['kind'] != 'runner'}
        after = {c['id']: c for c in no_runners['geometry']['components']}
        require(before == after, 'Runner removal changed unrelated component or port identities')
        require(build['sources'] == no_runners['sources'], 'Runner removal changed sources')
        # Structurally valid, recognized topology must also agree with the core.
        wrong = copy.deepcopy(build)
        port = wrong['geometry']['components'][0]['ports'][0]
        port['node_id'] = next(n['id'] for n in wrong['geometry']['nodes'] if n['id'] != port['node_id'])
        for rejected in (wrong, dict(build, parameters=dict(build['parameters'], rod=[.001]))):
            try: prepare_build(probe, rejected)
            except ValueError: pass
            else: raise ValueError('Native/topology adverse control was accepted')
        records.append(dict(preset=build['build_id'], fields=len(catalog()),
                            components=len(before), round_trip=True, identities_preserved=True,
                            inconsistent_topology_and_impossible_engine_rejected=True))
    return dict(schema_version=1, engines=records, scope='native contract proof; production integration is pending')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = check(args.probe.resolve())
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
