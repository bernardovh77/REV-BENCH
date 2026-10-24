"""Preserve useful anti-phase stereo in the Stage041 audio assay."""
import tempfile
from pathlib import Path
import unittest
import numpy as np
from reference_protocol import wav_payload
from combustion_repetition import analyze


class CombustionRepetitionTests(unittest.TestCase):
    def test_antiphase_channels_are_measured_without_cancellation(self):
        rate=48000;t=np.arange(3*rate)/rate
        y=.05*(np.sin(2*np.pi*100*t)+np.sin(2*np.pi*900*t)+np.sin(2*np.pi*4000*t))
        with tempfile.TemporaryDirectory() as tmp:
            wav=Path(tmp)/'anti.wav';wav.write_bytes(wav_payload(np.column_stack([y,-y])))
            r=analyze(wav,0,3)
            self.assertGreater(r['raw_stereo_power']['mean_channel_power'],0)
            np.testing.assert_allclose(r['channels'][0]['repetition_db'],r['channels'][1]['repetition_db'])
            self.assertFalse(r['s1_complete'])


if __name__=='__main__':unittest.main()
