#!/usr/bin/env python3
"""Build and run the isolated S2d-A/B0 candidates; production audio is unchanged.

Writes exact commands, source hashes, rejected cases and numeric evidence below
ignored renders/. No training, automatic promotion, or production parameter edits.
"""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / "renders/evidence/acoustic-candidates-2026-10-23"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    with log.open("w") as stream:
        subprocess.run([str(x) for x in command], cwd=ROOT, stdout=stream,
                       stderr=subprocess.STDOUT, check=True)


def read_row(text):
    rows = list(csv.DictReader(io.StringIO(text)))
    if len(rows) != 1 or None in rows[0] or any(v is None for v in rows[0].values()):
        raise ValueError("candidate must produce exactly one complete CSV row")
    return rows[0]


def cases(quick=False):
    """Physical matrix is independent of numerical rate/grid choices."""
    fv = []
    for temp in (300, 900):
        for flow in (-70, 0, 70):
            for frequency in ((800, 2000) if quick else (200, 800, 2000, 4000)):
                for dx in ((.0375, .009375) if quick else (.075, .0375, .01875, .009375)):
                    fv.append(("matrix", ["fv", dx, 192000, temp, flow, frequency, 10, .0025, 33.6]))
    if not quick:
        for temp in (300, 900):
            for flow in (-70, 0, 70):
                fv.append(("half-amplitude", ["fv", .01875, 192000, temp, flow, 800, 5, .0025, 33.6]))
                for rate in (48000, 96000, 384000):
                    fv.append(("time-control", ["fv", .0375, rate, temp, flow, 800, 10, .0025, 33.6]))
        for duration in (.01, .05, .1):
            fv.append(("growth-control", ["fv", .009375, 192000, 900, 70, 4000, 5, duration, 33.6]))
        # Periodic energy controls after packet wrap; phase/speed are not interpreted.
        fv.append(("long-energy-control", ["fv", .009375, 192000, 900, 70, 4000, 5, 1, 9.6]))
    wave = []
    for temp in (300, 900):
        for rate in ((48000,) if quick else (48000, 96000, 192000)):
            for segments in ((1, 4) if quick else (1, 4, 16)):
                for frequency in ((800, 4000) if quick else (200, 800, 2000, 4000, 12000, 16000)):
                    for interpolation in (0, 1):
                        wave.append(("transfer", ["wave", rate, temp, frequency, 1, segments, interpolation, .2]))
        for interpolation in (0, 1):
            wave.append(("long-closed-loop", ["loop", 48000, temp, 1000, 1, 4, interpolation, 1]))
    return fv, wave


