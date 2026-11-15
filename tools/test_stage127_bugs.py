"""Stage 127's planted bugs for its Python tools and its push hook
(tools/bugs/stage127_python.py) still match today's sources: each bug's text
occurs once in its file, and the test meant to catch it exists. A change that
moves a bug's text retargets the bug in the same stage (CLAUDE.md)."""
import importlib
import importlib.util
import unittest

import run_tests

SPEC = importlib.util.spec_from_file_location("stage127_python",
                                              run_tests.ROOT / "tools" / "bugs" / "stage127_python.py")
TABLE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TABLE)


class Stage127BugTableTests(unittest.TestCase):
    def test_every_bug_text_occurs_exactly_once_in_its_file(self):
        for name, (path, old, new, _test) in TABLE.BUGS.items():
            text = (run_tests.ROOT / path).read_text(encoding="utf-8")
            self.assertEqual(text.count(old), 1, f"{name}: {old!r} in {path}")
            self.assertNotEqual(old, new, name)

    def test_every_bug_names_a_test_that_exists(self):
        for name, (_path, _old, _new, test) in TABLE.BUGS.items():
            module, cls, method = test.split(".")
            found = getattr(importlib.import_module(module), cls)
            self.assertTrue(hasattr(found, method), f"{name}: no test {test}")

    def test_the_table_covers_the_hook_the_runner_the_planter_and_the_root(self):
        files = {bug[0] for bug in TABLE.BUGS.values()}
        self.assertEqual(files, {".githooks/pre-push", "tools/run_tests.py", "tools/plant_bugs.py",
                                 "tools/test_repo_root.py"})


if __name__ == "__main__":
    unittest.main()
