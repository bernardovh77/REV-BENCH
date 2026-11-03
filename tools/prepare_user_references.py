#!/usr/bin/env python3
"""Prepare Bernardo's local engine examples; never downloads or alters originals.

python tools/prepare_user_references.py --source-dir C:/Users/berna --ffmpeg PATH

Writes unfiltered float32 comparison excerpts to renders/real (stereo, as the
recordings are, unless a recording's "measurement_channels" is 1: the four
legacy recordings of user_engine_reference_sources.json keep their mono
excerpts, an L/R average, so they reproduce), a separate
CSV catalog to tools/user_engine_references.csv, and stereo listening compilations
to renders/user-reference-previews (or --preview-dir). Requires FFmpeg only.
Selected source intervals and acoustic groups live in user_engine_reference_sources.json.

--filtered-listening additionally writes conservative noise-reduced listening
copies under --filtered-dir. --filtered-only writes only those copies, leaving
the unfiltered measurements, catalog and ordinary previews untouched. Filtering
does not isolate an engine from overlapping wind, road or other machinery.
Use --config and --append-catalog to register additional local sources without
replacing the original catalog. --publish-filtered embeds matched listening
pairs into the existing RESULTS.html, preserving its engine examples.
"""
import argparse
from checkpoint_sections import publish_section
import base64
import csv
import hashlib
from html import escape
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from checks import require

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ROOT / "tools/user_engine_reference_sources.json"
# A listening aid, never an input to compare_real.py or calibration. Keep the
# reduction modest because engine broadband noise can resemble ambient noise.
FILTERED_CHAIN = "highpass=f=20:p=2,afftdn=nr=6:nf=-50:tn=1"
GAP_S = 0.7   # the silence between excerpts in every listening compilation, s


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def measurement_channel_args(recording):
    """FFmpeg's channel arguments for a recording's measurement excerpt: stereo
    (each channel's power is measured before averaging), unless the
    configuration asks for the legacy mono fold with "measurement_channels": 1."""
    channels = recording.get("measurement_channels", 2)
    if channels not in (1, 2):
        raise ValueError("Measurement channels must be 1 or 2")
    return ["-af", "pan=mono|c0=0.5*c0+0.5*c1"] if channels == 1 else ["-ac", "2"]


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
                              f"{GAP_S:g} s gaps, one static gain targeting -3 dBFS, 192 kbit/s MP3",
                "limitations": "Filtering can remove engine texture too. Overlapping road, wind, "
                               "intake and turbo sounds remain inseparable; cabin filtering and "
                               "pass-by Doppler are retained. Viewpoint evidence is recorded per source; "
                               "user-reported positions are not independently calibrated.",
                "recordings": []}
    for recording in recordings:
        key, source = recording["key"], inputs[recording["key"]]
        result = {**recording, "source_file": str(source), "source_sha256": sha256(source), "segments": []}
        with tempfile.TemporaryDirectory(prefix="rb-filtered-", dir=directory) as temporary:
            scratch = Path(temporary)
            run(ffmpeg, "-v", "error", "-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo",
                "-t", str(GAP_S), "-c:a", "pcm_f32le", scratch / "silence.wav")
            concat, position = [], 0.0
            for index, segment in enumerate(recording["segments"]):
                start, end = segment["start"], segment["end"]
                duration = end - start
                group = segment["acoustic_group"]
                excerpt = directory / f"user-{key}-{group}-{start}-filtered-listening.wav"
                # Flush the FFT denoiser before cutting to the source interval;
                # otherwise a fractional final block can truncate the recording.
                filters = (f"apad=pad_dur=0.1,{FILTERED_CHAIN},atrim=duration={duration},"
                           f"afade=t=in:d=0.01,afade=t=out:st={duration - 0.01}:d=0.01")
                run(ffmpeg, "-v", "error", "-ss", start, "-i", source, "-t", duration,
                    "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-af", filters,
                    "-c:a", "pcm_f32le", excerpt)
                # Copy to generated simple names: concat paths never contain source punctuation.
                shutil.copyfile(excerpt, scratch / f"{index}.wav")
                if index:
                    concat.append("file 'silence.wav'")
                    position += GAP_S
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
                "raw_end": duration, "filtered_start": duration + GAP_S, "filtered_end": 2 * duration + GAP_S,
                "raw_processing": "48 kHz stereo, 10 ms end fades only; shared static compilation gain"}
        manifest["recordings"].append(result)
        print(f"Filtered listening copy {key}: {len(result['segments'])} excerpts, {position:.1f}s", flush=True)
    (directory / "filtered-listening-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def publish_filtered(directory):
    """Append matched listening pairs without regenerating accepted engine audio."""
    manifest = json.loads((directory / "filtered-listening-manifest.json").read_text(encoding="utf-8"))
    # The gap the pairs actually have, for the prose (stage 119: numbers from the evidence).
    gaps = {round(r["paired_comparison"]["filtered_start"] - r["paired_comparison"]["raw_end"], 3)
            for r in manifest["recordings"]}
    gap = "/".join(f"{g:g}" for g in sorted(gaps))
    target = ROOT / "renders/RESULTS.html"
    page = target.read_text(encoding="utf-8")
    begin, end = "<!-- USER REFERENCES BEGIN -->", "<!-- USER REFERENCES END -->"
    if begin in page:
        before, rest = page.split(begin, 1)
        _, after = rest.split(end, 1)
        page = before + after
    parts = [begin, '<section id="user-references"><h2>New engine references</h2>',
             '<p>Position and distance evidence are recorded below. Microphone response, '
             'recording gain and exact motion remain uncalibrated. '
             'No distance compensation or pitch correction is applied.</p>',
             '<p>Each player contains the original audio first, a ' + gap + '-second gap, then the lightly '
             'filtered copy. Both halves share one fixed listening gain; different recordings '
             'have independent gains. Filtering reduces rumble and background noise modestly, '
             'and may also reduce engine texture. Unfiltered stereo float WAVs remain the measurement sources.</p>']
    for recording in manifest["recordings"]:
        pair = recording["paired_comparison"]
        payload = base64.b64encode(Path(pair["preview_file"]).read_bytes()).decode("ascii")
        parts.append(f'<article><h3>{escape(recording["title"])}</h3>'
                     f'<p>{escape(recording.get("viewpoint_note", "Viewpoint unverified."))}</p>'
                     f'<p>Original: 0–{pair["raw_end"]:.2f}s · '
                     f'Filtered: {pair["filtered_start"]:.2f}–{pair["filtered_end"]:.2f}s</p>'
                     f'<audio controls preload="none" aria-label="{escape(recording["title"], quote=True)} raw then filtered" '
                     f'src="data:audio/mpeg;base64,{payload}"></audio></article>')
    parts += ['</section>', end]
    if page.count('</main>') != 1:
        raise ValueError("Expected one results main element")
    page = page.replace('</main>', '\n'.join(parts) + '</main>', 1)
    link = '<a href="#user-references">New references</a>'
    if link not in page:
        page = page.replace('</nav>', link + '</nav>', 1)
    publish_section(target, 'user-references', '\n'.join(parts))


def prepare(args):
    ffmpeg = args.ffmpeg or shutil.which("ffmpeg")
    if not ffmpeg:
        raise SystemExit("Supply --ffmpeg PATH or install FFmpeg on PATH.")
    config = json.loads(getattr(args, "config", CONFIG).read_text(encoding="utf-8"))
    source_dir, out, previews = args.source_dir.resolve(), (ROOT / "renders/real").resolve(), args.preview_dir.resolve()
    # Resolve every input before producing files, so a missing source fails early.
    inputs = {}
    for recording in config["recordings"]:
        matches = [p for p in source_dir.glob("*.mp3") if recording["video_id"] in p.name]
        if len(matches) != 1:
            raise SystemExit(f"Expected one MP3 containing {recording['video_id']}; found {len(matches)}.")
        expected_hash = recording.get("source_sha256")
        if expected_hash and sha256(matches[0]) != expected_hash:
            raise ValueError(f"Source hash differs for {recording['key']}; no outputs written")
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
        result = {**recording, "source_file": str(source), "source_sha256": sha256(source), "segments": []}
        with tempfile.TemporaryDirectory(prefix="rb-refs-", dir=previews) as temporary:
            scratch = Path(temporary).resolve()
            require(scratch.is_relative_to(previews), 'cleanup is confined to this created folder')
            run(ffmpeg, "-v", "error", "-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo",
                "-t", str(GAP_S), "-c:a", "pcm_f32le", scratch / "silence.wav")
            concat, preview_position = [], 0.0
            for index, segment in enumerate(recording["segments"]):
                start, end, group = segment["start"], segment["end"], segment["acoustic_group"]
                duration = end - start
                identifier = f"user-{key}-{group}-{start}"
                reference = out / (identifier + ".wav")
                # Measurement copy: no EQ, gain normalisation, denoising, gate or fade.
                channel_args = measurement_channel_args(recording)
                run(ffmpeg, "-v", "error", "-ss", start, "-i", source, "-t", duration,
                    "-map", "0:a:0", "-ar", "48000", *channel_args,
                    "-c:a", "pcm_f32le", reference)
                # Listening copy: retain stereo and dynamics; suppress only subsonic rumble.
                filters = f"highpass=f=12:p=2,afade=t=in:d=0.01,afade=t=out:st={duration - 0.01}:d=0.01"
                run(ffmpeg, "-v", "error", "-ss", start, "-i", source, "-t", duration,
                    "-map", "0:a:0", "-ar", "48000", "-ac", "2", "-af", filters,
                    "-c:a", "pcm_f32le", scratch / f"{index}.wav")
                if index:
                    concat.append("file 'silence.wav'")
                    preview_position += GAP_S
                concat.append(f"file '{index}.wav'")
                result["segments"].append({**segment, "reference_file": str(reference),
                    "preview_start": round(preview_position, 3), "preview_end": round(preview_position + duration, 3)})
                preview_position += duration
                rows.append({"id": identifier, "preview": "", "author": "User-supplied recording",
                    "engine": recording["preset"], "cylinders": recording["cylinders"],
                    "place": segment.get("place", "inferred-" + group), "licence": "User-supplied; redistribution not granted",
                    "kind": "rev", "t0": 0, "t1": duration,
                    "label": f"{key} {group} {start}-{end}s", "source_start": start, "source_end": end,
                    "source_url": recording.get("source_url", "https://www.youtube.com/watch?v=" + recording["video_id"])})
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
    fieldnames = list(rows[0])
    append = getattr(args, "append_catalog", False) and args.catalog.exists()
    if append:
        with args.catalog.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            if reader.fieldnames != fieldnames:
                raise ValueError("Existing catalog schema differs; refusing to alter it")
            existing = {row["id"]: row for row in reader}
        additions = []
        for row in rows:
            if row["id"] in existing:
                if existing[row["id"]] != {key: str(value) for key, value in row.items()}:
                    raise ValueError(f"Conflicting catalog ID {row['id']}; existing catalog preserved")
            else:
                additions.append(row)
        rows = additions
    with args.catalog.open("a" if append else "w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        if not append:
            writer.writeheader()
        writer.writerows(rows)
    (previews / "reference-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--config", type=Path, default=CONFIG, help="source configuration; default keeps the original four recordings")
    parser.add_argument("--ffmpeg")
    parser.add_argument("--preview-dir", type=Path, default=ROOT / "renders/user-reference-previews")
    parser.add_argument("--catalog", type=Path, default=ROOT / "tools/user_engine_references.csv")
    parser.add_argument("--append-catalog", action="store_true", help="append new IDs while preserving existing catalog entries; identical reruns add nothing")
    parser.add_argument("--filtered-listening", action="store_true", help="also create optional noise-reduced listening copies")
    parser.add_argument("--filtered-only", action="store_true", help="create only filtered listening copies; preserve existing measurement files and catalog")
    parser.add_argument("--filtered-dir", type=Path, default=ROOT / "renders/user-reference-previews/filtered")
    parser.add_argument("--publish-filtered", action="store_true", help="append filtered A/B pairs to the existing RESULTS.html")
    args = parser.parse_args()
    if args.publish_filtered and not (args.filtered_only or args.filtered_listening):
        parser.error("--publish-filtered requires --filtered-listening or --filtered-only")
    prepare(args)
    if args.publish_filtered:
        publish_filtered(args.filtered_dir)


if __name__ == "__main__":
    main()
