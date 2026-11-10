"""Tests for tools/gen_snapshot_ts.py (stage 124): the app's snapshot decoder is generated
from snapshot_v2.py's layout, never copied by hand, and the committed files are what the
generator makes today."""
import json
import unittest
from unittest import mock

import gen_snapshot_ts
import snapshot_v2


class CommittedFilesTests(unittest.TestCase):
    def test_the_committed_decoder_is_what_the_generator_makes(self):
        self.assertEqual(gen_snapshot_ts.TS_FILE.read_text(encoding="utf-8"), gen_snapshot_ts.render_ts(),
                         "stale: run python tools/gen_snapshot_ts.py")

    def test_the_committed_golden_is_what_snapshot_v2_reads_from_the_example(self):
        self.assertEqual(gen_snapshot_ts.GOLDEN_FILE.read_text(encoding="utf-8"), gen_snapshot_ts.render_golden(),
                         "stale: run python tools/gen_snapshot_ts.py")

    def test_the_golden_has_every_snapshot_of_the_example(self):
        golden = json.loads(gen_snapshot_ts.GOLDEN_FILE.read_text(encoding="utf-8"))
        parsed = snapshot_v2.check_stream(gen_snapshot_ts.EXAMPLE.read_bytes())
        self.assertEqual(len(golden), len(parsed))
        self.assertEqual([g["sequence"] for g in golden], [p["sequence"] for p in parsed])

    def test_the_check_mode_reports_a_stale_file(self):
        with mock.patch.object(gen_snapshot_ts, "render_ts", return_value="// something else\n"), \
                mock.patch("sys.argv", ["gen_snapshot_ts.py", "--check"]):
            with self.assertRaises(SystemExit) as stop:
                gen_snapshot_ts.main()
        self.assertIn("stale", str(stop.exception))


class FollowsTheLayoutTests(unittest.TestCase):
    """The offsets come from snapshot_v2's tables: change a table, and the decoder changes."""

    def test_a_moved_header_field_moves_in_the_decoder(self):
        moved = [(offset + 8, name, kind) if name == "cycle_index" else (offset, name, kind)
                 for offset, name, kind in snapshot_v2.HEADER]
        with mock.patch.object(snapshot_v2, "HEADER", moved):
            text = gen_snapshot_ts.render_ts()
        self.assertIn('cycle_index: counter(view, 80, "cycle_index")', text)
        self.assertNotIn('cycle_index: counter(view, 72, "cycle_index")', text)

    def test_a_new_instrument_is_in_the_decoder_and_moves_the_cylinders(self):
        with mock.patch.object(snapshot_v2, "INSTRUMENTS", snapshot_v2.INSTRUMENTS + ["new_one"]):
            text = gen_snapshot_ts.render_ts()
        self.assertIn('"new_one",', text)
        self.assertIn("export const CYLINDERS_OFFSET = 312;", text)

    def test_the_build_id_stays_exact_and_the_counters_are_numbers(self):
        text = gen_snapshot_ts.render_ts()
        self.assertIn("build_id: view.getBigUint64(32, true)", text)
        self.assertIn("build_id: bigint;", text)
        self.assertIn("sequence: number;", text)


if __name__ == "__main__":
    unittest.main()
