# Persistence, final acceptance and release

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-095"></a>
## Stage 095 — Versioned local build persistence

**Dependencies:** [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037), [067](../../docs/roadmap/07-browser.md#stage-067), [082](../../docs/roadmap/09-turbo.md#stage-082), [088](../../docs/roadmap/10-limits.md#stage-088), [094](../../docs/roadmap/11-engine-catalog.md#stage-094)

**Deliverable:** Save canonical configuration, preset revision, tuning, scene and quality locally; load atomically with version migration and validated ranges/topology. Distinguish a saved build from a live simulation snapshot or recorded session.

**Required exit evidence:** Round-trip all supported layouts/features; old/unknown versions, malformed data, interrupted storage and reset/default cases tested. Loading does not cause one rebuild per field or resurrect stale physical state.

**Stop/reject rule:** Unstable numeric pipe IDs cannot be the sole saved identity; no silent substitution for an unsupported exhaust.

**Legacy coverage:** 24 (save).

**Starting evidence/specification:** [GLOSSARY.md](../../GLOSSARY.md).

**Execution record:** for new work, `docs/stage-095.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-096"></a>
## Stage 096 — Shareable builds and validated imports

**Dependencies:** [095](../../docs/roadmap/12-release.md#stage-095)

**Deliverable:** Encode shareable static URL builds and a portable configuration representation with schema/version/size limits. Provide explicit useful errors and coherent core-side validation.

**Required exit evidence:** Link round-trips on a clean second browser, URL size/encoding limits, hostile/malformed/unknown data and migration. Same build produces the same canonical descriptor and deterministic seeded baseline.

**Stop/reject rule:** Do not embed local reference recordings, private file paths or full runtime histories in a shared build.

**Legacy coverage:** 24 (share).

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md).

**Execution record:** for new work, `docs/stage-096.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-097"></a>
## Stage 097 — Usability, accessibility and visual polish

**Dependencies:** [067](../../docs/roadmap/07-browser.md#stage-067), [068](../../docs/roadmap/07-browser.md#stage-068), [070](../../docs/roadmap/08-3d.md#stage-070), [077](../../docs/roadmap/08-3d.md#stage-077), [082](../../docs/roadmap/09-turbo.md#stage-082), [089](../../docs/roadmap/10-limits.md#stage-089), [094](../../docs/roadmap/11-engine-catalog.md#stage-094), [096](../../docs/roadmap/12-release.md#stage-096)

**Deliverable:** Finish layout/theme, onboarding, keyboard/metric-imperial controls, front/side/cutaway/follow-exhaust camera presets, reduced motion, accessible legends and mobile-friendly operation. Explain quality and unsupported hardware; prefer visual reduction before any explicit physics-tier change.

**Required exit evidence:** A new user can start and understand the bench within the original 30-second target in observed trials; keyboard, focus, text alternatives, touch/layout and no-3D fallback checked. Bernardo approves appearance and interaction.

**Stop/reject rule:** Mobile-friendly controls do not promise real-time full simulation on every phone. No hidden automatic sound-fidelity downgrade.

**Legacy coverage:** 24 (UX).

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md).

**Execution record:** for new work, `docs/stage-097.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-098"></a>
## Stage 098 — Final per-preset listening and realism acceptance

**Dependencies:** [039](../../docs/roadmap/03-contracts-and-sources.md#stage-039), [063](../../docs/roadmap/05-engine-qualification.md#stage-063), [084](../../docs/roadmap/09-turbo.md#stage-084), [089](../../docs/roadmap/10-limits.md#stage-089), [094](../../docs/roadmap/11-engine-catalog.md#stage-094), [097](../../docs/roadmap/12-release.md#stage-097)

**Deliverable:** Review the complete catalog in the browser and consolidated RESULTS.html, including turbo and all supported listeners, layers, limiters and long coasts. Tune only physically justified/calibrated defaults and revisit changed qualifications.

**Required exit evidence:** Record Bernardo's per-preset/scenario verdict, matched-reference evidence and repeated listening controls. Complete relevant S1/S1e/S2/S4/S5 obligations explicitly; retain unresolved findings rather than averaging them away.

**Stop/reject rule:** Final listening cannot waive a failed physical, safety-envelope or runtime gate; late tuning requires rerunning affected tests.

**Reference entry gate:** Check current claim-specific coverage against the [039 source-search review](../stage-039-reference-search.md) and later qualification records. An implemented reference protocol does not close matched-recording acquisition, real-engine calibration, recording rights, or the per-preset listening gates.

**Legacy coverage:** S5; S1/S1e/S2/S4 (final).

**Starting evidence/specification:** [docs/render-workflow.md](../../docs/render-workflow.md).

**Execution record:** for new work, `docs/stage-098.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-099"></a>
## Stage 099 — Complete-browser and hardware release qualification

**Dependencies:** [077](../../docs/roadmap/08-3d.md#stage-077), [084](../../docs/roadmap/09-turbo.md#stage-084), [089](../../docs/roadmap/10-limits.md#stage-089), [094](../../docs/roadmap/11-engine-catalog.md#stage-094), [096](../../docs/roadmap/12-release.md#stage-096), [097](../../docs/roadmap/12-release.md#stage-097), [098](../../docs/roadmap/12-release.md#stage-098)

**Deliverable:** Qualify the entire supported product on named hardware and Chrome/Firefox versions with full 3D, turbo, damage, controls and sharing. Measure worst/tail frame and callback behavior, memory and lifecycle robustness.

**Required exit evidence:** Declared long-session/stress matrix with zero observed underruns/deadline misses, bounded memory and measured supported frame/latency targets; dev/production e2e and native/WASM state/audio parity. Explicitly settle remaining legacy 4x/2x goals with Bernardo if unachieved.

**Stop/reject rule:** No claim of universal device support or glitch freedom from a finite run. Failed targets require a bounded fix or explicit scope/hardware decision before release.

**Legacy coverage:** 13b-c; 13b; 14; 19; 24; R17.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** for new work, `docs/stage-099.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-100"></a>
## Stage 100 — Release documentation, evidence and asset review

**Dependencies:** [099](../../docs/roadmap/12-release.md#stage-099)

**Deliverable:** Write the actual equations/limits/how-it-works guide, supported hardware/browser instructions, screenshots, build/share guidance and stage-evidence index. Check licenses/redistribution for code and any public assets; local user recordings stay ignored.

**Required exit evidence:** Clean checkout builds from documented pinned dependencies; docs links, commands, WASM/source consistency and public asset inventory verified. Separate historical experiments from shipped claims and identify remaining optional/deferred work.

**Stop/reject rule:** Do not publish an unverified realism claim, copyrighted local reference clip or stale test count as release evidence.

**Legacy coverage:** 25 (documentation).

**Starting evidence/specification:** [README.md](../../README.md), [RESEARCH.md](../../RESEARCH.md).

**Execution record:** for new work, `docs/stage-100.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-101"></a>
## Stage 101 — Static production deployment

**Dependencies:** [100](../../docs/roadmap/12-release.md#stage-100)

**Deliverable:** Prepare and publish the static web build on the intended Vercel static target, unless explicitly revised, with correct WASM MIME/cache/version handling, browser audio activation and production configuration.

**Required exit evidence:** Prepare the reviewable production build first; verify publication authorization from the actual execution request/session and ask only if it is absent, then verify the actual URL, assets, WASM updates, share links and rollback path. No secrets or local references in the bundle.

**Stop/reject rule:** This planning request does not authorize deployment now. A successful local preview is not a deployed acceptance result.

**Legacy coverage:** 25 (deployment).

**Starting evidence/specification:** [docs/workspace.md](../../docs/workspace.md).

**Execution record:** for new work, `docs/stage-101.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.

<a id="stage-102"></a>
## Stage 102 — Deployed second-device acceptance and full release

**Dependencies:** [101](../../docs/roadmap/12-release.md#stage-101)

**Deliverable:** Run the real public deployment on Bernardo's supported machine and a second named device/browser. Complete public README/demo/portfolio linkage with an explicit supported envelope and final acceptance record.

**Required exit evidence:** Start/play/control/dyno/3D/save/share/turbo/limits work at the deployed URL; check cold/warm cache and updated WASM compatibility, listening, performance and clean-device onboarding.

**Stop/reject rule:** A deployment URL alone does not complete the project. Open mandatory feature, physics, listening or hardware gates block full-release status.

**Legacy coverage:** 25 (launch).

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md).

**Execution record:** for new work, `docs/stage-102.md` plus the protocol's source/command/measurement/decision record. Historical rows use their existing linked records. No stage closes from a planned test alone.
