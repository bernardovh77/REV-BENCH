#!/usr/bin/env python3
"""Plot the audio experiment and append it to the single offline RESULTS.html.

Read-only with respect to engine parameters/current recordings. Diagnostic
listening copies share a constant gain, and no source is normalized separately.
"""
import argparse
import base64
import csv
import hashlib
from html import escape
import json
import os
from pathlib import Path
import struct

os.environ.setdefault("MPLCONFIGDIR", "/tmp/rev-bench-mpl")
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from audio_wav import read_wav
from analyze_audio_source_probe import powers, signal_metrics

ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / "renders/evidence/audio-experiment-2026-10-22"
BEGIN = "<!-- AUDIO EXPERIMENT BEGIN -->"
END = "<!-- AUDIO EXPERIMENT END -->"
LABELS = {"i6": "Inline six", "v8-cross": "Cross V8", "v8-flat": "Flat V8"}
COLORS = ["#82d4a0", "#66b6ee", "#eda564", "#d09ae2", "#e0cb6a"]
RETAINED = ROOT/"renders/current/experiment"


def db(value):
    return 10*np.log10(np.maximum(value, 1e-30))


def self_test():
    rate = 48000
    t = np.arange(rate)/rate
    signal = np.sin(2*np.pi*1000*t)+.5*np.sin(2*np.pi*4000*t)
    # Antiphase stereo must preserve power under per-channel measurement.
    p = powers(np.column_stack((signal, -signal)), rate)
    total = sum(p.values())
    assert np.allclose(total, .625, atol=1e-4)
    fraction = p["2000_5000_Hz"]/total
    assert np.allclose(fraction, .2, atol=1e-4)
    assert np.mean(np.mean(np.column_stack((signal, -signal)), axis=1)**2) == 0
    print("Analytic two-tone band-power and antiphase-channel checks passed.")


def finish_plot(fig, path):
    fig.savefig(path, dpi=160, bbox_inches="tight", facecolor=fig.get_facecolor())
    plt.close(fig)


def transport_plot(evidence):
    with (evidence/"transport/transport.csv").open() as stream:
        rows = list(csv.DictReader(stream))
    fig, axes = plt.subplots(1, 2, figsize=(11, 4), layout="constrained")
    series = [(0.075, 48000, "75 mm / 48 kHz"), (0.075, 192000, "75 mm / 192 kHz"),
              (0.0375, 192000, "37.5 mm / 192 kHz"), (0.009375, 384000, "9.375 mm / 384 kHz")]
    for index, (dx, rate, label) in enumerate(series):
        selected = [r for r in rows if r["status"] == "ok" and float(r["temperature_K"]) == 900
                    and float(r["mean_flow_m_s"]) == 70 and float(r["dx_m"]) == dx
                    and float(r["rate_Hz"]) == rate]
        selected.sort(key=lambda r: float(r["frequency_Hz"]))
        frequency = [float(r["frequency_Hz"]) for r in selected]
        axes[0].semilogx(frequency, [100*float(r["energy_ratio"]) for r in selected],
                         "o-", color=COLORS[index], label=label)
        axes[1].semilogx(frequency, [float(r["carrier_phase_error_deg"]) for r in selected],
                         "o-", color=COLORS[index], label=label)
    axes[0].axhline(100, color="white", alpha=.4, linestyle="--")
    axes[0].set(ylabel="Packet energy retained (%)", ylim=(0, 130))
    axes[1].set(ylabel="Carrier phase error (degrees)")
    for axis in axes:
        axis.set(xlabel="Frequency (Hz)", xticks=[200, 800, 2000, 4000])
        axis.set_xticklabels(["200", "800", "2,000", "4,000"])
        axis.grid(alpha=.15)
    axes[0].legend(fontsize=8)
    fig.suptitle("Frictionless pipe · 900 K · +70 m/s · 2.5 ms\nMissing points fail the spatial-resolution guard")
    path = evidence/"transport-detail.png"
    finish_plot(fig, path)
    return path


