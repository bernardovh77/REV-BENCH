# Initial audio bottleneck diagnosis — 2026-10-22

Diagnostic review of production commit `2330da55bc81ebed9a39b677b580335d26b976d7`,
following the selective I6/flat-V8 rollback. Bernardo asks for an architecture
review and an initial hypothesis, using common neural-audio failure modes as
questions to investigate. This is not authorization to implement the audit
backlog, introduce a neural renderer or change the accepted sound checkpoint.

## Architecture actually present

REV-BENCH currently generates sound with physical and procedural models,
without a trained neural audio generator, vocoder, training loop or loss
function. The earlier roadmap explicitly selects physics-generated sound.
The current pipeline is:

1. Zero-dimensional thermodynamic cylinders/chambers, compressible valve
   flows, modeled combustion, crank dynamics and ECU state excite the engine.
2. A staggered finite-volume, one-dimensional intake/exhaust network carries
   the pressure/flow pulses through runners, headers, collectors and exhausts.
3. Outlet volume-flow changes generate monopole pressure, approximately
   `p(r,t) = rho0/(4*pi*r) * dQ(t-r/c)/dt`. Separate procedural jet noise and
   six mechanical modes supply detail absent from the one-dimensional gas.
4. Antialias filtering, listener/scene propagation, directivity, reflections,
   cabin filtering, DC removal, gain and soft limiting produce the audio.

These are reduced physical models, not a resolved three-dimensional turbulent
CFD simulation. Source locations: `core/src/rb_sim.c:174`,
`core/src/engine.c:1281`, `core/src/audio.c:243`, `core/src/noise.c:146` and
`core/src/scene.c:121`. The native CLI produces the engine WAVs in the offline
listening result; C also builds to WASM. The browser's interactive worklet
still uses the stage-2 test tone until stage 14. Python tools handle reference
preparation, measurements and publication; they do not train an audio model.

At 48-kHz output the default/high configuration uses 192-kHz engine/valve
steps and 48-kHz slow pipe steps. Ultra targets 384/96 kHz and halves the
nominal spatial cells. High targets 5-cm runners/headers and 7.5-cm long-pipe
cells; stability and fixed storage can constrain the actual mesh. Pipe-flow
values are held between pipe updates, their changes form derivative impulses
at physics rate, and the FIR reduces to output rate. The coarse CSV telemetry
is diagnostic output, not the signal from which sound is upsampled.
See `rb_sim.c:23`, `engine.c:28,486,502` and `audio.c:243,278`.

Current model selection matters: I6 has the restored twin packed exhaust and
8500-rpm limit; flat V8 has the restored unvalved Tubi-inspired path without
an X and retains 9000 rpm; cross V8 retains its X/rear-delete and 7500 rpm.
Old audit examples involving the superseded I6/flat bypasses or flat-V8 X
must not be described as the current configuration.

## Ranked initial hypotheses

### 1. Numerical acoustic transport limits coherent detail

This is the first inspection priority and has the strongest existing
controlled evidence. The S2b frictionless-pipe fixture uses an 800-Hz packet
with approximately 70-m/s mean flow. After 2.5 ms the shipped limited momentum
scheme retains about **59.9% of perturbation energy**, versus 39.0% for its
donor-cell control; the no-mean-flow result is approximately 100.1%.
See [S2b](stage-S2b.md) and `tests/test_pipe.c:243`.

The revision helps, but substantial numerical loss remains. It cannot be
attributed to the disabled physical wall friction. Limited reconstruction
changes momentum transport only; mass/species/enthalpy transport remains
first order, and extrema/boundaries use donor transport (`pipe.c:454`).
This supplies a plausible mechanism for suppressing loaded detail differently
from low-flow overrun, not proof of the entire robotic-deceleration cause.

