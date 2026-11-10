"""Did this change cost the engine real time? (the per-stage speed check)

    python tools/speed_check.py [BASE] [--quality low,balanced,high] [--drive hold]
                                [--rounds 3] [--tolerance 0.03] [--keep DIR]

Every stage that touches core/ runs this before it is done (PLAN.md, "How a
stage is done"). It builds rb_bench_render twice, at BASE (a git commit, by
default HEAD: the last commit, so the working tree's change is what is
measured) and from the working tree, with the target compiler (MSVC through
CMake on Windows; gcc through make elsewhere). Then tools/bench_render.py
times both, interleaved, best of --rounds, on the performance cores where the
CPU has two kinds, for every preset at each --quality.

The verdict: the median over the cells of after/before decides. More than
--tolerance slower (3 % by default: the laptop's interleaved noise is about
that) fails, exit 1. The slowest cells are listed either way, and cells whose
sound changed are reported (expected when a stage changes the physics: then
the slowdown is the price of that sound, to be stated in the stage's doc, not
hidden). BASE must be stage 118's tier commit or later (the `balanced` quality).
"""
import argparse
import csv
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
PRESETS = "i4,i6,v8-cross,v8-flat,boxer4"


def verdict(rows, tolerance):
    """Compares the "before" and "after" builds' rows (bench_render's CSV),
    cell by cell. Returns {median, worst: [(cell, ratio)...] slowest first,
    changed_sound: [cell...], failed}. A cell only one build has is an error."""
    cells = {}
    for r in rows:
        key = f"{r['cores']} {r['preset']} {r['quality']} {r['drive']}"
        cells.setdefault(key, {})[r["build"]] = r
    ratios, changed = [], []
    for key, builds in cells.items():
        if set(builds) != {"before", "after"}:
            raise ValueError(f"cell {key} has builds {sorted(builds)}, not before and after")
        b, a = builds["before"], builds["after"]
        ratios.append((key, float(a["x_real"]) / float(b["x_real"])))
        if a["hash"] != b["hash"]:
            changed.append(key)
    median = statistics.median(r for _k, r in ratios)
    return {"median": median, "worst": sorted(ratios, key=lambda kr: kr[1]),
            "changed_sound": sorted(changed), "failed": median < 1.0 - tolerance}


def sh(cmd, cwd=None):
    r = subprocess.run([str(c) for c in cmd], cwd=cwd, capture_output=True, text=True, encoding="utf-8",
                       errors="replace")
    if r.returncode:
        sys.exit(f"speed_check: {' '.join(map(str, cmd))} failed ({r.returncode})\n{r.stdout[-3000:]}{r.stderr[-3000:]}")
    return r.stdout


def build(src, out):
    """rb_bench_render (and rb_cli beside it) from the sources in `src`, into
    `out`; returns the benchmark's path."""
    if os.name == "nt":
        msvc = [sys.executable, ROOT / "tools/msvc_env.py"]
        sh(msvc + ["cmake", "-S", src, "-B", out, "-G", "Ninja"])
        sh(msvc + ["cmake", "--build", out, "--target", "rb_bench_render", "rb_cli"])
        return Path(out, "rb_bench_render.exe")
    sh(["make", "-C", src, f"BUILD={Path(out).as_posix()}", "-j4", "bench-render"])
    return Path(out, "rb_bench_render")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("base", nargs="?", default="HEAD")
    ap.add_argument("--quality", default="low,balanced,high")
    ap.add_argument("--drive", default="hold")
    ap.add_argument("--presets", default=PRESETS)
    ap.add_argument("--rounds", type=int, default=3)
    ap.add_argument("--tolerance", type=float, default=0.03)
    ap.add_argument("--keep", help="build and keep everything in this folder (default: a temporary one)")
    a = ap.parse_args(argv)

    work = Path(a.keep) if a.keep else Path(tempfile.mkdtemp(prefix="rb-speed-"))
    work.mkdir(parents=True, exist_ok=True)
    base_src = work / "base-src"
    if base_src.exists():
        sh(["git", "worktree", "remove", "--force", base_src], cwd=ROOT)
    sh(["git", "worktree", "add", "--detach", base_src, a.base], cwd=ROOT)
    try:
        print(f"speed_check: building {a.base} and the working tree ...", flush=True)
        before = build(base_src, work / "base-build")
        after = build(ROOT, work / "tree-build")
    finally:
        sh(["git", "worktree", "remove", "--force", base_src], cwd=ROOT)

    sys.path.insert(0, str(ROOT / "tools"))
    import bench_render
    cores = "p" if bench_render.core_masks() else "any"
    table = work / "speed.csv"
    bench = subprocess.run([sys.executable, ROOT / "tools/bench_render.py", "--exe", f"before={before}",
                            "--exe", f"after={after}", "--presets", a.presets, "--quality", a.quality,
                            "--drive", a.drive, "--rounds", str(a.rounds), "--cores", cores, "--csv", table])
    if bench.returncode or not table.exists():
        sys.exit("speed_check: the benchmark failed")
    with open(table, encoding="utf-8") as f:
        v = verdict(list(csv.DictReader(f)), a.tolerance)

    print(f"\nspeed_check: {a.base} -> working tree, {len(v['worst'])} cells on the '{cores}' cores")
    print(f"  median speed after/before: {v['median']:.3f} ({100 * (v['median'] - 1):+.1f} %), "
          f"tolerance -{100 * a.tolerance:.0f} %")
    for key, ratio in v["worst"][:5]:
        print(f"  {key:28} {100 * (ratio - 1):+.1f} %")
    if v["changed_sound"]:
        print(f"  sound changed in {len(v['changed_sound'])} cells (state the cost in the stage's doc)")
    print("  FAILED: slower beyond the tolerance" if v["failed"] else "  ok")
    if not a.keep:
        shutil.rmtree(work, ignore_errors=True)
    return 1 if v["failed"] else 0


if __name__ == "__main__":
    sys.exit(main())
