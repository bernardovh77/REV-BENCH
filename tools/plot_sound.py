#!/usr/bin/env python3
"""
plot_sound.py - what the engine sounds like (stages 11-12b), from rb_cli sound's WAVs:

    python3 tools/plot_sound.py check WAV...
        for each file: its level (dB SPL and dBA at the listener, full scale
        being 40 Pa), DC, peak, how much of it is above 5 kHz, its loudest
        5 ms there (a click would stand out) and its strongest line above 2 kHz
    python3 tools/plot_sound.py spectrograms WAV...
        a spectrogram of each, the whole band     -> renders/spectrograms.png
    python3 tools/plot_sound.py sweep WAV CSV
        a rev sweep (rb_cli sound --sweep --csv CSV): its spectrogram with the
        firing orders over it, and its level     -> renders/sweep.png
    python3 tools/plot_sound.py orders WAV WAV... --rpm N
        engines held at the same speed: their spectra by engine order
                                                 -> renders/orders.png
    python3 tools/plot_sound.py levels WAV CSV [WAV CSV...]
        wide-open pulls (rb_cli sound --sweep --csv CSV): each one's dBA
        against the speed, while the throttle is open -> renders/levels.png
    python3 tools/plot_sound.py bands WAV@T0-T1[/G]:LABEL... [--panels N,N...]
        third-octave levels (dB SPL at the listener) of each stretch t0..t1 s
        (G: that render's --gain), in panels of N each, titled by --titles
                                                 -> renders/bands.png

-o FILE writes the chart somewhere else; --gain G undoes a render's rb_cli sound
--gain G in the levels. Needs matplotlib and numpy (sudo apt install
python3-matplotlib).
"""
import argparse
import os
import sys
from audio_wav import read_wav as read_audio_wav

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LinearSegmentedColormap

from rbplot import (AXIS, BLUE, DASH, INK, INK_2, MUTED, ORANGE, SURFACE, label, read_csv, setup,
                    style, titles)

FULL_SCALE_PA = 40.0   # Pa at the listener for a sample of 1 (core/src/audio.h)
P_REF = 2e-5           # Pa: 0 dB SPL
FLOOR_DB = -130.0      # dBFS: the bottom of the charts' colour scale
TOP_DB = -10.0

# Quiet in the surface colour, loud in dark blue and finally ink.
LEVELS = LinearSegmentedColormap.from_list(
    "levels", [SURFACE, "#dfe9f7", "#86b6ef", BLUE, "#0d366b", INK])


GAIN = 1.0   # the renders' --gain, undone by read_wav (main sets it)


def read_wav(path):
    """Mono PCM16 or float32 samples, without the render's gain, and rate."""
    try:
        x, rate = read_audio_wav(path)
    except (OSError, ValueError) as error:
        sys.exit(f"plot_sound: cannot read {path}: {error}")
    if x.shape[1] != 1:
        sys.exit(f"plot_sound: {path} isn't mono")
    return x[:, 0] / GAIN, rate


def a_weighting(f):
    """IEC 61672's A-weighting, as a gain (not dB), at frequencies f (Hz)."""
    f2 = np.asarray(f, dtype=float) ** 2
    ra = 12194.0**2 * f2**2 / ((f2 + 20.6**2) * np.sqrt((f2 + 107.7**2) * (f2 + 737.9**2))
                               * (f2 + 12194.0**2))
    return ra * 10 ** (2.0 / 20)


def levels(x, fs):
    """dB SPL and dBA at the listener of a whole signal."""
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / fs)
    power = np.abs(X) ** 2
    rms = np.sqrt(np.sum(power) * 2) / len(x)
    rms_a = np.sqrt(np.sum(power * a_weighting(f) ** 2) * 2) / len(x)
    spl = lambda r: 20 * np.log10(max(r, 1e-30) * FULL_SCALE_PA / P_REF)
    return spl(rms), spl(rms_a)


