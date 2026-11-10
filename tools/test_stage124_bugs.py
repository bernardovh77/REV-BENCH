"""The app's planted bugs (tools/bugs/stage124_app.py) still match today's sources: each bug's text
occurs once in its file, and the test meant to catch it is there under that name. A change that
moves a bug's text retargets the bug in the same stage (CLAUDE.md, the C bugs' rule)."""
import importlib.util
import unittest

import run_tests

SPEC = importlib.util.spec_from_file_location("stage124_app", run_tests.ROOT / "tools" / "bugs" / "stage124_app.py")
BUGS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUGS)


class AppBugTableTests(unittest.TestCase):
    def test_every_bug_text_occurs_exactly_once_in_its_file(self):
        for name, (path, old, new, _test, _name) in BUGS.BUGS.items():
            text = (run_tests.ROOT / path).read_text(encoding="utf-8")
            self.assertEqual(text.count(old), 1, f"{name}: {old!r} in {path}")
            self.assertNotEqual(old, new, name)

    def test_every_bug_names_a_test_that_exists(self):
        for name, (_path, _old, _new, test, test_name) in BUGS.BUGS.items():
            source = (run_tests.ROOT / "app" / test).read_text(encoding="utf-8").replace("\\'", "'")
            wanted = test_name
            if test_name.endswith(" uses only tokens"):
                # a test made for each file by it.each("%s uses only tokens"): the file must exist
                file = test_name.split()[0]
                self.assertTrue(list((run_tests.ROOT / "app" / "src" / "ui").rglob(file)), f"{name}: no file {file}")
                wanted = "%s uses only tokens"
            self.assertTrue(wanted in source, f"{name}: no test called {test_name!r} in app/{test}")

    def test_the_names_are_unique_and_there_are_at_least_the_six_the_stage_asks_for(self):
        self.assertGreaterEqual(len(BUGS.BUGS), 6)
        wanted = {"decoder-off-by-one-field", "csp-loosened", "node-integration-on", "reply-id-not-echoed",
                  "panel-uses-a-raw-colour", "restart-without-the-autosaved-preset"}
        self.assertLessEqual(wanted, set(BUGS.BUGS))


if __name__ == "__main__":
    unittest.main()
