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


if __name__ == "__main__":
    unittest.main()
