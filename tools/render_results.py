#!/usr/bin/env python3
"""Publish one portable RESULTS.html; keep only the latest checkpoint's raw audio.

make render stages the standard examples and five extended coasts before replacing
the current result. --publish-only repackages current recordings without rerendering.
Only generated renders/current/ is replaced; reference folders are never scanned.
Uses Python's standard library, including for float-WAV listening conversion.
"""
import argparse
from array import array
import base64
import csv
from datetime import datetime, timezone
import hashlib
from html import escape
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile

from audio_wav import read_wav_bytes
from checkpoint_sections import adopt_current, carry_diagnostics, compose, diagnostic_names

ROOT = Path(__file__).resolve().parents[1]
ENGINES = {
    "i4": ("Inline four", "Toyota 1AZ-FE · BRExhaust inspiration"),
    "i6": ("Inline six", "8,500 rpm I6 · restored Supersprint-inspired twin exhaust"),
    "v8-cross": ("Cross-plane V8", "7,500 rpm Mustang-inspired V8 · 3-inch X + rear muffler delete"),
    "v8-flat": ("Flat-plane V8", "9,000 rpm Ferrari 458 inspiration · restored Tubi-style exhaust"),
    "boxer4": ("Boxer four", "Subaru FB20 · HKS inspiration"),
}
STANDARD = {
    "i4": ("idle", "idle-front", "idle-cabin", "sweep", "sweep-cabin", "rev",
           "3000-none", "3000-straight", "3000-chambered"),
    "i6": ("rev",),
    "v8-cross": ("3000-behind", "3000-tailpipe", "3000-side", "3000-front",
                 "3000-cabin", "sweep", "rev"),
    "v8-flat": ("3000", "rev"),
    "boxer4": ("rev", "3000", "3000-equal"),
}
LABELS = {
    "idle": "Start and idle · behind the car", "idle-front": "Start and idle · bonnet",
    "idle-cabin": "Start and idle · cabin", "sweep": "Pull and return to idle",
    "sweep-cabin": "Pull and return to idle · cabin", "rev": "Neutral rev and deceleration",
    "3000": "Held at 3000 rpm", "3000-after": "Researched exhaust · held at 3000 rpm",
    "3000-none": "3000 rpm · rear muffler removed", "3000-straight": "3000 rpm · packed core",
    "3000-chambered": "3000 rpm · chambered muffler", "3000-equal": "3000 rpm · equal headers",
    "3000-behind": "3000 rpm · behind the car", "3000-tailpipe": "3000 rpm · tailpipe",
    "3000-side": "3000 rpm · beside the car", "3000-front": "3000 rpm · bonnet",
    "3000-cabin": "3000 rpm · cabin", "long-coast": "Full pull + long loaded coast",
    "packed-control": "Long coast · packed rear mufflers (comparison)",
}
REV_LIMITS = {"i6": 8500, "v8-cross": 7500, "v8-flat": 9000}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def expected_clips():
    clips = []
    for engine in ENGINES:
        for scenario in ("long-coast", "3000-after"):
            clips.append((engine, scenario, f"feedback/{engine}-{scenario}.wav"))
        if engine == "v8-cross":
            clips.append((engine, "packed-control", "feedback/v8-cross-packed-control.wav"))
        clips.extend((engine, scenario, f"standard/{engine}-{scenario}.wav")
                     for scenario in STANDARD[engine])
    clips.append(("test", "test", "standard/test.wav"))
    return clips


