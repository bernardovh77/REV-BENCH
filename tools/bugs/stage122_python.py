"""Stage 122's planted bugs for the new tools' tests: the test runner's
pre-flight and the cross-compiler parity check.

python tools/bugs/stage122_python.py [NAME...]
Each bug is applied to a fresh copy of tools/ (with the repo's contracts/),
and the named test runs there. A bug is caught when that test fails.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
VERDICT = "test_compiler_parity.CompilerParityTests.test_the_tolerances_decide"

BUGS = {
    'old-cmake-accepted': ("tools/run_tests.py",
        "        if have < CMAKE_LEAST:",
        "        if False:",
        "test_run_tests.PreflightTests.test_an_old_cmake_is_reported"),
    'parity-ignores-the-level': ("tools/compiler_parity.py",
        'limits = {"sound": SOUND_RMS, "rpm": RPM, "spl": SPL_DB}',
        'limits = {"sound": SOUND_RMS, "rpm": RPM}',
        VERDICT),
    'parity-ignores-misaligned-renders': ("tools/compiler_parity.py",
        '    if "length" in d or d["blocks"][0] != d["blocks"][1]:',
        '    if "length" in d:',
        VERDICT),
    'parity-takes-the-mean-quarter': ("tools/compiler_parity.py",
        '        "sound": max(quarters),',
        '        "sound": sum(quarters) / 4,',
        "test_compiler_parity.CompilerParityTests.test_differences_are_measured_per_quarter"),
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
