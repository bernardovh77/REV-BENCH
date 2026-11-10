#!/usr/bin/env python3
"""Builds the Windows packages (stage 124) and checks them: an unsigned NSIS installer and a
portable zip, x64, from app/electron-builder.config.cjs.

    python tools/package_app.py [--out FOLDER] [--install-check]

Everything is made outside the repo, in FOLDER (default <RB_TESTS_OUT or ~/rb-build/tests>/package):
  addon-build/rb_native.node   the add-on, built as tools/run_tests.py addon does
  app-copy/app                 a copy of app/ with node_modules, built (npm ci, npm run build)
  release/                     the installer, the zip and win-unpacked/
Then it checks the unpacked app: the add-on is beside the asar (resources/rb_native.node) and
not inside it, the fake add-on is nowhere, the notices are there, Electron's fuses are the ones the
config sets; and it starts the unpacked app and the app from the extracted zip, on the null audio
backend (no sound device), through app/e2e/packaged-smoke.mjs. The installer is built but not
run here (it would install on this machine); CI passes --install-check, which installs it silently
into a temporary folder of the runner and starts that.
"""
import argparse
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

import run_tests

# what electron-builder.config.cjs sets (checked in the package, not only in the config)
EXPECTED_FUSES = {
    "RunAsNode": "Disabled", "EnableCookieEncryption": "Enabled", "EnableNodeOptionsEnvironmentVariable": "Disabled",
    "EnableNodeCliInspectArguments": "Disabled", "EnableEmbeddedAsarIntegrityValidation": "Enabled",
    "OnlyLoadAppFromAsar": "Enabled", "LoadBrowserProcessSpecificV8Snapshot": "Disabled",
    "GrantFileProtocolExtraPrivileges": "Enabled",
}


def run(command, cwd, env, what):
    print(f"\n--- {what}: {' '.join(map(str, command))}", flush=True)
    if subprocess.run(command, cwd=cwd, env=env).returncode:
        sys.exit(f"package_app: {what} failed")


def capture(command, cwd, env):
    done = subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return done.returncode, done.stdout + done.stderr


def check_unpacked(unpacked, app, env):
    """The unpacked app has what a package must, and nothing it must not."""
    resources = unpacked / "resources"
    problems = []
    for name in ("rb_native.node", "THIRD_PARTY_NOTICES.md", "licenses/Inter-OFL.txt", "licenses/miniaudio-LICENSE.txt", "app.asar"):
        if not (resources / name).is_file():
            problems.append(f"resources/{name} is missing")
    npx = run_tests.tool("npx", env)
    code, listing = capture([npx, "asar", "list", str(resources / "app.asar")], app, env)
    inside = listing.replace("\\", "/")
    if code:
        problems.append("could not list app.asar: " + listing[-300:])
    for must in ("/dist/engine/engine.cjs", "/dist/main/main.cjs", "/dist/preload/preload.cjs", "/dist/ui/index.html"):
        if must not in inside:
            problems.append(f"app.asar lacks {must}")
    for never in ("rb_native.node", "fake-addon", ".map", "node_modules"):
        if never in inside:
            problems.append(f"app.asar holds {never}")
    code, out = capture([npx, "electron-fuses", "read", "--app", str(unpacked / "Rev-Bench.exe")], app, env)
    got = {m.group(1): m.group(2) for m in (re.match(r"\s*(\w+) is (Enabled|Disabled)\s*$", l) for l in out.splitlines()) if m}
    for fuse, want in EXPECTED_FUSES.items():
        if got.get(fuse) != want:
            problems.append(f"fuse {fuse} should be {want} (it is {got.get(fuse)})")
    if problems:
        sys.exit("package_app: the unpacked app is wrong:\n  " + "\n  ".join(problems))
    print("the unpacked app has the add-on beside the asar, the notices, no fake add-on, and the fuses the config sets")


def smoke(exe, app, env):
    run([run_tests.tool("node", env), "e2e/packaged-smoke.mjs", str(exe)], app, env, f"start {exe}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", default=str(run_tests.OUT / "package"))
    parser.add_argument("--install-check", action="store_true", help="also install the installer silently into a temporary folder and start that (for CI)")
    args = parser.parse_args()
    if os.name != "nt":
        sys.exit("package_app: Windows only until stage 126")
    run_tests.OUT = Path(args.out)
    out = run_tests.OUT
    env = run_tests.node_env()
    problems = run_tests.preflight(["app-e2e"], env)
    if problems:
        sys.exit("\n".join(f"package_app: {p}" for p in problems))
    out.mkdir(parents=True, exist_ok=True)
    for command in run_tests.addon_build_commands(env):
        run(command, run_tests.ROOT, env, "build the add-on")
    app = run_tests.app_copy()
    npm, npx = run_tests.tool("npm", env), run_tests.tool("npx", env)
    release = out / "release"
    shutil.rmtree(release, ignore_errors=True)
    env = dict(env, RB_NATIVE_PATH=str(out / "addon-build" / "rb_native.node"), CSC_IDENTITY_AUTO_DISCOVERY="false")
    run([npm, "ci", "--no-audit", "--no-fund"], app, env, "install the app's packages")
    run([npm, "run", "build"], app, env, "build the app")
    run([npx, "install-electron"], app, env, "Electron's binary")
    run([npx, "electron-builder", "--config", "electron-builder.config.cjs", "--win", "--x64", f"-c.directories.output={release.as_posix()}"],
        app, env, "electron-builder")

    unpacked = release / "win-unpacked"
    installers = sorted(release.glob("Rev-Bench-Setup-*.exe"))
    zips = sorted(release.glob("Rev-Bench-*-win-x64.zip"))
    if len(installers) != 1 or len(zips) != 1:
        sys.exit(f"package_app: expected one installer and one zip in {release}: {[p.name for p in release.iterdir()]}")
    print(f"\ninstaller {installers[0].name} ({installers[0].stat().st_size / 1e6:.1f} MB), zip {zips[0].name} ({zips[0].stat().st_size / 1e6:.1f} MB)")

    check_unpacked(unpacked, app, env)
    smoke(unpacked / "Rev-Bench.exe", app, env)
    extracted = out / "zip-check"
    shutil.rmtree(extracted, ignore_errors=True)
    with zipfile.ZipFile(zips[0]) as z:
        z.extractall(extracted)
    smoke(extracted / "Rev-Bench.exe", app, env)
    if args.install_check:
        target = Path(tempfile.mkdtemp(prefix="rb-installed-"))
        run([str(installers[0]), "/S", f"/D={target}"], app, env, "silent install into a temporary folder")
        smoke(target / "Rev-Bench.exe", app, env)
        uninstall = target / "Uninstall Rev-Bench.exe"
        if uninstall.is_file():
            subprocess.run([str(uninstall), "/S"], env=env)
    summary = {"installer": installers[0].name, "zip": zips[0].name, "folder": str(release)}
    (out / "package.json").write_text(json.dumps(summary, indent=2))
    print("\npackage_app: ok", json.dumps(summary))


if __name__ == "__main__":
    main()
