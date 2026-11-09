#!/usr/bin/env python3
"""Stage 121's proof that rb_cli sound renders the same on the public API.

    python tools/cli_parity.py render RB_CLI OUTDIR     every case, into OUTDIR
    python tools/cli_parity.py compare DIR_A DIR_B      byte for byte

Each case is one `rb_cli sound` drive: idle, rev, sweep, hold and long coast,
and the options that reach the engine (listener, layers, load, stereo scene,
muffler, headers, quality, variation, seed, burn model, the research
switches). `render` writes each case's WAV (float), CSV and what it printed;
`compare` checks the WAVs and CSVs are identical, and the printed text too
apart from the line timing the CPU. Same compiler on both sides: across
compilers libm's last bits differ (docs/app-design.md section 8).
"""
from pathlib import Path
import subprocess
import sys

CASES = {
    "idle-i4": ["--preset", "i4", "--idle"],
    "rev-v8cross": ["--preset", "v8-cross", "--rev"],
    "sweep-i6": ["--preset", "i6", "--sweep"],
    "hold-v8flat": ["--preset", "v8-flat", "--hold", "--rpm", "4000"],
    "hold-closed": ["--preset", "i4", "--hold", "--rpm", "3000", "--throttle", "0", "--seconds", "3"],
    "coast-i4": ["--preset", "i4", "--sweep", "--pull-seconds", "4", "--coast-seconds", "6",
                 "--seconds", "16", "--inertia", "2.5"],
    "rev-boxer-passby": ["--preset", "boxer4", "--rev", "--scene", "passby", "--seconds", "9"],
    "idle-cabin-load": ["--preset", "v8-cross", "--idle", "--listener", "cabin", "--load", "10",
                        "--layers", "pipes,flow", "--seconds", "5"],
    "hold-research": ["--preset", "i4", "--hold", "--burn-model", "capped", "--seed", "7",
                      "--momentum-order", "1", "--outlet-radiation", "constant",
                      "--muffler", "chambered", "--equal-headers", "--seconds", "3"],
    "idle-low-headers": ["--preset", "i4", "--idle", "--quality", "low", "--headers",
                         "0.5,0.4,0.3,0.5", "--seconds", "5", "--variation", "0.5",
                         "--overrun-transition", "0.3", "--gain", "0.7"],
    "rev-side-straight": ["--preset", "i6", "--rev", "--listener", "side", "--muffler", "straight",
                          "--seconds", "8", "--layers", "engine"],
}


def render(cli, out):
    out.mkdir(parents=True, exist_ok=True)
    for name, args in CASES.items():
        wav, csv = out / f"{name}.wav", out / f"{name}.csv"
        r = subprocess.run([cli, "sound", *args, "--float", "-o", str(wav), "--csv", str(csv)],
                           capture_output=True, text=True, encoding="utf-8")
        text = r.stdout.replace(str(wav), "WAV").replace(str(csv), "CSV") + r.stderr
        (out / f"{name}.txt").write_text(f"exit {r.returncode}\n{text}", encoding="utf-8")
        print(f"{name}: exit {r.returncode}", flush=True)


def compare(a, b):
    differ = 0
    for name in CASES:
        for ext in ("wav", "csv", "txt"):
            x, y = a / f"{name}.{ext}", b / f"{name}.{ext}"
            if ext == "txt":
                same = [l for l in x.read_text(encoding="utf-8").splitlines() if "cpu " not in l] == \
                       [l for l in y.read_text(encoding="utf-8").splitlines() if "cpu " not in l]
            else:
                same = x.read_bytes() == y.read_bytes()
            if not same:
                differ += 1
                print(f"{name}.{ext}: DIFFERS")
    print(f"{len(CASES)} cases: " + ("all identical" if not differ else f"{differ} files differ"))
    return 1 if differ else 0


if __name__ == "__main__":
    if len(sys.argv) != 4 or sys.argv[1] not in ("render", "compare"):
        sys.exit(__doc__)
    if sys.argv[1] == "render":
        render(sys.argv[2], Path(sys.argv[3]))
    else:
        sys.exit(compare(Path(sys.argv[2]), Path(sys.argv[3])))
