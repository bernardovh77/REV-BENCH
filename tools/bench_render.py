#!/usr/bin/env python3
"""Stage 118's benchmark of the app's render path, interleaved, as a table.

    python tools/bench_render.py --exe NAME=PATH [--exe NAME=PATH ...] [options]
    python tools/bench_render.py --cores            what the CPU's cores are

One command makes the whole table. `rb_bench_render` (native/bench_render.c)
times rb_render_stereo through the public API for each preset, quality and
drive; this runs it for every build given, cell by cell and round by round
(A, B, A, B ... for each cell), so that anything else the machine does while
it runs falls on the builds alike, and keeps each build's best round. Options:

    --presets i4,i6     the presets (default: all)
    --quality low,...   qualities (default low,medium,high)
    --drive idle,...    drives (default idle,hold,rev)
    --rounds N          rounds per cell (default 3); each runs the program's
                        own --runs 1, so the best of N is the table's figure
    --cores LIST        pin to these classes, one table each: p (performance
                        cores), e (efficiency cores), any (no pinning, the
                        default). A hybrid CPU's render thread may land on
                        either, and they differ by about 2x.
    --csv FILE          the cells as CSV (build, cores, preset, quality,
                        drive, x_real, worst_ms, p99_ms, hash)
    --md FILE           the tables as Markdown
    --ftz               pass --ftz (flush denormals to zero) to the program

Builds whose renders should be bit-identical (the same compiler before and
after a speed-up) are compared through the hash the program prints: a cell
whose hashes differ between builds is marked, and the exit code is 1. Builds
of different compilers are expected to differ: name them with different
prefixes before a colon (gcc:before=..., gcc:after=...) and only builds with
the same prefix are compared.
"""
import argparse
import csv
import ctypes
import io
import os
import re
from pathlib import Path
import subprocess
import sys


def core_masks():
    """{'p': mask, 'e': mask} of logical processors by efficiency class, from
    Windows (the highest class is the performance cores); {} elsewhere or if
    all cores are alike."""
    if os.name != "nt":
        return {}
    k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    needed = ctypes.c_ulong(0)
    k32.GetLogicalProcessorInformationEx(0, None, ctypes.byref(needed))
    buf = ctypes.create_string_buffer(needed.value)
    if not k32.GetLogicalProcessorInformationEx(0, buf, ctypes.byref(needed)):
        return {}
    classes = {}
    off = 0
    while off < needed.value:
        size = int.from_bytes(buf.raw[off + 4:off + 8], "little")
        # PROCESSOR_RELATIONSHIP at +8: Flags(1) EfficiencyClass(1) Reserved(20) GroupCount(2) GroupMask[]
        eff = buf.raw[off + 9]
        mask = int.from_bytes(buf.raw[off + 32:off + 40], "little")
        classes[eff] = classes.get(eff, 0) | mask
        off += size
    if len(classes) < 2:
        return {}
    hi, lo = max(classes), min(classes)
    return {"p": classes[hi], "e": classes[lo]}


def run(exe, args, mask):
    cmd = [exe, *args, "--runs", "1"]
    if mask:
        cmd += ["--affinity", f"{mask:x}"]
    out = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
    if out.returncode != 0:
        sys.exit(f"{exe}: exit {out.returncode}\n{out.stderr}")
    return list(csv.DictReader(io.StringIO(out.stdout)))[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", action="append", default=[], help="NAME=PATH (or GROUP:NAME=PATH)")
    ap.add_argument("--presets", default="")
    ap.add_argument("--quality", default="low,medium,high")
    ap.add_argument("--drive", default="idle,hold,rev")
    ap.add_argument("--rounds", type=int, default=3)
    ap.add_argument("--cores", default="any")
    ap.add_argument("--csv")
    ap.add_argument("--md")
    ap.add_argument("--ftz", action="store_true")
    ap.add_argument("--list-cores", action="store_true")
    a = ap.parse_args()
    masks = core_masks()
    if a.list_cores:
        print({k: f"{v:x}" for k, v in masks.items()} or "no hybrid cores found")
        return 0
    if not a.exe:
        ap.error("at least one --exe")
    builds = []
    for spec in a.exe:
        name, _, path = spec.partition("=")
        builds.append((name, path, name.partition(":")[0] if ":" in name else name))
    first = builds[0][1]
    presets = a.presets.split(",") if a.presets else [
        m.group(1) for m in (re.match(r"^  (\S+)\s+\d", line) for line in subprocess.run(
            [str(Path(first).with_name("rb_cli" + Path(first).suffix)), "presets"],
            capture_output=True, text=True).stdout.splitlines()) if m]
    rows, bad = [], 0
    for cores in a.cores.split(","):
        mask = masks.get(cores, 0)
        if cores != "any" and not mask:
            sys.exit(f"--cores {cores}: this machine has no such core class (--list-cores)")
        for preset in presets:
            for q in a.quality.split(","):
                for d in a.drive.split(","):
                    best = {}
                    for _ in range(a.rounds):
                        for name, path, _group in builds:
                            args = [preset, "--quality", q, "--drive", d] + (["--ftz"] if a.ftz else [])
                            r = run(path, args, mask)
                            if name not in best or float(r["cpu_s"]) < float(best[name]["cpu_s"]):
                                best[name] = r
                    groups = {}
                    for name, _path, group in builds:
                        groups.setdefault(group, set()).add(best[name]["hash"])
                    mismatch = any(len(h) > 1 for h in groups.values())
                    bad += mismatch
                    for name, _path, _group in builds:
                        r = best[name]
                        rows.append({"build": name, "cores": cores, "preset": preset, "quality": q, "drive": d,
                                     "x_real": r["x_real"], "worst_ms": r["worst_ms"], "p99_ms": r["p99_ms"],
                                     "deadline_ms": r["deadline_ms"], "hash": r["hash"],
                                     "mismatch": int(mismatch)})
                    print(f"{cores:4} {preset:9} {q:6} {d:5} " + "  ".join(
                        f"{n}: {float(best[n]['x_real']):.2f}x" for n, _p, _g in builds)
                          + ("   HASHES DIFFER" if mismatch else ""), flush=True)
    if a.csv:
        with open(a.csv, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)
    md = markdown(rows, [b[0] for b in builds], a.cores.split(","), presets, a.quality.split(","), a.drive.split(","))
    print(md)
    if a.md:
        Path(a.md).write_text(md, encoding="utf-8")
    if bad:
        print(f"{bad} cell(s) rendered different sound between builds of the same compiler")
    return 1 if bad else 0


def markdown(rows, names, cores_list, presets, qualities, drives):
    """One table per core class: a row per preset, quality and drive; per
    build the speed (x real time) and the slowest block (ms), with the
    deadline (128 frames) in the header."""
    out = []
    for cores in cores_list:
        out.append(f"\n**Cores: {cores}**\n")
        head = "| Preset | Quality | Drive | " + " | ".join(f"{n} x real | {n} worst ms" for n in names) + " |"
        out += [head, "| --- | --- | --- | " + " | ".join("---: | ---:" for _ in names) + " |"]
        for preset in presets:
            for q in qualities:
                for d in drives:
                    cells = []
                    for n in names:
                        r = next((r for r in rows if (r["build"], r["cores"], r["preset"], r["quality"], r["drive"])
                                  == (n, cores, preset, q, d)), None)
                        cells += [f"{float(r['x_real']):.2f}", f"{float(r['worst_ms']):.2f}"] if r else ["", ""]
                    out.append(f"| {preset} | {q} | {d} | " + " | ".join(cells) + " |")
    return "\n".join(out) + "\n"


if __name__ == "__main__":
    sys.exit(main())
