#!/usr/bin/env python3
"""
plot_run.py - chart of the engine on its own crank (stage 7), from the CSV
that `rb_cli run` writes:

    python3 tools/plot_run.py [renders/run.csv] [-o renders/run.png]

The whole drive (speed, pedal and idle air valve, plenum pressure), with
close-ups of the start and of the rev limiter.

Needs matplotlib (sudo apt install python3-matplotlib).
"""
import argparse
import os

import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator

from rbplot import (BLUE, DASH, GRID, INK_2, MUTED, ORANGE, SURFACE, label, read_csv, setup,
                    style, titles)

COLUMNS = ("t_s", "rpm", "throttle", "idle_valve", "map_kPa", "starter", "ignition", "spark_cut")


def first(t, flags, after=0.0):
    """The first time after `after` where a 0/1 column changes."""
    start = next(i for i, x in enumerate(t) if x >= after)
    for i in range(start + 1, len(t)):
        if flags[i] != flags[i - 1]:
            return t[i]
    return None


def window(d, lo, hi):
    keep = [i for i, x in enumerate(d["t_s"]) if lo <= x <= hi]
    return {k: [v[i] for i in keep] for k, v in d.items()}


def spans(t, flags):
    """(start, end) of each run of 1s in a 0/1 column."""
    out, start = [], None
    for x, f in zip(t, flags):
        if f > 0.5 and start is None:
            start = x
        elif f <= 0.5 and start is not None:
            out.append((start, x))
            start = None
    if start is not None:
        out.append((start, t[-1]))
    return out


