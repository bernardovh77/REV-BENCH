# Stage 043 — Improved conservative solver and production-mixture control

Bernardo authorizes 043 and then 044, reviewing each predecessor before the
next stage. Baseline: `5efc7d90a48cd8f25204eb00f51534db85980dd2`, clean tracked
tree, user videos/skills untracked and preserved. Local hashes and measurements
live in ignored `renders/evidence/stage-043/`.

## Preflight and frozen experiment

042's delivered-dose authority and no-replay proof are consistent with its
bounded claim. Its instantaneous IVC fueling, real needle forces/timing and
modal calibration remain unresolved for 055/059/063. Preserve 039's acquisition
gap, 041's dilution/COV/repetition findings and 058's I4 coast rise.

This stage implements a standalone uniform-area production-EOS conservative
candidate A2, retaining production A0 and constant-gamma MC/RK2 A1 unchanged.
No production renderer selection, engine adapter or audible improvement is
claimed. A failed candidate completes a rejection experiment, not qualification.

State densities are three species masses, axial momentum and sensible total
energy; rho is their sum. p=rho R(Y) T, E=rho e(T,Y)+rho u²/2,
c²=(1+R/cv)RT. Use gas.c properties without a constant-gamma energy shortcut.
Each face owns one shared species/momentum/total-energy flux. SSP-RK3 updates
all components together; smooth third-order reconstruction falls back to MC at
large relative gradients and to cell averages if reconstructed states fail.
No clipping/renormalization or hidden repair is permitted. Input/domain/CFL
errors fail, and a failed numerical step requires preparation before reuse.

