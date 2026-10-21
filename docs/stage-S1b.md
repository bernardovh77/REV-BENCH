# Stage S1b — Finite-time partial burns

Date: 2026-10-21. Complete as a substage. Native, WebAssembly and browser
regressions and the five-preset before/after audit pass. S1 remains in progress.

The charge model already predicts slower flame development for lean or diluted
charges. Its duration ceiling hid the consequence: even a charge that needed
longer than the interval before exhaust valve opening could finish burning.
With engine life enabled, the model now retains that longer demand and the
existing exhaust-valve event ends the burn, leaving the remaining fuel and air
in the gas. No random misfire probability or residual-fraction cutoff was added.

This is a model of **finite-time partial combustion**, not spark-kernel failure.
It does not complete S1's separate work on true ignition misfires or the low
band's excessive repetition.

## Equations and behavior

The stage-9 entrainment/burn-up approximation is retained. At intake valve
closing (IVC), with engine speed ratio `n`, kinematic-viscosity ratio `nu`, and
laminar flame-speed ratio `s_l`, the duration factor is

```text
n   = max(rpm, 100) / 3000
nu  = (T_ivc / 360)^1.7 * (122000 / p_ivc)
s_l = S_L(T_ivc, p_ivc, residual, phi) / S_L(360, 122000, 0.04, 1)
f   = 0.6 + 0.4 sqrt(n nu) / s_l
```

The existing gasoline flame-speed fit includes temperature, pressure, mixture
strength and a dilution multiplier `max(0.1, min(1, 1 - 2.1 residual))`. This fit
and its floors remain approximations outside their calibrated region; they do
not establish a universal flammability limit.

Previously, for reference duration `D0`, the base duration was

```text
D_cal = min(180 degrees, D0 * min(2, max(0.5, f)))
```

The new internal `rb_burn.demand_duration` records

```text
D_demand = min(720 degrees, D0 * max(0.5, f))
```

The 720-degree ceiling is a numerical bound of one engine cycle, not a misfire
threshold. With `variation > 0`, the existing seeded per-cycle and per-cylinder
duration variation acts on this demand, with its final duration also bounded to
720 degrees. With `variation == 0`, the old base and 180-degree ceiling remain.
Negative or NaN variation still counts as off, as before.

The ECU's spark advance and the existing start jitter continue to use `D_cal`.
The ECU therefore does not compensate for each actual slow burn by advancing
the spark to an unrealistic new best-torque setting. No extra random draws were
added, preserving the seeded sequence at unchanged operating points.

The normalized Wiebe law is unchanged:

```text
W(angle, D) = [1 - exp(-a (angle / D)^(m + 1))] / [1 - exp(-a)]
```

It is zero before the burn starts and one after duration `D`. If the available
angle `A` from the actual burn start to exhaust valve opening (EVO) is less than
`D`, the completed fraction is approximately `W(A, D) < 1`, with the normal
time-step discretization at the event. EVO clears both burning and after-spark
state. Remaining species are not discarded: normal gas exchange carries them
through the exhaust and overlap, providing the model's existing coupling
between cycles. Heat release remains proportional to fuel actually consumed.

## Why no fixed residual cutoff

High residual alone does not specify whether a charge can burn: its temperature,
pressure, mixture, speed and available expansion time matter. The current warm
flat-plane V8's sampled idle contains roughly 46–51% burned gas and still has
enough time for its modeled burn. A rule that declared every charge above 25–35%
residual a misfire would damage an existing preset without establishing the
missing ignition physics.

The pre-change audit runs each preset from its start into a 30-second idle, and
for 10 seconds held at 3000 rpm wide open, sampling after the first six seconds.
Its largest sampled warm-idle factors and durations were:

| Preset | Maximum raw factor | Maximum varied duration | Sampled idle speed |
|---|---:|---:|---:|
| I4 | 1.125 | 80.4 degrees | 784–809 rpm |
| I6 | 1.193 | 84.8 degrees | 686–709 rpm |
| Cross-plane V8 | 1.519 | 108.1 degrees | 640–659 rpm |
| Flat-plane V8 | 1.977 | 139.2 degrees | 976–1017 rpm |
| Boxer-4 | 1.123 | 78.6 degrees | 721–763 rpm |

The old factor and duration caps are inactive in those sampled warm states.
This supports preserving nominal calibration; it does not imply that startup,
limiter operation or deceleration is identical. The flat-plane rev render does
change, as reported below. The held-load comparison at 3000 rpm is a regression
point, not a replacement for a complete dyno sweep.

## Completed before/after audit

All five engines were started and run for 30 seconds with life enabled; warm
idle statistics cover seconds 6 to 30. Held-load runs cover 10 seconds, with
statistics after second 6. The 3000 rpm wide-open mean brake torque is unchanged
to the audit's six decimal places for every preset:

| Preset | Before / after mean torque | After warm-idle range |
|---|---:|---:|
| I4 | 168.745934 N m | 784.27 to 808.56 rpm |
| I6 | 262.767990 N m | 686.03 to 708.63 rpm |
| Cross-plane V8 | 413.228004 N m | 639.75 to 658.17 rpm |
| Flat-plane V8 | 352.384623 N m | 976.08 to 1016.75 rpm |
| Boxer-4 | 171.740709 N m | 720.86 to 762.95 rpm |

The V8s' startup changes leave slightly different later idle trajectories, but
both remain stable. Idle brake torque is near zero, so relative percentage
changes there would be misleading. No preset gains or physics quality settings
were retuned.

## Tests that can fail

Three integration tests in `tests/test_variation.c`, registered in the normal
test runner, exercise the new behavior:

- `test_variation_partial_burns`: an AFR-26 charge settles and is observed at six
  EVO events. Life off retains the previous completed burn; life on has a
  duration above 180 degrees, a 10–85% completed fraction, remaining fuel, and
  the expected Wiebe completion at the event. EVO stops the burn.
- `test_variation_partial_burn_books`: over 30 cycles, total mass, sensible
  energy and all three species balance. Unburned fuel leaves in the exhaust;
  it cannot vanish at EVO.
- `test_variation_partial_burn_residual`: installs controlled charges at IVC
  with 4% and 44% burned gas, keeping 360 K, 122 kPa, stoichiometric AFR and
  3000 rpm equal. The generated start is pinned to the same fixed spark in
  this test to isolate residence time. Greater dilution produces more than
  three times the duration demand and incomplete combustion at EVO. Those
  residual fractions are test inputs, not production thresholds.

All five deliberately planted bugs in `tools/bugs/stageS1b.py` were caught:

| Bug | Test that fails |
|---|---|
| Still use capped demand with life on | partial_burns |
| Use uncapped demand with life off | partial_burns |
| Ignore burned-gas dilution in flame speed | partial_burn_residual |
| Continue burning past EVO | partial_burns |
| Discard unburned fuel at EVO | partial_burn_books |

Each mutant was compiled from one source snapshot in an isolated scratch copy
with portable GCC and run against its named test. No mutation was missed; no
run relied on a timeout or compiler failure to count as caught.

The complete native suite passes: **151 tests, 2455 checks**, built with portable
GCC 16.2 on Windows. `native/csv.c` now opens output in binary mode so its existing
LF line endings remain the same on Windows and Unix; the pre-existing CSV test
passes. This is a host output correction, independent of the combustion model.

The WebAssembly core was rebuilt with isolated wasi-sdk 20 / Clang 16 and is
120,944 bytes. All 16 web unit tests pass, including the source fingerprint
check. Browser tests pass on both the development server and the production
preview, using installed Chrome through `PW_CHROMIUM`. Claude's configuration
and tool environments were preserved.

## Sound check and listening checkpoint

