"""Keep firing-harmonic measurements independent of listening gain and ear phase."""
import unittest
import numpy as np
from high_rpm_report import window_metrics


class HighRpmMetricsTests(unittest.TestCase):
    def test_known_firing_harmonics_with_opposite_ears(self):
        rate = 48000
        t = np.arange(rate*2)/rate
        x = np.sin(2*np.pi*425*t)+.5*np.sin(2*np.pi*850*t)+.25*np.sin(2*np.pi*1275*t)
        metrics = window_metrics(np.column_stack((x, -x)), rate, [8500, 8500], 6)
        orders = metrics['harmonics_db_relative']
        self.assertAlmostEqual(metrics['firing_hz'], 425)
        self.assertAlmostEqual(orders[1]-orders[0], -6.0206, places=3)
        self.assertAlmostEqual(orders[2]-orders[0], -12.0412, places=3)
        expected_centroid = (425+.25*850+.0625*1275)/1.3125
        self.assertAlmostEqual(metrics['centroid_hz'], expected_centroid, places=3)

    def test_gain_changes_raw_level_without_sharpening_the_spectrum(self):
        rate = 48000
        t = np.arange(rate)/rate
        x = (np.sin(2*np.pi*500*t)+.5*np.sin(2*np.pi*1500*t))[:, None]
        a = window_metrics(x, rate, [7500, 7500], 8)
        b = window_metrics(x*4, rate, [7500, 7500], 8)
        self.assertAlmostEqual(b['one_to_four_khz_dbfs']-a['one_to_four_khz_dbfs'], 12.0412, places=3)
        self.assertAlmostEqual(a['centroid_hz'], b['centroid_hz'])
        np.testing.assert_allclose(a['harmonics_db_relative'], b['harmonics_db_relative'])
        self.assertAlmostEqual(a['one_to_four_khz_db_relative'], b['one_to_four_khz_db_relative'])


if __name__ == '__main__':
    unittest.main()
