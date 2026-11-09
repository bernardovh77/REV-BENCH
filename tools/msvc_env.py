#!/usr/bin/env python3
"""Runs a command with Visual Studio's C compiler environment (stage 122).

    python tools/msvc_env.py cmake -S . -B BUILD -G Ninja
    python tools/msvc_env.py --print        what it found

MSVC's cl.exe needs the environment its vcvars64.bat sets (PATH, INCLUDE,
LIB...). Rather than asking for a "Developer Command Prompt", this finds the
newest Visual Studio (or Build Tools) with the C/C++ tools through vswhere,
runs its vcvars64.bat once, and starts the command with the environment that
leaves. Windows only.
"""
import os
from pathlib import Path
import subprocess
import sys

VSWHERE = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
               "Microsoft Visual Studio", "Installer", "vswhere.exe")
TOOLS = "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"


def install_path():
    """The newest Visual Studio with the C/C++ x64 tools, or None."""
    if not VSWHERE.is_file():
        return None
    out = subprocess.run([str(VSWHERE), "-latest", "-products", "*", "-requires", TOOLS,
                          "-property", "installationPath"], capture_output=True, text=True)
    path = out.stdout.strip()
    return Path(path) if path else None


def environment():
    """os.environ as vcvars64.bat leaves it; SystemExit if there's no MSVC."""
    if os.name != "nt":
        raise SystemExit("msvc_env: MSVC is Windows-only")
    vs = install_path()
    if not vs:
        raise SystemExit("msvc_env: no Visual Studio with the C/C++ x64 tools "
                         "(Build Tools with 'Desktop development with C++'; docs/workspace.md)")
    vcvars = vs / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    out = subprocess.run(f'cmd /s /c ""{vcvars}" >nul && set"', capture_output=True,
                         text=True, shell=True)
    if out.returncode:
        raise SystemExit(f"msvc_env: {vcvars} failed")
    env = {}
    for line in out.stdout.splitlines():
        name, sep, value = line.partition("=")
        if sep and name:
            env[name] = value
    return env


def main():
    if sys.argv[1:] == ["--print"]:
        env = environment()
        print(install_path())
        print("cl:", subprocess.run(["where", "cl"], capture_output=True, text=True, env=env).stdout.strip())
        return 0
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    return subprocess.run(sys.argv[1:], env=environment()).returncode


if __name__ == "__main__":
    sys.exit(main())
