#!/usr/bin/env python3
"""
plot_presets.py - charts of the presets (stage 8):

    python3 tools/plot_presets.py firing [renders/presets.csv]
        when each cylinder of each preset fires, from `rb_cli presets --csv`:
        every cylinder's pressure over one cycle at 3000 rpm wide open,
        coloured by bank, with the firing TDCs from the layout marked
                                                    -> renders/presets.png
    python3 tools/plot_presets.py runs renders/run-i4.csv renders/run-i6.csv ...
        each preset's drive, from `rb_cli run --preset KEY --csv
        renders/run-KEY.csv`, one panel each        -> renders/runs.png
    python3 tools/plot_presets.py idle OWN.csv OTHER.csv
        an engine settling to idle with its own idle gains and with other
        ones (two `rb_cli run` CSVs)                -> renders/idle.png

-o FILE writes the chart somewhere else. Needs matplotlib (sudo apt install
python3-matplotlib).
"""
import argparse
import os
import statistics
import sys

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.ticker import FixedLocator

from rbplot import (BLUE, DASH, INK, INK_2, MUTED, ORANGE, SURFACE, label, read_csv, setup,
                    style, titles)

FIRING_COLUMNS = ("preset", "cylinder", "bank", "firing_deg", "crank_deg", "p_bar")
RUN_COLUMNS = ("t_s", "rpm", "throttle", "spark_cut")
# The presets in the order of core/src/presets.c: key, name and cylinder
# count (checked against the CSVs, so a change there can't mislabel a chart).
PRESETS = [("i4", "2.0 L inline-4", 4), ("i6", "3.0 L inline-6", 6),
           ("v8-cross", "5.0 L cross-plane V8", 8), ("v8-flat", "4.5 L flat-plane V8", 8),
           ("boxer4", "2.0 L flat-4 (boxer)", 4)]
BANK_COLOURS = (BLUE, ORANGE)


def gaps(angles):
    """Degrees from each firing to the next, around the cycle, from the first."""
    a = sorted(angles)
    return [((a[(k + 1) % len(a)] - a[k]) % 720.0) or 720.0 for k in range(len(a))]


def spacing(gap_list):
    if all(abs(g - gap_list[0]) < 1e-6 for g in gap_list):
        return f"every {gap_list[0]:.0f}°"
    return "–".join(f"{g:.0f}" for g in gap_list) + "°"


def firing_panel(ax, d, preset):
    """One preset's pressure traces, peak labels and firing marks; returns its heading."""
    rows = [i for i, x in enumerate(d["preset"]) if round(x) == preset]
    cylinders = sorted({round(d["cylinder"][i]) for i in rows})
    bank_of, firing_of = {}, {}
    for c in cylinders:
        idx = [i for i in rows if round(d["cylinder"][i]) == c]
        x = [d["crank_deg"][i] for i in idx]
        y = [d["p_bar"][i] for i in idx]
        bank_of[c] = round(d["bank"][idx[0]])
        firing_of[c] = d["firing_deg"][idx[0]]
        colour = BANK_COLOURS[bank_of[c] - 1]
        ax.plot(x, y, color=colour, lw=1.3, zorder=3)
        k = max(range(len(y)), key=y.__getitem__)
        ax.annotate(str(c), (x[k], y[k]), xytext=(0, 2), textcoords="offset points",
                    ha="center", va="bottom", fontsize=8, color=INK_2)
        ax.plot([firing_of[c]], [-4.5], marker="^", ms=5.5, color=colour, lw=0, zorder=4,
                clip_on=False)

    if preset >= len(PRESETS) or PRESETS[preset][2] != len(cylinders):
        sys.exit("plot_presets: the presets changed: update PRESETS in tools/plot_presets.py")
    order = sorted(cylinders, key=firing_of.get)
    heading = (f"{PRESETS[preset][1]}:  firing {'-'.join(map(str, order))}, "
               f"{spacing(gaps(firing_of.values()))}")
    banks = sorted(set(bank_of.values()))
    if len(banks) > 1:
        heading += "; " + ", ".join(
            f"bank {b} {spacing(gaps([firing_of[c] for c in cylinders if bank_of[c] == b]))}"
            for b in banks)
    return heading


