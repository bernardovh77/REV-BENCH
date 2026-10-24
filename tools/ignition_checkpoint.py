"""Stage040 raw-stereo checkpoint comparison; descriptive metrics, not a realism score."""
import argparse
import csv
import json
from pathlib import Path
import numpy as np
from audio_wav import read_wav, read_wav_bytes
from reference_protocol import digest, measurements
from render_results import expected_clips


def measure(path):
    if read_wav_bytes(path)[1] != 3:
        raise ValueError("Measurements require unfiltered float WAV originals")
    x, rate = read_wav(path)
    if rate != 48000:
        raise ValueError('Expected the frozen48kHz render rate')
    result = measurements(x)
    result.update(sha256=digest(path), frames=len(x), channels=x.shape[1],
                  duration_s=len(x)/rate, peak=float(np.max(abs(x))))
    return x, result


def db_ratio(a, b):
    return float(10*np.log10(max(a, 1e-300)/max(b, 1e-300)))


def deltas(before, after):
    pa = np.array(before['mean_channel_band_power'])
    pb = np.array(after['mean_channel_band_power'])
    return dict(rms_db=db_ratio(after['mean_channel_power'], before['mean_channel_power']),
                upper_band_share_db=db_ratio(pb[2:].sum()/max(pb.sum(), 1e-300),
                                            pa[2:].sum()/max(pa.sum(), 1e-300)),
                duration_s=after['duration_s']-before['duration_s'])


def compare(before, after):
    rows = []
    for engine, scenario, name in expected_clips():
        a, ma = measure(before/name)
        b, mb = measure(after/name)
        item = dict(engine=engine, scenario=scenario, path=name, before=ma, after=mb,
                    identical=ma['sha256']==mb['sha256'], deltas=deltas(ma, mb))
        if scenario == 'long-coast':
            item['lift_windows'] = {}
            for label, folder, audio in (('before', before, a), ('after', after, b)):
                with (folder/name).with_suffix('.csv').open() as stream:
                    csv_rows = list(csv.DictReader(stream))
                lifted = next(r for r in csv_rows if float(r['throttle']) == 0)
                lift = float(lifted['t_s'])
                windows = {}
                for title, (start, end) in {'pre_lift':(-.8,-.2), 'first_200ms':(0,.2),
                                          'early_coast':(.2,.8), 'later_coast':(.8,1.4)}.items():
                    windows[title] = measurements(audio[round((lift+start)*48000):round((lift+end)*48000)])
                item['lift_windows'][label] = dict(lift_s=lift, lift_rpm=float(lifted['rpm']),
                    windows=windows, level_step_db=db_ratio(windows['early_coast']['mean_channel_power'],
                                                          windows['pre_lift']['mean_channel_power']))
        rows.append(item)
    return dict(scope='Same scenarios/controls/gains, differing engine revisions; unfiltered48kHz float channels, average powers. '
                      'Whole-clip spectral changes and pedal-lift windows are descriptive. '
                      'Different speed trajectories are not matched-RPM causal timbre scores; no reference fitting.',
                clips=rows, count=len(rows), identical=sum(x['identical'] for x in rows),
                all_finite=True, all_below_full_scale=all(x['after']['peak']<1 for x in rows),
                before_duration_s=sum(x['before']['duration_s'] for x in rows),
                after_duration_s=sum(x['after']['duration_s'] for x in rows))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--before', type=Path, required=True)
    p.add_argument('--after', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    result = compare(args.before, args.after)
    args.out.write_text(json.dumps(result, indent=2)+'\n')
    if not result['all_below_full_scale']:
        raise ValueError('Checkpoint requires headroom inspection before publication')
    print(f"Compared{result['count']} raw clips; {result['identical']} identical; "
          f"{result['after_duration_s']:.3f}s; all finite with headroom")


if __name__ == '__main__':
    main()