Primary method sources: [INL generalized-EOS HLLC](https://mooseframework.inl.gov/falcon/source/userobjects/ADNumericalFlux3EqnHLLC.html),
[Chen et al., general-EOS Riemann solvers](https://arxiv.org/abs/1903.04568),
and [Clawpack's constant-gamma comparison](https://www.clawpack.org/riemann_book/html/Euler_approximate.html).
These justify testing general-EOS conservative fluxes, not copying
p/(gamma-1) into our sensible-energy mixture. The production apparent
40-bar burned-gas property fit is inherited, not independently validated here.

Freeze before candidate runs:

- Weak sinusoidal waves over 1 m, 200/1000/2000/4000 Hz; air and 90% burned /
  10% air, 300/900 K, mean flow -70/0/+70 m/s, amplitude 0.1 Pa. Mesh
  0.05/0.025/0.0125 m and independently selected 192/384 kHz pipe clocks.
  Screen <=1 dB magnitude and <=10 degrees phase against analytic c+U.
  Unsupported CFL rows reject explicitly; no automatic cell enlargement.
- Finite-amplitude 100/1000 Pa controls, source-free long runs, stationary and
  advected species/temperature contacts, shocks and expansion fans. Periodic
  inventory tolerances: each species and energy 1e-10 relative; momentum
  scale uses rho*c*length, never an energy denominator. Composition-contact
  pressure drift <=1 Pa is a separate physical gate, not waived by conservation.
- Independent constant-property shock/rarefaction reference where the inherited
  EOS has constant cv; identify extrapolated-property controls separately from
  300–900 K production-mixture controls.
- Closed/open neighbor-face controls and integrated boundary books; engine
  tick 96/192/384 kHz held independently of pipe tick in the production motored
  control. The old uncoupled implicit-orifice optimization remains rejected.
  Explicit shared-face transport supersedes that optimization only inside its
  CFL/domain; no new production valve/port adapter is certified.
- Record every fallback/failure, storage and no-file/no-trace CPU cost. No
  promotion from uniform refinement or waveform brightness alone. Full native,
  Python and web regression suites; focused WASM and sanitizers for the candidate;
  deliberate conservation/phase/species/boundary faults must be detected.

## Executed results and rejection

The experiment is complete; **A2 is rejected for production promotion**. Neither
the existing A0 renderer nor the rejected A1 is replaced. This is decision
evidence, not an acoustic improvement or a passing production-mixture adapter.

| 1-m weak-wave frequency | Representable rows | Passing magnitude AND phase |
| --- | ---: | ---: |
| 200 Hz | 72 | 72 |
| 1000 Hz | 72 | 44 |
| 2000 Hz | 64 | 12 |
| 4000 Hz | 40 | 0 |

288 requested rows include 40 grids with fewer than four cells per wavelength.
The first analyzer classified these without a representation guard; some
sampled carriers alias to uniform pressure. Preserve that first classification,
then explicitly label those rows aliasing controls, never acoustic passes.
The remaining 248 rows have 128 passes and 120 failures; none trips CFL.
At 200 Hz the worst magnitude loss is 0.03933 dB and phase error 0.02377°.
Across representable rows, 4-kHz loss reaches 98.25 dB and phase error 165.56°.
This supports only the tested 200-Hz continuum fixture envelope across all
declared mixtures/flows/temperatures; passing isolated higher rows does not
establish a general engine bandwidth.

Stationary air/300-K versus 90%-burned/450-K contacts preserve pressure to
printed precision at 80/160/320 cells. At ±70 m/s, 80/160-cell cases develop
155.234/133.475 Pa false pressure excursions. The 320-cell cases reach
117.076 Pa and reject an inadmissible stage after 756 accepted steps
(1.96875 ms), at max CFL 0.20388. No invalid step is committed and no species
repair is applied. The first probe incorrectly printed requested time for
failed runs; the corrected probe reports accepted time and these contacts were
rerun. No claim of a 2-ms successful fine-grid run is retained.

This is consistent with the multicomponent pressure-equilibrium problem,
but the fixture does not identify every contribution. [Abgrall's primary
paper](https://doi.org/10.1006/jcph.1996.0085) and
[Ching/Johnson/Kercher](https://arxiv.org/abs/2501.12532) describe the
pressure-equilibrium/conservation difficulty; the latter also distinguishes
semidiscrete energy conservation from a fully discrete guarantee.
[Coppola/Aiello/De Michele](https://arxiv.org/abs/2605.03617) propose EOS-dependent
pressure-preserving conservative fluxes. These are research leads requiring
fresh positivity/shock/fully-discrete energy/cost proofs, not an implemented fix.
No pressure overwrite, energy reset or double-flux relaxation was added to
make the contact pass. 047/048/057/060 must retain this rejection evidence.

Periodic weak-wave species/momentum/energy balance stays within 1.476e-13
of the separately declared scales; the maximum across successful wave,
contact, finite-amplitude and boundary controls is 6.443e-13. This demonstrates
shared-flux cancellation, not acoustic or property validity. Six independent
exact-Riemann shock/expansion rows converge at 200/400/800 cells: shock max
normalized L1 error falls 0.003652→0.0009384; expansion 0.003107→0.0008245.
These controls deliberately use the inherited constant-cv extrapolation below
200 K. They do not establish real-gas high-temperature shock validity.
100/1000-Pa, 3/30-ms periodic runs remain admissible with no reconstruction or
flux fallback; 30 ms is the longest source-free observation, not a lifetime test.
Open/closed boundary books pass. The EOS residual is checked, but internal
Newton-iterate guards and the apparent burned-gas equilibrium fit are not
independently characterized by this prototype; 047 retains that obligation.

A2 no-trace/no-file stepping costs 0.000255–0.0663× real time across the
representable duct sweep (median 0.00922×), including every cell and RK stage.
The largest measured row owns 250,032 bytes. CPU timing excludes setup and
output; the concurrent host runs are a cost characterization, not a controlled
production speed comparison or browser deadline qualification. This expensive
uniform-grid approach is not an efficiency solution.

The unmodified A1 rerun retains its rejection: 2 kHz/300 K/192 kHz with
0.025/0.0125-m cells gives carrier magnitude 0.03298/0.48834 and phase
−9.7246/−12.8817°. The 0.05-m row is explicitly excluded by A1's own
points-per-wavelength guard. A0 production source hashes and accepted audio
remain unchanged; A0 is not retroactively called conservative.

## Valve/neighbor ownership and independent clocks

The old frozen-property implicit orifice solve remains rejected; no improved
convergence claim is made for it. A2's shared HLLC face responds to both
current neighbor states, transports all conserved variables once, and closes
wall/open-boundary impulse and energy books under its explicit CFL. That
supersedes the uncoupled optimization only as a **standalone method under
test**. It lacks a qualified Cd/valve/0D production interface; because A2 fails
the preceding physical gates, implementing/promoting that adapter is rejected
here, rather than exempted from future valve acceptance.

Rerun A0 motored/adiabatic/1500-rpm/WOT I4 and cross-V8 controls with the
pipe clock fixed at 48 kHz: pumping pressure at 96/192/384-kHz engine clocks
is I4 9164.352/8223.509/8213.959 Pa and cross
7760.508/6933.970/6935.043 Pa. Relative to 192 kHz, 96 kHz differs
+11.441/+11.920%; 384 kHz differs −0.1161/+0.0155%.
At the independent fixed 192-kHz engine clock, I4 pipe clocks
24/48/96 kHz give 8159.282/8223.509/8952.834 Pa. Spatial geometry is fixed
for the engine-clock comparison; pipe-clock-dependent production preparation
remains its own confound. This is not evidence that increasing pipe rate
alone qualifies the production solver. All eight controls settle within
23–33 cycles. Existing valve/two-rate/conservation tests also pass.

## Validation and review before 044

- Full native: **221 tests / 4,843,441 checks**, all pass.
- Focused candidate native: 4/987. Same-source WASM: 9/1212, including old FV
  controls. ASan/UBSan: 10/1214; leak detection disabled under ptrace, no leak
  qualification claimed.
- Python: 79 pass, no skips; exact-reference textbook Sod, uniform/mirror and
  symmetric expansion checks are independent of candidate output.
- Web: 17 pass plus typecheck/production build. The production WASM source
  fingerprint is unchanged; the isolated experiment is not linked into it.
- Six compiled faults caught: wrong pressure work, propagation speed, detached
  neighbor pressure, leaking wall, wrong boundary work and failed-state reuse.
- A probe indentation warning was fixed. Full source/commands, first-run
  classification and failed cases remain in the ignored evidence directory.

Reproduce with `make BUILD=/tmp/rev-bench-043 LTO=0` targets `rb_tests`,
`mixture_probe`, `mixture_engine_control`, `acoustic_candidate_probe` (each
prefixed by the BUILD directory). Run `tools/check_mixture.py --probe
/tmp/rev-bench-043/mixture_probe --out renders/evidence/stage-043/numerics`
with the analysis Python; `tools/plant_bugs.py 043 /tmp/rev-bench-043-faults all`
plants the six faults in isolated copies. The command logs/hashes retain
strict O3/C11/no-LTO/disabled contraction, rate, mixture and grid provenance.

Graphify queried before exploration and refreshed: 2963 nodes / 8116 edges /
167 communities; known rb.h parser/inferred-edge limits remain. No Claude
setup, reference split, accepted engine/exhaust parameters or production core
source changed. No new render is justified by this rejected isolated path;
the consolidated page and all accepted raw listening examples are preserved.

Review decision: close 043 as a completed **rejected-candidate experiment**.
It provides no qualified A2 artifact for 045/057. Keep a future conservative
replacement gate explicit; the B0/B1 comparison arm remains independent.
044's compiler depends on 037/038 and may proceed after this review. S1/S2,
039 matched acquisition, listening acceptance and runtime goals remain open.
