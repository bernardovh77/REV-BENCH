# REV-BENCH — canonical project roadmap

Updated 2026-10-23. **One sequence: Stages 001–113.** This file is the only
authority for stage status and scheduling. It replaces the overlapping numeric,
lettered and sound-track numbering. Historical evidence stays at its existing
paths; [every old ID is mapped](docs/roadmap/legacy-map.md), including open parent
acceptance checks. The [old plan is archived](docs/roadmap/legacy-plan.md).

REV-BENCH is a browser engine test bench: build and modify a physical engine,
hear its response, measure it on the dyno and inspect a synchronized procedural
3D engine. **Realism is the absolute priority.** More processing cost is
acceptable when demonstrated realism gains justify it and the required
hardware is stated. Every realism fix needs research and physical evidence.

## Scope and agreed order

Bernardo confirmed on 2026-10-23:

- The first complete release includes the engine/exhaust editor, instruments
  and dyno, full 3D cutaway/motion/gas views, turbocharging, knock/limits/damage,
  expanded engine catalog, save/share, polish and public deployment. Earlier
  playable milestones are useful; they do not shrink that release scope.
- Define shared contracts first, then complete necessary sound-physics and
  runtime gates, then the playable browser bench and 3D. Learned synthesis is
  conditional research, not an automatic prerequisite or a selected renderer.
- Optional vehicle/gearbox, exports, two-stroke, fuels, supercharger, ITBs,
  anti-lag, garage and video are separate later stages.

## Current state and next action

**Current checkpoint: Stage 038 complete.** The [Stage037 review and numerical
observation proof](docs/stage-038.md) fix native contract coercion/metadata loss
and add optional native flux/guard, junction, momentum-impulse, effective geometry/
CFL and bounded-trace diagnostics. Native/WASM/adverse/fault/cost checks pass;
all34 float renders (337.248s) match the accepted audio byte for byte. Numerical
observability does not close later physical accuracy or real-time requirements.

**Next default execution: Stage 039 — matched references and calibration
protocol.** Stage040 causal source work and043/044 propagation/geometry are
ready by dependency; this saved checkpoint does not start them automatically.

The physical native engine and five presets already exist. The browser still
plays the stage-2 tone; low-level TS/WASM engine bindings already exist, but the
worklet/application engine operation is not integrated. No product 3D, turbo
or damage implementation is claimed. The latest bounded open-duct proof
passes its declared numerical checks and remains below real time. It is not
a production-engine-to-B0 adapter. No trained runtime sound model exists.

Accepted sound selection stays: **I6 8500 rpm with its earlier twin exhaust;
flat V8 9000 rpm with its earlier exhaust; cross V8 7500 rpm with the sharper
rear-delete X.** These research limits are not durability certifications.
Preserve the accepted audio, original references, `CLAUDE.md` and `.claude/`.

## How to use the stages

- Say “do Stage 039” or “next stage.” Read this tracker, `CLAUDE.md`, `CODEX.md`
  and the linked stage specification/evidence first.
- Follow explicit dependencies, not just the preceding row. The numbering is
  the planned order; independent reference work and conditional branches are
  identified. IDs are stable from this migration onward. Future splits receive
  the next unused ID and dependencies; never create suffix families or rewrite
  historical IDs. Update the execution order/index when a new prerequisite is added.
- `Complete (bounded)` means the historical deliverable is documented within
  its stated scope; it does not close later realism or release requirements.
  An experiment may be complete while its tested candidate is rejected.
- `Ready` means its declared prerequisites are satisfied, not authorization to
  implement every later stage. `Not started` means planned work; `Conditional`
  and `Optional` stages do not block the default release unless explicitly adopted.
- Every new stage follows [the evidence protocol](docs/roadmap/README.md).
  A future test or engineering threshold is a requirement, never claimed proof.
  No roadmap guarantees zero rework; explicit contracts, independent tests,
  rejection criteria and review checkpoints reduce that risk.

## Mandatory architecture and working rules

