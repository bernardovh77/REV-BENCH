# Sound robustness and realism programme

**Scheduling authority (2026-10-23):** use the renumbered [PLAN.md](../PLAN.md)
and [legacy crosswalk](roadmap/legacy-map.md). This document retains the technical
programme and old IDs as design provenance; its table is not a second current
status tracker. Current stage specifications and evidence gates are in
[the roadmap catalogs](roadmap/README.md).

Started 2026-10-22; updated 2026-10-23. Bernardo authorizes the source/transport experiment and asks for a
method and plan for more robust, sophisticated sound generation. The current
accepted I6/flat/cross exhaust selection remains the baseline. This document
defines subsequent work; completing the experiment does not automatically
implement every stage below or close S1/S2/S4/13b.

The 2026-10-23 [S2d implementation specification](stage-S2d-design.md) turns
the next architecture comparison into explicit topology/port contracts,
conservative and wave-network algorithms, reflected-load coupling, source
ownership, mixed-rate/runtime tests and staged deliverables. It also specifies
standalone engine-specific learned-DSP controls and conditional source-only
integration. This is a completed design checkpoint, not completed S2d/S2e/S6
implementation or listening acceptance.

The first [native S2d checkpoint](stage-S2d.md) is now implemented: 344 candidate
rows, a conservative A1 control that fails upper-band acoustic accuracy, and
a B0 graph that passes the stated quiescent plane-mode fixture band. These
are isolated numerical candidates. Mean-flow/engine feedback, component
calibration, full runtime and listening gates remain open.

## Engineering direction

Use the gas simulation as the causal source of firing timing, pressure/flow,
temperature, combustion state and engine response. Give every audible source
an explicit physical origin and a documented path to the listener. The same
exhaust geometry must govern breathing, pulse propagation and broadband-noise
transmission. Keep nonlinear gas dynamics where they matter; characterize
what can safely use a more efficient linear acoustic model. Add statistical
detail only for unresolved processes, with operating-state-dependent energy,
spectrum and temporal modulation.

A different numerical method, a more expensive quality level or a learned
model is a candidate until the same fixtures, recordings and listening tests
show an improvement. Realism takes precedence over performance; document both.
An increase in spectral brightness alone is not sufficient evidence.

## One method for every change

1. **State the physical question.** Identify the symptom, proposed mechanism,
   source/transfer/output layer and competing explanations. Link a primary
   paper or measured component/engine evidence, including its validity range.
2. **Freeze a reproducible baseline.** Record source revision, geometry,
   actual grid/rates, operating trajectory, temperatures, ECU states, seeds,
   listener, output gain, compiler and source/output hashes. Preserve Claude's
   setup and current accepted recordings. Keep raw float measurement files.
3. **Test a minimal fixture first.** Use analytic limits or measured transfer
   data, then progressively add mean flow, heat, junctions and nonlinear pulses.
   Vary space, pipe time and engine time independently. Test multiple amplitudes
   and seeds; include unobserved trajectories/configurations before tuning.
4. **Prove the measurement can detect failure.** Include an independent
   reference and targeted deliberate faults. Check phase/delay as well as
   magnitude, energy bookkeeping and finite state. A green integration test
   does not establish physical accuracy. Record rejected cases, not just wins.
5. **Validate the whole engine and listening result.** Compare all five
   presets at idle, fixed loaded speed, high RPM, neutral rev-down and loaded
   lift/coast, plus state changes. Separate intake, tailpipe and structure.
   Use common playback gain for causal comparisons; keep a distinct,
   explicitly labeled loudness-matched audition if useful.
6. **Measure cost and publish a checkpoint.** Profile production rendering
   separately from diagnostic taps/file writing. Check native/WASM state and
   sonic parity, browser callback tails, memory and underruns. Publish one
   RESULTS.html with current examples and long coasts. Retain useful numerical
   evidence, clean superseded controls, and await Bernardo's listening verdict.

## Small stages and decision gates

The numerical values below are **proposed engineering targets**, not achieved
validation, universal acoustic standards or substitutes for listening. Confirm
each fixture's operating range before using the target as a release gate.

