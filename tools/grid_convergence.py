#!/usr/bin/env python3
"""How much of the pipes' sound the grid takes away (stage 144, area 1).

    python tools/grid_convergence.py RB_CLI OUTDIR [--presets i4,v8-cross,...] [--rpm 5000]
                                     [--throttle 1] [--seconds 5] [--fine ultra]
                                     [--coarse high] [--fine-cli RB_CLI]

Each preset is held at --rpm and --throttle on the dyno and heard from behind, the pipes'
sound alone (`--layers pipes`: no noise layer, no engine block), at the high quality (pipes
at 48 kHz, cells 5 and 7.5 cm) and at --fine (ultra: pipes at 96 kHz, cells half as long).
The same physics on two grids: where they agree the grid has converged; where the finer
grid is louder, the coarse one is taking that band away (numerical damping), and where the
level moves the other way its waves run at the wrong speed (dispersion) or its nulls fall in
other places.

--coarse sets the grid compared against (stage 148: studio against ultra), and --fine-cli a
second build for the fine grid (stage 148's reference, a scratch build whose studio has cells
an eighth of high's: whether studio itself has converged). Each build's renders go in their
own folder under OUTDIR.

It prints, per preset, each third-octave band's level on the fine grid against the coarse one
(dB) from 250 Hz to 8 kHz, after the first 2 s (the settling), and writes the table as CSV.
"""
import argparse
import csv
import os
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from audio_wav import read_wav  # noqa: E402

CENTRES = 1000.0 * 2.0 ** (np.arange(-6, 10) / 3.0)   # 250 Hz to 8 kHz


def render(cli, preset, quality, args, out):
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, f"{preset}-{quality}.wav")
    if not os.path.exists(path):
        cmd = [cli, "sound", "--preset", preset, "--hold", "--rpm", str(args.rpm), "--throttle",
               str(args.throttle), "--seconds", str(args.seconds), "--quality", quality,
               "--layers", "pipes", "--float", "-o", path]
        subprocess.run(cmd, check=True, capture_output=True, text=True)
    return path


def band_levels(path, skip):
    """Each band's mean power (dB, any reference: only differences are read)."""
    x, fs = read_wav(path)
    x = np.asarray(x, dtype=float)
    if x.ndim == 1:
        x = x[:, None]
    x = x[int(skip * fs):]
    x = x - x.mean(axis=0)
    window = np.hanning(len(x))
    power = np.mean([np.abs(np.fft.rfft(x[:, c] * window)) ** 2 for c in range(x.shape[1])], axis=0)
    f = np.fft.rfftfreq(len(x), 1.0 / fs)
    return np.array([10.0 * np.log10(power[(f >= c * 2 ** (-1 / 6)) & (f < c * 2 ** (1 / 6))].sum() + 1e-300)
                     for c in CENTRES])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("cli")
    ap.add_argument("out")
    ap.add_argument("--presets", default="i4,i6,v8-cross,v8-flat,boxer4,ferrari-458")
    ap.add_argument("--rpm", type=float, default=5000.0)
    ap.add_argument("--throttle", type=float, default=1.0)
    ap.add_argument("--seconds", type=float, default=5.0)
    ap.add_argument("--fine", default="ultra")
    ap.add_argument("--coarse", default="high")
    ap.add_argument("--fine-cli", default=None, help="another build for the fine grid")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    rows = []
    fine_cli = args.fine_cli or args.cli
    fine_out = os.path.join(args.out, "fine-cli") if args.fine_cli else args.out
    other = f" ({args.fine_cli})" if args.fine_cli else ""
    print(f"{args.fine}{other} against {args.coarse}, dB per third-octave band:")
    print("%-12s" % "preset" + "".join("%7.0f" % c for c in CENTRES))
    for preset in args.presets.split(","):
        coarse = band_levels(render(args.cli, preset, args.coarse, args, args.out), 2.0)
        fine = band_levels(render(fine_cli, preset, args.fine, args, fine_out), 2.0)
        d = fine - coarse
        rows.append([preset] + ["%.2f" % v for v in d])
        print("%-12s" % preset + "".join("%7.1f" % v for v in d))
    with open(os.path.join(args.out, "grid_convergence.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["preset"] + ["%.0f" % c for c in CENTRES])
        w.writerows(rows)


if __name__ == "__main__":
    main()