One physical engine owns crank, combustion and gas exchange. One canonical
description supplies breathing, acoustic routes, controls, persistence and 3D;
one actual event clock supplies sound and visualization. The active propagation
path owns each physical region and one conservative interface transfer. The
prototype comparison still decides whether conservative, coupled or a justified
component combination is promoted; this roadmap does not preselect the winner.

The intended stack remains a C11 core, native measurement harness, shared WASM
and AudioWorklet, TypeScript browser app and procedural 3D. Specific future
library/backend versions and capability assumptions must be verified at their
execution stage. Native validation precedes dependent browser integration.
Charts, controls and 3D must reuse core descriptions and actual state.

All realism work: primary research → explicit hypothesis/validity range →
independent native fixture → controlled engine comparison → native/WASM and
deployed runtime → Bernardo’s listening/appearance acceptance where relevant.
Keep hypotheses separate from findings, and prototype results separate from
production promotion. Record rejected controls and interventions, not only wins.

Every source-changing stage runs the native, existing Python and web unit
suites plus checks appropriate to its scope; core changes rebuild WASM and
render/compare the affected complete sound checkpoint.
Browser changes validate development and production builds. Physics tests need
targeted deliberate faults. Documentation-only stages check documentation and
preservation; they do not claim freshly rerun physics suites. Full details are
in [the protocol](docs/roadmap/README.md).

Publish one portable `renders/RESULTS.html` at each sound checkpoint, including
current examples and all long coasts. Measurements use unfiltered float stereo;
filtered copies are listening aids. Record source intervals, perspective
uncertainty, processing and gains; brightness is not a microphone position.
Seek additional recordings or ask Bernardo when evidence is insufficient.
Keep generated/reference audio ignored, preserve references and replace only
superseded generated audio after the replacement passes validation.

Query Graphify before code exploration; refresh its AST graph after code
changes. Keep current stage notes, source/evidence hashes, glossary and decisions
up to date. Do not replace Claude’s setup or stage user-added skills incidentally.
Commit/sync completed authorized work; publication/deployment is a separate
execution-stage action with a reviewable build.

## Canonical stage tracker

Click a stage for its deliverable, dependencies, evidence, rejection rule and
execution record. All future stages also inherit the common protocol. The
historical rows are not a claim that all current audio goals have passed.

