# Sound and engine-physics audit — 2026-10-22

**Review checkpoint: hold for Bernardo's instructions.** Reviewed production
commit `bd98ab9438e6165f8aea669cebd8d2c6670508a0`. The current sound is a useful,
improved checkpoint, but it is not yet a validated reproduction of these real
engines. This audit records **27 findings: nine confirmed implementation or
numerical issues and eighteen model/validation items requiring attention**.
No production physics, tuning, browser code or recordings were changed.

P1 means resolve or explicitly validate before claiming the affected realism
goal complete. P2 means a relevant approximation or coverage gap to address in
the subsequent research work. There is no claim that every listed limitation
is audible, or that one finding alone explains the robotic coast.

## Scope and evidence

Reviewed the sound roadmap and handoffs for stages 10, 11, 12, 12b, 12c, 13,
S1a–e, S1, S2/S2b, S3, S4, 13b-a, 13b, 13b-p, 13b-r and planned S5, together
with their supporting gas/combustion/valve/crank/dyno paths from stages 3–9.
Inspected the pipe transport, outlet load, source extraction, procedural flow
noise, mechanical modes, listener/scene processing, render driver, reference
workflow and analysis/test assumptions. Source locations below refer to the
reviewed production commit, not line numbers in a future fix.

The new read-only diagnostic is `tools/sound_physics_audit.c`. It links the
unchanged production core and makes no WAVs. Local outputs, build logs, hashes
and test logs are under `renders/evidence/sound-audit-2026-10-22/`. Historical
sound/COV/convergence observations below are identified as retained evidence;
they were not all re-recorded during this review. Existing current/reference
audio is preserved, with no older render sets recreated.

Fresh checks: **175 native tests / 4,444 assertions**, **29 Python tests**
(including real FFmpeg integration, no skips), **17 web unit tests**, and the
diagnostic under **ASan/UBSan** all pass. Leak detection is disabled for the
existing sandbox ptrace limitation. These passing tests coexist with the
findings; they do not certify acoustic realism. No new browser end-to-end or
WASM physics run is claimed in this review; production core/web are unchanged.

## Confirmed implementation and numerical issues

### D01 — P1: modeled burning can precede its recorded spark

`core/src/engine.c:997–1009,1359–1362`: the zero-centered start variation can
schedule `start_angle` before `spark_angle`, and `spark()` actually runs at
that varied start. There is no separate physical early-spark/preignition model.
The nominal ECU spark, actual ignition event and flame-development delay are
therefore conflated. This changes pressure rise, work, exhaust excitation and
the interpretation of ignition/limiter telemetry.

Fresh I4 probe: held 3000 rpm, variation 1, ambient initialization, twenty
cycles; cylinder 0 starts before the commanded spark in **10/20** cycles,
up to **6.842° early**, with **32.169 J** released before that spark over the
sample. This is a causality diagnostic, not a warm-idle event-rate estimate.
Research a nonnegative flame-development delay after a distinct spark event;
if actual spark jitter is needed, model and record it separately. Optical
ignition studies [S2, S3] support separating these processes.

### D02 — P1: the internal-jet noise model misidentifies choking

`core/src/noise.c:206–215`: velocity uses upstream reservoir density and
temperature, then treats `U/c_upstream` as the throat Mach number. Density
changes during acceleration through a compressible restriction. At a physically
choked air nozzle, the code infers **M=0.578704**, while the throat is **M=1**
[S1]. Its sonic clamp is not reached by the normal choked nozzle mass flow.

The independent 2-bar/300-K → .2-bar fixture yields **0.244612 W** from the
noise model versus **9.377849 W** using sonic throat conditions in its own
chosen efficiency law: **−15.836 dB of source power**. This is not a measured
15.8-dB error in the complete engine audio: coefficients were empirically
calibrated and the law itself is approximate. It demonstrates wrong operating
regime/scaling, which needs correction and recalibration together. Current
`tests/test_noise.c:131–139` forces mass flows at 2×/4× `rho_up A c_up`, beyond
the physical nozzle maximum, so it does not test real choking integration.

