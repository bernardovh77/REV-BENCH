"""Stage 124's planted bugs for the Node-API add-on's tests (app/native/test).

python tools/bugs/stage124_addon.py [NAME...]    plant the named bugs (default: all)
python tools/bugs/stage124_addon.py --list       the bugs and the test files that must catch each
python tools/bugs/stage124_addon.py --check      each bug's text still matches exactly once

The repo's sources the add-on is built from (CMakeLists.txt, core/, host/,
third_party/, native/, tests/, experiments/acoustics/, app/native/) are copied
once to a temporary folder, the add-on is built there with cmake-js (the tools
`python tools/run_tests.py addon` installed in <out>/addon-node, out being
$RB_TESTS_OUT or ~/rb-build/tests), and the node:test files run on the
unmodified copy first (the baseline must pass). Each bug is then one text
change to a file under app/native/src of that copy, an incremental rebuild and
the test files named for it; the file is put back after. A bug is caught when
those tests fail, MISSED when they pass, and a build that fails is an ERROR
(the bug's text needs retargeting, not a verdict). Nothing in the repo is
changed.
"""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import run_tests  # noqa: E402  (OUT, node_env, tool, kill_tree: the add-on suite's own)

# what the add-on's CMake build reads, relative to the repo root
COPIED = ["CMakeLists.txt", "core", "host", "third_party", "native", "tests", "experiments/acoustics",
          "tools/build_contract_probe.c", "app/native"]

HOST = "app/native/src/rbn_host.c"
UTIL = "app/native/src/rbn_util.c"
BUILD = "app/native/src/rbn_build.c"
META = "app/native/src/rbn_meta.c"

