# Stage047 — Gas-property and wall-thermal characterization

2026-10-25. Authorized047 then048; review before advancing and hold after048.
Baseline `c19357a`, clean tracked tree; original media/user skills preserved.
Ignored raw evidence: `renders/evidence/stage-047/`. Production core and accepted
clips stay unchanged during this characterization; no renderer promotion.

046 review: frozen uniform-flow results pass their stated numerical scope, but
+1mK entropy contacts do not qualify sharp hot/cold fronts. Its full correction
is far below real time. 043's Eulerian moving-contact failures remain rejected.

## Research, questions and gates frozen before measurements

[Cantera frozen/equilibrium sound-speed example](https://www.cantera.org/3.1/examples/python/thermo/sound_speed.html)
distinguishes fixed-composition isentropic derivatives from equilibrated ones.
The production burned-gas high branch is an apparent40bar combustion fit with
fixed molecular weight; neither frozen exhaust cp nor equilibrium acoustic
relaxation is thereby validated. [NASA](https://ntrs.nasa.gov/citations/20020085330)
provides the thermodynamic framework; the independent versioned reference is
[Cantera3.2 NASA gas data](https://raw.githubusercontent.com/Cantera/cantera/v3.2.0/data/nasa_gas.yaml),
which itself cites NASA TM4513(1993). Do not conflate that dataset with a direct
copy of the2002 NASA9 table. No chemistry package is a runtime dependency.

[Fu et al.2005](https://doi.org/10.4271/2005-01-0696) and
[Liu et al.1995](https://doi.org/10.4271/950617) identify wall/inlet/external heat
transfer and thermal storage in transient exhaust models (abstracts consulted;
no unseen paper equations asserted). [Thermal capacitance theory](https://doc.comsol.com/6.4/doc/com.comsol.help.heat/heat_ug_theory.07.045.html)
motivates a finite wall energy state with a declared small-Biot envelope.

Freeze: five mixtures (pure air/fuel/burned plus two mixed),20–5000K including
both fit joins; 200–4000K is fitted range, outside values extrapolation controls.
Check cp-cv=R, h-e=RT, positive cv, energy continuity and temperature inversion
with three initial guesses (relative residual<=1e-8). Numerical derivatives
away from joins agree with cv within1e-6 relative; independently integrate the
production entropy curve for isentropic sound speed within1e-5 relative.
Compare independent NASA7 air and frozen products separately: air cp<=.5% and
sound speed<=.2%; products below1000K cp<=1% and sound speed<=.5%. Report high
branch differences and all cp/c discontinuities without smoothing/calibration.

Instrument actual fixed-wall production pipe heat in prescribed-flow fixtures:
energy increment versus recorded wall heat<=1e-10 relative, both flow signs,
zero flow and wall/gas equal controls. Independently audit its existing
St=(f/8)*Pr^(-2/3) closure; do not call it measured heat-transfer data.

Prototype an isolated energy-conserving gas/wall pair before any production
thermal change: gas sensible E plus finite Cw*Tw, conductances Hi/Ho (W/K),
external reservoir heat explicit. Backward-Euler heat exchange solves one
bounded scalar energy transfer; no temperature overwrite. Constant-cv exact
limits, variable-cv independent integration, dt halving, equilibrium/no-flux,
energy closure<=1e-10 relative and no overshoot. Mesh-independent wall capacity
scales with physical wall volume, not number of cells. Parameter sensitivities
use D=.06m,L=1m,thickness=.001/.002m,rho_wall=7800,cp_wall=500,k_wall=16
and h_inner=50/200,h_outer=10/40 as explicit scenarios, not measured presets.
Reject a one-node wall interpretation if max(h)*thickness/k>=.1. Report gas,
wall and exterior thermal times separately. Full native/Python/web regressions,
focused WASM/sanitizers, meaningful planted faults; no full engine rerender
unless production sources change. Close with required048 property/wall contract.

## Executed result and review

All frozen characterization gates pass. `thermal_probe` exercises85 property
rows,18 actual production fixed-wall operator rows, six independent variable-cv
pair/convergence rows and eight10s physical wall scenarios. The original NASA
YAML is ignored; the attributed/pinned eight-species NASA7 reference subset is
committed in `tools/thermal_reference.json`. No new YAML/chemistry dependency:
a missing optional YAML reader was replaced by extraction of the pinned NASA7
blocks using the standard library; the exported coefficients retain source hash
`4de6199d65d2d3db782e30573720c723130953707336add59713b02d8667e4db`.

- Max temperature-inversion relative error6.362e-10; independent isentropic
  sound derivative error7.094e-7. cp/cv, enthalpy, energy derivative and branch
  energy continuity pass. Newton limit/floor counts remain in every raw row;
  out-of-fit20/199/4001/5000K cases are labeled extrapolation controls.
- Against separately evaluated NASA7 mixtures, air max cp/c differences are
  0.004339%/0.000780%; fuel0.003894%/0.001210%; frozen products at<=1000K
  cp<=0.005728%. Reference molar masses/mole fractions are explicit; these
  small composition/rounding differences are not exact production definitions.
- **Important property limit:** pure burned cp jumps−0.53810% across1000K;
  its implied c jumps+0.079743%. For the .1/.001/.899 mixture these are
  −0.48748%/+0.072865%. Internal energy is continuous. At2500K the inherited
  apparent cp exceeds the frozen-product reference by36.0125%, reducing its
  computed c by3.1031%; at4000K differences reach160.815%/−6.5743%. These are
  differing closures, not evidence the reference exhaust must be in chemical
  equilibrium. A pressure-specific equilibrium combustion fit and fixed R do
  not supply a general acoustic relaxation law. Do not silently reuse it as
  calibrated frozen/equilibrium exhaust acoustics or smooth it with audio EQ.
- Actual production pipe heat equals its declared Colburn/Stanton operator at
  both flow signs, zero flow and equal temperatures. Recorded gas+kinetic
  change closes against pipe heat. The prescribed-flow operator fixture does
  not claim a complete advecting pipe solution. Walls remain fixed by design;
  no solid-energy inventory exists in production and h vanishes at zero flow.
- The new isolated `thermal.[ch]` pair owns gas E and Cw*Tw, with explicit
  exterior heat. A bounded backward-Euler scalar solve uses variable cv,
  without clipping T. Constant-cv exact-ODE refinement is first order as
  expected. Independent RK4 references at the finest1us tick differ by only
  0.000140K (air) /0.000114K (mixture). Maximum absolute energy-book residual
  across the recorded pairs/scenarios is2.433e-8J, far below the1e-10 relative
  inventory gate. Caller-owned state is272bytes; no stepping allocation.
- Scenario wall capacities are747.385/1519.274J/K for1/2mm thickness, with
  Bi0.003125–0.025. Gas time constants0.07468–0.29874s differ from wall
  constants16.4296–132.858s. After10s, wall states span616.278–645.845K from
  initial650K. This demonstrates thermal-memory relevance, not measured
  temperatures/materials for the engine presets. One-node walls require the
  stated low-Biot/local-uniformity assumption; catalyst/packing storage,
  radial/axial conduction and radiation remain separate component work.
- Three no-I/O100000-call native microbenchmarks take128.90/130.34/133.78ns
  per pair call on this i7-1355U/GCC -O3/no-LTO host, including one reset per
 1000 calls. This272byte element cost is not an integrated exhaust/runtime
  speed claim. New wall material/geometry/calibration remains required.

Validation: full native **232tests/4,854,529checks**; focused thermal including
an existing diagnostics control **4tests/5,656checks** in native/WASM/ASan/UBSan;
**88Python**, no skips, and **17web tests plus build** pass. Native/WASM CSV
properties85/fixed-wall18/pair11 rows match exactly. Five deliberate faults
are caught: wall energy sign, missing external heat book, capacity duplicated
with mesh, lost gas heat and missing flow enthalpy work. Leak detection disabled
under ptrace; no leak/runtime qualification. Graphify queried then refreshed
3160nodes/8880edges/167communities, retaining the known rb.h parser/inferred-edge
and semantic-label limits.

Retained initial failures: a test incorrectly prohibited the wall briefly
warming above its initial650K, although its900K gas supplies heat; its absolute
1e-8J book tolerance also did not match the frozen relative inventory gate.
The corrected test checks the global initial/reservoir temperature envelope
and the original1e-10 relative requirement. The physical model is unchanged.
A probe loop-count guard now snaps numerically integral duration/dt ratios to
the exact requested count and rejects nonfinite clocks. Both matrix versions
are retained. The main frozen physical thresholds were never changed.

## Required048 contract and outcome

**047 complete and reviewed for048.** Keep the inherited combustion EOS unchanged
in production.048's passing propagation envelope must explicitly stay below
the apparent-cp branch (200–990K); high-temperature exhaust EOS/relaxation and
branch treatment remain required before057/060 promotion. The original047
characterization itself covers/exposes the hotter range. Wall state needs
physical capacity, fixed spatial identity and equal/opposite heat; gas moving
downstream must not drag the metal with it. Capacities cannot multiply with
refinement. Source/local gas T, wall T and whole-exhaust cooling stay separate.

Strong thermal/composition fronts expose043's Eulerian mixing failure, which
046's fixed small-perturbation reference cannot simply bypass.048 must test an
explicit material/contact-preserving alternative and a conservative physical
book, including coefficient-change/source-free controls and actual ambient
transport on reversal. A material-coordinate research reference is permitted;
it does not qualify a production arbitrary-delay updater or choose the final
renderer. Finite-junction and radiation/component qualification remain049–056.

Reproduce: `make BUILD=/tmp/rev-bench-047 LTO=0` targets `rb_tests`,
`thermal_probe`, `thermal_bench` (prefixed by that BUILD), then
`.venv-analysis/bin/python tools/check_thermal.py --probe /tmp/rev-bench-047/thermal_probe --out renders/evidence/stage-047/matrix-final`.
Run the full native/Python/web commands in the roadmap protocol and047 fault
harness. Raw initial/final matrices, validation logs, sources and cost records
remain ignored under047 evidence. No production code, accepted raw media,
reference split, engine/exhaust settings or Claude setup changed.048 is next
within this authorization; hold after048, without starting049.
