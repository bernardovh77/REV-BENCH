"""A stand-in program written in Python, runnable on Windows and Linux alike.

The pipeline tests replace the expensive rb_cli with a short Python script.
On Linux the script itself is the program (a #! line and the executable bit).
Windows can't run a script directly (WinError 193), so there the script goes
beside it as NAME.py, and a tiny C launcher, NAME.exe, runs it with this
Python; the program is then found as NAME, as Windows finds NAME.exe.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys

LAUNCHER = r'''
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <string.h>

/* Runs PYTHON -S NAME.py ARGS..., NAME.exe being this program. */
int main(int argc, char **argv)
{
    static char script[MAX_PATH + 8], quoted[256][1024];
    const char *args[260];
    DWORD n = GetModuleFileNameA(NULL, script, MAX_PATH);
    if (n < 4 || argc > 250) return 125;
    strcpy(script + n - 4, ".py");              /* NAME.exe -> NAME.py */
    snprintf(quoted[0], sizeof quoted[0], "\"%s\"", PYTHON);
    snprintf(quoted[1], sizeof quoted[1], "-S");
    snprintf(quoted[2], sizeof quoted[2], "\"%s\"", script);
    for (int i = 1; i < argc; i++)               /* the spawn joins its arguments */
        snprintf(quoted[i + 2], sizeof quoted[0], "\"%s\"", argv[i]);
    for (int i = 0; i < argc + 2; i++) args[i] = quoted[i];
    args[argc + 2] = NULL;
    fflush(stdout);
    return (int)_spawnv(_P_WAIT, PYTHON, args);
}
'''


def compiler():
    return os.environ.get("CC") or shutil.which("cc") or shutil.which("gcc")


def available():
    """Whether fake programs can be made here (on Windows they need a C compiler)."""
    return os.name != "nt" or compiler() is not None


def make(path, source):
    """Makes the program `path` run the Python `source` (with python -S)."""
    path = Path(path)
    if os.name != "nt":
        path.write_text(f"#!{sys.executable} -S\n" + source, encoding="utf-8")
        path.chmod(0o755)
        return path
    path.with_suffix(".py").write_text(source, encoding="utf-8")
    c = path.with_suffix(".c")
    c.write_text(LAUNCHER, encoding="utf-8")
    python = sys.executable.replace("\\", "\\\\")
    subprocess.run([compiler(), "-O1", f'-DPYTHON="{python}"', str(c), "-o", str(path.with_suffix(".exe"))],
                   check=True, capture_output=True)
    c.unlink()
    return path