def plot_firing(path, out):
    d = read_csv(path, FIRING_COLUMNS, "plot_presets")
    presets = sorted({round(p) for p in d["preset"]})

    setup()
    fig = plt.figure(figsize=(8.0, 9.4), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(len(presets), 1, left=0.085, right=0.975, top=0.855, bottom=0.065,
                          hspace=0.55)
    axes = []
    for row, preset in enumerate(presets):
        ax = fig.add_subplot(gs[row, 0], sharex=axes[0] if axes else None)
        style(ax)
        ax.set_title(firing_panel(ax, d, preset), loc="left", fontsize=9, color=INK, pad=5)
        ax.set_xlim(0, 720)
        ax.set_ylim(-9, 82)
        ax.yaxis.set_major_locator(FixedLocator([0, 25, 50, 75]))
        ax.xaxis.set_major_locator(FixedLocator(range(0, 721, 90)))
        if row < len(presets) - 1:
            ax.tick_params(labelbottom=False)
        axes.append(ax)
    axes[-1].set_xlabel("crank angle (degrees after cylinder 1 fires)")
    fig.text(0.018, (0.065 + 0.855) / 2, "cylinder pressure (bar)", rotation=90, va="center",
             ha="left", fontsize=9, color=INK_2)

    titles(fig, "When each cylinder fires",
           "Every cylinder's pressure over one cycle, held at 3000 rpm wide open. Each peak is "
           "a cylinder burning,\n10–12° after its firing TDC: the triangles, worked out from "
           "the crank, the banks and the firing\norder. The cross-plane V8's banks each fire "
           "unevenly; the flat-plane V8's take turns like two fours.", top=0.965)
    handles = [Line2D([], [], color=BLUE, lw=2, label="bank 1"),
               Line2D([], [], color=ORANGE, lw=2, label="bank 2")]
    fig.legend(handles=handles, loc="upper right", bbox_to_anchor=(0.975, 0.99), ncol=2,
               frameon=False, fontsize=8.5, labelcolor=INK_2, handlelength=2.2)
    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def preset_of(path):
    """The preset a run CSV is of, from its name (run-KEY.csv)."""
    stem = os.path.splitext(os.path.basename(path))[0]
    key = stem[4:] if stem.startswith("run-") else stem
    for k, name, _ in PRESETS:
        if k == key:
            return name
    sys.exit(f"plot_presets: {path}: name it run-KEY.csv, KEY one of "
             f"{', '.join(k for k, _, _ in PRESETS)}")


def between(d, lo, hi, column="rpm"):
    return [v for t, v in zip(d["t_s"], d[column]) if lo <= t < hi]


def plot_runs(paths, out):
    runs = [(preset_of(p), read_csv(p, RUN_COLUMNS, "plot_presets")) for p in paths]

    setup()
    fig = plt.figure(figsize=(8.0, 1.25 + 1.45 * len(runs)), dpi=135, facecolor=SURFACE)
    top = 1.0 - 1.15 / fig.get_figheight()
    gs = fig.add_gridspec(len(runs), 1, left=0.095, right=0.975, top=top,
                          bottom=0.5 / fig.get_figheight(), hspace=0.62)
    axes = []
    for row, (name, d) in enumerate(runs):
        ax = fig.add_subplot(gs[row, 0], sharex=axes[0] if axes else None)
        style(ax)
        t, rpm = d["t_s"], d["rpm"]
        ax.plot(t, rpm, color=BLUE, lw=1.2, zorder=3)
        idle = statistics.mean(between(d, 3.5, 4.5))
        limited = [r for r, cut in zip(rpm, d["spark_cut"]) if cut > 0.5]
        heading = f"{name}:  idles at {idle:.0f} rpm"
        if limited:
            heading += f", rev limiter at {max(limited):.0f}"
        ax.set_title(heading, loc="left", fontsize=9, color=INK, pad=5)
        ax.set_xlim(0, t[-1])
        ax.set_ylim(0, 10000)
        ax.yaxis.set_major_locator(FixedLocator([0, 3000, 6000, 9000]))
        if row < len(runs) - 1:
            ax.tick_params(labelbottom=False)
        axes.append(ax)
    axes[-1].set_xlabel("time (s)")
    fig.text(0.018, 0.5, "crank speed (rpm)", rotation=90, va="center", ha="left", fontsize=9,
             color=INK_2)
    titles(fig, "Every preset on its own crank",
           "The same drive for each: the key turned at 0 s, a blip of the throttle at 5 s, "
           "floored in neutral at 8.5 s\ninto the rev limiter, foot off at 10.5 s, ignition "
           "off at 13 s.", top=1.0 - 0.36 / fig.get_figheight())
    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def plot_idle(own_path, other_path, out):
    own = read_csv(own_path, RUN_COLUMNS, "plot_presets")
    other = read_csv(other_path, RUN_COLUMNS, "plot_presets")
    name = preset_of(own_path)
    lo, hi = 1.0, 5.0
    target = statistics.mean(between(own, 3.5, 4.5))

    setup()
    fig = plt.figure(figsize=(8.0, 3.9), dpi=135, facecolor=SURFACE)
    ax = fig.add_axes([0.085, 0.14, 0.89, 0.6])
    style(ax)
    for d, colour, z in ((other, ORANGE, 2), (own, BLUE, 3)):
        keep = [i for i, x in enumerate(d["t_s"]) if lo <= x <= hi]
        ax.plot([d["t_s"][i] for i in keep], [d["rpm"][i] for i in keep], color=colour, lw=1.3,
                zorder=z)
    ax.axhline(target, color=MUTED, lw=1.0, ls=DASH, zorder=1)
    label(ax, f"idle {target:.0f}", (hi, target), (-2, -7), ha="right", va="top", color=MUTED)
    ax.set_xlim(lo, hi)
    ax.set_ylim(target - 150, target + 200)
    ax.set_xlabel("time after the key (s)")
    ax.set_ylabel("crank speed (rpm)")
    titles(fig, f"Settling to idle: the {name}",
           "With the inline-4's idle gains (and an idle valve 2.5 times the four's, for 2.5 "
           "times the displacement) it hunts\nabout 40 rpm either side of idle, every 1.5 s. "
           "With gains tuned to it, it settles in 3 s. The ripple is its firings.",
           top=0.93)
    handles = [Line2D([], [], color=BLUE, lw=2, label="its own idle gains"),
               Line2D([], [], color=ORANGE, lw=2, label="the inline-4's")]
    fig.legend(handles=handles, loc="upper right", bbox_to_anchor=(0.975, 0.985), ncol=2,
               frameon=False, fontsize=8.5, labelcolor=INK_2, handlelength=2.2)
    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="chart", required=True)
    p = sub.add_parser("firing")
    p.add_argument("csv", nargs="?", default="renders/presets.csv")
    p.add_argument("-o", "--out", default="renders/presets.png")
    p = sub.add_parser("runs")
    p.add_argument("csv", nargs="+")
    p.add_argument("-o", "--out", default="renders/runs.png")
    p = sub.add_parser("idle")
    p.add_argument("own")
    p.add_argument("other")
    p.add_argument("-o", "--out", default="renders/idle.png")
    args = ap.parse_args()
    if args.chart == "firing":
        plot_firing(args.csv, args.out)
    elif args.chart == "runs":
        plot_runs(args.csv, args.out)
    else:
        plot_idle(args.own, args.other, args.out)


if __name__ == "__main__":
    main()
