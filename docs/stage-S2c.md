# S2c — Source, transport and generation-architecture experiment

Completed 2026-10-23, continuing the investigation started 2026-10-22.
Production baseline: `3d79db339b9a5f031191d9e80276863e33cde18f` (same engine
code as the accepted selective rollback). Bernardo authorizes the experiment,
results and a robust development method, including research into changing the
sound-generation architecture. Production engine/preset/Claude files remain
unchanged. **S1/S1e/S2/S4/full 13b remain open; stage 14 is unstarted.**

## Outcome

The evidence supports investigating a different acoustic implementation while
retaining the physical engine. Numerical dispersion and mean-flow-dependent
loss are measurable; finer space helps more than a faster engine timestep in
these controls. At upper RPM, cut-state sound becomes more concentrated in
pipe harmonics as additional flow noise falls. No output limiter causes this.
This localizes mechanisms; it does not yet establish a physically calibrated
fix or an audible realism improvement.

The next architectural comparison is an acoustic-preserving conservative 1D
control versus a coupled nonlinear-gas/acoustic-network prototype, including
reflected loading and common geometry-dependent pulse/noise transfer. Recent
engine-specific learned pulse/parameter models are a useful additional
benchmark, subject to data and physical-control validation. See the
[architecture review](sound-generation-architecture-review.md) and the
[staged robustness plan](sound-robustness-plan.md).

## Experimental scope and controls

- **Straight duct:** 312 matrix configurations, with 241 passing spatial and
  stability setup guards and 71 explicitly excluded as underresolved. Sweep
  200/800/2000/4000 Hz, 300/900 K, mean flows -70/0/+70 m/s, four cell sizes and
  independent timestep choices. Add amplitude, partition, donor, growth and
  deliberately damaged-measurement controls. The duct is periodic, frictionless
  and adiabatic; the wave does not reach its periodic seam within the window.
- **Engine source matrix:** 30 trajectories / 60 loaded and held-injector-cut
  captures: I6 at 3000/7500, cross V8 at 3000/6500, flat V8 at 3000/8000 rpm,
  across five rate/grid configurations. Each state settles for 1.5 s and is
  captured for 0.75 s. All layers share one engine/noise trajectory and gain.
- **Actual overrun:** three ten-second source captures, beginning near upper
  RPM, with 0.5 s free loaded operation, pedal lift, eight-second road coast,
  clutch-in and the first 1.5 s afterward. Actual ECU fuel cut occurs at 0.65 s
  in all three; no AFR override is used. These are not full acceleration pulls.
- **References:** six newly decoded, unfiltered stereo intervals from the
  original MP3s, with source hashes/timestamps. They supplement existing raw
  references without replacing or modifying them. Unknown RPM/load/perspective
  and turbo-M4/NA-I6 mismatch preclude calibrated waveform or level matching.
- **Cost:** production stereo rendering without diagnostic listeners, trace
  writing or WAV output, cross V8 at 6500 rpm, three independent repeats per
  configuration. Benchmark results are native only in this host environment.

The default matrix has engine/pipe/output rates 192/48/48 kHz. Engine-time
control changes only the engine rate to 384 kHz. Pipe-time control uses
192/96 kHz with the original grid held in an isolated `engine.c` copy. Space
control keeps those rates and uses the ordinary finer target mesh. Ultra
combines 384/96 kHz with that finer mesh. Actual cell counts are I6 170→354,
cross 226→446, flat 188→372. Production source is never patched.

## Pipe results: magnitude is not enough

At 900 K, +70 m/s and 800 Hz, after 2.5 ms at a fixed 192-kHz pipe step:

| Cell size | Perturbation energy retained | Carrier phase error |
| --- | ---: | ---: |
| 75 mm | 79.25% | 10.56° |
| 37.5 mm | 93.13% | 2.34° |
| 18.75 mm | 98.99% | 0.0014° |

Keeping 75-mm cells but changing only pipe rate 48→192 kHz instead changes
retention **88.10→79.25%**. Coarser temporal error can partially cancel spatial
error; faster time alone is not a robust remedy.

At 300 K, zero mean flow, 2 kHz and fixed 192-kHz stepping:

| Cell size | Perturbation energy retained | Phase error | Packet-speed error |
| --- | ---: | ---: | ---: |
| 37.5 mm | 99.953% | 134.65° | -23.00% |
| 18.75 mm | 99.978% | 34.01° | -5.92% |
| 9.375 mm | 99.992% | 8.30° | -1.45% |

