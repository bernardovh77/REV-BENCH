# Engine integration, architecture selection and base acceptance

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-053"></a>
## Stage 053 — Compressible and reversing turbulent-flow excitation

**Dependencies:** [040](../../docs/roadmap/03-contracts-and-sources.md#stage-040), [042](../../docs/roadmap/03-contracts-and-sources.md#stage-042), [047](../../docs/roadmap/04-propagation.md#stage-047), [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038)

**Deliverable:** Use physically attainable nozzle/throat states for choked and unchoked noise; research reverse flow, zero-flow limits, energy/spectrum/modulation and calibration uncertainty. Before a stochastic source enters reflected-load propagation, declare whether its acoustic power converts retained jet/mean dissipation, with matching heat/work changes, or is explicitly external modeled source work.

**Required exit evidence:** Nozzle-to-noise integration, sonic throat and upstream-state distinctions; matched-state forward/reverse and noise-off controls. Book source work once and reconcile source-off/source-strength controls against closed-loop gas/acoustic energy; check statistical expectations over seeds.

**Stop/reject rule:** A forced primitive Mach value or raw upstream velocity must not stand in for solved throat physics. Fix transfer before tuning source loudness.

**Legacy coverage:** S2e; S3b; D02.

**Starting evidence/specification:** [docs/audio-robustness-research.md](../../docs/audio-robustness-research.md).

**Execution record:** for new work, `docs/stage-053.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-054"></a>
## Stage 054 — Unified pulse and noise routing

**Dependencies:** [044](../../docs/roadmap/04-propagation.md#stage-044), [049](../../docs/roadmap/04-propagation.md#stage-049), [050](../../docs/roadmap/04-propagation.md#stage-050), [051](../../docs/roadmap/04-propagation.md#stage-051), [053](../../docs/roadmap/05-engine-qualification.md#stage-053)

**Deliverable:** Route all exhaust excitation through the same geometry, remaining path, temperature/flow and component transfer. Include actual center/link lengths, X/Y scattering and all outlets; separate resolved pulses from unresolved sources.

**Required exit evidence:** All-source/all-outlet complex response and source-sum tests, loss bypass controls and geometry edits affect both layers consistently. Same-state captures show no duplicated pulse or unconditional catalyst/muffler loss; stochastic source work retains the Stage 053 ownership through reflections and every outlet.

**Stop/reject rule:** Collector-index shortcuts, mirrored opposite-outlet noise or parallel filters that disagree with engine breathing fail the stage.

**Legacy coverage:** S2e; D03-D05.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-054.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-055"></a>
## Stage 055 — Engine-specific structure and accessories

**Dependencies:** [041](../../docs/roadmap/03-contracts-and-sources.md#stage-041), [042](../../docs/roadmap/03-contracts-and-sources.md#stage-042)

**Deliverable:** Research pressure-force, slap, valve, injector, cam-drive, alternator/fan and mount excitation. Fit bounded per-engine block/head/body modes and bank-aware paths with actual event timing.

**Required exit evidence:** Measured/bounded modal responses, order/modulation statistics, event and energy checks; engine-bay/cabin/outside comparisons and repeated-seed consistency. Report fitted constants and uncertainty.

**Stop/reject rule:** Generic bore-scaled modes and arbitrary random detail cannot establish engine-specific realism. Avoid tuning against recording EQ alone.

**Legacy coverage:** S3b; S5 (accessories); D07; R11.

**Starting evidence/specification:** [docs/stage-12b.md](../../docs/stage-12b.md), [docs/audio-robustness-research.md](../../docs/audio-robustness-research.md).

**Execution record:** for new work, `docs/stage-055.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-056"></a>
## Stage 056 — Listener, vehicle and recording-transfer validation

**Dependencies:** [024](../../docs/roadmap/02-existing-sound.md#stage-024), [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037), [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038), [051](../../docs/roadmap/04-propagation.md#stage-051)

**Deliverable:** Validate fractional delays, source/body/cabin coordinates, directivity, reflections and moving histories. Retain all eight documented native stereo scenes, including their close-exhaust, pass-by, cabin and ground/wall/room cases. Use the shared spatial frame and distinguish microphone processing from simulated propagation.

**Required exit evidence:** Analytic delay/Doppler and bounded-history controls, known-position stereo magnitude/phase and cabin/body transfer. Include distance and recording-perspective uncertainty; preserve per-channel power.

**Stop/reject rule:** No inference of exact microphone position from brightness; no wrapped/out-of-range history or unexplained upper-band loss. Simulated gas paths are not room reverb paths.

**Legacy coverage:** S4; S4b; D09; R12; R13.

**Starting evidence/specification:** [docs/stage-S4.md](../../docs/stage-S4.md), [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-056.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-057"></a>
## Stage 057 — Whole-engine candidate integration

**Dependencies:** [041](../../docs/roadmap/03-contracts-and-sources.md#stage-041), [042](../../docs/roadmap/03-contracts-and-sources.md#stage-042), [043](../../docs/roadmap/04-propagation.md#stage-043), [044](../../docs/roadmap/04-propagation.md#stage-044), [045](../../docs/roadmap/04-propagation.md#stage-045), [046](../../docs/roadmap/04-propagation.md#stage-046), [047](../../docs/roadmap/04-propagation.md#stage-047), [048](../../docs/roadmap/04-propagation.md#stage-048), [049](../../docs/roadmap/04-propagation.md#stage-049), [050](../../docs/roadmap/04-propagation.md#stage-050), [051](../../docs/roadmap/04-propagation.md#stage-051), [052](../../docs/roadmap/04-propagation.md#stage-052), [054](../../docs/roadmap/05-engine-qualification.md#stage-054), [055](../../docs/roadmap/05-engine-qualification.md#stage-055), [056](../../docs/roadmap/05-engine-qualification.md#stage-056)

**Deliverable:** Integrate each still-viable conservative/coupled candidate behind a controlled research selector into the real five-preset engine. Listed experiment dependencies require reported outcomes; only surviving arms need qualified artifacts. A rejected coupled adapter need not be made to pass before testing a qualified conservative arm, while common causality/component/thermal/flow gates remain mandatory. Keep A0 as the accepted baseline and one physical owner per region.

**Required exit evidence:** Startup/idle/held load/pull/limiter/cut/resume and full long coasts; torque, VE, pumping, mass/species/energy and guard books alongside raw float stems and full sound. Verify geometry edits return their load upstream.

**Stop/reject rule:** No candidate replaces production by default. If only one survives earlier gates, document the rejected arm rather than fabricate an equal comparison.

**Legacy coverage:** S2d; S2e.

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-057.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-058"></a>
## Stage 058 — Robotic deceleration and lift-off causal diagnosis

**Dependencies:** [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039), [057](../../docs/roadmap/05-engine-qualification.md#stage-057)

**Deliverable:** Test actual ECU lift/cut/resume using matched trajectories and separated pulse/flow/structure layers. Compare frozen temperature, velocity, coefficients and source controls before attributing the I6/flat-V8 boom or robotic texture.

**Required exit evidence:** Real loaded long-coast and neutral-fall references, rise/decay, pitch/orders, crest and modulation; controlled interventions identify or bound competing causes. Publish changed audio at common gain and retain rejected explanations.

**Stop/reject rule:** A brighter render or disappearance of one boom does not identify its cause. Missing decisive references or unexplained artifacts remain open.

**Legacy coverage:** S1e; R01; S1; S2.

**Starting evidence/specification:** [docs/stage-S1e.md](../../docs/stage-S1e.md), [docs/stage-S2c.md](../../docs/stage-S2c.md).

**Execution record:** for new work, `docs/stage-058.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-059"></a>
## Stage 059 — Five-engine physical and acoustic qualification

**Dependencies:** [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039), [057](../../docs/roadmap/05-engine-qualification.md#stage-057), [058](../../docs/roadmap/05-engine-qualification.md#stage-058)

**Deliverable:** Qualify base I4/I6/cross-V8/flat-V8/boxer configurations across startup, idle, loaded speeds, high RPM, neutral revs, limiter and eight-second loaded coasts. Record modified-engine assumptions and unsafe/unvalidated ranges.

**Required exit evidence:** Matched dyno/VE/backpressure/combustion maps, multiple-seed COV/repetition and per-layer/order/envelope statistics against valid references. Freeze a supported operating envelope with explicit uncertainty and unresolved mechanisms.

**Stop/reject rule:** Raised rev limits are not validated durability. Unknown reference hardware cannot certify an exact engine model; passing one trajectory is insufficient.

**Reference entry gate:** Stage039's processing protocol is implemented; matched-recording acquisition remains part of039 and is open. Recheck the [source-search/qualification review](../stage-039-reference-search.md) at execution. Lock claim-appropriate engine/exhaust/state references before fitting; manufacturer scalar data and video metadata alone are not matched audio measurements.

**Legacy coverage:** S1; S2; R02-R05; R14.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md), [docs/stage-13b-r-rollback.md](../../docs/stage-13b-r-rollback.md).

**Execution record:** for new work, `docs/stage-059.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-060"></a>
## Stage 060 — Renderer comparison and architecture decision

**Dependencies:** [043](../../docs/roadmap/04-propagation.md#stage-043), [057](../../docs/roadmap/05-engine-qualification.md#stage-057), [059](../../docs/roadmap/05-engine-qualification.md#stage-059)

**Deliverable:** Compare A0, improved conservative and coupled candidates at equal demonstrated fidelity and operating envelope, including complete retained inventories and source/scene cost. Choose a candidate/component combination, or explicitly reject all current replacements.

**Required exit evidence:** Predeclared accuracy, nonlinear/transient/thermal tests, full-engine evidence, exploratory listening and measured no-trace native/WASM costs. Publish the decision and rejected data in an ADR; state what the winner actually supports.

**Stop/reject rule:** No promotion from static graph speed, brightness or a paper score. A no-winner outcome blocks production promotion and creates a new bounded experiment stage.

**Legacy coverage:** S2d (decision).

**Starting evidence/specification:** [docs/adr/0001-coupled-exhaust-ownership.md](../../docs/adr/0001-coupled-exhaust-ownership.md), [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-060.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-061"></a>
## Stage 061 — Production renderer promotion and API integration

**Dependencies:** [060](../../docs/roadmap/05-engine-qualification.md#stage-060)

**Deliverable:** Promote only the selected, qualified path into production core/native/WASM interfaces. Maintain configuration/state ownership, defined fallback/rollback and existing accepted presets; retire redundant propagation only where replaced.

**Required exit evidence:** Production controls reproduce qualified candidate states/audio/loads; complete core/native/Python/WASM parity suites and deliberate integration faults pass. Publish changed engine renders and migration/rollback evidence.

**Stop/reject rule:** Experimental objects being linked into a probe is not production integration. Never sum old and new copies of the same physical pulse.

**Legacy coverage:** S2d; S2e (production).

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-061.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-062"></a>
## Stage 062 — Fidelity tiers, optimization and native/WASM parity

**Dependencies:** [061](../../docs/roadmap/05-engine-qualification.md#stage-061), [059](../../docs/roadmap/05-engine-qualification.md#stage-059)

**Deliverable:** Profile the validated full implementation; optimize preparation, storage, SIMD/SoA and scheduling where measurements justify it. Define each tier, hardware demand, transition behavior and preserved audible band.

**Required exit evidence:** No-tap/no-file native and WASM costs, state/audio parity, bounded memory/rebuild costs and repeated stress runs. Recheck all physics/audio gates after optimization. Keep old 4x-native/2x-WASM goals explicitly unmet until demonstrated or revised with Bernardo.

**Stop/reject rule:** Below-real-time correctness prototypes do not qualify the product. No silent physics-quality reduction; measured realism benefit must justify cost. Actual worklet deadlines are still Stage 069.

**Legacy coverage:** 13b; 13b-c; R17.

**Starting evidence/specification:** [docs/stage-13b.md](../../docs/stage-13b.md), [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-062.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-063"></a>
## Stage 063 — Base-engine sound and realism acceptance

**Dependencies:** [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039), [059](../../docs/roadmap/05-engine-qualification.md#stage-059), [061](../../docs/roadmap/05-engine-qualification.md#stage-061), [062](../../docs/roadmap/05-engine-qualification.md#stage-062)

**Deliverable:** Publish one complete RESULTS.html containing current five-engine examples, meaningful controls and all long coasts. Review idle, high RPM, neutral fall and loaded overrun with Bernardo and real references.

**Required exit evidence:** Numerical findings resolved or explicitly scoped; repeated hidden controls, common-gain causal and separately level-matched timbre auditions. Record Bernardo's acceptance per preset/scenario and any follow-up stage.

**Stop/reject rule:** Do not mark legacy S1/S1e/S2/S4 or full13b closed merely because renders exist. Listening cannot waive a failed conservation or runtime requirement.

**Reference entry gate:** Apply the [039 claim limits](../stage-039-review.md). Qualitative cabin/pass-by auditions cannot certify absolute SPL, a matched FB20, or a known-load response. Missing decisive evidence keeps the corresponding qualification/acceptance claim open.

**Legacy coverage:** S1; S1e; S2; S4 (base acceptance).

**Starting evidence/specification:** [docs/render-workflow.md](../../docs/render-workflow.md), [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-063.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
