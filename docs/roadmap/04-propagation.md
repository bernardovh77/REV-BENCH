# Propagation prototypes and physical components

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-043"></a>
## Stage 043 — Improved conservative solver and production-mixture control

**Dependencies:** [035](../../docs/roadmap/02-existing-sound.md#stage-035), [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038)

**Deliverable:** Extend or replace the rejected initial conservative configuration using the production mixture EOS, coupled transported variables and valid valve/boundary states. Keep the original A0 and rejected A1 as identified controls.

**Required exit evidence:** Independent weak-wave magnitude/phase, shock/contact/rarefaction, species/energy, amplitude and valve/neighbor-interface controls. Own the rejected coupled implicit-valve/neighbor-face convergence case: demonstrate a corrected solve or record why the selected method supersedes that attempted optimization without losing its physical acceptance requirements. Vary spatial mesh, pipe tick and engine tick independently; record supported bandwidth, errors, guard use and full cost.

**Stop/reject rule:** Conservation alone does not pass acoustic fidelity; uniform refinement alone is not accepted as an efficient architecture. Reject the candidate if its stated gates fail.

**Legacy coverage:** S2d-A; S2; R05; 13b.

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md), [docs/stage-S2d.md](../../docs/stage-S2d.md).

**Execution record:** [043](../stage-043.md): production-EOS A2 experiment complete;
rejected on acoustic bandwidth, moving-contact pressure/admissibility and cost.
No qualified production adapter; A0/A1 retained. Future conservative replacements
must re-pass these gates; B0/B1 work remains independent.

<a id="stage-044"></a>
## Stage 044 — Executable shared exhaust geometry compiler

**Dependencies:** [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037), [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038)

**Deliverable:** Compile a bounded supported subset of the canonical description into actual gas/acoustic connectivity, source paths and spatial identity. Include complete primaries, collectors, links, X/Y, catalysts, mufflers and outlets with measured/estimated dimensions distinguished.

**Required exit evidence:** Topology/length/area/volume and all source-outlet paths match the descriptor; five base configurations and identity changes are tested. Capacity and unsupported components fail during preparation. Verify derived 3D lengths and physical acoustic lengths agree.

**Stop/reject rule:** A snapshot, a catalog exhaust name or a collector-selected path is not an executable compiler. Do not silently drop branches or enlarge cells.

**Legacy coverage:** S2d-0; R07; D04.

