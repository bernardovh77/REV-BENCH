#!/usr/bin/env python3
"""Stage 123's proof that the host's offline mode renders what rb_cli renders.

    python tools/host_parity.py RB_CLI RB_HOST_RENDER OUTDIR [CASE...]

Each case is one `rb_cli sound` drive. It is rendered as a float WAV with
`--trace`, which also writes the commands the drive gave the engine, as
host/include/rb_host_trace.h describes. rb_host_render then plays that trace
through the host's offline mode (no device, a block at a time, the commands
taken from the host's command queue as the render thread takes them), and the
two WAVs must be identical, byte for byte. The same compiler on both sides:
across compilers libm's last bits differ (docs/app-design.md section 8).

Exit code 0 if every case is identical.
"""
from pathlib import Path
import subprocess
import sys

# short drives: the point is the path through the host, not the length. Each exercises
# something a driver does: the key, a blip, the rev limiter, a dyno, a car in gear, a
# stereo scene, engine fields edited before the build, settings (layers, listener, seed).
CASES = {
    "idle-i4-low": ["--preset", "i4", "--idle", "--quality", "low", "--seconds", "3"],
    "rev-v8cross-low": ["--preset", "v8-cross", "--rev", "--quality", "low", "--seconds", "7.5"],
    "sweep-i6-low": ["--preset", "i6", "--sweep", "--quality", "low", "--pull-seconds", "1",
                     "--coast-seconds", "1", "--seconds", "5"],
    "hold-v8flat-dyno": ["--preset", "v8-flat", "--hold", "--rpm", "4000", "--quality", "low",
                         "--seconds", "2"],
    "hold-closed-throttle": ["--preset", "i4", "--hold", "--rpm", "3000", "--throttle", "0",
                             "--quality", "low", "--seconds", "2"],
    "rev-boxer-passby": ["--preset", "boxer4", "--rev", "--scene", "passby", "--quality", "low",
                         "--seconds", "4"],
    "idle-cabin-load": ["--preset", "v8-cross", "--idle", "--listener", "cabin", "--load", "10",
                        "--layers", "pipes,flow", "--quality", "low", "--seconds", "3"],
    "idle-headers-seed": ["--preset", "i4", "--idle", "--quality", "low", "--headers", "0.5,0.4,0.3,0.5",
                          "--muffler", "chambered", "--seed", "7", "--variation", "0.5",
                          "--gain", "0.7", "--seconds", "3"],
    "idle-i4-high": ["--preset", "i4", "--idle", "--seconds", "2"],
}


def run(command):
    r = subprocess.run([str(c) for c in command], capture_output=True, text=True, encoding="utf-8")
    return r.returncode, r.stdout + r.stderr


def first_difference(a: bytes, b: bytes):
    """The offset of the first byte that differs, or None."""
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i
    return None if len(a) == len(b) else min(len(a), len(b))


def check(cli, render, out, names):
    out.mkdir(parents=True, exist_ok=True)
    differ = 0
    for name in names:
        args = CASES[name]
        ref, trace, host = out / f"{name}.wav", out / f"{name}.rbtrace", out / f"{name}.host.wav"
        code, text = run([cli, "sound", *args, "--float", "-o", ref, "--trace", trace])
        if code:
            print(f"{name}: rb_cli failed ({code})\n{text}")
            differ += 1
            continue
        code, text = run([render, trace, "-o", host, "--float"])
        if code:
            print(f"{name}: rb_host_render failed ({code})\n{text}")
            differ += 1
            continue
        a, b = ref.read_bytes(), host.read_bytes()
        at = first_difference(a, b)
        if at is None:
            print(f"{name}: identical ({len(a)} bytes, {len(trace.read_text().splitlines())} trace lines)", flush=True)
        else:
            differ += 1
            print(f"{name}: DIFFERS at byte {at} of {len(a)} / {len(b)}", flush=True)
    print(f"{len(names)} cases: " + ("all identical" if not differ else f"{differ} differ"))
    return 1 if differ else 0


def main(argv):
    if len(argv) < 4:
        sys.exit(__doc__)
    names = argv[4:] or list(CASES)
    unknown = [n for n in names if n not in CASES]
    if unknown:
        sys.exit(f"host_parity: unknown cases {unknown}; they are {list(CASES)}")
    return check(argv[1], argv[2], Path(argv[3]), names)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
