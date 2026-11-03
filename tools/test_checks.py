"""The shared checks and drive instants (stage 119)."""
import csv
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from checks import CheckFailed, clutch_at, lift_at, lift_instant, require
import render_results

BLOCK = 128 / 48000   # rb_cli sound writes a row at the end of each block


def drive(lift_block, clutch_block, blocks=4000):
    """rb_cli sound's --csv rows for a pull lifted in block lift_block and
    clutched in block clutch_block: the change shows in that block's row,
    written at its end."""
    rows = []
    for k in range(blocks):
        rows.append({"t_s": f"{(k + 1) * BLOCK:.9f}", "rpm": f"{7000 - k:.1f}",
                     "throttle": "1" if k < lift_block else "0",
                     "crank_inertia_kgm2": "3.2" if k < clutch_block else "0.2"})
    return rows


class CheckTests(unittest.TestCase):
    def test_require_survives_python_optimisation(self):
        require(True, "never raised")
        with self.assertRaisesRegex(CheckFailed, "the reason"):
            require(False, "the reason")
        self.assertTrue(issubclass(CheckFailed, ValueError))
        # python -O drops every assert; require must still fail.
        tools = Path(__file__).resolve().parent
        result = subprocess.run([sys.executable, "-O", "-c",
                                 "from checks import require; require(1 > 2, 'kept under -O')"],
                                cwd=tools, capture_output=True, text=True, encoding="utf-8")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("kept under -O", result.stderr)

    def test_lift_is_the_start_of_the_first_closed_block(self):
        rows = drive(lift_block=100, clutch_block=100 + 3000)
        t, row = lift_at(rows)
        self.assertAlmostEqual(t, 100 * BLOCK, places=9)   # not 101 blocks: that row's end
        self.assertEqual(row["rpm"], "6901.0")             # the speed at that instant
        t, row = clutch_at(rows)
        self.assertAlmostEqual(t, 3100 * BLOCK, places=9)
        self.assertEqual(lift_instant(drive(0, 10)), 0.0)  # lifted from the start
        with self.assertRaises(CheckFailed):
            lift_instant(drive(10**6, 10**6, blocks=5))

    def test_every_report_uses_the_same_instant(self):
        """render_results' coast markers and measure_lift_feedback's telemetry
        report the lift at the same instant as lift_at."""
        import measure_lift_feedback
        rows = drive(lift_block=1500, clutch_block=1500 + 3000, blocks=5000)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "coast.wav"
            with open(path.with_suffix(".csv"), "w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, list(rows[0]))
                writer.writeheader()
                writer.writerows(rows)
            markers = render_results.coast_markers(path, 30.0, "i4")
            _, lift = measure_lift_feedback.telemetry(path.with_suffix(".csv"))
        self.assertAlmostEqual(markers["lift_s"], 1500 * BLOCK, places=9)
        self.assertAlmostEqual(markers["clutch_s"], 4500 * BLOCK, places=9)
        self.assertEqual(lift, markers["lift_s"])


if __name__ == "__main__":
    unittest.main()
