# Conditional learned-synthesis research

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-064"></a>
## Stage 064 — Standalone learned pulse and DSP-parameter benchmark

**Dependencies:** [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039), [053](../../docs/roadmap/05-engine-qualification.md#stage-053), [054](../../docs/roadmap/05-engine-qualification.md#stage-054), [059](../../docs/roadmap/05-engine-qualification.md#stage-059)

**Deliverable:** Conditional research: compare structured pulse trains and learned order/noise parameter tables deployed as ordinary DSP against the physical baseline. Evaluate neural/GAN/diffusion alternatives only with a specific supported gap and data/bandwidth/cost rationale.

**Required exit evidence:** Held-out real sessions, RPM/load sweeps, fueling/cut and unseen exhaust changes; state usable bandwidth including upper bands, training-source dependence, control correctness and deployed cost. Keep full-waveform comparators outside engine feedback.

**Stop/reject rule:** Not a prerequisite for Stage 066 or first release. If data or benefit is absent, record deferred/rejected status; synthetic reconstruction alone cannot justify deployment.

**Legacy coverage:** S6 (standalone research).

**Starting evidence/specification:** [docs/sound-generation-architecture-review.md](../../docs/sound-generation-architecture-review.md), [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-064.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-065"></a>
## Stage 065 — Conditional learned unresolved-source integration

**Dependencies:** [064](../../docs/roadmap/06-learned-research.md#stage-064), [060](../../docs/roadmap/05-engine-qualification.md#stage-060), [061](../../docs/roadmap/05-engine-qualification.md#stage-061), [063](../../docs/roadmap/05-engine-qualification.md#stage-063)

**Deliverable:** Only for a demonstrated residual gap, integrate bounded state-conditioned unresolved-source parameters through the physical remaining path. Preserve causal events, physical load, energy limits and a conventional fallback outside the validated domain.

**Required exit evidence:** Blind listening benefit on held-out sessions/hardware/trajectories, unseen geometry response, no duplicate combustion pulse, streaming continuity and native/WASM runtime/memory/latency regression. Repeat affected engine acceptance.

**Stop/reject rule:** No optional neural dependency may silently become mandatory. Reject a realistic-sounding model with incorrect controls or unverified exhaust edits.

**Legacy coverage:** S6 (conditional integration).

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-065.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
