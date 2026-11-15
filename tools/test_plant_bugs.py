"""plant_bugs.py --check: every planted bug's text must still be in the sources."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import plant_bugs

TOOLS = Path(__file__).resolve().parent


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


class PlantBugsCheckTests(unittest.TestCase):
    def test_a_bug_whose_text_is_gone_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write(root / "core/a.c", "int x = 1;\nint y = 2;\n")
            write(root / "tools/bugs/stage1.py", "BUGS = {\n"
                  "    'live': ('core/a.c', 'x = 1', 'x = 3', ['t'], 5),\n"
                  "    'stale': ('core/a.c', 'z = 9', 'z = 0', ['t'], 5),\n"
                  "    'twice': ('core/a.c', ('int', 'y'), ('long', 'w'), ['t'], 5),\n"
                  "    'gone': ('core/b.c', 'q', 'r', ['t'], 5),\n"
                  "}\n")
            stale = plant_bugs.stale_bugs(root, root / "tools/bugs")
        self.assertEqual(sorted((table, bug) for table, bug, _ in stale),
                         [("stage1", "gone"), ("stage1", "stale"), ("stage1", "twice")])

    def test_the_repository_has_no_stale_bug(self):
        result = subprocess.run([sys.executable, str(TOOLS / "plant_bugs.py"), "--check"],
                                capture_output=True, text=True, encoding="utf-8")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class ReplaceOnceTests(unittest.TestCase):
    """A bug's text is found as written, or with its spacing changed."""

    SOURCE = "int f(int a)\n{\n    if (a > 0) return a;\n    return -a;\n}\n"

    def test_text_as_written_is_replaced(self):
        text, count = plant_bugs.replace_once(self.SOURCE, "    if (a > 0) return a;\n", "")
        self.assertEqual(count, 1)
        self.assertEqual(text, "int f(int a)\n{\n    return -a;\n}\n")

    def test_reindented_and_rewrapped_text_is_still_found(self):
        moved = "int f(int a)\n{\n        if (a > 0)\n            return a;\n        return -a;\n}\n"
        text, count = plant_bugs.replace_once(moved, "    if (a > 0) return a;\n", "")
        self.assertEqual(count, 1)
        self.assertEqual(text, "int f(int a)\n{\n        return -a;\n}\n")

    def test_changed_words_are_not_found(self):
        text, count = plant_bugs.replace_once(self.SOURCE, "if (a >= 0) return a;", "")
        self.assertEqual(count, 0)
        self.assertEqual(text, self.SOURCE)

    def test_two_places_as_written_refuse(self):
        twice = self.SOURCE + self.SOURCE
        text, count = plant_bugs.replace_once(twice, "return -a;", "return a;")
        self.assertEqual(count, 2)
        self.assertEqual(text, twice)

    def test_two_places_by_spacing_refuse(self):
        twice = "x  =  1;\nx   =   1;\n"
        text, count = plant_bugs.replace_once(twice, "x = 1;", "x = 2;")
        self.assertEqual(count, 2)
        self.assertEqual(text, twice)

    def test_words_are_not_joined_or_split(self):
        # "return a" must not match "returna", nor "re turn a"
        text, count = plant_bugs.replace_once("returna; re turn a;", "return a;", "")
        self.assertEqual(count, 0)

    def test_blank_text_matches_nothing(self):
        self.assertEqual(plant_bugs.replace_once(self.SOURCE, "  \n", "x"), (self.SOURCE, 0))


if __name__ == "__main__":
    unittest.main()
