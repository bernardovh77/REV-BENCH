#!/usr/bin/env python3
"""Stage 118: what a faster quality changes in the sound, against the default.

    python tools/tier_compare.py run RB_CLI OUTDIR [--presets i4,...] [--tier low] [--base high]
    python tools/tier_compare.py report OUTDIR [--tier low] [--base high]

`run` renders each preset through `rb_cli sound` (mono, heard from behind, as
the sound checks always are) on three seeded drives, at the default quality
(BASE), at the TIER, and at BASE again with another variation seed, for the
noise floor: how much each number moves between two renders of the same engine
that differ only by their random draws. `report` measures every WAV like
tools/compare_real.py and tools/orders.py do, and prints the tier's difference
from the default next to that floor, against the tolerances of docs/app-design.md
section 15:

    each compare_real measure (the band shares above 1, 2 and 4 kHz, the
    treble's slope, the cycle's repetition) and each orders measure: 1 dB
    (the slope: 1 dB per octave); loudness (A-weighted level): 1 dB

The drives: idle (5 s after the starter lets go), held 3000 rpm wide open on
the dyno, and a rev in neutral (the whole 12 s). The repetition and the
orders are measured on the idle and the held drives only, as compare_real does.
"""
import argparse
import concurrent.futures as cf
import json
import os
from pathlib import Path
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

PRESETS = {"i4": 4, "i6": 6, "v8-cross": 8, "v8-flat": 8, "boxer4": 4}
# drive: (rb_cli arguments, window t0, t1, repeats rpm range, orders rpm half-width)
DRIVES = {
    "idle": (["--idle", "--seconds", "8"], 3.0, 8.0),
    "hold": (["--hold", "--rpm", "3000", "--seconds", "5"], 0.5, 4.5),
    "rev": (["--rev", "--seconds", "12"], 0.0, 12.0),
}
RESEED = "2"
# Qualities that aren't one of rb_cli's four, as the arguments that make them:
# physics at 48 kHz times the oversample, the pipes at the default 48 kHz.
VARIANTS = {
    "os2": ["-x", "2"],       # physics 96 kHz, pipes 48 kHz
    "os3": ["-x", "3"],       # physics 144 kHz, pipes 48 kHz
    # stage 118b's V8 search: low's 96 kHz physics with the pipes slower than its 24 kHz
    "low-p19": ["--quality", "low", "--pipe-rate", "19200"],
    "low-p16": ["--quality", "low", "--pipe-rate", "16000"],
}


def quality_args(quality):
    return VARIANTS[quality] if quality in VARIANTS else ["--quality", quality]


def job(cli, out, preset, quality, drive, seed):
    name = f"{preset}-{quality}-{drive}" + (f"-seed{seed}" if seed else "")
    wav = out / f"{name}.wav"
    if wav.exists():   # a render already made (the default's, when another tier is tried)
        return name, 0
    args = [cli, "sound", "--preset", preset, *quality_args(quality), *DRIVES[drive][0], "--float", "-o", str(wav)]
    if seed:
        args += ["--seed", seed]
    r = subprocess.run(args, capture_output=True, text=True, encoding="utf-8")
    (out / f"{name}.txt").write_text(r.stdout + r.stderr, encoding="utf-8")
    return name, r.returncode


def run(a):
    out = Path(a.outdir)
    out.mkdir(parents=True, exist_ok=True)
    jobs = []
    for preset in a.presets.split(","):
        for drive in DRIVES:
            jobs += [(a.cli, out, preset, a.base, drive, None), (a.cli, out, preset, a.tier, drive, None),
                     (a.cli, out, preset, a.base, drive, RESEED)]
    with cf.ThreadPoolExecutor(max_workers=a.jobs) as pool:
        for name, code in pool.map(lambda j: job(*j), jobs):
            print(f"{name}: exit {code}", flush=True)


