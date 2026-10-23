#!/usr/bin/env python3
"""Decode documented engine reference intervals without EQ or mono folding.

These are observational references, not RPM/load-matched calibration targets.
The source MP3s are never modified. All outputs belong in ignored renders/.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SELECTIONS = {
    "bmw-m4": [(242, 249, "rising-pitch-candidate"), (250, 253, "falling-pitch")],
    "mustang-gt": [(200, 207, "rising-pitch-candidate"), (257, 263, "falling-pitch")],
    "ferrari458": [(49, 55, "rising-pitch-candidate"), (302, 310, "falling-pitch")],
}


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024*1024), b""):
            result.update(block)
    return result.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, default=Path("/mnt/c/Users/berna"))
    parser.add_argument("--ffmpeg", default=os.environ.get("RB_REFERENCE_FFMPEG", "ffmpeg"))
    parser.add_argument("--out", type=Path, default=ROOT / "renders/evidence/audio-experiment-2026-10-22/references")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    config = json.loads((ROOT / "tools/user_engine_reference_sources.json").read_text())
    rows = []
    for recording in config["recordings"]:
        key = recording["key"]
        if key not in SELECTIONS:
            continue
        sources = [p for p in args.source_dir.glob("*.mp3") if recording["video_id"] in p.name]
        if len(sources) != 1:
            raise ValueError(f"Expected one source for {key}, got {sources}")
        source = sources[0]
        before = digest(source)
        for start, end, kind in SELECTIONS[key]:
            # Every selection must lie inside an existing documented excerpt.
            assert any(s["start"] <= start < end <= s["end"] for s in recording["segments"])
            out = args.out / f"{key}-{kind}-{start}.wav"
            command = [args.ffmpeg, "-hide_banner", "-nostdin", "-v", "error", "-y",
                       "-ss", str(start), "-i", str(source), "-t", str(end-start),
                       "-ar", "48000", "-ac", "2", "-c:a", "pcm_f32le", str(out)]
            subprocess.run(command, check=True)
            rows.append(dict(key=key, preset=recording["preset"], source_file=str(source),
                             source_sha256=before, start_s=start, end_s=end, kind=kind,
                             file=out.name, sha256=digest(out),
                             processing="MP3 decoded to 48-kHz stereo float32; no EQ, gain, fade, gate or denoising",
                             uncertainty="No verified RPM, load, pedal lift, DFCO, microphone position or perspective transitions. "
                             "Falling pitch does not establish loaded overrun; rev window does not establish a pull."))
        assert digest(source) == before
    (args.out / "reference-intervals.json").write_text(json.dumps(dict(
        purpose="Observational texture/dynamic comparisons only; no synchronized simulation ground truth. "
                "BMW is turbocharged, whereas the current I6 is naturally aspirated.",
        intervals=rows), indent=2) + "\n")
    print(f"Decoded {len(rows)} documented stereo reference intervals; originals unchanged.")


if __name__ == "__main__":
    main()
