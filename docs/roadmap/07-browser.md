# Playable browser bench

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-118"></a>
## Stage 118 — A real-time tier for today's engine

**Dependencies:** [119](../../docs/roadmap/14-review-fixes.md#stage-119)

**Deliverable:** Profile the production engine (A0, with its noise and listener) natively and as
WebAssembly. Speed it up without changing what it computes, where measurements point. Declare a
quality tier that runs every preset at least 1.5 times real time as WebAssembly on Bernardo's
desktop baseline, and measure what that tier changes: torque, VE and the sound measures against
the default quality (`tools/compare_real.py`, `tools/orders.py`).

**Required exit evidence:**
- Interleaved native and WebAssembly timings per preset and tier, best of several runs.
- Renders at the default quality identical, or within stated tolerances, before and after the
  speed-ups.
- The tier's differences measured and listened to.
- All suites green.

**Stop/reject rule:** No silent quality loss: a tier states what it changes. Below-real-time
correctness prototypes don't count. The promoted renderer's own tiers stay stage 062.

**Why it's here (2026-11-01):** the browser stages 066–069 now come before the sound-physics
chain 050–063, so Bernardo can play the engine and hear each sound improvement live. They need
today's engine to run in real time first.

**Execution record:** `docs/stage-118.md`.

<a id="stage-066"></a>
## Stage 066 — Playable browser engine and audio lifecycle

**Dependencies:** [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037), [118](#stage-118). Until 2026-11-01 it waited for 061, 062 and 063. Now it plays today's engine, and 061/062 bring their renderer into the browser when they land.

**Deliverable:** Select the real engine source through the shared API; deliver ignition, starter, throttle/load and preset commands with acknowledgments. Implement actual stereo worklet output, minimal engine telemetry and user-gesture/suspend/resume/device-rate/dispose lifecycle.

**Required exit evidence:** Seeded native-versus-actual-worklet timelines match within declared numerical/audio budgets at supported sample rates and blocks. Start, idle, blip, lift, stop, restart, invalid commands and genuine stereo all work in dev/production builds.

**Stop/reject rule:** Reuse existing TS engine/pipe/stereo bindings. A tone, copied mono channel or WASM microbenchmark is not engine application acceptance.

**Legacy coverage:** 14.

**Starting evidence/specification:** [docs/stage-02.md](../../docs/stage-02.md), [docs/review-acoustic-architecture-2026-10-23.md](../../docs/review-acoustic-architecture-2026-10-23.md).

**Execution record:** for new work, `docs/stage-066.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-067"></a>
## Stage 067 — Engine, intake and exhaust control panel

**Dependencies:** [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037), [066](../../docs/roadmap/07-browser.md#stage-066)

**Deliverable:** Expose presets, bore/stroke/displacement, compression, cams, intake runners, headers, mufflers, AFR, spark-map offset, limits, volume and fidelity from core metadata. Add units, listener/scene selection and per-layer audition controls, plus validated atomic live/rebuild operations with explicit reset/crossfade semantics.

**Required exit evidence:** Each control has a measured physical or audible effect, valid range, reversible state and coherent geometry generation. Rapid mixed edits, stale commands and failed multi-field loads cannot leave half-applied builds or unbounded double rendering. Verify real channels, listener histories and reset behavior when changing scenes/layers; audition controls remain distinct from physical tuning.

**Stop/reject rule:** JavaScript must not fork the physics descriptor or claim every accepted slider combination is physically credible.

**Legacy coverage:** 15.

**Starting evidence/specification:** [CLAUDE.md](../../CLAUDE.md).

**Execution record:** for new work, `docs/stage-067.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-068"></a>
## Stage 068 — Live instruments, engine telemetry and native-owned dyno

**Dependencies:** [066](../../docs/roadmap/07-browser.md#stage-066), [067](../../docs/roadmap/07-browser.md#stage-067)

**Deliverable:** Add RPM, MAP/boost, AFR, torque/power, ECU cuts, output-meter ballistics and CPU/underrun status. Expose the shared native dyno through the simulation-owned API; live sweep, previous-run overlay and explicit units.

**Required exit evidence:** Browser/native curves use identical engine/trajectory and time alignment. Initial proposed gate: <=2% pointwise torque error above 10% of peak, absolute error <=1% of peak below that; freeze tolerance in preflight. Meter and sweep state/reset tests pass.

**Stop/reject rule:** Do not implement a separate JavaScript dyno or derive callback timing from main-thread simulation ratios. Unsupported worklet timing APIs require a tested measurement method.

**Legacy coverage:** 16.

**Starting evidence/specification:** [docs/stage-09.md](../../docs/stage-09.md), [docs/review-acoustic-architecture-2026-10-23.md](../../docs/review-acoustic-architecture-2026-10-23.md).

**Execution record:** for new work, `docs/stage-068.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-069"></a>
## Stage 069 — First playable browser qualification

**Dependencies:** [066](../../docs/roadmap/07-browser.md#stage-066), [067](../../docs/roadmap/07-browser.md#stage-067), [068](../../docs/roadmap/07-browser.md#stage-068), [118](#stage-118). Until 2026-11-01 it also waited for 062 and 063. Base sound acceptance stays 063, and the final listening stays 098.

**Deliverable:** Qualify the actual engine/control/stereo path in Chrome and Firefox on Bernardo's named desktop baseline. Define supported sample rates, lifecycle behavior, tier and command/telemetry budgets.

**Required exit evidence:** Record actual callback tail/max, deadline misses, memory and observed underruns during the declared 15-minute stress trajectory. Manual start/blip/pull/long-coast listening and dev/production e2e pass. Publish the first playable milestone.

**Stop/reject rule:** A p99 statistic alone can hide glitches; missing callbacks or unmeasured timing keeps qualification open. Unsupported devices receive an explicit limitation rather than silently reduced realism.

**Legacy coverage:** 14; 13b-c; R17.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-069.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
