#!/usr/bin/env python3
"""Do two compilers' builds of REV-BENCH sound the same? (stage 122)

    python tools/compiler_parity.py check RB_CLI_A RB_CLI_B WORKDIR
    python tools/compiler_parity.py measure RB_CLI_A RB_CLI_B WORKDIR

Within one compiler a change must leave every render bit-identical (stage
119, tools/cli_parity.py). Across compilers it can't: libm's exp and pow differ
in their last bits between MSVC, gcc's and Apple's libraries, and the engine
carries those bits on. Mostly they stay a billionth of the sound; but where a
decision is made at random against a computed share (the rev limiter's soft
cut), a last bit can flip one, and from there each render is its own valid
run. So the same seeded drives are rendered by both builds and checked twice:

  - before anything can flip, they must agree: the RMS of the difference over
    the drive's first quarter, relative to the sound's RMS, at most 1e-6;
  - over the whole drive they must be the same engine: the overall level, each
    octave band's level, and the speed (rpm, per 128-frame block, on average).

`measure` prints the differences; `check` also fails (exit 1) beyond the
tolerances (docs/stage-122.md). Either renders into WORKDIR.
"""
from pathlib import Path
import os
import struct
import subprocess
import sys

import numpy as np

CASES = {
    "idle-i4": ["--preset", "i4", "--idle", "--seconds", "6"],
    "hold-v8cross": ["--preset", "v8-cross", "--hold", "--rpm", "4000", "--seconds", "3"],
    "rev-i6": ["--preset", "i6", "--rev"],
    "sweep-i4": ["--preset", "i4", "--sweep"],
    "rev-boxer-passby": ["--preset", "boxer4", "--rev", "--scene", "passby", "--seconds", "9"],
}

# Frozen at stage 122 (docs/stage-122.md). MSVC 14.51 against gcc 16.1: the first
# quarter at most 2e-9, the rest identical in level, bands and speed; but on a
# GitHub runner the boxer's pass-by rev flipped a soft cut in its last quarter
# (0.35 of the sound, 10 dB in a block). The statistics are sized by a far
# bigger change: another variation seed changes the level by at most 0.23 dB,
# a band by 1.2 dB and the speed by 24 rpm on average.
FIRST_QUARTER = 1e-6   # RMS of the difference / RMS of the sound, first quarter
LEVEL_DB = 0.5         # the whole drive's level
BAND_DB = 2.0          # each octave band (31.5 Hz-16 kHz) within 60 dB of the loudest
RPM_MEAN = 40.0        # the speed's mean difference, rpm


def render(cli, out, name, args):
    wav, csv = out / f"{name}.wav", out / f"{name}.csv"
    r = subprocess.run([str(cli), "sound", *args, "--float", "-o", str(wav), "--csv", str(csv)],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(f"compiler_parity: {cli} failed on {name}:\n{r.stderr}")
    return wav, csv


def samples(wav):
    """A WAV's samples, its channels averaged."""
    data = Path(wav).read_bytes()
    channels = struct.unpack_from("<H", data, data.find(b"fmt ") + 10)[0]
    at = data.find(b"data")
    n = struct.unpack_from("<I", data, at + 4)[0]
    x = np.frombuffer(data[at + 8:at + 8 + n], dtype="<f4").astype(float)
    return x.reshape(-1, channels).mean(axis=1)


def rpm_of(csv):
    lines = Path(csv).read_text(encoding="utf-8").splitlines()
    column = lines[0].split(",").index("rpm")
    return np.array([float(l.split(",")[column]) for l in lines[1:] if l])


def octave_bands(x, rate=48000):
    """dB of each octave band, 31.5 Hz to 16 kHz."""
    f = np.fft.rfftfreq(len(x), 1.0 / rate)
    power = np.abs(np.fft.rfft(x)) ** 2
    return np.array([10 * np.log10(power[(f >= c / 2 ** 0.5) & (f < c * 2 ** 0.5)].sum() + 1e-30)
                     for c in (31.5 * 2 ** k for k in range(10))])


def compare(a_wav, a_csv, b_wav, b_csv):
    """The differences between two renders of one drive."""
    a, b = samples(a_wav), samples(b_wav)
    ra, rb = rpm_of(a_csv), rpm_of(b_csv)
    if len(a) != len(b):
        return {"length": (len(a), len(b))}
    rms = lambda x: float(np.sqrt(np.mean(x * x))) if len(x) else 0.0
    q = max(1, len(a) // 4)
    sound = rms(a) or 1.0
    ba, bb = octave_bands(a), octave_bands(b)
    heard = ba > ba.max() - 60.0
    n = min(len(ra), len(rb))
    return {
        "first": rms(a[:q] - b[:q]) / sound,
        "quarters": [rms(a[k * q:(k + 1) * q] - b[k * q:(k + 1) * q]) / sound for k in range(4)],
        "level": 20 * np.log10(sound / (rms(b) or 1.0)),
        "bands": float(np.max(np.abs(ba - bb)[heard])) if heard.any() else 0.0,
        "rpm": float(np.mean(np.abs(ra[:n] - rb[:n]))) if n else 0.0,
        "blocks": (len(ra), len(rb)),
    }


def out_of_tolerance(d):
    """What of compare's result is beyond the tolerances: a list of "first",
    "level", "bands", "rpm", or ["length"] if the renders don't line up."""
    if "length" in d or d["blocks"][0] != d["blocks"][1]:
        return ["length"]
    limits = {"first": FIRST_QUARTER, "level": LEVEL_DB, "bands": BAND_DB, "rpm": RPM_MEAN}
    return [what for what, most in limits.items() if not abs(d[what]) <= most]


def main():
    if len(sys.argv) != 5 or sys.argv[1] not in ("check", "measure"):
        raise SystemExit(__doc__)
    mode, a_cli, b_cli, work = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4])
    (work / "a").mkdir(parents=True, exist_ok=True)
    (work / "b").mkdir(parents=True, exist_ok=True)
    failed, table = 0, []

    def say(text):
        table.append(text)
        print(text, flush=True)

    say(f"{'drive':18s} {'first':>8s} {'level dB':>9s} {'bands dB':>9s} {'rpm':>7s}   (tolerances "
        f"{FIRST_QUARTER:g}, {LEVEL_DB:g}, {BAND_DB:g}, {RPM_MEAN:g})")
    for name, args in CASES.items():
        d = compare(*render(a_cli, work / "a", name, args), *render(b_cli, work / "b", name, args))
        bad = out_of_tolerance(d)
        failed += bool(bad)
        if bad == ["length"]:
            say(f"{name:18s} the renders don't line up: {d}")
            continue
        say(f"{name:18s} {d['first']:8.2g} {d['level']:+9.3f} {d['bands']:9.3f} {d['rpm']:7.2f}   "
            f"quarters {' '.join(f'{q:.2g}' for q in d['quarters'])}"
            + ("   OUT OF TOLERANCE: " + ", ".join(bad) if bad else ""))
    say(f"{len(CASES)} drives: " + ("within the tolerances" if not failed else f"{failed} out of tolerance"))
    if os.environ.get("GITHUB_ACTIONS"):   # the table on the run's page too, as a notice
        print("::notice title=compiler parity::" + "%0A".join(line.replace("%", "%25") for line in table))
    return 1 if failed and mode == "check" else 0


if __name__ == "__main__":
    sys.exit(main())
