# Stage 049 — finite junctions and all-port transfer

2026-10-26. Authorized after reviewed114, baseline ef08fe3. Hold after049.
Reference039, production promotion, listening and runtime gates remain open.

## Predecessor review and frozen design

114 proves current conservative fixed-port exchange, supported contact
transport and reflected load. It does not prove a finite junction, arbitrary
areas or efficient delivery. Its four primary packet failures, fine-grid
reference cost and200–990K EOS limit remain active. Its independent fractions,
star mass-flux precision, physical impedances, stage/final thermal guards and
failed-state lifecycle must survive this stage. No accepted sound is changed.

A compact uniform-pressure junction cannot resolve travel through a crossover
or momentum direction. Prototype a **fixed finite-volume vector Euler junction**
using114's actual one-dimensional EOS/characteristic face kernel in each face
normal. Each cell owns two constituent mass densities, three momentum
components, one total physical energy and independent volume fractions. Normal
Riemann states remove transverse kinetic energy for the scalar solve; upwind
transverse momentum and kinetic-energy flux are then restored. One common
physical face flux updates both cells with opposite signs. Rigid walls exchange
impulse but no mass/energy; external port exchanges are booked explicitly.
This is a bounded conservative correctness prototype, not runtime selection.

[Torregrosa et al.2017](https://www.mdpi.com/2076-3417/7/5/480)
motivates retaining finite geometry and momentum direction rather than treating
all branches as an ideal constant-pressure node. Direct publisher access was
rate-limited during preflight; its primary indexed abstract/text and limits are
identified as such, not claimed as our experimental validation. The implemented
Euler flux extension and all numerical evidence require independent checks.

Use physical extruded polygon fixtures: finite three-branch Y plenum with
angled stubs, four-branch X, two finite T regions/connecting neck in an H,
an unequal-area step/cavity, and a uniform duct control. One SI mesh definition
owns face areas/normals, volumes, cell/port identities and source-outlet binding.
No coincident ambient shortcut, invented gas volume, mirrored outlet or zero
length added for convenience is allowed. Shapes are explicit research fixtures,
not surveyed interiors of the five catalog exhausts. Preserve their existing
canonical geometry/source-outlet regression coverage.

Second-order limited primitive reconstruction and SSP-RK2 use only current
stage states. Preparation checks closed oriented surfaces, positive/consistent
volumes, unit normals, conforming connections, bounded storage and all port
bindings. No tick allocation, energy reset, positivity repair or temperature
clipping. Unsupported clocks/states/topologies fail and poison until prepare.

## Independent references and gates, before candidate measurements

- Freeze ideal-mixture gas200–990K, p0=101325Pa, weak pressure<=3% guard and
  bulk |M|<=.05 for junction qualification. The guard is not a proof at every
  amplitude. Start1Pa pulse transfers; finite5/100Pa controls retain actual
  nonlinear conservation and refinement evidence.
- No-flow complex all-port response is checked against an independent
  frequency-domain P1 finite-element Helmholtz solve, not a linearization of
  candidate time steps. Use independently refined meshes and quantify reference
  uncertainty. Uniform-duct phase/delay and compact pressure/volume-flow limits
  also have analytic controls. No measured gain/arrival fitting.
- For exp(i*w*t), assemble K-w²*M+i*w/c*B on matched open ports, with
  RHS2*i*w/c*incident pressure integrated over its source. Rigid walls have
  zero normal velocity. This follows the independently documented
  [Helmholtz weak/impedance formulation](https://jsdokken.com/dolfinx-tutorial/chapter2/helmholtz.html);
  no FEniCS runtime or copied implementation is required. The FE method and
  time-domain Godunov method share geometry and the linear acoustic limit, but
  do not share numerical fluxes, reconstruction, time stepping or fitted
  trajectories.
- Test every source against every return/outlet of each connected fixture,
  unequal areas, both300/900K and geometry/path changes. Primary junction
  subdivision16; report8 and predeclare24/32 finer controls where necessary.
  Independently refine time. Physical plane-port band200–2000Hz at300K and
  200–4000Hz at900K for widths<=.06m; report transverse/termination limits.
  Require full matrix relative error<=5%, strong entries(|S|>=.05) within
  1dB/10deg, weaker entries absolute complex error<=.01. Independent FE
  refinement changes must be<=2% before serving as an accuracy reference.
- Finite geometry versus compact controls must show the latter's actual error
  and validity range; a compact no-flow X is not a hot finite-crossover proof.
  Actual signed hot-flow/thermal-front controls must conserve each species,
  momentum and total energy with explicit wall/port contributions, exhibit
  upstream reflected-load response, and converge under spatial/time refinement.
  Separate exact uniform-translation references from uncalibrated branch losses.
- Whole and regional physical books<=1e-10 using separate mass/momentum/energy
  scales, supported source-free stability and no hidden limiting. Stated
  viscosity/turbulent losses, arbitrary 3D installations, higher port modes,
  engine maps and actual exhaust identity are not certified by inviscid fixtures.
- Native/Python/web, focused native/WASM/sanitizer, meaningful planted physical
  faults, actual no-I/O cost/storage and complete offline checkpoint/preservation
  checks. Report rejected controls and model limits; no automatic production
  renderer or new accepted engine sound. Document final review before holding.

Status at preflight: in progress. Results and final scope decision follow.

## 28 October continuation: rejected open-port extrapolation

The first no-flow matrix passes for all five shapes at both temperatures, but
the signed hot-front suite rejects the first implementation. At X-cell184,
117microseconds into the positive1000Pa control, the outgoing cold constituent
becomes negative(-7.94e-86kg/m³). This is a real domain failure despite its tiny
size; the failed traces remain in `renders/evidence/stage-049/flow/`.
An open-port gradient bound incorrectly allowed negative extrapolated material
fractions. Falling back only at that face then left an excessive opposite-face
outflow. The fix bounds the **reconstruction gradient** by the physical fraction,
temperature and pressure domain, including the open face. Cell inventories,
energy and pressure are never clipped or overwritten. A focused two-cell trace
constituent/open-port regression accompanies it. Re-run the complete matrix and
hot-flow qualification against the corrected binary; earlier passes alone do
not close the stage. Per-face first-order fallback counts remain explicit.

Before rerunning signed branch controls, freeze the quantitative refinement
gate: complete boundary-pressure traces and actual transported-material changes
must differ by<=5% between subdivisions8/16. Failure requires retained24/32
follow-ups or rejection, with unchanged physical book/domain gates. Independently
refine the clock on a representative hot branch and use exact straight-duct
thermal translation to distinguish transport errors from branch dynamics.

A separate16-grid failure then reveals floating underflow: one constituent's
mass rounds to the smallest positive double while its volume fraction rounds
to zero. For a thermodynamic **query** only, when both are below32*DBL_MIN,
049 uses the exact absent-constituent limit. The actual mass, fraction and
energy bits remain in storage, fluxes and all physical inventories. Finite
inconsistent constituents and negative underflows still reject. A regression
checks the limiting pressure, unchanged input inventory and finite/negative
rejections.114's finite-state EOS implementation is unchanged. This is a
floating representation limit, not a finite-temperature or mixing closure.
Retained failed and superseded runs do not qualify the corrected candidate.

Further24-grid signed-Y testing finds that this limiting query must also
preserve the actual donor at a face. Recreating density from a subnormal volume
fraction manufactured an outflow after its mass rounded to zero(-DBL_TRUE_MIN
at4940ticks). Candidatev2 uses first-order **stored trace mass/fraction only**
below the same32*DBL_MIN limit, retaining second-order pressure, velocity and
the major constituent. It counts these face predictions separately; no stored
inventory, pressure or energy is reset. A zero-total-minor-mass regression and
planted recreated-mass fault test this exact failure. All current-source gates
are rerun against a frozenv2 executable. Earlier passing physics and failed
traces stay available;v1 is not the qualification candidate.

Fine-Y initial geometry also exposed cancellation in the clipped half-plane
volume formula(1+epsilon fraction rejected by the parser). Compute both
nonnegative geometric half-volumes in translated coordinates and partition by
their sum.8/16/24/32-grid inventory/bounds tests pass; do not clamp an invalid
simulated fraction. A moving oversized-clock test initially missed a removed
CFL check because a later Mach guard still failed. An exactly quiet fixture
now isolates and catches that missing guard.

v1's Y-reversal32-grid net-material difference remains5.533%, above the5%gate.
Beforev2 fine measurements, extend follow-ups to40/48 at8MHz where the existing
8192-cell/65536-face capacity supports them. Retain24/32 misses and require the
same decreasing adjacent-grid difference, physical books and5% criteria;
unsupported larger meshes reject. This extends accuracy work, not the gate.
Also retain an independent16-grid area-reversal4MHz clock control: clock-only
pressure and net-material differences must be<=1%, alongside the positive-Y
clock control. These grids are costly correctness references, not a production
runtime proposal. Analytical compact comparisons retain all real stub lengths,
finite plenum volumes and the H connector; fully collapsed no-delay diagnostics
are separately labelled. No measured delay/gain is fitted.

The corrected model's16-grid area-reversal clock control changes net transported
material by1.07056% at2→4MHz, narrowly above its1%gate (pressure changes<=.0416%).
Retain that rejected primary and, before measuring it, add4→8MHz at the same
geometry, pulse, duration and amplitude. Qualification requires decreasing
material error<=1% and the original physical/pressure gates; do not relabel the
4MHz primary as a pass. This separates time sensitivity from the much larger
coarse spatial discrepancy. A passing finer clock is a representative convergence
control, not certification of every timestep, flow or installation.

For H, uniform40 would exceed the existing8192-cell capacity. Before measuring
the32-grid follow-up, predeclare a conservative local alternative **only if32
still misses**: retain32 subdivisions across the geometry and increase axial
resolution40/48 in the initial thermal-front bridge and the upper-left return
stub. Those are the two known thermal-front paths: the bridge initially crosses
the cold/hot partition, and that return's900K reservoir can enter its initially
300K branch. All other port identity, gas volume, wall and external geometry is
unchanged; terminal cross-section subdivisions still match the T boxes. These
meshes have7680/8192cells, stay at4MHz and use the same decreasing5%material,
pressure and physical-book gates. This is explicit local convergence evidence,
not a claim of uniform48-grid or full-installation convergence. Native
preparation/max-capacity and geometric/initial-inventory checks are required.

## Near-unit reconstruction review — candidatev3

The4→8MHz reversal comparison still changes net material by1.03529%, so the
clock gate remains unresolved. A direct two-cell regression then exposes an
independent implementation error: the EOS accepts an alpha average of
1+roundoff, but the open-face upper bound was exactly1. Its shared fraction
limiter could become negative or flatten the other constituent's valid slope,
and trigger a whole-face fallback. The regression fails both slope direction
and fallback-count checks onv2 (`near-unit-before.log`). It is not enough to
loosen the clock gate or use acoustic conservation alone.

[NASA's Wind-US limiter reference](https://www.grc.nasa.gov/WWW/winddocs/user/keywords/tvd.html)
describes scalar reconstruction limiting on unstructured meshes and the
different neighborhood choices. The numerical correction here keeps every
accepted cell average inside its admissible reconstruction interval and keeps
the gradient multiplier in[0,1]. The fraction bound now uses the same1+1e-12
roundoff envelope as the existing EOS. A16*DBL_EPSILON inward gradient margin
protects the short floating-point face dot products; this is a roundoff guard,
not a physical mixing/temperature/pressure parameter. It changes no stored
mass, fraction or energy. Remaining invalid reconstructions still report a
fallback and invalid accepted states still poison. This does not assert a
general positivity theorem for arbitrary meshes or real exhaust conditions.

Candidatev3 must first pass that regression and the hard fixed-grid reversal
clock comparison, then qualify the full current-source matrix and physical
controls. A new excluded-average fault must fail. Keepv2's complete passing
no-flow/physical-book evidence and rejected clock/refinement records;v2 is not
the final candidate. The current accepted engine audio and Claude setup remain
unchanged. Stage049 is still open until this review's gates pass.

For a rising face variation, theta=(upper-average)/variation must be
nonnegative. An upper of1 with an accepted average of1+epsilon violates that
condition. Including the accepted roundoff envelope restores a convex gradient
scaling. In the hard16-grid reversal pilot, v3's2→4MHz material difference is
0.00178283% and pressure-trace differences<=0.00278%; whole-face reconstruction
fallbacks fall to zero in both runs. Physical books still pass. The unchanged1%
clock gate now passes because of the numerical correction. The fullv3 replay,
including any required spatial follow-ups, remains necessary before closure.

## Frozen v3 results and validation

The final candidate is `qualified-v3/junction_probe`, SHA-256
`7954fad5d19de089e7d94ff721d490f8bfb8ba83f9691a5193fc4a87a4b81d87`.
The corresponding WASI executable is
`9f1c1ca7531ab84defabc0a8ac3d358d58ecb1e436dc43c665267db3a0c6ad28`.
The ignored evidence root is `renders/evidence/stage-049/`; its
`qualified-v3/sources.json` records physical source/header/probe/test hashes.
The final summary verifies those physical source files and the executable
identity of every selected and retained native trajectory. Earlier directories
and v2 measurements remain historical evidence, not substitutes for v3.

All ten no-flow matrices pass at subdivision 16 / 2 MHz. Thirty distinct source
runs cover 441 complex source/receiver/frequency responses. The table reports
full matrix differences from the independently refined Helmholtz reference:

| Geometry | 300 K | 900 K |
| --- | ---: | ---: |
| Straight control | 0.296% | 0.410% |
| Area step/cavity | 1.930% | 2.204% |
| Y | 0.934% | 1.182% |
| X | 2.420% | 2.490% |
| H | 2.362% | 2.877% |

Strong entries remain within 0.636 dB / 5.846 degrees; weak entries differ by
at most 8.903e-5 in complex amplitude. These are plane-port responses of the
specified synthetic fixtures, not measured hardware transfer functions.
The 300 K band stops at 2 kHz; the 900 K band reaches 4 kHz. Every independent
24-to-32 FE refinement passes its 2% uncertainty gate before use as truth.

Independent checks include Y acoustic clock refinement (0.01042%), H capture
length refinement (0.03280%), and doubled X stub lengths. The latter changes
the actual returned complex wave by as much as 0.83388; the edited candidate
also agrees with its independently solved edited geometry. The edited FE
refinement changes by 0.15189%. No observed gain or travel time is fitted.
Y and H spatial acoustic errors decrease from subdivision 8 to 16. Finite
5/100 Pa Y/H controls pass the same stated acoustic/book gates.

Four-residence exact straight thermal translations at +/-15 m/s preserve the
final material inventory within 3.385e-14 relative. Hot source-free open controls
at 0/+15 m/s show no sampled perturbation-energy growth over one second; the
closed native fixture checks every step for one second. This is stability
evidence in the stated domain, not proof for arbitrary flow, temperature or mesh.

The fair compact comparison retains every physical stub length, plenum volume
and H bridge. Its finite-volume form passes only the tested frequency points
through 100/200 Hz for the area fixture (300/900 K), 100/100 Hz for Y,
100/200 Hz for X, and 50/100 Hz for H. These are discrete tested validity points,
not certified continuous bandwidths. Fully collapsed zero-delay diagnostics
are separately labelled and do not decide the architecture.

Validation of the frozen v3 physical code:

- Full native: 248 tests / 4,951,907 checks.
- Focused native, WASI and ASan/UBSan: 11 tests / 96,653 checks each, including
  the four predecessor114 regressions. Leak detection was disabled; no leak
  qualification is claimed. The maximum-capacity local H mesh also passes
  native preparation and sanitizer checks.
- All 16 targeted planted faults are caught. Earlier missed-fault records are
  retained; the strengthened quiet-clock and zero-total-trace-mass assertions
  catch the specific omissions that the earlier controls missed.
- All 95 Python tests pass without skips, including seven independent mesh,
  local-refinement, compact and reference checks. The 17 web unit tests and
  typecheck/production build pass; application/core sources are unchanged.
- Eight raw native/WASI trajectories agree within the predeclared field
  tolerances. The largest pressure difference is 1.456e-11 Pa, material
  inventory is exact, and physical energy differs by at most 1.777e-15 J.
  WASI elapsed time is not reported as native process-CPU time.

The no-I/O native cost study uses three CPU-time repeats, excludes preparation,
allocation and diagnostic I/O, and times one uniform-gas fixture at 900 K.
Observed host: i7-1355U / WSL2, GCC 15.2 with O3/LTO and contraction disabled;
the native solver uses one thread. `environment-final.json` records the host
and isolated analysis versions. Frequency/thermal/host variation limits timing
comparisons, including comparisons to earlier candidates. This is not a
whole-engine or worklet benchmark or a deployment hardware certification:

| Fixture | Cells | Storage | Simulated time / CPU time, U=0 / 1 m/s |
| --- | ---: | ---: | ---: |
| Straight, subdivision 8 | 192 | 326,200 bytes | 0.001394 / 0.001411 |
| Y, subdivision 16 | 832 | 1,345,144 bytes | 0.0001644 / 0.0001567 |
| X, subdivision 16 | 1,280 | 2,107,960 bytes | 0.00009693 / 0.0001135 |
| H, subdivision 16 | 1,792 | 2,946,872 bytes | 0.00008612 / 0.00008099 |

This model is an offline correctness reference, far too slow for real-time
delivery. No renderer is promoted. Efficiency/scheduling and full-engine
acceptance remain at 052/057/060/061. The local 8192-cell H mesh uses 13,377,592
bytes, within caller-owned capacity; no tick allocation is introduced. Storage
counts cover the prepared state, geometry and cell/face buffers, not process RSS,
probe-only initialization arrays or the separate Python/FE analysis process.

Reproduction entry points (run from the repository; analysis dependencies in
the isolated `.venv-analysis`, single BLAS thread):

```sh
make BUILD=build/stage049-v3 LTO=1 build/stage049-v3/rb_tests build/stage049-v3/junction_probe
build/stage049-v3/rb_tests
python tools/qualify_junction.py --probe renders/evidence/stage-049/qualified-v3/junction_probe --out renders/evidence/stage-049/matrix-v3 --resume
python tools/check_junction_flow.py --probe renders/evidence/stage-049/qualified-v3/junction_probe --out renders/evidence/stage-049/flow-v3 --refine --resume
python tools/junction_controls.py --help
python tools/junction_compact.py --help
python tools/junction_parity.py --help
python tools/junction_costs.py --help
python tools/junction_summary.py --evidence renders/evidence/stage-049 --candidate v3
```

The evidence worklog records exact native/WASI/sanitizer/fault/Python/web
commands, environment and retained failures. Resume accepts a capture only when
both its exact native executable hash and full native-input hash agree.
Reports are assembled from raw CSV/JSON, not from filtered listening audio.

### Definitions needed by the next stage

- A **constituent** is one of the two fixed contact materials, each with its
  own named production-species mixture and temperature. It is not an arbitrary
  new chemical species or a general reaction/mixing model. The inherited 114
  Allaire contact closure uses the frozen-fraction modulus
  rho*c²=p*(1+1/D), D=sum(alpha_k*cv_k/R_k). This is a sharp-interface numerical
  reference with a diffuse numerical transition, not a Wood-law or measured
  miscible/turbulent-mixture closure. Preserve that distinction at integration.
- A **physical book** compares extensive mass, vector momentum and one total
  energy with actual accepted port/inter-region transfers and rigid-wall
  impulse. Volume fraction is a separate material coordinate; it is not an
  extra conserved physical energy or an audio reservoir.
- **Net transported material** is the vector of final-minus-initial constituent
  masses. Its refinement denominator is that change, not the much larger full
  initial gas inventory. Passing the 5% difference gate establishes tested
  resolution agreement, not an exact nonlinear branch-loss solution.
- Pressure/velocity/driver CSV columns represent the RK face average at the
  step midpoint. Inventory and perturbation columns represent the accepted
  end of that step. Inventory plots therefore add half a timestep; final JSON
  inventories use the actual final time. No measured time alignment is fitted.
- **Reflected load** here is the current physical pressure returned at a fixture
  port. The engine's resulting breathing/torque change still needs the actual
  whole-engine adapter and qualification at 057; no production coupling or
  volumetric-efficiency map is established by this fixture.

## Final review and bounded closeout — 28 October 2026

Reviewed 114 before this implementation, then re-reviewed its shared face/EOS
handoff after the new metadata/cached-primitive API. The original HLLC body is
unchanged; predecessor tests and cached-state consistency faults pass. The
fixed-domain 114, material-reference 048 and high-temperature/packet/cost limits
remain explicit rather than being hidden by a new stage number.

All twelve 6 ms signed hot-front cases pass the selected resolution gates.
The actual source-port velocity must have the commanded direction, and each
reversal must contain both signs of flow; driver sign changes alone do not pass.
Pressure histories use the full captured interval without gain/delay fitting;
net material uses the actual final-minus-initial mass vector. The table gives
adjacent-grid differences, not error against an exact nonlinear solution.
Every selected comparison meets both the 5% pressure and 5% material gates:

| Geometry / control | Primary8→16 material difference | Selected subdivision | Selected adjacent-grid material difference |
| --- | ---: | ---: | ---: |
| AREA / flow / -1000 Pa | 0.3453% | 16 | 0.3453% |
| AREA / flow / +1000 Pa | 0.0179% | 16 | 0.0179% |
| AREA / reversal / +1000 Pa | 14.6849% | 24 | 3.7502% |
| H / flow / -1000 Pa | 0.2658% | 16 | 0.2658% |
| H / flow / +1000 Pa | 1.2443% | 16 | 1.2443% |
| H / reversal / +1000 Pa | 23.1421% | 32 + axial 40 | 2.3139% |
| X / flow / -1000 Pa | 10.5559% | 24 | 2.6078% |
| X / flow / +1000 Pa | 1.0551% | 16 | 1.0551% |
| X / reversal / +1000 Pa | 16.9013% | 24 | 1.1915% |
| Y / flow / -1000 Pa | 7.8979% | 24 | 3.8255% |
| Y / flow / +1000 Pa | 0.0781% | 16 | 0.0781% |
| Y / reversal / +1000 Pa | 20.2900% | 32 | 3.9424% |

The H reversal retains its failed uniform 24/32 comparisons (11.7603% and
7.07604% material differences) and passes the
predeclared local transport-path refinement. This refines the thermal bridge
and hot-return axial path while preserving external geometry, cross-sections,
ports, physical volume and initial material inventories. It is not uniform 40/48
or arbitrary-installation convergence. Maximum-capacity native/sanitizer and
independent mesh identity tests pass.

Across current-source native qualification trajectories, normalized physical
book residuals are at most 9.76141e-12; peak Mach is
0.0431973 and peak CFL 0.42066. Whole-face
reconstruction fallbacks have maximum 0.
The separately reported absent-material donor prediction counter reaches
341,656,296 on the most demanding retained run;
these are face predictions at the documented floating-representation limit,
not repairs of stored mass/energy. Keep their counters and regression when
optimizing. This stage does not prove a general positivity theorem.

The final source-bound summary passes. Publication adds one guarded numerical
figure to the single offline RESULTS page. All 55 audio players advance, exclusive
playback and four jump buttons work, all 16 images load, and there are no page
errors or network requests. All 70 previous embedded payloads, 105 current media/
diagnostic files and 76 protected files including two reference locks are exact;
only the generated results timestamp manifest changes among existing current
files. Accepted clips/long coasts, raw references, production core/web/contracts,
AGENTS and Claude's active setup are preserved. Browser/preservation JSON records
bind the exact final page hash. No new engine listening acceptance is claimed.

Graphify code AST refreshed to 3434 nodes / 9875 edges / 176 communities. The known
rb.h415 parser failure, inferred edges and stale semantic/community labels
remain; doc/paper semantics were not regenerated. The 114-stage dependency DAG
and current planning/card/handoff file links pass. Generated audio/plots/raw
captures remain ignored; code, reproducible tools and notes are committed.

**Decision: Stage 049 complete within this bounded finite-junction research
scope and reviewed; HOLD before 050.** No real-time renderer is selected.
Measured junction loss/viscosity/turbulence, arbitrary 3D installations/higher
port modes, arbitrary reacting mixtures, hotter EOS, changing prepared geometry,
actual engine coupling/breathing, common audio routing and deployed worklet
performance still need their later gates. The caller must keep prepared geometry
immutable and rebuild before changing it; the stencil must have sufficient rank
(a one-cell double-open duct rejects). Physical tests are not calibrated hardware
or sound-realism acceptance. Stage 039, S1 and the overall listening gates remain open.
