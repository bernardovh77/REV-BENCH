# Turbocharged engine development

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-078"></a>
## Stage 078 — Turbo compressor and turbine map components

**Dependencies:** [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037), [061](../../docs/roadmap/05-engine-qualification.md#stage-061), [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039)

**Deliverable:** Research and implement corrected-speed/flow maps, efficiency, pressure ratio, surge/choke domain and declared size scaling for compressor/turbine components. Record map provenance and uncertainty.

**Required exit evidence:** Independent map interpolation/domain, mass/energy/enthalpy and zero/edge-flow controls; validate size scaling over a declared range before coupling to the engine.

**Stop/reject rule:** A turbo-size multiplier without defensible map physics does not qualify. Out-of-map behavior must be explicit.

**Legacy coverage:** 20 (maps).

**Starting evidence/specification:** [RESEARCH.md](../../RESEARCH.md).

**Execution record:** for new work, `docs/stage-078.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-079"></a>
## Stage 079 — Turbo shaft and engine gas-system integration

**Dependencies:** [078](../../docs/roadmap/09-turbo.md#stage-078)

**Deliverable:** Couple compressor/turbine work, shaft inertia and gas volumes into the physical engine/exhaust topology. Preserve reflected load, mean inventory and source ownership with turbo on/off.

**Required exit evidence:** Closed shaft/gas/heat energy and mass books, spool transients, startup/lift and native dyno controls. A larger valid turbo moves response in a physically supported way, not by an audio pitch knob.

**Stop/reject rule:** No prescribed shaft speed or boost curve may replace the coupled energy balance. Reopen affected propagation validity limits.

**Legacy coverage:** 20 (shaft/system).

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md).

**Execution record:** for new work, `docs/stage-079.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-080"></a>
## Stage 080 — Boost, wastegate, intercooler and blow-off control

**Dependencies:** [079](../../docs/roadmap/09-turbo.md#stage-079)

**Deliverable:** Add wastegate/boost target, intercooler heat/pressure loss, blow-off valve and ECU interactions with bounded actuators and explicit surge/choke behavior.

**Required exit evidence:** Throttle steps, lift/reapply, limits, target changes and valve-disable controls; map-domain/thermal/energy books and stable-control tests across supported sizes. Publish failure and unsupported regimes.

**Stop/reject rule:** Avoid fantasy boost/power outside credible thermal or map domains. Valve chatter needs a modeled cause.

**Legacy coverage:** 20 (controls).

**Starting evidence/specification:** [RESEARCH.md](../../RESEARCH.md).

**Execution record:** for new work, `docs/stage-080.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-081"></a>
## Stage 081 — Turbo acoustic sources and turbine propagation

**Dependencies:** [080](../../docs/roadmap/09-turbo.md#stage-080), [054](../../docs/roadmap/05-engine-qualification.md#stage-054), [055](../../docs/roadmap/05-engine-qualification.md#stage-055), [051](../../docs/roadmap/04-propagation.md#stage-051)

**Deliverable:** Research shaft/blade-pass and broadband noise, turbine pulse loading/attenuation, BOV/wastegate flow and supported surge acoustics. Route sources through the modified physical geometry.

**Required exit evidence:** Shaft-order, event, transfer/load, lift and control-ablation fixtures; matched turbo references and whole-engine float stems. Show no double-counted exhaust pulse and correct turbo-off baseline.

**Stop/reject rule:** No generic pssh/chatter sample or arbitrary whine can prove turbine acoustics. Numerical and listening gates remain distinct.

**Legacy coverage:** 21 (sound).

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-081.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-082"></a>
## Stage 082 — Turbo controls, instrumentation and build integration

**Dependencies:** [067](../../docs/roadmap/07-browser.md#stage-067), [068](../../docs/roadmap/07-browser.md#stage-068), [080](../../docs/roadmap/09-turbo.md#stage-080), [081](../../docs/roadmap/09-turbo.md#stage-081)

**Deliverable:** Expose turbo enable/size/boost/BOV controls, boost/shaft/temperature telemetry and coherent schema/rebuild updates using the shared core. Integrate dyno comparisons and unsupported-range feedback.

**Required exit evidence:** Native/browser commands and telemetry agree, rapid changes/reset remain valid, and saved-build schema extension is tested before persistence UI. Turbo sound tracks actual operating state.

**Stop/reject rule:** UI settings must not fork map physics or silently reset unrelated engine tuning.

**Legacy coverage:** 21 (UI).

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md).

**Execution record:** for new work, `docs/stage-082.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-083"></a>
## Stage 083 — Turbo geometry, shaft motion and valve visualization

**Dependencies:** [073](../../docs/roadmap/08-3d.md#stage-073), [075](../../docs/roadmap/08-3d.md#stage-075), [082](../../docs/roadmap/09-turbo.md#stage-082)

**Deliverable:** Generate the turbo and connections from shared geometry; animate compressor/turbine shaft and wastegate/BOV from actual state, using blur/alias-aware presentation at high speed.

**Required exit evidence:** Wheel/valve transforms and geometry generation match core snapshots; size/turbo-off changes rebuild correctly. Audio, boost and visible spool/lift timing agree within the declared presentation budget.

**Stop/reject rule:** An attractive spinning model is not a shaft-speed measurement; unsupported packaging detail stays illustrative.

**Legacy coverage:** 21 (3D).

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md).

**Execution record:** for new work, `docs/stage-083.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-084"></a>
## Stage 084 — Turbo engine, audio, visual and runtime qualification

**Dependencies:** [077](../../docs/roadmap/08-3d.md#stage-077), [080](../../docs/roadmap/09-turbo.md#stage-080), [081](../../docs/roadmap/09-turbo.md#stage-081), [082](../../docs/roadmap/09-turbo.md#stage-082), [083](../../docs/roadmap/09-turbo.md#stage-083)

**Deliverable:** Repeat physical, sound, control, native/WASM and combined browser/3D qualification for supported boosted builds. Publish pull, spool, lift/reapply, neutral and long-coast cases.

**Required exit evidence:** Energy/maps/thermal/load limits pass; Bernardo accepts turbo character; actual supported-browser runtime and visual/audio synchronization pass. Record the expanded supported envelope and remaining data uncertainty.

**Stop/reject rule:** Base naturally aspirated acceptance does not certify boosted conditions. Do not expand default boost on an untested hardware tier.

**Legacy coverage:** 20-21 (acceptance).

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-084.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