# name: (file, the text, its replacement, the test files that must fail)
BUGS = {
    # the snapshot is a copy, not a view
    'snapshot-bytes-not-copied': (HOST,
        "    memcpy(data, h->scratch, (size_t)size);\n",
        "    (void)data;\n",
        ["snapshot.test.mjs"]),
    'snapshot-is-a-view-of-the-scratch-buffer': (HOST,
        "    RBN_TRY(napi_create_arraybuffer(env, (size_t)size, &data, &bytes));\n    memcpy(data, h->scratch, (size_t)size);\n",
        "    (void)data;\n    RBN_TRY(napi_create_external_arraybuffer(env, h->scratch, (size_t)size, NULL, NULL, &bytes));\n",
        ["snapshot.test.mjs"]),
    'snapshot-info-sequence-is-the-generation': (HOST,
        'rbn_set_number(env, result, "sequence", (double)meta.sequence)',
        'rbn_set_number(env, result, "sequence", (double)meta.generation)',
        ["snapshot.test.mjs"]),
    # acknowledgments
    'ack-stale-and-refused-mixed-up': (UTIL,
        '{"stale", RB_HOST_ACK_STALE}',
        '{"stale", RB_HOST_ACK_REFUSED}',
        ["commands.test.mjs", "rebuild.test.mjs"]),
    'ack-reset-flag-dropped': (HOST,
        'rbn_set_bool(env, item, "reset", (a->flags & RB_HOST_ACK_F_RESET) != 0)',
        'rbn_set_bool(env, item, "reset", 0)',
        ["rebuild.test.mjs"]),
    'ack-swap-kind-misnamed': (UTIL,
        '{"swap", RB_HOST_CMD_SWAP}',
        '{"swap", RB_HOST_CMD_EPOCH}',
        ["rebuild.test.mjs"]),
    'ack-code-is-the-outcome': (HOST,
        'rbn_set_number(env, item, "code", a->code)',
        'rbn_set_number(env, item, "code", a->outcome)',
        ["commands.test.mjs"]),
    'request-id-off-by-one': (HOST,
        "    return rbn_number(env, (double)request_id);\n",
        "    return rbn_number(env, (double)request_id + 1.0);\n",
        ["commands.test.mjs", "rebuild.test.mjs"]),
    # generations
    'set-param-ignores-the-expected-generation': (HOST,
        "rb_host_set_param(h->host, (int)id, value, expected, &request)",
        "rb_host_set_param(h->host, (int)id, value, 0, &request)",
        ["commands.test.mjs"]),
    'set-engine-live-ignores-the-expected-generation': (HOST,
        "(int)index, value, expected, &request)",
        "(int)index, value, 0, &request)",
        ["commands.test.mjs"]),
    'swap-ignores-the-expected-generation': (HOST,
        "rb_host_swap(h->host, prepare_id, expected, &request)",
        "rb_host_swap(h->host, prepare_id, 0, &request)",
        ["rebuild.test.mjs"]),
    'set-param-value-truncated': (HOST,
        "rb_host_set_param(h->host, (int)id, value, expected, &request)",
        "rb_host_set_param(h->host, (int)id, (double)(int)value, expected, &request)",
        ["commands.test.mjs"]),
    # the handle
    'destroy-does-not-mark-the-handle': (HOST,
        "    h->host = NULL;\n    if (host) rb_host_destroy(host);",
        "    if (host) rb_host_destroy(host);",
        ["lifecycle.test.mjs"]),
    'finalizer-leaks-the-host': (HOST,
        "    rbn_host *h = (rbn_host *)data;\n    host_release(h);\n",
        "    rbn_host *h = (rbn_host *)data;\n",
        ["lifecycle.test.mjs"]),
    'host-methods-do-not-check-the-handle': (HOST,
        "    if (!h->host) {\n        rbn_throw(env, \"DESTROYED\", \"the host was destroyed\");\n        return NULL;\n    }\n",
        "",
        ["lifecycle.test.mjs"]),
    'start-error-ignored': (HOST,
        '    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "start");\n',
        "",
        ["lifecycle.test.mjs"]),
    'select-device-index-ignored': (HOST,
        "rb_host_select_device(h->host, (int)index)",
        "rb_host_select_device(h->host, 0)",
        ["lifecycle.test.mjs"]),
    # names and codes
    'quality-name-mapped-to-the-wrong-enum': (UTIL,
        '{"low", RB_QUALITY_LOW}',
        '{"low", RB_QUALITY_MEDIUM}',
        ["create.test.mjs"]),
    'quality-default-ignores-the-preset': (BUILD,
        "    out->quality = rb_preset_quality(preset);\n",
        "    out->quality = RB_QUALITY_HIGH;\n",
        ["create.test.mjs"]),
    'full-code-misnamed': (UTIL,
        '    case RB_HOST_ERR_FULL:    return "RB_HOST_ERR_FULL";',
        '    case RB_HOST_ERR_FULL:    return "RB_HOST_ERR_STATE";',
        ["commands.test.mjs", "create.test.mjs", "rebuild.test.mjs"]),
    'prepare-state-failed-reads-as-ready': (UTIL,
        '{"failed", RB_HOST_PREPARE_FAILED}',
        '{"failed", RB_HOST_PREPARE_READY}',
        ["rebuild.test.mjs"]),
    'prepare-failure-message-dropped': (HOST,
        'rbn_set_string(env, result, "message", why)',
        'rbn_set_string(env, result, "message", "")',
        ["rebuild.test.mjs"]),
    'discard-does-nothing': (HOST,
        "    const int rc = rb_host_prepare_discard(h->host, prepare_id);\n",
        "    const int rc = RB_HOST_OK;\n",
        ["rebuild.test.mjs"]),
    # reading a BuildSpec
    'edit-index-ignored': (BUILD,
        "rb_host_build_edit(out, (int)field, (int)index, value)",
        "rb_host_build_edit(out, (int)field, 0, value)",
        ["create.test.mjs"]),
    'settle-blocks-ignored': (BUILD,
        "        out->settle_blocks = (int32_t)blocks;\n",
        "        (void)blocks;\n",
        ["create.test.mjs"]),
    'sample-rate-ignored': (BUILD,
        "        out->sample_rate = (int32_t)rate;\n",
        "        (void)rate;\n",
        ["create.test.mjs"]),
    # metadata
    'snapshot-version-wrong': (META,
        'rbn_set_number(env, result, "snapshot", RB_SNAPSHOT_VERSION)',
        'rbn_set_number(env, result, "snapshot", 1)',
        ["meta.test.mjs"]),
    'integer-flag-misnamed': (META,
        '{RB_PF_INTEGER, "integer"}',
        '{RB_PF_LIVE, "integer"}',
        ["meta.test.mjs"]),
    'preset-quality-is-always-high': (META,
        "        const int quality = rb_preset_quality(i);\n",
        "        const int quality = RB_QUALITY_HIGH;\n",
        ["create.test.mjs"]),   # the listed quality must be what an engine built without one gets
    'stats-ring-target-uses-48k': (HOST,
        "(double)s.ring_target * 1000.0 / rate",
        "(double)s.ring_target * 1000.0 / 48000.0",
        ["create.test.mjs"]),
}


