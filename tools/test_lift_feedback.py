"""Keep the lift comparison's band power on the input pressure scale."""
import unittest
import numpy as np
from measure_lift_feedback import spectral_metrics


class LiftMetricsTests(unittest.TestCase):
    def test_band_and_centroid_match_known_tones(self):
        t = np.arange(48000) / 48000
        x = .2*np.sin(2*np.pi*100*t) + .1*np.sin(2*np.pi*1000*t)
        measured = spectral_metrics(np.column_stack((x,x)),48000)
        self.assertAlmostEqual(measured['overrun_band_dbfs'],20*np.log10(.1/np.sqrt(2)),places=4)
        self.assertAlmostEqual(measured['centroid_hz'],280,places=3)

    def test_gain_changes_level_and_preserves_spectral_shape(self):
        t = np.arange(48000) / 48000
        x = (.2*np.sin(2*np.pi*100*t) + .1*np.sin(2*np.pi*4000*t))[:,None]
        a,b = spectral_metrics(x,48000),spectral_metrics(.5*x,48000)
        self.assertAlmostEqual(a['above_2k_db'],10*np.log10(.2),places=4)
        self.assertAlmostEqual(b['rms_dbfs']-a['rms_dbfs'],-6.020599913,places=6)
        self.assertAlmostEqual(a['above_2k_db'],b['above_2k_db'],places=8)


if __name__ == '__main__':
    unittest.main()