### D03 — P1: added exhaust noise does not follow shared X/Y routing

`core/src/noise.c:176,231–235`: each cylinder's generated noise is sent to
one collector-numbered outlet. It does not scatter through the newer shared
junction. The gas-pressure layer does traverse the physical network.

Fresh one-valve fixtures give tail0/tail1 RMS **.00300817/0 Pa** for the
cross V8 and **.000851735/0 Pa** for the boxer. All boxer cylinders belong to
collector 0, so their added valve noise never reaches tail1 despite the real
modeled Y split. Both V8 X junctions likewise fail to mix this noise layer
across banks. Outlet coherence, stereo balance and timbre can disagree between
layers. Research a source-to-outlet transfer/scattering treatment, preserving
phase and power; the current test even requires the isolated opposite outlet
to remain silent. Merely copying noise to both outputs would not solve this.

### D04 — P2: added-noise travel times omit center and link pipes

`core/src/noise.c:170–175`: delay adds header/front/rear/tail lengths, omitting
`center_length` and `link_length`. It also uses 580 m/s for every condition,
rather than the evolving path's sound speed and mean flow. At the same assumed
580 m/s, the first cylinder's current delay versus the complete straight-path
length is:

| Engine | Implemented delay (ms) | Full-length control (ms) |
| --- | ---: | ---: |
| I4 | 3.922 | 6.147 |
| I6 | 4.000 | 5.379 |
| Cross V8 | 3.568 | 6.069 |
| Flat V8 | 2.500 | 2.914 |
| Boxer | 3.604 | 5.793 |

Junction residence/scattering is excluded from this length control; it is not
an exact physical arrival prediction. The omissions are confirmed regardless.
This shifts the pulse/noise envelope relationship. The fixed 4096-step ring
also clips sufficiently long path delays, with a shorter time capacity at
higher physics rates. Derive timing from the actual topology and validate
impulse arrival across rates, temperatures and split/merged paths.

### D05 — P1: synthetic noise retains hidden catalyst and obsolete losses

`core/src/noise.c:162–169`: `PASS_CATALYST=.5` always multiplies the exhaust
noise, including current I6/flat-V8 catalyst bypasses. The fresh I6 fixture
reports the same **.04** total power pass at catalyst resistance **0 and 25 Pa s/m**.
The bypass still loses 3.01 dB in this layer. This qualifies the earlier
13b-r statement that a hidden catalyst attenuation had been removed: it was
removed from gas dynamics, not from every sound path.

This layer also ignores center silencers/packing and uses a type-only rear
pass/corner, independent of actual core length, bore and perforations. Gas
states still respond to geometry, but explicit noise transmission does not.
Baffle noise is inserted before the same shared output filter, despite its
claimed downstream origin. Couple each source's transmission to its actual
remaining path and calibrated frequency-dependent impedance [S4, S5].

### D06 — P1: turning variation off also switches combustion physics

`core/src/engine.c:890,1004–1008`: `variation==0` restores requested-AFR
timing and the old capped burn model; any positive value uses trapped mixture
and uncapped demand. This is deliberate legacy compatibility, but it is a
physical discontinuity in what otherwise appears to be a randomness control.

For an identical controlled stoichiometric charge with 60% burned gas (a
diagnostic outside the calibrated flame-fit domain), **255.84°** demand becomes
a **120°** scheduled burn at variation 0 and **255.840000012°** at variation
1e−9. Such a control cannot cleanly isolate stochastic roughness or validate
the production combustion model. Separate legacy-model selection from
variation amplitude, then recheck both dyno and sound comparisons. Do not
discard historical controls or silently reinterpret their meaning.

### D07 — P2: injector clicks are detached from injection events

