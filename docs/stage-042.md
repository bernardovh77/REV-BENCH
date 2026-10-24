# Stage 042 — Actual fuel-injection events and acoustic triggers

Execution starts 2026-10-24 at `f412b727a52d93aafa17e11892d2174914028407`.
Bernardo authorizes 042 only. Review 041's continuity/mean-phasing evidence,
unqualified V8 dilution/COV, I4/boxer repetition and I4 coast rise first. Those
findings remain open; this stage preserves the charge model and ignition fix.
039 still needs matched references. Claude setup and user skill/media files
remain unchanged. Local evidence: ignored `renders/evidence/stage-042/`.

## Physical question and research

The current mechanical renderer predicts a click at intake opening from global
ignition/cut flags. The actual fuel addition is at intake closing and only tops
up a deficit after retained fuel is counted. A timing/state change between
those events can create a false click or hide a delivered dose. The engine's
actual mass/energy transfer must be the only source authority.

[Bosch's PFI description](https://www.bosch-mobility.com/en/solutions/powertrain/gasoline/gasoline-port-fuel-injection/)
places real port injection upstream of the inlet valve and describes detecting
actual needle opening/closing. [Allocca et al., SAE 2019-24-0058](https://doi.org/10.4271/2019-24-0058)
experimentally distinguish needle mechanics from fuel-flow effects using dry
and fueled GDI tests. [SAE J1832's published scope](https://doi.org/10.4271/J1832_202302)
includes separate low-pressure PFI flow, electromechanical and noise bench
tests. These sources do not establish our presets' pulse widths, acoustic
amplitudes, rail dynamics or per-engine needle transfer functions.

Our existing model adds homogeneous fuel vapor directly to the trapped control
volume at IVC. It is an instantaneous charge-fueling surrogate, not a resolved
PFI spray/port film or GDI needle schedule. Retain its mass/energy/timing here
and publish an explicit delivered-dose event at that existing transfer. One
collapsed acoustic excitation per positive dose retains the current modal
amplitude; do not invent opening/closing pairs, pulse width, dry actuation,
fuel-pressure dependence or a calibrated real injector sound. In this model,
zero delivered dose means no modeled actuation and no new injector excitation.
Existing modal ringing can continue after the final excitation.

## Contract and ownership

- Engine owns per-charge fueling decisions, actual added kilograms/sensible
  joules, cylinder/charge ID and engine tick plus fraction in `[0,1)`.
  Distinguish delivered, ignition disabled, invalid/disabled AFR, DFCO,
  no-air/invalid state and sufficient retained fuel. A fresh attempt record
  is emitted only when that cylinder crosses IVC.
- A fixed engine-local array contains the current step's attempts, at most
  one per cylinder. Positive delivered events are created beside the actual
  transfer, not reconstructed from flags in the sound renderer. No allocations,
  callback I/O or dependence on a diagnostic trace's remaining capacity.
- A separate optional bounded native trace records attempts and dropped
  diagnostic records explicitly. Full/zero-capacity traces cannot change gas,
  random streams or sound. This is internal observation, not the future public
  event transport ABI (061); the 84-field descriptor/API stays unchanged.
- Mechanical noise consumes a completed engine step once, even if queried
  repeatedly. Disabling its layer discards that step's excitation; enabling
  later or initializing a new noise state must not replay old doses. Actual
  injected mass is also accumulated as a diagnostic. A waveform renderer
  sampling only the latest step cannot catch up skipped engine steps.
- Source event time retains the substep fraction. The existing grid-based
  modal renderer excites after the engine step, so its onset is causal and
  quantized by at most one physics interval (5.21 microseconds at 192 kHz),
  before the existing listener and filters. No separate crank-clock trigger.

## Frozen gates before candidate evaluation

1. Independent per-step `fuel_in`/sensible-energy differences equal the sum of
   actual dose-event masses/energies: absolute mass error at most 1e-15 kg and
   energy error at most 1e-9 J for these small fixtures. A click occurs once per
   positive dose, in the same completed interval; no dose means no new click.
   Event cylinder/charge/IVC phase and fraction match independently calculated
   crank crossings, including exact-boundary and 720-degree wrap cases.
2. All five presets: startup, ignition off/on restart, AFR disabled, retained
   fuel satisfying/exceeding the target, natural DFCO cut/resume, hard/soft spark
   cut and normal loaded running. Spark suppression alone does not cancel a
   real injected dose. Include changes between IVO and IVC, positive but tiny
   deficits and doses with unequal cylinder trims. No random extra clicks.
3. Repeated noise calls, late noise initialization, disabled layers, stopped
   crank and trace overflow/capacity-zero cases cannot duplicate/replay/fabricate
   doses. Passive tracing and sound do not change the engine trajectory.
4. Retain 040/041 causal heat and books. Independent whole-system mass/species
   error <2e-11 initial mass, energy <2e-9 accounted energy and crank work
   <1e-9 accounted work. Use multiple seeds 17/29/61 at the native 192 kHz /
   48 kHz rates; focused boundary/rate cases also use 96/384 kHz.
5. Plant real detached-timing, missing/phantom/incorrect-dose, replay and
   disabled-layer faults in isolated copies; the independent checks must fail.
   Run native/Python/web suites, focused ASan/UBSan and WASM tests; rebuild
   WASM. Measure complete sound path cost separately from observation overhead.
6. Rebuild and measure all 34 current float clips and five long coasts at the
   same commands/gains. Preserve original channels and reference/research
   sections in the single offline 55-player RESULTS page. Zero nonfinite or
   limiter activity; verify offline playback and protected source/raw hashes.

Stage 042 is not complete until its numerical gates execute. Real injection
hardware/spray/needle calibration, overall sound realism, runtime qualification
and Bernardo's listening acceptance remain distinct later claims.

## Implementation and predecessor review

041's mean combustion policy, conditional phasing, ignition lifecycle and work
books are retained. Five independently compiled before/after held-engine
observers give exactly equal fuel use, heat release, gas mass/internal energy,
crank angle and cycle-random state. Their 0.3-second runs use 3,000 rpm,
throttle 0.6, variation 1, seed 17 and the declared 192/48 kHz rates.

The old aggregate acoustic counter identifies 165 intervals with an unearned
click and 165 with a delivered dose but no click across these runs. After the
fix both totals are zero. Flat-V8 aggregate counts were already coincident:
an IVO on one cylinder can coincide with another cylinder's IVC, so that
aggregate check alone cannot validate cylinder identity. The independent
per-cylinder charge/IVC/fraction tests cover this blind spot.

`injection.[ch]` adds an internal caller-owned, bounded diagnostic trace and
explicit attempt reasons. `engine.c` creates the source record beside the
actual mass/energy transfer. The fixed engine-local source array is always
available, including when a trace is absent or full. It is reset every engine
step. At most one IVC per cylinder is processed by the existing stepper; this
stage's envelope is 96–384 kHz and canonical engines, not arbitrarily large
crank steps or a new multi-crossing integrator.

`noise.c` consumes positive current-step doses once, independently of current
ignition/fuel-cut flags, reconstructed crank events or trace capacity. Later
control changes cannot retroactively cancel a dose or invent a skipped one.
The common sample path in `rb_sim.c` already steps engine then noise; its
actual rendered output is tested. Reinitializing noise attaches at the present
tick without replay. Muting drops that interval's injector excitation; the
current observer is not a historical queue for hosts that skip physics steps.

No stochastic draw, needle amplitude, block/rail mode, fuel-pressure law,
exhaust selection, engine descriptor or public parameter ID is changed. The
same 84-field contract round-trips all five presets. The native/WASM prepared
state grows; hosts already allocate using the core's reported size. A rebuilt
engine must still reinitialize its sound state, as the production rebuild does.

For supported valid states, fueling math is unchanged. Unsupported nonfinite
requests/charge inputs now have explicit disabled/invalid outcomes and cannot
be mistaken for a delivered source. Invalid-state trace fields may retain an
invalid upstream target for diagnosis; consumers must check its reason, and
061 must reject or sanitize such data before public serialization. These are
internal records, not a promised JSON event ABI.

## Scenario results

The committed probe/driver runs seven cases per engine and seed: loaded,
startup/restart, AFR disabled/resumed, DFCO/resume, hard spark cut/resume,
soft spark cut, and ignition changes every 7 ms between valve events. All
five engines and seeds 17/29/61 give **105 passing runs** at 192/48 kHz.
Loaded/control cases last 0.6 seconds; startup/restart lasts 0.8 seconds.
The large-inertia cut fixtures, altered limiter thresholds and 30 ms DFCO
transition are explicitly diagnostic controls, not new preset calibrations.

| Observation | Result |
| --- | --- |
| Fueling attempts | 9,426 |
| Positive delivered doses / new injector excitations | 7,488 / 7,488 |
| Ignition-off attempts | 822 |
| AFR-disabled attempts | 450 |
| DFCO-skipped attempts | 666 |
| Physics steps in fuel cut / new injector clicks in cut | 633,615 / 0 |
| Observed cut-to-resume transitions | 15 |
| Invalid, dropped, missing or phantom event intervals | 0 |
| Pre-discharge/pre-kernel heat steps | 0 |
| Maximum per-step dose mass mismatch | 4.337e-19 kg |
| Maximum per-step sensible-energy mismatch | 5.596e-14 J |

The short natural scenarios do not happen to reach no-air, invalid-input or
sufficient-retained-fuel decisions. Those are separately forced in independent
five-engine fixtures, including a 1e-14 kg positive deficit. Retained fuel
burns with zero new injection/excitation. A free-crank hard-spark-cut control
still delivers its independently required positive dose. Nonfinite trim
requests are explicitly invalid, and zero/NaN AFR requests do not inject.
Exact interval-start/end cases, wrap and 96/192/384 kHz cases pass.

There are 666 hard and 706 soft suppressed sparks, separately from fueling
attempts. Six no-reactant ignition outcomes and 1,283 partial-at-EVO outcomes
remain physical observations; they are not injection failures or random new
misfires. Event counts differ slightly at finite observation boundaries because
scheduled events can finish after the recording ends.

Maximum normalized whole-engine residuals are mass 5.023e-14, species
3.008e-14, energy 1.948e-14 and crank work 4.964e-12, below their distinct
frozen tolerances. No invalid observations, mass/species/energy repairs,
temperature/density floors, invalid flow/wall/radiation resets, sonic/mass
caps or CFL violations occur in the 105 cases. Existing interventions remain
visible: 12,291,558 equalisation caps, 639,808 property extrapolations and
74,955,397 scalar inversion fallbacks. Choking and donor/reconstruction counts
are retained separately. These are call counts, not failure probabilities or
proof of acoustic neutrality; 043/047/059 still own solver/property qualification.

## Independent tests, faults and environment

| Fresh check | Result |
| --- | --- |
| Full native suite | 217 tests, 4,842,144 checks pass |
| Final native injection/ignition/noise focus | 10 tests, 4,801,796 checks pass; final hard-spark-cut extension also passes separately |
| Scenario map | 105 cases pass, three seeds per engine |
| WASM combustion/ignition/injection/noise regression | 16 tests, 4,803,470 checks pass |
| Latest WASM injection-only edge cases | 5 tests, 4,146,521 checks pass; final hard-spark-cut extension rerun separately |
| ASan/UBSan ignition/injection | 9 tests, 4,801,696 checks pass; final injection edge cases and hard-cut extension rerun |
| Python behavior suite | 76 tests pass, no skips |
| Web unit suite / production build | 17 tests pass; type checking/build pass |
| Five-preset build contract | 84 fields unchanged, round trips and adverse rejection pass |
| Deliberate compiled faults | All eight caught by their named independent tests |

The full native suite runs before the last test-only boundary/consumer
extensions; focused native/WASM/sanitizer reruns cover those extensions using
the same unchanged production core. The counts above describe actual runs,
not an invented combined suite size. Address and undefined-behavior checks
use `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer is incompatible with
this host's ptrace; this is not a leak-test pass.

Faults add a detached IVO click, drop a real source, halve its recorded mass,
reverse sensible energy, discard substep time, replay a dose, replay a muted
interval and make source delivery depend on trace capacity. Per-step physical
books and independent crank/source assertions catch the first five; consumer
and bounded-trace tests catch the others. No mutation is a compile failure or
an unapplied text replacement counted as detection.

Early harness failures are retained: the baseline observer initially omitted
the public preset-name declaration; the render-path test initially used an
invalid pipe-rate configuration and then expected a status code instead of
the API's rendered-frame count. Correct configuration and return-contract
checks pass. A probe indentation warning is fixed and its full 105-case map
reproduced under the new binary fingerprint. An initial web-copy command used
a relative path from the temporary web folder; only the corrected absolute
copy of the current core/WASM is used for the claimed web checks. No physical
threshold, reference target or sound constant is relaxed to turn these green.

The rebuilt production WASM is 173,274 bytes, with source fingerprint
`cca7ec993b7588bb39de15bb2fb2861d1d8bc0130d3083068b68ceefd3617f24`.
The unqualified 041 calibration targets, S1 listening gate, 039 reference gap
and unresolved exhaust chemistry are unchanged.

## Complete audio checkpoint and preservation

The rebuilt single `renders/RESULTS.html` retains all 34 current float clips
and five eight-second loaded coasts, totaling 336.883 seconds. Original channel
layouts, native gains, accepted exhausts and research rev limits are preserved.
Every engine render reports zero limited/nonfinite samples; raw peak is
0.537246 or less. All 55 page players advance offline, exclusive playback and
four seeks work, and eight research plots plus three reference-view images
load with no page errors or HTTP requests.

Twenty-nine current raw WAVs change. The test tone and four flat-V8 examples
remain byte-identical; global modal excitation can already coincide across
different cylinders in that layout. Across the 34 clips, maximum absolute
whole-clip RMS change is 0.00104 dB and maximum upper-band-share change is
0.249 dB. This is a source-correctness fix, not evidence of a perceptible timbre
improvement or realistic needle acoustics. Current preview gains retain the
existing static normalization policy; measure the raw files, not previews.

All 71,872 rows in 15 trajectory CSVs have exactly equal engine/control/time
columns and row counts. Their `out_peak`, `out_rms` and `spl_dB` columns can
change because they measure the corrected sound; an initial whole-file byte
comparison correctly detected those differences and was not treated as a gas
regression. The I4 early-coast rise remains +5.337 dB, I6 −0.422 dB, cross
V8 −5.171 dB, flat V8 −1.849 dB and boxer −2.037 dB. No deceleration fix is
claimed; 058 retains that investigation.

All 97 protected files outside intentional current-engine/audio changes match
the preflight hashes. All six newly supplied source-video fingerprints match.
All 21 embedded experimental/reference audio payloads match the unchanged
registered source sections. The payload audit uses an HTML parser, including
single-quoted attributes and nested source elements, rather than an incomplete
regular-expression count. Current raw hashes match the manifest; the checked
page SHA-256 is
`af82a93d89fc219f4bd1b1d6e1b46505fa5052a435aa542b8ea58a17598cd8c9`.
Claude setup, reference locks/catalogs, original recordings and user skills are
preserved. Original media and generated audio remain unstaged/ignored.

## Processing cost and memory

Native engine/noise/sim storage changes from 368,776/33,784/1,420,320 bytes
to 369,944/33,800/1,421,504 bytes: +1,168/+16/+1,184 bytes. The optional
caller-owned injection trace is bounded to 256 records and allocates nothing
in the core. Source records always exist, independently of that optional tap.

Three interleaved two-second whole stereo startup/idle rounds use high quality,
asphalt, seed 17, gain 0.5 and unfiltered float outputs at both revisions.
Printed CPU timing has 0.1-second precision and the shared host also runs other
validation. The ratios of per-engine medians range from −13.33% to +14.81%;
these noisy results do not isolate event overhead or prove a speedup/regression.
They are retained, including the initially slower I4/boxer observations.

After the main render completes, longer five-second I4/boxer controls repeat
three interleaved rounds. I4 CPU seconds before/after are
`[3.5,4.1,4.4] / [3.5,4.1,4.1]`; boxer is
`[4.1,4.4,4.6] / [4.5,4.4,4.3]`. Both median differences are 0%, while paired
differences still range from −6.82% to +9.76%. No systematic regression is
established, and zero overhead is not demonstrated. These short native idle
controls do not qualify the whole engine/trajectory/browser for real time.

Separate 0.6-second loaded observer runs toggle only injection tracing, with
the gas diagnostics, ignition tap, fixed source list and mechanical synthesis
still active. Three paired rounds per engine give identical non-timing outputs;
median timing differences span −2.81% to +4.54% on this host. This is not a
measurement of the entire diagnostic system's cost. Runtime envelope/hardware
qualification remains 062/069/099; no quality setting is lowered here.

## Reproduction, final review and handoff

Evidence stays in `renders/evidence/stage-042/`: baseline revision/source and
raw hashes, baseline-compatible observer source/results, every scenario command
and binary fingerprint, numerical intervention counts, tests/fault logs, cost
rounds, raw-channel comparison, trajectory/preservation and offline browser proof.
Essential results and rejected controls are summarized above for other checkouts.

```sh
make BUILD=/tmp/rev-bench-042 LTO=0 all \
  /tmp/rev-bench-042/rb_tests /tmp/rev-bench-042/injection_scenarios_probe
/tmp/rev-bench-042/rb_tests
PYTHONPATH=tools .venv-analysis/bin/python tools/check_injection_events.py \
  --probe /tmp/rev-bench-042/injection_scenarios_probe \
  --out renders/evidence/stage-042/scenarios --workers 2
MAKEFLAGS='-j2 LTO=0' .venv-analysis/bin/python tools/plant_bugs.py \
  042 /tmp/rev-bench-042-faults all
MPLBACKEND=Agg RB_REFERENCE_FFMPEG=/path/to/ffmpeg PYTHONPATH=tools \
  .venv-analysis/bin/python -m unittest discover -s tools -p 'test_*.py' -v
.venv-analysis/bin/python -S tools/render_results.py \
  --cli /tmp/rev-bench-042/rb_cli --build /tmp/rev-bench-042
```

The same strict C11/O3/no-LTO native flags and source fingerprint are recorded
for measured programs. WASM retains SIMD/O3/LTO and disabled contraction; its
same-source tests are separate runtime evidence, not native timing estimates.
Reproduce before/after controls from the frozen executable or revision
`f412b727a52d93aafa17e11892d2174914028407`; temporary duplicate baseline and
timing WAVs are removed only after hashes/measurements are saved. Current raw
examples, references, CSVs, logs and scientific metrics remain local.

Graphify is queried before exploration and refreshed after the final source/
test edits: 2,904 nodes, 7,945 edges, 161 communities. The known `rb.h:415`
parser warning and inferred-edge limitations remain explicit. No Claude setup
or tool packages are replaced.

Stage 042 completes its declared actual-source/mass/acoustic-causality gates.
It does not establish calibrated PFI/GDI timing, needle opening/closing forces,
rail pressure/flow, port film, evaporation, physical per-engine injector modes
or SPL. The existing sensible-internal-energy vapor addition is a stated
surrogate, not measured injector-stream enthalpy/evaporative cooling. Those
physical/calibration assumptions must be resolved for the relevant engines
before 055/059/063 promotion; 061 owns public event serialization/transport.

Preserve 041's V8 dilution/COV, I4/boxer repetition, I4 coast rise, property/
solver interventions, missing late chemistry and matched-reference limits.
039 remains open; no new candidate is admitted or fitted here. S1/S1e/S2/S4,
real-time delivery and Bernardo's listening acceptance remain open. Save/sync
this checkpoint, then hold. Review 042 before the next authorized stage 043;
no 043 implementation is included in this work.