def listening_wav(path):
    """A constant gain across the whole clip; the measurement WAV is untouched."""
    raw, tag, channels, rate = read_wav_bytes(path)
    if tag != 3:
        raise ValueError(f"{path}: checkpoint originals must be unfiltered float32 WAVs")
    samples = array("f")
    samples.frombytes(raw)
    if sys.byteorder != "little":
        samples.byteswap()
    if not all(math.isfinite(x) for x in samples):
        raise ValueError(f"{path}: non-finite audio")
    peak = max(abs(x) for x in samples)
    if peak >= 1:
        raise ValueError(f"{path}: inspect clipping before publishing")
    gain = 0.6 / peak if peak else 1.0
    pcm = array("h", (round(x * gain * 32767) for x in samples))
    if sys.byteorder != "little":
        pcm.byteswap()
    payload = pcm.tobytes()
    fmt = struct.pack("<HHIIHH", 1, channels, rate, rate*channels*2, channels*2, 16)
    wav = (b"RIFF" + struct.pack("<I", 36+len(payload)) + b"WAVEfmt "
           + struct.pack("<I", 16) + fmt + b"data" + struct.pack("<I", len(payload)) + payload)
    return wav, {"source_sha256": sha(path), "rate": rate, "channels": channels,
                 "seconds": len(samples)/channels/rate, "static_preview_gain": gain}


def coast_markers(path, seconds, engine):
    with path.with_suffix(".csv").open() as stream:
        rows = list(csv.DictReader(stream))
    lift = next(float(r["t_s"]) for r in rows if float(r["throttle"]) == 0)
    clutch = next(float(r["t_s"]) for r in rows if float(r["crank_inertia_kgm2"]) < 2.9)
    if not (0 < lift < clutch < seconds and 7.98 < clutch-lift < 8.02):
        raise ValueError(f"{path}: missing the eight-second loaded coast")
    markers = {"lift_s": lift, "clutch_s": clutch}
    if engine in REV_LIMITS:
        limit = REV_LIMITS[engine]
        if max(float(r["rpm"]) for r in rows if float(r["t_s"]) <= lift) < limit-150:
            raise ValueError(f"{path}: pull did not reach the requested {limit} rpm limiter region")
        markers.update(top_band_start_s=next(float(r["t_s"]) for r in rows
                       if float(r["rpm"]) >= limit-1000 and float(r["throttle"]) > .9),
                       top_band_rpm=[limit-1000, limit])
    return markers


def write_page(current, destination, renders=None):
    """Validate the complete inventory before atomically replacing the single file."""
    renders = current.parent if renders is None else renders
    expected = {name for _, _, name in expected_clips()} | diagnostic_names(current, renders)
    actual = {str(p.relative_to(current)) for p in current.rglob("*.wav")}
    if actual != expected:
        raise ValueError(f"Incomplete checkpoint: missing {sorted(expected-actual)}; "
                         f"unexpected {sorted(actual-expected)}")
    if any(p.is_symlink() for p in current.rglob("*")):
        raise ValueError("Symlinks are not allowed inside a generated checkpoint")
    template = Path(__file__).with_name("results_template.html").read_text()
    inventory = []
    sections = {engine: [] for engine in (*ENGINES, "test")}
    for engine, scenario, name in expected_clips():
        path = current / name
        preview, info = listening_wav(path)
        info.update(source=name, engine=engine, scenario=scenario,
                    preview_sha256=hashlib.sha256(preview).hexdigest())
        markers = ""
        if scenario in ("long-coast", "packed-control"):
            info.update(coast_markers(path, info["seconds"], engine))
            markers = (f"Lift at {info['lift_s']:.1f}s · clutch in at {info['clutch_s']:.1f}s"
                       " · 8 seconds of loaded deceleration")
        title = LABELS.get(scenario, "Test tone")
        data = base64.b64encode(preview).decode("ascii")
        detail = markers or f"{info['seconds']:.1f} seconds"
        jump = ""
        player_id = f"{engine}-{scenario}"
        if "top_band_start_s" in info:
            low, high = info["top_band_rpm"]
            jump = (f'<button type="button" data-player="{player_id}" data-seek="{info["top_band_start_s"]}">'
                    f'Hear {low:,}–{high:,} rpm</button>')
        card = (f'<article><h3>{escape(title)}</h3><p>{detail}</p>{jump}'
                f'<audio id="{player_id}" controls preload="none" aria-label="{escape(title, quote=True)}" '
                f'data-source="{escape(name, quote=True)}" src="data:audio/wav;base64,{data}"></audio></article>')
        sections[engine].append(card)
        inventory.append(info)
    body = []
    for engine, (title, subtitle) in ENGINES.items():
        cards = sections[engine]
        body.append(f'<section id="{engine}"><h2>{title}</h2><p class="system">{subtitle}</p>'
                    + cards[0] + f'<details><summary>Other examples ({len(cards)-1})</summary>'
                    + "".join(cards[1:]) + "</details></section>")
    body.append('<details class="diagnostics"><summary>Audio test</summary>'
                + "".join(sections["test"]) + "</details>")
    generated = datetime.now(timezone.utc).isoformat(timespec="seconds")
    manifest = {"format_version": 1, "generated_utc": generated, "clips": inventory,
                "processing": "Listening only: constant whole-clip gain to peak 0.6, PCM16. "
                              "No EQ, denoising, compression, time stretch or pitch changes. "
                              "Measurements use the byte-identical unfiltered float originals."}
    page = template.replace("<!-- CONTENT -->", "".join(body)).replace("<!-- GENERATED -->", generated).replace(
        "<!-- MANIFEST -->", json.dumps(manifest, ensure_ascii=True).replace("<", "\\u003c"))
    page = compose(page, renders)
    temporary = destination.with_suffix(".html.tmp")
    try:
        temporary.write_text(page, encoding="utf-8")
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    (current / "results.json").write_text(json.dumps(manifest, indent=2)+"\n")
    return manifest