`core/src/noise.c:349,369–371` clicks at intake opening based on global
ignition/fuel-cut flags. Actual fuel is added at intake closing in
`engine.c:975–986`, only when additional fuel is required. A cut/resume between
those events, or sufficient retained fuel, can produce a click without modeled
injection or miss a real addition. Decide the intended port/direct-injection
schedule and drive needle noise from that event and delivered fuel, rather
than a separate timing surrogate.

### D08 — P1: randomized stability checks omit the new junction chamber

`native/fuzz.c:56–83`: pressure/temperature checks and repair totals include
cylinders, airbox, plenum, collectors and pipe cells, but omit `e->junction`.
The new X/Y state can escape direct validation, and reported repair totals
can undercount it. The dedicated high-RPM probe does include junction repairs;
the broad fuzzer does not. Also, recorded repairs do not themselves fail
`rb_fuzz_run` or `test_fuzz_random_engines`. This is a coverage defect, not
evidence that the current junction actually failed. Add independently checked
junction bounds, repair acceptance and representative topology coverage.

### D09 — P2: propagation interpolation itself darkens the sound

`core/src/scene.c:115–118`, also fractional reads in `audio.c`: linear delay
interpolation is not spectrally neutral [S6]. For fractional part a its gain
is `|(1-a)+a exp(-i omega)|`. In the actual asphalt geometry at 48 kHz, direct
paths alone introduce **−1.40 to −6.00 dB at 16 kHz**, before intended head,
aperture and ground effects. This is a quantified numerical property, not a
claim that all intentional spatial filtering is wrong. A moving fractional
delay also changes that coloration over time. Establish an error budget and
research a more accurate delay interpolator; test phase and band amplitude
across fractional positions and speeds, rather than only the current 4 kHz check.

## Model limitations and validation still requiring attention

### R01 — P1: robotic deceleration and lift-off boom are not diagnosed fully

Retained S2b controls identify loaded-flow numerical damping as one contributor,
not the complete cause. Latest 13b-r raw coast-band contrast increases to
**6.37 dB I6, .80 dB cross V8, 3.28 dB flat V8**; I6 broadband RMS rises
**1.94 dB** at lift. A 600–1600 Hz step alone is not a perceptual failure
criterion. Current 0.15 s/12° retard, instantaneous pedal/area change, fuel
cut/resume and idle air/spark handoff are illustrative, not engine-specific ECU
calibrations (`engine.c:945–960,1195–1245`). Need matched-RPM/load source-isolated
controls, reference lift/cut timing and perceptual confirmation before fixing
or declaring this solved. Do not conceal the transition with an audio envelope.
The internal-jet layer also discards reverse flow (`noise.c:208`); whether
overlap/reversion noise matters on the coast is unvalidated. The generic road
load, reflected vehicle inertia and blockwise pedal schedule need matching to
the reference scenario; a neutral rev and a loaded coast are different tests.

### R02 — P1: V8 idle combustion and its flame-speed extrapolation need review

Latest retained warm-idle work COV: **7.83–11.70% cross V8**, **20.59–29.49%
flat V8**, with **135** incomplete modeled flat-V8 burns at EVO. These are
net-work statistics and Wiebe progress, not measured real misfire rates; a
small idle net-work denominator magnifies COV. Stable RPM is insufficient
validation. The flame-speed fit's original dilution measurements extend only
to .2 mass fraction [S7]; earlier flat-V8 idle samples reach .46–.51. The
model floors dilution/flame speed rather than resolving extinction. Revisit
cams, residual exchange, ignition, heat transfer, mixture and gross versus net
work against relevant data; raising noise or globally raising variation is
not a justified response.

### R03 — P1: ignition failure, late burning and exhaust chemistry are absent

`spark()` ignites every eligible burnable charge, and EVO abruptly terminates
its prescribed Wiebe burn (`engine.c:1034–1080,1366–1369`). There is no
spark-kernel failure/quenching, continued exhaust-port oxidation/afterfire,
or port fuel-film storage. Therefore limiter fuel, transient residual mixture
and overrun texture lack mechanisms that may matter. Actual significance must
be established from references and ignition/combustion research [S2, S3, S7].
Do not add arbitrary random misfires, prerecorded pops or an unexplained
residual threshold to satisfy a repetition score.

