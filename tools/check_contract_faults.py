#!/usr/bin/env python3
"""Plant Stage 037 faults in temporary copies, never the active sources."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FAULTS = {
    'numeric-pipe-identities': ('tools/build_contract.py',
        "cid = f'{namespace}/{suffix}'", "cid = f'{namespace}/pipe-{len(components)}'", 'native'),
    'mixed-generation-snapshot': ('tools/build_contract.py',
        "snapshot['generation'] == build['generation']", 'True', 'unit'),
    'inconsistent-audio-clock': ('tools/build_contract.py',
        "(tick-pt)*snapshot['audio_hz'] == (frame-af)*snapshot['physics_hz']", 'True', 'unit'),
    'stale-history-after-commit': ('tools/build_contract.py',
        'self.active = prepared\n        self.last_snapshot = None',
        'self.active = prepared\n        # fault: retain previous generation history', 'unit'),
    'rpm-treated-as-radians-per-second': ('tools/build_contract_probe.c',
        '#define RPM (3.14159265358979323846/30.0)', '#define RPM 1.0', 'c'),
}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--probe', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    records = []
    files = ('tools/build_contract.py', 'tools/test_build_contract.py', 'tools/check_build_contract.py',
             'tools/build_contract_probe.c', 'tools/build_contract_fields.inc',
             'contracts/engine-fields-v1.json', 'contracts/examples/builds-v1.json')
    for name, (file, before, after, mode) in FAULTS.items():
        with tempfile.TemporaryDirectory(prefix='rev-bench-037-fault-') as tmp:
            root = Path(tmp)
            for item in files:
                target = root/item; target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(ROOT/item, target)
            target = root/file; source = target.read_text()
            if source.count(before) != 1: raise ValueError(f'{name}: fault anchor not unique')
            target.write_text(source.replace(before, after))
            if target.suffix == '.py': compile(target.read_text(), str(target), 'exec')
            probe = args.probe.resolve()
            if mode == 'c':
                mutated = root/'probe'
                objects = sorted((probe.parent/'core/src').glob('*.o'))
                if not objects: raise ValueError('Build native probe first; object files required')
                subprocess.run(['gcc', '-O3', '-std=c11', '-ffp-contract=off',
                                '-I'+str(ROOT/'core/include'), '-I'+str(ROOT/'core/src'),
                                str(target), *map(str, objects), '-lm', '-o', str(mutated)], check=True)
                probe = mutated
            command = ([sys.executable, '-m', 'unittest', 'test_build_contract'] if mode == 'unit'
                       else [sys.executable, 'tools/check_build_contract.py', '--probe', str(probe)])
            result = subprocess.run(command, cwd=root, env=dict(os.environ, PYTHONPATH=str(root/'tools')),
                                    text=True, capture_output=True, timeout=60)
            (args.output/f'{name}.log').write_text(result.stdout+result.stderr)
            if result.returncode == 0: raise ValueError(f'{name}: MISSED')
            records.append(dict(fault=name, caught=True, mode=mode, returncode=result.returncode))
    (args.output/'summary.json').write_text(json.dumps(records, indent=2)+'\n')
    print(json.dumps(records, indent=2))


if __name__ == '__main__':
    main()
