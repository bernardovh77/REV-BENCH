#!/usr/bin/env python3
"""Run source diagnostics with independently controlled engine/pipe/grid rates.

Production sources remain unchanged. A fixed-grid control is compiled from an
exactly checked, isolated copy of engine.c. Measurement files stay in renders/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "renders/evidence/audio-experiment-2026-10-22"
GRID_LINE = "const double grid_scale = fmin(1.0, RB_PIPE_RATE * e->pipe_dt);"
UPPER_RPM = {"i6": 7500, "v8-cross": 6500, "v8-flat": 8000}
CASES = {
    "baseline": (192000, 48000, False),
    "engine-time": (384000, 48000, False),
    "pipe-time": (192000, 96000, True),
    "space": (192000, 96000, False),
    "ultra": (384000, 96000, False),
}


def run(command, log):
    with log.open("w") as stream:
        subprocess.run([str(c) for c in command], cwd=ROOT, stdout=stream,
                       stderr=subprocess.STDOUT, check=True)


def build(directory):
    directory.mkdir(parents=True, exist_ok=True)
    run(["make", "-j2", f"BUILD={directory}", "LTO=0", "all"], directory / "build.log")
    includes = ["-Icore/include", "-Icore/src", "-Inative"]
    flags = ["-O3", "-std=c11", "-Wall", "-Wextra", "-ffp-contract=off"]
    core = sorted((directory / "core/src").glob("*.o"))
    if not core:
        raise RuntimeError("No core objects were built")
    original = ROOT / "core/src/engine.c"
    text = original.read_text()
    if text.count(GRID_LINE) != 1:
        raise RuntimeError("Grid fixture needs review against the current engine source")
    isolated = directory / "engine-fixed-grid.c"
    isolated.write_text(text.replace(GRID_LINE,
        "const double grid_scale = 1.0; /* diagnostic fixed-grid control only */"))
    fixed_object = directory / "engine-fixed-grid.o"
    run(["cc", *flags, *includes, "-c", isolated, "-o", fixed_object], directory / "grid-build.log")
    executables = {}
    for fixed in (False, True):
        objects = [fixed_object if fixed and p.name == "engine.o" else p for p in core]
        binary = directory / ("source-probe-fixed-grid" if fixed else "source-probe")
        run(["cc", *flags, *includes, ROOT / "tools/audio_source_probe.c", *objects,
             directory / "native/wav.o", "-lm", "-o", binary],
            directory / ("probe-fixed-build.log" if fixed else "probe-build.log"))
        executables[fixed] = binary
    return executables


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("/tmp/rev-bench-audio-experiment"))
    parser.add_argument("--output", type=Path, default=EVIDENCE / "engine")
    parser.add_argument("--cases", nargs="+", choices=CASES, default=list(CASES))
    parser.add_argument("--presets", nargs="+", choices=UPPER_RPM, default=list(UPPER_RPM))
    parser.add_argument("--rpms", choices=("3000", "upper", "both"), default="both")
    parser.add_argument("--settle", type=float, default=1.5)
    parser.add_argument("--seconds", type=float, default=0.75)
    parser.add_argument("--reuse", action="store_true", help="Reuse only exact-command completed cases")
    args = parser.parse_args()
    args.build_dir = args.build_dir.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    executables = build(args.build_dir)
    sources = list((ROOT / "core").rglob("*.c")) + list((ROOT / "core").rglob("*.h"))
    sources += [ROOT / "tools/audio_source_probe.c", Path(__file__).resolve()]
    source_hashes = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in sorted(sources)}
    records = []
    for case in args.cases:
        physics, pipe, fixed = CASES[case]
        for preset in args.presets:
            rpms = [3000] if args.rpms == "3000" else [UPPER_RPM[preset]]
            if args.rpms == "both":
                rpms = [3000, UPPER_RPM[preset]]
            for rpm in rpms:
                out = args.output / case / f"{preset}-{rpm}"
                out.mkdir(parents=True, exist_ok=True)
                command = [str(executables[fixed]), "--preset", preset, "--rpm", str(rpm),
                           "--physics-rate", str(physics), "--pipe-rate", str(pipe),
                           "--settle", str(args.settle), "--seconds", str(args.seconds),
                           "--out-dir", str(out)]
                if case == "baseline":
                    command.append("--verify-core")
                stamp = out / "command.json"
                identity = dict(command=command, source_hashes=source_hashes)
                artifacts = [out/"metadata.json"] + [out/f"{state}-{suffix}" for state in ("loaded", "cut")
                    for suffix in ("raw.wav", "after-aa.wav", "filtered.wav", "trace.f64")]
                if (args.reuse and stamp.exists() and all(p.exists() for p in artifacts)
                        and json.loads(stamp.read_text()) == identity):
                    records.append(dict(case=case, preset=preset, rpm=rpm, reused=True))
                    continue
                start = time.monotonic()
                run(command, out / "probe.log")
                stamp.write_text(json.dumps(identity, indent=2) + "\n")
                record = dict(case=case, preset=preset, rpm=rpm, physics_rate=physics,
                              pipe_rate=pipe, fixed_grid=fixed, wall_seconds=time.monotonic()-start)
                records.append(record)
                print(json.dumps(record), flush=True)
    label = "-".join(args.cases)+"_"+"-".join(args.presets)+"_"+args.rpms
    (args.output / f"resolution-runs-{label}.json").write_text(json.dumps(dict(
        method="Independent engine timestep; pipe timestep at fixed grid; grid at fixed pipe timestep. "
               "The ultra screen combines refinements. Noise sequences can differ with rate: compare "
               "statistics, not sample-wise residuals. See source metadata for actual grids.",
        source_hashes=source_hashes, runs=records), indent=2) + "\n")


if __name__ == "__main__":
    main()
