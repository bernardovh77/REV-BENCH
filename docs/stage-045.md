# Stage 045 — Production-mixture gas to actual B0 graph

2026-10-25. Bernardo authorizes045 then046, with predecessor review. Baseline
`c33545aa0efec75520422f5954b067908877bd6b`; tracked sources clean, user videos/
skills untouched. Hashes/evidence: ignored `renders/evidence/stage-045/`.

## Preflight, research and ownership

044's compiler/native consumer agree on all physical regions, finite volumes
and all source/outlet paths. Its gas slots were initialized only; B0 remains
a lossless geometry control. Production engine coupling, full component loss,
real installation routes and listening acceptance are not inferred from044.
043's conservative candidate stays rejected. No accepted engine configuration
or sound changes are authorized by these standalone propagation experiments.

Implement the smallest actual B0 coupling: one retained nonlinear gas region
and one supported uniform-area B0 duct, initially quiescent and homogeneous
with the production three-species EOS. Multiple ports/finite junctions await049.
Lengths need not be sample aligned; reject insufficient acoustic delay or
storage rather than round geometry. This is distinct from the old constant-
gamma, sample-aligned `coupled_duct` control.

Primary starting references: [Clawpack Euler](https://www.clawpack.org/riemann_book/html/Euler_approximate.html),
[Bailly/Juvé linearized Euler](https://acoustique.ec-lyon.fr/publi/bailly_aiaaj00.pdf),
[Smith's fractional delays](https://www.dsprelated.com/freebooks/pasp/Delay_Line_Signal_Interpolation.html)
and the existing [ownership ADR](adr/0001-coupled-exhaust-ownership.md).
These motivate characteristic/residual splitting, not certify this new method.

Conserved state: three species densities, axial momentum and sensible total
energy. For quiescent reference T0/Y0, beta=R0/cv0,
As=T0*Rs-beta*es(T0), and
pi=beta*(E-E0)+sum(As*(rho_s-rho_s0)). The acoustic coordinates are
a=(pi+c0*m)/2, b=(pi-c0*m)/2, with stationary contact coordinates
zs=(rho_s-rho_s0)-Y0s*pi/c0². Linear flux is
[Y0s*m, p0+pi, h0*m]; nonlinear residual is actual production-EOS Euler flux
minus that flux. Pressure from the nonlinear EOS is not blindly a+b+p0.

B0 wave delay/filter coordinates and the contact coordinates jointly own the
downstream physical state. A derived gas cache is never advanced independently.
Residual updates are conservative and projected back to these coordinates;
the upstream gas uses the same single accepted interface flux with opposite
sign. Physical total-energy transfer already contains pressure work. B0's
quadratic acoustic work is a diagnostic of these same degrees of freedom,
never an extra thermodynamic energy reservoir added to the gas book.
Residual dissipation separates acoustic and contact directions; a stationary
same-temperature composition contact has no artificial contact-speed term.
The initial upstream acoustic trial is third order on small smooth gradients
and MC-limited otherwise; the chosen.002*p0 local gradient switch is a
numerical experiment setting, not an acoustic realism calibration.

For the existing orthogonal fractional filter, coefficient A=(1-f)/(1+f),
memory v has signed DC inventory sqrt(f)*v: its change equals x-y. Retain this
inventory as well as its quadratic storage. Conservative overlap/remapping
reconstructs opposite-traveling coordinates on a shared physical grid. Merge
the fractional end with its neighboring control volume without changing any
acoustic path length; do not evolve a vanishing-size explicit cell.

## Gates frozen before implementation measurements

- 300/900K, pure air and mixture Y=[.1,.001,.899]; p0=101325Pa, area=.003m².
  Duct1.50317m, two gas/network positions .313/.557m, 192kHz common tick;
  independent 96/384kHz controls. Small .00001/.001 and finite .01 relative
  pressure pulses; .05/.1 are explicit adverse/domain screens, not promised
  operating amplitudes. No repair/normalization of negative species.
  Upstream cell counts are floor(interface_x*96000/c0), held fixed for the
  independent tick comparison. Additional upstream spatial controls do not
  retime the B0 path or change the physical geometry.
- Independent fixed-composition variable-cp isentropic simple-wave reference
  (thermodynamic integrals and characteristic mapping): <=2% pressure waveform
  relative L2 before shock, over1m travel. Weak 200/1000/2000/4000Hz carriers
  additionally screen <=1dB/10° on the tested path; report failures separately.
- Same-tick return at R=-1/0/+1; correct upstream polarity and ideal path timing
  within5 ticks; matched return below.1Pa at .001 amplitude in its declared
  post-pulse window. Catch a one-tick feedback delay by a direct clock test.
- Both regions separately close each species, momentum and total energy with
  quantity-specific1e-10 relative inventory scales, plus dimensional residuals.
  Nonzero actual gas/species crosses the interface. Stationary contacts and
  bounded composition perturbations exercise retained gas, not a diaphragm.
- Source-free compact-duct1s controls at all three terminal loads: sampled
  perturbation-energy maximum<=1.02 initial and final<.001 initial with a
  matched left end. This is a leaky-domain stability screen, not engine runtime
  or an indefinitely trapped nonlinear standing-wave proof.
- Record all domain/CFL failures, limiter/residual interventions, coordinate
  work, storage and complete no-file/no-trace cost. No speedup claim from
  static B0 geometry timings. Reject the arm if physical acceptance fails.
- Native fixtures first; full native/Python/web regression, focused WASM and
  ASan/UBSan, and deliberate sign/delay/duplicate-energy/species faults.

## Executed method and rejected controls

`experiments/acoustics/gas_graph.[ch]` now calls actual B0 receive/commit.
Upstream gas uses production EOS sensible energy; downstream B0 coordinates
and three contact coordinates jointly define conserved gas state. A physical
cache is reconstructed through conservative overlap maps, never advanced as a
second copy. Signed fractional-filter inventory includes sqrt(f)*memory;
integer and nearly integer lengths are explicitly tested. No physical length
or branch is rounded to make a solver convenient. Unsupported topology remains
explicit: one uniform duct, one gas/graph interface; arbitrary junctions and
component loss are not covered by this proof.

The final split uses conservative MC residual reconstruction/RK2 half steps,
two SSP-RK3 acoustic substeps, fifth-order WENO-Z-type characteristic
reconstruction and one-sided third-order outflow faces. See the primary
[Borges et al. paper](https://doi.org/10.1016/j.jcp.2007.11.038) for WENO-Z's
smoothness-weight principle; the coupled scheme and boundary projection are
our experiment, not that paper's engine validation. WENO weights blend
smoothly from optimal linear weights between1e-4 and4e-4 of reference pressure
to preserve weak-wave linear superposition. These are numerical settings,
not reference-audio fits or universal physical thresholds. All numerical
interface contributions use the same RK weights on both sides.

The first diagnostic test used B0's balance with the wrong sign for added
coordinate work. B0 defines source-minus-storage-minus-loss, so its residual
is **minus** the separately recorded coordinate-energy increment. Correcting
the observer sign fixes that test; it never changes a physical energy state
or relaxes an acceptance threshold. First-run failures remain local.

Initial first-order residual transport and the amplitude/gradient-switched
upstream trial are rejected for diffusion: the first sweep's finite900K pulse
errors reach16.65%; the next trial reduces them to5.034% but still fails.
The following WENO-only trial passes Gaussian pulses but has a2.470% worst
weak-carrier waveform error, also retained as a rejection. The final weak-
linear/finite-WENO construction passes the original2% screen. No failed case
is removed or relabeled as a success. A NumPy-bool serialization bug in the
first report writer is fixed; its raw CSV/reference arrays remain available.

## Numerical results and valid envelope

- **62 independent physical profiles:**24 Gaussian pulse rows,32 carriers,
  four fixed-upstream-mesh tick controls and two spatial controls. All pass
  declared waveform, phase and inventory gates. Worst waveform L2 is
  **1.9021%**; Gaussian-only maximum **1.0549%**. The tested weak carriers have
  at most **0.02237dB** magnitude and **0.007891°** carrier-phase error. Pressure
  waveform error includes envelope distortion; a carrier score alone is not
  substituted for it.
- **Six return cases:** R=-1/+1 returns have correct polarity, with peaks about
  −100.845/+100.931Pa at the first partition and−100.781/+100.870Pa at the
  second. Maximum ideal-path peak timing error **1.476ticks**. Matched-return
  peaks are **0.007172Pa**, below0.1Pa. The actual received and used pressure
  values agree on every observed tick, with no callback or extra feedback tick.
- **Actual species/enthalpy transfer:** mixture-contact pulse transfer is
  approximately[1.58129e-8,1.59726e-10,1.43754e-7]kg for air/fuel/burned,
  with0.0139631J total-energy transfer. A separate nonuniform-fuel fixture
  detects replacing actual fuel transport with the background mixture ratio.
  Stationary same-temperature composition contacts preserve pressure/species;
  sharp thermal fronts are not certified here.
- **Separate regional books:** maximum normalized species/momentum/total-
  energy residual across profiles, returns and1s cases is **8.098e-14**.
  Reference scales are rho*A*L for each species, rho*c*A*L for momentum and
  rho*c²*A*L for energy; dimensional residuals are retained. B0 coordinate
  work is reconciled separately and is never added twice to thermodynamic E.
- **Three1s compact-duct screens:** no sampled perturbation-energy growth
  (maximum/initial=1); final/initial <=**3.185e-20**. Matched upstream leakage
  permits energy to leave: these are not a closed nonlinear standing-wave or
  physical exhaust radiation qualification.
- Residual max CFL **0.03719**, zero reconstructed-state fallback across the
  profile matrix. All sampled supported cases remain admissible. Relative
  amplitudes.05/.1 explicitly reject initial domain; no hidden limiter,
  temperature repair or pressure overwrite makes them pass.
- **Ten shared-geometry component controls:** one exhaust and one tailpipe
  segment from each actual044 compiled base build feed this adapter with exact
  lengths/areas/port IDs. All exhibit reflected load and close their gas books.
  This validates the selected uniform subducts, not complete5-engine exhaust
  operation, catalyst/muffler loss, engine breathing or a production renderer.

The retained upstream mesh is held fixed while96/192/384kHz clocks vary.
B0's propagation coordinates change naturally with the clock and preserve
the same physical length; this is not a claim of an independently refined
mean-gas mesh. Supported accuracy is limited to the tested reference mixtures,
temperatures, amplitudes, uniform areas and prescribed terminal conditions.
The production apparent gas-property fit is inherited, not newly calibrated.

## Regression, failures, cost and handoff

Full native **226tests/4,845,645checks** pass. The final five new coupling tests
also pass **5tests/2268checks** after the last reconstruction changes and
expanded integer/near-integer coverage. Python **87 pass/no skips**,
web **17 pass** plus typecheck/production build. Focused same-source WASM and
ASan/UBSan each pass5tests/2268checks including the final fractional
coverage extension. No leak qualification: leak detection is disabled under
ptrace. Production core, WASM and all accepted audio remain unchanged.

All seven compiled faults are caught: stale feedback, reversed feedback,
lost fractional inventory, duplicate pressure work, missing fuel transport,
reused failed state and kinetic-pressure confusion. Sources/binaries, commands,
old rejected matrices and independent reference arrays are preserved under
`renders/evidence/stage-045/`; the final measured probe is frozen separately
to avoid mixing versions while subsequent stages are built.

Complete profile-step costs range about0.000834–0.01138x real time on the
shared host, including retained correction grids and actual EOS/state guards.
Maximum measured storage **511584bytes**. Interleaved no-trace/no-file cost
controls are retained separately from return/stability traces. This is a
correctness reference and **not an efficiency solution** or native/WASM engine
speedup. The downstream correction grid is still expensive;052/060/062 retain
the mixed-rate/architecture/cost decisions. Uniform fine transport cannot
be silently promoted as the long-term answer to the original bottleneck.

Reproduce with `make BUILD=/tmp/rev-bench-045 LTO=0` targets `rb_tests` and
`gas_graph_probe`, prefixed by BUILD. The native matrix is
`tools/check_gas_graph.py --probe /tmp/rev-bench-045/gas_graph_probe --out
renders/evidence/stage-045/final-matrix --native-contract
/tmp/rev-bench-044/build_contract_probe` with the analysis Python/PYTHONPATH.
`tools/plant_bugs.py 045 /tmp/rev-bench-045-final-faults all` runs the seven faults.

Review before046:045 completes its bounded production-mixture/actual-B0
interface proof and has a surviving **research** artifact. Retain full regional
books, raw reference/gas states, domain failures and adverse cases. Extend
uniform flow next; do not reuse quiescent mass-flow/power formulas. No current
engine clip changes or production promotion.039/S1/S2/listening/runtime remain
open. Graphify is queried/refreshed3101nodes/8655edges/165communities, retaining
the known rb.h parser/inferred-edge limitations. Claude setup is preserved.
