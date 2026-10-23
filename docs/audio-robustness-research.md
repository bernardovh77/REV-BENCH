# Audio robustness: research and candidate roadmap — 2026-10-22

This is research for the newly authorized bottleneck experiment and subsequent
roadmap. It does not change the accepted sound, select a replacement solver, or
claim that the experiment has already established its result. The initial
[diagnosis](audio-bottleneck-diagnosis-2026-10-22.md), the
[27-finding audit](sound-physics-audit-2026-10-22.md), and the
[selective rollback](stage-13b-r-rollback.md) remain the starting evidence.
Production at the diagnosis is `2330da55bc81ebed9a39b677b580335d26b976d7`.
S1/S1e/S2/S4/full 13b remain open; stage 14 is unstarted.

The current renderer is native C gas dynamics plus procedural noise and modal
sound. There is no trained neural renderer or training loss to replace. A
sophisticated solution should preserve causal physical controls and establish
the useful bandwidth of each approximation. Greater cost is acceptable when
demonstrated realism justifies it; neither higher order nor higher sample rate
alone establishes that gain.

## What the literature supports

The following are primary papers, author-hosted manuscripts or official model
documentation. None validates REV-BENCH simply because it uses related ideas.

| ID | Primary source and relevant result | Application and limitation here |
| --- | --- | --- |
| P01 | [Tam and Webb, 1993: dispersion-relation-preserving schemes](https://ntrs.nasa.gov/citations/19930067183). Designs numerical space/time operators for accurate acoustic wave propagation and compatible boundaries. | Measure dispersion, dissipation and boundary reflection separately from mean-flow convergence. This finite-difference paper is not a drop-in conservative engine solver. |
| P02 | [Torregrosa et al., 2017: engine gas-exchange flux limiters](https://doi.org/10.1016/j.ijmecsci.2017.09.029), [author manuscript](https://riunet.upv.es/server/api/core/bitstreams/29ac68a0-4010-4f2b-abf6-9205c38d7013/content). Compares staggered finite-volume FCT, TVD and momentum diffusion with shock, finite-pulse and complex-geometry cases; no method performs best everywhere. | Particularly relevant to the present staggered network. Select transport using weak acoustics, finite pulses and operating-engine evidence together, rather than one packet-energy number. |
| P03 | [Popescu, Shyy and Garbey, 2005: finite-volume DRP/compact formulations](https://uh.edu/nsm/_docs/cosc/technical-reports/2005/05_03.pdf). Evaluates linear/nonlinear waves and conservative face treatment. | A research direction for reducing acoustic loss while retaining conservation. Its scalar test problems do not validate mixture thermodynamics, valve coupling or engine junctions. |
| P04 | [Hota and Munjal, 2004: hybrid thermo-acoustic engine exhaust model](https://www.iiav.org/ijav/content/volumes/9_2004_1722161272264886/vol_3/396_fullpaper_1757111286453860.pdf). Couples a nonlinear source to a linear downstream description, includes reflected load and radiation, and compares with single-cylinder diesel measurements. | Supports testing a hybrid, with source/load feedback. Explicitly neglects downstream nonlinear propagation. Its validation cannot be transferred to multicylinder, hot transient SI exhausts without new checks. |
| P05 | [Rocchesso and Smith, 2003: generalized digital waveguide networks](https://www.di.univr.it/documenti/ArticoloRivista/allegato/allegato979612.pdf). Derives scattering, complex power, loss filters and passivity conditions for linear waveguide networks. | Useful for efficient pressure/volume-flow network realization with explicit energy norms. It does not justify treating mean-flow engine components as lossless passive musical tubes. |
| P06 | [COMSOL: coupled 1D pipe and 3D junction/bend](https://doc.comsol.com/6.4/doc/com.comsol.help.models.aco.acoustics_pipe_system/acoustics_pipe_system.html). Demonstrates local complex-geometry modeling with plane-wave ports. | Supports local component characterization instead of full-engine 3D CFD. This tutorial's coupling assumes zero background flow; it is not a mean-flow validation. |
| P07 | [COMSOL/KTH: measured perforated muffler](https://doc.comsol.com/6.4/doc/com.comsol.help.models.aco.perforated_muffler/perforated_muffler.html). Uses actual hole/plate geometry, porosity and impedance to compare an acoustic model with supplied measurements. | Distinguish shunt compliance, perforate inertance/resistance and porous loss. The demonstration is room-temperature, no-flow and 20–600 Hz; it is not a universal hot muffler fit. |
| P08 | [Jones, Kreitzman and Nark, NASA 2024: perforate impedance models](https://ntrs.nasa.gov/citations/20240006417). Evaluates models against normal-incidence and grazing-flow measurements across different SPL and Mach conditions. | Use measured-domain error and sensitivity, not one generic attenuation constant. Facesheet data are useful method evidence, not measurements of the project's proprietary silencers. |
| P09 | [Silva et al., 2009: cylindrical-pipe radiation impedance](https://arxiv.org/abs/0811.3625). Supplies physical, causal approximations and explicitly distinguishes a more accurate noncausal frequency-domain fit. | Retain the existing validated causal load as a control. Jointly check terminal impedance and radiated output; do not put a noncausal fit into streaming playback. |
| P10 | [Gorazd, Jurkiewicz and Snakowska, 2012: unflanged radiation with low mean flow](https://acoustics.ippt.gov.pl/index.php/aa/article/view/117). Compares measured directivity with flow-aware theory, including multimodal behavior. | Warns against assuming quiescent radiation/directivity remains exact with flow. Their fan-driven duct differs from a hot automotive outlet and body. |
| P11 | [Schiffmann, Reuss and Sick, 2017/2018: flame-initiation variability](https://journals.sagepub.com/doi/10.1177/1468087417720558). Combines optical/pressure/flow measurements over repeated cycles. Its multivariate correlations do not establish causal mechanisms. | Separate spark, kernel development and bulk burn, and validate state-conditioned distributions. Do not infer a unique jitter law from sound alone. |
| P12 | [Daw et al., 1996: physically oriented cyclic-variation model](https://saemobilus.sae.org/papers/a-simple-model-cyclic-variations-a-spark-ignition-engine-962086). Couples stochastic fluctuations with prior-cycle residual effects and compares with engine data. | A candidate state-based model family, not a mandate to add arbitrary random misfires or deterministic chaos to every operating condition. |
| P13 | [Metghalchi and Keck, 1982: burning-velocity measurements](https://james-keck-memorial-collection.unibs.it/JCKeck-papers/MetghalchiKeck-CombustionFlame-48-191-1982.pdf). Reports fuel/pressure/temperature ranges and simulated-product dilution up to 0.2 mass fraction. | The current high-residual idle extrapolation needs a validity boundary and relevant data. This fit is not an ignition-extinction or exhaust-oxidation model. |
| P14 | [DDSP, 2020](https://arxiv.org/html/2001.04643v1). Learns interpretable harmonic/noise DSP controls with multiscale L1 spectral reconstruction; low-rate controls drive audio-rate synthesis. | Structured conditioning is worth an optional later comparison. DDSP does not supply validated combustion, geometry or conservation by itself. |
| P15 | [HiFi-GAN, 2020](https://arxiv.org/html/2010.05646v2) and [Parallel WaveGAN, 2020](https://arxiv.org/html/1910.11480v2). Combine waveform adversarial training with spectral/feature losses; multiple STFT windows address time/frequency resolution tradeoffs. | These objectives can improve perceptual texture in a trained system. They do not require correct engine events or transfer physics, and their speech evidence does not diagnose this C renderer. |
| P16 | [DiffWave, 2021](https://arxiv.org/html/2009.09761v3). Uses L2 noise prediction and iterative stochastic sampling, including a six-step fast variant; neural vocoding uses informative mel conditioning and a speech dataset. | Diffusion is not a guaranteed cure for dullness. Its published GPU speed is not a browser callback budget or evidence of engine control from RPM alone. |

## Two transport candidates, after the diagnostic experiment

**Candidate A: improve the conservative nonlinear 1D path.** Keep mass,
species, momentum, total energy, changing gas properties and valve/source/load
interaction in one physical description. Evaluate consistent higher-order
reconstruction and time integration of the transported state, with
positivity-preserving fallback and independently checked boundary fluxes.
Compare acoustic amplitude/phase and finite-amplitude behavior across limiter
choices. A momentum-only improvement does not imply that mass/enthalpy and
boundary errors disappeared. A nominal high-order stencil can still lose
coherence at extrema, discontinuities, grid joins and the two-rate interface.
P01–P03 support these questions, not a predetermined winning algorithm.

Do not select a low-Mach preconditioner from its name alone. An alteration
designed to accelerate a steady flow solve may alter unsteady acoustic speeds.
The candidate must recover the independently specified sound-wave problem in
both propagation directions and still handle compressible valve/blowdown
events without hiding errors in a clamp.

**Candidate B: a coupled nonlinear source and linear acoustic network.** Keep
cylinders, valves, near-port blowdown and any materially nonlinear pipe region
in gas dynamics. Characterize the downstream small-signal region as a causal
multiport network with delays, dispersive losses, junction scattering,
perforates/side cavities and radiation. Feed the reflected pressure/impedance
back to the nonlinear boundary; a fixed source followed by an attractive
filter omits the source/load interaction that P04 explicitly addresses.

Operate the network about documented temperature, composition and mean-flow
states, and verify how quickly those states may change. Use characteristic
speeds `U+c` and `U-c` in the uniform-flow reference problem; path time in a
nonuniform route depends on its actual local state and direction. Validate
parameter updates, stored acoustic energy and transient state conversion;
arbitrary interpolation between individually stable filters need not give a
stable or physically meaningful time-varying system. Do not duplicate the
same coherent pressure pulse in both representations.

The interface and linearization domain are experimental outputs. Measure
pressure amplitude relative to mean pressure, wave steepening, local Mach,
flow reversal, temperature change and the difference from a converged
nonlinear control. Do not assume a universal RPM boundary or use only mean
Mach to declare linearity. If the hybrid loses demonstrated finite-amplitude
character, move its boundary downstream, improve its model, or reject it.
P04's diesel validation and P05's linear waveguides are narrower than the
project's target; neither supports replacing the entire engine with delay lines.

Both candidates may be useful: a conservative nonlinear core for source and
gas exchange, with validated local multiport components where 1D geometry is
insufficient. Offline component calculations or measurements can produce a
reduced streaming model; full-engine 3D CFD is not a prerequisite. Geometry,
linearization and available measurements decide where that reduction is valid.

## Proposed gates and reproducible experiment fixtures

All numerical thresholds below are **proposed engineering targets**, not
achieved results. The promotion targets in
[the integrated plan](sound-robustness-plan.md) govern the roadmap; any tighter
numbers below are exploratory stretch targets for comparing candidates, not
additional or contradictory release gates. None is a universal acoustic standard.

The following research discussion likewise does not report
achieved results or universal audibility limits. Freeze them, the stated
bandwidth and the test envelope before evaluating a candidate. If evidence
requires changing a target, preserve the original result and explain why.

1. **One reproducible record per experiment.** Save production/source hashes,
   build/compiler flags, descriptor/topology, initial gas/wall state, seeds,
   sample/pipe/engine rates, actual cell lengths, control timeline, scene,
   gain and exact measurement intervals. Include layer taps before radiation,
   propagation and limiter. Use unfiltered float WAVs and raw pressure/flow
   traces in ignored `renders/`; no measurement is taken from RESULTS.html's
   independently normalized playback copies. Record every cap, effective-length
   adjustment, fallback and repair, including junction state.
2. **Straight-pipe acoustics.** Start with an ideal-gas fixture of known
   analytical sound speed, uniform state, known length and anechoic ends;
   separate mixture-property validation. Sweep 80–4000 Hz initially, with
   300/600/900 K and `U = 0, ±35, ±70 m/s`, plus small amplitudes spanning
   `p'/p = 1e-5` to `1e-3`. Screen stronger pulses separately. Use at least
   three spatial refinements and independently refine pipe and engine time
   steps. A candidate target is magnitude error within 0.5 dB and phase error
   within 5 degrees over a declared 1-m certified path/band, with the two finest
   runs differing by at most 0.25 dB/2.5 degrees. Report the actual certified
   band; do not extend the claim to unresolved frequencies or arbitrary lengths.
3. **Energy and boundaries.** In a finite lossless moving-packet fixture,
   propose 95–105% perturbation-energy retention before boundary interaction,
   with the energy definition and mean-state subtraction published. This is
   separate from global gas/kinetic energy conservation. Check analytical
   reflection amplitude/sign, through-delay, area steps, closed/open ends,
   anechoic leakage and two-rate/grid interfaces. Check pressure and signed
   flow; pressure symmetry alone can miss an incorrect connection. Do not
   score phase at a near-zero transmission notch without an uncertainty floor.
4. **Conservation, positivity and causality.** Preserve independently checked
   mass/species/energy books with dimensioned absolute and scale-relative
   tolerances fixed from the baseline. Include wall work/heat, boundary work,
   thermal storage and physical losses. Require finite positive admissible
   states, no unreported interventions, and zero repairs/caps in the stated
   canonical validation envelope; report the supported envelope for cases
   beyond it. A physical source impulse must not appear before its cause or
   before the shortest physically admissible path. Retain safety guards while
   investigating their activation.
5. **Passive reference networks, with a strict mean-flow distinction.** At
   zero mean flow, power-normalize ports by their characteristic admittances.
   Require a lossless reference junction to conserve incident wave energy;
   a passive lossy reference may only store or dissipate supplied energy.
   For a fitted passive one-port, check stable poles, causal impulse response
   and nonnegative real impedance over the declared band; for a multiport,
   check contraction in the correct power norm. A pressure coefficient larger
   than one is not by itself energy creation. **With mean flow**, derive the
   appropriate convective energy flux and account for energy exchange with the
   mean flow, entropy perturbations and any active noise source. Do not impose
   a blanket no-gain rule on a flow-driven component or measured engine.
6. **Topology and transfer.** Use unequal two-port controls, a compact Y, an
   equal-port X, unequal temperatures/areas, and the actual I6/cross/flat paths.
   At zero mean flow an ideal equal-admittance compact four-way junction has
   reflected pressure `-1/2` and each of three transmitted pressures `+1/2`
   for one incident unit wave; each carries one quarter of the incident power.
   Derive this independently from pressure continuity and volume-flow balance.
   Use it only as a limiting fixture, not the full finite hot X. Test complex
   transfer from every source location to every outlet, including center/link
   lengths, phase, cross-bank coherence and downstream source insertion.
7. **Muffler/radiation components.** Validate empty expansion chambers first,
   then one measured or analytically tractable side cavity/perforate before a
   complete estimated silencer. Compare complex impedance/transmission and
   backpressure, not only scalar transmission loss. For no-flow radiation,
   compare reflected/transmitted energy and integrated far-field power with
   the load's resistive power, separating reactive storage. State ka,
   plane-wave/cut-on and near-/far-field domains. Use flow-aware radiation and
   measured component data when those domains are exceeded. A useful proposed
   component target is 1 dB magnitude/10-degree phase agreement with an
   independent reference in its valid band; retain measurement uncertainty.
8. **Engine integration and listening.** Use fixed-RPM/load holds, the same
   repeatable free-crank drive, neutral rev-down and loaded coast separately,
   and multiple fixed seeds. Trace cylinder/header/collector/outlet signals
   through pedal lift, actual spark, fuel cut/resume and clutch-in. Compare
   per-ear or mean-ear-power spectra, order tracks, phase/coherence where
   meaningful, band envelopes, crest, modulation and repeated-cycle statistics.
   Do not infer lift from one 600–1600 Hz step or turn mixed-reference
   repetition into a universal engine law. Mean torque/VE and transient
   trajectory changes must be reported alongside acoustic changes.

Useful deliberate faults for isolated copies include: restore donor transport;
reverse an advection sign; omit one conservative face flux; mis-scale energy;
drop a center/link delay; swap `c+U`/`c-U`; disconnect reflected load feedback;
copy a source to both outlets without scattering; remove one X transmission;
replace a side-cavity compliance with series inertance; introduce a negative
resistance or unstable fitted pole; use a noncausal response; shift burning
before actual spark; switch deterministic physics at variation zero; omit
junction repair counters; and use phase-blind or independently normalized
measurement outputs. Each test must catch the physically relevant fault,
including integrated source-to-outlet connections, before its acceptance is
trusted. These are proposed fixture/mutant designs, not newly executed tests.

## Reference use and data still needed

Existing metadata is in `tools/user_engine_reference_sources.json`,
`tools/user_engine_references.csv` and
[the reference notes](user-sound-references.md). Historical exact falling-pitch
selections and hashes are retained in ignored
`renders/evidence/history/checkpoint-13b/comparison.json`. All selected raw
12-second measurement WAVs exist in `renders/real/`.

| Family | Documented falling-pitch interval, source seconds | Exact legacy float input and local seconds | Rising-pitch screening candidate, source seconds |
| --- | --- | --- | --- |
| I6 | BMW M4 250–253 | `user-bmw-m4-brighter-242.wav`, 8–11 | BMW M4 242–249; local 0–7 of the same file; several abrupt ridge drops remain |
| Cross V8 | Mustang GT 257–263 | `user-mustang-gt-darker-257.wav`, 0–6 | Mustang 200–207; local 3–10 of `user-mustang-gt-brighter-197.wav` |
| Flat V8 | Ferrari 458 302–310 | `user-ferrari458-brighter-302.wav`, 0–8 | Ferrari 49–55; local 5–11 of `user-ferrari458-darker-44.wav` |

The falling selections are prior documented observations. The rising choices
above were screened here by visually inspecting STFT harmonic ridges in the
existing **unfiltered mono float** excerpts; no listening-based event annotation
or physical operating-state verification is claimed. They are useful candidate
changes of pitch, not verified loaded pulls. No RPM is inferred from ridge
frequency: harmonic-order ambiguity, shifts and motion remain possible.
The temporary screening plot is in `/tmp`, not a new reference deliverable.

The older measurement excerpts are arithmetic L/R mono averages. They preserve
their existing documented method but can cancel spatial components. For new
stereo comparisons decode the same original source interval to unfiltered float
stereo, retain the original source hash and processing record, and measure
channels separately or average channel **powers**. Preserve the older excerpts
for continuity. Do not calibrate with the filtered listening copies.

There are no measured RPM, gear, torque, pedal, spark/fuel-cut or temperature
traces for these source windows. Brighter/darker describes acoustics, not
verified cabin/exterior/exhaust microphone location. Viewpoint, motion and
processing can change inside a window. MP3-derived float WAVs do not restore
lost coding detail or provide calibrated sound pressure. The turbocharged M4
is not the current naturally aspirated M54-inspired I6, and the modified
8500-rpm I6 exceeds its original validation. Mustang/Ferrari also have different
exact hardware/calibration from the research exhaust equivalents. Use these
recordings for texture and change-of-state observations; do not fit a universal
EQ, absolute level, ECU delay or source-power coefficient to them.

The present 16 windows have already informed listening and tuning, so they are
not a pristine held-out test set. Before the next calibration, designate a
development set, a locked validation set and prospective new recordings with
known conditions. Split by recording session/engine/hardware, not adjacent
frames from the same passage. With this small library, report results per
source and acoustic group rather than a deceptively precise aggregate score.
Do not time-stretch/pitch-shift the real waveform to manufacture matched RPM.
Use verified telemetry for order tracking; without it report a pitch/ridge
comparison with uncertainty and exclude ambiguous shifts/transitions from
causal claims.

For R2, the highest-value addition is a naturally aspirated inline-six with
known intake/exhaust configuration, fixed rear/outlet microphone position,
logged RPM, gear/pedal and repeated warm steady/neutral/loaded lift sequences.
Equivalent cross/flat-V8 material would reduce their calibration uncertainty.
Prefer uncompressed fixed-gain stereo or synchronized close-source plus
listener recordings. Document microphone distance/angle, wind, room/road,
temperature, modifications, any AGC/limiting and whether fuel-cut is measured
or merely suspected. Obtain component transfer/backpressure/temperature data
when hardware internals remain unknown; a source video cannot uniquely
identify muffler geometry.

Use two complementary listening comparisons: common gain preserves meaningful
level/dynamics differences; separately level-matched and randomized pairs help
judge timbre without rewarding loudness alone. Preserve every applied gain,
avoid peak-normalizing each short phase, include hidden repeated controls and
publish uncertainty/disagreement. Bernardo's listening checkpoint remains a
required acceptance. A promising preference alone does not close the separate
conservation, causality or reference-matching checks.

## Candidate small-stage sequence and audit coverage

These labels are a proposed integration into the existing roadmap, not tracker
updates or a declaration that every item is currently authorized to ship.

| Stage | Concrete reviewable result | Gate before advancing | Audit items covered |
| --- | --- | --- | --- |
| **S2c: diagnostic experiment** | Frozen layer/transport/scene controls; actual grids/rates and all guard counters; matched-state source-to-output evidence. | Distinguish established numerical loss from unproven perceptual cause; save reproduction and measurement uncertainty; no production change justified by one scalar score. | D08; R01,R05,R06,R16,R18 |
| **R2: matched references and validation protocol** | Annotated development/locked reference ledger and a targeted data request; current windows retained with limits. | No NA-I6 calibration from turbo M4; no calibrated SPL/RPM/load claim without data; hardware/rev-limit assumptions explicit. | R07,R09,R14,R15,R16 |
| **S1f: combustion and event causality** | Distinct commanded/actual spark, nonnegative kernel development, deterministic model choice independent of randomness, actual injection events. | Zero heat before actual spark; continuous variation-to-zero control; fuel/energy books and matched dyno; conditional burn/COV/residual evidence, repeated seeds and listening. A realistic failure law requires supporting data. | D01,D06,D07; R01,R02,R03,R04,R18 |
| **S2d: acoustic transport decision** | Compare conservative nonlinear candidate A with the current control; prototype candidate B only if the measured error/cost warrants it. | Certified magnitude/phase/energy band, independent space/time refinement, shock/finite-pulse robustness, source/load feedback and explicit linearization domain; no hidden quality reduction. | R05,R06,R09,R17,R18 |
| **S2e: unified source-to-outlet transfer** | All source layers use actual topology, location, temperature/mean-flow timing and causal frequency-dependent component transfer. | Every-source/every-outlet complex transfer; compact-junction limits, physical/no-flow energy tests, measured/limited muffler model, joint radiation/output check. Recalibrate source strength only after transfer is stable. | D03,D04,D05; R07,R08,R09,R10,R18 |
| **S3b: flow and mechanical sources** | Correct nozzle/throat state and flow direction; event-linked mechanical excitation and engine-specific modal/transfer evidence where available. | Nozzle-to-noise integration with physically attainable choking; zero-flow and reversing controls; no unvalidated global treble/noise increase or double-counted resolved content. | D02,D07; R03,R05,R11,R18 |
| **S4b: listener and propagation** | Fractional-delay error budget, source/body coordinates, temperature/distance consistency, defined pass-by range and scene transfer. | Proposed delay-interpolator target of 0.25 dB amplitude error through 16 kHz in the supported stationary/moving range; verify phase/Doppler, reflection/decay, output power and matched perspective before sound acceptance. | D09; R10,R12,R13,R16 |
| **13b-c: full delivery/runtime** | Interleaved native/WASM full-stereo block benchmarks, bounded memory/hot-loop behavior, instrumented high-load/closed-throttle/custom-topology runs. | Real measured callback headroom on intended hardware plus unchanged accepted physical/acoustic gates; stage-2 tone playback does not count as engine-worklet acceptance. | D08; R06,R17,R18 |
| **Optional S6: learned residual** | Offline, controlled residual/DSP-parameter comparison using an adequate matched dataset; pure physical control retained. | Held-out control response and listening benefit, bounded/causal contribution, no state/phase/hardware leakage, native/WASM cost measurement before any shipping decision. | R03,R04,R11,R15,R18; does not close their physical/data gaps by itself |

The table covers all D01–D09 and R01–R18 findings. A stage can document a
validated model limit or a still-missing measurement; coverage does not imply
that all 27 findings need to become one implementation task or that a research
limitation has been repaired by naming it.

## Runtime budgets consistent with realism first

Measure full stereo rendering, not engine stepping alone: sources, transport,
radiation, delays, room/cabin, filters, output conversion and callback overhead.
For 128 frames at 48 kHz the audio quantum is about **2.667 ms**. Proposed
browser acceptance is p99.9 rendering below 50% of that quantum on a named
reference machine, with measured worst-case margin below the deadline during
long interactive stress runs. Record browser, CPU/power mode, thermal state,
concurrent UI work and duration; report maximum and deadline misses, not only
average throughput. Keep the original 4x-native/2x-WASM aims visible as separate
roadmap goals until explicitly revised.

If the demonstrably better model exceeds the present budget, report the
quality/cost frontier and optimize measured hot paths while preserving its
acceptance fixtures. An offline reference-quality renderer can be useful
during research. Do not silently claim real-time support, lower physical
quality, remove acoustic layers, or replace the reference with a cheaper
model to pass a timing number. A separate validated runtime mode requires
documented measured/listening error and Bernardo's checkpoint.

## Conditions for an exploratory neural residual

First validate physical source timing and transfer. A later model could predict
missing flow/structural texture or interpretable filter/modal parameters,
conditioned on crank phase, RPM and acceleration, load, actual spark/injection/
cut state, residual/mixture/temperature, hardware and source/listener geometry.
Keep physical coherent components and their phase intact; inject a local
residual through the validated remaining path rather than an unrelated output
effect. Compare a simple conditional statistical/DSP model before a larger
waveform generator. DDSP-style structure is an option, not a new requirement.

Waveform residual targets require synchronized source/time/phase and compatible
microphone transfer. Unaligned subtractive targets from user videos are not
ground truth. For stochastic texture, compare conditional distributions and
modulation statistics rather than demand exact sample equality. Hold out
sessions, operating trajectories and hardware; verify extrapolation/fallback,
zero/fuel-cut behavior, seed reproducibility, causal streaming, bounded output
and contribution to power. A realistic-sounding hallucination of combustion
during a measured cut fails the control test.

One-shot waveform L2 regression can average phase/timing-ambiguous targets;
that is different from saying every L1/L2 loss blurs sound. DDSP uses L1
spectral reconstruction, HiFi-GAN includes L1 mel/feature terms, and DiffWave
uses L2 noise prediction. Multi-resolution STFT loss compares multiple
time/frequency scales, while adversarial/feature objectives can encourage
plausible texture; neither supplies missing engine labels or physical event
constraints. Diffusion requires iterative inference and a validated streaming
schedule; its speech MOS/data/GPU results do not establish the engine data
volume, fidelity or deployment cost. Current references are too weakly
conditioned for a defensible paired neural engine-residual claim.

## Research scope and verification

Read PLAN.md/CLAUDE.md/CODEX.md, the latest rollback/diagnosis/audit/reference
notes and source-interval metadata. Graphify was queried before code navigation;
the query used the existing 1890-node graph. This work did not inspect or alter
production source, regenerate audio, modify original/filtered references,
change Claude's setup, or run a new production validation suite. Apart from a
temporary unfiltered-reference spectrogram under `/tmp`, the only deliverable
is this research note. Candidate target values and mutants are proposals;
actual experiment findings belong in the separately reproduced stage report.