def source_plot(evidence, reports):
    chosen = [r for r in reports if r["rpm"] == 3000 and "/baseline/" in r["directory"]]
    fig, axes = plt.subplots(1, 3, figsize=(12, 4), sharey=True, layout="constrained")
    names = ["pipe_tail", "pipe_intake", "jet_tail", "jet_intake", "mechanical"]
    for axis, preset in zip(axes, LABELS):
        report = next(r for r in chosen if r["preset"] == preset)
        for state, offset, color in (("loaded", -.10, COLORS[1]), ("cut", .10, COLORS[2])):
            signals = report["captures"][state]["filtered"]
            power = lambda n: sum(signals[n]["band_mean_square"].values())
            values = [float(db(power(name)/power("full"))) for name in names]
            axis.scatter(values, np.arange(5)+offset, color=color,
                         label="Loaded" if state == "loaded" else "Injector cut", s=45)
        axis.set(title=LABELS[preset], xlim=(-90, 5), xlabel="Level relative to full mix (dB)",
                 yticks=np.arange(5), yticklabels=["Tailpipe pulses", "Intake pulses", "Tail flow noise", "Intake flow noise", "Mechanical"])
        axis.axvline(0, color="white", alpha=.3, ls="--")
        axis.grid(axis="x", alpha=.15)
    axes[0].invert_yaxis()
    axes[2].legend(fontsize=8)
    fig.suptitle("Same-trajectory sound layers · held 3,000 rpm\nPowers do not add: coherent pipe sources can interfere")
    path = evidence/"source-balance.png"
    finish_plot(fig, path)
    return path


def reference_features(evidence):
    manifest = json.loads((evidence/"references/reference-intervals.json").read_text())
    results = []
    for row in manifest["intervals"]:
        samples, rate = read_wav(evidence/"references"/row["file"])
        p = {band: float(np.mean(value)) for band, value in powers(samples, rate).items()}
        high = sum(value for band, value in p.items() if band.startswith(("2000_", "5000_", "10000_")))
        results.append(dict(**row, above_2k_to_20_20k_dB=float(db(high/sum(p.values()))),
                            crest_dB=float(db(np.max(samples*samples)/np.mean(samples*samples))),
                            band_mean_square=p))
    (evidence/"reference-features.json").write_text(json.dumps(results, indent=2)+"\n")
    return results