### 1. Historical foundations and engine core

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [001](docs/roadmap/01-foundations.md#stage-001) | Initial research and native-first project setup | Complete (bounded) | — |
| [002](docs/roadmap/01-foundations.md#stage-002) | Native scaffold, WAV/CSV output and C tests | Complete (bounded) | — |
| [003](docs/roadmap/01-foundations.md#stage-003) | WASM and AudioWorklet tone scaffold | Complete (bounded) | — |
| [004](docs/roadmap/01-foundations.md#stage-004) | Gas properties, chambers and orifice flow | Complete (bounded) | — |
| [005](docs/roadmap/01-foundations.md#stage-005) | Slider-crank and motored cylinder | Complete (bounded) | — |
| [006](docs/roadmap/01-foundations.md#stage-006) | Valvetrain, throttle and intake plenum | Complete (bounded) | — |
| [007](docs/roadmap/01-foundations.md#stage-007) | Baseline combustion and wall heat | Complete (bounded) | — |
| [008](docs/roadmap/01-foundations.md#stage-008) | Free crank, starter, ECU and multicylinder running | Complete (bounded) | — |
| [009](docs/roadmap/01-foundations.md#stage-009) | Five base engine layouts and description API | Complete (bounded) | — |
| [010](docs/roadmap/01-foundations.md#stage-010) | Native absorbing dyno and original engine calibration | Complete (bounded) | — |
| [011](docs/roadmap/01-foundations.md#stage-011) | Original one-dimensional intake and exhaust pipes | Complete (bounded) | — |
| [012](docs/roadmap/01-foundations.md#stage-012) | Acoustic extraction and digital audio output chain | Complete (bounded) | — |
| [013](docs/roadmap/01-foundations.md#stage-013) | Original mufflers, intake path and listener | Complete (bounded) | — |
| [014](docs/roadmap/01-foundations.md#stage-014) | Initial performance, quality levels and stability pass | Complete (bounded) | — |
| [015](docs/roadmap/01-foundations.md#stage-015) | Original flow noise, mechanical sound and cabin | Complete (bounded) | — |
| [016](docs/roadmap/01-foundations.md#stage-016) | Initial variation, limiter and deceleration scenarios | Complete (bounded) | — |

### 2. Existing sound, measurements and bounded prototypes

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [017](docs/roadmap/02-existing-sound.md#stage-017) | Float-audio analysis and Windows paths | Complete (bounded) | — |
| [018](docs/roadmap/02-existing-sound.md#stage-018) | Finite burns stop at exhaust opening | Complete (bounded) | — |
| [019](docs/roadmap/02-existing-sound.md#stage-019) | Five-preset repetition and combustion audit | Complete (bounded) | — |
| [020](docs/roadmap/02-existing-sound.md#stage-020) | Trapped-mixture burn feedback | Complete (bounded) | — |
| [021](docs/roadmap/02-existing-sound.md#stage-021) | Raw user-reference catalog | Complete (bounded) | — |
| [022](docs/roadmap/02-existing-sound.md#stage-022) | Filtered reference listening library | Complete (bounded) | — |
| [023](docs/roadmap/02-existing-sound.md#stage-023) | Loaded exhaust detail and lift-transition correction | Complete (bounded) | — |
| [024](docs/roadmap/02-existing-sound.md#stage-024) | Native stereo scenes implementation | Complete (bounded) | — |
| [025](docs/roadmap/02-existing-sound.md#stage-025) | Complete sound and engine-physics audit | Complete (bounded) | — |
| [026](docs/roadmap/02-existing-sound.md#stage-026) | Batched thermodynamics and SIMD optimization | Complete (bounded) | — |
| [027](docs/roadmap/02-existing-sound.md#stage-027) | Five model-specific exhaust research configurations | Complete (bounded) | — |
| [028](docs/roadmap/02-existing-sound.md#stage-028) | High-RPM configurations and selective rollback | Complete (bounded) | — |
| [029](docs/roadmap/02-existing-sound.md#stage-029) | Independent source, mesh and timestep experiment | Complete (bounded) | — |
| [030](docs/roadmap/02-existing-sound.md#stage-030) | Acoustic architecture comparison specification | Complete (bounded) | — |
| [031](docs/roadmap/02-existing-sound.md#stage-031) | Initial conservative-duct experiment | Complete (bounded) | — |
| [032](docs/roadmap/02-existing-sound.md#stage-032) | Static quiescent acoustic graph fixtures | Complete (bounded) | — |
| [033](docs/roadmap/02-existing-sound.md#stage-033) | Complete results inventory and atomic rebuilding | Complete (bounded) | — |
| [034](docs/roadmap/02-existing-sound.md#stage-034) | Same-tick acoustic receive and commit interface | Complete (bounded) | — |
| [035](docs/roadmap/02-existing-sound.md#stage-035) | Homogeneous open gas/characteristic duct proof | Complete (bounded) | — |

### 3. Roadmap, shared contracts and trustworthy excitation

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [036](docs/roadmap/03-contracts-and-sources.md#stage-036) | Unified roadmap and evidence protocol | Complete | 035 |
| [037](docs/roadmap/03-contracts-and-sources.md#stage-037) | Shared build, geometry, event and snapshot contracts | Complete | 036 |
| [038](docs/roadmap/03-contracts-and-sources.md#stage-038) | Complete numerical guard and flux observability | Complete | 037 |
| [039](docs/roadmap/03-contracts-and-sources.md#stage-039) | Matched references and calibration protocol | Ready | 036 |
| [040](docs/roadmap/03-contracts-and-sources.md#stage-040) | Spark, ignition and flame-development causality | Ready | 038 |
| [041](docs/roadmap/03-contracts-and-sources.md#stage-041) | Continuous combustion variation and residual validity | Not started | 040, 038 |
| [042](docs/roadmap/03-contracts-and-sources.md#stage-042) | Actual fuel-injection events and acoustic triggers | Not started | 040, 038 |

### 4. Propagation prototypes and physical components

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [043](docs/roadmap/04-propagation.md#stage-043) | Improved conservative solver and production-mixture control | Ready | 035, 038 |
| [044](docs/roadmap/04-propagation.md#stage-044) | Executable shared exhaust geometry compiler | Ready | 037, 038 |
| [045](docs/roadmap/04-propagation.md#stage-045) | General zero-flow open gas-to-graph coupling | Not started | 034, 035, 043, 044, 113 |
| [046](docs/roadmap/04-propagation.md#stage-046) | Uniform mean-flow coupling | Not started | 045 |
| [047](docs/roadmap/04-propagation.md#stage-047) | Gas-property and wall-thermal model characterization | Not started | 043, 046 |
| [048](docs/roadmap/04-propagation.md#stage-048) | Thermal fronts, coefficient updates and reverse entrainment | Not started | 046, 047 |
| [049](docs/roadmap/04-propagation.md#stage-049) | Branches, finite junctions and all-port transfer | Not started | 044, 048 |
| [050](docs/roadmap/04-propagation.md#stage-050) | Catalysts, mufflers, perforates and packing | Not started | 047, 049 |
| [051](docs/roadmap/04-propagation.md#stage-051) | Radiation load and audible-field consistency | Not started | 049 |
| [052](docs/roadmap/04-propagation.md#stage-052) | Mixed-rate scheduler and state transitions | Not started | 048, 049, 050, 051 |

### 5. Engine integration, architecture selection and base acceptance

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [053](docs/roadmap/05-engine-qualification.md#stage-053) | Compressible and reversing turbulent-flow excitation | Not started | 040, 042, 047, 038 |
| [054](docs/roadmap/05-engine-qualification.md#stage-054) | Unified pulse and noise routing | Not started | 044, 049, 050, 051, 053 |
| [055](docs/roadmap/05-engine-qualification.md#stage-055) | Engine-specific structure and accessories | Not started | 041, 042 |
| [056](docs/roadmap/05-engine-qualification.md#stage-056) | Listener, vehicle and recording-transfer validation | Not started | 024, 037, 038, 051 |
| [057](docs/roadmap/05-engine-qualification.md#stage-057) | Whole-engine candidate integration | Not started | 041, 042, 043, 044, 045, 046, 047, 048, 049, 050, 051, 052, 054, 055, 056 |
| [058](docs/roadmap/05-engine-qualification.md#stage-058) | Robotic deceleration and lift-off causal diagnosis | Not started | 039, 057 |
| [059](docs/roadmap/05-engine-qualification.md#stage-059) | Five-engine physical and acoustic qualification | Not started | 039, 057, 058 |
| [060](docs/roadmap/05-engine-qualification.md#stage-060) | Renderer comparison and architecture decision | Not started | 043, 057, 059 |
| [061](docs/roadmap/05-engine-qualification.md#stage-061) | Production renderer promotion and API integration | Not started | 060 |
| [062](docs/roadmap/05-engine-qualification.md#stage-062) | Fidelity tiers, optimization and native/WASM parity | Not started | 061, 059 |
| [063](docs/roadmap/05-engine-qualification.md#stage-063) | Base-engine sound and realism acceptance | Not started | 039, 059, 061, 062 |

### 6. Conditional learned-synthesis research

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [064](docs/roadmap/06-learned-research.md#stage-064) | Standalone learned pulse and DSP-parameter benchmark | Conditional | 039, 053, 054, 059 |
| [065](docs/roadmap/06-learned-research.md#stage-065) | Conditional learned unresolved-source integration | Conditional | 064, 060, 061, 063 |

### 7. Playable browser bench

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [066](docs/roadmap/07-browser.md#stage-066) | Playable browser engine and audio lifecycle | Not started | 037, 061, 062, 063 |
| [067](docs/roadmap/07-browser.md#stage-067) | Engine, intake and exhaust control panel | Not started | 037, 066 |
| [068](docs/roadmap/07-browser.md#stage-068) | Live instruments, engine telemetry and native-owned dyno | Not started | 066, 067 |
| [069](docs/roadmap/07-browser.md#stage-069) | First playable browser qualification | Not started | 062, 063, 066, 067, 068 |

### 8. Complete 3D engine experience

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [070](docs/roadmap/08-3d.md#stage-070) | 3D scene, visual style and capability foundation | Not started | 037, 069 |
| [071](docs/roadmap/08-3d.md#stage-071) | Procedural bottom end and manual kinematics | Not started | 037, 070 |
| [072](docs/roadmap/08-3d.md#stage-072) | Procedural top end and valve motion | Not started | 071 |
| [073](docs/roadmap/08-3d.md#stage-073) | Coherent snapshots and sound-synchronized live animation | Not started | 037, 066, 072 |
| [074](docs/roadmap/08-3d.md#stage-074) | Diagnostic slow motion and four-stroke inspection | Not started | 073 |
| [075](docs/roadmap/08-3d.md#stage-075) | Shared 3D intake and complete exhaust routing | Not started | 044, 049, 071, 073 |
| [076](docs/roadmap/08-3d.md#stage-076) | Gas, combustion, pulse and thermal visualization | Not started | 047, 074, 075 |
| [077](docs/roadmap/08-3d.md#stage-077) | Combined 3D/audio performance and fallback qualification | Not started | 069, 073, 074, 075, 076 |

### 9. Turbocharged engine development

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [078](docs/roadmap/09-turbo.md#stage-078) | Turbo compressor and turbine map components | Not started | 037, 061, 039 |
| [079](docs/roadmap/09-turbo.md#stage-079) | Turbo shaft and engine gas-system integration | Not started | 078 |
| [080](docs/roadmap/09-turbo.md#stage-080) | Boost, wastegate, intercooler and blow-off control | Not started | 079 |
| [081](docs/roadmap/09-turbo.md#stage-081) | Turbo acoustic sources and turbine propagation | Not started | 080, 054, 055, 051 |
| [082](docs/roadmap/09-turbo.md#stage-082) | Turbo controls, instrumentation and build integration | Not started | 067, 068, 080, 081 |
| [083](docs/roadmap/09-turbo.md#stage-083) | Turbo geometry, shaft motion and valve visualization | Not started | 073, 075, 082 |
| [084](docs/roadmap/09-turbo.md#stage-084) | Turbo engine, audio, visual and runtime qualification | Not started | 077, 080, 081, 082, 083 |

### 10. Knock, mechanical limits and damage

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [085](docs/roadmap/10-limits.md#stage-085) | End-gas and knock physical model | Not started | 041, 080 |
| [086](docs/roadmap/10-limits.md#stage-086) | Knock-limited ECU, audible knock and feedback | Not started | 084, 085, 055 |
| [087](docs/roadmap/10-limits.md#stage-087) | Piston speed, valve float and mechanical limits | Not started | 086 |
| [088](docs/roadmap/10-limits.md#stage-088) | Optional damage state, warnings and reset behavior | Not started | 086, 087 |
| [089](docs/roadmap/10-limits.md#stage-089) | Knock and damage presentation effects | Not started | 076, 088 |

### 11. Expanded engine and exhaust catalog

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [090](docs/roadmap/11-engine-catalog.md#stage-090) | Additional inline and single-cylinder engines | Not started | 009, 071, 073, 087 |
| [091](docs/roadmap/11-engine-catalog.md#stage-091) | 60-degree and 90-degree V6 engines | Not started | 009, 071, 073, 087 |
| [092](docs/roadmap/11-engine-catalog.md#stage-092) | V10 and V12 engines | Not started | 009, 071, 073, 087 |
| [093](docs/roadmap/11-engine-catalog.md#stage-093) | Boxer-six engine | Not started | 009, 071, 073, 087 |
| [094](docs/roadmap/11-engine-catalog.md#stage-094) | Complete engine-specific exhaust and catalog calibration | Not started | 039, 084, 086, 087, 089, 090, 091, 092, 093 |

### 12. Persistence, final acceptance and release

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [095](docs/roadmap/12-release.md#stage-095) | Versioned local build persistence | Not started | 037, 067, 082, 088, 094 |
| [096](docs/roadmap/12-release.md#stage-096) | Shareable builds and validated imports | Not started | 095 |
| [097](docs/roadmap/12-release.md#stage-097) | Usability, accessibility and visual polish | Not started | 067, 068, 070, 077, 082, 089, 094, 096 |
| [098](docs/roadmap/12-release.md#stage-098) | Final per-preset listening and realism acceptance | Not started | 039, 063, 084, 089, 094, 097 |
| [099](docs/roadmap/12-release.md#stage-099) | Complete-browser and hardware release qualification | Not started | 077, 084, 089, 094, 096, 097, 098 |
| [100](docs/roadmap/12-release.md#stage-100) | Release documentation, evidence and asset review | Not started | 099 |
| [101](docs/roadmap/12-release.md#stage-101) | Static production deployment | Not started | 100 |
| [102](docs/roadmap/12-release.md#stage-102) | Deployed second-device acceptance and full release | Not started | 101 |

### 13. Optional expansions

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [103](docs/roadmap/13-optional.md#stage-103) | Gearbox and vehicle-driving mode | Optional | 102 |
| [104](docs/roadmap/13-optional.md#stage-104) | WAV recording and audio export | Optional | 102 |
| [105](docs/roadmap/13-optional.md#stage-105) | Two-stroke engine cycle | Optional | 102 |
| [106](docs/roadmap/13-optional.md#stage-106) | Alternative fuels | Optional | 102 |
| [107](docs/roadmap/13-optional.md#stage-107) | Supercharger system | Optional | 102 |
| [108](docs/roadmap/13-optional.md#stage-108) | Individual throttle bodies | Optional | 102 |
| [109](docs/roadmap/13-optional.md#stage-109) | Anti-lag system | Optional | 102, 084, 085, 087 |
| [110](docs/roadmap/13-optional.md#stage-110) | 3D garage or dyno-room environment | Optional | 102 |
| [111](docs/roadmap/13-optional.md#stage-111) | Synchronized video capture | Optional | 102 |

### 14. Bounded review repairs (execute before 037)

| Stage | Deliverable | Status | Depends on |
| --- | --- | --- | --- |
| [112](docs/roadmap/14-review-fixes.md#stage-112) | Recoverable checkpoint section publication — C08 | Complete | 033, 036 |
| [113](docs/roadmap/14-review-fixes.md#stage-113) | Acoustic derived-state and numerical failure guards — C07 | Complete | 032, 034, 036 |

## Release gates and unresolved legacy obligations

- **Base sound checkpoint:** 063; first playable browser: 069; full 3D: 077;
  turbo: 084; full catalog: 094; final listening: 098; hardware/browser: 099;
  actual public release: 102. Optional 064–065 and 103–111 are excluded unless adopted.
- Legacy **S1/S1e** remain open through causal/variation/events 040–042,
  structure/deceleration 055/058, qualification 059 and listening 063/098.
- Legacy **S2/S2d/S2e** remain open through 043–061 and acceptance 063/098.
  The tested initial conservative candidate is rejected; the coupled method is
  not yet an arbitrary graph/mixture/engine implementation.
- Legacy **S4** remains open through 056/063 and actual browser/visual delivery
  069/073/077/099. A completed native scene implementation does not close it.
- Full legacy **13b** remains open through transport/decision/optimization and
  actual delivery 043/060/062/069/099. Historical 4x-native/2x-WASM targets and
  rejected implicit probes are retained; targets are met only with evidence or
  changed explicitly with Bernardo. Optimization tactics are candidates, not
  substitutes for the fidelity and delivery goals.
- All **27 audit findings** have [explicit owners and closure evidence](docs/roadmap/audit-coverage.md).
  A scoped limitation, deferred decision or rejected hypothesis is recorded as
  such, never silently converted into a fix by this renumbering.

## Documentation map

- [Protocol, quantitative gates, evidence package and architecture sketch](docs/roadmap/README.md)
- [All 62 old stage IDs → current stages](docs/roadmap/legacy-map.md)
- [27-finding audit coverage](docs/roadmap/audit-coverage.md)
- [Roadmap decision](docs/adr/0002-canonical-roadmap-and-evidence-gates.md)
- [Domain language](GLOSSARY.md), [research](RESEARCH.md), [current handoff](CODEX.md)
- [Accepted results workflow](docs/render-workflow.md), [reference provenance](docs/user-sound-references.md)
