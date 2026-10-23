# Knock, mechanical limits and damage

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-085"></a>
## Stage 085 — End-gas and knock physical model

**Dependencies:** [041](../../docs/roadmap/03-contracts-and-sources.md#stage-041), [080](../../docs/roadmap/09-turbo.md#stage-080)

**Deliverable:** Research an explicit end-gas/octane/temperature/pressure model and its limits before choosing a Livengood-Wu-style or other supported formulation. Add the thermodynamic state the current lumped burn cannot provide.

**Required exit evidence:** Independent ignition-delay/knock-limit controls, fuel/octane/load sweeps and conservation. Calibrate within measured domains; record whether remaining behavior is qualitative or quantitatively supported.

**Stop/reject rule:** A single-zone burn fraction alone cannot establish end-gas knock prediction. Reject unvalidated extreme extrapolation.

**Legacy coverage:** 22 (knock).

**Starting evidence/specification:** [RESEARCH.md](../../RESEARCH.md).

**Execution record:** for new work, `docs/stage-085.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-086"></a>
## Stage 086 — Knock-limited ECU, audible knock and feedback

**Dependencies:** [084](../../docs/roadmap/09-turbo.md#stage-084), [085](../../docs/roadmap/10-limits.md#stage-085), [055](../../docs/roadmap/05-engine-qualification.md#stage-055)

**Deliverable:** Use actual knock state to constrain spark/power and drive bounded structure sound plus warning/telemetry. Keep controlled test configurations and explicit disabled-knock behavior.

**Required exit evidence:** High-load/low-speed and compression/boost/octane cases alter advance and work consistently; no arbitrary random knock clicks. Native/browser, sound and event timelines agree.

**Stop/reject rule:** Warnings and dramatic audio cannot replace a validated cause. Feedback must not create unstable spark/power loops.

**Legacy coverage:** 22 (knock integration).

**Starting evidence/specification:** [docs/stage-06.md](../../docs/stage-06.md), [docs/stage-09.md](../../docs/stage-09.md).

**Execution record:** for new work, `docs/stage-086.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-087"></a>
## Stage 087 — Piston speed, valve float and mechanical limits

**Dependencies:** [086](../../docs/roadmap/10-limits.md#stage-086)

**Deliverable:** Research model-specific valvetrain/rotating-assembly limits and supported modified-engine assumptions. Add justified over-rev/valve-float consequences and safe operating-envelope feedback.

**Required exit evidence:** Per-engine RPM/load/geometry boundary sweeps, no-limit controls and affected torque/valve/event/acoustic regressions. Revisit the 8500/7500/9000-rpm research configurations without silently rewriting accepted sound.

**Stop/reject rule:** A rev-limit slider is not durability proof. No universal cutoff may masquerade as a detailed stress or fatigue simulation.

**Legacy coverage:** 22 (limits); R14.

**Starting evidence/specification:** [docs/stage-13b-r-rollback.md](../../docs/stage-13b-r-rollback.md).

**Execution record:** for new work, `docs/stage-087.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-088"></a>
## Stage 088 — Optional damage state, warnings and reset behavior

**Dependencies:** [086](../../docs/roadmap/10-limits.md#stage-086), [087](../../docs/roadmap/10-limits.md#stage-087)

**Deliverable:** Implement the first-release damage feature as an explicit user-selectable mode with researched failure triggers, bounded degraded/failed state, warnings and reliable reset. Define what is phenomenological versus physically predicted.

**Required exit evidence:** Extreme compression/boost/overrev combinations stop producing fantasy output; state transitions, energy consequences, reset and saved configuration versus dynamic damage are tested.

**Stop/reject rule:** Do not claim a solved fracture/fatigue model or let invalid numerical state become the damage mechanism.

**Legacy coverage:** 22 (damage model).

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md).

**Execution record:** for new work, `docs/stage-088.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-089"></a>
## Stage 089 — Knock and damage presentation effects

**Dependencies:** [076](../../docs/roadmap/08-3d.md#stage-076), [088](../../docs/roadmap/10-limits.md#stage-088)

**Deliverable:** Add event-driven cylinder highlights, heat cues, smoke or thrown-rod illustrations where appropriate. Keep text/status alternatives, reduced motion and audio-state synchronization.

**Required exit evidence:** Every effect has an actual cause/event, correct geometry generation and reset/disposal behavior. Bernardo approves presentation; graphics cannot change physical damage state.

**Stop/reject rule:** Effects are explanatory presentation, not evidence of a CFD fire or structural fracture calculation.

**Legacy coverage:** 22 (3D effects).

**Starting evidence/specification:** [GLOSSARY.md](../../GLOSSARY.md).

**Execution record:** for new work, `docs/stage-089.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