| Order / stage | Work | Evidence required before promotion |
| --- | --- | --- |
| **S2c — Source and transport experiment** | Locate layer contributions; independently vary grid/pipe/engine time; characterize ideal-pipe attenuation, phase and failure cases. | Reproducible data, analytic/linearity/partition checks, production-chain identity, source-sum checks, documented reference limitations and listening checkpoint. This authorized investigation is the first step. |
| **R2 — References and identifiability** | Curate comparable recordings for all five engines. Prioritize NA I6 and boxer gaps, stable microphone perspective, RPM/load information, loaded lift and neutral fall separately. Ask Bernardo for targeted material when needed. | Provenance, source intervals, raw stereo, processing and uncertainty ledger. Split by recording/session before fitting; do not tune on every available excerpt. Unknown viewpoint/RPM examples remain perceptual targets. |
| **S1f — Causal excitation and diagnostics** | Prevent pre-spark heat release; separate kernel/bulk burn and residual-state limits; make variation controls continuous without switching the underlying combustion model; align injector events; instrument all state repairs/caps and junction state. | Zero pre-spark heat to numerical tolerance; zero injection events when injection is disabled; controlled noise-off comparison preserves the same mean model; event-level energy/fuel books; multiple-seed COV/repetition, idle and transient checks. Do not invent random pops/misfires. |
| **S2d — Acoustic transport decision** | Compare a conservative acoustic-preserving 1D method with the current solver. Separately prototype nonlinear-source/linear-network coupling, including reflected load feedback. | Select using weak waves, finite pulses, shock/rarefaction, steady breathing and engine sound together. Initial ideal-pipe target: ≤1 dB magnitude error and ≤10° phase error over a stated 1-m path, 200 Hz–4 kHz, 300–900 K and tested ±70 m/s flows; no unexplained growing modes in long-duration controls. Extend bandwidth only after demonstrating it. Publish actual achievable band and cost. |
| **S2e — One acoustic transfer model per exhaust** | Route broadband and pulse sound through consistent geometry, X/Y scattering, temperatures/mean flow, catalysts, perforates/packing, radiation load and far-field output. Correct choked-source throat conditions and reverse-flow treatment with calibration. | Analytic straight/expansion/Helmholtz limits; measured component magnitude **and phase** where available; pressure/volume-flow continuity and correct source loading; energy ledger. Passive no-flow components must not generate energy. With mean flow, explicitly account for convective energy and mean-flow work rather than imposing a blanket no-gain rule. No double counting between resolved pulses and unresolved noise. |
| **S3b — Engine-specific turbulence and structure** | Replace generic broadband/filter assumptions with state-conditioned source models; improve pressure-force coupling, block/head/mount responses, valvetrain/injection timing and accessories. | Same-state layer captures, modulation/order statistics and measured or bounded transfer responses. Report uncertainty in mode/packing/source constants. Verify identical physical changes affect each relevant outlet and listener consistently. Repeated seeds must preserve expected energy and distributions. |
| **S4b — Listener and recording validation** | Validate fractional-delay bandwidth, source directivity, vehicle geometry, cabin/body transfer, reflections and moving-source history. | Analytic delay/Doppler cases, finite-history guards, known-position stereo comparisons and frequency-dependent response checks. Initial digital-path target: ≤0.5 dB unintended magnitude error through 12 kHz; real measured attenuation remains part of the physical model. Distinguish arbitrary recording processing from simulated acoustics. |
| **13b-c — Fidelity tiers and runtime** | Profile the validated implementation, choose grids/bands per component, move work out of the callback and expose documented fidelity settings. Complete relevant remaining 13b work without silently weakening sound. | Benchmark without diagnostic output; native/WASM numerical/audio consistency; at 48 kHz the 128-frame callback has 2.67 ms. Proposed targets: p99 ≤70% of that period and zero observed deadline misses/underruns during a stated 15-minute stress trajectory on each supported hardware/tier, with measured worst case. If realism requires more hardware, state that requirement and its audible benefit. Old 4×/2× targets are not silently declared met. |
| **S6 — Optional learned unresolved-sound model** | Only if a demonstrated perceptual gap remains and appropriate data exist, compare a small conditioned residual/parameter model with structured DSP and then GAN/diffusion candidates. | Condition on crank phase/RPM, load, flow/temperature, fueling/spark/cut, hardware and listener as needed. Validate unseen sessions, RPM sweeps and hardware. Preserve control causality, bounded energy and streaming continuity. Show blind listening benefit and measured latency/memory beyond the physical baseline. A plausible but incorrectly controlled engine sound fails. |