**Starting evidence/specification:** [docs/stage-13b-p.md](../../docs/stage-13b-p.md), [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** [044](../stage-044.md): canonical description compiles into
gas region/port/cell preparation and actual B0 lossless geometry controls for
all five builds. Full topology/storage/provenance,42 connected/14 disconnected
pairs, spatial consistency, native/WASM and eight compiler faults pass.
Full component impedance, engine coupling and real installation routes remain
later gates; unsupported physical-exhaust execution explicitly rejects.

<a id="stage-045"></a>
## Stage 045 — General zero-flow open gas-to-graph coupling

**Dependencies:** [034](../../docs/roadmap/02-existing-sound.md#stage-034), [035](../../docs/roadmap/02-existing-sound.md#stage-035), [043](../../docs/roadmap/04-propagation.md#stage-043), [044](../../docs/roadmap/04-propagation.md#stage-044), [113](14-review-fixes.md#stage-113)

**Deliverable:** Connect retained production-mixture gas to the actual B0 graph for supported arbitrary physical lengths. Define current receive/solve/commit, pressure/flow normals and one accepted mass/species/total-energy interface flux with reflected load.

**Required exit evidence:** Compare two interface positions, amplitudes and terminal loads against independent physical controls; test current reflection sign/time, actual transported gas, finite pulses, separate regional books and source-free stability. Catch wrong sign, extra delay and duplicate work.

**Stop/reject rule:** The existing homogeneous characteristic duct is a control, not this adapter. No pressure overwrite, extra acoustic energy reservoir or callback-sized feedback delay.

**Legacy coverage:** S2d-0; S2d-B1; S2d.

**Starting evidence/specification:** [docs/stage-S2d-0b-B1a.md](../../docs/stage-S2d-0b-B1a.md), [docs/adr/0001-coupled-exhaust-ownership.md](../../docs/adr/0001-coupled-exhaust-ownership.md).

**Execution record:** [045](../stage-045.md): production-EOS retained gas and
actual B0 uniform-duct graph with fractional lengths, one accepted conservative
flux and current reflection. Independent profile/phase/return/book/stability
gates pass; seven faults caught. Finite junctions, full-engine integration,
thermal fronts and efficient scheduling remain later work.

<a id="stage-046"></a>
## Stage 046 — Uniform mean-flow coupling

**Dependencies:** [045](../../docs/roadmap/04-propagation.md#stage-045)

**Deliverable:** For every surviving transport arm, add or qualify frozen positive/negative mean flow with entropy/species advection, correctly oriented characteristics and explicit convective-energy/mean-flow work ownership. A recorded rejection of the coupled arm removes that arm-specific adapter requirement, not mean-flow validation for a surviving conservative solver.

**Required exit evidence:** Independent forward/back travel, flow-dependent impedance, transported contacts and gas-volume versus acoustic-mass-flow controls; conservation and long-duration stability across a declared Mach range.

**Stop/reject rule:** Do not reuse zero-flow scattering or impose a no-gain rule that ignores mean-flow energy. Reject unsupported transonic/strongly nonlinear regimes.

**Legacy coverage:** S2d-B1; S2d.

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Executed:** [Stage046](../stage-046.md) passes its frozen uniform-flow
matrix with separate c±U delays, conservative composition/weak-entropy advection,
current reflected feedback and signed convected work. Gaussian shape, carrier
transfer, directional travel, separate regional books and six1s cases pass;
resolved reflection energy gain is accounted. Full correction is far below
real time; thermal fronts, flowing junctions and production integration remain
later gates. Weak-carrier envelope error and all limits are retained.

<a id="stage-047"></a>
## Stage 047 — Gas-property and wall-thermal model characterization

**Dependencies:** [043](../../docs/roadmap/04-propagation.md#stage-043), [046](../../docs/roadmap/04-propagation.md#stage-046)

**Deliverable:** Research and measure production cp/composition/sound-speed assumptions and fixed-wall sensitivity. Determine what wall heat capacity and heat-transfer state are required before introducing a new thermal model.

**Required exit evidence:** Property consistency, hot-gas and wall-energy controls, known thermal limits and sensitivity/convergence experiments. Distinguish local gas temperature, wall temperature and whole-exhaust cooling.

**Stop/reject rule:** A gas-temperature jump at lift does not prove the metal instantly cooled. Added complexity requires measured relevance and cost.

**Legacy coverage:** R09; S2d; S2e.

**Starting evidence/specification:** [docs/stage-S2c.md](../../docs/stage-S2c.md), [docs/audio-robustness-research.md](../../docs/audio-robustness-research.md).

**Execution record:** for new work, `docs/stage-047.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-048"></a>
## Stage 048 — Thermal fronts, coefficient updates and reverse entrainment

**Dependencies:** [046](../../docs/roadmap/04-propagation.md#stage-046), [047](../../docs/roadmap/04-propagation.md#stage-047)

**Deliverable:** Progress from fixed thermal gradients to evolving hot/cold and species inventories, changing impedances/delays and realistic open-end reversal. Retain downstream mean state while propagating acoustics.

**Required exit evidence:** Independent thermal/contact-front and reversal fixtures; refine space/time separately; reconcile gas/species/enthalpy and coefficient-update work. Repeat interface locations and source-free tests under time variation.

**Stop/reject rule:** A retimed delay is not transported mean gas. Coefficient changes must not create unbooked energy or hide the documented refined-network growth failure.

**Legacy coverage:** S2d; S2e; R01; R09.

**Starting evidence/specification:** [docs/stage-S2c.md](../../docs/stage-S2c.md), [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-048.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-049"></a>
## Stage 049 — Branches, finite junctions and all-port transfer

**Dependencies:** [044](../../docs/roadmap/04-propagation.md#stage-044), [048](../../docs/roadmap/04-propagation.md#stage-048)

**Deliverable:** Validate area changes, cavities, Y/X/H routes and finite junction volumes under unequal areas, temperatures and supported flows. Test each physically connected source against every outlet and upstream load.

**Required exit evidence:** Analytic no-flow limits plus finite hot-flow controls; complex transfer, conservation, pressure/flow continuity and breathing response. Compare compact versus finite-junction validity and branch identity.

**Stop/reject rule:** An ideal compact zero-flow X does not certify a real finite hot crossover. Unverified ports or copied outlet signals fail.

**Legacy coverage:** S2d; S2e; D03; R07.

**Starting evidence/specification:** [docs/stage-13b-p.md](../../docs/stage-13b-p.md), [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-049.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-050"></a>
## Stage 050 — Catalysts, mufflers, perforates and packing

**Dependencies:** [047](../../docs/roadmap/04-propagation.md#stage-047), [049](../../docs/roadmap/04-propagation.md#stage-049)

**Deliverable:** Research causal impedance, perforated/packed paths, cavities and pressure loss for the actual model-specific exhausts. Replace inappropriate topology or fixed loss assumptions only with supported component models.

**Required exit evidence:** Magnitude and phase, storage/loss, steady backpressure and uncertainty against analytic/measured controls; passivity for no-flow components and explicit mean-flow energy accounting. Hold out at least one geometry/configuration.

**Stop/reject rule:** Brightness alone and a one-pole attenuation fit do not validate a full muffler. Estimated internals must remain marked as estimates.

**Legacy coverage:** S2e; R07; R08; D05.

**Starting evidence/specification:** [docs/stage-13b-p.md](../../docs/stage-13b-p.md), [docs/audio-robustness-research.md](../../docs/audio-robustness-research.md).

**Execution record:** for new work, `docs/stage-050.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-051"></a>
## Stage 051 — Radiation load and audible-field consistency

**Dependencies:** [049](../../docs/roadmap/04-propagation.md#stage-049)

**Deliverable:** Jointly validate aperture impedance, reactive storage, radiated pressure/power and directivity over a declared ka/flow/geometry range. Define body, flare and finite-aperture limits.

**Required exit evidence:** Independent terminal response and far-field magnitude/phase/work controls; correct source distance and causality. Test no-flow versus supported mean-flow cases without double loading the outlet.

**Stop/reject rule:** Radiation load is not far-field sound. Do not count filter storage as radiated energy or add a second load in the listener.

**Legacy coverage:** S2e; R10.

**Starting evidence/specification:** [docs/stage-13b-p.md](../../docs/stage-13b-p.md), [GLOSSARY.md](../../GLOSSARY.md).

**Execution record:** for new work, `docs/stage-051.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-052"></a>
## Stage 052 — Mixed-rate scheduler and state transitions

**Dependencies:** [048](../../docs/roadmap/04-propagation.md#stage-048), [049](../../docs/roadmap/04-propagation.md#stage-049), [050](../../docs/roadmap/04-propagation.md#stage-050), [051](../../docs/roadmap/04-propagation.md#stage-051)

**Deliverable:** Compare reduced-rate gas/mean/acoustic scheduling and coefficient/rebuild transitions against the common-clock control; move preparation out of the sample loop without changing load ownership.

**Required exit evidence:** Integrated flux/work, phase/delay, resampling and long-run state consistency across sample/block sizes and transitions. Record no-trace full-fixture cost and catch an inserted feedback sample.

**Stop/reject rule:** Do not break algebraic coupling by adding an undocumented delay. A crossfade cannot conceal an incorrect physical state transfer.

**Legacy coverage:** S2d; 13b; 13b-c.

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-052.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
