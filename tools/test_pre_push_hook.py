"""The pre-push hook (.githooks/pre-push) refuses a push while a quick suite fails.

It is run here by a real `sh`, in a scratch git repo whose tools/run_tests.py
is a stand-in that records its arguments and exits as told."""
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HOOK = os.path.join(ROOT, ".githooks", "pre-push")
ZERO = "0" * 40
SOME = "1" * 40

STAND_IN = """import os, sys
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "ran.txt"), "w") as f:
    f.write(" ".join(sys.argv[1:]))
sys.exit(int(os.environ.get("FAKE_EXIT", "0")))
"""


@unittest.skipUnless(shutil.which("sh") and shutil.which("git"), "needs sh and git")
class PrePushHookTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.repo = os.path.join(self.temp.name, "repo")
        os.makedirs(os.path.join(self.repo, "tools"))
        subprocess.run(["git", "init", "-q", self.repo], check=True)
        with open(os.path.join(self.repo, "tools", "run_tests.py"), "w", encoding="utf-8") as f:
            f.write(STAND_IN)
        self.ran = os.path.join(self.repo, "tools", "ran.txt")

    def tearDown(self):
        self.temp.cleanup()

    def push(self, lines, **env):
        """Runs the hook as git does: the refs being pushed on its input."""
        full = dict(os.environ)
        # No venv in this HOME, so the hook falls back to the python on PATH: this one.
        full["HOME"] = self.temp.name
        full["PATH"] = os.path.dirname(sys.executable) + os.pathsep + full["PATH"]
        for name in ("RB_SKIP_PREPUSH", "RB_PREPUSH_SUITES", "FAKE_EXIT"):
            full.pop(name, None)
        full.update(env)
        return subprocess.run(["sh", HOOK.replace(os.sep, "/"), "origin", "url"], cwd=self.repo, env=full,
                              input="".join(lines), capture_output=True, text=True, timeout=120)

    def suites_run(self):
        if not os.path.exists(self.ran):
            return None
        with open(self.ran, encoding="utf-8") as f:
            return f.read()

    def test_green_suites_let_the_push_through(self):
        r = self.push([f"refs/heads/main {SOME} refs/heads/main {ZERO}\n"])
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(self.suites_run(), "native python host")

    def test_a_failing_suite_refuses_the_push(self):
        r = self.push([f"refs/heads/main {SOME} refs/heads/main {ZERO}\n"], FAKE_EXIT="1")
        self.assertEqual(r.returncode, 1)
        self.assertIn("nothing was pushed", r.stderr)

    def test_the_suites_can_be_chosen(self):
        r = self.push([f"refs/heads/main {SOME} refs/heads/main {ZERO}\n"], RB_PREPUSH_SUITES="python")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(self.suites_run(), "python")

    def test_skipping_is_explicit_and_runs_nothing(self):
        r = self.push([f"refs/heads/main {SOME} refs/heads/main {ZERO}\n"], RB_SKIP_PREPUSH="1", FAKE_EXIT="1")
        self.assertEqual(r.returncode, 0)
        self.assertIn("skipped", r.stderr)
        self.assertIsNone(self.suites_run())

    def test_deleting_a_branch_runs_nothing(self):
        r = self.push([f"(delete) {ZERO} refs/heads/old {SOME}\n"], FAKE_EXIT="1")
        self.assertEqual(r.returncode, 0)
        self.assertIsNone(self.suites_run())

    def test_a_push_of_nothing_runs_nothing(self):
        r = self.push([], FAKE_EXIT="1")
        self.assertEqual(r.returncode, 0)
        self.assertIsNone(self.suites_run())


if __name__ == "__main__":
    unittest.main()