def plot(path, out):
    d = read_csv(path, COLUMNS, "plot_run")
    t, rpm = d["t_s"], d["rpm"]
    released = first(t, d["starter"])
    off = first(t, d["ignition"])
    pedal = [100.0 * x for x in d["throttle"]]
    valve = [100.0 * x for x in d["idle_valve"]]

    setup()
    fig = plt.figure(figsize=(8.0, 6.6), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(3, 2, width_ratios=[2.3, 1.0], height_ratios=[2.1, 1.0, 1.0],
                          left=0.085, right=0.975, top=0.80, bottom=0.085, hspace=0.34,
                          wspace=0.30)
    ax_rpm = fig.add_subplot(gs[0, 0])
    ax_ped = fig.add_subplot(gs[1, 0], sharex=ax_rpm)
    ax_map = fig.add_subplot(gs[2, 0], sharex=ax_rpm)
    right = gs[:, 1].subgridspec(2, 1, height_ratios=[1.0, 1.25], hspace=0.42)
    ax_start = fig.add_subplot(right[0])
    ax_lim = fig.add_subplot(right[1])
    for ax in (ax_rpm, ax_ped, ax_map, ax_start, ax_lim):
        style(ax)

    titles(fig, "The engine on its own crank: start, idle, rev, stop",
           "Turn the key: the starter cranks it, it fires and settles to 800 rpm on its idle air "
           "valve. A blip of\nthe throttle, then floored in neutral into the rev limiter, which "
           f"cuts the spark at 6800 rpm. Ignition off at {off:.0f} s.\nCoasting with the foot "
           "off, the computer cuts the fuel: those are the straight-line falls.", top=0.955)

    # ---- speed, the whole drive
    ax_rpm.plot(t, rpm, color=BLUE, lw=1.6, zorder=3)
    ax_rpm.set_xlim(0, t[-1])
    ax_rpm.set_ylim(0, 7800)
    ax_rpm.yaxis.set_major_locator(FixedLocator(range(0, 7001, 1000)))
    ax_rpm.set_ylabel("crank speed (rpm)")
    ax_rpm.tick_params(labelbottom=False)
    i_flare = max((i for i, x in enumerate(t) if x < 2.0), key=rpm.__getitem__)
    label(ax_rpm, f"starts, flares to {rpm[i_flare]:.0f}", (t[i_flare], rpm[i_flare]), (5, 4),
          va="bottom")
    label(ax_rpm, "idles at 800", (3.0, 800.0), (0, 8), ha="center", va="bottom")
    i_blip = max((i for i, x in enumerate(t) if 5.0 <= x < 8.0), key=rpm.__getitem__)
    label(ax_rpm, "a blip", (t[i_blip], rpm[i_blip]), (5, 4), va="bottom")
    label(ax_rpm, "floored: on the limiter", (8.55, 6830.0), (0, 8), va="bottom")
    if off is not None:
        ax_rpm.axvline(off, color=MUTED, lw=1.0, ls=DASH)
        label(ax_rpm, "ignition off", (off, 4200.0), (5, 0), color=MUTED)

    # ---- pedal and idle air valve
    ax_ped.plot(t, pedal, color=BLUE, lw=1.6, zorder=3, label="throttle pedal")
    ax_ped.plot(t, valve, color=ORANGE, lw=1.4, zorder=2, label="idle air valve")
    ax_ped.set_ylim(0, 110)
    ax_ped.yaxis.set_major_locator(FixedLocator([0, 50, 100]))
    ax_ped.set_ylabel("open (%)")
    ax_ped.tick_params(labelbottom=False)
    ax_ped.legend(loc="upper left", frameon=False, fontsize=8, labelcolor=INK_2, ncol=2,
                  handlelength=1.8, borderaxespad=0.1)

    # ---- plenum pressure
    ax_map.plot(t, d["map_kPa"], color=BLUE, lw=1.4, zorder=3)
    ax_map.set_ylim(0, 115)
    ax_map.yaxis.set_major_locator(FixedLocator([0, 50, 100]))
    ax_map.set_ylabel("plenum (kPa)")
    ax_map.set_xlabel("time (s)")

    # ---- close-up: the start
    s = window(d, 0.0, 0.8)
    ax_start.plot(s["t_s"], s["rpm"], color=BLUE, lw=1.6, zorder=3)
    ax_start.set_xlim(0, 0.8)
    ax_start.set_ylim(0, 2300)
    ax_start.set_title("the start", fontsize=9, color=INK_2, loc="left")
    ax_start.set_xlabel("time (s)")
    if released is not None:
        ax_start.axvline(released, color=MUTED, lw=1.0, ls=DASH)
        label(ax_start, "starter\nreleased", (released, 450.0), (5, 0), color=MUTED, size=8)
    i_crank = max((i for i, x in enumerate(s["t_s"]) if x < 0.15), key=s["rpm"].__getitem__)
    label(ax_start, "cranking", (0.03, s["rpm"][i_crank]), (0, 8), va="bottom", size=8)

    # ---- close-up: the rev limiter
    lim = window(d, 9.8, 10.3)
    for a, b in spans(lim["t_s"], lim["spark_cut"]):
        ax_lim.axvspan(a, b, color=GRID, lw=0, zorder=0)
    ax_lim.plot(lim["t_s"], lim["rpm"], color=BLUE, lw=1.6, zorder=3)
    for level in (6800.0, 6700.0):
        ax_lim.axhline(level, color=MUTED, lw=1.0, ls=DASH, zorder=1)
    ax_lim.set_xlim(lim["t_s"][0], lim["t_s"][-1])
    ax_lim.set_ylim(6620, 6980)
    ax_lim.yaxis.set_major_locator(FixedLocator([6650, 6700, 6750, 6800, 6850, 6900]))
    ax_lim.set_title("the rev limiter", fontsize=9, color=INK_2, loc="left")
    ax_lim.set_xlabel("time (s)")
    ax_lim.set_ylabel("crank speed (rpm)")
    ax_lim.text(0.03, 0.97, "spark cut (shaded) above 6800,\nback on below 6700", transform=ax_lim.transAxes,
                ha="left", va="top", fontsize=8, color=INK_2, linespacing=1.3)

    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("csv", nargs="?", default="renders/run.csv")
    ap.add_argument("-o", "--out", help="output PNG (default: the CSV's name with .png)")
    args = ap.parse_args()
    plot(args.csv, args.out or os.path.splitext(args.csv)[0] + ".png")


if __name__ == "__main__":
    main()
