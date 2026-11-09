"""compiler_parity: how two builds' renders of one drive differ, and when
that's too much."""
from pathlib import Path
import math
import random
import struct
import tempfile
import unittest

import compiler_parity


def write_wav(path, samples, rate=48000):
    """A mono 32-bit float WAV, as rb_cli --float writes."""
    data = struct.pack(f"<{len(samples)}f", *samples)
    fmt = struct.pack("<HHIIHH", 3, 1, rate, rate * 4, 4, 32)
    body = b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", len(data)) + data
    Path(path).write_bytes(b"RIFF" + struct.pack("<I", len(body)) + body)


def write_csv(path, rpm):
    lines = ["t_s,rpm,spl_dB"] + [f"{k},{r},80" for k, r in enumerate(rpm)]
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def noise(n, seed):
    rng = random.Random(seed)
    return [rng.gauss(0.0, 0.1) for _ in range(n)]


class CompilerParityTests(unittest.TestCase):
    def render(self, tmp, name, samples, rpm):
        wav, csv = Path(tmp, f"{name}.wav"), Path(tmp, f"{name}.csv")
        write_wav(wav, samples)
        write_csv(csv, rpm)
        return wav, csv

    def test_a_flip_late_on_passes_the_statistics(self):
        """Two renders the same in their first quarter, then each its own
        (as when a last bit flips a random spark cut): the first quarter
        agrees, and the level, the bands and the speed stay close."""
        n = 48000
        a = noise(n, 1)
        b = a[: n // 2] + noise(n - n // 2, 2)   # its own from half way
        with tempfile.TemporaryDirectory() as tmp:
            d = compiler_parity.compare(*self.render(tmp, "a", a, [1000.0, 2000.0]),
                                        *self.render(tmp, "b", b, [1000.0, 2010.0]))
        self.assertEqual(d["first"], 0.0)
        self.assertLess(abs(d["level"]), 0.2)
        self.assertLess(d["bands"], compiler_parity.BAND_DB)
        self.assertAlmostEqual(d["rpm"], 5.0, places=6)
        self.assertEqual(compiler_parity.out_of_tolerance(d), [])

    def test_an_early_difference_or_a_louder_render_is_caught(self):
        n = 48000
        a = noise(n, 1)
        early = [x + (1e-4 if i == 100 else 0.0) for i, x in enumerate(a)]
        louder = [1.2 * x for x in a]   # +1.6 dB, every band
        with tempfile.TemporaryDirectory() as tmp:
            ra = self.render(tmp, "a", a, [1000.0] * 4)
            d_early = compiler_parity.compare(*ra, *self.render(tmp, "e", early, [1000.0] * 4))
            d_loud = compiler_parity.compare(*ra, *self.render(tmp, "l", louder, [1000.0] * 4))
        self.assertGreater(d_early["first"], compiler_parity.FIRST_QUARTER)
        self.assertEqual(compiler_parity.out_of_tolerance(d_early), ["first"])
        self.assertAlmostEqual(d_loud["level"], -20 * math.log10(1.2), places=3)
        self.assertIn("level", compiler_parity.out_of_tolerance(d_loud))

    def test_one_band_louder_is_caught(self):
        """A quiet 8 kHz whistle on a loud 100 Hz note over a hiss: the level
        hardly moves (0.01 dB) but the 8 kHz band rises some 17 dB."""
        n, rate = 48000, 48000.0
        hiss = noise(n, 3)
        a = [math.sin(2 * math.pi * 100 * i / rate) + 0.1 * h for i, h in enumerate(hiss)]
        b = [x + 0.05 * math.sin(2 * math.pi * 8000 * i / rate) for i, x in enumerate(a)]
        with tempfile.TemporaryDirectory() as tmp:
            d = compiler_parity.compare(*self.render(tmp, "a", a, [1000.0] * 4),
                                        *self.render(tmp, "b", b, [1000.0] * 4))
        self.assertLess(abs(d["level"]), 0.05)
        self.assertGreater(d["bands"], 10.0)
        self.assertEqual(compiler_parity.out_of_tolerance(d), ["first", "bands"])

    def test_the_tolerances_decide(self):
        ok = {"first": 1e-9, "level": 0.1, "bands": 0.5, "rpm": 1.0, "blocks": (2, 2)}
        self.assertEqual(compiler_parity.out_of_tolerance(ok), [])
        for what, worse in (("first", 1e-3), ("level", -1.0), ("bands", 3.0), ("rpm", 100.0)):
            with self.subTest(what):
                self.assertEqual(compiler_parity.out_of_tolerance(dict(ok, **{what: worse})), [what])
        self.assertTrue(compiler_parity.out_of_tolerance({"length": (10, 11)}))
        self.assertTrue(compiler_parity.out_of_tolerance(dict(ok, blocks=(2, 3))))


if __name__ == "__main__":
    unittest.main()