def replace_current(staged, renders):
    """A failed render/page leaves the last complete result available."""
    current = renders / "current"
    if current.exists() and not (current / "results.json").is_file():
        raise ValueError("Refusing to replace an unrecognized current folder")
    if current.is_symlink() or (renders / "RESULTS.html").is_symlink():
        raise ValueError("Refusing to replace a symlink")
    page = staged / "RESULTS.html"
    carry_diagnostics(current, staged, renders)
    write_page(staged, page, renders)
    backup = staged.parent / "previous"
    if current.exists():
        current.rename(backup)
    try:
        staged.rename(current)
        (current / "RESULTS.html").replace(renders / "RESULTS.html")
    except BaseException:
        if current.exists():
            current.rename(staged)
        if backup.exists():
            backup.rename(current)
        raise
    if backup.exists():
        shutil.rmtree(backup)


def render_checkpoint(cli, build, renders):
    with tempfile.TemporaryDirectory(prefix=".pending-results-", dir=renders) as workspace:
        staged = Path(workspace) / "next"
        staged.mkdir()
        print("Recording standard examples…", flush=True)
        run = subprocess.run(["make", "render-standard", f"BUILD={build}",
                              f"RENDER_DIR={staged/'standard'}", "RENDER_FLAGS=--float"],
                             cwd=ROOT, capture_output=True, text=True)
        log = run.stdout + run.stderr
        if run.returncode:
            print(log, file=sys.stderr)
        run.check_returncode()
        (staged/"standard/render.log").write_text(log)
        diagnostics = re.findall(r"soft-limited samples (\d+), non-finite (\d+)", log)
        if len(diagnostics) != 22 or any(a != "0" or b != "0" for a, b in diagnostics):
            raise ValueError("Standard recordings: inspect limiter/non-finite diagnostics before publishing")
        print("Recording five long coasts and held examples…", flush=True)
        subprocess.run([sys.executable, str(ROOT/"tools/exhaust_feedback.py"),
                        "--cli", str(cli.resolve()), "--output", str(staged/"feedback")],
                       cwd=ROOT, check=True)
        replace_current(staged, renders)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", type=Path, default=ROOT/"build/rb_cli")
    parser.add_argument("--build", default="build")
    parser.add_argument("--publish-only", action="store_true")
    parser.add_argument("--adopt-current", action="store_true", help="register existing research/reference sections and diagnostic assets once")
    args = parser.parse_args()
    renders = ROOT / "renders"
    renders.mkdir(exist_ok=True)
    if args.adopt_current:
        adopt_current(renders)
    if args.publish_only:
        write_page(renders/"current", renders/"RESULTS.html")
    else:
        render_checkpoint(args.cli, args.build, renders)
    print(f"Result: {renders/'RESULTS.html'}", flush=True)


if __name__ == "__main__":
    main()
