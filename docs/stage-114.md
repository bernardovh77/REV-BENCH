# Stage 114 — fixed-geometry thermal transport bridge

2026-10-26. Authorized after review of048, baseline40dc6bd. Bernardo requests
114 then049, reviewing each predecessor before closing its successor. Hold
after049. Reference acquisition039 remains open and is not waived here.

## Predecessor review and design, before candidate measurements

048's compatible material force/work reference remains useful, including its
fine-grid geometric-work regression and reported4kHz packet error. Its moving
outer boundaries are not engine ports; fixed observation cuts prove neither
Eulerian energy exchange nor bounded transport memory. No production renderer
or>990K gas closure was qualified. Recheck its focused native tests before
using it as an independent continuum/convergence reference.

Prototype a separate **fixed finite-volume, pressure-equilibrium contact
transport** arm. This does not alter048, the rejected043 arm, production core
or the accepted sound selection. [Allaire, Clerc and Kokh2002](https://www.cmap.polytechnique.fr/~allaire/ack.pdf)
motivates the extra advected volume fraction and isobaric rather than
instantaneous isothermal averaging. This is numerical treatment of a sharp
thermal/species contact, not a claim that exhaust is immiscible or that a
diffuse interface has measured turbulent mixing physics.

Two immutable material compositions have production ideal-mixture u(T), R and
cv(T), restricted to200–990K. Each stationary cell owns partial mass densities
r0/r1, axial momentum density, ONE physical total-energy density and volume
fractions a0/a1 whose sum is checked against one. Both are represented
independently so subtraction from one cannot erase a trace constituent. A common pressure solves
I=sum(ri*u_i(p*ai/(ri*Ri))) with I=E-momentum²/(2*rho).
No energy or pressure overwrite, temperature clipping or acoustic reservoir is
allowed. Empty constituents are exact zeros; invalid states poison the object.
For this Allaire closure the frozen-a acoustic modulus is
rho*c²=p*(1+1/D), D=sum(ai*cv_i/Ri); it is not a Wood/miscible-mixture claim.
Pure-cell acoustics reduce to the production gas law. Interface-zone error must
converge against sharp two-medium references.

Use conservative weak-wave HLLC mass/momentum/total-enthalpy fluxes with
one-sided physical characteristic impedances, MC primitive
reconstruction and SSP-RK2. Volume fraction obeys material advection, with the
nonconservative divergence term evaluated from the same face velocity. One
accepted RK-averaged face flux supplies both neighboring inventories and the
stationary-port ledger. Fixed cells never appear, disappear, move or remap;
there is no parcel queue. Ports expose current pressure/velocity and signed
partial-mass/momentum/total-energy flux. A two-region step must solve the
shared face once at each RK stage and give both sides exactly that transfer.
An independent full-duct versus split-duct proof checks reflected feedback,
not delayed sampled pressure. No actual engine integration is claimed.

One fixed wall owns its capacity/temperature; gas/wall/exterior energy is
explicit. No tick allocation. Caller storage and preparation bounds, failed
reuse and unsupported clocks/configurations must reject. Primitive-slope
fallbacks must be counted, not hidden as physical fixes.

## Frozen gates and planned controls

- p0=101325Pa, uniform A=.003m², weak pressure<=3%, bulk |M|<=.25,
  gas200–990K, CFL<=.45. No shock, chemistry, transonic, arbitrary EOS or
  calibrated heat-transfer claim. Two named materials bound the contact class.
- Constant-p hot/cold and composition fronts at U=-70/0/+70; false pressure
  <=1Pa and velocity<=.001m/s. Actual inlet/outlet species and enthalpy transport
  must match analytic translation within1%; include a front exiting the duct,
  sustained multiple residence times, changed inlet composition and reversal.
- Whole and regional mass/species/momentum/total-energy closure<=1e-10 with
  separate physical scales. Shared stationary-interface transfers must agree
  exactly, and both interface positions must match an unsplit trajectory.
- Independent Gaussian and200/1000/2000/4000Hz finite-packet references for
  cold/hot and hot/cold contacts, signed flow and two front positions. Require
  converging whole-packet L2<=5% and carrier<=1dB/10° on a declared grid.
  Primary2048cells/2MHz; report512/1024 and independent clock refinements;
 4096cells/4MHz is a predeclared finer correctness reference if needed. Retain
  all coarse/primary failures; no assertion of real-time efficiency.
- Current reflected pressure/return timing versus an independent characteristic
  reference and full versus split duct; positive/negative reservoir flow and
  actual pressure-driven ambient return must carry the upwind material.
- Fixed wall capacity and heat work under refinement and clock halving;
  source-free growth checks, including hot/+70m/s/4kHz, and long throughflow.
  No unbooked coefficient changes or normalization. Numerical dissipation is
  measured; conservation is not accepted as acoustic fidelity.
- Native/Python/web suites, focused native/WASM/sanitizers and behavioral
  faults for interface sign/energy/species, closure, current-stage coupling,
  unsupported domain and failed reuse. Preserve engine sources, accepted audio,
  reference locks and Claude setup; publish research evidence in the one results
  page. Record no-I/O CPU cost/storage separately from instrumented captures.

Status at design: in progress. Measurements, decisions, rejected cases and
final review follow below. The prototype is not a production promotion or
completion of039, listening acceptance or runtime qualification.

## Executed method and results

**The fixed-port prototype passes the declared bounded research gates.**
`fixed_transport.[ch]` owns the same stationary cells for an arbitrarily long
run; it has no parcel insertion/removal or coordinate remapping. The public
current-face solve and paired RK step return reflected pressure and use one
accepted total-enthalpy flux on both sides. Full versus split ducts, with
interfaces at0.25/0.625m and a downstream thermal load, agree within1e-13
momentum density/1e-9 energy density; their shared transfer ledgers are identical.
This is a two-region native interface proof, not production engine integration.

The main matrix, raw CSVs, commands and executable hashes are in ignored
`renders/evidence/stage-114/matrix/`. All60 primary profiles are retained.
Four primary4kHz cases miss the complete-packet gate and are replaced in the
qualified set by the predeclared4096-cell/4MHz correctness controls. No coarse
or primary failure is suppressed or reclassified as a pass.

| Control | Recorded result |
| --- | --- |
|60 primary profiles plus4 finer replacements | Accepted set maximum packet L2=4.7088%, carrier0.34173dB/0.82362°. Primary maximum7.15644%; fine maximum2.41419%. |
| Independent clock/spatial check, cold→hot,+70m/s,4kHz |2048cells/2MHz7.15644%;2048/4MHz6.76087%;4096/4MHz2.41419%. A faster clock alone does not explain the mesh improvement. |
|6 main contact transports;6 additional temperature-only/species-only controls | Main maximum false pressure4.366e-11Pa and velocity1.706e-13m/s. Additional controls include a subcell front and both signs of flow. |
|4-residence throughflow,64/128cells and±70m/s | A front exits the finite duct; donor gas fills it without growing memory. Final mass error<=1.696e-14, inlet mass error<=1.593e-13 and inlet total-enthalpy error<=1.013e-13 relative. Enthalpy reference comes from047's audited property export. |
| Whole and stationary regional accounts | Maximum normalized mass/momentum/total-energy residual4.109e-11 over the main matrix, including one-second cases. Each quantity uses its own physical scale. |
| Pressure-driven outlet reversal | Both signed mass flows,[-1.10849e-4,+3.98189e-5]kg/s, and actual ambient constituent inside up to6.92464e-7kg. Negative velocity alone is not the criterion. |
| Fixed wall,64/128cells,256/512kHz |400J/K wall remains fixed. Final Tw649.979624358–649.979624434K; gas heat6.750267–6.750298J. Pure-exterior analytic cooling and exactly zero inner exchange also pass at two resolutions. These are scenarios, not measured material calibration. |
| Source-free hot4kHz fields, U=0/+70,1s | Sampled maximum/initial perturbation energy=1; final ratios<7.4e-19. Physical boundary transfers remain in the separate total-energy accounts. |

An additional finite-amplitude check compares this Eulerian candidate with the
independent048 material solver before either pulse reaches an outer boundary.
5/500Pa Gaussian pulses pass the same5% shape gate. At500Pa,1024→2048 fixed
cells reduce mismatch against the3072-cell material reference from1.35351%
to0.46679%. The material reference retains its own discretization uncertainty;
it is not treated as an exact experimental recording. No gain, arrival or
phase is fitted to the measured waveform. The3% pressure rejection limit is
an engineering domain bound, not proof of accuracy at every allowed amplitude.

## Review findings and failure detection

The initial implementation failed meaningful checks, and the evidence is kept:

- Deriving the second fraction as1-a loses a trace constituent near one.
  Both fractions are now represented independently and their sum is validated;
  conserved material masses are neither clipped nor normalized.
- The subtractive HLLC mass-flux form can round mass flow to zero near rest
  while its material fraction moves. The algebraically equivalent star
  advective form avoids that cancellation without a numerical flow threshold.
- Using the largest sound speed on both sides of a hot/cold interface gives
  the wrong acoustic reflection. One-sided physical impedances pass the
  independent reflected-load check; strong shocks remain unsupported.
- A final wall half-step could leave the pressure envelope and return success
  until the next tick. The new adverse regression fails the old code. Every RK
  stage and the final thermal state are now checked before the accepted transfer
  ledger/step count commits; a failed pair is poisoned together.
- Exactly zero conductance must be an identity. Skipping that exchange avoids
  otherwise unnecessary thermodynamic round trips and tiny spurious heat.
  Exterior cooling still acts once per physical wall, not once per cell.

All13 compiled behavioral faults are caught: wrong impedance, cancelled
contact mass, doubled pressure enthalpy, stale shared state, unequal shared
energy, dropped material inventory, stale RK ledger, missing gas/wall energy,
failed-state reuse, unloaded stepping, pressure overwrite, unchecked final heat
and exterior loss repeated per cell. Initial and successive matrices, failing
tests and the final frozen-binary captures remain available. Tolerances are
unchanged; the finer packet grids were declared before measurement.

## Validation, cost and integration limits

Final full native **241tests/4,871,163checks**, focused native/WASM/ASan/UBSan
**4tests/15,909checks each**, **88Python tests without skips**, and **17web
tests plus production build** pass. Eight final native/WASM trajectories have
zero measured per-column difference. Leak detection is disabled under the
debugger-restricted environment; no leak qualification is claimed.

Native GCC-O3/no-LTO/strict double, same i7-1355U environment, six interleaved
no-I/O256-cell/512kHz10ms cost controls give median simulated/CPU factors
**0.0079773× at U=0 and0.0079863× at+70m/s**, storage121936bytes. Primary2048
cells require967760bytes;4096 require1934416bytes. These are isolated
correctness-reference costs and are far below real time. They are not a
full-engine comparison or an efficient renderer solution. Mixed-rate scheduling,
cost reduction and architecture choice remain052/060/062.

Frozen measured native probe SHA256:
`1e37548f2d9cfe7dd55ac6d8e110634a027d9fc585cdfa7371207632f4b891cc`.
Its executable is retained separately from evolving build output. A subsequent
report-only change explicitly labels native process CPU time versus WASI
monotonic elapsed time; its CSV is identical to the measured binary and across
native/WASM. The legacy `cpu_s` field must be interpreted with `timing_clock`;
WASI captures here establish parity, not CPU performance. Source fingerprints,
the metadata-only replay and all actual commands are in the evidence folder.

The qualified scope is a uniform-area duct, two named thermal/species contact
constituents, weak waves and the stated temperature/flow range. The diffuse
interface closure is not measured miscible mixing, an arbitrary thermal history,
a shock solver, a general variable-area junction or a complete engine adapter.
No hot>990K, real wall correlation, radiation, actual valve/engine coupling,
production efficiency or listening acceptance is inferred. Those remain later
common gates, including039 and057/059/060/061. Old A2 remains rejected; B0 and
the accepted production engine/audio are unchanged.

Reproduce the native build with
`make BUILD=/tmp/rev-bench-114 LTO=0 /tmp/rev-bench-114/rb_tests /tmp/rev-bench-114/fixed_transport_probe`;
run the full test binary and the `fixed_transport` filter. Then run the analysis
Python with `PYTHONPATH=tools tools/check_fixed_transport.py --probe /tmp/rev-bench-114/fixed_transport_probe --out renders/evidence/stage-114/replay --properties renders/evidence/stage-047/matrix-final/properties.csv`.
The property CSV is reproduced by047's `thermal_probe properties` command.
The new probe accepts the same nine numeric fixture arguments as documented
in its source; all measured commands are retained per case. The WASI targets
and meaningful fault table are part of Makefile dependency tracking.

## Closeout and handoff

The single RESULTS page gains one numerical section/figure and rebuilds from
the registered checkpoint in one operation. All55 players play offline,
exclusive playback and all four seek controls work, and all15 images load
without page errors or network requests. All69 prior embedded payloads,
105 accepted current files and74 protected production/contract/Claude files
plus both session locks remain exact. Only the results timestamp manifest and
page/section registry changes are intentional; no engine sound is regenerated.

**114 is complete within this fixed-port research scope and reviewed before049.**
049 must extend the actual conservative current interface to unequal areas,
finite storage and all physical ports; it must not reuse a zero-flow compact
X as proof of a real hot crossover. Geometry, source/outlet identity, physical
storage, stability and the expensive-reference limits remain active. Bernardo
has authorized049 next; hold after its own review. Do not start050.

Closeout checks pass for the114-stage dependency DAG and982 local planning/card
file links. Graphify was queried before exploration and refreshed to3296nodes,
9358edges/179communities. Its existing public-header parser warning, inferred
edges and stale semantic/community labels remain limitations; an AST/headings
refresh is not new semantic validation. The first automatic permission review
for that refresh timed out; its permitted retry succeeded.
