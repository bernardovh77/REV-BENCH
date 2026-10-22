"""Publishing must preserve references and the last good result on failure."""
import base64
from html.parser import HTMLParser
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from render_results import expected_clips, listening_wav, render_checkpoint, replace_current, write_page


def float_wav(path, samples=(0.1, -0.2, 0.05, -0.1)):
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = struct.pack("<" + "f"*len(samples), *samples)
    fmt = struct.pack("<HHIIHH", 3, 1, 1000, 4000, 4, 32)
    path.write_bytes(b"RIFF" + struct.pack("<I", 36+len(payload)) + b"WAVEfmt "
                     + struct.pack("<I", 16) + fmt + b"data"
                     + struct.pack("<I", len(payload)) + payload)


def checkpoint(folder, samples=(0.1, -0.2, 0.05, -0.1)):
    for _, scenario, name in expected_clips():
        path = folder/name
        float_wav(path, samples*3000 if scenario in ("long-coast", "rear-delete-control") else samples)
        if scenario in ("long-coast", "rear-delete-control"):
            path.with_suffix(".csv").write_text(
                "t_s,throttle,crank_inertia_kgm2\n0,1,3\n1,0,3\n9,0,0.2\n")


class Players(HTMLParser):
    def __init__(self):
        super().__init__()
        self.sources = []

    def handle_starttag(self, tag, attrs):
        if tag == "audio":
            self.sources.append(dict(attrs)["src"])


