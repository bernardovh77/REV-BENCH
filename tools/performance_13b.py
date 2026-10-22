#!/usr/bin/env python3
"""Interleave preserved before/after benchmarks; verify unfiltered WAV identity.

The benchmark times engine physics only, not browser deadlines or stereo audio.
Keep the machine otherwise idle. All binaries, logs and audio stay in renders/.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ("i4", "i6", "v8-cross", "v8-flat", "boxer4")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bench(args):
    commands = {
        "native-before": [str(args.before_native.resolve())],
        "native-after": [str(args.after_native.resolve())],
        "wasm-before": [args.node, "tools/bench_wasm.mjs", str(args.before_wasm.resolve())],
        "wasm-after": [args.node, "tools/bench_wasm.mjs", str(args.after_wasm.resolve())],
    }
    binaries = {"native-before": args.before_native, "native-after": args.after_native,
                "wasm-before": args.before_wasm, "wasm-after": args.after_wasm}
    rows = []
    for round_number in range(args.rounds):
        order = list(commands)
        if round_number % 2:
            order = order[1::-1] + order[:1:-1]
        for name in order:
            command = commands[name] + ["--seconds", str(args.seconds)]
            print(f"Round {round_number+1}: {name}", flush=True)
            run = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                 timeout=300, check=True)
            log = args.output / f"bench-{round_number+1}-{name}.txt"
            log.write_text(run.stdout + run.stderr)
            for line in run.stdout.splitlines():
                fields = line.split()
                if len(fields) == 5 and fields[0] in PRESETS:
                    rows.append(dict(round=round_number+1, build=name, preset=fields[0],
                                     cells=int(fields[1]), seconds_per_second=float(fields[3])))
    summary = {}
    for target in ("native", "wasm"):
        summary[target] = {}
        for preset in PRESETS:
            values = {}
            for side in ("before", "after"):
                data = [r["seconds_per_second"] for r in rows
                        if r["build"] == target+"-"+side and r["preset"] == preset]
                if len(data) != args.rounds:
                    raise ValueError(f"missing benchmark rows: {target}/{side}/{preset}")
                values[side] = statistics.median(data)
                values[side+"_range"] = [min(data), max(data)]
            values["speedup"] = values["before"] / values["after"]
            values["before_realtime"] = 1 / values["before"]
            values["after_realtime"] = 1 / values["after"]
            summary[target][preset] = values
    result = dict(host=platform.platform(), rounds=args.rounds, seconds=args.seconds,
                  method="Median of interleaved runs, each internally best of three; same rates/cells.",
                  caveat="Engine-only throughput; shared-host timings vary. Not an AudioWorklet deadline test.",
                  binary_sha256={name: sha(path) for name, path in binaries.items()},
                  commands=commands, rows=rows, summary=summary)
    (args.output / "benchmarks.json").write_text(json.dumps(result, indent=2)+"\n")


def compare(args):
    rows = []
    for folder in ("mono", "long-coast"):
        expected = sorted((args.before_audio / folder).glob("*.wav"))
        if not expected:
            raise ValueError(f"no baseline WAVs in {args.before_audio/folder}")
        for before in expected:
            after = args.output / folder / before.name
            if not after.exists():
                raise ValueError(f"missing output: {after}")
            rows.append(dict(path=f"{folder}/{before.name}", before_sha256=sha(before),
                             after_sha256=sha(after), identical=before.read_bytes()==after.read_bytes()))
    result = dict(before=str(args.before_audio.resolve()), after=str(args.output.resolve()),
                  processing="Original unfiltered float WAVs, no gain, alignment or spectral transforms.",
                  all_identical=all(row["identical"] for row in rows), rows=rows)
    (args.output / "audio-identity.json").write_text(json.dumps(result, indent=2)+"\n")
    print(f"{sum(r['identical'] for r in rows)}/{len(rows)} WAVs byte-identical")
    if not result["all_identical"]:
        raise SystemExit("Audio differs: inspect numerical and listening comparisons before acceptance")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=("bench", "compare"))
    p.add_argument("--output", type=Path, default=ROOT/"renders/stage-13b")
    p.add_argument("--before-audio", type=Path, default=ROOT/"renders/feedback-13b")
    p.add_argument("--before-native", type=Path, default=ROOT/"renders/stage-13b/before-bench")
    p.add_argument("--after-native", type=Path, default=ROOT/"build/rb_bench")
    p.add_argument("--before-wasm", type=Path, default=ROOT/"renders/stage-13b/before-bench.wasm")
    p.add_argument("--after-wasm", type=Path, default=ROOT/"build/stage-13b-wasm/rb_bench.wasm")
    p.add_argument("--node", default="node")
    p.add_argument("--rounds", type=int, default=3)
    p.add_argument("--seconds", type=float, default=0.3)
    args = p.parse_args()
    if not 1 <= args.rounds <= 10 or not 0 < args.seconds <= 60:
        p.error("rounds must be 1..10 and seconds 0..60")
    args.output.mkdir(parents=True, exist_ok=True)
    (bench if args.action == "bench" else compare)(args)


if __name__ == "__main__":
    main()
