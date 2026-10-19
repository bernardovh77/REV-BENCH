#!/usr/bin/env python3
"""
plot_dyno.py - charts of the dyno and the calibration (stage 9):

    python3 tools/plot_dyno.py curves renders/dyno-i4.csv renders/dyno-i6.csv ...
        each preset's sweep on the absorbing dyno (rb_cli dyno --preset KEY
        --csv renders/dyno-KEY.csv): brake torque and power against speed,
        with the real engine's published peaks where there is one
                                                    -> renders/dyno.png
    python3 tools/plot_dyno.py changes STOCK.csv SMALL.csv BIG.csv LOW.csv HIGH.csv
        the four's torque with smaller and bigger cams and with lower and
        higher compression (rb_cli fire --rpm-sweep, with --intake/--exhaust
        or --cr; the stock engine's sweep first)   -> renders/changes.png
    python3 tools/plot_dyno.py spark WOT.csv PART.csv LIGHT.csv
        the burn's length and the spark for the best torque against speed at
        three throttle openings (rb_cli fire --rpm-sweep --throttle X; the
        openings are read from the CSVs)           -> renders/spark-map.png

-o FILE writes the chart somewhere else. Needs matplotlib (sudo apt install
python3-matplotlib).
"""
import argparse
import os
import sys

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

from rbplot import (BLUE, BLUE_RAMP, INK_2, ORANGE, SURFACE, label, read_csv, setup, style,
                    titles)

DYNO_COLUMNS = ("rpm", "throttle", "brake_torque_Nm", "brake_power_kW")
SWEEP_COLUMNS = ("rpm", "throttle", "spark_deg", "brake_torque_Nm", "burn_deg")
# The presets by key, and the real engines three of them stand for, from the
# makers' figures (as in tests/test_dyno.c): name, peak brake torque (N m) and
# its rpm, peak power (kW) and its rpm.
PRESETS = {"i4": "2.0 L inline-4", "i6": "3.0 L inline-6", "v8-cross": "5.0 L cross-plane V8",
           "v8-flat": "4.5 L flat-plane V8", "boxer4": "2.0 L flat-4 (boxer)"}
REAL = {"i4": ("Toyota 1AZ-FE", 190.0, 4000.0, 108.0, 6000.0),
        "i6": ("BMW M54B30", 300.0, 3500.0, 170.0, 5900.0),
        "boxer4": ("Subaru FB20", 196.0, 4200.0, 110.0, 6200.0)}
TOLERANCE = 0.15   # stage 9's "done when": within 15 % of the real peaks


def preset_of(path):
    """The preset key in a file name such as renders/dyno-i6.csv."""
    stem = os.path.splitext(os.path.basename(path))[0]
    key = stem.split("-", 1)[1] if "-" in stem else stem
    if key not in PRESETS:
        sys.exit(f"plot_dyno: can't tell the preset from {path} (want dyno-KEY.csv, KEY one of "
                 f"{', '.join(PRESETS)})")
    return key


def end_labels(ax, ends, gap=11.0):
    """Labels at the right ends of lines, [(text, x, y)], pushed apart vertically
    so that no two are closer than `gap` points, and kept centred on the lines."""
    pt = ax.figure.dpi / 72.0
    placed = sorted((ax.transData.transform((x, y))[1], text, x, y) for text, x, y in ends)
    ys = [p[0] for p in placed]
    for k in range(1, len(ys)):
        ys[k] = max(ys[k], ys[k - 1] + gap * pt)
    shift = (sum(ys) - sum(p[0] for p in placed)) / len(ys)   # the same for all: gaps stay
    for (y_px, text, x, y), y_new in zip(placed, ys):
        label(ax, text, (x, y), (6, (y_new - shift - y_px) / pt), color=INK_2, size=8.5)


def rpm_axis(ax, rpm):
    """Speed ticks every 1000 rpm over the data, with room on the right for labels."""
    top = max(rpm)
    ax.set_xlim(500, top + 1900)
    ax.set_xticks([t for t in range(1000, int(top) + 1, 1000)])
    ax.set_xlabel("engine speed (rpm)")


def peak(x, y):
    i = max(range(len(y)), key=y.__getitem__)
    return x[i], y[i]