### R04 — P1: low-frequency repetition acceptance is still open

Last dedicated accepted method found **I4 +19.89 dB / boxer +17.71 dB** against
S1's 0…+15 dB target. Those are historical checkpoint measurements, not a fresh
27-finding audit result; the latest pipe configurations need a current complete
five-preset repetition/order audit. The final I6 equal effective paths also
change cylinder differentiation. A mixed-reference correlation threshold is a
diagnostic, not a universal engine law. Separate deterministic firing coherence
from artificial repeatability, controller hunting and microphone jet noise.

### R05 — P1: acoustic spatial/rate convergence is not established

Default downstream cells are about 7.5 cm (`engine.c:35–36,504–532`). This
staggered scheme's acoustic dispersion/cutoff can alter the 1–3 kHz harmonics
now central to the high-rev request. Time-step convergence of mean torque/VE
with the pipe grid fixed does not establish waveform, impedance, phase or
spectral convergence [S8]. S2b improves momentum reconstruction only; species
and enthalpy remain donor-cell transport, with appreciable damping under flow.
Need harmonic/packet transfer tests across frequency, mean Mach, direction,
temperature, grid and pipe rate, followed by matched native float/listening
checks. Fresh cell counts high/ultra are 111/217, 156/330, 226/446, 182/356, 127/257
for I4/I6/cross/flat/boxer: **none of these presets exhausts the 512-cell pool**.
Pool starvation is a risk for larger custom geometries, not a demonstrated
fault in these current five presets.
As the grid resolves more pressure detail, quantify overlap with the separately
generated noise. Its unchanged efficiency coefficient is neither proof of
double-counting nor proof that the two layers partition the spectrum correctly.

### R06 — P1: protective flow changes are not fully observable

`pipe.c:289–292,441–450,537–539` caps transported mass and sonic flow,
resets invalid face flow and can inflate an effective momentum length for
stability. Chamber repair counts do not count those interventions. CFL sizing
assumes a 1000 m/s wave-plus-flow bound; there is no full runtime margin trace
for the actual hottest/fastest cells. A run with zero chamber repairs does
not establish that its sound is free of guard-induced distortion. Instrument
these events and validate worst-case closed-throttle/high-RPM conditions;
retain guards until a validated alternative exists.

### R07 — P1: complete exhaust paths are still estimated research models

`exhaust_models.c` and 13b-p/r ledgers distinguish published tubing OD from
an assumed 1.2 mm wall, inferred lengths, can internals, header equivalents,
packing and junction volumes. The I6 equal-path compensation is not surveyed
manifold geometry. The flat V8 is a static valve-open, two-outlet equivalent,
not the complete switching/three-tip Kline system. X/Y chambers impose compact
mixed pressure and omit directional jets, bend losses/scattering and higher
modes [S9]. Characterize each route's transfer, backpressure and temperatures
before claiming real-system fidelity; request measurements/reference detail
where they would reduce uncertainty. An actual improvement is not proof that
every inferred dimension is right.

### R08 — P1: packing and chambered mufflers need a physical impedance model

`pipe.h` explicitly acknowledges that its swing-only series resistance adds
**inertance**, whereas a perforated side cavity adds **compliance**. Constants
1500 Pa s/m² and .25 ms are generic. Chambered cans are wide 1D volumes with
an equivalent exit restriction, not modeled internal partitions/perforations.
Correct attenuation in one band can coexist with incorrect phase, impedance
and backpressure. Research causal perforate/porous/shunt models and calibrate
transfer loss and phase over temperature/flow [S4, S5]. Flexible shells and
material-dependent radiation are absent; add them only when evidence justifies
their benefit. A higher cell count cannot repair the wrong topology/impedance.

### R09 — P2: thermal and gas-property assumptions affect resonance and overrun