`make render` regenerated all 23 WAVs. Every render reports zero non-finite
samples. Twenty-two report zero soft-limited samples; the flat-plane V8 rev
render reports **six soft-limited samples** and needs listening review.

Compared with the preserved previous Linux renders, 15 WAVs are byte-identical.
The other eight are the revs and pulls. Seven of those have matching energy
above 2 kHz, A-weighting loss and crest factor to 0.0001 dB. Such tiny differences
across operating systems/compilers are not evidence of audible improvement.

The flat-plane V8 rev is the material exception:

| Whole-render measurement | Previous | S1b | Change |
|---|---:|---:|---:|
| Energy share above 2 kHz | -27.2063 dB | -27.3897 dB | -0.1834 dB |
| A-weighting loss | 4.7764 dB | 4.7089 dB | -0.0675 dB |
| Crest factor | 21.4628 dB | 23.8431 dB | +2.3803 dB |
| Absolute peak, full scale 1 | 0.641632 | 0.821655 | +0.180023 |
| RMS, full scale 1 | 0.054218 | 0.052788 | -0.001430 |
| Time of largest peak | 6.7850 s | 6.5639 s | transient moved |

The largest peak is within the floored/limiter part of the scripted rev. The
crest increase comes with a higher peak and slightly lower RMS, rather than
an overall level increase. The first S1b render at gain 0.4 reaches the soft limiter briefly.
The Makefile render recipe now uses gain 0.38 for this one WAV
to provide headroom for analysis. Its final rerender peaks at 0.781 with
zero soft-limited or non-finite samples. This changes
only the example render recipe, not the physics, engine calibration or browser
gain. The table above deliberately retains the initial same-gain comparison,
so reducing the example level does not conceal the higher transient.
This report does not call that a sound-quality improvement. Bernardo should
compare `renders/v8-flat-rev.wav` with the prior version, particularly around
6.5–6.8 seconds and the return to idle, for an objectionable isolated transient.

No sound was traded for speed, and no quality default was reduced. Only the
flat-plane rev example recipe gains the explicit analysis headroom above. The I4's
existing idle remains byte-identical; its low-band repetition measured in S1a
is still +20.4 dB, above S1's 0 to +15 dB target. There is no new listening claim
that the normal idle now sounds less like a loop.

## Remaining work

- Listen to the changed flat-plane V8 transient; the lower example gain
  provides measurement headroom and is not evidence of improved physics.
- Continue S1's low-band variation work and genuine ignition-misfire model.
  Preserve the distinction between a slow burn stopped by EVO and a spark
  that fails to establish a flame.
- The knowledge graph is refreshed with the isolated graphify installation
  after the final documentation and reference-tool changes.

## Sources and limits

- The retained entrainment/burn-up and laminar-speed approximation is documented
  with its stage-9 sources in `docs/stage-09.md`. S1b removes a calibration cap
  in the life-enabled path; it does not establish a new measured flame-speed
  fit, universal residual threshold, or calibrated misfire frequency.
- [Kaul, Wagner and Green, SAE 2013-01-0270](https://saemobilus.sae.org/articles/analysis-cyclic-variability-heat-release-high-egr-gdi-engine-operation-observations-implications-effective-control-2013-01-0270):
  diluted SI combustion develops cyclic variability and ignition/propagation
  behavior depends on charge composition, including prior-cycle residuals.
- [Maldonado and Kaul, Evaluation of residual gas fraction estimation methods](https://journals.sagepub.com/doi/10.1177/1468087420983087):
  residual composition provides cycle coupling, and control-oriented combustion
  maps require calibration. Its fitted parameters are not transplanted here.
- [Stiffler and Drallmeier, Investigation of the cyclic dynamics in the misfire
  and partial burn regime](https://scholarsmine.mst.edu/mec_aereng_facwork/4876/):
  experimental weak-cycle/strong-cycle sequences support retaining residual
  coupling. They do not validate S1b's numerical ceilings or its extreme-lean
  predictions for the five presets.