def write_csv(path, rows):
    if not rows:
        raise ValueError("empty experiment")
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def check_topology_identity(original, changed):
    """Removing intake runners must not rename an exhaust component or geometry."""
    if original["unknown_connections"] or changed["unknown_connections"]:
        raise ValueError("unknown topology connection")
    for engine in original["engines"]:
        other = next(e for e in changed["engines"] if e["preset"] == engine["preset"])
        exhaust = lambda e: {p["component"]: p for p in e["pipes"]
                            if p["kind"] not in ("runner", "snorkel")}
        a, b = exhaust(engine), exhaust(other)
        if a.keys() != b.keys():
            raise ValueError("exhaust components changed during intake-only identity control")
        for key in a:
            if a[key]["id"] != b[key]["id"] or a[key]["id"] != f'{engine["preset"]}/{key}':
                raise ValueError("exhaust identity depends on unrelated pipe ordinal")
            for field in ("length_m", "area_m2"):
                if a[key][field] != b[key][field]:
                    raise ValueError("exhaust geometry changed during intake-only identity control")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("/tmp/rev-bench-s2d"))
    parser.add_argument("--output", type=Path, default=DEFAULT)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--reuse", action="store_true", help="Reuse exact-source/command cases only")
    args = parser.parse_args()
    build, out = args.build_dir.resolve(), args.output.resolve()
    build.mkdir(parents=True, exist_ok=True)
    (out / "cases").mkdir(parents=True, exist_ok=True)
    run(["make", "-j2", f"BUILD={build}", "LTO=0", "acoustic-probe"], out / "build.log")
    binary = build / "acoustic_candidate_probe"
    sources = sorted(list((ROOT / "core").rglob("*.c")) + list((ROOT / "core").rglob("*.h"))
                     + list((ROOT / "experiments/acoustics").glob("*.[ch]"))
                     + [ROOT / "tools/acoustic_candidate_probe.c", ROOT / "Makefile"])
    hashes = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    run([binary, "topology"], out / "production-topology.json")
    topology = json.loads((out / "production-topology.json").read_text())
    if topology["unknown_connections"] or len(topology["engines"]) != 5:
        raise RuntimeError("production topology snapshot failed")
    run([binary, "topology-no-runners"], out / "identity-topology-no-runners.json")
    check_topology_identity(topology, json.loads((out / "identity-topology-no-runners.json").read_text()))
    old = ROOT / "renders/evidence/audio-experiment-2026-10-22"
    commands = sorted((old / "engine").rglob("command.json"))
    baseline = {"available": False}
    if commands and (old / "transport/transport.csv").exists():
        stamp = json.loads(commands[0].read_text())
        original = {k: v for k, v in stamp["source_hashes"].items() if k.startswith("core/")}
        if not original or any(hashes.get(k) != v for k, v in original.items()):
            raise RuntimeError("historical A0 core differs: recapture transport controls")
        target = out / "a0-transport.csv"
        target.write_bytes((old / "transport/transport.csv").read_bytes())
        baseline = dict(available=True, csv_sha256=digest(target),
                        identity_source=str(commands[0].relative_to(ROOT)),
                        verified_core_files=len(original),
                        timing_scope="historical paired base/packet wall times; not a candidate speedup benchmark")
    families = cases(args.quick)
    records = []
    for family, family_cases in zip(("fv", "wave"), families):
        rows = []
        for index, (group, arguments) in enumerate(family_cases):
            name = f"{family}-{index:03d}"
            command = [str(binary), *map(str, arguments)]
            identity = dict(command=command, source_hashes=hashes)
            stamp, log = out / "cases" / f"{name}.json", out / "cases" / f"{name}.csv"
            start = time.monotonic()
            reused = (args.reuse and stamp.exists() and log.exists()
                      and json.loads(stamp.read_text()) == identity)
            if not reused:
                result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
                (out / "cases" / f"{name}.stderr").write_text(result.stderr)
                row = read_row(result.stdout)
                if result.returncode and row["status"] != "failed":
                    raise RuntimeError(f"{name}: unexpected exit {result.returncode}")
                log.write_text(result.stdout)
                stamp.write_text(json.dumps(identity, indent=2) + "\n")
            row = read_row(log.read_text())
            rows.append(dict(case=name, group=group, **row))
            records.append(dict(case=name, group=group, status=row["status"],
                                wall_seconds=time.monotonic()-start, reused=reused))
            if index % 24 == 0:
                print(f"{family}: {index+1}/{len(family_cases)}", flush=True)
        write_csv(out / f"{family}.csv", rows)
    manifest = dict(schema=1, stage="S2d initial native candidates", quick=args.quick,
                    source_revision=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    source_hashes=hashes, binary_sha256=digest(binary),
                    compiler=subprocess.check_output(["cc", "--version"], text=True).splitlines()[0],
                    flags="Makefile -O3 -g -ffp-contract=off; LTO=0; no fast-math",
                    historical_A0=baseline, records=records, semantic_exhaust_id_control=True,
                    limitations=["A1 freezes gamma at each reference gas state; engine mixture/EOS adapter not implemented",
                                 "B0 is homogeneous, quiescent and linear; no mean flow, thermal fronts or engine coupling",
                                 "transverse modes and real component material losses not validated",
                                 "candidate timings are fixture timings, not complete-engine or browser speedups",
                                 "production snapshot is not an acoustic compiler or complete production cap trace"])
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Completed {len(records)} cases in {out}", flush=True)


if __name__ == "__main__":
    main()
