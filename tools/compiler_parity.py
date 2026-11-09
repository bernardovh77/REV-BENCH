#!/usr/bin/env python3
"""Do two compilers' builds of REV-BENCH sound the same? (stage 122)

    python tools/compiler_parity.py check RB_CLI_A RB_CLI_B WORKDIR
    python tools/compiler_parity.py measure RB_CLI_A RB_CLI_B WORKDIR

Within one compiler a change must leave every render bit-identical (stage
119, tools/cli_parity.py). Across compilers it can't: libm's exp and pow differ
in their last bits between MSVC, gcc's and Apple's libraries, and the engine
carries those bits on. So the same seeded drives are rendered by both builds
(float WAVs and their per-block CSVs), and the differences must stay within
tolerances frozen from the measurement (docs/stage-122.md):

  - the sound: the RMS of the difference, relative to the sound's own RMS,
    over the whole drive and over each quarter of it;
  - the engine: its speed (rpm) at every 128-frame block;
  - the level: the SPL of every block.

`measure` prints the differences; `check` also fails (exit 1) beyond the
tolerances. Either renders into WORKDIR.
"""
from pathlib import Path
import math
import struct
import subprocess
import sys

CASES = {
    "idle-i4": ["--preset", "i4", "--idle", "--seconds", "6"],
    "hold-v8cross": ["--preset", "v8-cross", "--hold", "--rpm", "4000", "--seconds", "3"],
    "rev-i6": ["--preset", "i6", "--rev"],
    "sweep-i4": ["--preset", "i4", "--sweep"],
    "rev-boxer-passby": ["--preset", "boxer4", "--rev", "--scene", "passby", "--seconds", "9"],
}

# Frozen at stage 122 from MSVC 14.51 against w64devkit's gcc 16.1 (docs/stage-122.md).
# The last-bit differences grow as the engine runs: most drives stay near 1e-9 of
# the sound, but the six's 12 s rev reached 1.2e-4 in its last quarter (the idle
# after the limiter), 0.0013 rpm and 0.078 dB. About 8 times that:
SOUND_RMS = 1e-3     # RMS of the difference / RMS of the sound (-60 dB), each quarter
RPM = 0.5            # rpm, at any block
SPL_DB = 0.5         # dB, at any block


def render(cli, out, name, args):
    wav, csv = out / f"{name}.wav", out / f"{name}.csv"
    r = subprocess.run([str(cli), "sound", *args, "--float", "-o", str(wav), "--csv", str(csv)],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(f"compiler_parity: {cli} failed on {name}:\n{r.stderr}")
    return wav, csv


def samples(wav):
    data = Path(wav).read_bytes()
    at = data.find(b"data")
    n = struct.unpack_from("<I", data, at + 4)[0]
    return struct.unpack_from(f"<{n // 4}f", data, at + 8)


def table(csv):
    lines = Path(csv).read_text(encoding="utf-8").splitlines()
    head = lines[0].split(",")
    rows = [[float(v) for v in l.split(",")] for l in lines[1:] if l]
    return {h: [r[k] for r in rows] for k, h in enumerate(head)}


def compare(a_wav, a_csv, b_wav, b_csv):
    """The differences between two renders of one drive."""
    a, b = samples(a_wav), samples(b_wav)
    if len(a) != len(b):
        return {"length": (len(a), len(b))}
    rms = math.sqrt(sum(x * x for x in a) / len(a)) or 1.0
    quarter = max(1, len(a) // 4)
    quarters = []
    for k in range(4):
        d = [a[i] - b[i] for i in range(k * quarter, min(len(a), (k + 1) * quarter))]
        quarters.append(math.sqrt(sum(x * x for x in d) / max(1, len(d))) / rms)
    ta, tb = table(a_csv), table(b_csv)
    return {
        "sound": max(quarters),
        "quarters": quarters,
        "rpm": max(abs(x - y) for x, y in zip(ta["rpm"], tb["rpm"])),
        "spl": max(abs(x - y) for x, y in zip(ta["spl_dB"], tb["spl_dB"])),
        "blocks": (len(ta["rpm"]), len(tb["rpm"])),
    }


def out_of_tolerance(d):
    """What of compare's result is beyond the tolerances: a list of "sound",
    "rpm", "spl", or ["length"] if the renders don't even line up."""
    if "length" in d or d["blocks"][0] != d["blocks"][1]:
        return ["length"]
    limits = {"sound": SOUND_RMS, "rpm": RPM, "spl": SPL_DB}
    return [what for what, most in limits.items() if not d[what] <= most]


def main():
    if len(sys.argv) != 5 or sys.argv[1] not in ("check", "measure"):
        raise SystemExit(__doc__)
    mode, a_cli, b_cli, work = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])
    (work / "a").mkdir(parents=True, exist_ok=True)
    (work / "b").mkdir(parents=True, exist_ok=True)
    failed = 0
    print(f"{'drive':18s} {'sound':>9s} {'rpm':>9s} {'SPL dB':>9s}   (worst; tolerances "
          f"{SOUND_RMS:g}, {RPM:g}, {SPL_DB:g})")
    for name, args in CASES.items():
        d = compare(*render(a_cli, work / "a", name, args), *render(b_cli, work / "b", name, args))
        bad = out_of_tolerance(d)
        failed += bool(bad)
        if bad == ["length"]:
            print(f"{name:18s} the renders don't line up: {d}")
            continue
        print(f"{name:18s} {d['sound']:9.2g} {d['rpm']:9.2g} {d['spl']:9.2g}   "
              f"quarters {' '.join(f'{q:.2g}' for q in d['quarters'])}{'   OUT OF TOLERANCE: ' + ', '.join(bad) if bad else ''}")
    print(f"{len(CASES)} drives: " + ("within the tolerances" if not failed else f"{failed} out of tolerance"))
    return 1 if failed and mode == "check" else 0


if __name__ == "__main__":
    sys.exit(main())
