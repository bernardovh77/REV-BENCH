"""tier_compare: the qualities it can render, as rb_cli sound arguments."""
import unittest

import tier_compare


class QualityArgsTests(unittest.TestCase):
    def test_named_qualities_go_through(self):
        self.assertEqual(tier_compare.quality_args("low"), ["--quality", "low"])
        self.assertEqual(tier_compare.quality_args("balanced"), ["--quality", "balanced"])

    def test_the_v8_search_slows_only_the_pipes(self):
        """Stage 118b: low's 96 kHz physics, the pipes at 19.2 or 16 kHz (96 kHz / 5 and / 6)."""
        self.assertEqual(tier_compare.quality_args("low-p19"), ["--quality", "low", "--pipe-rate", "19200"])
        self.assertEqual(tier_compare.quality_args("low-p16"), ["--quality", "low", "--pipe-rate", "16000"])

    def test_the_oversample_variants_keep_the_default_pipes(self):
        self.assertEqual(tier_compare.quality_args("os2"), ["-x", "2"])
        self.assertEqual(tier_compare.quality_args("os3"), ["-x", "3"])


if __name__ == "__main__":
    unittest.main()
