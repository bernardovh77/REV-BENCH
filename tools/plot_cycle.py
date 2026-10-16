#!/usr/bin/env python3
"""
plot_cycle.py - charts of one engine cycle from the CSV that `rb_cli motor`
writes (columns crank_deg, piston_mm, volume_cm3, p_bar, T_K, gamma):

  left   pressure against volume (the p-V diagram)
  right  pressure, gas temperature and gamma against crank angle

Each is compared with the textbook result for air with a constant gamma of 1.4.

    python3 tools/plot_cycle.py renders/motor.csv          # writes renders/motor.png
    python3 tools/plot_cycle.py renders/motor.csv --log    # p-V on log-log axes

Needs matplotlib (sudo apt install python3-matplotlib).
"""
import argparse
import math
import os

import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, FuncFormatter, NullFormatter

from rbplot import (BLUE, DASH, INK_2, MUTED, SURFACE, crank_axis, label, read_csv, setup,
                    style, titles)

GAMMA = 1.4   # the textbook's constant gamma for air


def textbook_line(ax, y, text, x_right):
    """A horizontal dashed reference, labelled at its right end."""
    ax.axhline(y, color=MUTED, lw=1.3, ls=DASH)
    label(ax, text, (x_right, y), (0, 4), ha="right", va="bottom", color=MUTED)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("csv", nargs="?", default="renders/motor.csv")
    ap.add_argument("-o", "--out", help="output PNG (default: the CSV's name with .png)")
    ap.add_argument("--log", action="store_true", help="p-V diagram on log-log axes")
    ap.add_argument("--title", default="One cylinder with its valves shut, turned by a motor")
    args = ap.parse_args()
    out = args.out or os.path.splitext(args.csv)[0] + ".png"

    d = read_csv(args.csv, ("crank_deg", "volume_cm3", "p_bar", "T_K"), "plot_cycle")
    theta, V, p, T = d["crank_deg"], d["volume_cm3"], d["p_bar"], d["T_K"]
    n = len(theta)

    # Of equal peaks (one at every TDC with the valves shut), label the one
    # nearest 0 degrees.
    def peak(y):
        return max(range(n), key=lambda i: (round(y[i], 6), -abs(theta[i])))

    i_tdc, i_bdc = peak(p), max(range(n), key=V.__getitem__)
    v_min, v_max = min(V), max(V)
    p_bdc, T_bdc = p[i_bdc], T[i_bdc]
    cr = v_max / v_min
    p_book = p_bdc * cr ** GAMMA            # p V^gamma = constant
    T_book = T_bdc * cr ** (GAMMA - 1.0)    # T V^(gamma - 1) = constant
    # Work, the area under the p-V curve (1 bar * 1 cm^3 = 0.1 J).
    seg = [0.5 * (p[i] + p[i + 1]) * (V[i + 1] - V[i]) * 0.1 for i in range(n - 1)]
    net_work = sum(seg)
    falling = [V[i + 1] < V[i] for i in range(n - 1)]
    compressions = sum(1 for i, f in enumerate(falling) if f and (i == 0 or not falling[i - 1])) or 1
    squeeze_work = -sum(w for w in seg if w < 0) / compressions

    engine = f"compression ratio {cr:.1f}"
    if "piston_mm" in d:
        stroke = max(d["piston_mm"]) - min(d["piston_mm"])
        bore = math.sqrt(4.0 * (v_max - v_min) * 1e3 / (math.pi * stroke))
        engine = f"{bore:.0f} × {stroke:.0f} mm cylinder, {engine}"

    setup()
    has_gamma = "gamma" in d
    rows = 3 if has_gamma else 2
    fig = plt.figure(figsize=(8.0, 5.9 if has_gamma else 4.9), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(rows, 2, width_ratios=[1.0, 1.2], left=0.075, right=0.975,
                          top=0.84 if has_gamma else 0.80, bottom=0.10 if has_gamma else 0.12,
                          hspace=0.14, wspace=0.26)
    ax_pv = fig.add_subplot(gs[:, 0])
    right = [fig.add_subplot(gs[0, 1])]
    right += [fig.add_subplot(gs[r, 1], sharex=right[0]) for r in range(1, rows)]
    for ax in [ax_pv] + right:
        style(ax)

    titles(fig, args.title,
           f"{engine}, starting at {p_bdc:.0f} bar and {T_bdc:.0f} K at BDC. Squeezing and\n"
           "releasing follow the same curve, so the p–V loop has no area: no net work.",
           top=0.955 if has_gamma else 0.95)

    # ---- p-V diagram, with the textbook curve p = p_bdc (V_max / V)^1.4
    V_ref = [v_min * cr ** (i / 200) for i in range(201)]
    ax_pv.plot(V, p, color=BLUE, lw=2, label="simulation", zorder=3)
    ax_pv.plot(V_ref, [p_bdc * (v_max / v) ** GAMMA for v in V_ref], color=MUTED, lw=1.3,
               ls=DASH, label=f"constant γ = {GAMMA}", zorder=4)
    ax_pv.scatter([V[i_tdc]], [p[i_tdc]], s=30, color=BLUE, edgecolor=SURFACE, lw=1.5, zorder=5)
    ax_pv.set_xlabel("cylinder volume (cm³)")
    ax_pv.set_ylabel("pressure (bar)")
    if args.log:
        nice = [x * 10 ** k for k in range(-1, 5) for x in (1, 2, 5)]
        ax_pv.set_xscale("log")
        ax_pv.set_yscale("log")
        for axis, lo, hi in ((ax_pv.xaxis, v_min, v_max), (ax_pv.yaxis, min(p), p_book)):
            axis.set_major_locator(FixedLocator([t for t in nice if lo / 1.5 <= t <= hi * 1.5]))
            axis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
            axis.set_minor_formatter(NullFormatter())
    else:
        ax_pv.set_xlim(0, v_max * 1.06)
        ax_pv.set_ylim(0, max(p_book, p[i_tdc]) * 1.1)
    label(ax_pv, f"TDC  {p[i_tdc]:.2f} bar", (V[i_tdc], p[i_tdc]), (9, -3))
    label(ax_pv, f"{p_book:.2f} bar", (v_min, p_book), (9, 9), color=MUTED)
    if args.log:   # the curve climbs to the left of BDC: label below it
        label(ax_pv, "BDC", (v_max, p_bdc), (-4, -9), ha="right", va="top")
    else:
        label(ax_pv, "BDC", (v_max, p_bdc), (0, 9), ha="center", va="bottom")
    ax_pv.legend(loc="upper right", frameon=False, fontsize=8.5, labelcolor=INK_2,
                 handlelength=2.4)

    # ---- against crank angle
    x_right = max(theta)
    ax = right[0]
    ax.plot(theta, p, color=BLUE, lw=2, zorder=3)
    textbook_line(ax, p_book, f"constant γ: {p_book:.1f} bar", x_right)
    label(ax, f"{p[i_tdc]:.2f} bar", (theta[i_tdc], p[i_tdc]), (7, -6))
    ax.set_ylim(0, max(p_book, p[i_tdc]) * 1.15)
    ax.set_ylabel("pressure (bar)")

    ax = right[1]
    i_hot = peak(T)
    ax.plot(theta, T, color=BLUE, lw=2, zorder=3)
    textbook_line(ax, T_book, f"constant γ: {T_book:.0f} K", x_right)
    label(ax, f"{T[i_hot]:.0f} K", (theta[i_hot], T[i_hot]), (7, -6))
    ax.set_ylim(0, max(T_book, T[i_hot]) * 1.15)
    ax.set_ylabel("temperature (K)")

    if has_gamma:
        ax = right[2]
        g = d["gamma"]
        i_low = min(range(n), key=lambda i: (round(g[i], 6), abs(theta[i])))
        ax.plot(theta, g, color=BLUE, lw=2, zorder=3)
        textbook_line(ax, GAMMA, f"constant γ = {GAMMA}", x_right)
        label(ax, f"{g[i_low]:.3f}", (theta[i_low], g[i_low]), (7, 2))
        lo, hi = min(g + [GAMMA]), max(g + [GAMMA])
        ax.set_ylim(lo - 0.25 * (hi - lo), hi + 0.45 * (hi - lo))
        ax.set_ylabel("γ = cp / cv")

    for ax in right[:-1]:
        ax.tick_params(labelbottom=False)
    crank_axis(right[-1], min(theta), max(theta))

    fig.savefig(out, facecolor=SURFACE)
    print(f"TDC: {p[i_tdc]:.2f} bar, {T[i_hot]:.1f} K (constant gamma {GAMMA}: "
          f"{p_book:.2f} bar, {T_book:.1f} K)")
    print(f"work done on the gas per compression {squeeze_work:.1f} J, "
          f"net over the whole trace {net_work:+.4f} J")
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
