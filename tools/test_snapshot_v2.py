"""snapshot-v2: the checker accepts real snapshots and rejects broken ones,
and its layout is the contract's (contracts/snapshot-v2.md)."""
from pathlib import Path
import re
import struct
import unittest

import snapshot_v2

ROOT = Path(__file__).resolve().parents[1]
EXAMPLE = ROOT / "contracts/examples/snapshot-v2-i4-idle.bin"


def patch(data, offset, fmt, value):
    """data with the value packed at offset in the snapshot at index 1."""
    at = len(snapshot_v2.split(data)[0]) + offset
    return data[:at] + struct.pack("<" + fmt, value) + data[at + struct.calcsize(fmt):]


def alone(data, offset, fmt, value):
    """The snapshot at index 1 on its own, with the value packed at offset:
    only its own checks can refuse it, none between snapshots."""
    snap = snapshot_v2.split(data)[1]
    return snap[:offset] + struct.pack("<" + fmt, value) + snap[offset + struct.calcsize(fmt):]


class SnapshotV2Tests(unittest.TestCase):
    def setUp(self):
        self.data = EXAMPLE.read_bytes()

    def test_the_committed_example_is_valid(self):
        snaps = snapshot_v2.check_stream(self.data)
        self.assertGreaterEqual(len(snaps), 4)
        self.assertEqual(snaps[0]["cylinders"], 4)
        snapshot_v2.check_stream(snapshot_v2.split(self.data)[1])   # each is valid alone

    def test_broken_snapshots_are_refused(self):
        broken = {
            "magic": alone(self.data, 0, "I", 0x31534252),
            "version": alone(self.data, 4, "I", 1),
            "size": patch(self.data, 8, "I", 464 + 8),
            "a sequence that doesn't follow": patch(self.data, 48, "Q", 99),
            "the generation changing mid-stream": patch(self.data, 40, "Q", 77),
            "time going backwards": patch(self.data, 56, "Q", 0),
            "a phase out of [0, 4 pi)": alone(self.data, 96, "d", 13.0),
            "a negative speed": alone(self.data, 104, "d", -1.0),
            "a burn fraction over 1": alone(self.data, 112 + 8 * 24 + 8 * 2, "d", 1.5),
            "a pressure not positive": alone(self.data, 112 + 8 * 24, "d", 0.0),
            "a NaN instrument": alone(self.data, 112, "d", float("nan")),
            "cut short": self.data[:-8],
        }
        for what, data in broken.items():
            with self.subTest(what):
                with self.assertRaises(ValueError):
                    snapshot_v2.check_stream(data)

    def test_the_layout_is_the_contracts(self):
        text = (ROOT / "contracts/snapshot-v2.md").read_text(encoding="utf-8")
        rows = re.findall(r"^\| (\d+) \| `(\w+)` \| (u32|u64|f64) \|", text, re.M)
        self.assertEqual([(int(o), n, t) for o, n, t in rows], snapshot_v2.HEADER)
        listed = re.findall(r"^\| (\d+) \| `(\w+)` \| (\S*) \|$", text, re.M)
        self.assertEqual([n for _, n, _ in listed], snapshot_v2.INSTRUMENTS)


if __name__ == "__main__":
    unittest.main()