def plot_curves(paths, out):
    keys = [preset_of(p) for p in paths]
    runs = [read_csv(p, DYNO_COLUMNS, "plot_dyno") for p in paths]
    for k, d in zip(keys, runs):
        if any(abs(t - 1.0) > 1e-9 for t in d["throttle"]):
            sys.exit(f"plot_dyno: the {k} sweep isn't wide open")

    setup()
    n = len(paths)
    fig = plt.figure(figsize=(3.2 * n + 0.6, 5.9), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(2, n, left=0.07 if n > 2 else 0.1, right=0.98, top=0.72, bottom=0.09,
                          wspace=0.28, hspace=0.42)
    for col, (key, d) in enumerate(zip(keys, runs)):
        rpm = d["rpm"]
        real = REAL.get(key)
        for row, (column, unit, name) in enumerate((("brake_torque_Nm", "N m", "torque"),
                                                     ("brake_power_kW", "kW", "power"))):
            ax = fig.add_subplot(gs[row, col])
            style(ax)
            y = d[column]
            ax.plot(rpm, y, color=BLUE, lw=2, zorder=3)
            x_pk, y_pk = peak(rpm, y)
            ax.scatter([x_pk], [y_pk], s=30, color=BLUE, edgecolor=SURFACE, lw=1.5, zorder=4)
            top = max(y)
            # The peaks, written in the panel's empty top left corner rather
            # than beside the marks, where they would collide.
            lines = [f"model    {y_pk:.0f} {unit} at {x_pk:.0f} rpm"]
            if real:
                ref, ref_rpm = (real[1], real[2]) if row == 0 else (real[3], real[4])
                ax.errorbar([ref_rpm], [ref], yerr=[[TOLERANCE * ref], [TOLERANCE * ref]],
                            fmt="D", ms=6, color=ORANGE, mec=SURFACE, mew=1.2, ecolor=ORANGE,
                            elinewidth=1.2, capsize=3, zorder=5)
                top = max(top, (1 + TOLERANCE) * ref)
                lines.append(f"real       {ref:.0f} {unit} at {ref_rpm:.0f} rpm")
            for k, text in enumerate(lines):
                ax.text(0.03, 0.96 - 0.1 * k, text, transform=ax.transAxes, ha="left", va="top",
                        fontsize=8, color=INK_2)
            ax.set_xlim(1000, max(rpm) + 500)
            ax.set_ylim(0, top * 1.35)
            if row == 0:
                heading = PRESETS[key] + (f"\nvs {real[0]}" if real else "\n(no real engine to match)")
                ax.set_title(heading, fontsize=9.5, color=INK_2, loc="left", pad=8)
            else:
                ax.set_xlabel("engine speed (rpm)")
            if col == 0:
                ax.set_ylabel(f"brake {name} ({unit})")

    handles = [Line2D([], [], color=BLUE, lw=2, label="REV-BENCH, on the dyno"),
               Line2D([], [], color=ORANGE, marker="D", ms=6, lw=0, mec=SURFACE,
                      label=f"the real engine's peak, ±{TOLERANCE:.0%}")]
    left = 0.07 if n > 2 else 0.1
    fig.legend(handles=handles, loc="lower left", bbox_to_anchor=(left - 0.01, 0.815), ncol=2,
               frameon=False, fontsize=8.5, labelcolor=INK_2)
    titles(fig, "On the dyno: the presets against the real engines",
           "Wide open, swept from 1500 rpm at 100 rpm/s by the absorbing dyno, with the spark "
           "where the torque is best.\nThe real engines' published peaks, with stage 9's ±15 % "
           "margin: the model's come out within 5 %.",
           top=0.955, left=left)
    fig.savefig(out, facecolor=SURFACE)
    for key, d in zip(keys, runs):
        (tq_rpm, tq), (pw_rpm, pw) = peak(d["rpm"], d["brake_torque_Nm"]), peak(d["rpm"], d["brake_power_kW"])
        print(f"{key}: {tq:.1f} N m at {tq_rpm:.0f} rpm, {pw:.1f} kW at {pw_rpm:.0f} rpm")
    print(f"wrote {out}")


def sweep(path):
    """A held sweep from rb_cli fire --rpm-sweep."""
    return read_csv(path, SWEEP_COLUMNS, "plot_dyno")


def curve_panel(ax, runs, names, heading):
    """Torque curves for ordered settings, light to dark, labelled at their right ends."""
    style(ax)
    ramp = BLUE_RAMP[:1] + BLUE_RAMP[2:] if len(runs) == 3 else BLUE_RAMP
    top = 0.0
    for k, d in enumerate(runs):
        colour = ramp[k]
        ax.plot(d["rpm"], d["brake_torque_Nm"], color=colour, lw=2, marker="o", ms=3, zorder=3)
        x_pk, y_pk = peak(d["rpm"], d["brake_torque_Nm"])
        ax.scatter([x_pk], [y_pk], s=36, color=colour, edgecolor=SURFACE, lw=1.5, zorder=4)
        top = max(top, y_pk)
    ax.set_title(heading, fontsize=9.5, color=INK_2, loc="left", pad=8)
    rpm_axis(ax, runs[0]["rpm"])
    ax.set_ylim(100, top * 1.08)
    return [(name, d["rpm"][-1], d["brake_torque_Nm"][-1]) for d, name in zip(runs, names)]


def plot_changes(paths, out):
    if len(paths) != 5:
        sys.exit("plot_dyno: changes takes five sweeps: stock, small cams, big cams, "
                 "low compression, high compression")
    stock, small, big, low, high = (sweep(p) for p in paths)
    for d in (stock, small, big, low, high):
        if any(abs(t - 1.0) > 1e-9 for t in d["throttle"]):
            sys.exit("plot_dyno: the changes sweeps must be wide open")
    setup()
    fig = plt.figure(figsize=(8.6, 4.2), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(1, 2, left=0.075, right=0.975, top=0.70, bottom=0.13, wspace=0.18)
    ax_c, ax_r = fig.add_subplot(gs[0, 0]), fig.add_subplot(gs[0, 1])
    ends_c = curve_panel(ax_c, (small, stock, big), ("230°, 8.5 mm", "250°, 9.5 mm (stock)",
                                                    "280°, 10.5 mm"), "Cams: duration and lift")
    ends_r = curve_panel(ax_r, (low, stock, high), ("9:1", "10:1 (stock)", "12:1"), "Compression ratio")
    ax_c.set_ylabel("brake torque (N m)")
    ax_r.sharey(ax_c)
    ax_r.tick_params(labelleft=False)
    end_labels(ax_c, ends_c)
    end_labels(ax_r, ends_r)
    peaks = [peak(d["rpm"], d["brake_torque_Nm"]) for d in (small, stock, big)]
    gain = peak(high["rpm"], high["brake_torque_Nm"])[1] / peaks[1][1] - 1
    titles(fig, "Changes go the way they do on real engines",
           f"The 2.0 L four wide open, held at each speed. Bigger cams move the torque up the rev "
           f"range: its peak goes from\n{peaks[0][0]:.0f} to {peaks[1][0]:.0f} to "
           f"{peaks[2][0]:.0f} rpm, and the low end falls. More compression is more torque "
           f"everywhere: {gain:+.0%} from 10:1 to 12:1.", top=0.94)
    fig.savefig(out, facecolor=SURFACE)
    print(f"cam peaks: {', '.join(f'{y:.1f} N m at {x:.0f}' for x, y in peaks)}; 12:1 {gain:+.1%}")
    print(f"wrote {out}")


def plot_spark(paths, out):
    runs = sorted((sweep(p) for p in paths), key=lambda d: -d["throttle"][0])
    setup()
    fig = plt.figure(figsize=(8.6, 4.0), dpi=135, facecolor=SURFACE)
    gs = fig.add_gridspec(1, 2, left=0.075, right=0.975, top=0.78, bottom=0.13, wspace=0.25)
    ax_b, ax_s = fig.add_subplot(gs[0, 0]), fig.add_subplot(gs[0, 1])
    ramp = list(reversed(BLUE_RAMP[:1] + BLUE_RAMP[2:])) if len(runs) == 3 else list(reversed(BLUE_RAMP))
    for ax, column, name in ((ax_b, "burn_deg", "burn, spark to end (crank degrees)"),
                             (ax_s, "spark_deg", "spark advance (degrees before TDC)")):
        style(ax)
        ends = []
        for k, d in enumerate(runs):
            ax.plot(d["rpm"], d[column], color=ramp[k], lw=2, marker="o", ms=3, zorder=3)
            opening = d["throttle"][0]
            text = "wide open" if opening >= 1.0 else f"{opening:.0%} throttle"
            ends.append((text, d["rpm"][-1], d[column][-1]))
        rpm_axis(ax, runs[0]["rpm"])
        ax.set_ylabel(name)
        lo = min(min(d[column]) for d in runs)
        ax.set_ylim(lo * 0.85, max(max(d[column]) for d in runs) * 1.08)
        end_labels(ax, ends)
    wot = runs[0]
    titles(fig, "The engine computer's spark",
           "The 2.0 L four, held at each speed. The burn takes more crank angle as the speed rises and "
           "as the load falls, and the\ncomputer advances the spark to match, keeping the middle of "
           f"the burn 8° after TDC: wide open, {wot['spark_deg'][0]:.0f}° at "
           f"{wot['rpm'][0]:.0f} rpm, {wot['spark_deg'][-1]:.0f}° at {wot['rpm'][-1]:.0f}.", top=0.94)
    fig.savefig(out, facecolor=SURFACE)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("what", choices=("curves", "changes", "spark"))
    ap.add_argument("csv", nargs="+")
    ap.add_argument("-o", "--out", help="output PNG")
    args = ap.parse_args()
    if args.what == "curves":
        plot_curves(args.csv, args.out or "renders/dyno.png")
    elif args.what == "changes":
        plot_changes(args.csv, args.out or "renders/changes.png")
    else:
        plot_spark(args.csv, args.out or "renders/spark-map.png")


if __name__ == "__main__":
    main()
