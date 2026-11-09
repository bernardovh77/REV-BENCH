"""bench_render: the interleaved render-path benchmark's table, its best-of
rounds, and the check that builds of one compiler render the same sound."""
import io
from pathlib import Path
import contextlib
import tempfile
import unittest

import bench_render
import fake_program

# A stand-in for rb_bench_render: prints one CSV row. SPEED and HASH come from
# files beside it, so a test can change what each "build" says; the best of
# several rounds is the smallest cpu_s it was ever given.
FAKE = r'''
import pathlib, sys
here = pathlib.Path(sys.argv[0]).resolve().parent
name = pathlib.Path(sys.argv[0]).stem
speeds = (here / (name + ".speeds")).read_text().split()
used = here / (name + ".used")
n = int(used.read_text()) if used.exists() else 0
used.write_text(str(n + 1))
cpu = float(speeds[min(n, len(speeds) - 1)])
hash_ = (here / (name + ".hash")).read_text().strip()
print("preset,quality,drive,window_s,cpu_s,x_real,worst_ms,p99_ms,deadline_ms,hash,nonfinite,rpm_end")
print(f"i4,low,hold,3.000,{cpu},{3.0 / cpu:.3f},4.000,3.000,2.667,{hash_},0,3000.0")
'''


class BenchRenderTests(unittest.TestCase):
    def build(self, tmp, name, speeds, hash_):
        exe = fake_program.make(Path(tmp, name), FAKE)
        Path(tmp, name + ".speeds").write_text(" ".join(map(str, speeds)))
        Path(tmp, name + ".hash").write_text(hash_)
        return str(exe)

    def run_main(self, argv):
        out = io.StringIO()
        old = bench_render.sys.argv
        bench_render.sys.argv = ["bench_render.py"] + argv
        try:
            with contextlib.redirect_stdout(out):
                code = bench_render.main()
        finally:
            bench_render.sys.argv = old
        return code, out.getvalue()

    @unittest.skipUnless(fake_program.available(), "on Windows the fake program needs a C compiler")
    def test_keeps_each_builds_best_round_and_flags_a_changed_sound(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = self.build(tmp, "before", [2.0, 1.5, 1.8], "aaaa")
            b = self.build(tmp, "after", [1.2, 1.0, 1.4], "bbbb")
            code, text = self.run_main(["--exe", f"gcc:before={a}", "--exe", f"gcc:after={b}",
                                        "--presets", "i4", "--quality", "low", "--drive", "hold",
                                        "--rounds", "3"])
            self.assertEqual(code, 1)                     # same compiler, different hashes
            self.assertIn("HASHES DIFFER", text)
            self.assertIn("gcc:before: 2.00x", text)     # 3.0 s of sound in 1.5 s
            self.assertIn("gcc:after: 3.00x", text)      # in 1.0 s

    @unittest.skipUnless(fake_program.available(), "on Windows the fake program needs a C compiler")
    def test_different_compilers_may_differ_and_same_hashes_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = self.build(tmp, "one", [1.0], "aaaa")
            b = self.build(tmp, "two", [1.0], "bbbb")
            c = self.build(tmp, "three", [1.0], "aaaa")
            code, text = self.run_main(["--exe", f"gcc:one={a}", "--exe", f"msvc:two={b}", "--exe", f"gcc:three={c}",
                                        "--presets", "i4", "--quality", "low", "--drive", "hold", "--rounds", "1"])
            self.assertEqual(code, 0)
            self.assertNotIn("HASHES DIFFER", text)

    def test_the_table_has_a_row_per_cell_and_a_column_per_build(self):
        rows = [{"build": n, "cores": "any", "preset": "i4", "quality": "low", "drive": d,
                 "x_real": x, "worst_ms": "3.1"} for n, d, x in
                (("a", "idle", "2.5"), ("b", "idle", "2.7"), ("a", "hold", "2.0"), ("b", "hold", "2.2"))]
        md = bench_render.markdown(rows, ["a", "b"], ["any"], ["i4"], ["low"], ["idle", "hold"])
        self.assertIn("| i4 | low | idle | 2.50 | 3.10 | 2.70 | 3.10 |", md)
        self.assertIn("| i4 | low | hold | 2.00 | 3.10 | 2.20 | 3.10 |", md)

    def test_core_masks_are_two_disjoint_sets_or_nothing(self):
        masks = bench_render.core_masks()
        if masks:
            self.assertEqual(masks["p"] & masks["e"], 0)


if __name__ == "__main__":
    unittest.main()
