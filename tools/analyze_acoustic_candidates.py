#!/usr/bin/env python3
"""Validate, plot and publish the initial S2d candidate measurements.

Numerical harness checks and architecture-promotion gates are deliberately
different. An acoustically inaccurate but conservative candidate is a result,
not a reason to conceal its rows or change the measurement tolerance.
"""
import argparse
from checkpoint_sections import publish_section
import base64
import cmath
import csv
import hashlib
from html import escape
import json
import math
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / "renders/evidence/acoustic-candidates-2026-10-23"
BEGIN = "<!-- ACOUSTIC CANDIDATES BEGIN -->"
END = "<!-- ACOUSTIC CANDIDATES END -->"
STRINGS = {"case", "group", "kind", "status", "interpolation"}


def read(path):
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise ValueError(f"empty evidence: {path}")
    for row in rows:
        if None in row or any(v is None for v in row.values()):
            raise ValueError(f"malformed evidence: {path}")
        for k, value in row.items():
            if k not in STRINGS:
                row[k] = float(value)
                if not math.isfinite(row[k]):
                    raise ValueError(f"nonfinite evidence: {path}:{k}")
        if row["status"] not in ("ok", "excluded", "failed"):
            raise ValueError(f"unknown status: {row['status']}")
    return rows


def magnitude_db(row):
    return 20*math.log10(max(row["carrier_magnitude"], 1e-30))


def accuracy(row):
    return abs(magnitude_db(row)) <= 1 and abs(row["carrier_phase_error_deg"]) <= 10


def analytic_line(row, frequency=None):
    """Independent z-domain reference; no copy of the C state update."""
    frequency = row["frequency_Hz"] if frequency is None else frequency
    delay = row["rate_Hz"]*row["length_m"]/(row["sound_m_s"]*row["segments"])
    integer = math.floor(delay)
    fraction = delay-integer
    z = cmath.exp(-2j*math.pi*frequency/row["rate_Hz"])
    if row["interpolation"] == "linear":
        interpolation = 1-fraction+fraction*z
    elif fraction == 0:
        interpolation = 1
    else:
        a = (1-fraction)/(1+fraction)
        interpolation = (a+z)/(1+a*z)
    return (z**integer*interpolation)**int(row["segments"])


def verify_transfer(rows):
    rows = [r for r in rows if r["kind"] == "B0-transfer" and r["status"] == "ok"]
    error = 0
    configurations = {}
    for r in rows:
        phase = math.radians(r["carrier_phase_error_deg"])-2*math.pi*r["frequency_Hz"]*r["length_m"]/r["sound_m_s"]
        measured = r["carrier_magnitude"]*cmath.exp(1j*phase)
        error = max(error, abs(measured-analytic_line(r)))
        if r["interpolation"] == "allpass":
            configurations[(r["rate_Hz"], r["temperature_K"], r["segments"])] = r
    worst_phase = worst_magnitude = 0
    for r in configurations.values():
        for i in range(1025):
            frequency = 200+3800*i/1024
            h = analytic_line(r, frequency)
            compensated = h*cmath.exp(2j*math.pi*frequency*r["length_m"]/r["sound_m_s"])
            worst_phase = max(worst_phase, abs(math.degrees(cmath.phase(compensated))))
            worst_magnitude = max(worst_magnitude, abs(20*math.log10(abs(h))))
    return dict(compiled_transfer_max_complex_error=error,
                compiled_matches_independent_reference=bool(rows) and error < 2e-6,
                reference_uses_rounded_csv_sound_speed=True,
                analytic_configurations=len(configurations), dense_analytic_points_per_configuration=1025,
                dense_analytic_max_phase_error_deg=worst_phase,
                dense_analytic_max_magnitude_error_dB=worst_magnitude,
                scope="Compiled responses checked at recorded frequencies; dense sweep is an analytic static-line reference, not additional engine measurements")


