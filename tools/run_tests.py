#!/usr/bin/env python3
"""Runs every test suite on this system, Windows or Linux (WSL) alike.

    python tools/run_tests.py                 everything, the e2e test included
    python tools/run_tests.py native python   only those suites
    python tools/run_tests.py --skip e2e      all but those

Run it with the Python that has tools/requirements-dev.txt installed (its
zig builds the WebAssembly). The suites:
  native   make test: the C tests (5-8 min)
  wasm     make wasm into the build folder: the committed web/src/wasm core
           must be what today's sources build, byte for byte
  wasm-c   make test-wasm: a few C tests as WebAssembly, under Node
  python   the tools' unit tests
  web      npm ci and npm test, in a copy outside the repo (CLAUDE.md)
  e2e      the Playwright test, dev server and production build, in that copy

Builds go to ~/rb-build/tests, never into the repo. Needs a C compiler and
make (w64devkit on Windows), Node 22+ (npm on PATH, or ~/.local/node22) and,
for e2e, a Chromium: $PW_CHROMIUM, an installed Chrome or Edge on Windows, or
Playwright's own (npx playwright install chromium).
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SUITES = ("native", "wasm", "wasm-c", "python", "web", "e2e")
OUT = Path.home() / "rb-build" / "tests"
COPY_IGNORE = shutil.ignore_patterns("node_modules", "dist", "test-results", "build", "build-*",
                                     "renders", ".git", "graphify-out", ".venv*", "*.zip",
                                     "stage-115-renders", ".agents", "__pycache__")


def node_env():
    """The environment, with ~/.local/node22 on PATH if it is there (WSL)."""
    env = dict(os.environ)
    local = Path.home() / ".local/node22/bin"
    if local.is_dir():
        env["PATH"] = str(local) + os.pathsep + env.get("PATH", "")
    return env


def tool(name, env):
    found = shutil.which(name, path=env.get("PATH"))
    if not found:
        raise SystemExit(f"run_tests: {name} isn't on PATH")
    return found


def chromium():
    if os.environ.get("PW_CHROMIUM"):
        return os.environ["PW_CHROMIUM"]
    if os.name == "nt":
        for base in (os.environ.get("ProgramFiles", r"C:\Program Files"),
                     os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")):
            for browser in (r"Google\Chrome\Application\chrome.exe",
                            r"Microsoft\Edge\Application\msedge.exe"):
                if Path(base, browser).is_file():
                    return str(Path(base, browser))
    return None   # Playwright's own, if installed


def make(*targets):
    zig = f'{Path(sys.executable).as_posix()} -m ziglang'
    jobs = f"-j{os.cpu_count() or 2}"
    return ["make", jobs, f"BUILD={OUT.as_posix()}", f"ZIG={zig}", *targets]


def web_copy():
    copy = OUT / "web-copy"
    if copy.exists():
        shutil.rmtree(copy)
    shutil.copytree(ROOT, copy, ignore=COPY_IGNORE, symlinks=True)
    return copy / "web"


def run_suite(name, state):
    env = node_env()
    if name == "native":
        return [make("test")], ROOT, env
    if name == "wasm":
        out = OUT / "wasm-check" / "rb_core.wasm"
        return [make("wasm", f"WASM_OUT={out.as_posix()}"),
                [sys.executable, "-c",
                 "import sys,pathlib;a,b=pathlib.Path(sys.argv[1]),pathlib.Path(sys.argv[2]);"
                 "same=a.read_bytes()==b.read_bytes() and a.with_suffix('.sources.sha256').read_text().split()"
                 "==b.with_suffix('.sources.sha256').read_text().split();"
                 "print('the committed core is current' if same else 'the committed core is stale: run make wasm');"
                 "sys.exit(0 if same else 1)",
                 str(out), str(ROOT / "web/src/wasm/rb_core.wasm")]], ROOT, env
    if name == "wasm-c":
        return [make("test-wasm")], ROOT, env
    if name == "python":
        env["PYTHONPATH"] = str(ROOT / "tools")
        return [[sys.executable, "-m", "unittest", "discover", "-s", "tools", "-p", "test_*.py"]], ROOT, env
    if name == "web":
        web = web_copy()
        state["web"] = web
        npm = tool("npm", env)
        return [[npm, "ci", "--no-audit", "--no-fund"], [npm, "test"]], web, env
    if name == "e2e":
        web = state.get("web") or web_copy()
        npm, npx = tool("npm", env), tool("npx", env)
        commands = [] if (web / "node_modules").is_dir() else [[npm, "ci", "--no-audit", "--no-fund"]]
        env["CI"] = "1"   # start its own servers, never reuse one that is running
        browser = chromium()
        if browser:
            env["PW_CHROMIUM"] = browser
        preview = dict(env, E2E_PREVIEW="1")
        return commands + [[npx, "playwright", "test"], ([npx, "playwright", "test"], preview)], web, env
    raise ValueError(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("suites", nargs="*", metavar="SUITE", help=" ".join(SUITES))
    parser.add_argument("--skip", nargs="*", default=[], metavar="SUITE")
    args = parser.parse_args()
    unknown = sorted(set(args.suites + args.skip) - set(SUITES))
    if unknown:
        parser.error(f"unknown suites {unknown}; they are {' '.join(SUITES)}")
    chosen = [s for s in (args.suites or SUITES) if s not in args.skip]
    OUT.mkdir(parents=True, exist_ok=True)
    state, results = {}, []
    for name in chosen:
        print(f"\n=== {name} ===", flush=True)
        start = time.monotonic()
        ok = True
        commands, cwd, env = run_suite(name, state)
        for command in commands:
            command, command_env = command if isinstance(command, tuple) else (command, env)
            if subprocess.run(command, cwd=cwd, env=command_env).returncode:
                ok = False
                break
        results.append((name, ok, time.monotonic() - start))
    print(f"\n{'suite':8s} result  time   ({sys.platform})")
    for name, ok, seconds in results:
        print(f"{name:8s} {'pass' if ok else 'FAIL':6s} {seconds:5.0f} s")
    sys.exit(0 if all(ok for _, ok, _ in results) else 1)


if __name__ == "__main__":
    main()
