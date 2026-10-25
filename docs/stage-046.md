# Stage 046 — Frozen signed mean-flow coupling

2026-10-25. After reviewing045's passing bounded mixture/B0 proof and explicit
cost/domain limits, Bernardo's authorization proceeds to046. Baseline commit
`de936e7`; frozen045 executable and source hashes remain available. Only046
is authorized after this review; stop before047. Evidence is ignored under
`renders/evidence/stage-046/`. Preserve all accepted audio and Claude setup.

## Model and frozen acceptance

The surviving coupled research arm needs actual mean/species/energy transport,
not only a retimed sound delay. Keep one physical conserved state and the
production mixture EOS. With frozen uniform U, disturbance momentum
j=delta_m-U*delta_rho and pressure coordinate
pi=beta*(delta_E-U*delta_m+U²*delta_rho/2)+sum(As*delta_rho_s).
Acoustic coordinates (pi±c*j)/2 travel at U±c; contact/species coordinates
travel at U. Use the complete reference flux plus the linearized conserved
Euler Jacobian and conservative nonlinear residual. Total enthalpy includes
the reference kinetic term; do not add pressure work twice.

Use separate B0 traveling-line delays L/(c+U) and L/(c-U), with a conservative
overlap map between their different physical grids and the retained contacts.
No zero-flow multiport scattering law is reused: this supported experiment
has one uniform two-terminal duct. Junctions with flow remain049. Boundary
R is a specified pressure-wave load on a permeable uniform-flow reservoir,
not a rigid closed wall that somehow passes steady gas.

For isentropic acoustic disturbances, Q'=A*u' while
mdot'=rho0*Q'+A*U*rho'. Pressure/mass-flux impedances are c/[A*(1±M)];
pressure/volume-velocity impedance remains rho0*c/A for a plane component.
The uniform-medium quadratic diagnostic has eac=p'²/(2rho0c²)+rho0*u'²/2
and Fac=U*eac+p'*u'. Track directional storage and signed source/terminal
flux with these convective weights. This diagnostic is distinct from full
thermodynamic total-energy conservation and is never added as another gas E.

