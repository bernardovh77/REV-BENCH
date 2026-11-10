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
  cmake    the CMake build with the system's own compiler (MSVC on Windows,
           through tools/msvc_env.py; gcc or clang elsewhere) and its C tests
  host     the real-time audio host (stage 123), in the cmake suite's build folder:
           its tests on miniaudio's null device, then rb_cli's drives against the
           host's offline renders of their command traces (tools/host_parity.py)
  addon    the Node-API add-on (stage 124, app/native/): cmake-js 8.0.0 and node-api-headers
           1.9.0 are installed with npm ci in a folder outside the repo (<out>/addon-node), the
           add-on is built with MSVC (Windows, through tools/msvc_env.py) or gcc into
           <out>/addon-build, and app/native/test/*.test.mjs (node:test, the null backend
           only) run against it
  app      the Electron app (stage 124): the snapshot decoder is what tools/gen_snapshot_ts.py
           makes, then npm ci, typecheck, build and Vitest, in a copy of app/ outside the repo

Builds go to ~/rb-build/tests (or $RB_TESTS_OUT), never into the repo. Needs a C compiler and
make (w64devkit on Windows), Node 22+ (npm on PATH, or ~/.local/node22) and,
for e2e, a Chromium: $PW_CHROMIUM, an installed Chrome or Edge on Windows, or
Playwright's own (npx playwright install chromium).
"""
import argparse
import os
from pathlib import Path
import shutil
import signal
import stat
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SUITES = ("native", "wasm", "wasm-c", "python", "web", "e2e", "cmake", "host", "addon", "app")
# each suite's limit in seconds, far above its usual time: a hang is killed,
# with every process it started, and the suite fails as "timeout"
TIMEOUTS = {"native": 1800, "wasm": 600, "wasm-c": 900, "python": 900, "web": 1200, "e2e": 1200,
            "cmake": 3600, "host": 1800, "addon": 1800, "app": 1200}
OUT = Path(os.environ.get("RB_TESTS_OUT") or Path.home() / "rb-build" / "tests")
COPY_IGNORE = shutil.ignore_patterns("node_modules", "dist", "test-results", "build", "build-*",
                                     "renders", ".git", "graphify-out", ".venv*", "*.zip", "*.webm", "*.wav",
                                     ".agents", "__pycache__")


def node_env():
    """The environment, with ~/.local/node22 on PATH if it is there (WSL),
    and this Python's scripts (the venv's cmake and ninja)."""
    env = dict(os.environ)
    paths = [str(Path(sys.executable).parent)]
    local = Path.home() / ".local/node22/bin"
    if local.is_dir():
        paths.insert(0, str(local))
    env["PATH"] = os.pathsep.join(paths + [env.get("PATH", "")])
    return env


def tool(name, env):
    found = shutil.which(name, path=env.get("PATH"))
    if not found:
        raise SystemExit(f"run_tests: {name} isn't on PATH")
    return found


# the programs each suite needs on PATH, and the oldest Node it runs on
NEEDS = {"native": ["make"], "wasm": ["make"], "wasm-c": ["make", "node"],
         "web": ["node", "npm"], "e2e": ["node", "npm", "npx"], "cmake": ["cmake", "ninja"],
         "host": ["cmake", "ninja"], "addon": ["node", "npm", "cmake", "ninja"], "app": ["node", "npm"]}
NODE_MAJOR = 22
CMAKE_LEAST = (3, 25)   # CMakeLists.txt's cmake_minimum_required


def preflight(suites, env):
    """What's missing for these suites, one line each (empty when nothing is):
    a needed program not on env's PATH, or a Node older than NODE_MAJOR."""
    needed = sorted({p for s in suites for p in NEEDS.get(s, [])})
    problems = [f"{p} isn't on PATH (needed by {', '.join(s for s in suites if p in NEEDS.get(s, []))})"
                for p in needed if not shutil.which(p, path=env.get("PATH"))]
    node = shutil.which("node", path=env.get("PATH")) if "node" in needed else None
    if node:
        version = subprocess.run([node, "--version"], capture_output=True, text=True, env=env).stdout.strip()
        try:
            major = int(version.lstrip("v").split(".")[0])
        except ValueError:
            major = 0
        if major < NODE_MAJOR:
            problems.append(f"node is {version or 'of no known version'} ({node}); the web suites need {NODE_MAJOR} or newer")
    cmake = shutil.which("cmake", path=env.get("PATH")) if "cmake" in needed else None
    if cmake:
        first = subprocess.run([cmake, "--version"], capture_output=True, text=True, env=env).stdout.split("\n")[0]
        try:
            have = tuple(int(x) for x in first.split()[-1].split(".")[:2])
        except ValueError:
            have = (0, 0)
        if have < CMAKE_LEAST:
            problems.append(f"{first or 'cmake'} ({cmake}); the cmake suite needs "
                            f"{CMAKE_LEAST[0]}.{CMAKE_LEAST[1]} or newer (tools/requirements-dev.txt)")
    if ("cmake" in suites or "host" in suites or "addon" in suites) and os.name == "nt":
        import msvc_env
        if not msvc_env.install_path():
            problems.append("no Visual Studio with the C/C++ x64 tools for the cmake suite (docs/workspace.md)")
    return problems


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
    zig = f'"{Path(sys.executable).as_posix()}" -m ziglang'   # quoted: a path may have spaces
    jobs = f"-j{os.cpu_count() or 2}"
    return ["make", jobs, f"BUILD={OUT.as_posix()}", f"ZIG={zig}", *targets]


def remove_tree(path, tries=5):
    """Deletes a folder. On Windows a file can be read-only (npm's) or still
    held for a moment by a process that just exited: clear the flag and try
    again for a few seconds before giving up."""
    def writable(function, name, _):
        os.chmod(name, stat.S_IWRITE)
        function(name)
    for attempt in range(tries):
        try:
            shutil.rmtree(path, onerror=writable)
            return
        except OSError:
            if attempt == tries - 1 or not path.exists():
                raise
            time.sleep(1 + attempt)


def web_copy():
    copy = OUT / "web-copy"
    if copy.exists():
        remove_tree(copy)
    shutil.copytree(ROOT, copy, ignore=COPY_IGNORE, symlinks=True)
    return copy / "web"


def app_copy():
    """A copy of app/ (and the contracts' examples its decoder test reads, at the same relative
    path) in OUT/app-copy, without node_modules, builds or the add-on's folder app/native (the
    addon suite's); returns the copy's app folder."""
    copy = OUT / "app-copy"
    if copy.exists():
        remove_tree(copy)

    def ignore(directory, names):
        skipped = set(COPY_IGNORE(directory, names))
        if Path(directory) == ROOT / "app":
            skipped |= {"native"} & set(names)
        return skipped

    shutil.copytree(ROOT / "app", copy / "app", ignore=ignore, symlinks=True)
    shutil.copytree(ROOT / "contracts" / "examples", copy / "contracts" / "examples")
    return copy / "app"


def kill_tree(process):
    """Kills a process and every process it started."""
    if os.name == "nt":
        subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    process.wait()


def run_command(command, cwd, env, timeout):
    """Runs one command: "pass", "fail", or "timeout" when it ran longer than
    timeout seconds, after which it and its children are killed."""
    group = ({"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP} if os.name == "nt"
             else {"start_new_session": True})
    process = subprocess.Popen(command, cwd=cwd, env=env, **group)
    try:
        code = process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        kill_tree(process)
        return "timeout"
    except KeyboardInterrupt:
        kill_tree(process)
        raise
    return "pass" if code == 0 else "fail"


def run_suite(name, state):
    """The suite's commands, folder, environment and time limit."""
    commands, cwd, env = suite_commands(name, state)
    return commands, cwd, env, TIMEOUTS[name]


def suite_commands(name, state):
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
    if name == "app":
        app = app_copy()
        npm = tool("npm", env)
        return [[sys.executable, str(ROOT / "tools/gen_snapshot_ts.py"), "--check"],
                [npm, "ci", "--no-audit", "--no-fund"], [npm, "run", "typecheck"],
                [npm, "run", "build"], [npm, "test"]], app, env
    if name == "cmake":
        build = (OUT / "cmake").as_posix()
        msvc = [sys.executable, str(ROOT / "tools/msvc_env.py")] if os.name == "nt" else []
        return [msvc + ["cmake", "-S", str(ROOT), "-B", build, "-G", "Ninja"],
                msvc + ["cmake", "--build", build],
                msvc + ["ctest", "--test-dir", build, "-LE", "host", "--output-on-failure", "-V"]], ROOT, env
    if name == "host":
        # the cmake suite's folder: the core is built once for both
        build = (OUT / "cmake").as_posix()
        msvc = [sys.executable, str(ROOT / "tools/msvc_env.py")] if os.name == "nt" else []
        exe = ".exe" if os.name == "nt" else ""
        return [msvc + ["cmake", "-S", str(ROOT), "-B", build, "-G", "Ninja"],
                msvc + ["cmake", "--build", build, "--target", "rb_host_tests", "rb_host_render", "rb_host_stress",
                        "rb_host_devices", "rb_cli"],
                msvc + ["ctest", "--test-dir", build, "-L", "host", "--output-on-failure", "-V"],
                [sys.executable, str(ROOT / "tools/host_parity.py"), f"{build}/rb_cli{exe}",
                 f"{build}/rb_host_render{exe}", (OUT / "host-parity").as_posix()]], ROOT, env
    if name == "addon":
        return addon_commands(env), ROOT, addon_env(env)
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


def addon_commands(env):
    """The add-on suite's commands (stage 124): install cmake-js and node-api-headers outside
    the repo from app/native/build-tools (package.json and its lock, copied: npm ci never runs
    inside the repo), build app/native with cmake-js into <out>/addon-build (Ninja; MSVC on
    Windows through tools/msvc_env.py, gcc elsewhere), then run its node:test files one at a
    time (each makes real-time threads, so they must not compete)."""
    tools = OUT / "addon-node"
    tools.mkdir(parents=True, exist_ok=True)
    for name in ("package.json", "package-lock.json"):
        shutil.copy2(ROOT / "app/native/build-tools" / name, tools / name)
    npm, node = tool("npm", env), tool("node", env)
    msvc = [sys.executable, str(ROOT / "tools/msvc_env.py")] if os.name == "nt" else []
    cmake_js = str(tools / "node_modules/cmake-js/bin/cmake-js")
    tests = sorted(str(p) for p in (ROOT / "app/native/test").glob("*.test.mjs"))
    return [[npm, "ci", "--no-audit", "--no-fund", "--prefix", str(tools)],
            msvc + [node, cmake_js, "build", "-d", str(ROOT / "app/native"), "-O", str(OUT / "addon-build"),
                    "-G", "Ninja"],
            [node, "--test", "--test-concurrency=1", *tests]]


def addon_env(env):
    """The environment of the add-on's tests: where the built add-on is."""
    return dict(env, RB_ADDON=str(OUT / "addon-build" / "rb_native.node"))


def run_suites(names, prepare, keep_going):
    """Runs the named suites in order, each from prepare(name) = (commands,
    folder, environment, time limit), and gives (name, status, seconds) for
    each: pass, fail, timeout, or skipped after an earlier failure unless
    keep_going."""
    results, failed = [], False
    for name in names:
        if failed and not keep_going:
            results.append((name, "skipped", 0.0))
            continue
        print(f"\n=== {name} ===", flush=True)
        start = time.monotonic()
        status = "pass"
        commands, cwd, env, timeout = prepare(name)
        for command in commands:
            command, command_env = command if isinstance(command, tuple) else (command, env)
            left = timeout - (time.monotonic() - start)
            status = run_command(command, cwd, command_env, max(left, 1))
            if status != "pass":
                break
        if status == "timeout":
            print(f"run_tests: {name} took over {timeout} s and was killed", flush=True)
        failed = failed or status != "pass"
        results.append((name, status, time.monotonic() - start))
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("suites", nargs="*", metavar="SUITE", help=" ".join(SUITES))
    parser.add_argument("--skip", nargs="*", default=[], metavar="SUITE")
    parser.add_argument("--keep-going", action="store_true",
                        help="run the other suites after one fails (by default the run stops)")
    args = parser.parse_args()
    unknown = sorted(set(args.suites + args.skip) - set(SUITES))
    if unknown:
        parser.error(f"unknown suites {unknown}; they are {' '.join(SUITES)}")
    chosen = [s for s in (args.suites or SUITES) if s not in args.skip]
    problems = preflight(chosen, node_env())
    if problems:
        for problem in problems:
            print(f"run_tests: {problem}")
        sys.exit(2)
    OUT.mkdir(parents=True, exist_ok=True)
    state = {}
    results = run_suites(chosen, lambda name: run_suite(name, state), args.keep_going)
    print(f"\n{'suite':8s} result   time   ({sys.platform})")
    for name, status, seconds in results:
        print(f"{name:8s} {status:8s} {seconds:5.0f} s")
    sys.exit(0 if all(status == "pass" for _, status, _ in results) else 1)


if __name__ == "__main__":
    main()