R2 begins alongside numerical work; it must precede physical calibration and
any supervised learned residual. S1f provides trustworthy excitation for
S2d/S2e. Runtime profiling starts early, but optimization cannot choose the
physical model before its error is measured. Neural work is optional and last.

The actual S2c coast taps show near-mouth gas temperature approaching ambient
soon after lift. S2d/S2e must distinguish physical reverse-flow entrainment and
transient thermal behavior from coarse-grid mixing and boundary artifacts.
Validate end-cell and upstream temperatures separately; do not interpret the
local gas value as the wall or whole-system cooling rate.

## Architecture decision after S2d

The explicit [generation-architecture review](sound-generation-architecture-review.md)
also compares recent engine-specific learned pulse/parameter models, complete
waveform generators and offline component characterization. It recommends a
coupled gas/acoustic prototype against the improved-1D control, with the
learned model as a data-dependent additional benchmark. No architecture is
promoted solely from published reconstruction scores.

Use the [implementation specification](stage-S2d-design.md) for the A0/A1/B0/B1
comparison, equations, source/flux ownership and C integration boundaries.
Begin with fixed-background fixtures, then return-load/mean-flow coupling;
validate any reduced-rate scheduler against a common-rate control. Retain
downstream hot/cold gas advection and species inventories. A two-wave line
alone cannot reproduce entrainment or contact fronts. Learned complete-sound
comparators stay outside engine feedback; only identified unresolved-source
parameters may later enter the shared physical graph.

**Candidate A: improve the conservative 1D gas network.** Reconstruct/update
the coupled transported variables consistently, retain robust nonlinear
behavior and bound weak-wave dispersion/dissipation. Test valves, two-rate
interfaces, junctions, boundaries and thermodynamics together. A higher-order
momentum patch alone does not establish higher-order acoustic accuracy.

**Candidate B: a coupled nonlinear gas source and validated acoustic network.**
Use a defined interface where perturbations are sufficiently small for the
downstream model. Return its pressure/load response to the source; otherwise
breathing, pulses and sound respond to different exhausts. Use conservative
matching and explicit bandwidth/overlap rules. This is not simply convolving
an unchanged valve pulse with a convenient impulse response. Strong blowdown,
hot transient flow and nonlinear propagation remain decision constraints.

Test both with the same source states, geometry and error/cost criteria. Choose
one, or a documented component-wise combination, only after comparing results.
[Torregrosa et al.](https://doi.org/10.1016/j.ijmecsci.2017.09.029) show why
engine-flow schemes need several types of tests; [Hota and Munjal](https://www.iiav.org/ijav/content/volumes/9_2004_1722161272264886/vol_3/396_fullpaper_1757111286453860.pdf)
provide a specific coupled-hybrid precedent with explicit nonlinear limits.

## Reference and listening method

Current BMW M4 audio is turbocharged while the I6 model is naturally aspirated;
the Golf recording has a similar mismatch for the I4. Ferrari/Mustang guide
their respective V8 characters but do not establish identical exhausts or
calibration. No supplied boxer example exists. Brighter/darker groups do not
identify verified microphone positions. Seek additional recordings when these
gaps prevent identifying the proposed correction.

Use synchronized RPM/crank phase, throttle/load and known hardware where
available. For unsynchronized examples, compare documented phase-dependent
distributions and perceptual texture, not absolute SPL or claimed exact
waveform error. Retain stereo and average ear **powers** for spectral measures;
do not cancel channels by folding to mono. Filtered references remain listening
aids only. MP3 decoding cannot restore information removed by its codec or
earlier recording processing.

Track crank-order levels, cycle repetition, transient rise/decay and crest,
tonal concentration, broadband modulation and intake/tail/structure balance.
Avoid one scalar realism score. Repeat randomized A/B listening at equal gain
for changes in physical level, plus explicitly loudness-matched comparisons
for timbre when appropriate. Bernardo's acceptance remains a separate gate.

## Research and audit coverage

[The research companion](audio-robustness-research.md) links primary sources,
specifies model validity limits and maps every D01–D09/R01–R18 audit item to
these stages. [The original audit](sound-physics-audit-2026-10-22.md) remains
the issue ledger, including superseded-configuration caveats. Record future
fixes against those IDs and retain rejected controls and unresolved questions.
No status in this plan asserts that these future changes have been implemented.
