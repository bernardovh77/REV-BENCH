#!/usr/bin/env python3
"""
plot_pipes.py - charts of the 1D pipes (stage 10):

    python3 tools/plot_pipes.py pulse renders/pulse.csv
        a pressure pulse in a 2 m pipe, closed at one end and open at the
        other (rb_cli pipes --pulse): where it is at every moment, and the
        pressure at two points a metre apart  -> renders/pulse.png
    python3 tools/plot_pipes.py engine renders/pipes.csv
        one cycle of cylinder 1's runner and header, cell by cell, and the
        pressures either side of its valves (rb_cli pipes --preset KEY --rpm N)
                                               -> renders/pipes.png
    python3 tools/plot_pipes.py compare BEFORE.csv AFTER.csv
        volumetric efficiency and brake torque against speed, wide open,
        before and after the pipes (rb_cli fire --rpm-sweep, stage 9's build
        and stage 10's)                        -> renders/pipes-ve.png

-o FILE writes the chart somewhere else. Needs matplotlib and numpy (sudo apt
install python3-matplotlib).
"""
import argparse
import os

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LinearSegmentedColormap, TwoSlopeNorm

from rbplot import (AXIS, BLUE, DASH, INK_2, MINUS, MUTED, ORANGE, SURFACE, crank_axis, label,
                    read_csv, setup, style, titles)  # noqa: F401 (MINUS: tick labels)

PULSE_COLUMNS = ("time_ms", "x_m", "p_Pa")
ENGINE_COLUMNS = ("crank_deg", "pipe", "cell", "x_m", "p_kPa", "intake_lift_mm", "exhaust_lift_mm")
SWEEP_COLUMNS = ("rpm", "ve", "brake_torque_Nm")
P_AMBIENT = 101.325   # kPa: the presets' air
C_300K = 347.2        # m/s: the speed of sound in the pulse's air (rb_cli pipes --pulse says)
END_CORRECTION = 0.3 * 0.040   # m: the pulse pipe's open end, 0.3 diameters

# Pressure above the surroundings in blue, below in orange, nothing in white:
# the series colours, so a compression and a rarefaction read like the lines.
WAVES = LinearSegmentedColormap.from_list("waves", [ORANGE, "#f6b394", "#ffffff", "#9cc2ef", BLUE])


def grid(data, xcol, ycol, vcol, select=None):
    """Rows of a long CSV as a 2D array: one row per distinct y, one column per x."""
    rows = [i for i in range(len(data[xcol])) if select is None or select(i)]
    xs = sorted({data[xcol][i] for i in rows})
    ys = sorted({data[ycol][i] for i in rows})
    xi = {x: k for k, x in enumerate(xs)}
    yi = {y: k for k, y in enumerate(ys)}
    v = np.full((len(ys), len(xs)), np.nan)
    for i in rows:
        v[yi[data[ycol][i]], xi[data[xcol][i]]] = data[vcol][i]
    return np.array(xs), np.array(ys), v


def colorbar(fig, mesh, ax, text):
    bar = fig.colorbar(mesh, ax=ax, pad=0.015, fraction=0.035, aspect=18)
    bar.outline.set_visible(False)
    bar.ax.tick_params(colors=AXIS, labelcolor=MUTED, labelsize=8, length=2)
    bar.set_label(text, color=INK_2, fontsize=8.5)
    return bar


