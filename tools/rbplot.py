"""
rbplot.py - the shared look of the REV-BENCH charts: colours, fonts, axes,
and reading the CSV files the native tools write.
"""
import csv
import sys

import matplotlib

matplotlib.use("Agg")  # no window: the scripts only write PNGs
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib import font_manager  # noqa: E402
from matplotlib.ticker import FixedLocator  # noqa: E402

SURFACE = "#fcfcfb"   # background
INK = "#0b0b0b"       # titles
INK_2 = "#52514e"     # axis titles, labels and annotations
MUTED = "#898781"     # tick labels and reference lines
GRID = "#e1e0d9"
AXIS = "#c3c2b7"
BLUE = "#2a78d6"      # first series (and the simulation when there is one)
ORANGE = "#eb6834"    # second series
# Ordered steps of one blue, light to dark, for ordered categories such as
# throttle openings (checked: each step distinct, the lightest still visible).
BLUE_RAMP = ["#86b6ef", "#3987e5", "#1c5cab", "#0d366b"]
DASH = (0, (4, 3))
MINUS = "−"


def read_csv(path, need, tool):
    """The CSV as a dict of columns (lists of floats); exits if columns are missing."""
    try:
        with open(path, newline="") as f:
            rows = list(csv.DictReader(f))
    except OSError as e:
        sys.exit(f"{tool}: cannot read {path}: {e.strerror}")
    if not rows or any(c not in rows[0] for c in need):
        sys.exit(f"{tool}: {path} needs the columns {', '.join(need)}")
    return {c: [float(r[c]) for r in rows] for c in rows[0]}


def setup():
    """Fonts and defaults for every chart: Inter if it's installed."""
    installed = {f.name for f in font_manager.fontManager.ttflist}
    plt.rcParams["font.family"] = "Inter" if "Inter" in installed else "DejaVu Sans"
    plt.rcParams["axes.labelcolor"] = INK_2
    plt.rcParams["axes.labelsize"] = 9


def style(ax):
    """Light grid, no box, quiet ticks."""
    ax.set_facecolor(SURFACE)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(AXIS)
    ax.tick_params(which="both", colors=AXIS, labelcolor=MUTED, labelsize=8.5, length=3)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)


def label(ax, text, xy, offset, ha="left", va="center", color=INK_2, size=8.5):
    """A direct label: text placed `offset` points away from the data point `xy`."""
    ax.annotate(text, xy, xytext=offset, textcoords="offset points",
                ha=ha, va=va, fontsize=size, color=color)


def titles(fig, title, subtitle, top=0.95, left=0.075):
    """Bold title and a grey subtitle (which may have line breaks) at the top left."""
    h = fig.get_figheight()          # inches
    line = 13.0 / 72.0 / h           # one 9-point line with its spacing, as a figure fraction
    fig.text(left, top, title, fontsize=12, fontweight="bold", color=INK)
    lines = subtitle.count("\n") + 1
    # The y position is the baseline of the subtitle's last line.
    fig.text(left, top - 0.2 / h - line * (lines - 1), subtitle, fontsize=9, color=INK_2,
             linespacing=1.45)


def crank_axis(ax, lo, hi):
    """Crank angle ticks every 180 degrees, named TDC and BDC."""
    ax.set_xlim(lo, hi)
    ticks = [t for t in (-360, -180, 0, 180, 360) if lo <= t <= hi]
    ax.xaxis.set_major_locator(FixedLocator(ticks))
    ax.set_xticklabels([f"{t:g}".replace("-", MINUS) + ("\nTDC" if t % 360 == 0 else "\nBDC")
                        for t in ticks])
    ax.set_xlabel("crank angle (degrees)")
