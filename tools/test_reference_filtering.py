"""Optional real-FFmpeg integration check for listening-only noise reduction.

RB_REFERENCE_FFMPEG may point to an isolated binary; an unusable/missing FFmpeg
skips this integration test while the ordinary audio analysis tests still run.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import wave

import numpy as np

from audio_wav import read_wav
import prepare_user_references as references


class ReferenceFilteringTests(unittest.TestCase):
    def test_filtered_only_preserves_measurements_sources_and_stereo(self):
        ffmpeg = os.environ.get("RB_REFERENCE_FFMPEG") or shutil.which("ffmpeg")
        if not ffmpeg:
            self.skipTest("FFmpeg is not installed")
        ready = subprocess.run([ffmpeg, "-version"], capture_output=True)
        if ready.returncode:
            self.skipTest("FFmpeg cannot run; set RB_REFERENCE_FFMPEG to a usable binary")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rate = 48000
            t = np.arange(5 * rate) / rate
            # An engine-like harmonic tone on each channel with subsonic rumble.
            x = np.column_stack((0.2 * np.sin(2 * np.pi * 180 * t),
                                 0.2 * np.sin(2 * np.pi * 350 * t)))
            x += (0.15 * np.sin(2 * np.pi * 12 * t))[:, None]
            original = root / "synthetic.wav"
            with wave.open(str(original), "wb") as writer:
                writer.setnchannels(2)
                writer.setsampwidth(2)
                writer.setframerate(rate)
                writer.writeframes(np.rint(x * 32767).astype("<i2").tobytes())
            source = root / "source [test-video].mp3"
            references.run(ffmpeg, "-v", "error", "-i", original, "-c:a", "libmp3lame", "-b:a", "192k", source)
            digest = hashlib.sha256(source.read_bytes()).hexdigest()
            config = root / "sources.json"
            config.write_text(json.dumps({"recordings": [{"key": "test", "video_id": "test-video",
                "title": "Synthetic reference", "preset": "i4", "viewpoint_status": "unverified",
                "segments": [{"start": 0.5, "end": 2.5, "acoustic_group": "brighter"},
                             {"start": 2.5, "end": 4.5, "acoustic_group": "darker"}]}]}))
            catalog = root / "catalog.csv"
            catalog.write_bytes(b"unfiltered measurement catalog\n")
            ordinary_preview = root / "ordinary" / "keep.mp3"
            ordinary_preview.parent.mkdir()
            ordinary_preview.write_bytes(b"existing unfiltered preview")
            args = argparse.Namespace(ffmpeg=ffmpeg, source_dir=root, preview_dir=ordinary_preview.parent,
                catalog=catalog, filtered_listening=False, filtered_only=True, filtered_dir=root / "filtered")
            with patch.object(references, "CONFIG", config):
                references.prepare(args)
            self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), digest)
            self.assertEqual(catalog.read_bytes(), b"unfiltered measurement catalog\n")
            self.assertEqual(ordinary_preview.read_bytes(), b"existing unfiltered preview")
            manifest = json.loads((args.filtered_dir / "filtered-listening-manifest.json").read_text())
            recording = manifest["recordings"][0]
            self.assertEqual(recording["source_sha256"], digest)
            self.assertEqual(recording["viewpoint_status"], "unverified")
            self.assertAlmostEqual(recording["preview_seconds"], 4.7)
            self.assertAlmostEqual(recording["segments"][1]["preview_start"], 2.7)
            filtered, actual_rate = read_wav(recording["segments"][0]["filtered_listening_file"])
            self.assertEqual(actual_rate, rate)
            self.assertEqual(filtered.shape, (2 * rate, 2))
            self.assertGreater(np.sqrt(np.mean((filtered[:, 0] - filtered[:, 1]) ** 2)), 0.1)
            self.assertTrue(np.isfinite(filtered).all())
            # Check the actual signal, not merely that a filter argument was sent.
            spectrum = np.abs(np.fft.rfft(filtered[:, 0]))
            frequencies = np.fft.rfftfreq(len(filtered), 1 / rate)
            rumble = spectrum[np.argmin(abs(frequencies - 12))]
            harmonic = spectrum[np.argmin(abs(frequencies - 180))]
            self.assertLess(rumble / harmonic, 0.4)  # source ratio is 0.75
            self.assertGreater(harmonic / len(filtered), 0.075)  # preserve engine tone


if __name__ == "__main__":
    unittest.main()
