"""Guard the S1 baseline against incomplete or limited analysis recordings."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import numpy as np

import audit_s1


class S1AuditTests(unittest.TestCase):
    def test_renderer_must_report_clean_diagnostics(self):
        self.assertEqual(audit_s1.diagnostics("  soft-limited samples 0, non-finite 0\n"),
                         {"soft_limited_samples": 0, "nonfinite_samples": 0})
        for log in ("", "soft-limited samples 1, non-finite 0",
                    "soft-limited samples 0, non-finite 1",
                    "soft-limited samples 0, non-finite 0\n" * 2):
            with self.subTest(log=log), self.assertRaises(ValueError):
                audit_s1.diagnostics(log)

    def test_rejects_short_wrong_rate_or_stereo_audio(self):
        for shape, rate in (((8 * 48000, 1), 48000), ((30 * 24000, 1), 24000),
                            ((30 * 48000, 2), 48000)):
            with self.subTest(shape=shape, rate=rate), \
                 patch.object(audit_s1, "read_wav", return_value=(np.zeros(shape), rate)), \
                 self.assertRaises(ValueError):
                audit_s1.validate_audio("unused.wav")

    def test_rejects_silent_warm_window(self):
        samples = np.zeros((30 * 48000, 1))
        samples[:6 * 48000:2] = 0.1  # audible startup must not hide a silent warm idle
        with patch.object(audit_s1, "read_wav", return_value=(samples, 48000)), \
             self.assertRaisesRegex(ValueError, "6-30"):
            audit_s1.validate_audio("unused.wav")

    def test_measurement_preserves_intervals_and_failed_acceptance(self):
        m = {"slope": -5, "a_loss": 4, "crest": 12, "above": [-10, -20, -30],
             "repeat": [20.4, -7, -12]}
        with patch.object(audit_s1, "measure", return_value=m) as measure:
            rows = audit_s1.measurements("unused.wav")
        self.assertEqual([call.args[1:] for call in measure.call_args_list], [(3, 8), (6, 30)])
        self.assertFalse(rows[1]["low_repeat_in_s1_range"])
        self.assertEqual(rows[1]["repeat_db"], [20.4, -7, -12])
        m["repeat"][0] = float("nan")
        with patch.object(audit_s1, "measure", return_value=m), self.assertRaises(ValueError):
            audit_s1.measurements("unused.wav")

    def test_check_fails_outside_target_without_claiming_s1_complete(self):
        clean = {"preset": "i4", "windows": [{"start_s": 6, "end_s": 30,
                  "repeat_db": [20.4, -7, -12], "low_repeat_in_s1_range": False}]}
        with tempfile.TemporaryDirectory() as directory, \
             patch.object(audit_s1, "record", return_value=clean):
            # A real file satisfies the CLI existence/hash checks; the renderer is mocked.
            cli = Path(directory) / "renderer"
            cli.write_bytes(b"test renderer")
            argv = ["--cli", str(cli), "--output", directory, "--preset", "i4", "--check"]
            self.assertEqual(audit_s1.main(argv), 1)
            self.assertIn('"s1_complete": false', (Path(directory) / "sound.json").read_text())


if __name__ == "__main__":
    unittest.main()
