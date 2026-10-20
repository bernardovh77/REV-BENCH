#!/usr/bin/env python3
"""
compare_real.py - REV-BENCH's sound against recordings of real engines.

    python3 tools/compare_real.py fetch
        downloads the recordings listed in tools/real_engines.csv (Freesound
        previews, 128 kbit/s MP3) into renders/real/ and turns each into a mono
        48 kHz WAV (needs ffmpeg: sudo apt install ffmpeg)
    python3 tools/compare_real.py compare [--kind idle|steady|rev] [WAV@T0-T1:LABEL ...]
        measures the listed recordings of that kind (idling, the default; held
        at a steady speed off idle, like rb_cli sound --hold; or revving; each
        one's stretch t0..t1 seconds) and each given render the same way,
        prints the numbers and draws them  -> renders/real-KIND.png

The numbers, all independent of the microphone's level:
  >1k, >2k, >4k  the share of the energy above 1, 2 and 4 kHz, in dB
  slope          how fast the third-octave levels fall from 0.5 to 8 kHz (dB/octave)
  A loss         how much A-weighting takes off the level (dB): large when the
                 sound is mostly low thumps
  crest          the peak over the RMS (dB): sharp firings and noise raise it
  repeat lo/mid/hi
                 how much of each engine cycle repeats in the next, below 500 Hz,
                 0.5-2 kHz and 2-8 kHz: the best normalised correlation r between
                 0.5 s of sound and the same a cycle later (the lag searched over
                 75-267 ms, an idle's 720 degrees at 450-1600 rpm; the bands above
                 500 Hz at the low band's lag), as 10 log10(r / (1 - r)). 0 dB:
                 as much changes as repeats. A sound that repeats exactly reads
                 +40 dB or more. Idling (and steady: the lag searched over
                 17-80 ms, 1500-7000 rpm) only: revving changes the cycle's
                 length.

-o FILE writes the chart somewhere else. Needs numpy and matplotlib.
"""
import argparse
import csv
import os
import shutil
import subprocess
import sys
import urllib.request
import wave

import matplotlib.pyplot as plt
import numpy as np

from rbplot import BLUE, INK_2, MUTED, ORANGE, SURFACE, label, setup, style, titles

HERE = os.path.dirname(os.path.abspath(__file__))
LIST = os.path.join(HERE, "real_engines.csv")
REAL_DIR = os.path.join(HERE, "..", "renders", "real")
BANDS = [(20.0, 500.0), (500.0, 2000.0), (2000.0, 8000.0)]
BAND_NAMES = ["below 500 Hz", "0.5–2 kHz", "2–8 kHz"]


def read_list():
    with open(LIST, newline="") as f:
        return list(csv.DictReader(f))


def fetch(_args):
    if not shutil.which("ffmpeg"):
        sys.exit("compare_real: needs ffmpeg to decode the MP3s (sudo apt install ffmpeg)")
    os.makedirs(REAL_DIR, exist_ok=True)
    rows = {row["id"]: row for row in read_list()}   # a recording can be listed twice
    for row in rows.values():
        mp3 = os.path.join(REAL_DIR, row["id"] + ".mp3")
        wav = os.path.join(REAL_DIR, row["id"] + ".wav")
        if not os.path.exists(mp3):
            print(f"downloading {row['id']} ({row['engine']}, by {row['author']}, {row['licence']})")
            req = urllib.request.Request(row["preview"], headers={"User-Agent": "REV-BENCH compare_real.py"})
            with urllib.request.urlopen(req, timeout=60) as r, open(mp3, "wb") as out:
                out.write(r.read())
        if not os.path.exists(wav):
            subprocess.run(["ffmpeg", "-nostdin", "-loglevel", "error", "-y", "-i", mp3,
                            "-ac", "1", "-ar", "48000", "-c:a", "pcm_s16le", wav], check=True)
    print(f"the recordings are in {os.path.normpath(REAL_DIR)}")


def read_wav(path):
    """Samples (floats, full scale 1) and the rate of a 16-bit WAV, mixed to mono."""
    try:
        w = wave.open(path)
    except (OSError, wave.Error) as e:
        sys.exit(f"compare_real: cannot read {path}: {e}")
    if w.getsampwidth() != 2:
        sys.exit(f"compare_real: {path} isn't 16-bit PCM")
    x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float) / 32768.0
    return x.reshape(-1, w.getnchannels()).mean(axis=1), w.getframerate()


def band(x, fs, lo, hi):
    """x with everything outside lo..hi Hz taken out (a brick wall in the FFT)."""
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1.0 / fs)
    spec[(f < lo) | (f > hi)] = 0.0
    return np.fft.irfft(spec, len(x))