def highpass(x, fs, cutoff, taps=511):
    """x through a windowed-sinc high-pass (linear, not circular: no wrap-round)."""
    n = np.arange(taps) - (taps - 1) / 2
    lp = 2 * cutoff / fs * np.sinc(2 * cutoff / fs * n) * np.blackman(taps)
    lp /= lp.sum()
    hp = -lp
    hp[(taps - 1) // 2] += 1.0
    return np.convolve(x, hp, mode="same")


def check(args):
    for path in args.files:
        x, fs = read_wav(path)
        spl, dba = levels(x, fs)
        rms = np.sqrt(np.mean(x ** 2))
        hp = highpass(x, fs, 5000.0)
        edge = int(0.02 * fs)                    # the filter's start and end
        n = int(0.005 * fs)
        body = hp[edge:len(hp) - edge]
        windows = np.sqrt(np.mean(body[: len(body) // n * n].reshape(-1, n) ** 2, axis=1))
        worst = int(np.argmax(windows))
        win = np.hanning(len(x))
        X = np.abs(np.fft.rfft(x * win)) * 2 / win.sum()
        f = np.fft.rfftfreq(len(x), 1 / fs)
        above = f > 2000
        line = int(np.argmax(X[above]))
        print(f"{path}: {len(x) / fs:.2f} s")
        print(f"  level {spl:.1f} dB SPL, {dba:.1f} dBA at the listener; rms {20 * np.log10(rms):.1f} dBFS, "
              f"peak {20 * np.log10(np.abs(x).max()):.1f} dBFS")
        print(f"  DC {x.mean():.1e} ({20 * np.log10(abs(x.mean()) / rms + 1e-30):.0f} dB below the rms)")
        print(f"  above 5 kHz: {100 * np.mean(hp ** 2) / np.mean(x ** 2):.4f} % of the power; its loudest 5 ms "
              f"({(edge + worst * n) / fs:.3f} s) {20 * np.log10(windows[worst] / rms):.1f} dB from the rms")
        print(f"  strongest line above 2 kHz: {f[above][line]:.0f} Hz, {20 * np.log10(X[above][line]):.1f} dBFS")


def spectrogram(ax, x, fs, nfft=8192):
    """dBFS against time and (log) frequency, 20 Hz to the Nyquist."""
    hop = nfft // 4
    win = np.hanning(nfft)
    frames = range(0, max(len(x) - nfft, 0) + 1, hop)
    S = np.array([np.abs(np.fft.rfft(x[i:i + nfft] * win)) * 2 / win.sum() for i in frames]).T
    f = np.fft.rfftfreq(nfft, 1 / fs)
    t = (np.array(list(frames)) + nfft / 2) / fs
    db = 20 * np.log10(np.maximum(S, 1e-12))
    keep = f >= 20
    mesh = ax.pcolormesh(t, f[keep], db[keep], cmap=LEVELS, vmin=FLOOR_DB, vmax=TOP_DB, shading="auto",
                         rasterized=True)
    ax.set_yscale("log")
    ax.set_ylim(20, fs / 2)
    ax.set_yticks([20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000])
    ax.set_yticklabels(["20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"])
    return mesh


def spectrograms(args):
    n = len(args.files)
    cols = 2 if n > 1 else 1
    rows = (n + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(11, 1.2 + 3.0 * rows), squeeze=False,
                             facecolor=SURFACE)
    fig.subplots_adjust(left=0.075, right=0.88, top=1 - 1.05 / fig.get_figheight(), bottom=0.07,
                        hspace=0.45, wspace=0.15)
    names = args.titles.split("|") if args.titles else [os.path.basename(p) for p in args.files]
    mesh = None
    for k, path in enumerate(args.files):
        ax = axes[k // cols][k % cols]
        style(ax)
        ax.grid(False)
        x, fs = read_wav(path)
        mesh = spectrogram(ax, x, fs)
        spl, dba = levels(x, fs)
        ax.set_title(f"{names[k]}  ·  {spl:.0f} dB SPL, {dba:.0f} dBA", fontsize=9.5, color=INK, loc="left")
        ax.set_xlabel("time (s)")
        if k % cols == 0:
            ax.set_ylabel("frequency (Hz)")
    for k in range(n, rows * cols):
        axes[k // cols][k % cols].set_visible(False)
    cax = fig.add_axes([0.905, 0.25, 0.012, 0.5])
    bar = fig.colorbar(mesh, cax=cax)
    bar.set_label("dB of full scale", color=INK_2, fontsize=8.5)
    bar.ax.tick_params(colors=AXIS, labelcolor=MUTED, labelsize=8)
    bar.outline.set_edgecolor(AXIS)
    titles(fig, args.title or "What the listener hears, the whole band",
           args.subtitle or "through the anti-alias filter (flat to 20 kHz, 100 dB down from 28 kHz); "
           "16-bit, dithered", top=1 - 0.42 / fig.get_figheight())
    save(fig, args.out or "renders/spectrograms.png")


def sweep(args):
    x, fs = read_wav(args.files[0])
    d = read_csv(args.files[1], ("t_s", "rpm", "throttle"), "plot_sound")
    t, rpm = np.array(d["t_s"]), np.array(d["rpm"])
    cylinders = args.cylinders
    fig, (ax, lv) = plt.subplots(2, 1, figsize=(10, 7.2), sharex=True, facecolor=SURFACE,
                                 gridspec_kw={"height_ratios": [3, 1.2]})
    fig.subplots_adjust(left=0.085, right=0.87, top=0.86, bottom=0.08, hspace=0.12)
    style(ax)
    ax.grid(False)
    nfft = 4096
    hop = nfft // 8
    win = np.hanning(nfft)
    frames = range(0, len(x) - nfft + 1, hop)
    S = np.array([np.abs(np.fft.rfft(x[i:i + nfft] * win)) * 2 / win.sum() for i in frames]).T
    f = np.fft.rfftfreq(nfft, 1 / fs)
    ts = (np.array(list(frames)) + nfft / 2) / fs
    keep = f <= args.top
    mesh = ax.pcolormesh(ts, f[keep], 20 * np.log10(np.maximum(S[keep], 1e-12)), cmap=LEVELS,
                         vmin=-100, vmax=TOP_DB, shading="auto", rasterized=True)
    firing = cylinders / 2.0   # firings per revolution: the firing order
    for k, (order, name) in enumerate(((firing, "the firing"), (2 * firing, "twice it"), (3 * firing, ""))):
        ax.plot(t, order * rpm / 60.0, color=ORANGE, linewidth=0.9, linestyle=DASH, alpha=0.9)
        i = np.searchsorted(t, 7.0)
        if order * rpm[i] / 60.0 < args.top:
            ax.annotate(f"order {order:g}" + (f": {name}" if name else ""), (t[i], order * rpm[i] / 60.0),
                        xytext=(6, -12), textcoords="offset points", fontsize=8, color=ORANGE,
                        bbox=dict(boxstyle="round,pad=0.25", fc=SURFACE, ec="none", alpha=0.9))
    ax.set_ylim(0, args.top)
    ax.set_ylabel("frequency (Hz)")
    cax = fig.add_axes([0.89, 0.42, 0.012, 0.44])
    bar = fig.colorbar(mesh, cax=cax)
    bar.set_label("dB of full scale", color=INK_2, fontsize=8.5)
    bar.ax.tick_params(colors=AXIS, labelcolor=MUTED, labelsize=8)
    bar.outline.set_edgecolor(AXIS)

    style(lv)
    n = int(0.1 * fs)
    seg = x[: len(x) // n * n].reshape(-1, n)
    tl = (np.arange(len(seg)) + 0.5) * n / fs
    spl = [levels(s, fs) for s in seg]
    lv.plot(tl, [s[0] for s in spl], color=BLUE, linewidth=1.3)
    lv.plot(tl, [s[1] for s in spl], color=ORANGE, linewidth=1.3)
    label(lv, "dB SPL", (tl[-1], spl[-1][0]), (5, 5), color=BLUE)
    label(lv, "dBA", (tl[-1], spl[-1][1]), (5, -5), color=ORANGE)
    lv.set_ylabel("level at the listener")
    lv.set_xlabel("time (s)")
    sp = lv.twinx()
    sp.plot(t, rpm, color=MUTED, linewidth=1.0, linestyle=DASH)
    sp.set_ylabel("rpm", color=MUTED)
    sp.tick_params(colors=AXIS, labelcolor=MUTED, labelsize=8.5)
    for side in ("top", "left", "bottom"):
        sp.spines[side].set_visible(False)
    sp.spines["right"].set_color(AXIS)
    titles(fig, args.title or "A wide-open pull, as the listener hears it",
           f"{args.subtitle or 'the spectrogram with the engine orders over it (dashed); below, the level and speed'}")
    save(fig, args.out or "renders/sweep.png")


def orders(args):
    if not args.rpm:
        sys.exit("plot_sound orders: --rpm is the speed both were held at")
    base = float(args.rpm) / 60.0   # order 1, Hz
    names = args.titles.split("|") if args.titles else [os.path.basename(p) for p in args.files]
    fig, axes = plt.subplots(len(args.files), 1, figsize=(10, 1.4 + 2.4 * len(args.files)), sharex=True,
                             facecolor=SURFACE)
    fig.subplots_adjust(left=0.085, right=0.97, top=1 - 1.1 / fig.get_figheight(), bottom=0.09, hspace=0.3)
    top = args.max_order
    for k, path in enumerate(args.files):
        ax = axes[k]
        style(ax)
        x, fs = read_wav(path)
        cycles = int(len(x) / fs * base / 2)       # whole cycles (two revolutions each)
        seg = x[: int(round(cycles * 2 / base * fs))]
        X = np.abs(np.fft.rfft(seg)) * 2 / len(seg)
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        db = 20 * np.log10(np.maximum(X, 1e-12) * FULL_SCALE_PA / P_REF / np.sqrt(2))
        keep = f / base <= top + 0.25
        ax.plot(f[keep] / base, db[keep], color=BLUE if k == 0 else ORANGE, linewidth=0.9)
        ax.set_xlim(0, top + 0.25)
        ax.set_ylim(0, 130 if db[keep].max() > 110 else 120)   # room for the loudest line's label
        ax.set_xticks(np.arange(0, top + 0.01, 1.0))
        ax.set_xticks(np.arange(0.5, top + 0.01, 1.0), minor=True)
        ax.set_ylabel("dB SPL")
        ax.set_title(names[k], fontsize=9.5, color=INK, loc="left")
        for o in np.arange(0.5, top + 0.01, 0.5):
            i = int(np.argmin(abs(f - o * base)))
            if db[i] > 60:
                label(ax, f"{o:g}", (o, db[i]), (0, 6), ha="center", va="bottom",
                      color=BLUE if k == 0 else ORANGE, size=7.5)
    axes[-1].set_xlabel("engine order (multiples of the crank's speed)")
    titles(fig, args.title or f"Engines held at {float(args.rpm):.0f} rpm, by engine order",
           args.subtitle or "each line's level at the listener")
    save(fig, args.out or "renders/orders.png")


def levels_chart(args):
    if len(args.files) % 2:
        sys.exit("plot_sound levels: give each pull's WAV and its CSV")
    names = args.titles.split("|") if args.titles else [os.path.basename(p) for p in args.files[::2]]
    fig, ax = plt.subplots(figsize=(10, 4.6), facecolor=SURFACE)
    fig.subplots_adjust(left=0.085, right=0.83, top=0.8, bottom=0.12)
    style(ax)
    colours = (BLUE, ORANGE, INK_2, MUTED)
    for k in range(len(args.files) // 2):
        x, fs = read_wav(args.files[2 * k])
        d = read_csv(args.files[2 * k + 1], ("t_s", "rpm", "throttle"), "plot_sound")
        t, rpm, throttle = np.array(d["t_s"]), np.array(d["rpm"]), np.array(d["throttle"])
        n = int(0.1 * fs)
        seg = x[: len(x) // n * n].reshape(-1, n)
        tl = (np.arange(len(seg)) + 0.5) * n / fs
        dba = np.array([levels(s, fs)[1] for s in seg])
        speed = np.interp(tl, t, rpm)
        pulling = (np.interp(tl, t, throttle) > 0.5) & (np.interp(tl, t, rpm) < np.max(rpm) - 150)
        colour = colours[k % len(colours)]
        ax.plot(speed[pulling], dba[pulling], color=colour, linewidth=1.3)
        label(ax, names[k], (speed[pulling][-1], dba[pulling][-1]), (6, 0), color=colour, va="center")
    ax.set_xlabel("rpm")
    ax.set_ylabel("dBA at the listener")
    titles(fig, args.title or "Wide-open pulls: how loud, against the speed",
           args.subtitle or "the level in 0.1 s windows, while the throttle is open")
    save(fig, args.out or "renders/levels.png")


def third_octaves(x, fs):
    """Third-octave band centres (25 Hz to 16 kHz) and each band's level, dB SPL."""
    win = np.hanning(len(x))
    power = np.abs(np.fft.rfft(x * win)) ** 2
    f = np.fft.rfftfreq(len(x), 1 / fs)
    power *= np.mean(x ** 2) / max(power.sum(), 1e-30)   # Parseval: the bands sum to the mean square
    centres = 1000.0 * 2.0 ** (np.arange(-16, 13) / 3.0)
    p2 = [power[(f >= c * 2 ** (-1 / 6)) & (f < c * 2 ** (1 / 6))].sum() for c in centres]
    return centres, 10 * np.log10(np.maximum(p2, 1e-30) * FULL_SCALE_PA ** 2 / P_REF ** 2)


def bands(args):
    stretches = []
    for spec in args.files:
        try:
            path, rest = spec.split("@", 1)
            span, name = rest.split(":", 1)
            span, gain = span.split("/") if "/" in span else (span, "1")
            t0, t1 = (float(v) for v in span.split("-"))
        except ValueError:
            sys.exit(f"plot_sound bands: {spec} isn't WAV@T0-T1[/GAIN]:LABEL")
        x, fs = read_wav(path)
        x = x[int(t0 * fs):int(t1 * fs)] / float(gain)
        stretches.append((name, third_octaves(x - x.mean(), fs)))
    counts = [int(c) for c in args.panels.split(",")] if args.panels else [len(stretches)]
    if sum(counts) != len(stretches):
        sys.exit("plot_sound bands: --panels must add up to the number of stretches")
    names = args.titles.split("|") if args.titles else [""] * len(counts)
    cols = 2 if len(counts) > 1 else 1
    rows = (len(counts) + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(10, 1.3 + 3.3 * rows), squeeze=False, facecolor=SURFACE)
    fig.subplots_adjust(left=0.075, right=0.97, top=1 - 1.15 / fig.get_figheight(), bottom=0.07,
                        hspace=0.5, wspace=0.18)
    colours = (BLUE, ORANGE, INK_2, MUTED)
    first = 0
    for k, count in enumerate(counts):
        ax = axes[k // cols][k % cols]
        style(ax)
        for j, (name, (centres, db)) in enumerate(stretches[first:first + count]):
            ax.plot(centres, db, color=colours[j % len(colours)], linewidth=1.4, label=name)
        first += count
        ax.set_xscale("log")
        ax.set_xlim(22, 18000)
        ax.set_xticks([31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000])
        ax.set_xticklabels(["31.5", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"])
        ax.minorticks_off()
        ax.set_ylim(0, 110)
        ax.set_title(names[k] if k < len(names) else "", fontsize=9.5, color=INK, loc="left")
        ax.legend(frameon=False, fontsize=8.5, labelcolor=INK_2, loc="lower left")
        if k % cols == 0:
            ax.set_ylabel("dB SPL at the listener, third octave")
        if k // cols == rows - 1:
            ax.set_xlabel("third-octave band (Hz)")
    for k in range(len(counts), rows * cols):
        axes[k // cols][k % cols].set_visible(False)
    titles(fig, args.title or "Where the sound's energy is",
           args.subtitle or "third-octave levels at the listener", top=1 - 0.42 / fig.get_figheight())
    save(fig, args.out or "renders/bands.png")


def save(fig, out):
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    fig.savefig(out, dpi=150, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("chart", choices=("check", "spectrograms", "sweep", "orders", "levels", "bands"))
    parser.add_argument("files", nargs="+")
    parser.add_argument("-o", "--out")
    parser.add_argument("--rpm", help="orders: the speed both engines were held at")
    parser.add_argument("--titles", help="panel titles, separated by | (default: the file names)")
    parser.add_argument("--panels", help="bands: how many stretches go in each panel, as N,N,...")
    parser.add_argument("--title", help="the chart's title")
    parser.add_argument("--subtitle", help="the chart's subtitle")
    parser.add_argument("--cylinders", type=int, default=4, help="sweep: the engine's cylinders (default 4)")
    parser.add_argument("--top", type=float, default=1000.0, help="sweep: the highest frequency shown (Hz)")
    parser.add_argument("--max-order", type=float, default=12.0, help="orders: the highest order shown")
    parser.add_argument("--gain", type=float, default=1.0, help="the renders' rb_cli sound --gain (default 1)")
    args = parser.parse_args()
    global GAIN
    GAIN = args.gain
    if args.chart == "sweep" and len(args.files) != 2:
        parser.error("sweep needs the WAV and its CSV")
    if args.chart == "orders" and len(args.files) < 2:
        parser.error("orders needs two WAVs or more")
    setup()
    {"check": check, "spectrograms": spectrograms, "sweep": sweep, "orders": orders,
     "levels": levels_chart, "bands": bands}[args.chart](args)


if __name__ == "__main__":
    main()
