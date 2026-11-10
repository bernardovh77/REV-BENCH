"""tools/run_tests.py: its timeouts, its order and its pre-flight check."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

import fake_program
import run_tests

PY = sys.executable


def alive(pid):
    """Whether a process with this id is running."""
    if os.name == "nt":
        out = subprocess.run(["tasklist", "/FI", f"PID eq {pid}", "/NH"],
                             capture_output=True, text=True).stdout
        return str(pid) in out
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    return True


class TimeoutTests(unittest.TestCase):
    def test_a_hang_and_the_child_it_started_are_killed(self):
        with tempfile.TemporaryDirectory() as tmp:
            pidfile = Path(tmp, "child.pid")
            # a parent that starts a sleeping child, notes its id and sleeps too
            hang = ("import subprocess,sys,time;"
                    f"c=subprocess.Popen([sys.executable,'-c','import time;time.sleep(120)']);"
                    f"open(sys.argv[1],'w').write(str(c.pid));time.sleep(120)")
            start = time.monotonic()
            result = run_tests.run_command([PY, "-c", hang, str(pidfile)], tmp, dict(os.environ), 3)
            took = time.monotonic() - start
            child = int(pidfile.read_text())
            time.sleep(0.5)
            self.assertEqual(result, "timeout")
            self.assertLess(took, 30)
            self.assertFalse(alive(child), "the hang's child outlived the timeout")

    def test_a_quick_command_passes_or_fails_by_its_exit_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(run_tests.run_command([PY, "-c", "pass"], tmp, dict(os.environ), 30), "pass")
            self.assertEqual(run_tests.run_command([PY, "-c", "raise SystemExit(3)"], tmp,
                                                   dict(os.environ), 30), "fail")


class OrderTests(unittest.TestCase):
    def prepare(self, name):
        """Suites made of one command each: "bad" fails, the rest pass."""
        code = "raise SystemExit(1)" if name == "bad" else "pass"
        return [[PY, "-c", code]], os.getcwd(), dict(os.environ), 30

    def statuses(self, keep_going):
        results = run_tests.run_suites(["good", "bad", "after"], self.prepare, keep_going)
        return [(name, status) for name, status, _ in results]

    def test_the_first_failed_suite_stops_the_run(self):
        self.assertEqual(self.statuses(False), [("good", "pass"), ("bad", "fail"), ("after", "skipped")])

    def test_keep_going_runs_every_suite(self):
        self.assertEqual(self.statuses(True), [("good", "pass"), ("bad", "fail"), ("after", "pass")])


class AppSuiteTests(unittest.TestCase):
    """Stage 124's app suite: a copy of app/ outside the repo, with what its tests read."""

    def fake_repo(self):
        """A repo of its own in a temp folder, swapped in for run_tests.ROOT and OUT."""
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        root, out = Path(tmp.name, "repo"), Path(tmp.name, "out")
        for path in ("app/package.json", "app/package-lock.json", "app/src/shared/a.ts",
                     "app/node_modules/dep/index.js", "app/dist/ui/index.html", "app/native/CMakeLists.txt",
                     "app/native/build/rb_native.node", "contracts/examples/snapshot-v2-i4-idle.bin"):
            (root / path).parent.mkdir(parents=True, exist_ok=True)
            (root / path).write_text("x")
        for name, value in (("ROOT", root), ("OUT", out)):
            self.addCleanup(setattr, run_tests, name, getattr(run_tests, name))
            setattr(run_tests, name, value)
        return root, out

    def test_the_copy_has_the_app_and_the_contracts_examples_at_their_relative_place(self):
        _root, out = self.fake_repo()
        app = run_tests.app_copy()
        self.assertEqual(app, out / "app-copy" / "app")
        self.assertTrue((app / "package.json").is_file())
        self.assertTrue((app / "package-lock.json").is_file())
        self.assertTrue((app / "src" / "shared" / "a.ts").is_file())
        self.assertTrue((out / "app-copy" / "contracts" / "examples" / "snapshot-v2-i4-idle.bin").is_file())

    def test_the_copy_leaves_out_modules_builds_and_the_addons_folder(self):
        self.fake_repo()
        app = run_tests.app_copy()
        for folder in ("node_modules", "dist", "native"):
            self.assertFalse((app / folder).exists(), folder)

    def test_a_second_copy_replaces_the_first(self):
        root, _out = self.fake_repo()
        first = run_tests.app_copy()
        (first / "stale.txt").write_text("old")
        (root / "app" / "src" / "shared" / "a.ts").write_text("new")
        second = run_tests.app_copy()
        self.assertFalse((second / "stale.txt").exists())
        self.assertEqual((second / "src" / "shared" / "a.ts").read_text(), "new")

    def test_the_suite_is_known_needs_node_and_npm_and_checks_the_generated_decoder_first(self):
        self.fake_repo()
        self.assertIn("app", run_tests.SUITES)
        self.assertEqual(run_tests.NEEDS["app"], ["node", "npm"])
        self.assertIn("app", run_tests.TIMEOUTS)
        commands, cwd, _env, _timeout = run_tests.run_suite("app", {})
        flat = [" ".join(map(str, c if isinstance(c, list) else c[0])) for c in commands]
        self.assertIn("gen_snapshot_ts.py --check", flat[0])
        self.assertTrue(any(" ci " in c for c in flat))
        self.assertTrue(any(c.endswith("typecheck") for c in flat))
        self.assertTrue(any(c.endswith("build") for c in flat))
        self.assertTrue(flat[-1].endswith("test"))
        self.assertEqual(cwd.name, "app")


