"""Stage 121's planted bugs for the snapshot-v2 checker's tests.

python tools/bugs/stage121_python.py [NAME...]
Each bug is applied to a fresh copy of tools/ (with the repo's contracts/),
and the named test runs there. A bug is caught when that test fails.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
REFUSED = "test_snapshot_v2.SnapshotV2Tests.test_broken_snapshots_are_refused"

BUGS = {
    'magic-not-checked': ("tools/snapshot_v2.py",
        '    if h["magic"] != MAGIC:',
        '    if False:',
        REFUSED),
    'sequence-not-checked': ("tools/snapshot_v2.py",
        '        if b["sequence"] != a["sequence"] + 1:',
        '        if False:',
        REFUSED),
    'phase-not-checked': ("tools/snapshot_v2.py",
        '    if not (0.0 <= h["phase_rad"] < 4 * math.pi):',
        '    if False:',
        REFUSED),
    'burn-not-checked': ("tools/snapshot_v2.py",
        '        if not 0.0 <= row["burn"] <= 1.0:',
        '        if False:',
        REFUSED),
    'layout-drifts': ("tools/snapshot_v2.py",
        '(96, "phase_rad", "f64"),',
        '(96, "phase", "f64"),',
        "test_snapshot_v2.SnapshotV2Tests.test_the_layout_is_the_contracts"),
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
