"""Stage 124's planted bugs for the per-stage speed check (tools/speed_check.py),
added before stage 124 began (Bernardo, 2026-11-09).

python tools/bugs/stage124_python.py [NAME...]
Each bug is applied to a fresh copy of tools/ (with the repo's contracts/),
and the named test runs there. A bug is caught when that test fails.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEED = "test_speed_check.VerdictTests"

BUGS = {
    'speed-mean-not-median': ("tools/speed_check.py",
        "    median = statistics.median(r for _k, r in ratios)",
        "    median = statistics.mean(r for _k, r in ratios)",
        SPEED + ".test_one_noisy_cell_does_not_fail_but_is_named"),
    'speed-ratio-upside-down': ("tools/speed_check.py",
        "        ratios.append((key, float(a[\"x_real\"]) / float(b[\"x_real\"])))",
        "        ratios.append((key, float(b[\"x_real\"]) / float(a[\"x_real\"])))",
        SPEED),
    'speed-tolerance-ignored': ("tools/speed_check.py",
        '"failed": median < 1.0 - tolerance}',
        '"failed": median < 0.5}',
        SPEED + ".test_a_median_slowdown_beyond_the_tolerance_fails"),
    'speed-changed-sound-fails': ("tools/speed_check.py",
        '"failed": median < 1.0 - tolerance}',
        '"failed": median < 1.0 - tolerance or bool(changed)}',
        SPEED + ".test_a_changed_sound_is_reported_not_failed"),
    'speed-missing-cell-ignored': ("tools/speed_check.py",
        "        if set(builds) != {\"before\", \"after\"}:",
        "        if not builds:",
        SPEED + ".test_a_cell_one_build_lacks_is_an_error"),
    'speed-worst-not-sorted': ("tools/speed_check.py",
        "\"worst\": sorted(ratios, key=lambda kr: kr[1]),",
        "\"worst\": ratios,",
        SPEED + ".test_one_noisy_cell_does_not_fail_but_is_named"),
}

def run(name):
    path, old, new, test = BUGS[name]
    with tempfile.TemporaryDirectory(prefix="rb-pybug-") as work:
        work = pathlib.Path(work)
        for folder in ("tools", "contracts"):
            shutil.copytree(ROOT / folder, work / folder,
                            ignore=shutil.ignore_patterns("__pycache__"))
        target = work / path
        text = target.read_text(encoding="utf-8")
        if text.count(old) != 1:
            return f"{name}: NOT APPLIED ({text.count(old)} matches in {path})"
        target.write_text(text.replace(old, new), encoding="utf-8", newline="\n")
        r = subprocess.run([sys.executable, "-m", "unittest", test], cwd=work / "tools",
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        if r.returncode:
            return f"{name}: caught by {test.split('.')[-1]}"
        return f"{name}: MISSED ({test} passes)"


if __name__ == "__main__":
    for name in sys.argv[1:] or BUGS:
        print(run(name), flush=True)
