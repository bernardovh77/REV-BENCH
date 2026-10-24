# Stage041 — continuous combustion variation and residual validity

Execution begins2026-10-24 atf8def2326870433433a0d04e4d8a82030358697e.
Bernardo authorizes041.039 remains open;041 depends on038/040 and can use
independent numerical evidence. No042 or later implementation is authorized.
Original supplied media, user skills, accepted exhausts/rev limits and Claude
setup are preserved. Evidence stays in ignored`renders/evidence/stage-041/`.

## Predecessor review and physical question

040 separates actual discharge, nonnegative kernel development and chemical
heat. Its independent pre-command oracle, accounting and six deliberate faults
are reviewed. Retain that causal lifecycle. The kernel-delay distribution is
still a bounded surrogate; the previous ECU ignores its mean delay.

Current`variation==0` selects requested AFR, a capped burn and180deg ceiling;
any positive value selects actual trapped AFR, uncapped demand and720deg
ceiling. This changes the mean physical model discontinuously. A stochastic
amplitude control must not select deterministic physics. The current flame
fit also silently floors out-of-domain mixtures/dilution, and idle net-work
COV alone cannot distinguish combustion fluctuations from pumping/low mean.

## Research and intended contract

[Metghalchi and Keck1982](https://james-keck-memorial-collection.unibs.it/JCKeck-papers/MetghalchiKeck-CombustionFlame-48-191-1982.pdf)
provides the existing indolene coefficients and temperature/pressure exponents.
Use the conservative0.8–1.4 equivalence range,0.4–50atm and350–700K envelope
for the power-law screen; the paper's tested diluent fraction stops at0.2 and
its dilution experiments use stoichiometric isooctane. Applying this to our
surrogate and IVC-state proxy remains a modeling assumption, even inside those
numeric ranges. No extrapolation flag is an empirical extinction threshold.

[Maldonado and Kaul](https://doi.org/10.1177/1468087420983087) motivates keeping
burned and unburned species across partial cycles rather than independent
random misfire draws. [Schiffmann/Reuss/Sick](https://doi.org/10.1177/1468087417720558)
supports separate discharge/early development. [The MIT late-spark study](https://dspace.mit.edu/entities/publication/fb14fba3-54d1-4ac6-b95d-1db1a3c3f8b5)
motivates examining exhaust oxidation separately: thermochemical late oxidation
is not equivalent to an arbitrary audible pop. No new exhaust reaction or
calibrated spark-kernel failure law is assumed from these recordings/papers.

Planned implementation: one charge-conditioned default mean model at all
variation amplitudes; an explicit capped diagnostic control independent of
amplitude; pure, observable charge-fit evaluation with preserved safe bounds;
expected-duration/kernel-delay phasing for the ECU; per-cylinder gross/net/
pumping books. Internal model/seed controls are native research controls,
not new persisted build parameters or a public browser feature. Preserve the
existing84-field build contract and public parameter IDs.

The engine owns charge, event, species, heat and work. Diagnostics allocate
nothing or perform I/O in stepping. Keep no-reactants, suppressed discharge,
pending-at-EVO and partial-at-EVO distinct; transported fuel remains physical.
Out-of-fit states are recorded, not converted into universal misfire claims.

## Frozen evidence gates before candidate evaluation

- Independent same-charge controls at variation0,1e-9,1e-6,1e-3 approach one
  deterministic duration/AFR/model in each explicit mode. Same-state tiny
  variation deviations scale with amplitude, without an O(1) jump; use relative
  duration/heat/work differences at1e-9 no greater than1e-6 in short fixtures.
- Fit coefficients match independent equations at in-range points; each
  out-of-range input, numerical floor/cap and invalid-state fallback is visible.
  Range membership never certifies the IVC proxy or a true ignition limit.
- Retain040 zero pre-discharge heat1e-12J, suppression/kernel/EVO ordering,
  mass/species at2e-11 of initial inventory, energy2e-9 of accounted energy and
  crank work1e-9 of accounted work. Gross=net+pumping at existing floating
  tolerances; seeded repeats and passive diagnostic taps preserve physics.
- Five presets, three positive independent engine seeds:30s free idles with
 6s warmup, plus held low/mid/high-speed low/high-load maps. Record per-cylinder
  gross/net IMEP and COV, residual/actual AFR, CA50, fit flags, kernel/partial
  outcomes, retained fuel and guard interventions. No warm stalls or nonfinite
  observations in the declared canonical idle envelope.
- For complete zero-variation, unrestricted held burns, model CA50 and actual
  integrated heat50 agree within0.2deg at192kHz. Finite-sample nominal variation
  statistics and the legacy1% mean-torque control are reported explicitly;
  unmatched real COV/low-band repetition targets remain qualification gates,
  not grounds for arbitrary randomization or relaxed thresholds.
- Plant actual continuity/state/CA50/work/causality faults in isolated copies.
  Run native/Python/web suites, focused sanitizers and WASM parity; rebuild
  WASM after core changes. Compare complete34 float clips, all five long
  coasts and raw-channel metrics; atomically rebuild the single RESULTS page,
  preserving research/reference sections and their audio payloads.

The full warm map, measured combustion/kernel/late-chemistry calibration,
propagation realism, runtime targets and Bernardo's listening acceptance remain
separate claims. No041 completion is recorded before executing its gates.

## Execution

The baseline controlled rich/dilute charge has actualphi1.5 and burned-product
mass fraction0.44 at360K/122kPa. At variation0 it demands255.84deg but selects
120deg; at1e-9 it demands397.880024841deg and selects397.880024854deg. The
discontinuity changes both mixture conditioning and duration policy. The
baseline-compatible fixture/source, output and executable are retained locally.

`combustion.[ch]` now evaluates the existing fit as a pure, explicitly flagged
charge-duration function. It retains the previous numerical floors/cycle ceiling
and distinguishes them from published range membership. `engine.c` always uses
actual trapped mixture and selects its mean policy independently of stochastic
amplitude. Charge is the default; capped is an explicit native research control.
Both approach the same chosen model continuously at zero variation.

The ECU now uses the normalized Wiebe heat50 quantile and conditional mean
duration plus mean nonnegative kernel delay. The duration mean integrates the
bounded lognormal distribution, including its tails, cylinder trim and both
duration bounds. Underlying normal draws are correlated before each marginal
is bounded, so the marginal used by this calculation matches the actual draw.
No variance coefficient is increased to meet a target. The ECU uses expected
statistics, not advance knowledge of this cycle's random outcome. Idle reserve,
cranking spark, overrun retard and the90deg scheduling limit remain explicit.

Per-cylinder gross/net/pumping work now shares the same complete overlap-TDC
window. Internal initial-only seed/model controls reject mid-run changes and
invalid values. `rb_cli sound` exposes these controls for research. Seeds change
engine cycle/valve variation, not the independent mechanical-noise stream. No
public field ID, persisted descriptor,84-field contract, engine geometry,
accepted rev limit, exhaust choice or browser control is changed.

The observer reports actual eligible-fuel heat50, conditional phasing, complete
work windows, mixture/dilution, finite-burn outcomes, reverse valve flow, fuel
transport, temperature screens and all numerical guard counts. Its
`residual_fraction` is the legacy **burned-product mass fraction**, not a
measured total fraction of gas retained from the preceding cycle. Retained
unburned fuel/air stays in the conservative species inventories and affects
actualphi; no gas-origin tracer, stratification, fuel film or detailed chemistry
is claimed. Published isooctane dilution data do not validate this IVC proxy.

Initial full-suite results are retained: five tests reject obsolete expectations
or changed phasing sensitivity. The capped partial-burn control is now selected
explicitly; trapped-mixture tests require actual mixture even at zero amplitude;
the independent spark oracle includes the normalized Wiebe curve. Partial burns
still have to match the independent available-angle/Wiebe calculation at EVO;
improved mean advance can increase completion above the historical0.85 bound.

One loaded-cylinder COV falls below the historical0.8% unit lower bound after
correcting mean phasing. This is retained as a target miss, not repaired by extra
randomness or called real-engine agreement. The unit assay now checks numerical
dose response/repeatability, its existing upper regression budget and mean-work
penalty; the original1–3% loaded/4–8% idle/repetition goals remain reported
qualification targets with their engine/state/reference applicability unresolved.
No S1 or listening acceptance is implied by a green consistency test.

## Executed numerical evidence

The 120-run map uses all five presets, seeds 17/29/61, 192 kHz engine stepping
and 48 kHz pipes. Fifteen free idles run for 30 seconds with a 6-second warmup.
Ninety held cases cover 1,000/3,000/90%-of-rev-limit rpm at throttle 0.1/1.0;
fifteen zero-variation 3,000 rpm wide-open controls complete the map. Research
rev limits are preserved; these cases do not certify durability. The held runs
are four seconds with two seconds discarded, not long-duration calibration.

All 120 pass the frozen accounting, event, work-window and finite-state checks.
There are no warm stalls, early-heat steps, invalid/dropped events, invalid
charge inputs, species/energy repairs, temperature/density floors, invalid
flow/wall/radiation resets, transport/sonic caps or runtime CFL violations.
Maximum normalized residuals are mass 1.865e-12, species 1.580e-12, energy
1.006e-13 and crank work 5.568e-13, below their separate frozen tolerances.
All 15 nominal/zero-variation mean-torque differences are below 1%:
−0.638% to −0.239%. This is a consistency control, not a real-engine torque map.

Across seeds and cylinders, warm idle observations are:

| Engine | Smoothed rpm range | Net IMEP COV % | Gross IMEP COV % | Mean burned-product fraction |
| --- | --- | --- | --- | --- |
| I4 | 787.59–808.41 | 2.28–3.91 | 1.22–2.11 | 0.269–0.276 |
| I6 | 686.98–709.57 | 3.32–5.41 | 1.69–2.78 | 0.297–0.302 |
| Cross V8 | 638.62–661.05 | 5.58–12.40 | 2.94–6.15 | 0.368–0.377 |
| Flat V8 | 979.84–1014.68 | 17.59–26.70 | 11.07–15.60 | 0.465–0.474 |
| Boxer4 | 732.73–759.15 | 1.54–3.47 | 0.81–1.83 | 0.254–0.260 |

The V8 discrepancy is retained, especially the flat V8: a stable governed rpm
does not establish credible combustion variation. Gross work confirms that
pumping alone does not explain it. The flat V8 records 447 partial-at-EVO
events, all in these idles; the other mapped cases record none. No scheduled
discharge failure is inferred from these finite burns. Counts differ slightly
at finite observation boundaries because some scheduled events finish later.

At 3,000 rpm wide open, per-cylinder net-work COV ranges are I4 0.63–1.50%,
I6 0.61–1.26%, cross V8 0.60–1.65%, flat V8 0.60–1.59% and boxer 0.68–1.55%.
Several values miss the historical 1–3% loaded target; several idle values miss
4–8%. These are explicit unqualified targets, not passed acceptance thresholds.
Engine-specific measurements must determine applicable targets before fitting.

### Validity and numerical intervention

The map records 18,433 pressure-range and 20,622 dilution-range flags; 4,797
charges reach the existing dilution floor. No temperature/phi range, speed
floor, demand-cycle cap or spark-limit flags occur in this map. Even an
unflagged IVC proxy remains an assumption about the unburned spark-state gas.
All five idle burned-product fractions exceed the paper's 0.2 dilution range.
The especially dilute flat V8 is a model-validity limitation, not evidence of
measured flammability or a reason to invent random misfires.

Numerical safeguards remain observable: 375,936,673 equalisation-cap calls,
740,203 property extrapolations and 482,937,701 scalar gas-inversion fallbacks
occur across these differing run lengths. The scalar path restarts the existing
inversion and is not automatically a repair. Physical choking and donor/limited
reconstruction are counted separately in the full report. None of these call
counts is a cycle probability, and their acoustic neutrality is not established.
Solver/property qualification remains with 043/047/059.

### Heat-release phasing

An independent observer built against each revision measures actual integrated
heat reaching half the eligible charge, rather than reading the ECU's prediction.
For a 2-second held 3,000 rpm/throttle-0.6/default-seed comparison after 0.2 s:

| Engine | Before mean heat50, deg ATDC | After | Samples per revision |
| --- | --- | --- | --- |
| I4 | 9.953 | 8.368 | 180 |
| I6 | 9.681 | 8.092 | 270 |
| Cross V8 | 9.595 | 8.023 | 360 |
| Flat V8 | 9.629 | 8.028 | 360 |
| Boxer4 | 9.961 | 8.369 | 180 |

This moves the mean toward the existing 8-degree scheduling target. It does
not prove that 8 degrees is optimal for these real engines. The stochastic
standard deviations remain about 3.4–3.6 degrees. Separate native zero-amplitude
fixtures satisfy the frozen 0.2-degree actual-heat criterion; the mean formulas
also match independent quadrature and the normalized Wiebe inverse.

All five independent pre-discharge observers still record exactly zero early
heat; all 25 inherited ignition scenarios pass. Stage 040's causal correction
therefore survives the new duration/ECU behavior.

### Late chemistry decision

Transported unburned fuel, reverse intake/exhaust flow, partial burns and warm
outflow exist in the map. Across each engine's collection of runs, recorded
fuel leaving the open system is approximately 10.4–25.7 g, while the maximum
post-step positive exhaust-flow temperature screen is 1,890–1,929 K. These
quantities have different sampling/time bases: they do not prove co-located
reactants, oxygen availability, ignition or a chemical timescale. The current
three-species surrogate cannot resolve the intermediates in the late-spark
research. No calibrated exhaust oxidation or extinction mechanism is added.

Before adding chemistry, 058/059 must establish species/temperature/residence
histories, oxygen and heat balances, reaction-rate sensitivity and matched
coast evidence. A future reaction source must consume transported reactants
and return heat conservatively; independent sound pops are rejected. The
evidence makes this worth investigating, without claiming its audible size.

## Validation and rejected controls

| Check | Fresh result |
| --- | --- |
| Full native suite | 212 tests, 695,818 checks pass |
| Focused combustion/ignition suite | 23 tests, 2,100 checks pass |
| Seeded operating map | 120 cases pass; 15 stable warm idles |
| ASan/UBSan focused suite | 14 tests, 657,137 checks pass; leak detection disabled as explained below |
| WASM focused regression | 19 tests, 657,309 checks pass |
| Python behavior suite | 72 tests pass, no skips |
| Web unit suite | 17 tests pass |
| Web production build | Type checking and build pass |
| Shared build contract | All five presets retain the same 84 fields and round trip |
| CLI controls | Identical seeded repeats, different nominal seeds, identical zero-amplitude output across seeds; invalid options rejected before output |
| Deliberate faults | All seven compile and are caught by the named independent checks |

Faults cover amplitude selecting the mean model, ignored trapped mixture,
hidden dilution extrapolation, omitted mean kernel delay, wrong gross-work
sign, ignored seed and heat at IVC without actual discharge. The isolated-copy
runner now excludes local media and user skill folders so it cannot copy
gigabytes of private reference files into every mutation workspace.

Retained failed attempts include the obsolete expectations described above,
the first sanitizer run (LeakSanitizer cannot operate under this host's ptrace),
a Windows WASI build targeting an unsupported Linux `/tmp` output path, and
the first offline-playback check's fixed 90 ms readiness assumption. The
sanitizer rerun enables address/undefined-behavior checks with leak detection
off; it is not a leak-test pass. The corrected WASI build uses a relative
project output directory. Browser playback now waits up to three seconds for
actual advancement. No production behavior or physical threshold is relaxed
to fix these environment/harness failures.

The rebuilt WASM is 172,343 bytes; its checked source fingerprint is
`337b54262e61e4ba6748056a8ab148741a9633491c92f81d0c4ed2bf153028ac`.
Core C11 builds retain strict floating-point contraction settings. The public
API/parameter IDs and persisted descriptor are unchanged. New observations and
native research controls live in internal headers, not the browser contract.

## Complete listening checkpoint

`renders/RESULTS.html` contains the latest 34 current examples plus retained
experimental/reference sections: 55 players total, four seek controls, eight
research plots and three reference-view panels. All play offline with no
page errors or HTTP requests; exclusive playback and seek behavior pass.
The current raw measurement set totals 336.883 seconds, preserves original
mono/stereo channel layouts, and has a maximum absolute sample of 0.53725.
Every render log reports zero limited and nonfinite samples. Thirty-three
engine clips change; the test tone remains byte-identical. Page previews retain
the existing per-clip static gain policy; use raw files for level comparisons.

At held 3,000 rpm, raw RMS changes for I4/I6/cross/flat/boxer are respectively
−0.122/−0.098/−0.002/+0.091/−0.087 dB. These small changes are not evidence of
improved timbre. The pre-lift (−0.8…−0.2 s) to early-coast (+0.2…+0.8 s) raw
mean-channel level steps are:

| Engine | Before, dB | After, dB |
| --- | --- | --- |
| I4 | +4.547 | +5.337 |
| I6 | −0.334 | −0.422 |
| Cross V8 | −5.226 | −5.171 |
| Flat V8 | −1.713 | −1.849 |
| Boxer4 | −2.168 | −2.037 |

Different speed trajectories make these descriptive pedal-lift observations,
not matched-rpm causal comparisons. The I4 rise becomes larger and remains
an explicit 058 investigation, not a successful deceleration fix. Accepted
I6/flat/cross exhausts and raised research rev limits are unchanged. Listening
acceptance and the overall S1 realism gate remain open.

### Three-seed stereo repetition

Fifteen additional 30-second float stereo idles use seeds 17/29/61, the same
high quality, asphalt scene and 0.5 native gain. Each channel is measured
independently over 6–30 seconds; an antiphase test prevents mono cancellation.
Legacy correlation searches 75–267 ms over half-second frames and tests the
upper bands within 3% of the selected low-band lag. The selected lag may be a
harmonic; it is not an independent rpm estimate or a universal realism score.

| Engine | Below 500 Hz, dB | 0.5–2 kHz, dB | 2–8 kHz, dB |
| --- | --- | --- | --- |
| I4 | +21.71…+22.78 | −5.96…−5.66 | −11.88…−11.48 |
| I6 | +13.37…+14.97 | −7.92…−7.11 | −12.72…−12.03 |
| Cross V8 | +6.67…+7.82 | −8.34…−6.73 | −6.84…−6.08 |
| Flat V8 | +9.04…+9.58 | −9.32…−8.81 | −9.92…−9.62 |
| Boxer4 | +18.05…+19.93 | −8.44…−8.03 | −12.01…−11.61 |

The I4 and boxer miss the historical 0–15 dB low-band screen in all six
channel/seed observations. The other three engines satisfy that screen, which
does not establish physical realism or validate their IMEP statistics. All
15 recordings are finite, unclipped and report zero limited/nonfinite samples.
No variance coefficient or physical constant was fitted to these observations.

## Processing cost and reproducibility

Three interleaved before/after native benchmark rounds each take the best of
three 0.5-second held-engine windows after a 0.3-second warmup. Both builds use
GCC C11/O3, no LTO, strict contraction, 192 kHz physics and 48 kHz pipes. The
benchmark's stock descriptors and zero variation are unchanged commands, but
zero variation now follows charge physics, so this is not an isolated operation
overhead measurement. Concurrent audio jobs and a shared host limit precision.

| Engine | Before median seconds per simulated second | After | Median change |
| --- | --- | --- | --- |
| I4 | 0.872 | 0.869 | −0.34% |
| I6 | 1.454 | 1.420 | −2.34% |
| Cross V8 | 1.740 | 1.794 | +3.10% |
| Flat V8 | 1.493 | 1.512 | +1.27% |
| Boxer4 | 0.973 | 0.967 | −0.62% |

These measurements do not establish a speedup or production real-time delivery.
Expected-duration integration runs at charge closing, not every gas timestep;
no allocations or diagnostic I/O were added to stepping. Whole-audio timings
are retained separately with the seeded stereo render logs: native complete
stereo idle rendering ranges from 0.39× to 0.98× real time across these presets
and shared-host runs. This path remains too slow for general real-time use.
Browser/worklet
performance remains a later gate, not a conclusion from native core timing.

Evidence is under ignored `renders/evidence/stage-041/`: baseline revision and
file hashes, independent observer sources/results, probe commands and binary
fingerprints, every map result and guard count, raw audio metrics, test/fault/
build logs, cost rounds, browser proof and preservation records. Source and
raw hashes are checked again after all generated work. The known Graphify
`rb.h:415` extraction warning remains; graph navigation is not physics proof.

Reproduction, using an existing compiler/analysis environment:

```sh
make BUILD=/tmp/rev-bench-041 LTO=0 all \
  /tmp/rev-bench-041/rb_tests \
  /tmp/rev-bench-041/combustion_variation_probe
/tmp/rev-bench-041/rb_tests
PYTHONPATH=tools .venv-analysis/bin/python tools/check_combustion_variation.py \
  --probe /tmp/rev-bench-041/combustion_variation_probe \
  --out renders/evidence/stage-041/maps --workers 2
MPLBACKEND=Agg PYTHONPATH=tools .venv-analysis/bin/python \
  tools/combustion_repetition.py --cli /tmp/rev-bench-041/rb_cli \
  --out renders/evidence/stage-041/repetition --workers 2
MAKEFLAGS='-j2 LTO=0' .venv-analysis/bin/python tools/plant_bugs.py \
  041 /tmp/rev-bench-041-faults all
RB_REFERENCE_FFMPEG=/path/to/ffmpeg MPLBACKEND=Agg PYTHONPATH=tools \
  .venv-analysis/bin/python -m unittest discover -s tools -p 'test_*.py' -v
.venv-analysis/bin/python -S tools/render_results.py \
  --cli /tmp/rev-bench-041/rb_cli --build /tmp/rev-bench-041
```

The before/after comparison additionally uses the frozen baseline executable
from revision `f8def2326870433433a0d04e4d8a82030358697e` and the same render
commands. Temporary duplicate before WAVs may be removed after their measured
hashes are saved; regenerate them from that revision for a fresh comparison.
Current originals, original references, source intervals, CSVs and metrics stay
local. No reference is fitted or admitted during this stage.

## Review, bounded outcome and handoff

Stage 041 completes the declared continuity/observability implementation and
numerical characterization after reviewing 040. All frozen numerical gates,
five-preset/multiple-seed observations, deliberate faults, rebuilt WASM and the
complete sound checkpoint execute successfully. The reference-qualified COV,
repetition, kernel/residual/late-chemistry and listening claims are explicitly
unresolved; they were not numerical exit gates silently changed after testing.
The new flags and gross/net books expose those questions rather than solve
them. No overall S1/S1e/S2/S4 or full engine realism acceptance is claimed.

All 93 protected non-current-engine/core files match the preflight hashes,
including accepted exhaust/preset sources, Claude setup, locked reference
metadata and retained research recordings. All six newly supplied original
videos also match their prior hashes and stay untracked. The 34 new raw clips
match the published manifest, and the checked page hash is
`7f1e926f81a60ad74103b646f0a997f2d59f0baef0192bab256fec7a31d7bb9c`.
Verified duplicate before WAVs are removed after comparison; current originals,
scientific records and references remain local. User skill files are unstaged.

Graphify is refreshed to 2,845 nodes, 7,772 edges and 160 communities. Its
existing parser warning and inferred edges remain explicit. No Claude setup
or packages are installed/removed to accomplish this refresh.

Stage 039 remains open for matched reference acquisition; none of its six
new candidates is silently admitted. Stage 042 is the next planned development
step after review, with no implementation started here. Retain the I4 coast
rise, I4/boxer repetition, high V8 idle dilution/COV, fit extrapolation, gas
inversion/property interventions and unknown exhaust chemistry for 043/047/
055/058/059 and acceptance 063/098. Save/sync this checkpoint and hold for
Bernardo's feedback/instructions.
