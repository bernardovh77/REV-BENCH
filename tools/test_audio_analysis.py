"""Analysis regressions: python -m unittest discover -s tools -p 'test_*.py'."""
import argparse
import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

import numpy as np

from audio_wav import read_wav
import compare_real
import orders
import plot_sound


def chunk(kind, payload):
    return kind + struct.pack("<I", len(payload)) + payload + b"\0" * (len(payload) & 1)


def wav_bytes(values, floating=False, channels=1, extensible=False):
    tag, bits, dtype = (3, 32, "<f4") if floating else (1, 16, "<i2")
    align = channels * bits // 8
    fmt = struct.pack("<HHIIHH", 0xFFFE if extensible else tag, channels,
                      48000, 48000 * align, align, bits)
    if extensible:
        fmt += struct.pack("<HHII", 22, bits, 0, tag) + bytes.fromhex("00001000800000aa00389b71")
    elif floating:
        fmt += b"\0\0"
    payload = chunk(b"fmt ", fmt) + chunk(b"JUNK", b"odd")
    if floating:
        payload += chunk(b"fact", struct.pack("<I", np.size(values) // channels))
    payload += chunk(b"data", np.array(values, dtype=dtype).tobytes())
    return b"RIFF" + struct.pack("<I", len(payload) + 4) + b"WAVE" + payload


class AudioAnalysisTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "audio.wav"

    def write(self, data):
        self.path.write_bytes(data)
        return self.path

    def test_pcm_scale_channels_and_metadata(self):
        x, rate = read_wav(self.write(wav_bytes([-32768, 32767, 0, 16384], channels=2)))
        self.assertEqual(rate, 48000)
        np.testing.assert_array_equal(x, [[-1, 32767 / 32768], [0, 0.5]])

    def test_float_preserves_quiet_samples_and_overload(self):
        expected = [0, 1e-8, -1e-8, 1.25, -2]
        x, _ = read_wav(self.write(wav_bytes(expected, floating=True)))
        np.testing.assert_array_equal(x[:, 0], np.array(expected, dtype=np.float32).astype(float))

    def test_extensible_pcm_and_float(self):
        for floating, values in ((False, [-32768, 16384]), (True, [-1, 0.5])):
            with self.subTest(floating=floating):
                x, _ = read_wav(self.write(wav_bytes(values, floating=floating, extensible=True)))
                np.testing.assert_array_equal(x[:, 0], [-1, 0.5])

    def test_rejects_truncated_and_missing_chunks(self):
        good = wav_bytes([0.5], floating=True)
        for bad in (b"", good[:11], good[:-1], b"RIFF\x04\0\0\0WAVE"):
            with self.subTest(size=len(bad)), self.assertRaises(ValueError):
                read_wav(self.write(bad))

    def test_rejects_nonfinite_empty_and_partial_frames(self):
        for values, channels in (([float("nan")], 1), ([float("inf")], 1), ([], 1), ([1], 2)):
            with self.subTest(values=values, channels=channels), self.assertRaises(ValueError):
                read_wav(self.write(wav_bytes(values, floating=True, channels=channels)))

    def test_existing_tools_use_float_and_channel_policy(self):
        path = self.write(wav_bytes([0.25, 0.75, -0.25, -0.75], floating=True, channels=2))
        x, _ = compare_real.read_wav(path)
        np.testing.assert_array_equal(x, [0.5, -0.5])
        with self.assertRaises(SystemExit):
            plot_sound.read_wav(path)
        self.write(wav_bytes([0.25, -0.25], floating=True))
        with patch.object(plot_sound, "GAIN", 0.5):
            x, _ = plot_sound.read_wav(path)
        np.testing.assert_array_equal(x, [0.5, -0.5])

    def test_compare_float_and_pcm_measurements_agree(self):
        t = np.arange(96000) / 48000
        rng = np.random.default_rng(7)
        signal = 0.2 * np.sin(2 * np.pi * 100 * t) + 0.02 * rng.normal(size=len(t))
        self.write(wav_bytes(signal, floating=True))
        floating = compare_real.measure(self.path, 0, 2)
        self.write(wav_bytes(np.rint(signal * 32768)))
        pcm = compare_real.measure(self.path, 0, 2)
        for key in ("above", "repeat", "crest", "a_loss", "slope"):
            np.testing.assert_allclose(floating[key], pcm[key], atol=0.01, rtol=0)

    def test_orders_accepts_windows_drive_and_label_colons(self):
        spec = r"C:\engine renders\idle.wav@1-5@700-900:idle: original @ tailpipe"
        with patch.object(orders.sys, "argv", ["orders.py", "4", spec]), \
             patch.object(orders, "orders", return_value=(800, 0.8, -12, [])) as measure, \
             contextlib.redirect_stdout(io.StringIO()) as output:
            orders.main()
        measure.assert_called_once_with(r"C:\engine renders\idle.wav", 4, 1, 5, 700, 900)
        self.assertIn("idle: original @ tailpipe", output.getvalue())


    def test_custom_catalog_keeps_engine_and_acoustic_groups_separate(self):
        catalog = Path(self.directory.name) / "catalog.csv"
        catalog.write_text("id,engine,place,kind,t0,t1,label\n"
                           "bright,v8-flat,inferred-brighter,rev,0,2,bright V8\n"
                           "dark,v8-flat,inferred-darker,rev,0,2,dark V8\n"
                           "other,i6,inferred-brighter,rev,0,2,other engine\n", encoding="utf-8")
        for key in ("bright", "dark", "other"):
            (Path(self.directory.name) / (key + ".wav")).write_bytes(b"placeholder")
        args = argparse.Namespace(kind="rev", files=[], catalog=catalog, engine="v8-flat",
                                  place="inferred-brighter", out="unused.png")
        metrics = {"above": [-10, -20, -30], "slope": -4, "a_loss": 5, "crest": 12}
        with patch.object(compare_real, "REAL_DIR", self.directory.name), \
             patch.object(compare_real, "measure", return_value=metrics) as measure, \
             patch.object(compare_real, "chart") as chart, \
             contextlib.redirect_stdout(io.StringIO()):
            compare_real.compare(args)
        self.assertEqual(measure.call_count, 1)
        self.assertEqual(Path(measure.call_args.args[0]).name, "bright.wav")
        self.assertEqual([label for label, _ in chart.call_args.args[0]], ["bright V8"])


if __name__ == "__main__":
    unittest.main()