`engine.c:71–75` fixes warm runner/header/front/rear/tail wall temperatures;
all pipes initialize at ambient, without warm-up/wall inertia. Fixed Darcy/
Colburn coefficients are not matched Reynolds/oscillatory viscothermal losses.
Gas composition uses three pseudo-species, with burned-gas apparent equilibrium
cp fitted at 40 bar above 1000 K (`gas.h:6–25`) throughout the engine/exhaust.
That is not a pressure-dependent chemistry or frozen/equilibrium acoustic
relaxation model [S10]. Quantify sensitivity of local c, damping, residuals
and outlet temperature before extending it; no specific current temperature or
pitch error is established here. Warm steady references and cold startup
should not be treated as thermally equivalent.

### R10 — P2: radiation load and audible output need a joint validation

The Silva unflanged load is passive and its interior energy books pass. Keep
that verified result. Its domain is a quiescent thin unflanged circular outlet
[S11], while playback uses compact monopole `dQ/dt`, and stereo adds a baffled
disk directivity approximation. These are not a validated single radiator at
large ka, hot mean flow, flared tips or near a vehicle body. Check integrated
far-field power/phase versus load resistance over their useful band. Signed
exterior work includes reactive storage and is not all dissipated far-field
sound. The donor-density reversal smoothing is also an exterior-plume
approximation, not modeled hot gas reingestion. No conservation failure is
inferred from these open acoustic checks.

### R11 — P2: mechanical sound remains generic rather than engine-specific

`noise.c:68–99,181–195,329–383` reuses six modes scaled only by bore,
scalar-summed axial gas force, and fixed dead-center slap impulses. Real
piston secondary motion depends on rod side thrust, clearance and oil-film/
contact dynamics; the present rule is not a solved impact model. Bank-force
directions and structural transfer differ between inline/V/boxer engines.
Engine-specific modal/transfer data, physically tied valve-seat excitations
and the planned cam/alternator/fan sources remain S5 work. This mainly affects
engine-bay/cabin identity; it is not proven to be the cause of rear-exhaust boom.

### R12 — P2: cabin, ground and vehicle geometry are provisional

Both listeners hardcode a front-engined 4.5 m car (`audio.c:15–42`,
`scene.c:10–11`), including the mid-engined Ferrari-inspired preset. Mounts
use gas torque only; the cabin uses common 100/200 Hz filters and a 55 Hz mode.
Ground/wall gains are scalar, head shadow is approximate and rooms use four
unmeasured delay lines. This can misattribute a perspective difference to an
engine. Need model-specific source/outlet coordinates, body/cabin/mount
transfer measurements, frequency-dependent reflection and room-decay checks;
S4's listening acceptance remains open.

### R13 — P2: pass-by is a limited demonstration

`scene.c:41,59–60,112–118`: constant 15 m/s motion is independent of simulated
road speed and the buffer silences sources beyond 32766 samples of flight time
(.683 s/about 234 m at 48 kHz). Long pass-by runs therefore lose paths entirely
around 22 s; the documented 12 s example is within its intended horizon. Motion
correctly uses retarded emission time, but convective radiation amplitude and
pass-by resampling bandwidth are not validated. Couple trajectory/velocity
when physical drive-by comparisons are needed, define duration limits and
check amplitude as well as tone frequency. Current long coasts use static
asphalt scenes, so this is not their observed boom cause.

### R14 — P1: high-rev settings exceed the validation of the reference engines

The I6 8500 / cross 7500 / flat 9000 rpm configurations are generic modified engines.
Kinematic valves keep following their cams; valve float, oiling, temperature,
knock and rotating-assembly limits are not modeled (planned stage 22). Original
stage 9 torque calibration and broad 15% regression tests cannot certify these
new engines or their high-RPM pulse shapes. Need revised prepared-engine
assumptions, full torque/VE/backpressure/combustion maps and reference curves.
Retained high-RPM conservation/three-rate means are useful and are not discarded.

