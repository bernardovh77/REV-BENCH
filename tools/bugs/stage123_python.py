"""Stage 123's planted bugs for the host parity checker's tests.

python tools/bugs/stage123_python.py [NAME...]
Each bug is applied to a fresh copy of tools/ (with the repo's contracts/),
and the named test runs there. A bug is caught when that test fails.
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]

BUGS = {
    'truncated-render-passes': ("tools/host_parity.py",
        "    return None if len(a) == len(b) else min(len(a), len(b))",
        "    return None",
        "test_host_parity.HostParityTests.test_a_truncated_render_differs"),
    'failed-host-passes': ("tools/host_parity.py",
        '            print(f"{name}: rb_host_render failed ({code})\\n{text}")\n            differ += 1',
        '            print(f"{name}: rb_host_render failed ({code})\\n{text}")',
        "test_host_parity.HostParityTests.test_a_failed_program_fails_the_case"),
    'host_plays_its_own_trace': ("tools/host_parity.py",
        '        code, text = run([render, trace, "-o", host, "--float"])',
        '        code, text = run([render, out / "other.rbtrace", "-o", host, "--float"])',
        "test_host_parity.HostParityTests.test_the_host_plays_the_trace_the_drive_wrote"),
    'dithered-comparison': ("tools/host_parity.py",
        '        code, text = run([cli, "sound", *args, "--float", "-o", ref, "--trace", trace])',
        '        code, text = run([cli, "sound", *args, "-o", ref, "--trace", trace])',
        "test_host_parity.HostParityTests.test_the_host_plays_the_trace_the_drive_wrote"),
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
