# Optional expansions

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-103"></a>
## Stage 103 — Gearbox and vehicle-driving mode

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: research and couple clutch/gears, drivetrain inertia, road load and vehicle motion. Update engine/exhaust load, listener trajectories, controls and visuals through the same contracts.

**Required exit evidence:** Energy/load/shift/overrun fixtures, native driving cycle and browser/scene/3D runtime/listening regression; explicit vehicle assumptions and tire/drag limits.

**Stop/reject rule:** Do not fake shifts with an RPM/audio sweep unrelated to engine/drivetrain work.

**Legacy coverage:** 26 (gearbox/vehicle).

**Execution record:** for new work, `docs/stage-103.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-104"></a>
## Stage 104 — WAV recording and audio export

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: capture the actual selected stereo output or documented raw measurement stream with consistent sample-rate/channel/gain metadata, without file operations in the callback.

**Required exit evidence:** Frame continuity, long recording, clipping/nonfinite detection, container round-trip, memory/backpressure and main audio deadline checks.

**Stop/reject rule:** A listening-normalized export must not be presented as raw calibrated measurement audio.

**Legacy coverage:** 26 (WAV export).

**Execution record:** for new work, `docs/stage-104.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-105"></a>
## Stage 105 — Two-stroke engine cycle

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: research port timing, scavenging, trapped charge, lubrication/combustion assumptions and tuned-exhaust coupling as a distinct physical cycle; adapt UI/3D/events.

**Required exit evidence:** Independent cycle/scavenging/energy/port and expansion-chamber controls, reference-backed engine maps/sound and complete product regression.

**Stop/reject rule:** Changing a four-stroke firing multiplier does not model a two-stroke.

**Legacy coverage:** 26 (two-stroke).

**Execution record:** for new work, `docs/stage-105.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-106"></a>
## Stage 106 — Alternative fuels

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: implement researched E85/methanol and mixture thermochemistry, fueling, evaporation/knock/combustion limits with versioned composition and supported maps.

**Required exit evidence:** Species/energy, stoichiometry, fuel/knock/thermal sensitivity and per-fuel dyno/sound regressions; do not assume gasoline calibration transfers.

**Stop/reject rule:** A fuel label or power multiplier is not a physical fuel model.

**Legacy coverage:** 26 (fuels).

**Execution record:** for new work, `docs/stage-106.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-107"></a>
## Stage 107 — Supercharger system

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: research compressor map, mechanically driven shaft/work, bypass and thermal topology; add causal sound, controls, geometry and save/share extension.

**Required exit evidence:** Engine/shaft energy, map-domain and transient controls plus native/browser/audio/3D qualification and hardware costs.

**Stop/reject rule:** Do not reuse exhaust-driven turbo spool dynamics for a belt-driven compressor.

**Legacy coverage:** 26 (supercharger).

**Execution record:** for new work, `docs/stage-107.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-108"></a>
## Stage 108 — Individual throttle bodies

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: model separate throttle/runner/plenum paths, controller/balance behavior and intake acoustics; update geometry and tuning metadata.

**Required exit evidence:** Per-cylinder air balance, throttle steps, idle stability, order/transfer and native/browser/3D regressions with research-backed references.

**Stop/reject rule:** A shared throttle parameter or louder intake filter alone does not represent ITBs.

**Legacy coverage:** 26 (ITBs).

**Execution record:** for new work, `docs/stage-108.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-109"></a>
## Stage 109 — Anti-lag system

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102), [084](../../docs/roadmap/09-turbo.md#stage-084), [085](../../docs/roadmap/10-limits.md#stage-085), [087](../../docs/roadmap/10-limits.md#stage-087)

**Deliverable:** Optional: research actual fueling/ignition/air/turbine-energy and thermal mechanisms plus modeled safety limits; define supported afterfire chemistry before audible effects.

**Required exit evidence:** Fuel/energy/temperature/shaft and cut/resume controls, realistic turbine response and reference-backed sound; reopen turbo/knock/damage/runtime qualifications.

**Stop/reject rule:** Random exhaust bangs are not anti-lag physics and must not be substituted for missing chemistry.

**Legacy coverage:** 26 (anti-lag).

**Execution record:** for new work, `docs/stage-109.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-110"></a>
## Stage 110 — 3D garage or dyno-room environment

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: add the environment, lighting and camera experience while using a validated compatible acoustic room/scene model when it changes the listening setting.

**Required exit evidence:** Visual/audio placement consistency, reflections/room-response evidence where claimed, accessibility and combined GPU/audio performance.

**Stop/reject rule:** A room image does not validate room acoustics; retain the existing bench/no-3D fallback.

**Legacy coverage:** 26 (garage/room).

**Execution record:** for new work, `docs/stage-110.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-111"></a>
## Stage 111 — Synchronized video capture

**Dependencies:** [102](../../docs/roadmap/12-release.md#stage-102)

**Deliverable:** Optional: export engine visualization with actual audio using documented timestamps, sample/frame rates, encoder support and bounded buffers.

**Required exit evidence:** A/V drift, dropped frames, long capture, stop/reset, export playback and live callback/graphics impact; disclose capability and format limits.

**Stop/reject rule:** Do not assemble unrelated audio and animation timelines or degrade the live physics silently.

**Legacy coverage:** 26 (video).

**Execution record:** for new work, `docs/stage-111.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