def run(command, cwd, env, timeout):
    """Runs a command, returns (exit code, its output); a hang is killed with its children (code -1)."""
    group = ({"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP} if os.name == "nt"
             else {"start_new_session": True})
    process = subprocess.Popen(command, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, encoding="utf-8", errors="replace", **group)
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        run_tests.kill_tree(process)
        output, _ = process.communicate()
        return -1, (output or "") + f"\n[killed after {timeout} s]"
    return process.returncode, output


class Work:
    """The copy of the sources, its build and how to test it."""

    def __init__(self, folder):
        self.folder = pathlib.Path(folder)
        self.env = run_tests.node_env()
        tools = run_tests.OUT / "addon-node"
        self.cmake_js = tools / "node_modules/cmake-js/bin/cmake-js"
        if not self.cmake_js.is_file():
            raise SystemExit(f"cmake-js isn't in {tools}: run `python tools/run_tests.py addon` first")
        self.node = run_tests.tool("node", self.env)
        self.msvc = [sys.executable, str(ROOT / "tools/msvc_env.py")] if os.name == "nt" else []
        self.addon = self.folder / "build" / "rb_native.node"
        self.env["RB_ADDON"] = str(self.addon)
        for item in COPIED:
            source, target = ROOT / item, self.folder / item
            target.parent.mkdir(parents=True, exist_ok=True)
            if source.is_dir():
                shutil.copytree(source, target, ignore=shutil.ignore_patterns("build", "node_modules", "__pycache__"))
            else:
                shutil.copy2(source, target)

    def build(self):
        command = self.msvc + [self.node, str(self.cmake_js), "build", "-d", str(self.folder / "app/native"),
                               "-O", str(self.folder / "build"), "-G", "Ninja"]
        return run(command, self.folder, self.env, 900)

    def test(self, files):
        paths = [str(self.folder / "app/native/test" / name) for name in files]
        return run([self.node, "--test", "--test-concurrency=1", *paths], self.folder / "app/native", self.env, 600)


def check():
    """Every bug's text must be in its file exactly once."""
    bad = 0
    for name, (path, old, new, tests) in BUGS.items():
        text = (ROOT / path).read_text(encoding="utf-8")
        count = text.count(old)
        if count != 1 or old == new:
            bad += 1
            print(f"{name}: {count} matches in {path}")
        for test in tests:
            if not (ROOT / "app/native/test" / test).is_file():
                bad += 1
                print(f"{name}: no test file {test}")
    print("all bugs match" if not bad else f"{bad} problems")
    return 1 if bad else 0


def verdict(work, name):
    path, old, new, tests = BUGS[name]
    target = work.folder / path
    original = target.read_text(encoding="utf-8")
    if original.count(old) != 1:
        return f"{name}: NOT APPLIED ({original.count(old)} matches in {path})"
    target.write_text(original.replace(old, new), encoding="utf-8", newline="\n")
    try:
        code, output = work.build()
        if code != 0:
            return f"{name}: ERROR the planted build failed\n" + "\n".join(output.splitlines()[-12:])
        code, output = work.test(tests)
        failed = [line.strip() for line in output.splitlines() if line.lstrip().startswith("✖") and "(" in line]
        if code != 0:
            first = failed[0] if failed else "the process failed"
            return f"{name}: caught by {', '.join(tests)}: {first}"
        return f"{name}: MISSED ({', '.join(tests)} pass)"
    finally:
        target.write_text(original, encoding="utf-8", newline="\n")


def main(argv):
    if "--list" in argv:
        for name, (path, _old, _new, tests) in BUGS.items():
            print(f"{name:52s} {pathlib.Path(path).name:12s} {', '.join(tests)}")
        return 0
    if "--check" in argv:
        return check()
    names = argv or list(BUGS)
    unknown = [n for n in names if n not in BUGS]
    if unknown:
        print(f"unknown bugs {unknown}; --list shows them")
        return 2
    with tempfile.TemporaryDirectory(prefix="rb-addonbug-") as folder:
        work = Work(folder)
        code, output = work.build()
        if code != 0:
            print("the baseline does not build:\n" + "\n".join(output.splitlines()[-20:]))
            return 1
        everything = sorted({t for _p, _o, _n, tests in BUGS.values() for t in tests})
        code, output = work.test(everything)
        if code != 0:
            print("the baseline does not pass:\n" + "\n".join(output.splitlines()[-30:]))
            return 1
        print("baseline: builds and passes", flush=True)
        missed = 0
        for name in names:
            result = verdict(work, name)
            print(result, flush=True)
            missed += "caught" not in result
        # put the unmodified add-on back in the build folder
        work.build()
        print(f"{len(names) - missed} of {len(names)} caught")
        return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
