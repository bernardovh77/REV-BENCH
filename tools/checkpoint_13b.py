#!/usr/bin/env python3
"""Reproduce the local listening checkpoint; original reference audio is never rewritten.

Run make render RENDER_DIR=renders/checkpoint-13b/mono RENDER_FLAGS=--float first.
Then pass --cli PATH --render to record controls/scenes; --analyze creates metrics,
paired listening aids and plots. numpy/matplotlib are the existing analysis deps.
Reference windows describe falling pitch, not verified throttle, RPM or perspective.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

import numpy as np

from audio_wav import read_wav
from compare_real import measure

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ("i6", "v8-cross", "v8-flat")
SCENES = ("asphalt", "grass", "wall", "garage", "dyno", "iso", "passby", "cabin")
# Visually inspected descending harmonics in the unfiltered 12-second windows.
# Absolute source intervals; no claim of matched RPM/load or camera position.
REFERENCES = {
    "i6": ("user-bmw-m4-brighter-242.wav", 8.0, 11.0, 250.0, 253.0),
    "v8-cross": ("user-mustang-gt-darker-257.wav", 0.0, 6.0, 257.0, 263.0),
    "v8-flat": ("user-ferrari458-brighter-302.wav", 0.0, 8.0, 302.0, 310.0),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_float(path, samples, rate):
    data = np.asarray(samples, dtype="<f4")
    channels = data.shape[1]
    payload = data.tobytes()
    fmt = struct.pack("<HHIIHH", 3, channels, rate, rate*channels*4, channels*4, 32)
    path.write_bytes(b"RIFF" + struct.pack("<I", 36+len(payload)) + b"WAVEfmt "
                     + struct.pack("<I", 16) + fmt + b"data"
                     + struct.pack("<I", len(payload)) + payload)


def record(args, folder, name, options):
    folder.mkdir(parents=True, exist_ok=True)
    command = [str(args.cli.resolve()), "sound", *options, "--float", "--out",
               str(folder / (name+".wav")), "--csv", str(folder / (name+".csv"))]
    print("Recording", name, flush=True)
    r = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=900)
    (folder / (name+".log")).write_text(r.stdout+r.stderr)
    r.check_returncode()
    if "soft-limited samples 0, non-finite 0" not in r.stdout:
        raise ValueError(f"{name}: inspect limiter/non-finite diagnostics before listening")
    return dict(command=command, wav_sha256=sha(folder / (name+".wav")))


def render(args):
    rows = {}
    for p in PRESETS:
        base = ["--preset", p, "--rev", "--seconds", "12", "--gain", "0.2"]
        rows[p+"-immediate"] = record(args, args.output/"controls", p+"-immediate",
                                       base+["--overrun-transition", "0"])
        rows[p+"-stereo"] = record(args, args.output/"decel-stereo", p,
                                    base+["--scene", "asphalt"])
    for s in SCENES:
        rows[s] = record(args, args.output/"scenes", s,
                         ["--preset", "v8-cross", "--hold", "--rpm", "3000",
                          "--seconds", "12", "--scene", s, "--gain", "0.12"])
    rows["ultra"] = record(args, args.output/"quality", "v8-cross-ultra",
                           ["--preset", "v8-cross", "--hold", "--rpm", "3000",
                            "--seconds", "4", "--quality", "ultra", "--gain", "0.2"])
    rows["high"] = record(args, args.output/"quality", "v8-cross-high",
                          ["--preset", "v8-cross", "--hold", "--rpm", "3000",
                           "--seconds", "4", "--quality", "high", "--gain", "0.2"])
    (args.output/"recordings.json").write_text(json.dumps(rows, indent=2)+"\n")


def analyze(args):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    folder = args.output/"listening"
    folder.mkdir(parents=True, exist_ok=True)
    report = {"s1_complete": False, "sound_acceptance": "pending Bernardo's listening",
              "reference_caveat": "Falling pitch only; RPM/load, cabin/exterior/flyby unverified. "
                                   "BMW turbo and current I6 naturally aspirated.", "pairs": []}
    fig, axes = plt.subplots(3, 3, figsize=(12, 8))
    for row, preset in enumerate(PRESETS):
        reference, t0, t1, source0, source1 = REFERENCES[preset]
        refpath = ROOT/"renders/real"/reference
        old = ROOT/"renders/s1d/after-renders"/(preset+"-rev.wav")
        new = args.output/"mono"/(preset+"-rev.wav")
        paths = [refpath, old, new]
        intervals = [(t0,t1), (6.9,9.4), (6.9,9.4)]
        clips = []
        metrics = []
        for col, (path, (a,b)) in enumerate(zip(paths, intervals)):
            x, fs = read_wav(path)
            if fs != 48000:
                raise ValueError(f"unexpected rate in {path}")
            clip = x[round(a*fs):round(b*fs)].copy()
            if not len(clip):
                raise ValueError(f"empty interval in {path}")
            clips.append(clip)
            # Numerical inputs remain unfiltered and at the render/source gain.
            m = measure(path,a,b,repeats=False)
            metrics.append({"path":str(path.relative_to(ROOT)),"sha256":sha(path),
                            "interval_s":[a,b],"above_2k_db":float(m["above"][1]),
                            "slope_db_oct":float(m["slope"]),"crest_db":float(m["crest"])})
            axes[row,col].specgram(clip[:,0],NFFT=2048,Fs=fs,noverlap=1536,cmap="magma")
            axes[row,col].set_ylim(60,4000)
            axes[row,col].set_title(preset+": "+("real falling pitch","S1d neutral rev","current neutral rev")[col],fontsize=10)
            axes[row,col].set_xlabel("seconds within selected interval")
        # A single gain across both simulation excerpts. Reference gain is
        # independent because the original microphone level is uncalibrated.
        gains = [0.6/float(np.max(np.abs(clips[0])))]
        shared = 0.6/max(float(np.max(np.abs(c))) for c in clips[1:])
        gains += [shared,shared]
        played = []
        for clip,gain in zip(clips,gains):
            clip *= gain
            fade = min(480,len(clip)//2)
            clip[:fade] *= np.linspace(0,1,fade)[:,None]
            clip[-fade:] *= np.linspace(1,0,fade)[:,None]
            played.extend([clip,np.zeros((24000,1))])
        output = folder/(preset+"-reference-before-after.wav")
        write_float(output,np.concatenate(played[:-1]),48000)
        report["pairs"].append({"preset":preset,"absolute_reference_interval_s":[source0,source1],
            "order":"real reference, 0.5 s gap, S1d, 0.5 s gap, current",
            "listening_only_processing":"10 ms fades, static gain; no pitch/EQ/denoising",
            "listening_gains":gains,"measurement_inputs":metrics,
            "wav":str(output.relative_to(ROOT)),"sha256":sha(output)})
    fig.suptitle("Qualitative references and neutral revs: operating conditions are not matched")
    fig.tight_layout()
    fig.savefig(args.output/"deceleration-comparison.png",dpi=140)
    (args.output/"comparison.json").write_text(json.dumps(report,indent=2,allow_nan=False)+"\n")
    print("Wrote listening pairs and comparison.json; no listening approval is implied.")


def long_coast(args):
    import csv
    rows = []
    for preset in PRESETS:
        name = preset+"-long-pull-coast"
        row = record(args,args.output/"long-coast",name,
                     ["--preset",preset,"--sweep","--coast-seconds","8",
                      "--pull-seconds","12","--seconds","45","--scene","asphalt",
                      "--inertia","3","--gain","0.15"])
        with (args.output/"long-coast"/(name+".csv")).open() as stream:
            telemetry = list(csv.DictReader(stream))
        lift = next(r for r in telemetry if float(r["throttle"])==0.0)
        clutch = next(r for r in telemetry if float(r["crank_inertia_kgm2"])<2.9)
        row.update(preset=preset,lift_s=float(lift["t_s"]),clutch_s=float(clutch["t_s"]),
                   final_s=float(telemetry[-1]["t_s"]),lift_rpm=float(lift["rpm"]),
                   clutch_rpm=float(clutch["rpm"]),
                   processing="Stereo asphalt scene; standard float output at gain 0.15; "
                              "no time stretch, normalization, extra filtering or denoising.")
        if not 7.98 < row["clutch_s"]-row["lift_s"] < 8.02:
            raise ValueError("long coast did not retain the requested vehicle inertia interval")
        rows.append(row)
    (args.output/"long-coast/manifest.json").write_text(json.dumps(rows,indent=2)+"\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", type=Path, default=ROOT/"build/rb_cli")
    parser.add_argument("--output", type=Path, default=ROOT/"renders/checkpoint-13b")
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--analyze", action="store_true")
    parser.add_argument("--long-coast", action="store_true",
                        help="three longer sweeps with 8-second in-gear deceleration")
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True,exist_ok=True)
    if args.render:
        render(args)
    if args.analyze:
        analyze(args)
    if args.long_coast:
        long_coast(args)


if __name__ == "__main__":
    main()