An independent staggered-wave dispersion calculation predicts approximately
134.73/34.03/8.30°, closely matching the phase extractor. This is expected
coarse-grid dispersion, not proof of a separate implementation mistake.
The reported energy is a staggered quadratic acoustic-perturbation diagnostic,
not the complete nonlinear system's total conserved energy.

**Recorded growth warning:** the tested segmented network at 900 K/+70 m/s,
4 kHz/9.375 mm/192 kHz reaches 215.9% perturbation energy at 10 ms. Reducing
timestep suppresses the observed growth. Position, halfstep and pressure-only
controls reproduce it, but changing segment partition changes its magnitude
substantially. Bulk transport versus interface attribution remains unresolved.
This is not the production default grid, and these cases are excluded from
attenuation claims. Ordinary CFL/finite-state checks alone miss this warning.

## Engine results

Energy above 2 kHz as a fraction of 20 Hz–20 kHz output energy, dB, loaded at
the upper held RPM (less negative means a larger fraction):

| Engine / held RPM | Baseline | Engine time only | Pipe time / old grid | Finer space | Combined ultra |
| --- | ---: | ---: | ---: | ---: | ---: |
| I6 / 7500 | -20.86 | -21.01 | -20.60 | -14.67 | -14.27 |
| Cross V8 / 6500 | -11.00 | -10.86 | -11.03 | -10.18 | -9.72 |
| Flat V8 / 8000 | -13.20 | -13.30 | -13.60 | -8.31 | -8.35 |

Thus doubling engine rate alone changes this measure by at most 0.15 dB;
refining space at the same 96-kHz pipe rate changes it by +5.93/+0.85/+5.29 dB.
This is a feature comparison, not a brightness or realism acceptance score.
Refinement can also change breathing/temperature/excitation; it is not an
isolated engine transfer-function measurement. Noise sequences differ by rate.

At default rates, closing the throttle and disabling injection while holding
the same upper RPM gives:

| Engine | Full RMS change | Tail-position flow-noise change | Strongest pipe-line concentration, loaded→cut |
| --- | ---: | ---: | ---: |
| I6 / 7500 | +6.62 dB | -11.06 dB | 23.0→34.9% |
| Cross V8 / 6500 | -0.54 dB | -10.68 dB | 31.6→55.5% |
| Flat V8 / 8000 | -1.29 dB | -16.41 dB | 45.7→66.4% |

Concentration is the strongest line ±5 Hz divided by that pipe stem's
20 Hz–20 kHz power, from equal 0.75-s windows; it is not S1's repetition metric.
Tail-position flow noise combines valve noise and outlet jets. Bay mechanical
sound includes pressure-force excitation and impacts. Coherent source powers
do not add because their phases can interfere.

Held mode clears the real ECU fuel-cut flag. This fixture instead uses the
existing AFR-zero injection guard; all cut injection is zero, but residual
heat is 7.21e-7…3.18e-5 of the loaded heat. A known synthesized injector-click
gating limitation persists in the bay layer. Temperature and mean flow also
change markedly, so this is not a pure mean-flow-only experiment.

The actual ECU coasts corroborate the source localization. Comparing 0.1–0.4 s
loaded with 0.7–1.3 s early coast gives full RMS changes **+6.06/+1.07/+0.31 dB**
and tail flow-noise changes **-10.54/-9.93/-18.76 dB** for I6/cross/flat.
The full mix closely follows pipe pulses. These windows differ in RPM and
thermodynamic state; their differences are not causal transfer coefficients.
Cross/flat resume fuel near 9.70/9.56 s; I6 does not resume within this capture.
None reaches its idle checkpoint before the ten-second ending. Existing full
pull/recovery examples remain available in the accepted result.

The local **tailpipe end-cell gas** also changes sharply: mean loaded→early
coast temperature is approximately 848→319 K / 904→312 K / 974→316 K, while
mean axial velocity changes about 99→-0.39 / 90→-0.69 / 179→-0.11 m/s.
These are not wall temperatures or a claim that the entire exhaust cools in
0.2 s. Ambient reverse flow, numerical mixing, near-mouth thermal treatment
and actual propagation changes need separate tests. This is a concrete
transient-thermal validation priority, not evidence to assume numerical
damping alone causes the tone.

