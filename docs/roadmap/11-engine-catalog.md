# Expanded engine and exhaust catalog

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-090"></a>
## Stage 090 — Additional inline and single-cylinder engines

**Dependencies:** [009](../../docs/roadmap/01-foundations.md#stage-009), [071](../../docs/roadmap/08-3d.md#stage-071), [073](../../docs/roadmap/08-3d.md#stage-073), [087](../../docs/roadmap/10-limits.md#stage-087)

**Deliverable:** Add I3, I5 and motorcycle-single configurations with their own dimensions, firing/valve data, suitable exhaust layouts and referenced or explicitly generic calibration assumptions.

**Required exit evidence:** Each starts/idles, passes kinematics/events/dyno sanity and native/browser/audio/3D checks over its supported envelope. Treat each preset separately in the evidence and split this stage if implementation exceeds one bounded checkpoint.

**Stop/reject rule:** Core cylinder-count support is not a complete engine product. No copied exhaust calibration without physical justification.

**Legacy coverage:** 23 (inline/single).

**Starting evidence/specification:** [docs/stage-08.md](../../docs/stage-08.md).

**Execution record:** for new work, `docs/stage-090.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-091"></a>
## Stage 091 — 60-degree and 90-degree V6 engines

**Dependencies:** [009](../../docs/roadmap/01-foundations.md#stage-009), [071](../../docs/roadmap/08-3d.md#stage-071), [073](../../docs/roadmap/08-3d.md#stage-073), [087](../../docs/roadmap/10-limits.md#stage-087)

**Deliverable:** Implement both V6 bank/crank/firing variants and model-specific intake/exhaust configuration with consistent spatial and acoustic identity.

**Required exit evidence:** Independent firing/kinematics, startup/idle, dyno, source routing, stereo and visual checks for each variant. Document even/odd firing assumptions and reference gaps.

**Stop/reject rule:** A rotated V8 mesh or generic six-cylinder sound does not validate a V6.

**Legacy coverage:** 23 (V6).

**Starting evidence/specification:** [docs/stage-08.md](../../docs/stage-08.md).

**Execution record:** for new work, `docs/stage-091.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-092"></a>
## Stage 092 — V10 and V12 engines

**Dependencies:** [009](../../docs/roadmap/01-foundations.md#stage-009), [071](../../docs/roadmap/08-3d.md#stage-071), [073](../../docs/roadmap/08-3d.md#stage-073), [087](../../docs/roadmap/10-limits.md#stage-087)

**Deliverable:** Add researched or explicitly generic V10/V12 bank/pin/firing and full exhaust descriptions. Prepare larger engine capacities and runtime/geometry budgets explicitly.

**Required exit evidence:** Per-layout event, conservation, dyno, sound, native/WASM capacity and combined 3D/runtime qualification; stable IDs and snapshots at higher cylinder counts.

**Stop/reject rule:** No silent capacity truncation, missing source branch or duplicated bank audio.

**Legacy coverage:** 23 (V10/V12).

**Starting evidence/specification:** [docs/stage-08.md](../../docs/stage-08.md).

**Execution record:** for new work, `docs/stage-092.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-093"></a>
## Stage 093 — Boxer-six engine

**Dependencies:** [009](../../docs/roadmap/01-foundations.md#stage-009), [071](../../docs/roadmap/08-3d.md#stage-071), [073](../../docs/roadmap/08-3d.md#stage-073), [087](../../docs/roadmap/10-limits.md#stage-087)

**Deliverable:** Add opposed-cylinder crank, layout, intake/exhaust and distinctive supported calibration with the same physical/visual descriptor.

**Required exit evidence:** Opposed-piston phase and rod transforms, firing/valve events, start/idle/dyno and audio/3D checks; verify exhaust equality/inequality effects rather than invent a generic boxer rumble.

**Stop/reject rule:** Opposed appearance alone does not certify crank/firing or sound behavior.

**Legacy coverage:** 23 (boxer-6).

**Starting evidence/specification:** [docs/stage-08.md](../../docs/stage-08.md).

**Execution record:** for new work, `docs/stage-093.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-094"></a>
## Stage 094 — Complete engine-specific exhaust and catalog calibration

**Dependencies:** [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039), [084](../../docs/roadmap/09-turbo.md#stage-084), [086](../../docs/roadmap/10-limits.md#stage-086), [087](../../docs/roadmap/10-limits.md#stage-087), [089](../../docs/roadmap/10-limits.md#stage-089), [090](../../docs/roadmap/11-engine-catalog.md#stage-090), [091](../../docs/roadmap/11-engine-catalog.md#stage-091), [092](../../docs/roadmap/11-engine-catalog.md#stage-092), [093](../../docs/roadmap/11-engine-catalog.md#stage-093)

**Deliverable:** Give every supported base and added engine a documented complete real-system-inspired exhaust with published versus estimated dimensions, source intervals and model validity. Calibrate each default using its own torque, event, acoustic and visual evidence. Deliver at least two additional generically named inspired-by configurations with their own documented engine/exhaust assumptions, rather than only relabeling the added layouts.

**Required exit evidence:** Per-engine test/recording matrix, all supported NA/turbo variants, orders/COV/response, known-position or uncertain-reference distinctions, native/browser/3D and runtime. Generic inspired builds are labeled; unsupported combinations do not inherit certification.

**Stop/reject rule:** One successful V8 or one parameter sweep cannot qualify the catalog. Missing reference hardware remains an explicit gap and may require Bernardo's recordings.

**Reference entry gate:** Reopen acquisition where the [039 coverage matrix](../stage-039-review.md) is insufficient. A related FA24 cabin recording does not qualify FB20 exhaust response; partial hardware descriptions do not qualify a complete exhaust model. Preserve the locked development/validation history.

**Legacy coverage:** 23; 13b-p; S5 (catalog).

**Starting evidence/specification:** [docs/user-sound-references.md](../../docs/user-sound-references.md), [docs/stage-13b-p.md](../../docs/stage-13b-p.md).

**Execution record:** for new work, `docs/stage-094.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