def pulse(args):
    d = read_csv(args.csv[0], PULSE_COLUMNS, "plot_pipes")
    t, x, p = grid(d, "time_ms", "x_m", "p_Pa")
    length = x[-1] + 0.5 * (x[1] - x[0])

    fig = plt.figure(figsize=(7.5, 6.6), facecolor=SURFACE)
    top = fig.add_axes([0.085, 0.42, 0.80, 0.40])
    low = fig.add_axes([0.085, 0.085, 0.80, 0.24])
    style(top)
    style(low)
    top.grid(False)
    edge = max(abs(np.nanmin(p)), abs(np.nanmax(p)))
    mesh = top.pcolormesh(t, x, p, cmap=WAVES, norm=TwoSlopeNorm(0.0, -edge, edge),
                          shading="nearest", rasterized=True)
    colorbar(fig, mesh, top, "pressure above 1 bar (Pa)")
    # where a wave from the closed end would be at the speed of sound: out to
    # the open end (and 0.3 diameters beyond it), back, and out again
    turn = length + END_CORRECTION
    legs = [(0.0, 0.0, turn / C_300K * 1e3, turn), (turn / C_300K * 1e3, turn, 2 * turn / C_300K * 1e3, 0.0),
            (2 * turn / C_300K * 1e3, 0.0, 3 * turn / C_300K * 1e3, turn)]
    for t0, x0, t1, x1 in legs:
        top.plot([t0, t1], [x0, x1], color=INK_2, linewidth=0.8, linestyle=DASH)
    top.set_xlim(t[0], t[-1])
    top.set_ylim(0.0, length)
    top.set_ylabel("along the pipe (m)")
    top.set_xticklabels([])
    label(top, "closed end", (t[-1], 0.0), (-4, 7), ha="right", color=INK_2)
    label(top, "open end", (t[-1], length), (-4, -7), ha="right", color=INK_2)
    mid = 0.5 * turn / C_300K * 1e3
    label(top, "the speed of sound", (mid, 0.5 * turn), (6, -4), ha="left", color=INK_2)

    for where, colour in ((15, BLUE), (47, ORANGE)):
        low.plot(t, p[where], color=colour, linewidth=1.6)
        dip = int(np.argmin(p[where]))   # labelled under its first dip
        label(low, f"{x[where]:.2f} m along", (t[dip], p[where][dip]), (0, -10), ha="center",
              va="top", color=colour)
    gone = (x[47] - x[15]) / C_300K * 1e3
    t15 = t[np.argmax(p[15])]
    high = 1.3 * p[15].max()
    low.annotate("", (t15 + gone, high), (t15, high),
                 arrowprops=dict(arrowstyle="->", color=INK_2, linewidth=0.8))
    label(low, f"1 m at the speed of sound: {gone:.2f} ms", (t15 + gone, high), (6, 0))
    low.set_ylim(-1.6 * p[15].max(), 1.5 * p[15].max())
    low.axhline(0.0, color=AXIS, linewidth=0.8)
    low.set_xlim(t[0], t[-1])
    low.set_xlabel("time (ms)")
    low.set_ylabel("pressure above 1 bar (Pa)")
    low.yaxis.set_major_formatter(plt.FuncFormatter(lambda v, _: f"{v:g}".replace("-", MINUS)))

    titles(fig, "A pressure pulse in a pipe",
           "2 m of still air at 300 K, 40 mm across, in 64 cells: closed at one end, open at the other.\n"
           "The pulse runs at the speed of sound, comes back from the closed end as it went and\n"
           "from the open end turned upside down, as if the pipe were 0.3 diameters longer.",
           top=0.955, left=0.085)
    save(fig, args.out or "renders/pulse.png")


def runs(xs, flags):
    """The stretches of xs (sorted) where flags is true, as (first, last) pairs."""
    out, first = [], None
    for x, f in zip(xs, flags):
        if f and first is None:
            first = x
        if not f and first is not None:
            out.append((first, prev))
            first = None
        prev = x
    if first is not None:
        out.append((first, prev))
    return out


