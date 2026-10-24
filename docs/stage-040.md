# Stage040 — Spark, ignition and flame-development causality

In progress after review038 and the qualified039 protocol. Baseline1f964f4
physics;039 reference work changes no engine/audio code. User authorizes through
040 and review of each predecessor. Matched039 calibration evidence remains
pending and does not gate independent numerical040 fixtures.

## Frozen physical question and research

The previous engine schedules `spark()` at the randomly shifted burn-start
angle, rather than the commanded spark angle. Negative variation therefore
allows modeled chemical heat before the intended discharge. A baseline-compatible
independent commanded-angle oracle at held3000rpm/.6 throttle/variation1 over0.5s
measures154.44J (I4),247.11J (I6),356.42J (cross V8),276.62J (flat V8),160.54J
(boxer) before command. This is a causality failure; it does not diagnose all
robotic deceleration sound or supply a calibrated timbre correction.

Primary basis: [Schiffmann/Reuss/Sick](https://doi.org/10.1177/1468087417720558)
distinguish discharge and early flame growth; [optical early-flame study](https://doi.org/10.2516/ogst/2017028)
examines early development and subsequent combustion; [ignition-strategy experiment](https://doi.org/10.1021/acs.energyfuels.0c02698)
shows that mixture/flow/discharge conditions matter. These support ordered
spark→kernel→bulk heat, not a universal jitter distribution or an audio-derived
misfire threshold. No explicit autoignition model is introduced.

Minimum causal correction: discharge at the frozen commanded angle, then a
nonnegative development interval. Retain the positive arm of the existing
onset-variation parameter as a bounded surrogate; clip the unphysical negative
arm to zero. Zero-delay means unresolved kernel development within the existing
Wiebe curve, not physically instantaneous complete combustion. State-conditioned
kernel-delay statistics/extinction calibration remain041. Do not invent new
noise, random pops, fuel losses or afterfire to resemble a recording.

## Architecture and predeclared gates

The physical engine owns a uint64 step clock and per-cylinder charge sequence.
Separate scheduled angle, commanded/discharged event, kernel progress and heat.
Native optional caller-owned event records include tick, fractional-step time,
cylinder/charge identity, angle, delay and fuel, with counted bounded overflow.
No callback allocation/I/O. Full public event/snapshot/WASM transport stays061.

The discharge and kernel occur within the actual step interval. Chemical heat
can use only elapsed time after discharge and after nonnegative kernel delay;
no modulo wrap can turn a small negative age into nearly720degrees. Inhibit
new discharge for ignition off or hard/soft cut. A previously lit flame may
continue after switching ignition off. Distinguish no reactants, pending-kernel
cancellation at EVO, and partial burn; remaining fuel stays in species books.
No unsupported thermochemical flammability/extinction threshold is claimed.

Gates: zero pre-command/pre-discharge heat at1e-12J step tolerance; exact event
ordering at zero delay and across cycle wrap; first-step heat bounded by the
post-event interval; closed mass/species/fuel/energy/crank-work books at existing
native tolerances. All five presets: startup, idle, cut/resume, overrun and
retained-fuel scenarios. Exercise zero/nominal/high variation, positive delay,
empty mixture and event overflow. Taps on/off preserve physical state. Plant
an early-heat fault and incorrect scheduling/delay/cut faults in copies; checks
must reject. Rebuild native/WASM and full34 float renders, then publish the one
results page preserving039 controls and all prior research/reference sections.

Review039 first: preserve its freeze, uncertainty and exclusion rules. No use
of its new validation/quarantine sources for fitting or listening-based tuning.