def a_level(path, t0, t1):
    """The A-weighted level of a stretch, dB re full scale (mean of the channels' power)."""
    import numpy as np
    from compare_real import a_weighting, read_wav
    x, fs = read_wav(path)
    x = x[int(t0 * fs):int(t1 * fs)]
    x = x - x.mean(axis=0)
    f = np.fft.rfftfreq(len(x), 1.0 / fs)
    power = np.mean([np.mean(np.fft.irfft(np.fft.rfft(x[:, c]) * a_weighting(f), len(x)) ** 2)
                     for c in range(x.shape[1])])
    return 10.0 * np.log10(power + 1e-30)


def measures(path, preset, drive):
    """{measure: value} for one render, the numbers the tolerances name."""
    from compare_real import measure
    from orders import orders
    _args, t0, t1 = DRIVES[drive]
    steady = drive != "rev"
    rpm = (450.0, 1600.0) if drive == "idle" else (1500.0, 7000.0)
    m = measure(str(path), t0, t1, repeats=steady, rpm=rpm)
    out = {">1k": m["above"][0], ">2k": m["above"][1], ">4k": m["above"][2], "slope": m["slope"],
           "A loss": m["a_loss"], "crest": m["crest"], "level_dBA": a_level(path, t0, t1)}
    if steady:
        out.update({"repeat lo": m["repeat"][0], "repeat mid": m["repeat"][1], "repeat hi": m["repeat"][2]})
        lo, hi = (500.0, 1300.0) if drive == "idle" else (2800.0, 3200.0)
        _rpm, _share, others, _top = orders(str(path), PRESETS[preset], t0, t1, lo, hi)
        out["orders others"] = others
    return out


def report(a):
    out = Path(a.outdir)
    rows = {}
    for preset in PRESETS:
        for drive in DRIVES:
            files = {k: out / f"{preset}-{q}-{drive}{s}.wav" for k, q, s in
                     (("base", a.base, ""), ("tier", a.tier, ""), ("reseed", a.base, f"-seed{RESEED}"))}
            if not all(p.exists() for p in files.values()):
                continue
            rows[(preset, drive)] = {k: measures(p, preset, drive) for k, p in files.items()}
    (out / "measures.json").write_text(json.dumps({f"{p}/{d}": v for (p, d), v in rows.items()}, indent=1))
    tol = lambda name: 1.0   # dB (the slope: dB per octave)
    print(f"tier {a.tier} against {a.base}; floor = {a.base} against itself with another seed\n")
    print(f"{'preset':9s} {'drive':5s} {'measure':14s} {a.base:>9s} {a.tier:>9s} {'diff':>6s} {'floor':>6s}  tol")
    worst = {}
    for (preset, drive), v in rows.items():
        for name in v["base"]:
            if name in ("A loss", "crest"):
                continue   # reported by compare_real, not named in the tolerances
            d = v["tier"][name] - v["base"][name]
            floor = v["reseed"][name] - v["base"][name]
            ok = abs(d) <= tol(name)
            print(f"{preset:9s} {drive:5s} {name:14s} {v['base'][name]:9.2f} {v['tier'][name]:9.2f} {d:+6.2f} "
                  f"{floor:+6.2f}  {'ok' if ok else 'OVER'}")
            w = worst.setdefault(name, [0.0, 0.0, 0, 0])
            w[0] = max(w[0], abs(d))
            w[1] = max(w[1], abs(floor))
            w[2] += 1
            w[3] += (not ok)
    print("\nworst over every preset and drive (|diff|, |floor|, cells over tolerance / cells):")
    for name, (d, f, n, over) in worst.items():
        print(f"  {name:14s} {d:5.2f} {f:5.2f}   {over}/{n}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("cli")
    r.add_argument("outdir")
    r.add_argument("--presets", default=",".join(PRESETS))
    r.add_argument("--jobs", type=int, default=3)
    p = sub.add_parser("report")
    p.add_argument("outdir")
    for s in (r, p):
        s.add_argument("--tier", default="low")
        s.add_argument("--base", default="high")
    a = ap.parse_args()
    (run if a.cmd == "run" else report)(a)