def summarize(fv, wave, topology, manifest=None):
    good = [r for r in fv if r["status"] == "ok"]
    transfer = [r for r in wave if r["kind"] == "B0-transfer" and r["status"] == "ok"]
    loops = [r for r in wave if r["kind"] == "B0-loop" and r["status"] == "ok"]
    allpass_band = [r for r in transfer if r["interpolation"] == "allpass" and r["frequency_Hz"] <= 4000]
    if not good or not allpass_band or not loops:
        raise ValueError("required candidate families missing")
    conservation = {k: max(abs(r[k]) for r in good) for k in (
        "mass_relative_error", "momentum_relative_error", "total_energy_relative_error", "tag_relative_error")}
    allpass_loops = [r for r in loops if r["interpolation"] == "allpass"]
    if not allpass_loops:
        raise ValueError("all-pass closed-loop controls missing")
    checks = {
        "unique_case_ids": len({r["case"] for r in fv+wave}) == len(fv+wave),
        "no_failed_solver_rows": all(r["status"] != "failed" for r in fv+wave),
        "no_candidate_state_failures": all(r["failures"] == 0 for r in good+transfer+loops),
        "fv_conserved_quantities": max(conservation.values()) < 5e-10,
        "wave_ledger": max(abs(r["relative_ledger_error"]) for r in transfer+loops) < 1e-8,
        "allpass_source_free_energy": all(abs(r["energy_ratio"]-1) < 1e-8 and
                                            r["max_energy_ratio"] <= 1+1e-8 for r in allpass_loops),
        "allpass_flat_magnitude": max(abs(magnitude_db(r)) for r in allpass_band) < .01,
        "five_named_topologies": {e["preset"] for e in topology["engines"]} ==
            {"i4", "i6", "v8-cross", "v8-flat", "boxer4"} and len(topology["engines"]) == 5
            and topology["unknown_connections"] == 0,
    }
    if manifest is not None:
        expected = {r["case"] for r in manifest["records"]}
        checks["complete_recorded_matrix"] = (len(expected) == len(manifest["records"])
            and expected == {r["case"] for r in fv+wave})
    matrix = [r for r in good if r["group"] == "matrix"]
    by_grid = []
    for dx in sorted({r["dx_m"] for r in matrix}, reverse=True):
        selected = [r for r in matrix if abs(r["dx_m"]-dx) < 1e-12]
        phase_rows = [r for r in selected if r["carrier_magnitude"] >= .1]
        by_grid.append(dict(dx_m=dx, cases=len(selected), passing=sum(accuracy(r) for r in selected),
                            worst_magnitude_error_dB=max(abs(magnitude_db(r)) for r in selected),
                            worst_phase_error_deg=max((abs(r["carrier_phase_error_deg"]) for r in phase_rows), default=None),
                            phase_rows_below_ten_percent_carrier=len(selected)-len(phase_rows)))
    gates = dict(A1_matrix_at_each_grid=by_grid,
                 B0_quiescent_1m_200_4000_Hz=all(accuracy(r) for r in allpass_band),
                 B0_max_magnitude_error_dB=max(abs(magnitude_db(r)) for r in allpass_band),
                 B0_max_phase_error_deg=max(abs(r["carrier_phase_error_deg"]) for r in allpass_band),
                 mean_flow_and_engine_feedback_validated=False,
                 real_engine_realism_accepted=False,
                 real_time_engine_or_browser_accepted=False)
    return dict(harness_checks=checks, harness_pass=all(checks.values()), promotion_gates=gates,
                fv_cases=len(fv), fv_exclusions=sum(r["status"] == "excluded" for r in fv),
                wave_cases=len(wave), max_conservation_error=conservation,
                fv_fallback_totals={k: sum(r[k] for r in good) for k in ("reconstruction_fallbacks", "flux_fallbacks")},
                closed_loop_controls=loops,
                cold_2k_fv=[r for r in matrix if r["temperature_K"] == 300 and r["mean_flow_m_s"] == 0 and r["frequency_Hz"] == 2000],
                topology=[dict(preset=e["preset"], pipes=len(e["pipes"]),
                               cells=sum(p["cells"] for p in e["pipes"]), rev_limit=e["rev_limit"])
                          for e in topology["engines"]],
                limitations=["A1 uses constant gamma frozen at each gas state; no engine EOS/valve adapter",
                             "B0 band gate is homogeneous zero-flow plane-mode numerics, not mean-flow or full exhaust validation",
                             "A1 packet travel is (c+U)*2.5ms, not exactly 1m; B0 transfer uses 1m",
                             "higher-frequency B0 rows test a mathematical plane-mode line, not transverse-mode completeness",
                             "source-free B0 loops are linear graph controls, not reproduction of the nonlinear hot-flow failure",
                             "fixture timing is not full-engine speedup or a callback deadline test"])


