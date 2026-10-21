#!/usr/bin/env python3
"""Record the five S1 idle baselines and measure them without extra processing.

Build with `make` first, then `python3 tools/audit_s1.py --cli build/rb_cli`.
WAVs, block telemetry, logs and a machine-readable report stay in ignored renders/.
`--check` additionally fails if any measured low-band repetition misses S1's target;
a successful recording alone never completes S1 or its listening checkpoint.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

import numpy as np

from audio_wav import read_wav
from compare_real import measure

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ("i4", "i6", "v8-cross", "v8-flat", "boxer4")
WINDOWS = ((3, 8), (6, 30))
RATE = 48000
SECONDS = 30


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def core_fingerprint(root=ROOT):
    # Same file ordering as Makefile's committed WebAssembly source fingerprint.
    paths = sorted(list((root / "core/src").glob("*.c"))
                   + list((root / "core/src").glob("*.h"))
                   + list((root / "core/include").glob("*.h")))
    if not paths:
        raise ValueError(f"no core sources under {root}")
    digest = hashlib.sha256()
    for path in paths:
        digest.update(path.read_bytes())
    return digest.hexdigest()


def diagnostics(log):
    matches = re.findall(r"soft-limited samples (\d+), non-finite (\d+)", log)
    if len(matches) != 1:
        raise ValueError("missing or ambiguous renderer diagnostics")
    limited, nonfinite = map(int, matches[0])
    if limited or nonfinite:
        raise ValueError(f"render has {limited} limited and {nonfinite} non-finite samples")
    return {"soft_limited_samples": limited, "nonfinite_samples": nonfinite}


def validate_audio(path):
    samples, rate = read_wav(path)
    if rate != RATE or samples.shape != (SECONDS * RATE, 1):
        raise ValueError(f"expected {SECONDS} s of mono {RATE} Hz audio; "
                         f"got {samples.shape} at {rate} Hz")
    # The shared reader already rejects non-finite samples. Reject silence per
    # analysis window too, before logarithms/correlations can produce bogus data.
    for start, end in WINDOWS:
        window = samples[start * rate:end * rate, 0]
        if not np.any(window - window.mean()):
            raise ValueError(f"no varying audio in seconds {start}-{end}")
    return {"sample_rate_hz": rate, "frames": len(samples), "channels": 1,
            "peak_full_scale": float(np.max(np.abs(samples)))}


def measurements(path):
    rows = []
    for start, end in WINDOWS:
        m = measure(path, start, end)
        scalar = {key: float(m[key]) for key in ("slope", "a_loss", "crest")}
        scalar["above_db"] = [float(v) for v in m["above"]]
        scalar["repeat_db"] = [float(v) for v in m["repeat"]]
        values = [scalar[key] for key in ("slope", "a_loss", "crest")]
        if not all(np.isfinite(v) for v in values + scalar["above_db"] + scalar["repeat_db"]):
            raise ValueError(f"non-finite measurement for seconds {start}-{end}")
        rows.append({"start_s": start, "end_s": end, **scalar,
                     "low_repeat_in_s1_range": 0 <= scalar["repeat_db"][0] <= 15})
    return rows


def record(cli, preset, output, gain):
    wav = output / f"{preset}-idle-float.wav"
    telemetry = output / f"{preset}-idle.csv"
    command = [str(cli), "sound", "--preset", preset, "--idle", "--seconds", str(SECONDS),
               "--quality", "high", "--variation", "1", "--listener", "behind",
               "--gain", str(gain), "--float", "--out", str(wav), "--csv", str(telemetry)]
    print(f"Recording {preset}: {SECONDS} seconds, default high quality", flush=True)
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=900)
    log = output / f"{preset}-idle.log"
    log.write_text(result.stdout + result.stderr, encoding="utf-8")
    result.check_returncode()
    counts = diagnostics(result.stdout)
    audio = validate_audio(wav)
    return {"preset": preset, "command": command, "wav": str(wav),
            "wav_sha256": sha256(wav), "telemetry": str(telemetry), "log": str(log),
            "format": "IEEE float32 (rb_cli --float)", **audio, **counts,
            "windows": measurements(wav)}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", type=Path, default=ROOT / "build/rb_cli")
    parser.add_argument("--output", type=Path, default=ROOT / "renders/s1-audit")
    parser.add_argument("--gain", type=float, default=0.5,
                        help="recording gain, in (0, 1]; default 0.5 for analysis headroom")
    parser.add_argument("--source-root", type=Path, default=ROOT,
                        help="source snapshot used to build --cli (default current repository)")
    parser.add_argument("--preset", choices=PRESETS, action="append",
                        help="audit only the selected preset(s); default all five")
    parser.add_argument("--check", action="store_true", help="also gate low-band repetition")
    args = parser.parse_args(argv)
    if not 0 < args.gain <= 1:
        parser.error("--gain must be finite and in (0, 1]")
    cli, output = args.cli.resolve(), args.output.resolve()
    if not cli.is_file():
        parser.error(f"renderer missing: {cli}; build with make first")
    output.mkdir(parents=True, exist_ok=True)
    presets = list(dict.fromkeys(args.preset or PRESETS))
    provenance = {"core_sources_sha256": core_fingerprint(args.source_root.resolve()),
                  "source_root": str(args.source_root.resolve()), "renderer_sha256": sha256(cli),
                  "analysis_sources_sha256": {name: sha256(ROOT / "tools" / name) for name in
                                               ("audit_s1.py", "compare_real.py", "audio_wav.py")},
                  "quality": "high", "physics_rate_hz": 192000, "pipe_rate_hz": 48000,
                  "variation": 1, "seed": "core default initialization",
                  "listener": "behind", "gain": args.gain,
                  "processing": "Standard simulated listener, anti-alias/DC filters, gain and "
                                "soft limiter; float32 WAV; no additional processing.",
                  "metric": "Unchanged compare_real.measure maximum lag correlation; "
                            "not a crank-phase or ignition-failure measurement."}
    report = {"schema_version": 1, "created_utc": datetime.now(timezone.utc).isoformat(),
              "provenance": provenance, "presets": [], "s1_complete": False,
              "remaining": ["Genuine ignition-misfire model", "Full dyno sweep versus baseline",
                            "Bernardo's held/idle and flat-plane rev listening checkpoints"]}
    for preset in presets:
        report["presets"].append(record(cli, preset, output, args.gain))
    report["all_low_repeat_windows_pass"] = all(
        w["low_repeat_in_s1_range"] for p in report["presets"] for w in p["windows"])
    path = output / "sound.json"
    path.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    for p in report["presets"]:
        for w in p["windows"]:
            lo, mid, hi = w["repeat_db"]
            status = "within target" if w["low_repeat_in_s1_range"] else "outside target"
            print(f"{p['preset']:9s} {w['start_s']:2d}-{w['end_s']:2d} s: "
                  f"repeat {lo:+.2f} / {mid:+.2f} / {hi:+.2f} dB; low band {status}")
    print(f"Wrote {path}; S1 and its listening checkpoint remain open.")
    return int(args.check and not report["all_low_repeat_windows_pass"])


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        sys.exit(f"audit_s1: {error}")
