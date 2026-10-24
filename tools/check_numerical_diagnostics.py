"""Stage 038 native measurement gates; no audio normalization or fitted targets."""
import argparse
import json
from pathlib import Path
import statistics
import subprocess


def check(probe, ticks=48000, repeats=3):
    rows = []
    for preset in range(5):
        runs = {}
        for mode in range(3):
            runs[mode] = [json.loads(subprocess.check_output(
                [str(probe), str(preset), str(mode), str(ticks), str(int(mode > 0 and r == 0))],
                text=True)) for r in range(repeats)]
        control = runs[0][0]
        for mode in range(3):
            for row in runs[mode]:
                assert row['state'] == control['state'], (preset, mode, 'trajectory')
                assert abs(row['mass_error_kg']) < 2e-11*row['mass_scale_kg'], row
                assert max(map(abs, row['species_error_kg'])) < 2e-11*row['mass_scale_kg'], row
                assert abs(row['energy_error_J']) < 2e-9*row['energy_scale_J'], row
                if mode:
                    assert not row['invalid'], row
                    assert abs(row['tap_boundary_mass_error_kg']) < 2e-11*row['mass_scale_kg'], row
                    assert max(map(abs,row['tap_boundary_species_error_kg'])) < 2e-11*row['mass_scale_kg'], row
                    assert abs(row['tap_boundary_energy_error_J']) < 2e-9*row['energy_scale_J'], row
                    assert 0 < row['slots'] <= 2048 and row['trace_records'] <= 256, row
        record = runs[1][0]
        record['cost_cpu_s_median'] = {str(mode): statistics.median(x['cpu_s'] for x in runs[mode]) for mode in runs}
        record['cost_ratio_to_off'] = {str(mode): record['cost_cpu_s_median'][str(mode)]/record['cost_cpu_s_median']['0'] for mode in runs}
        record['trace_records_on'] = runs[2][0]['trace_records']
        record['trace_dropped_on'] = runs[2][0]['dropped']
        record['trace'] = runs[2][0]['trace']
        rows.append(record)
    return {'schema_version': 1, 'ticks': ticks, 'repeats': repeats, 'engines': rows}


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--ticks',type=int,default=48000)
    parser.add_argument('--repeats',type=int,default=3)
    args=parser.parse_args()
    if not 4 <= args.ticks <= 1920000 or not 1 <= args.repeats <= 10:
        parser.error('ticks 4..1920000, repeats 1..10')
    result=check(args.probe.resolve(), args.ticks, args.repeats)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print('Five engines: conservation, boundary taps, trajectory and capacities passed')
