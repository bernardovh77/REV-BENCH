#!/usr/bin/env python3
"""Measure unfiltered before/after sweeps; write separate listening-only comparisons.

Requires tools/requirements-analysis.txt. No filtering, normalisation or fades are
applied to measurement inputs. Reference viewpoints and operating states remain
unverified. Brightness is not a measurement of perceived pitch or acceptance.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ('i6', 'v8-cross', 'v8-flat')


def spectral_metrics(samples, rate):
    import numpy as np
    window = np.hanning(len(samples))
    # Average the ears' powers; do not create interaural cancellation by
    # summing their pressures before measuring the spectrum.
    power = np.mean(abs(np.fft.rfft(samples * window[:,None], axis=0)) ** 2, axis=1)
    frequency = np.fft.rfftfreq(len(samples), 1 / rate)
    audible = (frequency >= 20) & (frequency < 8000)
    prominent = (frequency >= 600) & (frequency < 1600)
    norm = len(samples) * (window @ window) / 2
    return {
        'rms_dbfs': float(20 * np.log10(np.sqrt(np.mean(samples ** 2)) + 1e-30)),
        'overrun_band_dbfs': float(10 * np.log10(power[prominent].sum() / norm + 1e-30)),
        'centroid_hz': float((frequency[audible] * power[audible]).sum() / power[audible].sum()),
        'above_2k_db': float(10 * np.log10(power[(frequency >= 2000) & audible].sum()
                                         / power[audible].sum() + 1e-30)),
    }


def telemetry(path):
    with path.open() as stream:
        rows = list(csv.DictReader(stream))
    index = next(i for i, row in enumerate(rows) if float(row['throttle']) == 0)
    # Pedal changes at the start of the first closed block, not its CSV end.
    lift = float(rows[index - 1]['t_s']) if index else 0.0
    return rows, lift


def excerpt(x, rate, a, b):
    if a < 0 or b <= a or round(b * rate) > len(x):
        raise ValueError(f'incomplete requested interval {a}..{b}')
    return x[round(a * rate):round(b * rate)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, default=ROOT/'renders/checkpoint-13b/long-coast')
    parser.add_argument('--after', type=Path, default=ROOT/'renders/feedback-13b/long-coast')
    parser.add_argument('--output', type=Path, default=ROOT/'renders/feedback-13b')
    args = parser.parse_args()
    try:
        import numpy as np
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        from audio_wav import read_wav
        from checkpoint_13b import write_float, REFERENCES
    except ModuleNotFoundError as error:
        parser.exit(2, f'Analysis needs {error.name}; install tools/requirements-analysis.txt '
                      'in .venv-analysis and use its Python.\n')
    args.output.mkdir(parents=True, exist_ok=True)
    listening = args.output/'listening'
    listening.mkdir(exist_ok=True)
    fig, axes = plt.subplots(3, 2, figsize=(11, 9))
    report = {'s1_complete': False, 'listening_acceptance': 'pending Bernardo',
              'diagnostic_band_hz': [600, 1600],
              'caveat': 'Band chosen after inspecting initial renders; diagnostic, not a universal '
                        'perceptual target. References are not matched RPM/load/viewpoints. '
                        'Centroid describes brightness, not perceived pitch.', 'engines': []}
    for row, preset in enumerate(PRESETS):
        record = {'preset': preset, 'variants': []}
        clips = []
        for variant, folder, color in [('before', args.before, '#777777'),
                                        ('after', args.after, '#176d9c')]:
            path = folder/(preset+'-long-pull-coast.wav')
            x, rate = read_wav(path)
            rows, lift = telemetry(path.with_suffix('.csv'))
            windows = {}
            for name, (a,b) in {'pre_lift': (-.8,-.2), 'first_200ms': (0,.2),
                               'early_coast': (.2,.8), 'later_coast': (.8,1.4)}.items():
                windows[name] = spectral_metrics(excerpt(x,rate,lift+a,lift+b), rate)
            item = {'variant': variant, 'path': str(path.relative_to(ROOT)),
                    'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                    'lift_s': lift, 'measurement_windows': windows,
                    'band_step_db': windows['early_coast']['overrun_band_dbfs']
                                    - windows['pre_lift']['overrun_band_dbfs']}
            if preset == 'v8-cross':
                item['pull_windows'] = []
                for rpm in (3000,5000,6500):
                    start = next(float(r['t_s']) for r in rows if float(r['rpm']) >= rpm)
                    item['pull_windows'].append({'crossing_rpm': rpm, 'interval_s': [start,start+.2],
                        **spectral_metrics(excerpt(x,rate,start,start+.2),rate)})
                start = next(float(r['t_s']) for r in rows if float(r['rpm']) >= 4500)
                clip = excerpt(x,rate,start,lift+.5).copy()
            else:
                clip = excerpt(x,rate,lift-1,lift+2).copy()
            clips.append(clip)
            record['variants'].append(item)
            times = np.arange(-1.0, 1.91, .025)
            metrics = [spectral_metrics(excerpt(x,rate,lift+t,lift+t+.1),rate) for t in times]
            axes[row,0].plot(times+.05,[m['overrun_band_dbfs'] for m in metrics],
                             color=color,label=variant)
            axes[row,1].plot(times+.05,[m['centroid_hz'] for m in metrics],
                             color=color,label=variant)
        gain = .65/max(float(np.max(abs(c))) for c in clips)
        played = []
        for clip in clips:
            clip *= gain
            fade = min(round(.01*rate),len(clip)//2)
            clip[:fade] *= np.linspace(0,1,fade)[:,None]
            clip[-fade:] *= np.linspace(1,0,fade)[:,None]
            played.extend((clip,np.zeros((round(.5*rate),clip.shape[1]))))
        output = listening/(preset+'-before-after.wav')
        write_float(output,np.concatenate(played[:-1]),rate)
        reference,a,b,source_a,source_b = REFERENCES[preset]
        refpath = ROOT/'renders/real'/reference
        ref,ref_rate = read_wav(refpath)
        record['reference'] = {'path': str(refpath.relative_to(ROOT)),
            'source_interval_s': [source_a,source_b], 'local_interval_s': [a,b],
            'conditions': 'Falling pitch candidate only; RPM/load and cabin/exterior/flyby unverified.',
            **spectral_metrics(excerpt(ref,ref_rate,a,b),ref_rate)}
        record['listening'] = {'path': str(output.relative_to(ROOT)), 'order': 'before, after',
            'processing': 'One shared static gain, 10 ms edge fades, 0.5 s silence; listening only.',
            'shared_gain': gain}
        report['engines'].append(record)
        for column in range(2):
            axes[row,column].axvline(0,color='#333333',ls=':',lw=1)
            axes[row,column].set_xlabel('Seconds relative to pedal lift')
            axes[row,column].legend()
        axes[row,0].set_title(preset+' — 600–1600 Hz band')
        axes[row,0].set_ylabel('dBFS (same render gain 0.15)')
        axes[row,1].set_title(preset+' — spectral brightness, 20 Hz–8 kHz')
        axes[row,1].set_ylabel('Centroid, Hz')
    fig.suptitle('Native unfiltered sweeps: old and revised momentum transport\n'
                 'Cross-plane V8 also changes from packed to unfilled exhaust; listening approval pending')
    fig.tight_layout()
    fig.savefig(args.output/'lift-feedback.png',dpi=140)
    (args.output/'measurements.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print('Wrote measurements.json, lift-feedback.png and separate listening comparisons.')


if __name__ == '__main__':
    main()