The nominal grid also represents short acoustic wavelengths with very few
cells. The project's `c/(pi*dx)` estimate (about 2.4 kHz at its nominal hot
exhaust conditions) is a numerical/model estimate, not a universal hard
cutoff or measured accurate passband. Raising timestep frequency alone
cannot restore spatially unresolved waves. Acoustic amplitude and phase
must converge separately from average torque and conservation. NASA's
[computational aeroacoustics report](https://ntrs.nasa.gov/citations/19960008581)
likewise distinguishes wave-dispersion accuracy from ordinary flow resolution;
its particular points-per-wavelength result is not a validation of this solver.

### 2. Procedural highs are only partly coupled to the modeled exhaust

The high-frequency sources use shaped random noise and simple mechanical
resonators. Their energy can be plausible without reproducing realistic
transient structure, cycle modulation or geometry-dependent transfer.
Confirmed code limitations include (`noise.c:167–176,206–215`):

- Fixed catalyst and muffler transmission factors/filter shapes, independent
  of much of the actual pipe/can geometry.
- A fixed 580-m/s delay and a path omitting center/link sections. Those
  omitted lengths exist in the current I6 and cross V8.
- Assignment of each cylinder's additional exhaust noise to one outlet,
  without the physical solver's X/Y scattering. Current cross V8 and boxer
  are relevant examples; the restored flat V8 has no X.
- Jet velocity/Mach inferred from upstream density/temperature rather than
  throat conditions in choked flow, plus omission of reverse-flow noise.

The earlier audit reproduces these issues. Their net audible importance and
the empirical source coefficients still require calibration. They do not
justify adding arbitrary treble or claiming that more broadband energy alone
means more realism. The existing `dQ/dt` radiation step also means that adding
another generic `dp/dt` block is not automatically a missing-physics fix.

### 3. Excitation and state transitions may be too approximate

Ignition/burn timing causality, cycle variation, residual-gas sensitivity,
fuel-cut/resume behavior and generic mechanical modes remain relevant to
robotic repetition and lift-off character. The earlier audit found modeled
heat release before the recorded spark and a combustion-model discontinuity
at variation zero. The shared code is unchanged by the exhaust rollback.
These are physical correctness concerns, but they have not been shown to
explain all current audible defects. See the confirmed issues and S1/S2
questions in the [sound audit](sound-physics-audit-2026-10-22.md).

### 4. Current evidence does not implicate final compression

Fresh measurements of all 33 current engine float WAVs verify their source
hashes against the current manifest and put every peak below the 0.8 output
limiter knee. The current render logs also report zero limited/non-finite
samples. The I6/cross/flat long-coast results are:

| Current long coast | Peak dBFS | RMS dBFS | Whole-clip crest |
| --- | ---: | ---: | ---: |
| I6 | -18.96 | -35.07 | 16.11 dB |
| Cross V8 | -17.15 | -37.14 | 19.99 dB |
| Flat V8 | -14.52 | -32.45 | 17.93 dB |

RMS averages both ear powers rather than folding stereo to mono. The
publisher applies one constant gain per complete clip to peak 0.6
(`tools/render_results.py:77`): it preserves within-clip dynamics but conceals
absolute level differences between clips. These numbers do not establish
calibrated real-world dynamic range or realistic dynamics in every phase.
Engine rev-limiter spark cuts are distinct from the inactive output limiter.
The hash/measurement record is ignored
`renders/evidence/audio-bottleneck-2026-10-22/measurements.json`.

## First proposed experiment, before choosing a remedy

Keep the current accepted result. In an independently authorized investigation:

1. Use matched RPM/load and the same scene/common gain; inspect cylinder,
   header, collector and tailpipe pressure/flow, plus pipe-only, jet-only and
   mechanical-only audio before and after rendering filters. Separate lift,
   fuel-cut coast and clutch-in. Existing mixed clips cannot isolate layers.
2. Measure magnitude/phase transport in straight-pipe controls across
   frequency, temperature and both mean-flow directions. Then test the full
   exhaust. Compare high/ultra for screening, but independently refine space,
   pipe timestep and engine timestep to attribute changes. Log actual cell
   sizes, stability caps/repairs and CPU cost; the quality switch changes
   several variables and is not a clean convergence experiment by itself.
3. Compare crank-order spectra, transient envelopes, crest and repetition
   against documented real intervals. Distinguish neutral rev-down from
   loaded overrun. Use unfiltered float recordings; unknown RPM/load,
   microphone perspective and recording processing limit causal comparisons.

This separates missing excitation, numerical attenuation, acoustic transfer
and final presentation before committing to a large solver or model change.
More processing is justified by demonstrated realism, not sample rate alone.

## How the suggested neural remedies apply

There is currently no L1/L2 training-loss or HiFi-GAN bottleneck to replace.
More generally, one-shot waveform L2 prediction under phase/timing ambiguity
can average events; this is not a claim that every L1/L2 objective blurs sound.
[DDSP](https://arxiv.org/html/2001.04643v1) demonstrates structured harmonic/noise
synthesis with multiscale L1 spectral loss without adversarial training.
[HiFi-GAN](https://arxiv.org/html/2010.05646v2) combines adversarial, feature and
mel losses. [DiffWave](https://arxiv.org/html/2009.09761v3) itself uses squared
L2 error to predict noise. Those speech/music results do not establish that
any of these models will solve engine realism or meet this browser's budget.

A later hybrid could retain validated firing/flow/propagation and learn
missing turbulence or structural texture as a conditioned residual/parameter
model. Treat that as an experiment. It would need suitably matched recordings
with RPM/crank phase, load, throttle, spark/fuel-cut state, temperature,
hardware and documented listening geometry. The existing reference excerpts
are useful perceptual targets, not paired residual ground truth. Waveform
subtraction additionally requires alignment; stochastic texture can require
distributional comparison. Adding GAN or diffusion does not itself supply
missing physical information or correct control responses.

## Scope and validation

Graphify was queried before code exploration; the existing graph still
matches production code. This review changes documentation only and does not
claim a new semantic graph extraction. Current raw WAVs were measured and
manifest hashes checked; no audio was regenerated and no production/test
sources changed. S1/S1e/S2/S4/full 13b remain open, and stage 14 is unstarted.
The prior native/WASM/browser suite results belong to the rollback checkpoint;
they were not rerun for this documentation review. Claude's setup is preserved.
