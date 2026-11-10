"""speed_check: a stage that touches core/ must not lose real time unnoticed.

The verdict compares two builds' bench_render rows cell by cell: the median of
the speed ratios (after / before) decides, the worst cells are named, and a
changed sound is reported (expected when a stage changes the physics), never
failed on."""
import unittest

import speed_check


def rows(before, after, hashes=None):
    """bench_render CSV rows for cells i4, i6, ... with the given x_real."""
    out = []
    for k, (b, a) in enumerate(zip(before, after)):
        cell = {"cores": "p", "preset": f"p{k}", "quality": "low", "drive": "hold"}
        hb, ha = hashes[k] if hashes else ("h", "h")
        out.append(dict(cell, build="before", x_real=str(b), hash=hb))
        out.append(dict(cell, build="after", x_real=str(a), hash=ha))
    return out


class VerdictTests(unittest.TestCase):
    def test_the_same_speed_passes(self):
        v = speed_check.verdict(rows([2.0, 1.5, 1.3], [2.0, 1.5, 1.3]), tolerance=0.03)
        self.assertFalse(v["failed"])
        self.assertAlmostEqual(v["median"], 1.0)
        self.assertEqual(v["changed_sound"], [])

    def test_a_median_slowdown_beyond_the_tolerance_fails(self):
        v = speed_check.verdict(rows([2.0, 1.5, 1.3], [1.9, 1.42, 1.23]), tolerance=0.03)   # about -5 %
        self.assertTrue(v["failed"])
        self.assertLess(v["median"], 0.97)

    def test_one_noisy_cell_does_not_fail_but_is_named(self):
        v = speed_check.verdict(rows([2.0, 1.5, 1.3, 1.4, 2.2], [2.0, 1.5, 1.1, 1.4, 2.2]), tolerance=0.03)
        self.assertFalse(v["failed"])                       # the median is unchanged
        self.assertEqual(v["worst"][0][0], "p p2 low hold")  # but the slow cell leads the list
        self.assertAlmostEqual(v["worst"][0][1], 1.1 / 1.3)

    def test_faster_passes(self):
        v = speed_check.verdict(rows([1.3, 1.4], [1.5, 1.6]), tolerance=0.03)
        self.assertFalse(v["failed"])
        self.assertGreater(v["median"], 1.0)

    def test_a_changed_sound_is_reported_not_failed(self):
        v = speed_check.verdict(rows([2.0, 1.5], [2.0, 1.5], hashes=[("a", "a"), ("a", "b")]), tolerance=0.03)
        self.assertFalse(v["failed"])
        self.assertEqual(v["changed_sound"], ["p p1 low hold"])

    def test_a_cell_one_build_lacks_is_an_error(self):
        r = rows([2.0, 1.5], [2.0, 1.5])
        del r[-1]
        with self.assertRaises(ValueError):
            speed_check.verdict(r, tolerance=0.03)


if __name__ == "__main__":
    unittest.main()