def plots(directory, fv, wave, a0):
    os.environ.setdefault("MPLCONFIGDIR", "/tmp/rev-bench-mpl")
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    plt.style.use("dark_background")
    paths = []
    fig, axes = plt.subplots(1, 2, figsize=(11, 4), layout="constrained")
    for label, rows, color in (("A0 current staggered", a0, "#6ab7e8"), ("A1 conservative HLLC/MC", fv, "#eda66d")):
        selected = sorted([r for r in rows if r["status"] == "ok" and r["group"] == "matrix"
                           and r["temperature_K"] == 300 and r["mean_flow_m_s"] == 0
                           and r["frequency_Hz"] == 2000 and r["rate_Hz"] == 192000], key=lambda r:r["dx_m"])
        x = [r["dx_m"]*1000 for r in selected]
        axes[0].plot(x, [100*r["energy_ratio"] for r in selected], "o-", color=color, label=label)
        phase_rows = [r for r in selected if r["carrier_magnitude"] >= .1]
        axes[1].plot([r["dx_m"]*1000 for r in phase_rows],
                     [r["carrier_phase_error_deg"] for r in phase_rows], "o-", color=color, label=label)
    axes[0].axhline(100, color="white", alpha=.3, ls="--")
    axes[1].axhspan(-10, 10, color="#75cb9a", alpha=.15)
    axes[0].set(ylabel="Acoustic perturbation energy retained (%)", ylim=(0, 110))
    axes[1].set(ylabel="Carrier phase error (degrees)")
    for ax in axes:
        ax.set(xlabel="Cell size (mm)", xscale="log", xticks=[9.375, 18.75, 37.5])
        ax.set_xticklabels(["9.375", "18.75", "37.5"]); ax.grid(alpha=.15)
    axes[0].legend(fontsize=8)
    fig.suptitle("Conservation alone does not solve acoustic transport\n300 K · zero flow · 2 kHz · 192 kHz · 2.5 ms; phase shown above 10% carrier")
    path = directory / "conservative-comparison.png"
    fig.savefig(path, dpi=160); plt.close(fig); paths.append(path)
    fig, axes = plt.subplots(1, 2, figsize=(11, 4), layout="constrained")
    colors = {1:"#6ab7e8", 4:"#91d1a2", 16:"#eda66d"}
    for interp, style in (("allpass", "-"), ("linear", "--")):
        for segments in (1, 4, 16):
            selected = sorted([r for r in wave if r["kind"] == "B0-transfer" and r["status"] == "ok"
                               and r["temperature_K"] == 300 and r["rate_Hz"] == 48000
                               and r["segments"] == segments and r["interpolation"] == interp], key=lambda r:r["frequency_Hz"])
            if not selected:
                continue
            x = [r["frequency_Hz"] for r in selected]
            label = f"{segments} segment(s) · {interp}"
            axes[0].semilogx(x, [magnitude_db(r) for r in selected], "o"+style, color=colors[segments], label=label)
            axes[1].semilogx(x, [r["carrier_phase_error_deg"] for r in selected], "o"+style, color=colors[segments])
    axes[0].axhspan(-1, 1, color="#75cb9a", alpha=.12)
    axes[1].axhspan(-10, 10, color="#75cb9a", alpha=.12)
    axes[0].set(ylabel="Magnitude error (dB)")
    axes[1].set(ylabel="Wrapped phase error (degrees)")
    for ax in axes:
        ax.axvline(4000, color="white", ls=":", alpha=.5)
        ax.set(xlabel="Frequency (Hz)", xticks=[200, 800, 2000, 4000, 12000, 16000])
        ax.set_xticklabels(["200", "800", "2k", "4k", "12k", "16k"]); ax.grid(alpha=.15)
    axes[0].legend(fontsize=7)
    fig.suptitle("B0: 1 m fixed-background line · 300 K · 48 kHz\nAbove 4 kHz: interpolation study, not a validated full exhaust band")
    path = directory / "network-transfer.png"
    fig.savefig(path, dpi=160); plt.close(fig); paths.append(path)
    return paths


