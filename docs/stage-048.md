# Stage048 — Thermal fronts, evolving properties and reversal reference

2026-10-25. Authorized after reviewed047 (`1f40610`); hold after048, no049.
Raw evidence stays in ignored `renders/evidence/stage-048/`. Accepted clips,
production core, Claude setup and reference splits remain unchanged.

## Design and frozen gates — before candidate measurements

047 shows the need for finite wall energy and a clear EOS envelope.043's
Eulerian mixed-contact pressure error remains rejected;046 is restricted to a
frozen near-uniform reference. Retiming its B0 buffers alone cannot transport a
thermal front, and changing buffer/c coefficients can lose/create signed and
quadratic inventories. Do not silently broaden that arm's qualified envelope.

Test a material-coordinate, production-EOS **correctness reference**. It is a
new standalone research path, not a production B0 variable-delay update or
selection of a final renderer. [Caramana/Burton/Shashkov/Whalen1998](https://cnls.lanl.gov/~shashkov/papers/comp.pdf)
provides compatible staggered force/work principles; the new midpoint scheme,
limited domain and fixtures require their own proof. It avoids averaging two
materials into an unphysical mixed thermodynamic cell. A moving-cell reference
still needs remapping/bounded-memory/interface scheduling before production.

Cell species masses and sensible internal energy plus nodal momentum/kinetic
energy own one physical state. Node masses are half neighboring material-cell
masses. Current cell pressure supplies nodal force; the same accepted midpoint
pressure supplies -p*dV. Exact midpoint kinetic work closes the physical energy
book, with boundary work, body-force work and exterior wall heat explicit.
Production cp(T,Y) and density/current volume determine evolving c and impedance;
no filter-coefficient rewrite, pressure reset or hidden energy reservoir.

A fixed spatial wall interval owns ONE finite Cw/Tw. Gas/wall exchange uses047's
bounded operator with overlap area; exterior cooling acts once per heat step.
The metal does not advect with parcels. Spatial capacity stays independent of
cell count. Contacts move through fixed observation planes, including a duct/
ambient marker backed by explicit cold-air buffer parcels. Transparent1D
pressure-reservoir boundaries are not certified3D jet/radiation loads; those
remain051/053/056. Do not call a material outer boundary a fixed engine port.

Freeze physical envelope: ideal-mixture production closure200–990K (no apparent
high-cp branch),p0=101325Pa,A=.003m², weak pressure excursions<=3%, perturbation
Mach<=.05 and uniform reference |M|<=.25. Cells stay ordered/positive; material
CFL<=.45. Invalid/unsupported states fail and poison until prepare. No shock,
chemistry, arbitrary finite-junction or production efficiency claim.

Required numerical evidence:

- Uniform fixed thermal/composition jumps and hot/cold contacts at U=-70/0/+70,
  temperature pairs300/900 and900/300K, both air and burned-mixture identity;
  no false pressure>1Pa or velocity>.001m/s. Contact location and independently
  predicted fixed-plane species/enthalpy contrast transfers within1%; no
  cell-mixing repair. Piecewise material fronts fit actual boundaries; preserve
  physical length with explicit nonuniform spacing at arbitrary front positions.
- Independent small-signal Galilean two-medium pulse/scattering reference at
  two material-interface positions (.7/1.1m in2m). Primary768-cell/768kHz
  controls: Gaussian waveform L2<=5%, weak200/1000/2000/4000Hz transfer<=1dB/10°.
  Report coarser192/384-cell and independent clock controls (384/768/1536kHz)
  and reject unsupported CFL; coarse failures are not primary passes.
- Physical species/momentum/total-energy accounts<=1e-10 with separate scales.
  Pressure force and work must use the same current stage. Local c/Z must
  respond to actual gas energy, volume and composition; diagnostics alone must
  not change state. Compare interface positions and supported thermal profiles.
- A pressure-driven outlet/ambient-buffer experiment must show actual two-way
  material transport and sign-correct ambient composition on inward motion.
  Separately controlled bulk reversal has an independent prescribed-displacement
  reference and explicitly booked body-force/pressure work; distinguish these
  controls from a measured engine lift. Fixed-plane energy comparison is an
  analytic translation control, not an invented exact nodal-energy flux.
- Fixed spatial wall exchange with refinement, independent clock halving,
  source-free heating/cooling and gas+wall+exterior energy closure. Changing c
  from physical heat must have its heat source; pure coordinate/observer changes
  cannot inject physical energy. Reject direct unbooked temperature changes.
- Compact hot/cold1s source-free runs, including the prior900K/+70m/s/4kHz
  growth combination with a buffer. With matched moving material boundaries,
  sampled perturbation energy<=1.02initial and final<.001initial. This differs
  from046's fixed-reservoir pressure-reflection gain; use each fixture's actual
  boundary work. No growth hidden by limiting/normalization.
- Full native/Python/web regressions, focused WASM/sanitizer, meaningful force/
  work/species/wall/domain faults, actual cost/storage/iteration counts and
  unchanged accepted checkpoint media. Full-engine audible comparison remains057.

Status: bounded reference experiment complete; fixed-domain continuation114 is
explicitly required. Rejection/failure evidence is retained. Passing a bounded
material reference must not be advertised as a qualified evolving B0 adapter,
production real-time renderer, high-temperature exhaust EOS or listening closeout.

## Executed method and bounded results

The `material.[ch]` reference owns immutable material species masses and gas
internal E per cell, lab velocities/momentum and lumped nodal kinetic energy.
Pressure is derived from actual production EOS energy and current volume.
Matched1D pressure-reservoir characteristics act at moving outer material
boundaries; cold-air parcels explicitly occupy the buffer beyond the fixed
observation marker. These outer boundaries are not stationary engine ports.

An implicit midpoint pressure/force solve updates node velocities and geometry.
The same accepted midpoint pressure and velocity difference compute cell work.
No independent delay-state energy is added. A translated coordinate frame is
only a numerical representation: node velocities/kinetic energy remain lab
values; actual positions are relative x plus a compensated shift. Native tests
change this coordinate frame without supplying physical energy.

Physical wall overlap is evaluated in lab space. Each material gas cell uses
047's bounded pair operator against the same wall capacity/state; exterior
loss occurs once per wall/half-step. Gas/wall heat is equal and opposite and
source-separated per parcel. Pure exterior cooling is checked independently
against its exponential law, so refined gas meshes cannot duplicate cooling.
Configuration requires a successful single load. An unloaded object rejects
stepping/observations; repeated load rejects. Domain/numerical failure poisons
until reprepare. No tick allocation, EOS repair, species clipping or pressure
rewrite is used. Prescribed conductances are scenarios, not a calibrated
natural/pulsating exhaust heat-transfer correlation.

Final main evidence: `renders/evidence/stage-048/final-gcl/summary.json` and
raw per-case CSV/command/actual-probe hashes. Continuum scattering references
use the declared,047-audited initial EOS properties: R=(Z2-Z1)/(Z2+Z1),
T=2Z2/(Z1+Z2), Z=rho*c. Independent Galilean incident/reflected/transmitted
functions are averaged over actual control-volume widths; no measured arrival
or fitted filter tunes the reference. Reported frequencies are initial
material-frame modulation frequencies; lab values shift with U.

| Check | Coverage | Worst measured outcome |
| --- | --- | --- |
| Primary scattering | 12Gaussian/48carrier profiles; two temperatures/orders, three flows, two fronts | Gaussian3.46149% L2; carrier0.035889dB/7.21582°, pass frozen gates |
| Contact transport | 12 hot/cold material/partition/flow cases | False pressure4.366e-11Pa; velocity1.080e-12m/s; species error6.235e-19kg; enthalpy error1.333e-13J |
| Physical accounts | Profiles/contact/reversal/wall/long controls | Normalized total-E1.858e-14; momentum2.238e-14; immutable species preserved |
| Controlled bulk reversal | Explicitly booked uniform body acceleration, U=70cos(2pi100t) | Front displacement error2.486e-8m; actual ambient mass inside up to3.932e-4kg |
| Pressure-driven reversal |10Pa incoming rarefaction-first200Hz,20ms, no prescribed velocity/body force | Both flow signs, about±0.0314m/s, actual cold-air intrusion1.765e-7kg |
| Fixed wall |96/192/384 cells;384/768kHz clocks | One384.743158J/K wall; final Tw650.008262376–650.008262501K; heat books pass |
| Source-free stability | Two1s hot/cold4kHz-modulated fields, U=0/+70 | Sampled maximum/initial1; final ratios<=3.804e-10 |
| State/clock envelope | All supported main cases | Max8 midpoint iterations, material CFL<.291; unsupported768cells/192kHz rejects at1.161 before an accepted tick |

Regional fixed-plane species/enthalpy checks above are analytic translating
material controls. Global nodal/cell energy closure does not establish an exact
Eulerian energy flux at arbitrary observation cuts. That missing stationary
port/remap interface is explicitly assigned114; do not claim it was proved here.
Pressure-driven entrainment uses a finite1D buffer, not3D turbulent jet mixing
or a measured engine lift event. Radiation/excitation/listener work remains
051/053/056.

## Retained failures and improved numerical work

The first pressure-driven cycle started positive. It moved hot gas outside,
then drew that hot gas back; velocities reversed but no ambient parcel entered
the original duct marker. This correctly failed the intended ambient-entrainment
criterion. Keep this as `outlet-positive` history/control. A separate
rarefaction-first cycle proves actual cold-air return. The physics did not
change to relabel negative velocity as ambient entrainment.

**A carrier score is insufficient.** The frozen primary carrier gates pass,
but complete4kHz packet L2 reaches20.6457%; some coarser2kHz controls reach
48.669%. Coarse profiles are reported, not called full-waveform passes. The
768-cell Gaussian gate does not qualify all broadband transients. This is
retained as a required future whole-packet fidelity check, including114/057/060.

Supplemental finer controls initially rejected despite CFL<.446 and weak
pressure. They exposed loss of precision in recovering tiny volume changes
from differences of larger positions. The midpoint iteration could not satisfy
its unchanged2e-8Pa tolerance. The corrected compatible geometric-conservation
form uses `deltaV=dt*A*(v_mid_right-v_mid_left)` directly in pressure work and
midpoint volume, leaving geometric position rounding outside that work identity.
It does not relax the pressure tolerance, round pipe lengths or clip energy.
The former position-subtraction implementation is deliberately replanted; the
new fine-grid native regression catches it. All original failures remain.

With that correction,4kHz packets converge for both medium orders at the same
physical front/path and declared independent clocks:

| Cells / clock | Cold→hot packet L2 | Hot→cold packet L2 |
| --- | --- | --- |
|768 /768kHz|20.6457%|14.2339%|
|1536 /1536kHz|5.18198%|3.59270%|
|3072 /2000kHz|1.39217%|1.00585%|

These are supplemental accuracy references, not a uniform-refinement runtime
solution. The initial/subsequent matrices and both refinement reports are
retained. The final main matrix is rerun after the physical work correction.
The method remains a weak-acoustic, no-shock material reference within200–990K;
it does not repair043's Eulerian scheme or qualify046's evolving B0 coefficients.

## Validation and whole-system limits

- Final full native: **237tests/4,855,254checks pass**. Final focused material
  native/WASM/ASan/UBSan: **5tests/725checks each pass**, including the original
  fine-geometry regression. The final full native is rerun after the geometric
  work correction, not inferred from the preceding passing candidate.
- **88Python tests**, no skips, and **17web tests plus production build** pass.
  Seven native/WASM trajectories cover ±flow pressure profiles/contacts,
  pressure-driven outlet, controlled reversal and wall exchange; columns agree
  within1e-7 (full per-column residuals saved). No application engine worklet
  or browser deadline is certified. WASI observer timing is explicitly labeled
  monotonic wall time; native timing is process CPU time.
- **Eleven compiled behavioral faults are caught:** position-subtraction work,
  unloaded stepping, stale pressure force, duplicate pressure work, missing
  fuel inventory, missing body work, unbooked temperature rewrite, advected
  metal, exterior cooling repeated per gas cell, doubled coordinate drift and
  failed-state reuse. Earlier passing/failed matrices and all original fault
  outcomes remain available. No leak pass with ptrace-disabled leak detection.
- Native GCC -O3/no LTO/strict double, i7-1355U, six interleaved no-I/O10ms
  cost controls per U=0/+70 at768cells/768kHz: median simulated/CPU factors
  **0.014643/0.014554×**, ranges0.013224–0.016758 and0.013415–0.016509×.
  State/storage **246080bytes**. Two supplemental3072-cell/2MHz4kHz controls
  are **0.001123/0.001131×**, storage983360bytes. These are isolated reference
  duct costs, not comparable complete-engine speedups or a real-time solution.
  Uniform refinement here establishes convergence; it is not the runtime plan.

Final native measured probe SHA256:
`5e3863f0d5f91ad84afff1a70c20045a2903c27feba5333d429f3ec5e690e270`.
A frozen executable is retained outside the evolving build directory. Each raw
case records its actual command and executable hash; final source/validation
fingerprints are saved alongside the evidence. WASM trajectory executable:
`3b7f95cb2ba9e4ca524bf522f7522240f6f7753911e21cfa8a11a1216b42afc4`.

## Explicit split, decision and next prerequisite

**048 completes the bounded material-reference fixture experiment.** It is not
completion of a fixed-geometry evolving B0/engine adapter. That missing part is
now a distinct prerequisite, [Stage114](roadmap/04-propagation.md#stage-114),
**before049**. Existing IDs stay stable.049 cannot assume that a moving-cell
reference or arbitrary observation plane already exchanges conserved gas with
stationary ports.114 must prove a conservative contact-aware fixed-domain/ALE
bridge or independently qualify another evolving adapter against this reference.
No implementation/promotion of114 is claimed or started in this authorization.

This split preserves the original fixed-domain/thermal/common requirements
instead of silently exempting the new method. It does not select material
hydrodynamics over a future efficient gas/acoustic path. Branches/junctions049,
components050, radiation051, mixed-rate scheduling052 and engine qualification
057/059/060 still retain their own evidence gates. Absolute realism,039 matching,
S1/S2/S4/listening and runtime acceptance remain open. Inherited hot-gas EOS
and actual wall material/heat-transfer calibration remain unresolved before
whole-engine claims; the new passing propagation envelope stops at990K.

The combined `thermal-checkpoint` publisher adds the047/048 research figure to
the single RESULTS page; it regenerates no engine sound. All accepted examples,
long coasts, past numerical sections and reference views are retained. The
future whole-engine new-system comparisons remain057 and promotion061.

Reproduce: `make BUILD=/tmp/rev-bench-048 LTO=0` targets `rb_tests` and
`material_probe`; first reproduce047's coefficient export, then run
`tools/check_material.py --probe /tmp/rev-bench-048/material_probe --out renders/evidence/stage-048/final-gcl`
and `tools/check_material_refinement.py --probe /tmp/rev-bench-048/material_probe --out renders/evidence/stage-048/packet-refinement-gcl`
with the analysis Python. Native/WASM targets, source headers and tool objects
are included in Makefile dependency tracking. The publisher takes the047 main,
048 main and048 refinement summaries; standard `render_results.py --publish-only`
rebuilds everything from registered fragments without analysis dependencies.

**Hold after048. Next dependency is114; do not start049 or change accepted clips.**

Final page/preservation review: all55 players play offline, exclusive playback
and all four long-coast/top-range seek controls pass; all eleven research plots
and three recording-perspective images load without script/network errors.
All68 previous embedded media payloads are exact, with only the new figure
added. All74 protected production/contract/Claude files and105 accepted current
files are byte-identical. Publication timestamps/page/registered-section manifest
are intentional changes; no raw sound or recording was processed or replaced.
Graphify queried before code exploration, then refreshed3220nodes/9091edges/
176communities. The known public-header parser, inferred-edge and stale semantic
label limits remain; AST refresh does not claim new semantic Markdown extraction.
The114-stage dependency DAG and343 local planning/notes/card links pass.049 now
correctly depends on114. Final review confirms all scoped reference gates pass,
with the full fixed-domain continuation, high-band cost and hot EOS limits above
explicitly preserved. Save/sync the two stage commits; HOLD after048.