Primary references: [Myers's disturbance-energy paper](https://www.cambridge.org/core/journals/journal-of-fluid-mechanics/article/abs/transport-of-energy-by-disturbances-in-arbitrary-steady-flows/8239566249EBB907CB9D5BEEB94CD004),
[linearized Euler formulation](https://doc.comsol.com/6.3/doc/com.comsol.help.aco/aco_ug_aero.08.123.html),
and [045](stage-045.md). Their results motivate the equations, not certify our
mixture/characteristic projection or the performance of a complete engine.

Freeze before046 measurements:

- U=-70/0/+70m/s, air and Y=[.1,.001,.899],300/900K,p0=101325Pa,
  A=.003m²,L=1.50317m, interface.313/.557m,192kHz. Preparation rejects
  |U|/c>.25 and unsupported topology/state; no transonic or strong-wave claim.
- Gaussian amplitudes1e-5/.001/.01: independent Galilean-transformed,
  variable-cp simple-wave reference, <=2% pressure-waveform L2 before shock.
  Weak200/2000/4000Hz lab-frequency carriers: <=1dB/10°; also report envelope
  L2 without replacing either declared transfer gate by a brightness score.
- Independently measure source→terminal forward and terminal→interface/gas
  return timing under R=-1/0/+1; ideal U±c paths within5ticks, received/used
  return current-tick equality. Keep zero-U trajectories against frozen045.
- Contact patches initialized as conservative cell averages at uniform p/T/U:
  both signs cross the interface. Each excess-species transfer agrees with
  analytic U advection within5% of contrast transfer; pressure drift<=1Pa,
  velocity drift<=.001m/s. Global/per-region books retain1e-10 relative,
  quantity-specific scales. Distinguish Q' from mdot' with an independent
  port/accepted-flux control, not a duplicated helper formula alone.
- Full mass/species/momentum/energy accounts and signed acoustic-work residual
  <=1e-10 of the relevant inventory/work scale. Long1s, three loads at both
  flow signs. Allow physically explained gain: maximum perturbation energy
  <=1.10*max(1,R²*(c-U)/(c+U)) initial; final<.001 initial with matched left
  leakage. A no-gain rule borrowed from quiescent acoustics is invalid here.
- Independently vary96/192/384kHz clock and retained upstream spatial count.
  Report state/domain/CFL/limiter activity, no-file/no-trace full cost and
  storage. No real-time/efficiency claim from this correctness prototype.
- Native first; full native/Python/web regression, focused same-source WASM,
  sanitizers and faults for flow direction, missing density flux, dropped
  entropy transport, kinetic/enthalpy terms and convective work.

Status: complete within the scope measured below; HOLD before047. No production renderer choice, engine clip change,
resolved thermal-front/reversal transient or closure of039/S1/S2 is implied.

## Implemented contract and numerical method

`gas_graph.[ch]` specializes the actual B0 receive/commit path for its supported
one-edge/two-terminal geometry. The default B0 solver and production engine
remain unchanged. Positive U points from retained gas toward the right terminal.
Preparation fixes the reference state and rejects |M|>.25, inadequate delays,
upstream CFL/capacity violations and invalid physical inputs. The tested Mach
values are approximately .11–.20 in magnitude, not every value up to the guard.
One failed step poisons the wrapper; reprepare is required. No allocations,
rebuild or coefficient changes occur inside a tick.

The forward and backward B0 lines use L/(c+U) and L/(c-U), each with its own
orthogonal fractional-filter memory and signed inventory. A prepared sparse
physical-overlap map projects the backward line onto the forward/common control
volumes. Its conservative adjoint inserts residual changes into that line and
its fractional memory. No cell is rounded away and no second downstream gas
solver evolves a duplicate physical state. Memory remains caller-owned.

For reference H=h0+U²/2, density perturbation dr and momentum perturbation dm,
the linear conserved flux is F0 plus:

```
species: U*drho_s + Y_s*(dm-U*dr)
momentum: 2U*dm - U²*dr + pi
energy: H*dm - U*H*dr + U*(dE+pi)
```

The nonlinear residual is the full production-mixture Euler flux minus this
Jacobian flux. Acoustic/contact viscosity is measured relative to U, so a
constant transported composition contact is not damped as if it traveled at
sound speed. The two acoustic characteristics retain045's weak-linear/WENO
reconstruction and SSP-RK3 substeps at U±c. All three contact coordinates use
conservative upwind MC reconstruction and the same RK weights over both
physical regions. The shared interface and both external boundaries integrate
the actual accepted flux once. Compensated sums retain long mean-background
flux books without cancellation drift. Incoming contacts at the reservoir
boundaries have the declared reference state.

The moving-medium acoustic diagnostic weights each delay line's squared
pressure/filter storage by (1±M)/(rate*Z), Z=rho*c/A. Source and terminal
work use the matching signed directional flux. Residual coordinate changes
are recorded as coordinate work; B0's original mathematical delay-energy book
is retained separately. Neither quadratic diagnostic is added to physical
total energy. A zero-flow branch preserves045's physical trajectories exactly.

## Measured results

The entire frozen main matrix passes. Raw CSVs, per-case command/probe hashes,
independent reference arrays and `summary.json` are in
`renders/evidence/stage-046/matrix-1/`. The independent Python reference gains a
Galilean transform; a separate perfect-gas test checks both its characteristic
foot and transformed total energy. It does not read candidate travel/phase.

| Check | Executed coverage | Worst result |
| --- | --- | --- |
| Gaussian pressure shape | 72 primary rows plus8 clock/4 spatial controls | 1.11991% L2, below2% |
| Weak carrier transfer | 72 rows,200/2000/4000Hz | 0.027246dB,0.031316°, below1dB/10° |
| Weak carrier envelope | Same72 rows; reported separately | 2.52668% L2 at900K air,−70m/s,4kHz,.557m partition |
| Travel and return | 18 cases: three flows, two partitions, three loads | Forward0.313731tick; nonzero return0.475306tick |
| Matched return | Six R=0 cases at1e-5 pulse amplitude | 7.208e-7Pa maximum |
| Current feedback | Every measured tick | received pressure equals used pressure exactly |
| Zero-U preservation | 48 profiles versus frozen045 executable | All pressure samples exactly equal |
| Moving composition | Eight ±70m/s,300/900K/two-partition cases | 0.034266% contrast-transfer error,5.530e-10Pa false pressure,1.379e-12m/s velocity drift |
| Regional physical books | Profiles, returns, contacts and1s cases | 7.102e-13 normalized species/momentum/total-energy residual |
| Convected acoustic book | Same cases; scaled to initial perturbation energy | 1.406e-14 relative residual |
| Residual/state handling | Main matrix | Max residual CFL0.046493; zero reconstructed-state fallbacks or domain rejections |

The2% gate applies to Gaussian pulses in046. Carrier magnitude and phase have
their separately frozen gates; the2.527% envelope result is not hidden or
relabeled as passing045's stricter all-profile shape gate. All declared046
thresholds were written before its measurements and remain unchanged.

Six compact-duct one-second cases cover both flow signs and R=-1/0/+1.
Maximum sampled perturbation/initial energy is1; final ratios are between
5.134e-21 and2.899e-20. These leaky short-duct cases alone do not demonstrate
legitimate mean-flow amplification. Three additional resolved long-duct controls
at−70m/s do: R=-1/+1 peaks are1.505989/1.506029 initial, versus ideal
(c-U)/(c+U)=1.504971; matched R=0 peaks at1.000704. Physical books remain below
4.578e-15 normalized, with acoustic residual about3.594e-24J. The signed work
accounts for this bounded increase. Imposing the zero-flow no-gain rule would
misdiagnose it. These supplemental runs last8ms; the separate six runs supply
the declared1s duration.

The native actual-port fixture independently checks accepted acoustic mass
transfer against A/c*[(1+M)a-(1-M)b]. The quiescent rho*Q shortcut differs by
more than10% in that control and is rejected. A supplemental entropy-contact
fixture uses the same composition at fixed pressure with a +1mK temperature
contrast, in both directions; all species, momentum and energy contrast
transfers agree with continuum advection within5%. This is a weak entropy-mode
check, not the thermal-front/coefficient-update qualification of047/048.

## Validation, failures and performance

- Full native regression: **228tests/4,847,488checks pass**. After adding
  intermediate-return inventory checks and the supplemental entropy fixture,
  final focused native, same-source SIMD WASM and ASan/UBSan runs each pass
  **8tests/5,443checks**. The measured implementation is unchanged during those test-only extensions;
  these final extensions add test coverage. No leak qualification is claimed
  with ptrace-required leak detection disabled.
- **88 Python tests**, no skips, and **17 web tests** plus the production web
  build pass. Six same-source native/WASM trajectories (profile/return/contact
  for both signs) have **exactly equal CSV columns**, including conserved/EOS
  output, time, reflected pressure and perturbation energy. No application
  engine worklet or runtime deadline is claimed.
- Nine deliberate behavioral fault checks are caught: wrong forward delay,
  missing density mass flux, dropped composition advection, dropped entropy
  advection, wrong negative-flow upwind direction, omitted kinetic enthalpy,
  quiescent energy weights, missing convective work and missing backward
  overlap. The last initially escaped an end-only observer after the returning
  wave had left the duct. Its original missed result is retained; intermediate
  regional books now catch it. The physics/thresholds did not change to force
  that failure to pass.
- The first web-build command ran from the repository parent context and
  selected an unrelated ancestor package; it failed before building anything.
  The corrected command in the isolated existing REV-BENCH web workspace
  passes. No package/dependency change was made. The045 predecessor review
  also clarifies its prose sign: raw B0 balance=-coordinate_work, so the
  closed observer is raw balance+coordinate_work=0; executable code was correct.
- Eighteen interleaved, no-file/no-trace **20ms** cost controls (six per U)
  on this Intel i7-1355U, Linux GCC C11 -O3, no LTO, strict double arithmetic:
  median simulated/CPU-time factors **0.002298× at−70**, **0.003334× at0**,
  **0.003262× at+70**. Observed ranges are respectively0.001775–0.002951,
  0.002769–0.004224 and0.002778–0.004116×. Prepared storage for those fixtures
  is389112/280192/300472bytes. Host load and short-run variation are retained;
  these are not comparable whole-engine speedup measurements.

This expensive full-resolution nonlinear correction grid establishes a
correctness reference. **It is far below real time and is not an efficiency
solution.** Sparse/adaptive correction, component validity and complete cost
comparison remain052/060/062; the coupled arm is not preselected for production.
The rejected043 mixture arm is not rehabilitated by these results.

## Reproduction and handoff

```
make -j2 BUILD=/tmp/rev-bench-046 LTO=0 /tmp/rev-bench-046/gas_graph_probe /tmp/rev-bench-046/rb_tests
.venv-analysis/bin/python tools/check_mean_gas_graph.py --probe /tmp/rev-bench-046/gas_graph_probe --baseline /tmp/rev-bench-045-final-probe --out renders/evidence/stage-046/matrix-1
```

The045 baseline is a frozen prior executable, not an executable rebuilt from
current046 sources. Each raw case records its actual binary SHA256 and full
command. The measured046 native probe SHA256 is
`20fe3b8c7868b1350ba4dff1d23d7b3598bf3359e4154bba888fa370a7b92ec8`.
The six-trajectory WASM probe SHA256 is
`df1c7ab54330c24094cf67c5b7c2ad03c1f0cdc4412c543fb91b660ad64b9be5`.
The source/validation manifest records final sources and executable hashes;
ignored evidence includes the original first-pass checks and fault miss.

For a resolved gain control use the probe arguments
`stability 192000 300 0 1.50317 .313 86 .00001 0 .008 -1 .003 -70`, then
repeat reflection0/+1. Cost controls use mode`cost`, duration`.02`, amplitude
`1e-5`, reflection0, area`.003`, and six interleaved repetitions of U=-70/0/+70.
The two final optional numeric arguments are area and signed mean velocity.

`tools/flow_checkpoint.py` publishes045/046 summaries and the scientific figure
in the registered `flow-checkpoint` section; the standard
`tools/render_results.py --publish-only` rebuilds the complete single-page
checkpoint without analysis dependencies or rerendering accepted sound. All
prior audio, coasts, numerical plots and reference perspectives are retained.

**Outcome:046 complete within its frozen uniform-flow scope; HOLD before047.**
No changing thermal background, arbitrary hot/cold/reverse-entrainment front,
finite flowing junction, component loss/radiation, production engine integration,
real-engine agreement or runtime qualification is inferred. Preserve039 and
S1/S2/S4/listening gates. Whole-engine new-system comparisons remain057,
architecture choice060, public promotion061 and listening063.

Final checkpoint review: the offline page plays all55 clips, preserves exclusive
playback and all four top-range seeks, and loads all ten research plots plus
three recording-perspective images with zero script/network errors. All67 prior
embedded media payloads are exact; the new figure is the sole additional payload.
All74 protected production/contract/Claude files and105 accepted current files
remain byte-identical. The expected exception is `current/results.json`'s
publication timestamp, plus the page and section manifest themselves. The first
preservation assertion retained in evidence treated that timestamp manifest as
immutable; the final audit explicitly checks this known publication exception.
No raw audio, source reference, engine configuration or production WASM changed.

Graphify was queried before046 exploration and refreshed after final source/tests:
**3126nodes/8759edges/177communities**. It retains the known rb.h:415 parser
limitation, inferred edges and stale semantic community labels; an AST map is
navigation evidence, not physical verification or fresh semantic extraction of
all Markdown. The113-stage dependency DAG and50 local execution/card links pass.
The final review finds no unresolved failure inside the declared046 envelope;
all realism, thermal/front, component and cost limitations above remain open.