def a_weighting(f):
    """IEC 61672 A-weighting as a gain (1 at 1 kHz)."""
    f2 = np.maximum(f, 1e-9) ** 2
    ra = (12194.0**2 * f2**2) / ((f2 + 20.6**2) * np.sqrt((f2 + 107.7**2) * (f2 + 737.9**2))
                                * (f2 + 12194.0**2))
    return ra * 10.0 ** (2.0 / 20.0)


def repeat(x, fs, lag_lo, lag_hi, frame=0.5):
    """The best normalised correlation between each 0.5 s of x and x a lag later,
    the lag in lag_lo..lag_hi seconds; averaged over frames (half overlapping).
    Returns (mean r, median best lag in seconds)."""
    n, a, b = int(frame * fs), int(lag_lo * fs), int(lag_hi * fs)
    size = 1 << int(np.ceil(np.log2(2 * n + b)))
    energy = np.concatenate(([0.0], np.cumsum(x * x)))   # energy[j] = sum of x[:j]^2
    rs, lags = [], []
    for s in range(0, len(x) - n - b, n // 2):
        u, v = x[s:s + n], x[s:s + n + b]
        corr = np.fft.irfft(np.conj(np.fft.rfft(u, size)) * np.fft.rfft(v, size), size)[:b + 1]
        shift = np.arange(b + 1)
        ev = energy[s + shift + n] - energy[s + shift]
        r = corr / np.sqrt((energy[s + n] - energy[s]) * ev + 1e-30)
        k = a + int(np.argmax(r[a:b + 1]))
        rs.append(r[k])
        lags.append(k)
    if not rs:
        sys.exit("compare_real: a stretch must be longer than 0.8 s")
    return float(np.mean(rs)), float(np.median(lags)) / fs


def as_db(r):
    r = min(max(r, 1e-6), 1.0 - 1e-9)
    return 10.0 * np.log10(r / (1.0 - r))


def thirds(x, fs):
    """Third-octave band centres (25 Hz to 16 kHz) and each band's share of the energy."""
    power = np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    f = np.fft.rfftfreq(len(x), 1.0 / fs)
    centres = 1000.0 * 2.0 ** (np.arange(-16, 13) / 3.0)
    total = power[(f > 20.0) & (f < 18000.0)].sum()
    share = [power[(f >= c * 2 ** (-1 / 6)) & (f < c * 2 ** (1 / 6))].sum() / total for c in centres]
    return centres, np.array(share)


def measure(path, t0, t1, repeats=True, rpm=(450.0, 1600.0)):
    x, fs = read_wav(path)
    x = x[int(t0 * fs):int(t1 * fs)]
    x = x - x.mean()
    power = np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    f = np.fft.rfftfreq(len(x), 1.0 / fs)
    audible = (f > 20.0) & (f < 18000.0)

    def above(lo):
        return 10.0 * np.log10(power[audible & (f > lo)].sum() / power[audible].sum() + 1e-30)

    centres, share = thirds(x, fs)
    mid = (centres >= 500.0) & (centres <= 8000.0)
    slope = np.polyfit(np.log2(centres[mid]), 10.0 * np.log10(share[mid] + 1e-30), 1)[0]
    weighted = np.fft.irfft(np.fft.rfft(x) * a_weighting(f), len(x))
    a_loss = 10.0 * np.log10(np.mean(x * x) / np.mean(weighted * weighted))
    crest = 20.0 * np.log10(np.max(np.abs(x)) / np.sqrt(np.mean(x * x)))
    reps = []
    if repeats:
        r_lo, cycle = repeat(band(x, fs, *BANDS[0]), fs, 120.0 / rpm[1], 120.0 / rpm[0])
        reps = [as_db(r_lo)] + [as_db(repeat(band(x, fs, *b), fs, 0.97 * cycle, 1.03 * cycle)[0])
                                for b in BANDS[1:]]
    return {"above": [above(1000.0), above(2000.0), above(4000.0)], "slope": slope,
            "a_loss": a_loss, "crest": crest, "repeat": reps, "centres": centres, "share": share}


def compare(args):
    idle = args.kind != "rev"   # the repeats: idling or steady
    rpm = (1500.0, 7000.0) if args.kind == "steady" else (450.0, 1600.0)
    real = []
    for row in read_list():
        wav = os.path.join(REAL_DIR, row["id"] + ".wav")
        if row["kind"] == args.kind and os.path.exists(wav):
            real.append((row["label"], measure(wav, float(row["t0"]), float(row["t1"]), idle, rpm)))
    if not real:
        print("compare_real: no recordings in renders/real (run: python3 tools/compare_real.py fetch)")
    ours = []
    for spec in args.files:
        try:
            path, rest = spec.split("@", 1)
            span, name = rest.split(":", 1)
            t0, t1 = (float(v) for v in span.split("-"))
        except ValueError:
            sys.exit(f"compare_real: {spec} isn't WAV@T0-T1:LABEL")
        ours.append((name, measure(path, t0, t1, idle, rpm)))
    print(f"{'':34s} {'>1k':>6s} {'>2k':>6s} {'>4k':>6s} {'slope':>6s} {'A loss':>6s} {'crest':>6s}"
          + (f"   repeat {'lo':>5s} {'mid':>5s} {'hi':>5s}" if idle else ""))
    for name, m in real + ours:
        print(f"{name[:34]:34s} " + " ".join(f"{v:6.1f}" for v in m["above"])
              + f" {m['slope']:6.1f} {m['a_loss']:6.1f} {m['crest']:6.1f}"
              + ("          " + " ".join(f"{v:5.1f}" for v in m["repeat"]) if idle else ""))
    chart(real, ours, idle, args.out or os.path.join(HERE, "..", "renders", f"real-{args.kind}.png"),
          "idling" if args.kind == "idle" else "held at a steady speed")


def chart(real, ours, idle, out, doing="idling"):
    setup()
    if idle:
        fig, (sp, rp) = plt.subplots(2, 1, figsize=(10, 8.6), facecolor=SURFACE,
                                     gridspec_kw={"height_ratios": [1.25, 1.0], "hspace": 0.42})
        fig.subplots_adjust(left=0.085, right=0.83, top=0.86, bottom=0.07)
        titles(fig, f"Real engines and REV-BENCH, {doing}",
               f"{len(real)} recordings of real engines {doing} (orange) and {len(ours)} of REV-BENCH's "
               "(blue): where their energy is,\nand how much of each engine cycle repeats in the next")
    else:
        fig, sp = plt.subplots(figsize=(10, 5.4), facecolor=SURFACE)
        fig.subplots_adjust(left=0.085, right=0.83, top=0.80, bottom=0.11)
        titles(fig, "Real engines and REV-BENCH, revving",
               f"{len(real)} stretches of real engines revving (orange) and {len(ours)} of REV-BENCH's "
               "pulls (blue): where their energy is")

    # Where the energy is: each sound's third-octave levels, relative to its total.
    style(sp)
    for group, colour, side in ((real, ORANGE, "real engines"), (ours, BLUE, "REV-BENCH")):
        if not group:
            continue
        levels = [10.0 * np.log10(m["share"] + 1e-30) for _, m in group]
        centres = group[0][1]["centres"]
        for lv in levels:
            sp.semilogx(centres, lv, color=colour, linewidth=0.8, alpha=0.35)
        middle = np.median(levels, axis=0)
        sp.semilogx(centres, middle, color=colour, linewidth=2.2)
        sp.annotate(f"{side},\nmedian of {len(group)}", (centres[-1], middle[-1]), xytext=(8, 0),
                    textcoords="offset points", ha="left", va="center", fontsize=8.5, color=colour,
                    annotation_clip=False)
    sp.set_xlim(25, 16000)
    sp.set_ylim(-80, 0)
    sp.set_xticks([31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000])
    sp.set_xticklabels(["31.5", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"])
    sp.minorticks_off()
    sp.set_xlabel("third-octave band (Hz)")
    sp.set_ylabel("share of the energy (dB)")
    if not idle:
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        fig.savefig(out, dpi=150, facecolor=SURFACE)
        print(f"wrote {out}")
        return

    # How much repeats: one dot per sound and band.
    style(rp)
    rng = np.random.default_rng(1)
    for group, colour, dx in ((real, ORANGE, -0.13), (ours, BLUE, 0.13)):
        for _, m in group:
            for k, v in enumerate(m["repeat"]):
                rp.plot(k + dx + rng.uniform(-0.05, 0.05), v, "o", color=colour, markersize=5,
                        alpha=0.75, markeredgewidth=0)
    rp.axhline(0.0, color=MUTED, linewidth=1.0)
    label(rp, "as much changes as repeats", (1.5, 0.0), (0, 5), ha="center", va="bottom", color=MUTED)
    rp.set_xticks(range(len(BANDS)))
    rp.set_xticklabels(BAND_NAMES)
    rp.set_xlim(-0.5, len(BANDS) - 0.5)
    rp.set_ylabel("cycle repeats (dB, 10 log r/(1−r))")
    rp.grid(axis="x", visible=False)
    for k, (name, colour) in enumerate((("real engines", ORANGE), ("REV-BENCH", BLUE))):
        rp.text(1.02, 0.95 - 0.08 * k, name, transform=rp.transAxes, color=colour, fontsize=9)
    rp.text(1.02, 0.70, "each dot: one\nsound in one band", transform=rp.transAxes, color=INK_2,
            fontsize=8.5, va="top")
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    fig.savefig(out, dpi=150, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("fetch").set_defaults(run=fetch)
    c = sub.add_parser("compare")
    c.add_argument("files", nargs="*")
    c.add_argument("--kind", choices=("idle", "steady", "rev"), default="idle")
    c.add_argument("-o", "--out")
    c.set_defaults(run=compare)
    args = p.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
