#!/usr/bin/env python3
"""Record researched exhaust controls and extended coasts; keep float WAVs raw.

The before executable must be preserved from the preceding commit. Runs are
sequential; absolute recording gains are never adjusted to match a reference.
No reference file is rewritten. Analysis needs the existing numpy environment.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ("i4", "i6", "v8-cross", "v8-flat", "boxer4")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(cli, output, name, options):
    wav = output / (name + ".wav")
    csv = output / (name + ".csv")
    executable_sha = sha(cli)
    command = [str(cli.resolve()), "sound", *options, "--float", "--gain", "0.1",
               "--out", str(wav), "--csv", str(csv)]
    run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=900)
    log = run.stdout + run.stderr
    (output / (name + ".log")).write_text(log)
    run.check_returncode()
    if "soft-limited samples 0, non-finite 0" not in log:
        raise ValueError(f"{name}: inspect limiter/non-finite diagnostics")
    return {"command": command, "executable_sha256": executable_sha,
            "wav_sha256": sha(wav), "csv_sha256": sha(csv)}


def deceleration_report(output, previous_coasts=None, constant_coasts=None):
    import numpy as np
    import matplotlib.pyplot as plt
    from audio_wav import read_wav
    from measure_lift_feedback import spectral_metrics
    from checkpoint_13b import REFERENCES
    from rbplot import style

    fig, axes = plt.subplots(5, 2, figsize=(12, 15), constrained_layout=True)
    report = {"listening_acceptance": "pending Bernardo", "s1_complete": False,
              "caveat": "Time aligned to pedal lift, not matched RPM/load. References have unverified "
                        "gain, cabin/exterior/flyby and ECU state. Centroid measures brightness, not pitch.",
              "engines": []}
    for row, preset in enumerate(PRESETS):
        item = {"preset": preset, "variants": []}
        for side, path, gain in (
            ("previous checkpoint", previous_coasts/(preset+"-long-pull-coast.wav") if previous_coasts else None, .15),
            ("new pipes, constant boundary", constant_coasts/(preset+"-long-coast.wav") if constant_coasts else None, .1),
            ("new pipes, radiation", output/(preset+"-long-coast.wav"), .1),
            ("packed rear control", output/(preset+"-packed-control.wav"), .1)):
            if path is None or not path.exists():
                continue
            audio, rate = read_wav(path)
            if not np.all(np.isfinite(audio)):
                raise ValueError(f"nonfinite {path}")
            with path.with_suffix('.csv').open() as stream:
                rows = list(csv.DictReader(stream))
            lifted = next(r for r in rows if float(r['throttle']) == 0.0)
            lift = float(lifted['t_s'])
            windows = {}
            for name, (a,b) in {"pre_lift": (-.8,-.2), "first_200ms": (0,.2),
                               "early_coast": (.2,.8), "later_coast": (.8,1.4)}.items():
                clip = audio[round((lift+a)*rate):round((lift+b)*rate)]
                windows[name] = spectral_metrics(clip,rate)
            variant = {"name": side, "path": str(path), "sha256": sha(path), "gain": gain,
                       "lift_s": lift, "lift_rpm": float(lifted['rpm']), "windows": windows,
                       "band_step_db": windows['early_coast']['overrun_band_dbfs']
                                       - windows['pre_lift']['overrun_band_dbfs'], "pull_windows": []}
            for rpm in (3000,5000,6500):
                crossing = next((r for r in rows if float(r['rpm']) >= rpm and float(r['throttle']) > .9),None)
                if crossing is not None:
                    start = float(crossing['t_s'])
                    clip = audio[round(start*rate):round((start+.2)*rate)]
                    variant['pull_windows'].append({"crossing_rpm":rpm, "interval_s":[start,start+.2],
                                                    **spectral_metrics(clip,rate)})
            item['variants'].append(variant)
            times = np.arange(-1,1.901,.05)
            metrics = [spectral_metrics(audio[round((lift+t)*rate):round((lift+t+.1)*rate)],rate) for t in times]
            # Scalar correction is plotted separately; the input WAV and all
            # stored window measurements retain the original render gain.
            axes[row,0].plot(times+.05,[m['overrun_band_dbfs']-20*np.log10(gain) for m in metrics],label=side)
            axes[row,1].plot(times+.05,[m['centroid_hz'] for m in metrics],label=side)
        if preset in REFERENCES:
            name,a,b,source_a,source_b = REFERENCES[preset]
            path=ROOT/'renders/real'/name
            audio,rate=read_wav(path)
            item['reference']={"path":str(path),"sha256":sha(path),"local_interval_s":[a,b],
                               "source_interval_s":[source_a,source_b],
                               **spectral_metrics(audio[round(a*rate):round(b*rate)],rate)}
        for col in range(2):
            style(axes[row,col])
            axes[row,col].axvline(0,color='gray',ls=':')
            axes[row,col].set_xlabel('seconds relative to pedal lift')
            axes[row,col].legend(fontsize=7)
        axes[row,0].set(title=preset+' — 600–1600 Hz',ylabel='dBFS corrected to gain 1')
        axes[row,1].set(title=preset+' — spectral brightness',ylabel='centroid, Hz')
        report['engines'].append(item)
    fig.savefig(output/'deceleration.png',dpi=150)
    (output/'deceleration.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", type=Path, required=True)
    parser.add_argument("--before-cli", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "renders/work/exhaust-feedback")
    parser.add_argument("--previous-coasts", type=Path, help="optional preserved historical coast controls")
    parser.add_argument("--constant-coasts", type=Path, help="optional geometry-only radiation controls")
    parser.add_argument("--analyze", action="store_true")
    parser.add_argument("--presets", nargs="+", choices=PRESETS, default=PRESETS)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    if not args.analyze:
        manifest = args.output / "recordings.json"
        rows = json.loads(manifest.read_text()) if manifest.exists() else {}
        for preset in args.presets:
            for side, cli in (("before", args.before_cli), ("after", args.cli)):
                if cli is None:
                    continue
                name = preset + "-3000-" + side
                print("Recording", name, flush=True)
                rows[name] = record(cli, args.output, name,
                    ["--preset", preset, "--hold", "--rpm", "3000", "--seconds", "4"])
            name = preset + "-long-coast"
            print("Recording", name, flush=True)
            rows[name] = record(args.cli, args.output, name,
                ["--preset", preset, "--sweep", "--pull-seconds", "20" if preset == "i6" else "12", "--coast-seconds", "8",
                 "--seconds", "45", "--scene", "asphalt", "--inertia", "3"])
            manifest.write_text(json.dumps(rows, indent=2) + "\n")
        if "v8-cross" in args.presets:
            name = "v8-cross-packed-control"
            print("Recording", name, flush=True)
            rows[name] = record(args.cli, args.output, name,
                ["--preset", "v8-cross", "--sweep", "--pull-seconds", "12", "--coast-seconds", "8",
                 "--seconds", "45", "--scene", "asphalt", "--inertia", "3", "--muffler", "straight"])
            manifest.write_text(json.dumps(rows, indent=2) + "\n")
        return

    import numpy as np
    from audio_wav import read_wav
    from compare_real import measure
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from rbplot import setup, style
    setup()
    fig, axes = plt.subplots(5, 1, figsize=(9, 14), constrained_layout=True)
    rows = []
    for ax, preset in zip(axes, PRESETS):
        style(ax)
        pair = {}
        for side in ("before", "after"):
            path = args.output / (preset + "-3000-" + side + ".wav")
            if not path.exists():
                continue
            audio, rate = read_wav(path)
            if not np.all(np.isfinite(audio)):
                raise ValueError(f"nonfinite {path}")
            mono = audio.mean(axis=1)
            clip = mono[rate:3*rate]
            metrics = measure(path, 1.0, 3.0, repeats=False)
            pair[side] = {"path": str(path), "sha256": sha(path), "interval_s": [1, 3],
                "rms": float(np.sqrt(np.mean(clip*clip))), "peak": float(np.max(np.abs(audio))),
                "above_2k_db": float(metrics["above"][1]), "slope_db_oct": float(metrics["slope"])}
            spec = np.abs(np.fft.rfft(clip*np.hanning(len(clip))))**2
            freq = np.fft.rfftfreq(len(clip), 1/rate)
            # Relative spectral shape only; measurements above retain the raw gain.
            edges = np.geomspace(25, 16000, 100)
            power = np.array([spec[(freq >= a) & (freq < b)].sum() for a,b in zip(edges,edges[1:])])
            ax.semilogx(np.sqrt(edges[:-1]*edges[1:]), 10*np.log10(power/spec.sum()+1e-30), label=side)
        ax.set(title=preset + " — held 3000 rpm, render gain 0.1", xlabel="Hz", ylabel="band energy, dB")
        ax.set_xlim(25, 16000)
        ax.set_ylim(-85, 0)
        ax.legend()
        if len(pair) == 2:
            a, _ = read_wav(Path(pair["before"]["path"]))
            b, _ = read_wav(Path(pair["after"]["path"]))
            n = min(len(a), len(b))
            pair["raw_difference_rms"] = float(np.sqrt(np.mean((a[:n]-b[:n])**2)))
            pair["rms_change_db"] = float(20*np.log10(pair["after"]["rms"]/pair["before"]["rms"]))
        rows.append({"preset": preset, **pair})
    fig.savefig(args.output / "spectra.png", dpi=160)
    (args.output / "measurements.json").write_text(json.dumps(rows, indent=2) + "\n")
    deceleration_report(args.output, args.previous_coasts, args.constant_coasts)


if __name__ == "__main__":
    main()