def engine(args):
    d = read_csv(args.csv[0], ENGINE_COLUMNS, "plot_pipes")
    pipe, cell = d["pipe"], d["cell"]
    has_runner = any(v == 0 for v in pipe)

    fig = plt.figure(figsize=(7.5, 8.6), facecolor=SURFACE)
    rows = [(0, "runner", "from the plenum (m)", 0.645), (1, "header", "from the valve (m)", 0.425)]
    for j, name, ylabel, y0 in rows[0 if has_runner else 1:]:
        ax = fig.add_axes([0.085, y0, 0.80, 0.185])
        style(ax)
        ax.grid(False)
        a, xs, p = grid(d, "crank_deg", "x_m", "p_kPa", lambda i, j=j: pipe[i] == j)
        edge = max(abs(np.nanmin(p) - P_AMBIENT), abs(np.nanmax(p) - P_AMBIENT))
        mesh = ax.pcolormesh(a, xs, p - P_AMBIENT, cmap=WAVES, norm=TwoSlopeNorm(0.0, -edge, edge),
                             shading="nearest", rasterized=True)
        colorbar(fig, mesh, ax, "kPa above\nthe air outside")
        crank_axis(ax, -360, 360)
        ax.set_xlabel("")
        ax.set_xticklabels([])
        ax.set_ylabel(ylabel)
        ax.set_ylim(0.0, xs[-1] + 0.5 * (xs[1] - xs[0]))
        label(ax, f"cylinder 1's {name}", (-360, ax.get_ylim()[1]), (4, -9), color="#0b0b0b", size=9)

    # the pressures either side of the valves, and when they're open
    ax = fig.add_axes([0.085, 0.075, 0.80, 0.29])
    style(ax)
    angles = sorted(set(d["crank_deg"]))

    def series(j, k):
        at = {d["crank_deg"][i]: d["p_kPa"][i] for i in range(len(pipe)) if pipe[i] == j and cell[i] == k}
        return [at[a] for a in angles]

    lift = {d["crank_deg"][i]: (d["intake_lift_mm"][i], d["exhaust_lift_mm"][i])
            for i in range(len(pipe)) if pipe[i] == -1 and cell[i] == 0}
    lines = [("cylinder", INK_2, series(-1, 0), 1.2), ("header at the exhaust valve", ORANGE, series(1, 0), 1.6)]
    if has_runner:
        last = max(int(cell[i]) for i in range(len(pipe)) if pipe[i] == 0)
        lines.append(("runner at the intake valve", BLUE, series(0, last), 1.6))
    for _, colour, values, width in lines:
        ax.plot(angles, values, color=colour, linewidth=width)
    hi = 250.0
    for k, (name, colour) in enumerate((("intake valve open", BLUE), ("exhaust valve open", ORANGE))):
        y = hi - 7.0 - 9.0 * k
        stretches = runs(angles, [lift[a][k] > 0.0 for a in angles])
        for first, last in stretches:
            ax.plot([first, last], [y, y], color=colour, linewidth=3.0, solid_capstyle="butt", alpha=0.6)
        first, last = max(stretches, key=lambda r: r[1] - r[0])   # labelled beside the longest
        if last < 200.0:
            label(ax, name, (last, y), (5, 0), color=colour, size=8)
        else:
            label(ax, name, (first, y), (-5, 0), ha="right", color=colour, size=8)
    for k, (name, colour, _, _) in enumerate(lines):
        label(ax, name, (-350, 205 - 15 * k), (0, 0), color=colour)
    ax.axhline(P_AMBIENT, color=MUTED, linewidth=0.8, linestyle=DASH)
    crank_axis(ax, -360, 360)
    ax.set_ylim(0.0, hi)
    ax.set_ylabel("pressure (kPa)")

    rpm = args.rpm or "3000"
    titles(fig, "Pressure waves in the runner and the header",
           f"The 2.0 L four held at {rpm} rpm, wide open: one cycle of cylinder 1's pipes, cell by cell.\n"
           "Each valve opening sends a wave down its pipe; it comes back from the far end (the plenum,\n"
           "the collector) turned over, and its timing decides how full the cylinder gets.",
           top=0.955, left=0.085)
    save(fig, args.out or "renders/pipes.png")


def compare(args):
    before = read_csv(args.csv[0], SWEEP_COLUMNS, "plot_pipes")
    after = read_csv(args.csv[1], SWEEP_COLUMNS, "plot_pipes")
    fig = plt.figure(figsize=(7.5, 6.2), facecolor=SURFACE)
    top = fig.add_axes([0.085, 0.47, 0.72, 0.33])
    low = fig.add_axes([0.085, 0.08, 0.72, 0.30])
    for ax, col, name in ((top, "ve", "volumetric efficiency"), (low, "brake_torque_Nm", "brake torque (N m)")):
        style(ax)
        for data, colour, text in ((before, ORANGE, "stage 9: lumped"), (after, BLUE, "stage 10: pipes")):
            ax.plot(data["rpm"], data[col], color=colour, linewidth=1.8)
            label(ax, text, (data["rpm"][-1], data[col][-1]), (6, 0), color=colour)
        ax.set_ylabel(name)
        ax.set_xlim(min(after["rpm"]) - 200, max(after["rpm"]) + 200)
    top.set_xticklabels([])
    low.set_xlabel("engine speed (rpm)")
    titles(fig, "The pipes tune the breathing",
           "The 2.0 L four wide open, held at each speed. Stage 9 had a column of air in each runner\n"
           "and a lumped exhaust. Stage 10's waves fill the cylinders as well from 4000 to 5500 rpm,\n"
           "less below (the waves arrive out of step) and above (the exhaust's back pressure).",
           top=0.955, left=0.085)
    save(fig, args.out or "renders/pipes-ve.png")


def save(fig, out):
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    fig.savefig(out, dpi=150, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("chart", choices=("pulse", "engine", "compare"))
    parser.add_argument("csv", nargs="+")
    parser.add_argument("-o", "--out")
    parser.add_argument("--rpm", help="the engine's speed, for the engine chart's subtitle")
    args = parser.parse_args()
    if args.chart == "compare" and len(args.csv) != 2:
        parser.error("compare needs two CSVs: before and after")
    setup()
    {"pulse": pulse, "engine": engine, "compare": compare}[args.chart](args)


if __name__ == "__main__":
    main()