def coast_features_and_plot(evidence):
    fig, axes = plt.subplots(3, 1, figsize=(11, 9), sharex=True, layout="constrained")
    records = []
    for axis, preset in zip(axes, LABELS):
        directory = evidence/"engine/coast"/preset
        metadata = json.loads((directory/"metadata.json").read_text())
        source = directory/"coast-filtered.wav"
        samples, rate = read_wav(source if source.exists() else RETAINED/f"{preset}-coast-filtered-stems.wav")
        if (directory/"coast-trace.f64").exists():
            trace = np.memmap(directory/"coast-trace.f64", dtype=np.float64, mode="r",
                              shape=(len(samples), len(metadata["trace_columns"])))
            column = lambda name: trace[:, metadata["trace_columns"].index(name)]
            phases = {}
            for name, (start, end) in metadata["phase_windows_s"].items():
                a, b = int(start*rate), int(end*rate)
                phases[name] = dict(start_s=start, end_s=end,
                    rpm_range=[float(np.min(column("rpm")[a:b])), float(np.max(column("rpm")[a:b]))],
                    ecu_fuel_cut_fraction=float(np.mean(column("ecu_fuel_cut")[a:b])),
                    tail_temperature_mean_K=float(np.mean(column("tail0_end_T_K")[a:b])),
                    tail_velocity_mean_m_s=float(np.mean(column("tail0_end_u_m_s")[a:b])),
                    map_mean_Pa=float(np.mean(column("map_pa")[a:b])),
                    injected_fuel_kg=float(column("fuel_in_kg")[b-1]-column("fuel_in_kg")[a]),
                    heat_released_J=float(column("heat_released_J")[b-1]-column("heat_released_J")[a]),
                    signals=signal_metrics(samples[a:b], rate, metadata["stems"]))
        else:
            saved = json.loads((evidence/"coast-features.json").read_text())
            phases = next(r["phases"] for r in saved if r["preset"] == preset)
        record = dict(preset=preset, markers=metadata["markers"], phases=phases,
                      repairs=metadata["repairs_total"], limited=metadata["samples_above_limiter_knee_all_stems"],
                      method="Actual ECU upper-start road coast, mono behind listener, common gain. "
                             "Phase windows contain different RPM/temperature/flow states; changes are not an isolated transfer coefficient.")
        records.append(record)
        block = rate//20
        usable = len(samples)//block*block
        mean_square = np.mean(samples[:usable].reshape(-1, block, samples.shape[1])**2, axis=1)
        times = (np.arange(len(mean_square))+.5)*block/rate
        for channel, label, color in [(0, "Full", "white"), (1, "Tailpipe pulses", COLORS[0]),
                                      (3, "Tail flow noise", COLORS[2]), (5, "Mechanical", COLORS[3])]:
            axis.plot(times, db(mean_square[:, channel]), color=color, lw=1.2, label=label)
        for key, color in (("lift_s", "#dddddd"), ("ecu_fuel_cut_s", "#f0a060"), ("clutch_s", "#aaaaaa")):
            axis.axvline(metadata["markers"][key], color=color, alpha=.7, ls="--")
        axis.set(title=LABELS[preset], ylabel="50 ms RMS (dBFS)", ylim=(-100, -15))
        axis.grid(alpha=.12)
    axes[0].legend(ncol=4, fontsize=8)
    axes[-1].set_xlabel("Time (s); dashed lines: pedal lift, fuel cut, clutch-in")
    fig.suptitle("Actual ECU lift and eight-second road coast · same-trajectory sources")
    path = evidence/"coast-layers.png"
    finish_plot(fig, path)
    (evidence/"coast-features.json").write_text(json.dumps(records, indent=2)+"\n")
    return path


