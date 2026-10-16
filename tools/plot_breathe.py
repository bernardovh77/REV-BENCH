#!/usr/bin/env python3
"""
plot_breathe.py - charts of the breathing engine (stage 5), from the CSV files
that `rb_cli breathe` writes:

    python3 tools/plot_breathe.py cycle renders/breathe.csv renders/breathe-part.csv
        one cycle of cylinder 1 at one or two operating points: valve lift,
        cylinder pressure against crank angle, and the p-V diagram on log
        axes, where the pumping loop shows            -> renders/breathe.png

    python3 tools/plot_breathe.py map renders/ve.csv
        volumetric efficiency and plenum pressure against rpm, for four
        throttle openings (from rb_cli breathe --sweep) -> renders/ve.png

Needs matplotlib (sudo apt install python3-matplotlib).
"""
import argparse
import os
import sys

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.ticker import FixedLocator, FuncFormatter, NullFormatter

from rbplot import (AXIS, BLUE, BLUE_RAMP, DASH, GRID, INK_2, MUTED, ORANGE, SURFACE,
                    crank_axis, label, read_csv, setup, style, titles)

CYCLE_COLUMNS = ("crank_deg", "volume_cm3", "p_bar", "intake_lift_mm", "exhaust_lift_mm",
                 "plenum_kPa", "rpm", "throttle")


def describe(run):
    """'wide open' or '10 % throttle', for labels."""
    t = run["throttle"][0]
    return "wide open" if t >= 0.999 else f"{t * 100:.0f} % throttle"


def pumping_mep(run):
    """Pumping loss (bar) from the trace: the work the gas does on the piston
    during the exhaust and intake strokes (BDC at 180 to BDC at -180, through
    overlap TDC at +-360), divided by the swept volume, with the sign flipped."""
    th, V, p = run["crank_deg"], run["volume_cm3"], run["p_bar"]
    work = 0.0
    for i in range(len(th) - 1):
        mid = 0.5 * (th[i] + th[i + 1])
        if mid >= 180.0 or mid <= -180.0:
            work += 0.5 * (p[i] + p[i + 1]) * (V[i + 1] - V[i])
    return -work / (max(V) - min(V))


def log_axis(axis, lo, hi):
    """Log axis with plain 1-2-5 tick labels."""
    nice = [x * 10.0 ** k for k in range(-2, 5) for x in (1, 2, 5)]
    axis.set_major_locator(FixedLocator([t for t in nice if lo <= t <= hi]))
    axis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    axis.set_minor_formatter(NullFormatter())


