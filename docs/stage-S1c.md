# Stage S1c — Reproducible sound and combustion baselines

Date: 2026-10-21. Complete as a baseline substage. S1 remains open.

The previous handoff requested a five-preset baseline before further combustion
tuning. Its physics audit lived outside the repository, and its nominal 3000 rpm
point used 96 kHz physics rather than the default high-quality 192 kHz. This
substage makes the baseline repeatable from the repository without changing
combustion, presets, listener processing, quality defaults or the browser.

## Native cycle audit

`make audit` builds `native/rb_audit.c` and writes
`renders/s1-audit/physics.csv`. The target replaces the previous completed CSV
only after a successful run. Progress goes to stderr, while stdout is CSV.

For each of the five stock presets, independent runs cover:

- A start from rest followed by 30 seconds of free idle, including startup;
  the starter releases at 75% of nominal smoothed idle speed or after 3 seconds.
- Ten seconds held at 3000 rpm, wide-open throttle. This fixed-speed regression
  point is distinct from the free-crank absorbing dyno in `rb_cli sound --hold`.

Both use physics at 192 kHz, pipes at 48 kHz, stock calibration, variation 1,
and the core's deterministic initial random states, which are recorded in CSV.
The warm statistics window begins at 6 seconds. Every loop has a finite number
of steps. Non-finite observations, failed startup and insufficient complete
cycles fail the run. Stall observations are explicitly counted, not hidden by
a successful CSV export.

For each cylinder separately, work is sampled when the core closes its work
bookkeeping at overlap TDC. Only complete work intervals inside the warm window
are included. For work `W_i`, cylinder displacement `V_d`, and population mean
`mean`, the reported quantities are

```text
IMEP_i  = W_i / V_d
COV (%) = 100 sqrt(sum((IMEP_i - mean)^2) / N) / abs(mean)
```

This uses the existing definitions from `docs/stage-12c.md`; it adds no physics
equation or calibrated parameter. Computing COV independently for each cylinder
keeps fixed cylinder differences from being mistaken for cycle variation. Engine
brake torque is averaged over completed warm engine cycles; instantaneous and
smoothed RPM ranges are also retained.

At observed exhaust-valve-opening transitions, `min_completion_at_evo` describes
the normalized Wiebe progress of the **burnable charge**, rather than the fraction
of all injected fuel consumed. `partial_burns_observed` counts progress below
`1 - 1e-9`. Fuel/ignition-cut cycles without an established after-spark state
are not counted. These observations cannot identify genuine ignition misfires.

## Float sound audit

After `make`, run:

```sh
python3 tools/audit_s1.py
```

`--cli` selects another native build. `--preset` can select individual engines.
The default records every preset from rest for 30 seconds with `--quality high`,
`--variation 1`, the behind listener, gain 1 and `--float`. The WAVs, block
telemetry, renderer logs and `sound.json` stay under ignored `renders/s1-audit/`.
Exact commands, core-source and renderer hashes, intervals and processing are
recorded.

Historical note: the S1c results below used gain 1. S1d found a changed startup
peak and lowered the audit tool's default recording gain to 0.5 for headroom;
`--gain 1` selects the original level. This changes the analysis recording,
not the engine/browser gain. See `docs/stage-S1d.md` for paired validation.

Each file must contain exactly 30 seconds of finite mono audio at 48 kHz, with
varying audio in each measurement window and renderer diagnostics reporting
zero limited or non-finite samples. The script rejects incomplete output before
the comparison tool can silently truncate an interval. Any non-finite metric
also fails the audit.

The existing `compare_real.measure` algorithm is reused unchanged for seconds
3–8 (historical comparison) and 6–30 (warm window). Float output receives **no
additional processing**: the normal simulated listener, anti-alias filter, DC
blocker, gain and soft limiter are still part of the simulator's output path.
No denoised reference or PCM16 render enters these measurements.

Repetition remains a maximum normalized correlation over a lag range. Harmonics
or multiples of a cycle can win, so the inferred lag is not a verified crank
phase or independent RPM measurement. Mid/high bands search within 3% of the
median low-band lag. The correlation is reported as `10 log10(r/(1-r))` with
the comparison tool's existing numerical bounds.

An unmet 0 to +15 dB low-band target is reported explicitly. Recording valid
baselines succeeds even if that target fails; `--check` makes an out-of-range
measurement fail the command too. The JSON always leaves `s1_complete` false:
these tools cannot approve listening or the remaining ignition model.

## Results and validation

All ten native runs completed, producing 60 per-cylinder rows. The ranges below
are across individual cylinders; RPM ranges use the smoothed free-idle speed
over seconds 6–30. Every preset has zero warm stalled steps.

