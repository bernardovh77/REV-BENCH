"""Stage 058's coast diagnosis on synthetic coasts with known answers."""
import tempfile
import unittest
from pathlib import Path

import numpy as np

import coast_diagnosis as cd
from reference_protocol import wav_payload

RATE = 48000


def coast(noise, seconds=8.0, every_half_order=0.0):
    """A falling sweep, 4000 to 1600 rpm: orders 2 (1), 4 (0.3) and 6 (0.1) exactly by crank
    angle, plus white noise of RMS `noise`; the telemetry with the fuel cut on from 0.2 s."""
    t = np.arange(int(seconds * RATE)) / RATE
    rpm = 4000.0 - 300.0 * t
    theta = 2 * np.pi * np.cumsum(rpm / 60.0) / RATE
    y = 0.1 * (np.cos(2 * theta) + 0.3 * np.cos(4 * theta) + 0.1 * np.cos(6 * theta))
    for h in range(1, 49):   # every half-order to 24, if asked: still nothing between them
        y = y + 0.1 * every_half_order * np.cos(0.5 * h * theta)
    y = y + noise * np.random.default_rng(7).standard_normal(len(t))
    tb = np.arange(0, seconds, 128 / RATE)
    rows = ['t_s,rpm,throttle,fuel_cut']
    for x in tb:
        rows.append(f'{x:.6f},{4000.0 - 300.0 * x:.4f},0,{1 if x > 0.2 else 0}')
    return y.astype(np.float32), '\n'.join(rows) + '\n'


class CoastDiagnosisTests(unittest.TestCase):
    def measure(self, noise, every_half_order=0.0):
        y, rows = coast(noise, every_half_order=every_half_order)
        with tempfile.TemporaryDirectory() as tmp:
            wav, csv, out = Path(tmp) / 'c.wav', Path(tmp) / 'c.csv', Path(tmp) / 'c.json'
            wav.write_bytes(wav_payload(y.reshape(-1, 1)))
            csv.write_text(rows, encoding='utf-8')
            self.assertEqual(cd.main([str(wav), str(csv), '--cylinders', '4', '--lift', '0',
                                      '--clutch', '4', '--json', str(out)]), 0)
            import json
            return json.loads(out.read_text(encoding='utf-8'))['windows']

    def test_a_pure_set_of_orders(self):
        w = self.measure(0.0)
        self.assertEqual(set(w), {'loaded coast', 'neutral fall'})
        c = w['loaded coast']
        self.assertAlmostEqual(c['from_s'], 0.5, delta=0.01)
        self.assertAlmostEqual(c['to_s'], 4.0, delta=0.01)
        self.assertEqual(c['order'], 2.0)
        self.assertGreater(c['order_share'], 0.85)   # 1 / (1 + 0.09 + 0.01)
        self.assertGreater(c['floor_db'], 40.0)
        self.assertGreater(c['repetition'], 0.99)

    def test_noise_lifts_the_floor_and_breaks_the_repetition(self):
        pure, noisy = self.measure(0.0)['loaded coast'], self.measure(0.2)['loaded coast']
        self.assertLess(noisy['floor_db'], pure['floor_db'] - 10.0)
        self.assertLess(noisy['repetition'], pure['repetition'] - 0.05)
        self.assertEqual(noisy['order'], 2.0)

    def test_the_floor_is_between_the_orders(self):
        # every half-order present at a tenth of the dominant one: the floor between them is
        # still empty (deep); the half-orders themselves are not the floor
        c = self.measure(0.0, every_half_order=0.1)['loaded coast']
        self.assertGreater(c['floor_db'], 60.0)
        self.assertEqual(c['order'], 2.0)


if __name__ == '__main__':
    unittest.main()