class RenderResultsTests(unittest.TestCase):
    def test_full_makefile_and_scenario_pipeline_without_site_packages(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            renders = root/"renders"
            renders.mkdir()
            build = root/"build"
            build.mkdir()
            # Substitute only the expensive simulation; run the actual Makefile,
            # feedback script, validation, publication and replacement sequence.
            cli = build/"rb_cli"
            fixture = root/"fixture.wav"
            float_wav(fixture, (0.1, -0.2, 0.05, -0.1)*3000)
            cli.write_text(f"#!{sys.executable} -S\n"
                "from pathlib import Path\nimport sys\n"
                "args=sys.argv\n"
                f"Path(args[args.index('--out')+1]).write_bytes(Path({str(fixture)!r}).read_bytes())\n"
                "if '--csv' in args:\n"
                " Path(args[args.index('--csv')+1]).write_text('t_s,throttle,crank_inertia_kgm2\\n0,1,3\\n1,0,3\\n9,0,0.2\\n')\n"
                "if args[1]=='sound': print('soft-limited samples 0, non-finite 0')\n")
            cli.chmod(0o755)
            script = Path(__file__).with_name("render_results.py").resolve()
            # -S on the orchestrator and fake simulation also proves the complete
            # renderer does not accidentally import numpy/matplotlib.
            command = [sys.executable, "-S", "-c",
                f"import sys;sys.path.insert(0,{str(script.parent)!r});"
                "from pathlib import Path;from render_results import render_checkpoint;"
                f"render_checkpoint(Path({str(cli)!r}),{str(build)!r},Path({str(renders)!r}))"]
            env = {k:v for k,v in os.environ.items() if k not in ("PYTHONPATH", "MAKEFLAGS", "MFLAGS")}
            env["MAKEFLAGS"] = f"--assume-old={cli}"
            result = subprocess.run(command, capture_output=True, text=True, timeout=60, env=env)
            self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertEqual(len(list((renders/"current").rglob("*.wav"))), 34)
            recordings = json.loads((renders/"current/feedback/recordings.json").read_text())
            self.assertEqual(len(recordings), 11)
            self.assertIn("none", recordings["v8-cross-rear-delete-control"]["command"])
            self.assertFalse(list(renders.glob(".pending-results-*")))

    def test_listening_gain_preserves_waveform_and_original(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"input.wav"
            float_wav(path)
            original = path.read_bytes()
            preview, info = listening_wav(path)
            samples = struct.unpack("<4h", preview[44:])
            self.assertAlmostEqual(samples[0]/samples[1], -0.5, places=4)
            self.assertAlmostEqual(samples[2]/samples[0], 0.5, places=4)
            self.assertAlmostEqual(max(abs(x) for x in samples)/32767, 0.6, places=4)
            self.assertEqual(path.read_bytes(), original)
            self.assertAlmostEqual(info["static_preview_gain"], 3, places=5)

    def test_missing_or_short_coast_keeps_previous_result(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            current = root/"current"
            checkpoint(current)
            write_page(current, root/"RESULTS.html")
            before = (root/"RESULTS.html").read_bytes()
            staged = root/"pending/next"
            checkpoint(staged)
            coast = staged/"feedback/i6-long-coast.wav"
            coast.unlink()
            with self.assertRaisesRegex(ValueError, "missing"):
                replace_current(staged, root)
            self.assertEqual((root/"RESULTS.html").read_bytes(), before)
            float_wav(coast, (0.1, -0.2, 0.05, -0.1)*3000)
            coast.with_suffix(".csv").write_text(
                "t_s,throttle,crank_inertia_kgm2\n1,0,3\n5,0,0.2\n")
            with self.assertRaisesRegex(ValueError, "eight-second"):
                replace_current(staged, root)
            self.assertEqual((root/"RESULTS.html").read_bytes(), before)
            self.assertTrue((current/"feedback/i6-long-coast.wav").exists())

    def test_replacement_is_portable_and_preserves_references(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            references = root/"real/reference.wav"
            float_wav(references)
            reference_bytes = references.read_bytes()
            current = root/"current"
            checkpoint(current)
            write_page(current, root/"RESULTS.html")
            (current/"old-generated.mp3").write_bytes(b"old preview")
            staged = root/"pending/next"
            checkpoint(staged, samples=(0.15, -0.2, 0.05, -0.1))
            replace_current(staged, root)
            self.assertEqual(references.read_bytes(), reference_bytes)
            self.assertFalse((current/"old-generated.mp3").exists())
            self.assertFalse((root/"pending/previous").exists())
            page = (root/"RESULTS.html").read_text()
            parser = Players()
            parser.feed(page)
            self.assertEqual(len(parser.sources), 34)
            for src in parser.sources:
                self.assertTrue(src.startswith("data:audio/wav;base64,"))
                audio = base64.b64decode(src.split(",", 1)[1], validate=True)
                self.assertEqual(audio[:4], b"RIFF")
            self.assertEqual(len(json.loads((current/"results.json").read_text())["clips"]), 34)

    def test_page_install_failure_rolls_back_current(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            current = root/"current"
            checkpoint(current)
            write_page(current, root/"RESULTS.html")
            original = (current/"standard/i6-rev.wav").read_bytes()
            previous_page = (root/"RESULTS.html").read_bytes()
            staged = root/"pending/next"
            checkpoint(staged, samples=(0.15, -0.2, 0.05, -0.1))
            replace = Path.replace

            def fail_install(path, destination):
                if path == current/"RESULTS.html":
                    raise OSError("simulated result install failure")
                return replace(path, destination)

            with patch.object(Path, "replace", fail_install):
                with self.assertRaisesRegex(OSError, "install failure"):
                    replace_current(staged, root)
            self.assertEqual((current/"standard/i6-rev.wav").read_bytes(), original)
            self.assertEqual((root/"RESULTS.html").read_bytes(), previous_page)

    def test_invalid_audio_rejected_before_overwrite(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"bad.wav"
            for value in (float("nan"), float("inf"), 1.2):
                float_wav(path, (value,))
                with self.assertRaises(ValueError):
                    listening_wav(path)

    def test_publisher_does_not_require_analysis_packages(self):
        script = Path(__file__).with_name("render_results.py")
        run = subprocess.run([sys.executable, "-S", str(script), "--help"],
                             capture_output=True, text=True, timeout=20)
        self.assertEqual(run.returncode, 0, run.stderr)


if __name__ == "__main__":
    unittest.main()