The final DC-blocker changes measured 2–20-kHz power by approximately +0.006 dB,
and all captures remain below the audio limiter knee with zero repairs/limiter
counts. The high-frequency plateau is not explained by final limiting or that
DC stage. Raw physical taps are retained as diagnostics, with instantaneous
48-kHz pressure/flow sampling explicitly not treated as antialiased transfer
measurements.

## Cost and architecture decision

Production stereo asphalt rendering at held 6500-rpm cross V8, gain 0.1,
48-kHz output, 128-frame blocks; 1-s settle and 2-s measurement per repeat.
Median CPU-based real-time factor over three runs, larger is faster:

| Configuration | CPU real-time factor |
| --- | ---: |
| Baseline | 0.563× |
| Engine time only | 0.402× |
| Pipe time / old grid | 0.398× |
| Finer space | 0.256× |
| Combined ultra | 0.211× |

These are native O3/strict-double **LTO-disabled** measurements, not the
previous LTO-enabled engine-only benchmark, WASM timings or browser deadline
acceptance. Callback wall times are recorded but host scheduling is shared.
Uniform refinement is materially more expensive; no heavier default is promoted
without demonstrated listening benefit. A shared, accurate acoustic network is
therefore a useful candidate to test against a better 1D control, rather than
simply raising all rates. Its advantage remains to be measured.

## References and listening result

New stereo reference intervals: M4 rising candidate 242–249 s / falling pitch
250–253 s; Mustang 200–207 / 257–263 s; Ferrari 49–55 / 302–310 s. Falling pitch
does not establish loaded lift/DFCO; rising pitch does not establish a loaded
pull. Source recordings have unverified perspective/processing and MP3 losses.
Their measured above-2-kHz fractions vary approximately -24.8…-40.0 dB, which
is observational evidence against treating all references as one EQ target,
not a calibrated pass/fail range for the simulator.

`renders/RESULTS.html` retains all 34 accepted examples and five full long
coasts, and appends the research plots and twelve diagnostic listening clips.
They include source strips, upper-RPM resolution controls and actual ECU
coasts. Experimental previews all share one static gain with 5-ms edge fades;
measurement floats are never peak normalized or denoised. The standard clips
keep their earlier independently adjusted playback levels. Selected raw/output
diagnostic floats remain together in `renders/current/experiment/`.

## Reproduction, evidence and validation

New tools: `acoustic_transport_probe.c`/`analyze_acoustic_transport.py`,
`audio_source_probe.c`/`analyze_audio_source_probe.py`, `run_audio_resolution.py`,
`audio_coast_probe.c`, `audio_render_bench.c`, `audio_reference_probe.py` and
`audio_experiment_report.py`. Compile native probes with the unchanged core,
`-O3 -ffp-contract=off`, and use the analysis venv. The resolution runner builds
the isolated fixed-grid control and stores exact commands/source hashes.
Run long matrix chunks by case/preset; the host may terminate sessions near
ten minutes. Prefer `/tmp` capture output then bulk-copy to the evidence folder;
writing fine-grained traces on the mounted filesystem adds substantial cost.

Evidence lives in ignored `renders/evidence/audio-experiment-2026-10-22/`:
transport CSVs/method/provenance/review controls; engine metadata/metrics,
commands/hashes and phase summaries; reference intervals/features; render
timings; validation logs and result hashes. The date identifies the start of
the investigation. Retain selected raw sources and numerical evidence; remove
superseded temporary WAV/trace controls after publication checks succeed.

Native **175 tests / 4472 checks**, Python **29**, web **17**, source-probe and
transport ASan/UBSan smoke checks pass. The transport analyzer has nine checks;
injecting a failed-state/repair row correctly rejects acceptance. Independent
two-tone and antiphase-channel tests verify the spectral power normalization.
All six default source fixtures exactly match production mono output for
4096 checked frames; source reconstruction errors are approximately 1e-15
before float32 rounding. No new production/WASM build is needed for unchanged
core. Offline result publication, source preservation and Graphify refresh
are recorded in the final validation manifest. The final offline check plays
all **46 clips**, preserves the four top-end jump buttons, loads all four
plots and enforces exclusive playback. All **34 original previews and 59
reference files** remain byte-identical. The **42 selected float files** are
grouped in `renders/current/experiment/`; **252 temporary controls / 1.75 GB**
were removed. Republication succeeds using retained assets after cleanup.
Graphify's AST refresh reports **2011 nodes / 5485 edges / 113 communities**,
with its known `rb.h:415` extraction limit; no new semantic extraction of the
research documents is claimed. Tests do not close listening or complete-engine
physical validation.
