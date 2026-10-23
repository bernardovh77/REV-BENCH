# Should REV-BENCH change its sound-generation architecture?

Updated 2026-10-23 for Bernardo's explicit request to research alternative
generation methods. This is a decision proposal informed by the S2c experiment,
not an implemented replacement or a claim of listening improvement.

**Recommendation:** compare an acoustic-preserving 1D control with a coupled
nonlinear-gas/acoustic-network prototype. Keep simulated firing, pressure/flow,
temperature and ECU state as the causal inputs. Test an engine-specific learned
parameter model as an additional research control when suitable real data are
available. A generic waveform GAN/diffusion replacement is not yet supported
by this project's evidence.

The [S2d implementation specification](stage-S2d-design.md) now defines the
experimental arms, architecture sketches, conservative-variable and wave-port
equations, reflected-load/mean-flow coupling, C module boundaries, learned-DSP
deployment and staged validation/runtime gates. It is the design reference
for the native experiments below.

Subsequent [S2d native experiments](stage-S2d.md) implement the first isolated
A1/B0 candidates. The initial conservative control is too dissipative; the
homogeneous zero-flow graph passes its tested 200–4000-Hz numerical band.
This supports continuing the coupling experiment. It does not promote a
production renderer, establish engine realism or validate hot mean-flow behavior.

## Why changing the architecture may help

The experiment exposes numerical dispersion/loss in the current gas network
and shows that upper-RPM cut-state tonal concentration comes chiefly from
pipe-flow radiation. Added procedural noise takes a different, simplified
route. More noise or a faster timestep alone cannot make the two paths
physically consistent. A shared acoustic description could improve coherence,
geometry sensitivity and calibrated bandwidth without requiring the entire
engine to run on a uniformly fine gas mesh.

That last statement is an architectural hypothesis. Prove it by comparing the
same operating trajectories, reflected source loading, amplitude/phase errors,
listener recordings and measured cost. Do not select a rewrite from a paper's
audio demo or one numerical metric.

| Candidate | What could improve | Principal constraint | Decision |
| --- | --- | --- | --- |
| Improve the existing conservative 1D model | Direct physical response to combustion, exhaust changes and nonlinear pulses; lowest conceptual migration | Weak-wave dispersion/damping, split-rate interfaces and cost must all improve; higher order is not automatically better | Required comparison control in S2d |
| Couple nonlinear gas dynamics to a geometry/flow/temperature-aware acoustic network | Accurate propagation/scattering with explicit unresolved sources and common pulse/noise transfer | Return reflected impedance to the physical source; establish small-signal/nonlinear limits, overlap rules and flow-energy accounting | Preferred new architecture to prototype against the control |
| Learn structured pulse/noise/transfer parameters from recordings | More faithful engine-specific texture and economical conventional DSP deployment | Matched state/audio and measured hardware response; learned timbre does not certify breathing or unseen geometry edits | R2/S6 research comparison; use actual simulated ECU events and crank phase |
| End-to-end conditional GAN/diffusion waveform synthesis | Potentially expressive unresolved texture | Data, streaming continuity, causal control, phase coherence, hardware generalization and measured latency remain unproven here | Evaluate only for a demonstrated remaining gap, initially as a bounded residual |
| Offline local 3D/CFD/acoustic characterization | Resolve complex junctions, perforates, local flow-noise sources or structural modes | Appropriate geometry/boundary data and validation still required; no whole-engine browser cost claim has been measured | Supply component data where reduced models fail; not a prerequisite to every improvement |

