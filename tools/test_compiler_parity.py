"""compiler_parity: how two builds' renders of one drive differ, and when
that's too much."""
from pathlib import Path
import math
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


def write_csv(path, rpm, spl):
    lines = ["t_s,rpm,spl_dB"] + [f"{k},{r},{s}" for k, (r, s) in enumerate(zip(rpm, spl))]
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


class CompilerParityTests(unittest.TestCase):
    def render(self, tmp, name, samples, rpm, spl):
        wav, csv = Path(tmp, f"{name}.wav"), Path(tmp, f"{name}.csv")
        write_wav(wav, samples)
        write_csv(csv, rpm, spl)
        return wav, csv

    def test_differences_are_measured_per_quarter(self):
        sound = [math.sin(0.01 * i) for i in range(4000)]
        nudged = list(sound)
        for i in range(3000, 4000):   # only the last quarter differs: by 1e-3
            nudged[i] += 1e-3 * math.sin(0.37 * i)
        with tempfile.TemporaryDirectory() as tmp:
            a = self.render(tmp, "a", sound, [1000.0, 2000.0], [80.0, 90.0])
            b = self.render(tmp, "b", nudged, [1000.0, 2000.3], [80.0, 90.2])
            d = compiler_parity.compare(*a, *b)
        rms = math.sqrt(sum(x * x for x in sound) / len(sound))
        self.assertEqual(d["quarters"][:3], [0.0, 0.0, 0.0])
        self.assertAlmostEqual(d["sound"], 1e-3 * math.sqrt(0.5) / rms, delta=2e-5)
        self.assertAlmostEqual(d["rpm"], 0.3, places=6)
        self.assertAlmostEqual(d["spl"], 0.2, places=6)

    def test_the_tolerances_decide(self):
        ok = {"sound": 1e-4, "rpm": 0.1, "spl": 0.1, "quarters": [1e-4] * 4, "blocks": (2, 2)}
        self.assertEqual(compiler_parity.out_of_tolerance(ok), [])
        for what, worse in (("sound", 2e-3), ("rpm", 1.0), ("spl", 1.0)):
            with self.subTest(what):
                self.assertEqual(compiler_parity.out_of_tolerance(dict(ok, **{what: worse})), [what])
        self.assertTrue(compiler_parity.out_of_tolerance({"length": (10, 11)}))
        self.assertTrue(compiler_parity.out_of_tolerance(dict(ok, blocks=(2, 3))))


if __name__ == "__main__":
    unittest.main()
