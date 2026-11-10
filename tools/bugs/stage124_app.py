"""Stage 124's planted bugs for the app (app/): each is one change to a fresh copy of the sources,
and the named Vitest test must fail on it (caught) or the bug is MISSED.

    python tools/bugs/stage124_app.py [--list] [NAME...]        (no names: all of them)

The packages are installed once, with `npm ci`, in <RB_TESTS_OUT or ~/rb-build/tests>/app-bugs/base
(reused while it is there); each bug gets its own copy of app/ and the contracts' examples with
that node_modules linked in, and runs only the one test file and test name the table gives
(`vitest run FILE -t NAME`), so the bug is caught by the test meant to catch it and not by luck.
A bug whose text no longer matches (count not 1) is NOT APPLIED: retarget it in the same stage that
moves the text (tools/plant_bugs.py --check does this for the C bugs).
"""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = pathlib.Path(os.environ.get("RB_TESTS_OUT") or pathlib.Path.home() / "rb-build" / "tests") / "app-bugs"

# name: (file from the repo's root, text, replacement, test file under app/, test name filter)
BUGS = {
    "page-path-compared-as-text": (
        "app/src/main/window.ts",
        "    return fold(canonical(fileURLToPath(given))) === fold(canonical(rendererIndex));",
        "    return fileURLToPath(given) === rendererIndex;",
        "tests/security.test.ts", "knows its page under another spelling of the same path"),
    "decoder-off-by-one-field": (
        "app/src/shared/snapshot-v2.generated.ts",
        'physics_tick: counter(view, 56, "physics_tick"),',
        'physics_tick: counter(view, 64, "physics_tick"),',
        "tests/snapshot.test.ts", "reads every value as snapshot_v2.py does"),
    "decoder-accepts-a-wrong-magic": (
        "app/src/shared/snapshot-v2.generated.ts",
        "  if (h.magic !== SNAPSHOT_MAGIC) fail(",
        "  if (false) fail(",
        "tests/snapshot.test.ts", "refuses a wrong magic"),
    "csp-loosened": (
        "app/src/shared/csp.ts",
        '"script-src": ["\'self\'"],',
        '"script-src": ["\'self\'", "\'unsafe-inline\'"],',
        "tests/security.test.ts", "has no inline script or style, no eval, no wildcard and no remote host"),
    "node-integration-on": (
        "app/src/main/window.ts",
        "    nodeIntegration: false,",
        "    nodeIntegration: true,",
        "tests/security.test.ts", "sandboxes the renderer, isolates its context and gives it no Node, anywhere"),
    "sandbox-off": (
        "app/src/main/window.ts",
        "    sandbox: true,",
        "    sandbox: false,",
        "tests/security.test.ts", "sandboxes the renderer, isolates its context and gives it no Node, anywhere"),
    "navigation-allowed": (
        "app/src/main/window.ts",
        "    if (!isAppPage(target, rendererIndex)) {",
        "    if (false) {",
        "tests/security.test.ts", "blocks navigation and redirects to anywhere but the app's page"),
    "permissions-granted": (
        "app/src/main/window.ts",
        "    callback(false);",
        "    callback(true);",
        "tests/security.test.ts", "refuses every permission request and answers every permission check no"),
    "reply-id-not-echoed": (
        "app/src/engine/control.ts",
        'this.port?.post({ type: "reply", id, ...body });',
        'this.port?.post({ type: "reply", id: 0, ...body });',
        "tests/engine-control.test.ts", "acknowledges set-param with the window's own id"),
    "snapshot-not-transferred": (
        "app/src/engine/control.ts",
        "      [read.bytes],",
        "      [],",
        "tests/engine-control.test.ts", "sends the newest snapshot as a transferred ArrayBuffer"),
    "reply-to-a-gone-window": (
        "app/src/engine/control.ts",
        "      if (pending.epoch === this.epoch) this.reply(pending.id, { ack });",
        "      this.reply(pending.id, { ack });",
        "tests/engine-control.test.ts", "does not send the rebuild's reply to a window that has gone"),
    "rebuild-failure-ignored": (
        "app/src/engine/control.ts",
        '    } else if (state.state === "failed") {',
        '    } else if (state.state === "failed" && false) {',
        "tests/engine-control.test.ts", "leaves the running engine alone when the preparation fails"),
    "native-falls-back-to-the-fake": (
        "app/src/engine/addon-loader.ts",
        "  if (!found) {",
        "  if (!found) {\n    return deps.loadFake();",
        "tests/engine-process.test.ts", "says where it looked when the add-on isn't there"),
    "restart-without-the-autosaved-preset": (
        "app/src/main/supervisor.ts",
        "    return this.deps.autosave.load() ?? DEFAULT_BUILD;",
        "    return DEFAULT_BUILD;",
        "tests/supervisor.test.ts", "is restarted with the build it last made"),
    "restart-never-gives-up": (
        "app/src/main/supervisor.ts",
        "    if (this.deaths.length > this.policy.maxRestarts) {",
        "    if (false) {",
        "tests/supervisor.test.ts", "gives up after too many deaths in a minute"),
    "autosave-ignores-a-build-message": (
        "app/src/main/supervisor.ts",
        "          this.deps.autosave.save(choice);",
        "          void choice;",
        "tests/supervisor.test.ts", "is restarted with the build it last made"),
    "panel-uses-a-raw-colour": (
        "app/src/ui/panels/TopBar.svelte",
        "    background: var(--rb-color-surface);\n    border-bottom",
        "    background: #161920;\n    border-bottom",
        "tests/design-tokens.test.ts", "TopBar.svelte uses only tokens"),
    "panel-uses-a-raw-length": (
        "app/src/ui/panels/CentrePanel.svelte",
        "    gap: var(--rb-space-4);",
        "    gap: 12px;",
        "tests/design-tokens.test.ts", "CentrePanel.svelte uses only tokens"),
    "panel-uses-its-own-font": (
        "app/src/ui/panels/BottomPanel.svelte",
        "    font-size: var(--rb-font-size-sm);",
        "    font-family: Arial, sans-serif;",
        "tests/design-tokens.test.ts", "BottomPanel.svelte uses only tokens"),
    "linter-ignores-hex-colours": (
        "app/src/ui/design/lint.ts",
        "  if (HEX_COLOUR.test(bare) || COLOUR_FUNCTION.test(bare)) {",
        "  if (COLOUR_FUNCTION.test(bare)) {",
        "tests/design-tokens.test.ts", "finds a hex colour, in any of its lengths"),
    "preload-exposes-ipc": (
        "app/src/preload/bridge-impl.ts",
        "    platform: env.platform,",
        "    ipc: env.ipc,\n    platform: env.platform,",
        "tests/preload.test.ts", "exposes only the five names the type promises"),
    "client-ignores-detach": (
        "app/src/ui/lib/engine-client.ts",
        '      waiting.reject(new CommandError("DETACHED", reason));',
        "      void waiting;",
        "tests/engine-client.test.ts", "fails the waiting commands with DETACHED"),
    # phase 2: the add-on's path, the packaging and the driver defaults
    "addon-looked-for-inside-the-asar": (
        "app/src/engine/addon-loader.ts",
        "  if (deps.resourcesPath) out.push(join(deps.resourcesPath, ADDON_FILE));",
        '  if (deps.resourcesPath) out.push(join(deps.resourcesPath, "app.asar", ADDON_FILE));',
        "tests/packaging.test.ts", "puts the add-on in resources, beside the asar"),
    "dev-addon-path-keeps-release-folder": (
        "app/src/engine/addon-loader.ts",
        '"native", "build", ADDON_FILE));',
        '"native", "build", "Release", ADDON_FILE));',
        "tests/packaging.test.ts", "is one: the development build, the tests' and the package's all say rb_native.node"),
    "package-puts-the-addon-in-a-subfolder": (
        "app/electron-builder.config.cjs",
        'to: "rb_native.node" },',
        'to: "native/rb_native.node" },',
        "tests/packaging.test.ts", "puts the add-on in resources, beside the asar"),
    "package-keeps-the-fake-addon": (
        "app/electron-builder.config.cjs",
        ', "!dist/engine/fake-addon.cjs"],',
        '],',
        "tests/packaging.test.ts", "packs the built files into the asar, without source maps or the fake add-on"),
    "package-without-an-asar": (
        "app/electron-builder.config.cjs",
        "  asar: true,",
        "  asar: false,",
        "tests/packaging.test.ts", "packs the built files into the asar, without source maps or the fake add-on"),
    "fuse-run-as-node-on": (
        "app/electron-builder.config.cjs",
        "    runAsNode: false,",
        "    runAsNode: true,",
        "tests/packaging.test.ts", "turns off running as plain Node, NODE_OPTIONS and the inspect flags"),
    "package-without-the-inspect-fuse-off": (
        "app/electron-builder.config.cjs",
        "    enableNodeCliInspectArguments: false,",
        "    enableNodeCliInspectArguments: true,",
        "tests/packaging.test.ts", "turns off running as plain Node, NODE_OPTIONS and the inspect flags"),
    "engine-built-with-the-dyno-held-crank": (
        "app/src/engine/control.ts",
        "export const APP_BUILD_PARAMS: Record<string, number> = { hold: 0 };",
        "export const APP_BUILD_PARAMS: Record<string, number> = {};",
        "tests/engine-control.test.ts", "builds the first engine with a free crank"),
    "restarted-engine-gets-no-port": (
        "app/src/main/supervisor.ts",
        '    process.postMessage({ type: "init", config: this.deps.engineInit(build) });\n    this.connectWindow();',
        '    process.postMessage({ type: "init", config: this.deps.engineInit(build) });',
        "tests/supervisor.test.ts", "is told the new port after the restart, if the window had asked"),
    "window-keeps-a-dead-engines-port": (
        "app/src/ui/lib/engine-store.svelte.ts",
        "      if (this.client.connected) this.client.detach(",
        "      if (false) this.client.detach(",
        "tests/engine-store.test.ts", "drops the connection when the engine process restarts"),
    "fake-add-on-throws-for-an-unknown-parameter": (
        "app/src/engine/fake-addon.ts",
        '    if (!info) return this.ack("set-param", "refused", false, -2);',
        '    if (!info) throw addonError("RB_ERR_ID", `no parameter ${id}`);',
        "tests/fake-addon.test.ts", "refuses an id that is not a whole number by throwing"),
}