@unittest.skipUnless(fake_program.available(), "on Windows fake programs need a C compiler")
class PreflightTests(unittest.TestCase):
    def tools_dir(self, tmp, node_version, programs):
        """A PATH of fake programs only: node printing node_version, and the others."""
        for name in programs:
            fake_program.make(Path(tmp, name), "")
        fake_program.make(Path(tmp, "node"), f"print('{node_version}')\n")
        return dict(os.environ, PATH=tmp)

    def test_missing_make_and_an_old_node_are_both_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = self.tools_dir(tmp, "v20.11.0", ["npm", "npx"])
            problems = run_tests.preflight(["native", "web"], env)
        self.assertEqual(len(problems), 2, problems)
        self.assertTrue(any("make" in p for p in problems), problems)
        self.assertTrue(any("v20.11.0" in p and "22" in p for p in problems), problems)

    def test_everything_present_and_recent_is_no_problem(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = self.tools_dir(tmp, "v22.3.0", ["make", "npm", "npx"])
            suites = [s for s in run_tests.SUITES if s not in ("cmake", "host", "addon")]
            self.assertEqual(run_tests.preflight(suites, env), [])

    def test_an_old_cmake_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = self.tools_dir(tmp, "v22.3.0", ["ninja"])
            fake_program.make(Path(tmp, "cmake"), "print('cmake version 3.22.1')\n")
            problems = [p for p in run_tests.preflight(["cmake"], env) if "Visual Studio" not in p]
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("3.22", problems[0])

    def test_the_addon_suite_needs_node_npm_cmake_and_ninja(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = self.tools_dir(tmp, "v22.3.0", [])
            problems = [p for p in run_tests.preflight(["addon"], env) if "Visual Studio" not in p]
        for program in ("npm", "cmake", "ninja"):
            self.assertTrue(any(f"{program} isn't on PATH" in p and "addon" in p for p in problems), problems)

    def test_the_python_suite_needs_neither(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(run_tests.preflight(["python"], dict(os.environ, PATH=tmp)), [])


@unittest.skipUnless(fake_program.available(), "on Windows fake programs need a C compiler")
class AddonSuiteTests(unittest.TestCase):
    """The add-on suite (stage 124) installs its build tools outside the repo, builds with
    cmake-js and runs every node:test file one at a time."""

    def commands(self):
        with tempfile.TemporaryDirectory() as tmp:
            for name in ("npm", "node"):
                fake_program.make(Path(tmp, name), "")
            env = dict(os.environ, PATH=tmp)
            out = Path(tmp, "out")
            saved = run_tests.OUT
            run_tests.OUT = out
            try:
                commands = run_tests.addon_commands(env)
                environment = run_tests.addon_env(env)
                copied = sorted(p.name for p in Path(out, "addon-node").iterdir())
            finally:
                run_tests.OUT = saved
            return commands, environment, copied, out

    def test_the_tools_are_installed_outside_the_repo_from_the_locked_copy(self):
        commands, _, copied, out = self.commands()
        install = commands[0]
        self.assertEqual(install[1:3], ["ci", "--no-audit"])
        prefix = Path(install[install.index("--prefix") + 1])
        self.assertEqual(prefix, out / "addon-node")
        self.assertNotIn(run_tests.ROOT, prefix.parents)
        self.assertEqual(copied, ["package-lock.json", "package.json"])

    def test_the_build_is_cmake_js_on_app_native_into_the_out_folder(self):
        commands, _, _, out = self.commands()
        build = [c for c in commands if "build" in c and any("cmake-js" in a for a in c)]
        self.assertEqual(len(build), 1, commands)
        build = build[0]
        self.assertEqual(Path(build[build.index("-d") + 1]), run_tests.ROOT / "app/native")
        self.assertEqual(Path(build[build.index("-O") + 1]), out / "addon-build")
        self.assertEqual(build[build.index("-G") + 1], "Ninja")
        if os.name == "nt":
            self.assertTrue(build[1].endswith("msvc_env.py"), "MSVC through msvc_env on Windows")

    def test_every_test_file_runs_in_one_process_at_a_time_against_the_built_add_on(self):
        commands, environment, _, out = self.commands()
        tests = commands[-1]
        self.assertIn("--test", tests)
        self.assertIn("--test-concurrency=1", tests)
        found = sorted(Path(a).name for a in tests if a.endswith(".test.mjs"))
        self.assertEqual(found, sorted(p.name for p in (run_tests.ROOT / "app/native/test").glob("*.test.mjs")))
        self.assertGreaterEqual(len(found), 5)
        self.assertEqual(Path(environment["RB_ADDON"]), out / "addon-build" / "rb_native.node")


if __name__ == "__main__":
    unittest.main()
