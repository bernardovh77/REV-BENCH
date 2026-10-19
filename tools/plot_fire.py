#!/usr/bin/env python3
"""
plot_fire.py - charts of the engine running on fuel (stage 6), from the CSV
files that `rb_cli fire` writes:

    python3 tools/plot_fire.py cycle renders/fire.csv renders/fire-motored.csv
        one cycle of cylinder 1, fired against motored: pressure, burned
        fraction and temperature against crank angle, and the p-V diagram
                                                      -> renders/fire.png
    python3 tools/plot_fire.py spark renders/spark.csv
        torque and peak pressure against spark advance (rb_cli fire --spark-sweep)
                                                      -> renders/spark.png
    python3 tools/plot_fire.py power renders/power.csv
        wide-open torque and power against rpm (rb_cli fire --rpm-sweep): brake
        and indicated when the CSV has the brake columns (stage 7 on)
                                                      -> renders/power.png

Needs matplotlib (sudo apt install python3-matplotlib).
"""
import argparse
import os
import sys

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.ticker import FixedLocator, FuncFormatter, NullFormatter

from rbplot import (BLUE, DASH, GRID, INK_2, MINUS, MUTED, SURFACE, label, read_csv, setup,
                    style, titles)

CYCLE_COLUMNS = ("crank_deg", "volume_cm3", "p_bar", "T_K", "burned", "spark_deg", "ignition",
                 "rpm", "throttle")


def half_cycle_axis(ax):
    """Crank angle from BDC before firing TDC to BDC after it."""
    ax.set_xlim(-180, 180)
    ticks = [-180, -90, 0, 90, 180]
    names = {-180: "\nBDC", 0: "\nTDC", 180: "\nBDC"}
    ax.xaxis.set_major_locator(FixedLocator(ticks))
    ax.set_xticklabels([f"{t:g}".replace("-", MINUS) + names.get(t, "") for t in ticks])
    ax.set_xlabel("crank angle (degrees after firing TDC)")


def window(run, lo=-180.0, hi=180.0):
    """The rows of a cycle trace between two crank angles."""
    keep = [i for i, t in enumerate(run["crank_deg"]) if lo <= t <= hi]
    return {k: [v[i] for i in keep] for k, v in run.items()}


def at_volume(run, lo, hi, volume):
    """The (volume, pressure) point where the trace, between two crank angles,
    comes closest to a volume."""
    i = min((k for k, t in enumerate(run["crank_deg"]) if lo <= t <= hi),
            key=lambda k: abs(run["volume_cm3"][k] - volume))
    return run["volume_cm3"][i], run["p_bar"][i]


def imep(run):
    """Net mean effective pressure of the logged cycle (bar): the loop's work
    over the swept volume, by the trapezoid rule."""
    V, p = run["volume_cm3"], run["p_bar"]
    work = sum(0.5 * (p[k] + p[k + 1]) * (V[k + 1] - V[k]) for k in range(len(V) - 1))
    return work / (max(V) - min(V))


def degrees(x):
    return f"{x:.0f}".replace("-", MINUS)


def log_axis(axis, lo, hi):
    nice = [x * 10.0 ** k for k in range(-2, 5) for x in (1, 2, 5)]
    axis.set_major_locator(FixedLocator([t for t in nice if lo <= t <= hi]))
    axis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    axis.set_minor_formatter(NullFormatter())