### R15 — P1: reference conditions do not yet support precise calibration

BMW M4 turbo audio is not the naturally aspirated M54-inspired I6; Golf GTI
is not Toyota 1AZ-FE. Supplied videos may change cabin/exterior/flyby perspective,
gain, gearing and load. Raw excerpts are preserved; filtered copies remain
listening aids and cannot isolate engine-only sound with certainty. Obtain
model-relevant steady and lift-off examples with known RPM/gear/pedal and fixed
microphone context where possible. Bernardo may provide useful targeted sounds.
Do not infer position from brightness or treat unverified descending pitch as
a measured ECU lift/DFCO event (`docs/user-sound-references.md`).

### R16 — P2: analysis and previews can hide or confound differences

`high_rpm_report.py:23–38` integrates broad transient harmonic bands; changed
acceleration changes their widths and the captured broadband energy. Windows
with different RPM/load histories cannot isolate exhaust effects. Legacy
comparison mixes stereo to mono, allowing spatial cancellation, while newer
reports average ear powers. Repetition searches can select harmonic multiples.
The publisher peaks each whole clip at .6 (`render_results.py:91`), preserving
its internal dynamics but masking absolute differences between clips. These
limits are mostly documented already. Add order-tracked/matched-state and
per-channel analyses, and common-gain controls when level matters. CLI dB SPL
is a model-derived estimate, not microphone calibration; distinguish physical
pressure from compressed/limited output and reference recording gain.

### R17 — P1: full 13b and real-time audio delivery remain unresolved

Persistent pipe/face SoA and an accurate coupled implicit valve solve remain
unfinished. The rejected 96 kHz experiment must stay unshipped; reducing default
quality to hide runtime cost would require acoustic validation. Latest retained
cross-V8 engine-only timing is about **.90× native/.67× WASM real time**, below
the original 4×/2× goals and, on that host, below a real-time deadline before
full stereo overhead. A heavier, justified model is allowed, but it must still
meet its intended playback constraints. Benchmark full audio blocks, worst
cases and worklet headroom. Browser UI still plays the stage 2 tone until 14;
passing that browser test is not engine-playback acceptance.

### R18 — P1: test/roadmap acceptance must track physical integration

Existing tests often validate selected formulas/fixtures separately: the
impossible-flow noise choking fixture, old bank-isolation expectation and
omitted path lengths illustrate how correct assertions can preserve a wrong
integration. Add nozzle→noise, injection→click, spark→heat, junction→all-source
and transfer/power/phase checks tied to independent physics. Check mutations
of those connections. Keep S1/S1e/S2/S4/full 13b open, and distinguish completed
implementation substages from acoustic acceptance in older green rows.
Bernardo's latest “Decent” feedback approves continuing the review, not every
unresolved numerical or model assumption. No blanket failure of the working
engine or of the user's successful listening checkpoint is implied.

## Stage coverage and recommended dependency order

| Stage | Findings requiring attention |
| --- | --- |
| 3–9 supporting physics/dyno | D01,D02,D06,D07; R02,R03,R09,R14 |
| 10 pipes | R05,R06,R07,R09,R10 |
| 11 extraction/output | D09; R10,R16 |
| 12 exhaust/intake/listener | D04,D05; R07,R08,R10,R12 |
| 12b / S3 sources | D02–D05,D07; R03,R11 |
| 12c / S1a–e / S1 | D01,D06; R01–R04,R15,R16,R18 |
| S2 / S2b | D02–D05,D09; R01,R05–R10,R18 |
| S4 | D03,D09; R12,R13,R16 |
| 13 / 13b-a / 13b | D08; R05,R06,R17,R18 |
| 13b-p / 13b-r | D02–D05,D08; R01,R05,R07–R10,R14–R18 |
| S5 planned | R04,R11,R12,R15,R16 |