def link(target, where):
    """A folder link: a junction on Windows (no privileges needed), a symlink elsewhere."""
    if os.name == "nt":
        subprocess.run(["cmd", "/c", "mklink", "/J", str(where), str(target)], check=True, capture_output=True)
    else:
        os.symlink(target, where, target_is_directory=True)


def copy_sources(dest):
    """app/ without node_modules, builds and the add-on's folder, and the contracts' examples."""
    ignore = shutil.ignore_patterns("node_modules", "dist", "test-results", "native", "__pycache__")
    shutil.copytree(ROOT / "app", dest / "app", ignore=ignore)
    shutil.copytree(ROOT / "contracts" / "examples", dest / "contracts" / "examples")


def run(command, cwd):
    return subprocess.run(command, cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")


def modules():
    """The folder with the installed packages (made once, reused)."""
    base = OUT / "base"
    lock = ROOT / "app" / "package-lock.json"
    stamp = base / "app" / "lock.sha"
    import hashlib
    digest = hashlib.sha256(lock.read_bytes()).hexdigest()
    if (base / "app" / "node_modules").is_dir() and stamp.is_file() and stamp.read_text() == digest:
        return base / "app" / "node_modules"
    shutil.rmtree(base, ignore_errors=True)
    base.mkdir(parents=True)
    copy_sources(base)
    npm = shutil.which("npm")
    done = run([npm, "ci", "--no-audit", "--no-fund"], base / "app")
    if done.returncode:
        sys.exit("npm ci failed:\n" + done.stdout[-2000:] + done.stderr[-2000:])
    stamp.write_text(digest)
    return base / "app" / "node_modules"


def vitest(work, test, name):
    node = shutil.which("node")
    entry = work / "app" / "node_modules" / "vitest" / "vitest.mjs"
    return run([node, str(entry), "run", test, "-t", name], work / "app")


def attempt(name, node_modules, baseline=False):
    path, old, new, test, test_name = BUGS[name]
    with tempfile.TemporaryDirectory(prefix="rb-appbug-") as tmp:
        work = pathlib.Path(tmp)
        copy_sources(work)
        link(node_modules, work / "app" / "node_modules")
        try:
            target = work / path
            text = target.read_text(encoding="utf-8")
            if text.count(old) != 1:
                return f"{name}: NOT APPLIED ({text.count(old)} matches in {path})"
            if not baseline:
                target.write_text(text.replace(old, new), encoding="utf-8", newline="\n")
            r = vitest(work, test, test_name)
        finally:
            # remove the junction first: rmtree must never follow it into the real node_modules
            link_path = work / "app" / "node_modules"
            if link_path.exists():
                os.rmdir(link_path) if os.name == "nt" else os.unlink(link_path)
        if baseline:
            return f"{name}: baseline {'passes' if r.returncode == 0 else 'FAILS'}"
        if r.returncode:
            return f"{name}: caught by {test.split('/')[-1]} -t {test_name!r}"
        return f"{name}: MISSED ({test} -t {test_name!r} passes)"


def main(argv):
    if "--list" in argv:
        for name, (path, _old, _new, test, test_name) in BUGS.items():
            print(f"{name}: {path} -> {test} -t {test_name!r}")
        return
    baseline = "--baseline" in argv
    names = [a for a in argv if not a.startswith("--")] or list(BUGS)
    unknown = [n for n in names if n not in BUGS]
    if unknown:
        sys.exit(f"unknown bugs {unknown}; --list shows them")
    node_modules = modules()
    for name in names:
        print(attempt(name, node_modules, baseline), flush=True)


if __name__ == "__main__":
    main(sys.argv[1:])
