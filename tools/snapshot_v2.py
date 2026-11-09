#!/usr/bin/env python3
"""The executable check of contracts/snapshot-v2.md: reads the engine's
binary snapshots (rb_get_snapshot, rb_cli sound --snapshots) and refuses any
that break the contract.

    python tools/snapshot_v2.py FILE      checks a stream of snapshots

A stream is snapshots one after another, as a host takes them from one sim:
each must be whole and well formed, and together they must move forward
(sequence +1, the physics and audio clocks never back) within one engine
generation; a new generation may start the physics clock again.
"""
import math
from pathlib import Path
import struct
import sys

MAGIC, VERSION = 0x32534252, 2
# (offset, name, type) of the 112-byte header
HEADER = [
    (0, "magic", "u32"), (4, "version", "u32"), (8, "size", "u32"), (12, "cylinders", "u32"),
    (16, "instruments", "u32"), (20, "cylinder_fields", "u32"),
    (24, "reserved0", "u32"), (28, "reserved1", "u32"),
    (32, "build_id", "u64"), (40, "generation", "u64"), (48, "sequence", "u64"),
    (56, "physics_tick", "u64"), (64, "audio_frame", "u64"), (72, "cycle_index", "u64"),
    (80, "physics_hz", "f64"), (88, "audio_hz", "f64"), (96, "phase_rad", "f64"),
    (104, "speed_rad_s", "f64"),
]
HEADER_SIZE = 112
# the instruments, in order: rb.h's telemetry from RB_TLM_RPM on
INSTRUMENTS = [
    "rpm", "rpm_smooth", "cycle_rpm", "map", "torque", "power", "lambda", "spark",
    "idle_valve", "throttle", "throttle_area", "ignition", "starter", "held", "fuel_cut",
    "spark_cut", "limiter_cuts", "overrun_time", "load_torque", "crank_inertia", "air_flow",
    "fuel_flow", "exhaust_temperature", "tailpipe_temperature",
]
CYLINDER_FIELDS = ["pressure", "temperature", "burn", "intake_lift", "exhaust_lift"]
FORMAT = {"u32": "I", "u64": "Q", "f64": "d"}


def fail(why):
    raise ValueError(why)


def split(data):
    """The stream cut into snapshots, by each header's size."""
    out, at = [], 0
    while at < len(data):
        if len(data) - at < HEADER_SIZE:
            fail(f"a snapshot cut short at byte {at}")
        size = struct.unpack_from("<I", data, at + 8)[0]
        if size < HEADER_SIZE or at + size > len(data):
            fail(f"a snapshot at byte {at} says it has {size} bytes")
        out.append(data[at:at + size])
        at += size
    return out


def parse(snap):
    """One snapshot, checked on its own: a dict of its header's fields plus
    'instruments' (name: value) and 'cylinder' (a list of dicts)."""
    if len(snap) < HEADER_SIZE:
        fail("shorter than the header")
    h = {name: struct.unpack_from("<" + FORMAT[kind], snap, offset)[0] for offset, name, kind in HEADER}
    if h["magic"] != MAGIC:
        fail(f"magic {h['magic']:#x}, not RBS2")
    if h["version"] != VERSION:
        fail(f"version {h['version']}, not 2")
    if h["instruments"] != len(INSTRUMENTS) or h["cylinder_fields"] != len(CYLINDER_FIELDS):
        fail("instruments or cylinder fields aren't version 2's")
    if not 1 <= h["cylinders"] <= 16:
        fail(f"{h['cylinders']} cylinders")
    if h["reserved0"] or h["reserved1"]:
        fail("reserved words not 0")
    need = HEADER_SIZE + 8 * len(INSTRUMENTS) + 8 * len(CYLINDER_FIELDS) * h["cylinders"]
    if h["size"] != need or len(snap) != need:
        fail(f"size {h['size']}, the layout needs {need}")
    for name in ("physics_hz", "audio_hz"):
        if not (math.isfinite(h[name]) and h[name] > 0):
            fail(f"{name} {h[name]}")
    if not (0.0 <= h["phase_rad"] < 4 * math.pi):
        fail(f"phase {h['phase_rad']} outside [0, 4 pi)")
    if not (math.isfinite(h["speed_rad_s"]) and h["speed_rad_s"] >= 0):
        fail(f"speed {h['speed_rad_s']}")
    values = struct.unpack_from(f"<{len(INSTRUMENTS)}d", snap, HEADER_SIZE)
    if not all(math.isfinite(v) for v in values):
        fail("an instrument isn't a number")
    h["instruments"] = dict(zip(INSTRUMENTS, values))
    h["cylinder"] = []
    at = HEADER_SIZE + 8 * len(INSTRUMENTS)
    for c in range(h["cylinders"]):
        row = dict(zip(CYLINDER_FIELDS, struct.unpack_from("<5d", snap, at + 40 * c)))
        if not (row["pressure"] > 0 and row["temperature"] > 0):
            fail(f"cylinder {c}: pressure and temperature must be positive")
        if not 0.0 <= row["burn"] <= 1.0:
            fail(f"cylinder {c}: burn fraction {row['burn']}")
        if not (row["intake_lift"] >= 0 and row["exhaust_lift"] >= 0):
            fail(f"cylinder {c}: a negative valve lift")
        h["cylinder"].append(row)
    return h


def check_stream(data):
    """Every snapshot in the stream, checked, and checked against the one
    before it; returns them parsed."""
    snaps = [parse(s) for s in split(data)]
    if not snaps:
        fail("no snapshots")
    for a, b in zip(snaps, snaps[1:]):
        if b["sequence"] != a["sequence"] + 1:
            fail(f"sequence {a['sequence']} then {b['sequence']}")
        if b["audio_frame"] < a["audio_frame"]:
            fail("the audio clock went back")
        if b["generation"] == a["generation"]:
            if b["build_id"] != a["build_id"]:
                fail("the build changed within a generation")
            if b["physics_tick"] < a["physics_tick"]:
                fail("the physics clock went back within a generation")
            if (b["cycle_index"], b["phase_rad"]) < (a["cycle_index"], a["phase_rad"]):
                fail("the crank turned back")
        elif b["generation"] < a["generation"]:
            fail("the generation went back")
    return snaps


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    try:
        snaps = check_stream(Path(sys.argv[1]).read_bytes())
    except ValueError as e:
        sys.exit(f"snapshot_v2: {e}")
    print(f"snapshot_v2: {len(snaps)} snapshots, valid")


if __name__ == "__main__":
    main()