def publish(target, summary, figures):
    html = target.read_text()
    if html.count(BEGIN) != html.count(END) or html.count(BEGIN) > 1:
        raise ValueError("incomplete/duplicate prior candidate section")
    if BEGIN in html:
        before, tail = html.split(BEGIN, 1)
        if END not in tail:
            raise ValueError("incomplete prior candidate section")
        html = before + tail.split(END, 1)[1].removeprefix("\n")
    if html.count("</main>") != 1:
        raise ValueError("results must have exactly one main closing tag")
    gates = summary["promotion_gates"]
    text = [BEGIN, '<section id="transport-candidates"><h2>Acoustic renderer prototypes · S2d</h2>',
            '<p>The first native conservative solver and acoustic graph are implemented. '
            'The accepted engine recordings and all long coasts remain above. '
            'These are isolated transport experiments; no new engine sound is being claimed.</p>',
            '<p>The conservative control preserves total quantities but still loses upper-frequency detail. '
            f'The quiescent acoustic graph has at most {gates["B0_max_magnitude_error_dB"]:.3g} dB magnitude error '
            f'and {gates["B0_max_phase_error_deg"]:.2f}° phase error across the tested 1 m / 200–4000 Hz cases. '
            'Moving hot gas, real exhaust components and reflected engine coupling remain unvalidated.</p>']
    for path in figures:
        data = base64.b64encode(path.read_bytes()).decode()
        text.append(f'<img style="width:100%;height:auto" alt="{escape(path.stem)}" src="data:image/png;base64,{data}">')
    text.append('<p>One-second source-free graph loops test energy growth. All-pass propagation preserves energy; '
                'linear interpolation dissipates it. This does not resolve the earlier nonlinear hot-flow failure. '
                'Candidate timings are fixture costs; the full engine has not been made faster by these prototypes.</p></section>')
    text.append(END)
    html = html.replace("</main>", "\n".join(text)+"\n</main>", 1)
    link = '<a href="#transport-candidates">Renderer prototypes</a>'
    if link not in html:
        html = html.replace("</nav>", link+"</nav>", 1)
    publish_section(target, 'transport-candidates', '\n'.join(text))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, default=DEFAULT)
    parser.add_argument("--plot", action="store_true")
    parser.add_argument("--publish", action="store_true")
    args = parser.parse_args()
    directory = args.evidence.resolve()
    fv, wave = read(directory/"fv.csv"), read(directory/"wave.csv")
    topology = json.loads((directory/"production-topology.json").read_text())
    manifest = json.loads((directory/"manifest.json").read_text())
    summary = summarize(fv, wave, topology, manifest)
    summary["independent_transfer_reference"] = verify_transfer(wave)
    summary["harness_checks"]["independent_transfer_response"] = summary["independent_transfer_reference"]["compiled_matches_independent_reference"]
    summary["harness_pass"] = all(summary["harness_checks"].values())
    summary["source_manifest_sha256"] = hashlib.sha256((directory/"manifest.json").read_bytes()).hexdigest()
    summary["csv_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                             for p in (directory/"fv.csv", directory/"wave.csv")}
    (directory/"summary.json").write_text(json.dumps(summary, indent=2)+"\n")
    if not summary["harness_pass"]:
        raise SystemExit("candidate harness failed; inspect summary.json before interpreting results")
    figures = []
    if args.plot or args.publish:
        a0 = read(directory/"a0-transport.csv") if (directory/"a0-transport.csv").exists() else []
        figures = plots(directory, fv, wave, a0)
    if args.publish:
        publish(ROOT/"renders/RESULTS.html", summary, figures)
    print(json.dumps({k: summary[k] for k in ("harness_pass", "fv_cases", "fv_exclusions", "wave_cases", "promotion_gates")}, indent=2))


if __name__ == "__main__":
    main()