def listening_comparisons(evidence, reports):
    """Concatenate full/tail/intake/jet/mechanical from one capture, at one gain."""
    files = []
    retained = RETAINED
    retained.mkdir(exist_ok=True)
    def audition_segment(x, rate):
        x = x.copy()
        length = min(int(.005*rate), len(x)//2)
        ramp = np.sin(np.linspace(0, np.pi/2, length))**2
        x[:length] *= ramp
        x[-length:] *= ramp[::-1]
        return x
    for preset in LABELS:
        report = next(r for r in reports if r["preset"] == preset and r["rpm"] == 3000
                      and "/baseline/" in r["directory"])
        directory = Path(report["directory"])
        for state in ("loaded", "cut"):
            source = directory/f"{state}-filtered.wav"
            samples, rate = read_wav(source if source.exists() else retained/f"{preset}-{state}-filtered-stems.wav")
            # Keep selected raw float controls as well as their normal output chain.
            for suffix in ("raw", "after-aa", "filtered"):
                if (directory/f"{state}-{suffix}.wav").exists():
                    (retained/f"{preset}-{state}-{suffix}-stems.wav").write_bytes(
                        (directory/f"{state}-{suffix}.wav").read_bytes())
            clips = [samples[:, 0], samples[:, 1], samples[:, 2], samples[:, 3]+samples[:, 4], samples[:, 5]]
            gap = np.zeros(int(.35*rate))
            combined = np.concatenate([part for clip in clips for part in (audition_segment(clip, rate), gap)])
            files.append(dict(preset=preset, state=state, rate=rate, samples=combined,
                              kind="layers", title=f'{LABELS[preset]} · {"Loaded" if state == "loaded" else "Injector cut"}'))
    for preset in LABELS:
        upper = max(r["rpm"] for r in reports if r["preset"] == preset)
        clips = []
        for case in ("baseline", "engine-time", "pipe-time", "space", "ultra"):
            report = next(r for r in reports if r["preset"] == preset and r["rpm"] == upper
                          and f"/{case}/" in r["directory"])
            source = Path(report["directory"])/"loaded-filtered.wav"
            saved = retained/f"{preset}-{upper}-{case}-loaded-full.wav"
            if source.exists():
                samples, rate = read_wav(source)
                payload = samples[:, 0].astype("<f4").tobytes()
                fmt = struct.pack("<HHIIHH", 3, 1, rate, rate*4, 4, 32)
                saved.write_bytes(b"RIFF"+struct.pack("<I", 36+len(payload))+b"WAVEfmt "+struct.pack("<I", 16)+fmt+b"data"+struct.pack("<I", len(payload))+payload)
            else:
                samples, rate = read_wav(saved)
            clips.extend((audition_segment(samples[:, 0], rate), np.zeros(int(.35*rate))))
        files.append(dict(preset=preset, state="resolution", rate=rate, samples=np.concatenate(clips),
                          kind="resolution", title=f'{LABELS[preset]} · {upper:,} rpm · resolution controls'))
    for preset in LABELS:
        directory = evidence/"engine/coast"/preset
        source = directory/"coast-filtered.wav"
        samples, rate = read_wav(source if source.exists() else retained/f"{preset}-coast-filtered-stems.wav")
        if source.exists():
            (retained/f"{preset}-coast-filtered-stems.wav").write_bytes(source.read_bytes())
        # Preserve compact float32 full-mix raw/output sources for the selected coasts.
        for suffix in ("raw", "filtered"):
            if not (directory/f"coast-{suffix}.wav").exists():
                assert (retained/f"{preset}-coast-{suffix}-full.wav").exists()
                continue
            source, source_rate = read_wav(directory/f"coast-{suffix}.wav")
            payload = source[:, 0].astype("<f4").tobytes()
            fmt = struct.pack("<HHIIHH", 3, 1, source_rate, source_rate*4, 4, 32)
            wav = b"RIFF"+struct.pack("<I", 36+len(payload))+b"WAVEfmt "+struct.pack("<I", 16)+fmt+b"data"+struct.pack("<I", len(payload))+payload
            (retained/f"{preset}-coast-{suffix}-full.wav").write_bytes(wav)
        files.append(dict(preset=preset, state="coast", rate=rate,
                          samples=audition_segment(samples[:, 0], rate), kind="coast",
                          title=f'{LABELS[preset]} · actual lift + eight-second coast'))
    # Single gain across all comparisons, including quiet source stems.
    maximum = max(float(np.max(np.abs(row["samples"]))) for row in files)
    shared_gain = .6/maximum if maximum else 1
    output = []
    for row in files:
        samples = np.rint(np.clip(row.pop("samples")*shared_gain, -1, 1)*32767).astype("<i2")
        fmt = struct.pack("<HHIIHH", 1, 1, row["rate"], row["rate"]*2, 2, 16)
        payload = samples.tobytes()
        wav = b"RIFF"+struct.pack("<I", 36+len(payload))+b"WAVEfmt "+struct.pack("<I", 16)+fmt+b"data"+struct.pack("<I", len(payload))+payload
        row["wav_base64"] = base64.b64encode(wav).decode()
        row["shared_preview_gain"] = shared_gain
        row["seconds"] = len(samples)/row["rate"]
        row["preview_sha256"] = hashlib.sha256(wav).hexdigest()
        output.append(row)
    (retained/"manifest.json").write_text(json.dumps(dict(
        processing="All experiment comparisons share one scalar gain, PCM16 preview conversion, "
                   "5-ms edge fades on individual listening segments, .35-s gaps in strips. "
                   "Float measurement sources are unchanged. Standard34 previews use their original gains.",
        clips=[{key:value for key,value in row.items() if key != "wav_base64"} for row in output]), indent=2)+"\n")
    return output


def publish(evidence, figures, comparisons):
    target = ROOT/"renders/RESULTS.html"
    html = target.read_text()
    if BEGIN in html:
        start, rest = html.split(BEGIN, 1)
        _, end = rest.split(END, 1)
        html = start+end
    parts = [BEGIN, '<section id="audio-experiment"><div class="eyebrow">AUDIO RESEARCH / EXPERIMENT</div>',
             '<h2>Where the sound loses detail</h2>',
             '<p>The accepted engine setups and all five long coasts above are preserved. These controls diagnose the model; they are not a new sound tune.</p>',
             '<p>Key findings: finer cells improve wave timing and reduce some losses; smaller timesteps alone do not guarantee better acoustics. The upper-RPM cut-state tone is dominated by pipe pulses while procedural flow noise falls. Final output limiting is inactive. The report documents model limits and the next architecture comparisons.</p>']
    for path in figures:
        data = base64.b64encode(path.read_bytes()).decode()
        parts.append(f'<img style="width:100%;height:auto" alt="{escape(path.stem)}" src="data:image/png;base64,{data}">')
    parts.append('<h3>Listen to the separate layers at 3,000 rpm</h3><p>Each layer recording plays: full mix → tailpipe pulses → intake pulses → flow noise → mechanical sound. Every segment lasts 0.75 seconds, with short gaps and 5 ms edge fades on these listening copies. All experiment comparisons share one fixed gain; quiet layers stay quiet. Compare their levels with each other; standard clips above have independent listening-volume adjustment.</p><p>“Injector cut” is a held-speed test with injectors disabled and throttle closed. It is not a road coast; residual fuel and a known synthesized injector-click limitation are documented in the report.</p>')
    for row in comparisons:
        if row["kind"] == "resolution" and row["preset"] == "i6":
            parts.append('<h3>Resolution controls near the top of the rev range</h3><p>Order: baseline → engine timestep only → pipe timestep at the old grid → finer grid at that same pipe rate → combined ultra control. These short loaded captures are diagnostic; their random-noise sequences differ when simulation rate changes.</p>')
        if row["kind"] == "coast" and row["preset"] == "i6":
            parts.append('<h3>Actual lift-off and long coast</h3><p>These diagnostic clips start near the upper rev range, lift at 0.5 seconds, coast in gear for eight seconds, then disengage the clutch. They use the real engine controller and the same fixed experiment gain. The complete pulls and long coasts remain above.</p>')
        parts.append(f'<article><h3>{escape(row["title"])}</h3><audio controls preload="none" data-experiment="{row["preset"]}-{row["state"]}" src="data:audio/wav;base64,{row["wav_base64"]}"></audio></article>')
    parts.extend(['<p>Real recordings were measured separately without EQ, denoising or mono folding. Their RPM, load and microphone geometry are unknown, so they remain observational references rather than calibrated targets.</p>', '</section>', END])
    html = html.replace('</main>', '\n'.join(parts)+'</main>', 1)
    # Replace rather than duplicate this navigation link on reruns.
    link = '<a href="#audio-experiment">Research results</a>'
    if link not in html:
        html = html.replace('</nav>', link+'</nav>', 1)
    pending = target.with_suffix('.pending.html')
    pending.write_text(html)
    pending.replace(target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, default=DEFAULT)
    parser.add_argument("--publish", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    plt.style.use("dark_background")
    reports = json.loads((args.evidence/"engine/source-summary.json").read_text())
    figures = [args.evidence/"transport/transport-summary.png", transport_plot(args.evidence),
               source_plot(args.evidence, reports), coast_features_and_plot(args.evidence)]
    reference_features(args.evidence)
    if args.publish:
        publish(args.evidence, figures, listening_comparisons(args.evidence, reports))
    print("Experiment plots/reference features complete" + ("; single RESULTS.html updated." if args.publish else "."))


if __name__ == "__main__":
    main()
