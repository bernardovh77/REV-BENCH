"""tools/run_tests.py: its timeouts, its order and its pre-flight check."""
import os
from pathlib import Path
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
            suites = [s for s in run_tests.SUITES if s != "cmake"]
            self.assertEqual(run_tests.preflight(suites, env), [])

    def test_an_old_cmake_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            env = self.tools_dir(tmp, "v22.3.0", ["ninja"])
            fake_program.make(Path(tmp, "cmake"), "print('cmake version 3.22.1')\n")
            problems = [p for p in run_tests.preflight(["cmake"], env) if "Visual Studio" not in p]
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("3.22", problems[0])

    def test_the_python_suite_needs_neither(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(run_tests.preflight(["python"], dict(os.environ, PATH=tmp)), [])


if __name__ == "__main__":
    unittest.main()
