"""Summarize independent source-probe captures without normalizing or folding channels.

Usage: PYTHONPATH=tools .venv-analysis/bin/python tools/analyze_audio_source_probe.py DIR
Writes metrics.json for each metadata.json under DIR and a source-summary.json.
These measurements are diagnostics, not new sound-acceptance criteria.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np

from audio_wav import read_wav


BANDS = ((20, 500), (500, 2000), (2000, 5000), (5000, 10000), (10000, 20000))


def db(value):
    return float(10 * np.log10(value)) if value > 0 else None


def powers(samples, rate):
    """Welch-style windowed mean square, preserving each source channel separately."""
    if samples.ndim == 1:
        samples = samples[:, None]
    n = min(8192, len(samples))
    window = np.hanning(n)
    accumulated = np.zeros((n // 2 + 1, samples.shape[1]))
    count = 0
    for start in range(0, len(samples) - n + 1, n // 2):
        x = samples[start:start + n]
        x = (x - np.mean(x, axis=0)) * window[:, None]
        spectrum = abs(np.fft.rfft(x, axis=0)) ** 2 / (n * np.sum(window * window))
        spectrum[1:-1] *= 2
        accumulated += spectrum
        count += 1
    accumulated /= count
    frequencies = np.fft.rfftfreq(n, 1 / rate)
    result = {}
    for lo, hi in BANDS:
        result[f"{lo}_{hi}_Hz"] = np.sum(accumulated[(frequencies >= lo) & (frequencies < hi)], axis=0)
    return result


def signal_metrics(samples, rate, names, line_metrics=True):
    band_power = powers(samples, rate)
    result = {}
    for i, name in enumerate(names):
        x = samples[:, i]
        rms_power = float(np.mean(x * x))
        peak = float(np.max(abs(x)))
        p = {band: float(value[i]) for band, value in band_power.items()}
        audible = sum(p.values())
        high = p["2000_5000_Hz"] + p["5000_10000_Hz"] + p["10000_20000_Hz"]
        halves = [float(np.mean(a * a)) for a in np.array_split(x, 2)]
        result[name] = {
            "rms": float(np.sqrt(rms_power)), "rms_dBFS": db(rms_power), "peak": peak,
            "crest_dB": db(peak * peak / rms_power) if rms_power else None,
            "mean": float(np.mean(x)), "band_mean_square": p,
            "above_2k_to_20_20k_dB": db(high / audible) if audible else None,
            "second_half_to_first_half_power_dB": db(halves[1] / halves[0]) if halves[0] else None,
        }
        if line_metrics and audible > 1e-24:
            spectrum = abs(np.fft.rfft((x - np.mean(x)) * np.hanning(len(x)))) ** 2
            frequency = np.fft.rfftfreq(len(x), 1 / rate)
            audible_bins = (frequency >= 20) & (frequency < 20000)
            spectrum[~audible_bins] = 0
            peak_frequency = float(frequency[np.argmax(spectrum)])
            nearby = abs(frequency - peak_frequency) <= 5
            result[name]["dominant_line_Hz"] = peak_frequency
            result[name]["power_fraction_within_5Hz_of_dominant_line"] = (
                float(np.sum(spectrum[nearby]) / np.sum(spectrum))
            )
    summed = np.sum(samples[:, 1:], axis=1)
    result["linearity"] = {
        "max_abs_float_file_error": float(np.max(abs(samples[:, 0] - summed))),
        "rms_float_file_error": float(np.sqrt(np.mean((samples[:, 0] - summed) ** 2))),
        "caution": "Source powers are not additive: same-trajectory pipe/intake phases can interfere."
    }
    return result


def analyze(directory):
    metadata = json.loads((directory / "metadata.json").read_text())
    result = {
        "preset": metadata["preset"], "rpm": metadata["rpm"],
        "physics_rate_hz": metadata["physics_rate_hz"],
        "actual_pipe_rate_hz": metadata["actual_pipe_rate_hz"],
        "total_cells": metadata["total_cells"], "gain": metadata["gain"],
        "listener": metadata["listener"], "captures": {},
        "stem_descriptions": {
            "full": "All sources at the same mono behind-car listener.",
            "pipe_tail": "Monopole radiation from both modeled tailpipe volume flows.",
            "pipe_intake": "Monopole radiation from intake-mouth volume flow.",
            "jet_tail": "Combined procedural exhaust-valve and outlet-jet noise at tail positions.",
            "jet_intake": "Procedural throttle/idle hiss and intake-valve roar at the intake mouth.",
            "mechanical": "Complete bay modal sound: combustion-pressure forcing, slap, valves, injectors; "
                          "mount contribution is inaudible at the outside listener.",
        },
        "method": "No normalization; each channel is an isolated mono-listener source, not a stereo ear. "
                  "Band mean-square uses 8192-sample Hann windows, 50% overlap and mean removal. "
                  "Raw/filtered bands have different frequency resolution at different sample rates. "
                  "Trace pressure/flow columns are instantaneous, not antialiased. Thermodynamic and "
                  "outlet conditions change across the network, so their ratios are not isolated transfer functions. "
                  "Noise RNG sample sequences change with timestep; use feature/distribution comparisons. "
                  "Dominant-line metrics use the complete capture with a Hann window and a +/-5 Hz band; "
                  "this is not the project's S1 repetition acceptance metric.",
    }
    for state in ("loaded", "cut"):
        entry = dict(metadata["captures"][state])
        for suffix in ("raw", "after-aa", "filtered"):
            path = directory / f"{state}-{suffix}.wav"
            samples, rate = read_wav(path)
            entry[suffix] = signal_metrics(samples, rate, metadata["stems"], suffix == "filtered")
            entry[f"{suffix}_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
        path = directory / f"{state}-trace.f64"
        traces = np.fromfile(path, dtype=np.float64).reshape(-1, len(metadata["trace_columns"]))
        summaries = {}
        for i, name in enumerate(metadata["trace_columns"]):
            x = traces[:, i]
            summaries[name] = {
                "mean": float(np.mean(x)), "std": float(np.std(x)),
                "min": float(np.min(x)), "max": float(np.max(x)),
                "negative_fraction": float(np.mean(x < 0)),
            }
        entry["trace_summary"] = summaries
        # Pressure-spectrum features locate where acoustic shape changes; they do
        # not separate propagated/reflected waves or infer a transfer function.
        selected = ["cyl0_p_pa", "header0_start_p_pa", "header0_end_p_pa", "collector0_p_pa",
                    "tail0_start_p_pa", "tail0_end_p_pa", "tail0_q_m3_s", "exhaust0_mdot_kg_s"]
        values = traces[:, [metadata["trace_columns"].index(name) for name in selected]]
        bp = powers(values, metadata["output_rate_hz"])
        entry["trace_band_mean_square"] = {
            name: {band: float(value[i]) for band, value in bp.items()}
            for i, name in enumerate(selected)
        }
        result["captures"][state] = entry
    loaded_heat = result["captures"]["loaded"]["heat_released_J"]
    result["cut_heat_fraction_of_loaded"] = (
        result["captures"]["cut"]["heat_released_J"] / loaded_heat if loaded_heat else None
    )
    result["cut_injection_zero"] = result["captures"]["cut"]["injected_fuel_kg"] == 0
    (directory / "metrics.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    reports = []
    for path in sorted(args.directory.rglob("metadata.json")):
        if not {"stems", "captures", "trace_columns"} <= json.loads(path.read_text()).keys():
            continue
        report = analyze(path.parent)
        report["directory"] = str(path.parent)
        reports.append(report)
        print(json.dumps({key: report[key] for key in ("directory", "preset", "rpm", "physics_rate_hz", "total_cells")}))
    (args.directory / "source-summary.json").write_text(json.dumps(reports, indent=2) + "\n")


if __name__ == "__main__":
    main()
