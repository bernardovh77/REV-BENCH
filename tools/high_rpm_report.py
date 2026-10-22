#!/usr/bin/env python3
"""Measure raw pull/coast recordings, preserving level and operating intervals.

Transient RPM crossings are not steady-state matched-load measurements.
Spectral centroid is brightness; firing-harmonic energy is reported separately.
Use the existing analysis environment. This tool never rewrites source audio.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

import numpy as np
from audio_wav import read_wav
from checkpoint_13b import REFERENCES
from measure_lift_feedback import spectral_metrics, telemetry

ROOT = Path(__file__).resolve().parents[1]
ENGINES = {"i6": (6, 8500, 7000), "v8-cross": (8, 7500, 7000), "v8-flat": (8, 9000, 9000)}


def window_metrics(audio, rate, rpm, cylinders):
    result = spectral_metrics(audio, rate)
    window = np.hanning(len(audio))
    power = np.mean(abs(np.fft.rfft(audio*window[:, None], axis=0))**2, axis=1)
    frequency = np.fft.rfftfreq(len(audio), 1/rate)
    total = power[(frequency >= 20) & (frequency < 8000)].sum()
    norm = len(audio)*(window@window)/2
    result["rpm_range"] = [min(rpm), max(rpm)]
    result["rpm_mean"] = float(np.mean(rpm))
    result["firing_hz"] = result["rpm_mean"] * cylinders / 120
    result["harmonics_db_relative"] = []
    for harmonic in range(1, 5):
        low, high = np.array(result["rpm_range"]) * cylinders * harmonic / 120
        half_bin = rate/len(audio)
        mask = (frequency >= low-half_bin) & (frequency <= high+half_bin)
        result["harmonics_db_relative"].append(float(10*np.log10(power[mask].sum()/total+1e-30)))
    upper = (frequency >= 1000) & (frequency < 4000)
    result["one_to_four_khz_dbfs"] = float(10*np.log10(power[upper].sum()/norm+1e-30))
    result["one_to_four_khz_db_relative"] = float(10*np.log10(power[upper].sum()/total+1e-30))
    return result


def report(folder, baseline=False):
    results = {"baseline": baseline, "s1_complete": False, "full_13b_complete": False,
        "caveat": "Raw gain 0.1, no listening normalization. Transient RPM crossings have different "
                  "acceleration/load histories; centroid is brightness, not pitch or acceptance.", "engines": []}
    for preset, (cylinders, new_limit, old_limit) in ENGINES.items():
        path = folder/(preset+"-long-coast.wav")
        audio, rate = read_wav(path)
        rows, lift = telemetry(path.with_suffix('.csv'))
        limit = old_limit if baseline else new_limit
        item = {"preset": preset, "path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                "gain": 0.1, "limit_rpm": limit, "lift_s": lift,
                "maximum_rpm": max(float(r['rpm']) for r in rows), "crossings": []}
        targets = sorted(set((3000, 5000, 6000, 6500, 7000, 7500, 8000, 8500, limit-500)))
        for rpm in targets:
            row = next((r for r in rows if float(r['rpm']) >= rpm and float(r['throttle']) > .9), None)
            if row is None:
                continue
            start = float(row['t_s'])
            end = start+.2
            if end > lift:
                continue
            speeds = [float(r['rpm']) for r in rows if start <= float(r['t_s']) < end]
            item['crossings'].append({"target_rpm": rpm, "interval_s": [start, end],
                **window_metrics(audio[round(start*rate):round(end*rate)], rate, speeds, cylinders)})
        item['lift_windows'] = {}
        for name, (a, b) in {"pre_lift": (-.8, -.2), "early_coast": (.2, .8),
                             "later_coast": (.8, 1.4)}.items():
            item['lift_windows'][name] = {"interval_s": [lift+a, lift+b],
                **spectral_metrics(audio[round((lift+a)*rate):round((lift+b)*rate)], rate)}
        reference, a, b, source_a, source_b = REFERENCES[preset]
        refpath = ROOT/'renders/real'/reference
        ref, fs = read_wav(refpath)
        item['reference'] = {"path": str(refpath), "source_interval_s": [source_a, source_b],
                            "local_interval_s": [a, b],
                            "caveat": "Unverified RPM/load, gain and cabin/exterior/flyby perspective; "
                                      "turbo M4 recording is not the naturally aspirated I6.",
                            **spectral_metrics(ref[round(a*fs):round(b*fs)], fs)}
        results['engines'].append(item)
    return results


def comparison(before, after, output):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from rbplot import setup, style
    setup()
    fig, axes = plt.subplots(3, 2, figsize=(11, 10), constrained_layout=True)
    for row, preset in enumerate(ENGINES):
        for label, report_data in (("accepted checkpoint", before), ("13b-r", after)):
            data = next(e for e in report_data['engines'] if e['preset'] == preset)['crossings']
            x = [e['rpm_mean'] for e in data]
            axes[row, 0].plot(x, [e['one_to_four_khz_db_relative'] for e in data], 'o-', label=label)
            axes[row, 1].plot(x, [e['harmonics_db_relative'][1] for e in data], 'o-', label=label)
        for col in range(2):
            style(axes[row, col])
            axes[row, col].set_xlabel('Mean rpm in 0.2-second pull window')
            axes[row, col].legend()
        axes[row, 0].set(title=preset+' — 1–4 kHz', ylabel='Energy relative to 20–8000 Hz, dB')
        axes[row, 1].set(title=preset+' — second firing harmonic', ylabel='Order-band energy relative to total, dB')
    fig.savefig(output, dpi=150)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, default=ROOT/'renders/current/feedback')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--compare', type=Path, help='saved baseline JSON; writes adjacent comparison PNG')
    args = parser.parse_args()
    data = report(args.input, args.baseline)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(data, indent=2, allow_nan=False)+'\n')
    if args.compare:
        comparison(json.loads(args.compare.read_text()), data, args.output.with_suffix('.png'))


if __name__ == '__main__':
    main()