def plot_cycle(paths, out):
    runs = [read_csv(p, CYCLE_COLUMNS, "plot_breathe") for p in paths]
    colors = [BLUE, ORANGE]
    rpm = runs[0]["rpm"][0]
    same_rpm = all(r["rpm"][0] == rpm for r in runs)

    setup()
    fig = plt.figure(figsize=(8.0, 5.4), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(2, 2, width_ratios=[1.25, 1.0], height_ratios=[1.0, 2.4], left=0.075,
                          right=0.975, top=0.80, bottom=0.115, hspace=0.12, wspace=0.24)
    ax_lift = fig.add_subplot(gs[0, 0])
    ax_p = fig.add_subplot(gs[1, 0], sharex=ax_lift)
    ax_pv = fig.add_subplot(gs[:, 1])
    for ax in (ax_lift, ax_p, ax_pv):
        style(ax)

    names = [describe(r) for r in runs]
    if same_rpm and len(runs) == 2:
        title = f"Breathing at {rpm:.0f} rpm: {names[0]} and {names[1]}"
    else:
        title = f"Breathing at {rpm:.0f} rpm, {names[0]}"
    plenum = [sum(r["plenum_kPa"]) / len(r["plenum_kPa"]) for r in runs]
    sub = ("Cylinder 1 of the 2.0 L four, turned by a motor with no fuel. "
           f"The plenum is at {plenum[0]:.0f} kPa" + (f" and {plenum[1]:.0f} kPa" if len(runs) > 1 else "")
           + ";\nthe lower loop on the right is the work it takes to pull the air in and push it out.")
    titles(fig, title, sub, top=0.95)

    # ---- valve lift: the same cams for every run
    r0 = runs[0]
    th = r0["crank_deg"]
    for col, name, shade in (("exhaust_lift_mm", "exhaust", AXIS), ("intake_lift_mm", "intake", GRID)):
        lift = r0[col]
        ax_lift.fill_between(th, 0, lift, color=shade, lw=0)
        ax_lift.plot(th, lift, color=MUTED, lw=1.0)
        i_max = max(range(len(lift)), key=lift.__getitem__)
        label(ax_lift, name, (th[i_max], lift[i_max]), (0, -14), ha="center", va="top")
    ax_lift.set_ylabel("valve lift (mm)")
    ax_lift.set_ylim(0, 1.25 * max(max(r0["intake_lift_mm"]), max(r0["exhaust_lift_mm"])))
    ax_lift.tick_params(labelbottom=False)

    # ---- pressure against crank angle, on a log scale to show both the
    # compression peak and the small pressures while the valves are open
    p_ambient = 1.01325
    ax_p.axhline(p_ambient, color=MUTED, lw=1.2, ls=DASH)
    label(ax_p, "atmosphere", (-200.0, p_ambient), (0, 4), ha="center", va="bottom", color=MUTED)
    for r, c, name in zip(runs, colors, names):
        ax_p.plot(r["crank_deg"], r["p_bar"], color=c, lw=2, solid_capstyle="round", zorder=3)
        i_top = max(range(len(r["p_bar"])), key=lambda i: (r["p_bar"][i], -abs(r["crank_deg"][i])))
        label(ax_p, f"{name}: {r['p_bar'][i_top]:.1f} bar", (r["crank_deg"][i_top], r["p_bar"][i_top]),
              (16, 0))
    ax_p.set_yscale("log")
    lo = min(min(r["p_bar"]) for r in runs) / 1.4
    hi = max(max(r["p_bar"]) for r in runs) * 1.6
    ax_p.set_ylim(lo, hi)
    log_axis(ax_p.yaxis, lo, hi)
    ax_p.set_ylabel("cylinder pressure (bar)")
    crank_axis(ax_p, -360.0, 360.0)

    # ---- p-V diagram on log axes: compression and expansion are nearly
    # straight lines, and the gas exchange makes the lower loop
    for r, c, name in zip(runs, colors, names):
        ax_pv.plot(r["volume_cm3"], r["p_bar"], color=c, lw=2, zorder=3,
                   label=f"{name}: pumping loss {pumping_mep(r):.2f} bar")
    ax_pv.set_xscale("log")
    ax_pv.set_yscale("log")
    vlo = min(min(r["volume_cm3"]) for r in runs) / 1.15
    vhi = max(max(r["volume_cm3"]) for r in runs) * 1.15
    ax_pv.set_xlim(vlo, vhi)
    ax_pv.set_ylim(lo, hi)
    log_axis(ax_pv.xaxis, vlo, vhi)
    log_axis(ax_pv.yaxis, lo, hi)
    ax_pv.set_xlabel("cylinder volume (cm³)")
    ax_pv.set_ylabel("cylinder pressure (bar)")
    ax_pv.legend(loc="upper right", frameon=False, fontsize=8.5, labelcolor=INK_2, handlelength=2.0)
    # Name the strokes of the throttled run's pumping loop, where it is widest.
    r = runs[-1]
    v_mid = (min(r["volume_cm3"]) * max(r["volume_cm3"])) ** 0.5
    exhaust = [p for t, v, p in zip(r["crank_deg"], r["volume_cm3"], r["p_bar"])
               if t > 180 and abs(v / v_mid - 1) < 0.05]
    intake = [p for t, v, p in zip(r["crank_deg"], r["volume_cm3"], r["p_bar"])
              if t < -180 and abs(v / v_mid - 1) < 0.05]
    if exhaust and intake:
        label(ax_pv, "exhaust stroke", (v_mid, max(exhaust)), (0, 4), ha="center", va="bottom")
        label(ax_pv, "intake stroke", (v_mid, min(intake)), (0, -4), ha="center", va="top")

    fig.savefig(out, facecolor=SURFACE)
    for r, name in zip(runs, names):
        print(f"{name}: peak {max(r['p_bar']):.2f} bar, plenum {sum(r['plenum_kPa']) / len(r['plenum_kPa']):.1f} kPa, "
              f"pumping loss {pumping_mep(r):.3f} bar (from the trace)")
    print(f"wrote {out}")


def plot_map(path, out):
    d = read_csv(path, ("rpm", "throttle", "ve", "map_kPa"), "plot_breathe")
    wanted = [0.02, 0.1, 0.25, 1.0]   # light to dark: more throttle, darker
    lines = []
    for t, c in zip(wanted, BLUE_RAMP):
        pts = sorted((r, v, m) for r, th, v, m in zip(d["rpm"], d["throttle"], d["ve"], d["map_kPa"])
                     if abs(th - t) < 1e-9)
        if pts:
            lines.append((t, c, pts))
    if not lines:
        sys.exit(f"plot_breathe: {path} has none of the throttle openings 2/10/25/100 %")

    setup()
    fig = plt.figure(figsize=(8.0, 4.4), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(1, 2, left=0.075, right=0.915, top=0.70, bottom=0.13, wspace=0.30)
    ax_ve = fig.add_subplot(gs[0, 0])
    ax_map = fig.add_subplot(gs[0, 1], sharex=ax_ve)
    for ax in (ax_ve, ax_map):
        style(ax)
    titles(fig, "How the 2.0 L four breathes, turned by a motor",
           "Volumetric efficiency: the air drawn in per cycle, against the cylinders' volume of "
           "ambient air.\nWide open, the valves start to choke it at high rpm; with the throttle "
           "nearly shut, the plenum is starved.", top=0.94)

    for t, c, pts in reversed(lines):   # darkest (wide open) on top
        rpm = [p[0] for p in pts]
        name = "wide open" if t >= 0.999 else f"{t * 100:.0f} %"
        ax_ve.plot(rpm, [p[1] for p in pts], color=c, lw=2, marker="o", ms=4, zorder=3)
        ax_map.plot(rpm, [p[2] for p in pts], color=c, lw=2, marker="o", ms=4, zorder=3)
        label(ax_ve, name, (rpm[-1], pts[-1][1]), (6, 0))
        label(ax_map, name, (rpm[-1], pts[-1][2]), (6, 0))
    ax_ve.set_ylim(0, 1.0)
    ax_ve.set_ylabel("volumetric efficiency")
    ax_map.axhline(101.325, color=MUTED, lw=1.2, ls=DASH)
    label(ax_map, "atmosphere", (ax_map.get_xlim()[0], 101.325), (2, 4), va="bottom", color=MUTED)
    ax_map.set_ylim(0, 112)
    ax_map.set_ylabel("plenum pressure (kPa)")
    for ax in (ax_ve, ax_map):
        ax.set_xlabel("engine speed (rpm)")
        ax.set_xlim(0, max(d["rpm"]) * 1.02)

    # Idle: the lowest speed at 2 % throttle.
    idle = [(r, m) for r, th, m in zip(d["rpm"], d["throttle"], d["map_kPa"]) if abs(th - 0.02) < 1e-9]
    if idle:
        r, m = min(idle)
        ax_map.scatter([r], [m], s=40, facecolor="none", edgecolor=INK_2, lw=1.2, zorder=4)
        label(ax_map, f"idle: {m:.0f} kPa", (r, m), (0, 8), ha="center", va="bottom")

    handles = [Line2D([], [], color=c, lw=2, marker="o", ms=4) for _, c, _ in lines]
    fig.legend(handles, ["wide open" if t >= 0.999 else f"{t * 100:.0f} %" for t, _, _ in lines],
               title="throttle opening", loc="upper left", bbox_to_anchor=(0.07, 0.815), ncol=4,
               frameon=False, fontsize=8.5, title_fontsize=8.5, labelcolor=INK_2,
               alignment="left").get_title().set_color(INK_2)

    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("what", choices=("cycle", "map"))
    ap.add_argument("csv", nargs="*")
    ap.add_argument("-o", "--out", help="output PNG")
    args = ap.parse_args()
    if args.what == "cycle":
        paths = args.csv or ["renders/breathe.csv"]
        if len(paths) > 2:
            sys.exit("plot_breathe: cycle takes one or two CSV files")
        plot_cycle(paths, args.out or os.path.splitext(paths[0])[0] + ".png")
    else:
        path = args.csv[0] if args.csv else "renders/ve.csv"
        plot_map(path, args.out or os.path.splitext(path)[0] + ".png")


if __name__ == "__main__":
    main()