def plot_cycle(fired_path, motored_path, out):
    fired = read_csv(fired_path, CYCLE_COLUMNS, "plot_fire")
    motored = read_csv(motored_path, CYCLE_COLUMNS, "plot_fire") if motored_path else None
    f = window(fired)
    m = window(motored) if motored else None
    spark = fired["spark_deg"][0]
    rpm = fired["rpm"][0]
    throttle = fired["throttle"][0]
    i_peak = max(range(len(f["p_bar"])), key=f["p_bar"].__getitem__)
    i_hot = max(range(len(f["T_K"])), key=f["T_K"].__getitem__)

    setup()
    fig = plt.figure(figsize=(8.0, 5.9), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(2, 2, width_ratios=[1.2, 1.0], height_ratios=[1.6, 1.0], left=0.075,
                          right=0.975, top=0.83, bottom=0.11, hspace=0.32, wspace=0.26)
    ax_p = fig.add_subplot(gs[0, 0])
    ax_x = fig.add_subplot(gs[1, 0], sharex=ax_p)
    ax_pv = fig.add_subplot(gs[0, 1])
    ax_T = fig.add_subplot(gs[1, 1])
    for ax in (ax_p, ax_x, ax_pv, ax_T):
        style(ax)

    load = "wide open" if throttle >= 0.999 else f"{throttle * 100:.0f} % throttle"
    titles(fig, f"Firing: {rpm:.0f} rpm, {load}, spark {spark:g}° before TDC",
           "Cylinder 1 of the 2.0 L four burning a stoichiometric mixture, against the same "
           "cylinder\nmotored with no fuel. The burn lasts 60° from the spark; the p–V loop "
           f"works out to {imep(fired):.1f} bar net IMEP.", top=0.955)
    handles = [Line2D([], [], color=BLUE, lw=2, label="fired")]
    if motored:
        handles.append(Line2D([], [], color=MUTED, lw=1.4, ls=DASH, label="motored"))
    fig.legend(handles=handles, loc="upper right", bbox_to_anchor=(0.975, 0.985), ncol=2,
               frameon=False, fontsize=8.5, labelcolor=INK_2, handlelength=2.2)

    # ---- pressure against crank angle
    peak = f"{f['p_bar'][i_peak]:.1f} bar at {degrees(f['crank_deg'][i_peak])}° after TDC"
    if m:
        ax_p.plot(m["crank_deg"], m["p_bar"], color=MUTED, lw=1.4, ls=DASH, zorder=2)
        peak += f"\n(motored: {max(m['p_bar']):.1f} bar)"
    ax_p.plot(f["crank_deg"], f["p_bar"], color=BLUE, lw=2, zorder=3)
    ax_p.scatter([f["crank_deg"][i_peak]], [f["p_bar"][i_peak]], s=30, color=BLUE,
                 edgecolor=SURFACE, lw=1.5, zorder=4)
    label(ax_p, peak, (f["crank_deg"][i_peak], f["p_bar"][i_peak]), (9, 4), va="top")
    ax_p.axvline(-spark, color=MUTED, lw=1.0)
    label(ax_p, "spark", (-spark, max(f["p_bar"]) * 0.55), (-4, 0), ha="right", color=MUTED)
    ax_p.set_ylim(0, max(f["p_bar"]) * 1.12)
    ax_p.set_ylabel("cylinder pressure (bar)")
    ax_p.tick_params(labelbottom=False)

    # ---- burned fraction
    burned = [100.0 * x for x in f["burned"]]
    ax_x.plot(f["crank_deg"], burned, color=BLUE, lw=2, zorder=3)
    ax_x.axvline(-spark, color=MUTED, lw=1.0)
    for level in (10, 50, 90):
        k = next((i for i, x in enumerate(burned) if x >= level), None)
        if k is not None:
            ax_x.scatter([f["crank_deg"][k]], [level], s=22, color=BLUE, edgecolor=SURFACE,
                         lw=1.2, zorder=4)
            label(ax_x, f"{level} % at {degrees(f['crank_deg'][k])}°",
                  (f["crank_deg"][k], level), (7, -3), size=8)
    ax_x.set_ylim(0, 110)
    ax_x.yaxis.set_major_locator(FixedLocator([0, 50, 100]))
    ax_x.set_ylabel("burned (%)")
    half_cycle_axis(ax_x)

    # ---- p-V diagram (the whole cycle)
    if motored:
        ax_pv.plot(motored["volume_cm3"], motored["p_bar"], color=MUTED, lw=1.4, ls=DASH, zorder=2)
    ax_pv.plot(fired["volume_cm3"], fired["p_bar"], color=BLUE, lw=2, zorder=3)
    ax_pv.set_xscale("log")
    ax_pv.set_yscale("log")
    lo = min(min(fired["p_bar"]), min(motored["p_bar"]) if motored else 1e9) / 1.3
    hi = max(fired["p_bar"]) * 1.5
    vlo, vhi = min(fired["volume_cm3"]) / 1.15, max(fired["volume_cm3"]) * 1.15
    ax_pv.set_xlim(vlo, vhi)
    ax_pv.set_ylim(lo, hi)
    log_axis(ax_pv.xaxis, vlo, vhi)
    log_axis(ax_pv.yaxis, lo, hi)
    ax_pv.set_xlabel("cylinder volume (cm³, log scale)")
    ax_pv.set_ylabel("pressure (bar, log scale)")
    # The strokes, each labelled on the side of its line that stays clear.
    label(ax_pv, "compression", at_volume(fired, -170, -40, 150), (-4, -4), ha="right", va="top")
    label(ax_pv, "expansion", at_volume(fired, 40, 170, 200), (4, 4), va="bottom")
    v, top_p = at_volume(fired, 200, 340, 200)   # the exhaust stroke: the loop's top
    if motored:
        top_p = max(top_p, at_volume(motored, 200, 340, 200)[1])
    label(ax_pv, "pumping loop", (v, top_p), (0, 4), ha="center", va="bottom")

    # ---- temperature
    if m:
        ax_T.plot(m["crank_deg"], m["T_K"], color=MUTED, lw=1.4, ls=DASH, zorder=2)
    ax_T.plot(f["crank_deg"], f["T_K"], color=BLUE, lw=2, zorder=3)
    ax_T.scatter([f["crank_deg"][i_hot]], [f["T_K"][i_hot]], s=30, color=BLUE, edgecolor=SURFACE,
                 lw=1.5, zorder=4)
    label(ax_T, f"{f['T_K'][i_hot]:.0f} K", (f["crank_deg"][i_hot], f["T_K"][i_hot]), (7, 3),
          va="bottom")
    ax_T.set_ylim(0, max(f["T_K"]) * 1.15)
    ax_T.set_ylabel("gas temperature (K)")
    half_cycle_axis(ax_T)

    fig.savefig(out, facecolor=SURFACE)
    print(f"fired: peak {f['p_bar'][i_peak]:.1f} bar at {f['crank_deg'][i_peak]:.1f} deg, "
          f"{f['T_K'][i_hot]:.0f} K at {f['crank_deg'][i_hot]:.1f} deg")
    print(f"wrote {out}")


def plot_spark(path, out):
    d = read_csv(path, ("spark_deg", "torque_Nm", "p_max_bar", "angle_p_max", "rpm", "throttle"),
                 "plot_fire")
    x = d["spark_deg"]
    i_best = max(range(len(x)), key=d["torque_Nm"].__getitem__)
    best = x[i_best]

    setup()
    fig = plt.figure(figsize=(8.0, 3.9), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(1, 3, left=0.075, right=0.975, top=0.66, bottom=0.15, wspace=0.34)
    axes = [fig.add_subplot(gs[0, k]) for k in range(3)]
    for ax in axes:
        style(ax)
    load = "wide open" if d["throttle"][0] >= 0.999 else f"{d['throttle'][0] * 100:.0f} % throttle"
    titles(fig, f"Spark advance at {d['rpm'][0]:.0f} rpm, {load}",
           f"Advancing the spark always raises the peak pressure, but torque is best at {best:g}° "
           f"(MBT), with the peak\n{d['angle_p_max'][i_best]:.0f}° after TDC. Fire earlier and the "
           "burn pushes against the rising piston; fire later\nand it chases the falling one.",
           top=0.955)

    # Each value label sits on the side of its curve that stays clear.
    panels = ((axes[0], d["torque_Nm"], "indicated torque (N m)", "{:.0f} N m", (6, 7), "left"),
              (axes[1], d["p_max_bar"], "peak pressure (bar)", "{:.0f} bar", (-7, 5), "right"),
              (axes[2], d["angle_p_max"], "angle of peak pressure (° after TDC)", "{:.0f}°", (6, 7),
               "left"))
    for ax, y, name, fmt, offset, ha in panels:
        ax.axvline(best, color=MUTED, lw=1.0, ls=DASH)
        ax.plot(x, y, color=BLUE, lw=2, marker="o", ms=3.5, zorder=3)
        ax.scatter([best], [y[i_best]], s=34, color=BLUE, edgecolor=SURFACE, lw=1.5, zorder=4)
        label(ax, fmt.format(y[i_best]), (best, y[i_best]), offset, ha=ha, va="bottom")
        ax.set_xlabel("spark advance (° before TDC)")
        ax.set_ylabel(name)
        ax.set_xlim(min(x), max(x))
    axes[2].axhspan(10, 20, color=GRID, zorder=0)
    label(axes[2], "textbook\n10–20°", (max(x), 15), (-4, 0), ha="right", color=MUTED, size=8)
    axes[0].set_ylim(min(d["torque_Nm"]) * 0.97, max(d["torque_Nm"]) * 1.02)
    axes[1].set_ylim(0, max(d["p_max_bar"]) * 1.1)
    axes[2].set_ylim(min(0, min(d["angle_p_max"])), max(d["angle_p_max"]) * 1.1)
    label(axes[0], f"MBT {best:g}°", (best, axes[0].get_ylim()[0]), (4, 4), va="bottom",
          color=MUTED, size=8)

    fig.savefig(out, facecolor=SURFACE)
    print(f"best torque {d['torque_Nm'][i_best]:.1f} N m at {best:g} deg before TDC")
    print(f"wrote {out}")


def spark_text(d):
    """The spark the sweep ran with: fixed, or the engine computer's (stage 9)."""
    lo, hi = min(d["spark_deg"]), max(d["spark_deg"])
    if hi - lo < 0.05:
        return f"Spark {lo:g}° before TDC"
    return f"Spark for the best torque ({lo:.0f}–{hi:.0f}° before TDC)"


def plot_power(path, out):
    d = read_csv(path, ("rpm", "torque_Nm", "power_kW", "spark_deg"), "plot_fire")
    rpm = d["rpm"]
    brake = "brake_torque_Nm" in d and "brake_power_kW" in d   # rb_cli from stage 7 on

    setup()
    fig = plt.figure(figsize=(8.0, 3.9), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(1, 2, left=0.075, right=0.975, top=0.70, bottom=0.15, wspace=0.25)
    ax_t, ax_p = (fig.add_subplot(gs[0, k]) for k in range(2))
    for ax in (ax_t, ax_p):
        style(ax)
    if brake:
        f_lo, f_hi = d["fmep_bar"][0], d["fmep_bar"][-1]
        titles(fig, "Power curve at the crank: the 2.0 L four, wide open",
               f"Friction takes {f_lo:.1f} bar of mean pressure at {rpm[0]:.0f} rpm and "
               f"{f_hi:.1f} bar at {rpm[-1]:.0f}, so the brake torque\npeaks lower and earlier "
               f"than the gas's own (indicated) torque. {spark_text(d)}, stoichiometric.", top=0.94)
        handles = [Line2D([], [], color=BLUE, lw=2, marker="o", ms=3.5, label="brake (at the crank)"),
                   Line2D([], [], color=MUTED, lw=1.4, ls=DASH, label="indicated (before friction)")]
        fig.legend(handles=handles, loc="lower right", bbox_to_anchor=(0.975, 0.72), ncol=2,
                   frameon=False, fontsize=8.5, labelcolor=INK_2, handlelength=2.2)
        panels = ((ax_t, d["brake_torque_Nm"], d["torque_Nm"], "torque (N m)", "N m"),
                  (ax_p, d["brake_power_kW"], d["power_kW"], "power (kW)", "kW"))
    else:
        titles(fig, "First power curve: the 2.0 L four, wide open",
               f"Indicated, before friction (stage 7 takes that off), {spark_text(d).lower()}, "
               "stoichiometric.\nThe torque curve is nearly flat: there are no pressure "
               "waves in the pipes yet (stage 10) to tune it.", top=0.94)
        panels = ((ax_t, d["torque_Nm"], None, "indicated torque (N m)", "N m"),
                  (ax_p, d["power_kW"], None, "indicated power (kW)", "kW"))
    for ax, y, ref, name, unit in panels:
        if ref is not None:
            ax.plot(rpm, ref, color=MUTED, lw=1.4, ls=DASH, zorder=2)
        ax.plot(rpm, y, color=BLUE, lw=2, marker="o", ms=3.5, zorder=3)
        i = max(range(len(rpm)), key=y.__getitem__)
        ax.scatter([rpm[i]], [y[i]], s=34, color=BLUE, edgecolor=SURFACE, lw=1.5, zorder=4)
        text = f"{y[i]:.0f} {unit} at {rpm[i]:.0f} rpm"
        if i == len(rpm) - 1:   # at the right edge: below the curve, with a leader
            ax.annotate(text, (rpm[i], y[i]), xytext=(-4, -46), textcoords="offset points",
                        ha="right", va="top", fontsize=8.5, color=INK_2,
                        arrowprops=dict(arrowstyle="-", color=MUTED, lw=0.8, shrinkA=0, shrinkB=4))
        else:
            label(ax, text, (rpm[i], y[i]), (0, -10), ha="center", va="top")
        ax.set_xlabel("engine speed (rpm)")
        ax.set_ylabel(name)
        ax.set_xlim(0, max(rpm) * 1.04)
        ax.set_ylim(0, max(max(y), max(ref) if ref is not None else 0) * 1.18)

    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("what", choices=("cycle", "spark", "power"))
    ap.add_argument("csv", nargs="*")
    ap.add_argument("-o", "--out", help="output PNG")
    args = ap.parse_args()
    if args.what == "cycle":
        paths = args.csv or ["renders/fire.csv", "renders/fire-motored.csv"]
        if len(paths) > 2:
            sys.exit("plot_fire: cycle takes the fired CSV and, optionally, a motored one")
        motored = paths[1] if len(paths) > 1 and os.path.exists(paths[1]) else None
        plot_cycle(paths[0], motored, args.out or os.path.splitext(paths[0])[0] + ".png")
    else:
        path = args.csv[0] if args.csv else f"renders/{args.what}.csv"
        fn = plot_spark if args.what == "spark" else plot_power
        fn(path, args.out or os.path.splitext(path)[0] + ".png")


if __name__ == "__main__":
    main()