| Preset | Idle IMEP COV | Held WOT IMEP COV | Warm idle RPM | Held mean brake torque |
|---|---:|---:|---:|---:|
| I4 | 3.83–4.33% | 1.12–1.44% | 780.64–813.24 | 169.136086 N m |
| I6 | 5.36–6.57% | 1.11–1.99% | 674.43–712.29 | 263.604666 N m |
| Cross-plane V8 | 8.49–12.24% | 1.14–1.75% | 634.63–661.75 | 414.803895 N m |
| Flat-plane V8 | 21.40–30.22% | 1.34–1.81% | 976.75–1018.33 | 351.395853 N m |
| Boxer-4 | 5.43–5.95% | 1.17–1.58% | 716.37–768.03 | 172.090433 N m |

The flat-plane V8 has 132 partial completions among 1598 observed warm EVO burn
transitions, with minimum progress 0.874880213. Other presets have no such warm
events in this audit. Stable RPM does not imply small individual-cycle work
variation: the flat-plane cylinders' mean net IMEP is only about 0.68–0.85 bar,
so modest absolute work changes give a large relative COV. This is existing
model behavior, not a new sound improvement or newly introduced regression.

A matched flat-plane run at 96 kHz using the same driver/window produces COV
21.54–30.04%, 134/1598 partial completions and 976.25–1017.65 rpm. Its minimum
completion is 0.889679085. Thus the large COV and partial-burn existence cannot
be explained by changing the physics rate. The earlier S1b scratch summary
sampled burn durations at global cycle rollover instead of observing every
cylinder's EVO; that sampling could miss these events. Do not treat its sampled
maximum as proof that all warm burns complete.

An independent scratch build instrumented the core's actual overlap-TDC, spark
and EVO event sites. It reproduces all eight cylinders' cycle counts, COV and
means, all 1598 burn transitions and 132 partial completions, to the CSV's
printed precision. Production core files were not modified for this check.

The new held torque points differ from S1b's 96 kHz scratch numbers by at most
0.39%, but their rate and complete-cycle selection differ. This is **not**
a matched before/after dyno acceptance result. No core sources changed.

All five sound files contain exactly 30 seconds of finite 48 kHz mono float
audio, and all report zero soft-limited/non-finite samples. Their low-band
repetition, with the existing metric, is:

| Preset | Seconds 3–8 | Seconds 6–30 | Warm window in 0…+15 dB target? |
|---|---:|---:|---|
| I4 | +20.42 dB | +19.91 dB | No |
| I6 | +9.78 dB | +11.36 dB | Yes |
| Cross-plane V8 | +11.63 dB | +10.13 dB | Yes |
| Flat-plane V8 | +6.18 dB | +5.36 dB | Yes |
| Boxer-4 | +18.32 dB | +17.71 dB | No |

Warm mid/high repetitions range from -11.71 to -5.73 dB / -12.40 to -10.90 dB.
The I4's historical +20.42 dB reproduces S1a's +20.4 dB. The I4 and boxer still
miss the low-band target, while the V8 work statistics argue against simply
raising every engine's variation. These renders establish a baseline; the
simulated audio path and engine sound have not been changed in this substage.

The existing native suite passes **151 tests / 2455 checks** with GCC 15.2 on
WSL. All **16 web unit tests**, including the committed WASM source fingerprint,
and the production typecheck/build pass. The core and browser are unchanged;
no WASM regeneration or new browser interaction test is required for this
tools-only substage.

All **15 Python tests** pass, including five new sound-audit guards and the
real-FFmpeg reference-filtering integration test. Invalid diagnostic counts,
incomplete/stereo/wrong-rate audio, silent warm windows and non-finite metrics
are rejected; a failed target is retained as a failure rather than becoming a
false S1 completion. Native harness smoke checks confirm deterministic CSV,
near-zero COV with variation disabled, and rejection of ten invalid or
insufficient-run controls. No physics tests or mutation table are added because
the production physics is unchanged.

Builds and temporary dependencies stayed under `/tmp`; project dependencies
and Claude's tools/settings were preserved. `CLAUDE.md` and `.claude/` remain
active and unchanged. The Graphify graph is refreshed after final source and
documentation changes.

## Reference continuation and next S1 step

Bernardo's four MP3s also gained separate optional filtered listening copies,
with paired raw/filtered comparisons. See `docs/user-sound-references.md` for
their intervals, conservative filter chain and uncertain recording perspectives.
The original MP3s and unfiltered measurement WAVs remain the comparison sources.
Cabin filtering, exterior distance and pass-by motion cannot be collapsed into
a single engine EQ target. There is no supplied boxer reference or new measured
idle target in those rev excerpts.

Next, use these saved baselines to investigate the I4/boxer's excessive low-band
repetition, the V8s' idle work variability and genuine spark-kernel failure.
Do not invent a residual cutoff or random
misfire rate just to lower correlation. A physics change still needs conservation
tests, mutations, matched before/after sound and full dyno comparisons, stable
30-second idles, and Bernardo's listening review. The changed S1b flat-plane V8
rev transient also retains its listening checkpoint.