The gas/acoustic split has a direct engine precedent: [Hota and Munjal's
hybrid thermo-acoustic model](https://www.iiav.org/ijav/content/volumes/9_2004_1722161272264886/vol_3/396_fullpaper_1757111286453860.pdf)
couples a nonlinear source to downstream acoustics, with reflected loading.
Its downstream linearization and single-cylinder validation do not establish
validity for every hot multicylinder transient here. The [engine gas-exchange
scheme comparison](https://doi.org/10.1016/j.ijmecsci.2017.09.029) likewise
supports testing several wave regimes rather than promoting one reconstruction
because it retains more packet energy. Local [1D/3D acoustic coupling](https://doc.comsol.com/6.4/doc/com.comsol.help.models.aco.acoustics_pipe_system/acoustics_pipe_system.html)
is a useful component-characterization method; the cited example assumes no
background flow.

## Finer grids do not add missing acoustic modes

There are two separate bandwidth limits: numerical resolution and the assumed
acoustic modes. [COMSOL's pipe-acoustics documentation](https://doc.comsol.com/6.4/doc/com.comsol.help.aco/aco_ug_pipe.12.18.html)
states that a plane-wave model cannot represent higher transverse modes above
their cut-on. For an ideal, hard circular duct without background flow, the
first nonplane mode cuts on near `f = 1.84*c/(pi*D)`. As an illustrative
calculation, a 73.8-mm bore at `c=350 m/s` gives about 2.8 kHz; this is not a
measurement of the current hot, flowing exhaust. Above that frequency the
plane wave still exists, but additional modes can matter. Finer 1D cells
cannot restore those missing spatial patterns. Wider cans and complex
junctions therefore need a declared validity band and measured/multimode
component descriptions where the plane-wave approximation fails.

## Engine-specific neural research changes the comparison

[Pulse-Train-Resonator, 2026](https://arxiv.org/html/2603.09391v1) learns
firing-aligned pulse shapes and recursive resonators with RPM/torque conditioning.
It reports improved harmonic reconstruction over a harmonic-plus-noise baseline.
Crucially, training/evaluation uses **procedurally generated** engine subsets
at 16 kHz, not seven-and-a-half hours of independently measured real engines.
Its torque-derived throttle/DFCO gates and learned fixed resonators are useful
priors, not substitutes for actual ECU states or verified exhaust geometry.
This supports an engine-specific structured benchmark, not a claim of real
engine equivalence or 48-kHz browser suitability.

[EONE, 2026](https://arxiv.org/html/2606.21521v1) learns harmonic and noise
gain curves across RPM/torque and transfers them to conventional DSP, without
a neural network at runtime. Its twelve-participant listening study favors
that model over the truncated harmonic baseline, while expert listeners still
distinguish recordings. This is stronger application-specific evidence than
assuming a speech vocoder will fit engine sound. It does not demonstrate
physically correct sound from arbitrary user-built exhausts or unmeasured
engines; its 16-kHz experiments also leave upper-band validation to this project.

The related [Procedural Engine Sounds Dataset](https://arxiv.org/html/2603.07584v1)
contains annotated **synthetic** engine audio informed by real recordings.
It can help test controls and compare representations, but using it for both
training and realism validation risks learning its synthesizer's assumptions.
It does not remove the need for independent real recordings, known perspective
and sufficiently synchronized state data.

These are recent primary research reports, not independent validation of a
REV-BENCH implementation. Their reported percentages/MOS are not transferable
project gains. No third-party code, weights or dataset were installed in this
experiment.

## The prototype and comparison method

1. **Correct causal inputs first.** Use actual spark/burn/injection/cut events,
   not torque-sign guesses. Retain gas pressure/flow and crank phase. Eliminate
   known pre-spark heat and disabled-injection sound inconsistencies before
   training a model to imitate them.
2. **Define explicit ports and a shared exhaust description.** Match pressure,
   volume/mass flow, temperature, mean flow and acoustic load at the nonlinear
   interface. Include reflected loading in the engine response. Use the same
   topology for pulses and noise inserted at their physical source positions.
3. **Verify the control and candidate separately.** Straight ducts, step areas,
   junctions, perforated/packed components and outlet load must meet declared
   magnitude/phase/error limits. Check wave growth over long runs and both
   flow directions; conservation or ordinary CFL alone is insufficient.
4. **Compare complete trajectories.** Keep idle, held load, upper RPM,
   actual lift/DFCO/resume, long road coast and neutral fall distinct. Compare
   source balance, crank orders, transient envelopes, repetition, tonal
   concentration and stereo power with common-gain and separately labeled
   loudness-matched listening.
5. **Test unseen controls/hardware.** Hold out recordings, temperatures, RPM
   sweeps and at least one exhaust configuration. A reconstruction that sounds
   plausible but responds incorrectly to pipe length, bank connection, firing
   order or injection state is not a successful physical engine model.
6. **Profile the deployed version.** Compare the same fidelity band without
   taps/file writing, in native C and WASM. Validate callback continuity,
   worst-case processing and memory. Record the benefit that justifies any
   added hardware demand. Apply the gates in the [robustness plan](sound-robustness-plan.md).

The outcome may be a substantially different acoustic renderer while retaining
the existing physical engine. That is a more useful decision to test than
choosing between “physics” and “AI” as mutually exclusive complete systems.
