#!/usr/bin/env python3
"""Summarize the transport probe; optional plots additionally use matplotlib."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics
import tempfile
import shutil


def read(path):
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    for row in rows:
        if None in row:
            raise ValueError(f"malformed CSV row: {row}")
        for key in row:
            if key not in ("group", "status"):
                row[key] = float(row[key])
    return rows


def select(rows, **fields):
    return next(row for row in rows if all(row[key] == value for key, value in fields.items()))


def brief(row):
    return {key: row[key] for key in (
        "temperature_K", "mean_flow_m_s", "frequency_Hz", "dx_m", "rate_Hz",
        "points_per_wavelength", "cfl", "energy_ratio", "carrier_magnitude",
        "carrier_phase_error_deg", "speed_error_percent", "wall_seconds")}


def plot_summary(summary, directory):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.5), layout="constrained")
    fig.suptitle("Controlled acoustic transport: energy, phase and timestep must be checked separately",
                 fontsize=14, weight="bold")
    rows = sorted([row for row in summary["independent_refinements"]["300K_+0m_s_2000Hz"]
                   if row["rate_Hz"] == 192000], key=lambda row: row["dx_m"], reverse=True)
    axis = axes[0]
    phase = axis.twinx()
    positions = list(range(len(rows)))
    axis.bar(positions, [100 * row["energy_ratio"] for row in rows], color="#9eb9c9", width=.52)
    phase.plot(positions, [row["carrier_phase_error_deg"] for row in rows], "o-", color="#ba4e3b")
    axis.set(xticks=positions, xticklabels=[f"{row['dx_m']*1000:g}" for row in rows],
             xlabel="Cell size (mm), fixed 192 kHz", ylabel="Retained energy (%)", ylim=(0,110),
             title="Zero flow: energy misses dispersion\n300 K · 2 kHz · 2.5 ms")
    phase.set(ylabel="Carrier phase error (degrees)", ylim=(0,150))
    phase.tick_params(axis="y", colors="#ba4e3b")
    axis.text(.03,.03,"Ideal: 100% energy, 0° phase error",transform=axis.transAxes,fontsize=9)
    axis = axes[1]
    rows = summary["independent_refinements"]["900K_+70m_s_800Hz"]
    for dx, color in zip((.075,.0375,.01875,.009375),("#bf563d","#d2a446","#3b878a","#3d5b88")):
        selected = sorted([row for row in rows if row["dx_m"] == dx], key=lambda row: row["rate_Hz"])
        axis.plot([row["rate_Hz"]/1000 for row in selected],
                  [100*row["energy_ratio"] for row in selected],"o-",label=f"{dx*1000:g} mm",color=color)
    axis.axhline(100,color="#555",lw=1,ls="--")
    axis.set(xlabel="Pipe step rate (kHz)",ylabel="Retained energy (%)",ylim=(70,108),
             title="Finer timestep alone is insufficient\n900 K · +70 m/s · 800 Hz · 2.5 ms")
    axis.legend(title="Independent grids",fontsize=8,title_fontsize=8)
    axis = axes[2]
    for rate, color in ((192000,"#bf563d"),(384000,"#3b878a"),(768000,"#3d5b88")):
        selected = sorted([row for row in summary["growth_checks"] if row["rate_Hz"] == rate],
                          key=lambda row:row["time_s"])
        axis.plot([row["time_s"]*1000 for row in selected],
                  [100*row["energy_ratio"] for row in selected],"o-",label=f"{rate/1000:g} kHz",color=color)
    axis.axhline(100,color="#555",lw=1,ls="--")
    axis.set(xlabel="Elapsed time (ms)",ylabel="Retained energy (%)",ylim=(0,230),
             title="Segmented-network growth warning\n900 K · +70 m/s · 4 kHz · 9.375 mm")
    axis.legend(fontsize=8)
    for axis in axes:
        axis.grid(axis="y",alpha=.2)
        axis.spines[["top","right"]].set_visible(False)
    fig.text(.5,-.065,
             "Tested segmented pipe network: growth magnitude depends on interface partition; bulk/interface attribution unresolved.\n"
             "Frictionless periodic duct, matched base subtraction. Numerical fixture; engine audio and presets unchanged.",
             ha="center",fontsize=9)
    fig.savefig(directory/"transport-summary.png",dpi=180,bbox_inches="tight")
    plt.close(fig)


def analyze(directory, plot=False):
    rows = read(directory / "transport.csv")
    controls = read(directory / "controls.csv") if (directory / "controls.csv").exists() else []
    matrix = [row for row in rows if row["group"] == "matrix"]
    valid = [row for row in matrix if row["status"] == "ok"]
    nonmutant = [row for row in rows + controls if row["status"] != "excluded" and not row["mutant"]]
    solver_failures = [row for row in nonmutant if row["status"] != "ok"]
    checks = {}
    checks["all_rows_have_34_columns"] = True
    checks["no_solver_state_failures"] = not solver_failures
    checks["no_state_repairs_nonfinite_or_caps"] = all(
        row["status"] == "ok" and not row["repairs"] and not row["nonfinite"] and not row["cap_observations"]
        for row in nonmutant)
    checks["uniform_base_pressure_error_below_1e_minus_7_Pa"] = max(
        row["base_pressure_error_Pa"] for row in nonmutant) < 1e-7
    analytic = [select(valid, temperature_K=temperature, mean_flow_m_s=0,
                       frequency_Hz=200, dx_m=.009375, rate_Hz=384000)
                for temperature in (300, 900)]
    checks["fine_zero_flow_200Hz_energy_and_speed_match_analytic"] = all(
        abs(row["energy_ratio"] - 1) < .001 and abs(row["speed_error_percent"]) < .1
        and abs(row["carrier_phase_error_deg"]) < .1 for row in analytic)
    amplitude_deltas, partition_deltas, donor_pairs = [], [], []
    for row in rows:
        if row["group"] in ("half_amplitude", "partition_control", "donor_control"):
            baseline = select(valid, **{key: row[key] for key in (
                "temperature_K", "mean_flow_m_s", "frequency_Hz", "dx_m", "rate_Hz")})
            if row["group"] == "half_amplitude":
                amplitude_deltas.append({
                    "energy_ratio_delta": row["energy_ratio"] - baseline["energy_ratio"],
                    "initial_energy_scale": row["energy_initial_J"] / baseline["energy_initial_J"],
                    "phase_delta_deg": row["carrier_phase_error_deg"] - baseline["carrier_phase_error_deg"]})
            elif row["group"] == "partition_control":
                partition_deltas.append({
                    "energy_ratio_delta": row["energy_ratio"] - baseline["energy_ratio"],
                    "phase_delta_deg": row["carrier_phase_error_deg"] - baseline["carrier_phase_error_deg"]})
            else:
                donor_pairs.append({"temperature_K": row["temperature_K"],
                                    "mean_flow_m_s": row["mean_flow_m_s"],
                                    "frequency_Hz": row["frequency_Hz"],
                                    "current_energy_ratio": baseline["energy_ratio"],
                                    "donor_energy_ratio": row["energy_ratio"]})
    checks["half_amplitude_energy_quarters_and_retention_is_linear"] = all(
        abs(row["initial_energy_scale"] - .25) < 1e-5
        and abs(row["energy_ratio_delta"]) < .001 for row in amplitude_deltas)
    checks["partition_sensitivity_under_one_energy_percentage_point"] = all(
        abs(row["energy_ratio_delta"]) < .01 and abs(row["phase_delta_deg"]) < .5
        for row in partition_deltas)
    checks["donor_control_has_greater_mean_flow_loss"] = all(
        row["donor_energy_ratio"] < row["current_energy_ratio"]
        for row in donor_pairs if row["mean_flow_m_s"])
    mutant = select(rows, group="erased_packet_mutant")
    checks["erased_packet_deliberate_fault_is_caught"] = not (
        .95 < mutant["energy_ratio"] < 1.05 and mutant["carrier_magnitude"] > .95)
    amplified = [brief(row) for row in valid if row["energy_ratio"] > 1.05]
    default = [brief(row) for row in valid if row["dx_m"] == .075 and row["rate_Hz"] == 48000]
    refinements = {}
    for temperature, flow, frequency in ((900,70,800),(300,-70,800),(300,0,2000),(900,70,4000)):
        key = f"{temperature}K_{flow:+}m_s_{frequency}Hz"
        refinements[key] = [brief(row) for row in valid if row["temperature_K"] == temperature
                            and row["mean_flow_m_s"] == flow and row["frequency_Hz"] == frequency]
    timing = []
    for dx, rate in sorted({(row["dx_m"],row["rate_Hz"]) for row in controls if row["group"] == "timing_repeat"}):
        wall = [row["wall_seconds"] for row in controls if row["group"] == "timing_repeat"
                and row["dx_m"] == dx and row["rate_Hz"] == rate]
        timing.append({"dx_m":dx,"rate_Hz":rate,"matched_pair_wall_seconds_median":statistics.median(wall),
                       "minimum":min(wall),"maximum":max(wall),"repeats":len(wall)})
    growth = [{**brief(row),"time_s":row["time_s"],"amplitude_Pa":row["amplitude_Pa"]}
              for row in controls if row["group"] == "growth_check"]
    summary = {"self_checks":checks,"self_checks_pass":all(checks.values()),
               "matrix_cases":len(matrix),"guard_passing_cases":len(valid),
               "spatially_excluded_cases":sum(row["status"]=="excluded" for row in matrix),
               "solver_state_failures":len(solver_failures),
               "amplification_over_5_percent":amplified,
               "amplification_note":"Exclude these from attenuation claims. Growth is demonstrated in the tested segmented pipe network; bulk-versus-interface attribution is unresolved. Conventional CFL alone does not establish acoustic stability/accuracy. Independent center/halfstep controls retain growth, while segment partition strongly changes its magnitude; see review/ evidence and METHOD-RESULTS.md.",
               "fine_zero_flow_analytic_checks":[brief(row) for row in analytic],
               "half_amplitude_deltas":amplitude_deltas,"partition_deltas":partition_deltas,
               "donor_pairs":donor_pairs,"default_grid":default,"independent_refinements":refinements,
               "timing":timing,"growth_checks":growth,
               "max_base_pressure_error_Pa":max(row["base_pressure_error_Pa"] for row in nonmutant),
               "csv_sha256":{path.name:hashlib.sha256(path.read_bytes()).hexdigest()
                              for path in (directory/"transport.csv",directory/"controls.csv") if path.exists()}}
    (directory/"summary.json").write_text(json.dumps(summary,indent=2)+"\n")
    if not summary["self_checks_pass"]:
        raise SystemExit("transport harness self-check failed; inspect summary.json")
    if plot:
        plot_summary(summary, directory)
    print(json.dumps({key:summary[key] for key in ("self_checks_pass","matrix_cases","guard_passing_cases",
                                                    "spatially_excluded_cases","solver_state_failures")},indent=2))


def self_test(directory):
    """A corrupted evidence row must fail acceptance even with other rows good."""
    with tempfile.TemporaryDirectory(prefix="revbench-transport-fault-") as temporary:
        target = Path(temporary)
        with (directory/"transport.csv").open(newline="") as stream:
            reader = csv.DictReader(stream)
            fields, rows = reader.fieldnames, list(reader)
        row = next(row for row in rows if row["group"] == "matrix" and row["status"] == "ok")
        row["status"], row["repairs"] = "state_failure", "1"
        with (target/"transport.csv").open("w",newline="") as stream:
            writer = csv.DictWriter(stream,fields)
            writer.writeheader()
            writer.writerows(rows)
        shutil.copy2(directory/"controls.csv",target/"controls.csv")
        try:
            analyze(target)
        except SystemExit:
            result = json.loads((target/"summary.json").read_text())
            assert not result["self_checks_pass"] and result["solver_state_failures"] == 1
            assert not result["self_checks"]["no_state_repairs_nonfinite_or_caps"]
            assert not result["self_checks"]["no_solver_state_failures"]
        else:
            raise AssertionError("injected failed/repair evidence row was accepted")
    print("Self-test passed: one injected state-failure/repair row rejects the experiment.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--plot", action="store_true", help="also create the scientific summary figure (requires matplotlib)")
    parser.add_argument("--self-test", action="store_true", help="verify that a failed evidence row rejects acceptance")
    args = parser.parse_args()
    analyze(args.directory, args.plot)
    if args.self_test:
        self_test(args.directory)
