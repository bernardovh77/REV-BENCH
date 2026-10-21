"""The listening renderer must work on Python without optional analysis packages."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest


class CheckpointRenderTests(unittest.TestCase):
    def test_render_without_site_packages(self):
        script = Path(__file__).with_name("checkpoint_13b.py").resolve()
        with tempfile.TemporaryDirectory() as directory:
            # Exercise the real orchestration in an interpreter with site packages
            # disabled. Only the expensive native recording is substituted.
            fake = Path(directory) / "renderer"
            fake.write_text("#!/usr/bin/env python3\nimport pathlib,sys\n"
                            "pathlib.Path(sys.argv[sys.argv.index('--out')+1]).write_bytes(b'wave')\n"
                            "print('soft-limited samples 0, non-finite 0')\n")
            fake.chmod(0o755)
            result = subprocess.run([sys.executable, "-S", str(script), "--render",
                                     "--cli", str(fake), "--output", directory],
                                    capture_output=True, text=True, timeout=30,
                                    env={k:v for k,v in os.environ.items() if k != "PYTHONPATH"})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((Path(directory) / "recordings.json").is_file())

    def test_missing_analysis_dependencies_are_actionable(self):
        script = Path(__file__).with_name("checkpoint_13b.py").resolve()
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([sys.executable, "-S", str(script), "--analyze",
                                     "--output", directory], capture_output=True, text=True,
                                    env={k:v for k,v in os.environ.items() if k != "PYTHONPATH"})
            self.assertEqual(result.returncode, 2)
            self.assertIn("requirements-analysis.txt", result.stderr)
            self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
