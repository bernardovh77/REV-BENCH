#!/usr/bin/env python3
"""Prepare Bernardo's local engine examples; never downloads or alters originals.

python tools/prepare_user_references.py --source-dir C:/Users/berna --ffmpeg PATH

Writes unfiltered mono float32 comparison excerpts to renders/real, a separate
CSV catalog to tools/user_engine_references.csv, and stereo listening compilations
to renders/user-reference-previews (or --preview-dir). Requires FFmpeg only.
Selected source intervals and acoustic groups live in user_engine_reference_sources.json.

--filtered-listening additionally writes conservative noise-reduced listening
copies under --filtered-dir. --filtered-only writes only those copies, leaving
the unfiltered measurements, catalog and ordinary previews untouched. Filtering
does not isolate an engine from overlapping wind, road or other machinery.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ROOT / "tools/user_engine_reference_sources.json"
# A listening aid, never an input to compare_real.py or calibration. Keep the
# reduction modest because engine broadband noise can resemble ambient noise.
FILTERED_CHAIN = "highpass=f=20:p=2,afftdn=nr=6:nf=-50:tn=1"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(ffmpeg, *args):
    return subprocess.run([ffmpeg, "-hide_banner", "-nostdin", "-y", *map(str, args)],
                          check=True, capture_output=True, text=True, encoding="utf-8", errors="replace")


def encode_listening(ffmpeg, listing, destination, title):
    measured = run(ffmpeg, "-f", "concat", "-safe", "0", "-i", listing,
                   "-af", "volumedetect", "-f", "null", "-")
    match = re.search(r"max_volume:\s*([-\d.]+) dB", measured.stderr)
    if not match:
        raise RuntimeError("Could not measure listening preview peak")
    gain_db = -3.0 - float(match.group(1))
    run(ffmpeg, "-v", "error", "-f", "concat", "-safe", "0", "-i", listing,
        "-af", f"volume={gain_db:.4f}dB", "-c:a", "libmp3lame", "-b:a", "192k",
        "-metadata", "title=" + title, destination)
    return gain_db


def prepare_filtered(ffmpeg, recordings, inputs, directory):
    """Separate stereo listening artifacts; no writes to measurement inputs."""
    directory = directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    manifest = {"purpose": "Optional listening aid; do not use for measurements or calibration",
                "isolation": "Engine-dominant audio, not isolated engine stems",
                "filter_chain": FILTERED_CHAIN,
                "processing": "48 kHz stereo float excerpts; 10 ms end fades; compilation only: "
                              "0.7 s gaps, one static gain targeting -3 dBFS, 192 kbit/s MP3",
                "limitations": "Filtering can remove engine texture too. Overlapping road, wind, "
                               "intake and turbo sounds remain inseparable; cabin filtering and "
                               "pass-by Doppler are retained. Viewpoints are unverified.",
                "recordings": []}
    for recording in recordings:
        key, source = recording["key"], inputs[recording["key"]]
        result = {**recording, "source_file": str(source), "source_sha256": sha256(source), "segments": []}
        with tempfile.TemporaryDirectory(prefix="rb-filtered-", dir=directory) as temporary:
            scratch = Path(temporary)
            run(ffmpeg, "-v", "error", "-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo",
                "-t", "0.7", "-c:a", "pcm_f32le", scratch / "silence.wav")
            concat, position = [], 0.0
            for index, segment in enumerate(recording["segments"]):
                start, end = segment["start"], segment["end"]
                duration = end - start
                group = segment["acoustic_group"]
                excerpt = directory / f"user-{key}-{group}-{start}-filtered-listening.wav"
                filters = FILTERED_CHAIN + f",afade=t=in:d=0.01,afade=t=out:st={duration - 0.01}:d=0.01"
                run(ffmpeg, "-v", "error", "-ss", start, "-i", source, "-t", duration,
                    "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-af", filters,
                    "-c:a", "pcm_f32le", excerpt)
                # Copy to generated simple names: concat paths never contain source punctuation.
                shutil.copyfile(excerpt, scratch / f"{index}.wav")
                if index:
                    concat.append("file 'silence.wav'")
                    position += 0.7
                concat.append(f"file '{index}.wav'")
                result["segments"].append({**segment, "filtered_listening_file": str(excerpt),
                    "filter_chain": filters, "preview_start": round(position, 3),
                    "preview_end": round(position + duration, 3)})
                position += duration
            listing = scratch / "clips.txt"
            listing.write_text("\n".join(concat) + "\n", encoding="utf-8")
            preview = directory / f"{key}-filtered-listening.mp3"
            gain_db = encode_listening(ffmpeg, listing, preview,
                                      recording["title"] + " - filtered listening excerpts")
            result.update(preview_file=str(preview), preview_gain_db=gain_db, preview_seconds=position)
            # One matched A/B example per engine: raw first, then filtered, at
            # one shared gain. Fades only on raw; no EQ or denoising there.
            first = recording["segments"][0]
            duration = first["end"] - first["start"]
            fades = f"afade=t=in:d=0.01,afade=t=out:st={duration - 0.01}:d=0.01"
            run(ffmpeg, "-v", "error", "-ss", first["start"], "-i", source, "-t", duration,
                "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-af", fades,
                "-c:a", "pcm_f32le", scratch / "raw.wav")
            pair = scratch / "pair.txt"
            pair.write_text("file 'raw.wav'\nfile 'silence.wav'\nfile '0.wav'\n", encoding="utf-8")
            paired_preview = directory / f"{key}-raw-then-filtered.mp3"
            paired_gain = encode_listening(ffmpeg, pair, paired_preview,
                                          recording["title"] + " - raw then filtered")
            result["paired_comparison"] = {"preview_file": str(paired_preview), "gain_db": paired_gain,
                "source_start": first["start"], "source_end": first["end"], "raw_start": 0,
                "raw_end": duration, "filtered_start": duration + 0.7, "filtered_end": 2 * duration + 0.7,
                "raw_processing": "48 kHz stereo, 10 ms end fades only; shared static compilation gain"}
        manifest["recordings"].append(result)
        print(f"Filtered listening copy {key}: {len(result['segments'])} excerpts, {position:.1f}s", flush=True)
    (directory / "filtered-listening-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def prepare(args):
    ffmpeg = args.ffmpeg or shutil.which("ffmpeg")
    if not ffmpeg:
        raise SystemExit("Supply --ffmpeg PATH or install FFmpeg on PATH.")
    config = json.loads(CONFIG.read_text(encoding="utf-8"))
    source_dir, out, previews = args.source_dir.resolve(), (ROOT / "renders/real").resolve(), args.preview_dir.resolve()
    # Resolve every input before producing files, so a missing source fails early.
    inputs = {}
    for recording in config["recordings"]:
        matches = [p for p in source_dir.glob("*.mp3") if recording["video_id"] in p.name]
        if len(matches) != 1:
            raise SystemExit(f"Expected one MP3 containing {recording['video_id']}; found {len(matches)}.")
        inputs[recording["key"]] = matches[0]
    if args.filtered_listening or args.filtered_only:
        prepare_filtered(ffmpeg, config["recordings"], inputs, args.filtered_dir)
    if args.filtered_only:
        return
    out.mkdir(parents=True, exist_ok=True)
    previews.mkdir(parents=True, exist_ok=True)
    rows, manifest = [], []
    for recording in config["recordings"]:
        key, source = recording["key"], inputs[recording["key"]]
        result = {**recording, "source_file": str(source), "segments": []}
        with tempfile.TemporaryDirectory(prefix="rb-refs-", dir=previews) as temporary:
            scratch = Path(temporary).resolve()
            assert scratch.is_relative_to(previews)  # cleanup is confined to this created folder
            run(ffmpeg, "-v", "error", "-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo",
                "-t", "0.7", "-c:a", "pcm_f32le", scratch / "silence.wav")
            concat, preview_position = [], 0.0
            for index, segment in enumerate(recording["segments"]):
                start, end, group = segment["start"], segment["end"], segment["acoustic_group"]
                duration = end - start
                identifier = f"user-{key}-{group}-{start}"
                reference = out / (identifier + ".wav")
                # Measurement copy: no EQ, gain normalisation, denoising, gate or fade.
                run(ffmpeg, "-v", "error", "-ss", start, "-i", source, "-t", duration,
                    "-map", "0:a:0", "-ar", "48000", "-af", "pan=mono|c0=0.5*c0+0.5*c1",
                    "-c:a", "pcm_f32le", reference)
                # Listening copy: retain stereo and dynamics; suppress only subsonic rumble.
                filters = f"highpass=f=12:p=2,afade=t=in:d=0.01,afade=t=out:st={duration - 0.01}:d=0.01"
                run(ffmpeg, "-v", "error", "-ss", start, "-i", source, "-t", duration,
                    "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-af", filters,
                    "-c:a", "pcm_f32le", scratch / f"{index}.wav")
                if index:
                    concat.append("file 'silence.wav'")
                    preview_position += 0.7
                concat.append(f"file '{index}.wav'")
                result["segments"].append({**segment, "reference_file": str(reference),
                    "preview_start": round(preview_position, 3), "preview_end": round(preview_position + duration, 3)})
                preview_position += duration
                rows.append({"id": identifier, "preview": "", "author": "User-supplied recording",
                    "engine": recording["preset"], "cylinders": recording["cylinders"],
                    "place": "inferred-" + group, "licence": "User-supplied; redistribution not granted",
                    "kind": "rev", "t0": 0, "t1": duration,
                    "label": f"{key} {group} {start}-{end}s", "source_start": start, "source_end": end,
                    "source_url": "https://www.youtube.com/watch?v=" + recording["video_id"]})
            listing = scratch / "clips.txt"
            listing.write_text("\n".join(concat) + "\n", encoding="utf-8")
            measured = run(ffmpeg, "-f", "concat", "-safe", "0", "-i", listing,
                           "-af", "volumedetect", "-f", "null", "-")
            match = re.search(r"max_volume:\s*([-\d.]+) dB", measured.stderr)
            if not match:
                raise RuntimeError("Could not measure preview peak")
            gain_db = -3.0 - float(match.group(1))
            preview = previews / (key + "-engine-reference.mp3")
            run(ffmpeg, "-v", "error", "-f", "concat", "-safe", "0", "-i", listing,
                "-af", f"volume={gain_db:.4f}dB", "-c:a", "libmp3lame", "-b:a", "192k",
                "-metadata", "title=" + recording["title"] + " - engine reference excerpts", preview)
            result.update(preview_file=str(preview), preview_gain_db=gain_db, preview_seconds=preview_position)
        manifest.append(result)
        print(f"Prepared {key}: {len(result['segments'])} excerpts, {preview_position:.1f}s preview", flush=True)
    with args.catalog.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (previews / "reference-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--ffmpeg")
    parser.add_argument("--preview-dir", type=Path, default=ROOT / "renders/user-reference-previews")
    parser.add_argument("--catalog", type=Path, default=ROOT / "tools/user_engine_references.csv")
    parser.add_argument("--filtered-listening", action="store_true", help="also create optional noise-reduced listening copies")
    parser.add_argument("--filtered-only", action="store_true", help="create only filtered listening copies; preserve existing measurement files and catalog")
    parser.add_argument("--filtered-dir", type=Path, default=ROOT / "renders/user-reference-previews/filtered")
    prepare(parser.parse_args())


if __name__ == "__main__":
    main()