If subsequent work is authorized: establish independent integration checks
first; resolve ignition/variation and compressible source physics; repair
source topology/transmission; validate acoustic convergence and muffler
impedance; then investigate matched overrun, engine-specific transfers and
reference calibration, measuring runtime at each step. This is a dependency
recommendation, not authorization to begin those changes. Research every fix,
preserve realistic coherence and keep a single current results file with long
coasts at future sound checkpoints. **Hold after this audit.**

## Sources consulted for physical claims

- **S1** [NASA Glenn, compressible mass flow and choking](https://www.grc.nasa.gov/www/k-12/airplane/mflchk.html): throat versus reservoir quantities and sonic mass-flow limit.
- **S2** [Schiffmann, Reuss and Sick, ignition/flame-initiation variability](https://journals.sagepub.com/doi/10.1177/1468087417720558): empirical ignition variability; correlations alone do not establish causal mechanisms.
- **S3** [Aleiferis et al., early flame-kernel development](https://saemobilus.sae.org/papers/cyclic-variations-initial-flame-kernel-growth-a-honda-vtec-e-lean-burn-spark-ignition-engine-2000-01-1207): observations after actual spark, not pre-spark heat release.
- **S4** [COMSOL/KTH, muffler with perforates](https://doc.comsol.com/6.4/doc/com.comsol.help.models.aco.perforated_muffler/perforated_muffler.html): reactive/dissipative mechanisms, measured geometry/impedance and flow/temperature/shell limitations.
- **S5** [COMSOL, transfer impedance of a perforate](https://doc.comsol.com/6.4/doc/com.comsol.help.models.aco.transfer_impedance_perforate/transfer_impedance_perforate.html): frequency-dependent viscous/thermal transfer impedance and nonlinear-flow limits.
- **S6** [Julius O. Smith, interpolation frequency response](https://www.dsprelated.com/freebooks/pasp/Linear_Interpolation_Frequency_Response.html): interpolation coloration. The fixed fractional-delay values above are independently derived from the two-tap transfer function.
- **S7** [Metghalchi and Keck, original flame-speed measurements](https://james-keck-memorial-collection.unibs.it/JCKeck-papers/MetghalchiKeck-CombustionFlame-48-191-1982.pdf): measured phi/temperature/pressure/dilution domain, rather than an unbounded extinction model.
- **S8** [NASA NTRS, acoustic numerical propagation](https://ntrs.nasa.gov/citations/19960008581): acoustic dispersion/dissipation require accuracy checks separate from mean-flow convergence.
- **S9** [COMSOL, coupled 1D pipes and 3D junctions](https://doc.comsol.com/6.4/doc/com.comsol.help.models.aco.acoustics_pipe_system/acoustics_pipe_system.html): plane-wave and complex-geometry limits; not validation of our nonlinear solver.
- **S10** [Cantera, equilibrium and frozen sound speeds](https://www.cantera.org/3.1/examples/python/thermo/sound_speed.html): distinct thermochemical acoustic responses.
- **S11** [Silva et al., cylindrical-pipe radiation impedance](https://arxiv.org/abs/0811.3625): the basis and scope of the implemented unflanged load.

Hardware sources and dimension uncertainty remain in 13b-p/r ledgers. This
review does not substitute vendor descriptions for measured acoustic transfer.

## Reproduce the diagnostic

```sh
make BUILD=/tmp/rev-bench-sound-review /tmp/rev-bench-sound-review/rb_tests
gcc -O2 -std=c11 -Wall -Wextra -Wpedantic -Icore/include -Icore/src \
  tools/sound_physics_audit.c /tmp/rev-bench-sound-review/core/src/*.o \
  -lm -o /tmp/rev-bench-sound-review/probe
/tmp/rev-bench-sound-review/probe
/tmp/rev-bench-sound-review/rb_tests
```

Graphify was queried before source navigation and refreshed using AST after
adding the diagnostic. See the handoff for the refreshed counts and its known
public-header extraction limit. CLAUDE.md and .claude/ remain active and
unchanged. Current audio/result hashes are checked unchanged after the review.
