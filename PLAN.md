# REV-BENCH — the plan

Updated 2026-11-08. This file is the only place that says which stages are done and what comes
next. Each stage's full specification (deliverable, dependencies, evidence, rejection rule) is in
`docs/roadmap/`, and each finished stage has its record in `docs/stage-NNN.md`.

**Status marks:**
- ✅ done;
- 🟡 done, with a part still open (the note says which);
- ⬜ not started;
- 💤 optional or conditional, not needed for the release.

## What REV-BENCH is

An engine test bench you download and run: a desktop app for Windows, macOS and Linux. Pick an
engine, change its displacement, compression, cams, headers, exhaust and later its turbo, and
hear, measure and see the difference in real time. The sound comes from a physical simulation of
the gas moving through the engine, not from recordings. The app's design is in
[docs/app-design.md](docs/app-design.md): the C core on a real-time thread, played through
the sound card, behind an Electron and Svelte interface
([ADR 0004](docs/adr/0004-desktop-application.md), 2026-11-08). **Realism is the absolute priority**: every realism change needs research, physical
evidence and Bernardo's ears. The first full release includes:
- the engine and exhaust editor, instruments and a dyno;
- a 3D engine with cutaway, motion and gas views;
- turbocharging, knock, limits and damage;
- a larger engine catalog;
- saving and sharing builds as files and codes;
- polish, and signed installers with updates for all three systems.

## Where the project is (2026-11-08)

- **Done: stages 001–049, 112–117 and 119–121.** Five preset engines (I4, I6, cross-plane V8, flat-plane
  V8, boxer-4) with their own researched exhausts, all running natively in C. Their sound is
  rendered to WAV files as a listener around the car hears it, in mono or in eight stereo
  scenes. Native tests: 251 tests, 4,950,067 checks.
- **The sound now:**
  - **Under load:** the exhaust's rasp above 2 kHz is in the range of recordings of real engines
    (stage 115).
  - **The cycles:** each one differs from the last as much as a real engine's do (COV of IMEP
    1–2.3 %).
  - **Still open:** idling from behind is too dull above 2 kHz. The sound still repeats more
    exactly than a real engine's. The cross-plane V8 lost its burble at 3000 rpm to its X-pipe
    exhaust.
- **Research done but not in the engine:** stages 043–049 and 114 built exhaust-acoustics
  solvers in `experiments/acoustics/`. They run far below real time and only on weak, cool
  waves. They wait for stages 050–063 to decide whether any of them improves the engine.
- **Nothing plays the engine live yet.** The public API still plays a test tone (the driving
  lives in `rb_cli`), and the engine doesn't run in real time: on Bernardo's machine the four's
  engine step alone runs at 0.66× real time (2026-11-06); the cloud VM's whole render gave the
  cross-plane V8 0.4×. The old browser scaffold (`web/`) plays a tone in an AudioWorklet.
- **Every test runs on Windows and in WSL alike** (2026-11-06): `python tools/run_tests.py`,
  with the same results on both (docs/workspace.md).
- **The app replaces the browser** (2026-11-08): a downloadable app for Windows, macOS and
  Linux, chosen for its look, craft and reliable real-time sound
  ([ADR 0004](docs/adr/0004-desktop-application.md)). Its stages come first. It is built
  Windows first: Linux CI and a weekly macOS core run keep the code portable, and stage 126
  ports it to macOS and Linux before the release stages.
- **Bernardo's reference recordings and videos** are on his machine, in `renders/` (git-ignored;
  docs/user-sound-references.md). Stages 039, 058, 059 and 063 need them.

## The order of the work

Bernardo (2026-11-01): "Execute in the order you find most ideal as long as all the work is
complete and there are no gaps in between." The original order (2026-10-23) was contracts, then
all the sound physics and runtime work, then the browser, then 3D. One change:

**The engine plays in the app first.** The playable app (066–069) comes ahead of the long
sound-physics chain (050–063). It plays today's engine at a quality that runs in real time
(stage 118). Every later sound improvement then lands in something Bernardo can play and hear
live, not only in WAV files. Nothing is dropped. The sound stages still all come before 3D,
turbo and the rest, as agreed.

**2026-11-08: a desktop app, not a browser page** (Bernardo: a downloadable app for Windows,
Linux and Mac, chosen for what is best for the project, its look and its craft). Phase A now
builds the app's foundations first: the workspace's upkeep (120), the core's public engine API
(121), builds and CI (122), the real-time tier (118), the real-time audio
host (123) and the app's shell (124). Then 066–069, rewritten for the app.

**2026-11-08, later: Windows first.** Every stage up to the end of phase F is finished and
tested on Windows; Linux CI builds and tests the C, the add-on and the interface on every push,
and macOS runs the core's tests weekly. Stage 126, at the start of phase G, ports the app to
macOS and Linux ([app-design §1](docs/app-design.md#windows-first)).

1. **Phase A, the app and live play:**
   - [x] **119:** the review's leftovers;
   - [x] **120:** the workspace and toolset upkeep;
   - [x] **121:** the core's public engine-control, telemetry and snapshot API;
   - [ ] **122:** builds and CI: Windows (MSVC) and Linux (gcc) on every push, macOS's core weekly;
   - [ ] **118:** a real-time tier for today's engine;
   - [ ] **123:** the real-time audio host;
   - [ ] **124:** the application's foundation;
   - [ ] **066 → 067 → 068 → 069:** the playable app, qualified on Windows.
2. **Phase B, real sound:**
   - [ ] **039:** matched references (Bernardo's recordings, alongside the rest);
   - [ ] **050, 051 → 052;**
   - [ ] **053, 055, 056 → 054;**
   - [ ] **057 → 058 → 059 → 060 → 061 → 062 → 063:** base sound acceptance. Each of these
     updates the app too.
3. **Phase C, the 3D engine:** [ ] 070 → 071 → 072 → 073 → 074 → 075 → 076 → 077.
4. **Phase D, turbo:** [ ] 078 → 084.
5. **Phase E, knock and limits:** [ ] 085 → 089.
6. **Phase F, more engines:** [ ] 090 → 094.
7. **Phase G, release:** [ ] 126 (the port to macOS and Linux) → 095 → 102.
8. **Afterwards, if adopted:** 💤 064–065 (learned synthesis, only if 059/060 call for it),
   103–111 and 125 (optional extras, 125 a browser demo).

**Next stage: 122.**

## The stages, in the order they'll be done

Click a stage's number for its specification. "Depends on" lists stages that must be done first.

### Phase A — the desktop app and live play

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| ✅ | [119](docs/roadmap/14-review-fixes.md#stage-119) | The review's leftovers. Readable C in Codex's core code (no behaviour change, identical renders). The reference tools measure in stereo as they say. Failed publishes don't block `make results`. Checks aren't `assert`s. Windows paths. The research solvers' tests can fail. | 117 |
| ✅ | [120](docs/roadmap/14-review-fixes.md#stage-120) | The workspace and toolset upkeep. The 4.7 GB zip and old renders moved into `renders/`. 29 stale planted bugs: 26 retargeted, 3 retired, and `plant_bugs.py --check` in the Python suite. The runner's pre-flight, time limits and `--keep-going`, written test first. The Makefile finds the venv. Current docs and hooks. | 119 |
| ✅ | [121](docs/roadmap/07-app.md#stage-121) | The core's public engine API: driver controls, the car and its road load, a dyno the sim owns, settings, 24 telemetry values, snapshot-v2 and events, written test first. `rb_cli sound` on rb.h alone, its 11 drives bit-identical. 28 + 5 planted bugs caught. | 037, 120 |
| ⬜ | [122](docs/roadmap/07-app.md#stage-122) | Builds through CMake with MSVC (the target) and gcc, a cross-compiler parity tolerance, CI on Windows and Linux every push and the core's tests on macOS weekly | 120 |
| ⬜ | [118](docs/roadmap/07-app.md#stage-118) | A real-time tier for today's engine: benchmark the whole render path, profile, speed up without changing results, and declare a tier that runs every preset at least 1.5× real time natively on Bernardo's machine, with what it changes measured and listened to | 121, 122 |
| ⬜ | [123](docs/roadmap/07-app.md#stage-123) | The real-time audio host (C): a high-priority render thread, miniaudio, lock-free commands, snapshots and events, rebuilds with fades, the clock mapping, an offline mode and its instrumentation | 121, 122 |
| ⬜ | [124](docs/roadmap/07-app.md#stage-124) | The application's foundation: Electron, the engine process with the Node-API add-on, the sandboxed Svelte window, a provisional design system on tokens, tests and an unsigned Windows installer; the add-on and interface tests also in Linux CI | 122, 123 |
| ⬜ | [066](docs/roadmap/07-app.md#stage-066) | The playable engine in the app: start, idle, rev, stop, presets, the tier, stereo scenes, keyboard and game-controller throttle, devices, sleep and wake | 037, 118, 124 |
| ⬜ | [067](docs/roadmap/07-app.md#stage-067) | The engine, intake and exhaust editor, from the core's metadata, with atomic rebuilds and undo | 037, 066 |
| ⬜ | [068](docs/roadmap/07-app.md#stage-068) | Live instruments, telemetry and the core's dyno, with overlays | 066, 067 |
| ⬜ | [069](docs/roadmap/07-app.md#stage-069) | The first playable app, qualified on Windows: deadlines, 15-minute stress runs on real devices, listening; the first pre-release | 066, 067, 068, 118 |

### Phase B — the sound to real-engine standard

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| 🟡 | [039](docs/roadmap/03-contracts-and-sources.md#stage-039) | Matched references and calibration protocol. The protocol is done; recordings with known engine, exhaust, state and microphone are still needed | 036 |
| ⬜ | [050](docs/roadmap/04-propagation.md#stage-050) | Catalysts, mufflers, perforates and packing for the actual exhausts | 047, 049 |
| ⬜ | [051](docs/roadmap/04-propagation.md#stage-051) | Radiation load and audible-field consistency at the tailpipe | 049 |
| ⬜ | [052](docs/roadmap/04-propagation.md#stage-052) | Mixed-rate scheduler and state transitions | 048, 049, 050, 051 |
| ⬜ | [053](docs/roadmap/05-engine-qualification.md#stage-053) | Compressible and reversing turbulent-flow excitation (stage 115 fixed the choking; the energy ownership and reverse flow remain) | 040, 042, 047, 038 |
| ⬜ | [055](docs/roadmap/05-engine-qualification.md#stage-055) | Engine-specific structure and accessories (block, head, body modes per engine) | 041, 042 |
| ⬜ | [056](docs/roadmap/05-engine-qualification.md#stage-056) | Listener, vehicle and recording-transfer validation | 024, 037, 038, 051 |
| ⬜ | [054](docs/roadmap/05-engine-qualification.md#stage-054) | Unified pulse and noise routing (stage 115 fixed the paths and junction sharing; the frequency-dependent transfer remains) | 044, 049, 050, 051, 053 |
| ⬜ | [057](docs/roadmap/05-engine-qualification.md#stage-057) | Whole-engine candidate integration of the surviving research solvers | 041–052, 054, 055, 056 |
| ⬜ | [058](docs/roadmap/05-engine-qualification.md#stage-058) | Deceleration and lift-off: find the causes of the robotic texture and the boom | 039, 057 |
| ⬜ | [059](docs/roadmap/05-engine-qualification.md#stage-059) | Five-engine physical and acoustic qualification against matched references | 039, 057, 058 |
| ⬜ | [060](docs/roadmap/05-engine-qualification.md#stage-060) | Renderer comparison and architecture decision (an ADR) | 043, 057, 059 |
| ⬜ | [061](docs/roadmap/05-engine-qualification.md#stage-061) | Production renderer promotion, into the core, the real-time host and the app | 060 |
| ⬜ | [062](docs/roadmap/05-engine-qualification.md#stage-062) | Fidelity tiers, optimization and cross-compiler parity for the promoted renderer | 061, 059 |
| ⬜ | [063](docs/roadmap/05-engine-qualification.md#stage-063) | Base-engine sound and realism acceptance: Bernardo listens, per preset and scenario | 039, 059, 061, 062 |
| 💤 | [064](docs/roadmap/06-learned-research.md#stage-064) | Conditional: learned pulse and DSP-parameter benchmark | 039, 053, 054, 059 |
| 💤 | [065](docs/roadmap/06-learned-research.md#stage-065) | Conditional: learned unresolved-source integration | 064, 060, 061, 063 |

### Phase C — the 3D engine

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| ⬜ | [070](docs/roadmap/08-3d.md#stage-070) | 3D scene in the app's window, visual style and capability foundation | 037, 069 |
| ⬜ | [071](docs/roadmap/08-3d.md#stage-071) | Procedural bottom end and manual kinematics | 037, 070 |
| ⬜ | [072](docs/roadmap/08-3d.md#stage-072) | Procedural top end and valve motion | 071 |
| ⬜ | [073](docs/roadmap/08-3d.md#stage-073) | Coherent snapshots and sound-synchronized live animation | 037, 066, 072 |
| ⬜ | [074](docs/roadmap/08-3d.md#stage-074) | Diagnostic slow motion and four-stroke inspection | 073 |
| ⬜ | [075](docs/roadmap/08-3d.md#stage-075) | Shared 3D intake and complete exhaust routing | 044, 049, 071, 073 |
| ⬜ | [076](docs/roadmap/08-3d.md#stage-076) | Gas, combustion, pulse and thermal visualization | 047, 074, 075 |
| ⬜ | [077](docs/roadmap/08-3d.md#stage-077) | Combined 3D/audio performance and fallback qualification | 069, 073, 074, 075, 076 |

### Phase D — turbocharging

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| ⬜ | [078](docs/roadmap/09-turbo.md#stage-078) | Turbo compressor and turbine map components | 037, 061, 039 |
| ⬜ | [079](docs/roadmap/09-turbo.md#stage-079) | Turbo shaft and engine gas-system integration | 078 |
| ⬜ | [080](docs/roadmap/09-turbo.md#stage-080) | Boost, wastegate, intercooler and blow-off control | 079 |
| ⬜ | [081](docs/roadmap/09-turbo.md#stage-081) | Turbo acoustic sources and turbine propagation | 080, 054, 055, 051 |
| ⬜ | [082](docs/roadmap/09-turbo.md#stage-082) | Turbo controls, instrumentation and build integration | 067, 068, 080, 081 |
| ⬜ | [083](docs/roadmap/09-turbo.md#stage-083) | Turbo geometry, shaft motion and valve visualization | 073, 075, 082 |
| ⬜ | [084](docs/roadmap/09-turbo.md#stage-084) | Turbo engine, audio, visual and runtime qualification | 077, 080, 081, 082, 083 |

### Phase E — knock, mechanical limits and damage

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| ⬜ | [085](docs/roadmap/10-limits.md#stage-085) | End-gas and knock physical model | 041, 080 |
| ⬜ | [086](docs/roadmap/10-limits.md#stage-086) | Knock-limited ECU, audible knock and feedback | 084, 085, 055 |
| ⬜ | [087](docs/roadmap/10-limits.md#stage-087) | Piston speed, valve float and mechanical limits | 086 |
| ⬜ | [088](docs/roadmap/10-limits.md#stage-088) | Optional damage state, warnings and reset behavior | 086, 087 |
| ⬜ | [089](docs/roadmap/10-limits.md#stage-089) | Knock and damage presentation effects | 076, 088 |

### Phase F — more engines and exhausts

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| ⬜ | [090](docs/roadmap/11-engine-catalog.md#stage-090) | Additional inline and single-cylinder engines | 009, 071, 073, 087 |
| ⬜ | [091](docs/roadmap/11-engine-catalog.md#stage-091) | 60-degree and 90-degree V6 engines | 009, 071, 073, 087 |
| ⬜ | [092](docs/roadmap/11-engine-catalog.md#stage-092) | V10 and V12 engines | 009, 071, 073, 087 |
| ⬜ | [093](docs/roadmap/11-engine-catalog.md#stage-093) | Boxer-six engine | 009, 071, 073, 087 |
| ⬜ | [094](docs/roadmap/11-engine-catalog.md#stage-094) | Complete engine-specific exhaust and catalog calibration | 039, 084, 086, 087, 089, 090–093 |

### Phase G — saving, final acceptance and release

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| ⬜ | [126](docs/roadmap/12-release.md#stage-126) | The port to macOS and Linux: Apple clang universal and gcc builds, their real-time thread priorities, packages, every end-to-end test on both, real-device runs where machines exist | 069, 077, 084, 089, 094 |
| ⬜ | [095](docs/roadmap/12-release.md#stage-095) | Versioned `.rbbuild` files, autosaved sessions | 037, 067, 082, 088, 094, 126 |
| ⬜ | [096](docs/roadmap/12-release.md#stage-096) | Sharing builds: build codes and `rbench://` links, validated imports | 095 |
| ⬜ | [097](docs/roadmap/12-release.md#stage-097) | Usability, accessibility and visual polish | 067, 068, 070, 077, 082, 089, 094, 096 |
| ⬜ | [098](docs/roadmap/12-release.md#stage-098) | Final per-preset listening and realism acceptance | 039, 063, 084, 089, 094, 097 |
| ⬜ | [099](docs/roadmap/12-release.md#stage-099) | Release qualification on the supported systems and hardware | 077, 084, 089, 094, 096, 097, 098, 126 |
| ⬜ | [100](docs/roadmap/12-release.md#stage-100) | Release documentation, licences and asset review | 099 |
| ⬜ | [101](docs/roadmap/12-release.md#stage-101) | Signed installers and the update channel | 100 |
| ⬜ | [102](docs/roadmap/12-release.md#stage-102) | Public release and clean-machine acceptance | 101 |

### Optional, after the release

| | Stage | What | Depends on |
| --- | --- | --- | --- |
| 💤 | [103](docs/roadmap/13-optional.md#stage-103) | Gearbox and vehicle-driving mode | 102 |
| 💤 | [104](docs/roadmap/13-optional.md#stage-104) | WAV recording and audio export (cheap in the app: the host's capture tap) | 102 |
| 💤 | [105](docs/roadmap/13-optional.md#stage-105) | Two-stroke engine cycle | 102 |
| 💤 | [106](docs/roadmap/13-optional.md#stage-106) | Alternative fuels | 102 |
| 💤 | [107](docs/roadmap/13-optional.md#stage-107) | Supercharger system | 102 |
| 💤 | [108](docs/roadmap/13-optional.md#stage-108) | Individual throttle bodies | 102 |
| 💤 | [109](docs/roadmap/13-optional.md#stage-109) | Anti-lag system | 102, 084, 085, 087 |
| 💤 | [110](docs/roadmap/13-optional.md#stage-110) | 3D garage or dyno-room environment | 102 |
| 💤 | [111](docs/roadmap/13-optional.md#stage-111) | Synchronized video capture | 102 |
| 💤 | [125](docs/roadmap/13-optional.md#stage-125) | A browser demo on the WebAssembly core, at a declared lighter quality | 102 |

## Done

Each stage's record holds what it proved and its limits. 🟡 marks a stage whose part listed in
the note stays open and belongs to a later stage.

### The engine and its first sound (001–016)

| | Stage | What |
| --- | --- | --- |
| ✅ | [001](docs/roadmap/01-foundations.md#stage-001) | Initial research and native-first project setup |
| ✅ | [002](docs/roadmap/01-foundations.md#stage-002) | Native scaffold, WAV/CSV output and C tests |
| ✅ | [003](docs/roadmap/01-foundations.md#stage-003) | WebAssembly and AudioWorklet tone scaffold |
| ✅ | [004](docs/roadmap/01-foundations.md#stage-004) | Gas properties, chambers and orifice flow |
| ✅ | [005](docs/roadmap/01-foundations.md#stage-005) | Slider-crank and motored cylinder |
| ✅ | [006](docs/roadmap/01-foundations.md#stage-006) | Valvetrain, throttle and intake plenum |
| ✅ | [007](docs/roadmap/01-foundations.md#stage-007) | Baseline combustion and wall heat |
| ✅ | [008](docs/roadmap/01-foundations.md#stage-008) | Free crank, starter, engine computer and multicylinder running |
| ✅ | [009](docs/roadmap/01-foundations.md#stage-009) | Five engine layouts and the description API |
| ✅ | [010](docs/roadmap/01-foundations.md#stage-010) | Native dyno and calibration against real engines |
| ✅ | [011](docs/roadmap/01-foundations.md#stage-011) | One-dimensional intake and exhaust pipes |
| ✅ | [012](docs/roadmap/01-foundations.md#stage-012) | Sound extraction and the digital audio chain |
| ✅ | [013](docs/roadmap/01-foundations.md#stage-013) | Mufflers, intake path and the listener |
| 🟡 | [014](docs/roadmap/01-foundations.md#stage-014) | Performance, quality levels and stability. The 4× real-time native target was not met (→ 118, 062) |
| ✅ | [015](docs/roadmap/01-foundations.md#stage-015) | Flow noise, mechanical sound and the cabin |
| ✅ | [016](docs/roadmap/01-foundations.md#stage-016) | Cycle variation, the soft-cut limiter and deceleration drives |

### Sound work, measurements and early prototypes (017–035)

| | Stage | What |
| --- | --- | --- |
| ✅ | [017](docs/roadmap/02-existing-sound.md#stage-017) | Float-audio analysis and Windows paths |
| ✅ | [018](docs/roadmap/02-existing-sound.md#stage-018) | Finite burns stop at exhaust opening |
| ✅ | [019](docs/roadmap/02-existing-sound.md#stage-019) | Five-preset repetition and combustion audit |
| ✅ | [020](docs/roadmap/02-existing-sound.md#stage-020) | Trapped-mixture burn feedback |
| ✅ | [021](docs/roadmap/02-existing-sound.md#stage-021) | Bernardo's reference recordings cataloged |
| ✅ | [022](docs/roadmap/02-existing-sound.md#stage-022) | Filtered reference listening library |
| ✅ | [023](docs/roadmap/02-existing-sound.md#stage-023) | Loaded exhaust detail and lift-transition correction |
| ✅ | [024](docs/roadmap/02-existing-sound.md#stage-024) | Native stereo scenes |
| ✅ | [025](docs/roadmap/02-existing-sound.md#stage-025) | Complete sound and engine-physics audit (D01–D09, R01–R18) |
| ✅ | [026](docs/roadmap/02-existing-sound.md#stage-026) | Batched thermodynamics and SIMD |
| ✅ | [027](docs/roadmap/02-existing-sound.md#stage-027) | Five model-specific researched exhausts |
| ✅ | [028](docs/roadmap/02-existing-sound.md#stage-028) | High-rpm configurations and the selective rollback (the accepted sound) |
| ✅ | [029](docs/roadmap/02-existing-sound.md#stage-029) | Source, mesh and time-step experiment |
| ✅ | [030](docs/roadmap/02-existing-sound.md#stage-030) | Acoustic architecture comparison specification |
| ✅ | [031](docs/roadmap/02-existing-sound.md#stage-031) | Initial conservative-duct experiment (candidate rejected) |
| ✅ | [032](docs/roadmap/02-existing-sound.md#stage-032) | Static acoustic graph fixtures |
| ✅ | [033](docs/roadmap/02-existing-sound.md#stage-033) | Results inventory and atomic rebuilding |
| ✅ | [034](docs/roadmap/02-existing-sound.md#stage-034) | Same-tick acoustic receive and commit interface |
| ✅ | [035](docs/roadmap/02-existing-sound.md#stage-035) | Open gas/characteristic duct proof |

### The roadmap, contracts and the engine's events (036–042)

| | Stage | What |
| --- | --- | --- |
| ✅ | [036](docs/roadmap/03-contracts-and-sources.md#stage-036) | Unified roadmap and evidence protocol |
| ✅ | [037](docs/roadmap/03-contracts-and-sources.md#stage-037) | Shared build, geometry, event and snapshot contracts |
| ✅ | [038](docs/roadmap/03-contracts-and-sources.md#stage-038) | Numerical guard and flux observability |
| ✅ | [040](docs/roadmap/03-contracts-and-sources.md#stage-040) | Spark, ignition and flame-development causality |
| 🟡 | [041](docs/roadmap/03-contracts-and-sources.md#stage-041) | Combustion variation and the charge's burn model. Calibration against real engines stays open (→ 059); its variation shortfall was fixed in 115 |
| 🟡 | [042](docs/roadmap/03-contracts-and-sources.md#stage-042) | Fuel-injection events and their clicks. Real injector calibration stays open (→ 055) |

### Exhaust-acoustics research (043–049, 114)

These are research experiments, not part of the engine (`experiments/acoustics/`). They are
valid only for weak waves under 990 K and run far below real time. Stages 050–060 decide what, if
anything, goes into the engine.

| | Stage | What |
| --- | --- | --- |
| ✅ | [043](docs/roadmap/04-propagation.md#stage-043) | Improved conservative solver with the production gas. Its candidate, A2, was rejected |
| ✅ | [044](docs/roadmap/04-propagation.md#stage-044) | Shared exhaust geometry compiler |
| ✅ | [045](docs/roadmap/04-propagation.md#stage-045) | Zero-flow gas-to-acoustic-graph coupling |
| ✅ | [046](docs/roadmap/04-propagation.md#stage-046) | Uniform mean-flow coupling |
| ✅ | [047](docs/roadmap/04-propagation.md#stage-047) | Gas-property and wall-thermal characterization |
| ✅ | [048](docs/roadmap/04-propagation.md#stage-048) | Thermal fronts and reverse entrainment (material reference) |
| ✅ | [114](docs/roadmap/04-propagation.md#stage-114) | Fixed-geometry thermal transport bridge |
| ✅ | [049](docs/roadmap/04-propagation.md#stage-049) | Branches, finite junctions and all-port transfer |

### Review repairs (112–117, 119)

| | Stage | What |
| --- | --- | --- |
| ✅ | [112](docs/roadmap/14-review-fixes.md#stage-112) | Recoverable checkpoint publication |
| ✅ | [113](docs/roadmap/14-review-fixes.md#stage-113) | Acoustic numerical failure guards |
| ✅ | [115](docs/roadmap/14-review-fixes.md#stage-115) | The valves' jet noise fixed (the audit's D02–D05) and the cycles' variation back to real engines' |
| ✅ | [116](docs/roadmap/14-review-fixes.md#stage-116) | The loosened tests tightened, the make 4.3 Python test fixed |
| ✅ | [117](docs/roadmap/14-review-fixes.md#stage-117) | The docs readable and current |
| ✅ | [119](docs/stage-119.md) | The review's leftovers: Codex's C readable (byte-identical WebAssembly and objects, identical renders), stereo measurement, safe experiment publish, explicit checks, Windows, evidence-derived prose, solver tests that can fail |

## How a stage is done

1. Say "do stage 120" or "next stage". Read this file, `CLAUDE.md` and `CODEX.md`'s latest
   entry, then the stage's specification in `docs/roadmap/`.
2. Do only that stage. Follow its dependencies, not just the row above it.
3. End with all tests green:
   - native: `make test`;
   - Python: the tools' unit tests;
   - web: `npm test`;
   - the app (from stage 124): its unit tests and end-to-end tests;
   - `python tools/run_tests.py` runs them all, on Windows or WSL; CI runs them on Windows and
     Linux, and the core's on macOS weekly (from stage 122), and everything on all three
     systems from stage 126.
4. New physics tests must be able to fail: plant bugs (`tools/plant_bugs.py`).
5. A change in `core/` rebuilds the WebAssembly core (`make wasm`).
6. **The sound check:** a change in `core/` also renders the affected sound and compares it with
   the real recordings (`tools/compare_real.py`, `tools/orders.py`). Bernardo's ears decide
   realism.
7. Write `docs/stage-NNN.md`, mark the stage here with a one-line note, and add a short entry to
   `CODEX.md`. Then commit (one per stage) and push.
8. New work that no stage covers gets the next unused number (127, ...) and a place in the order
   above. Stage numbers never change.

The full evidence protocol, gates and architecture sketch are in
[docs/roadmap/README.md](docs/roadmap/README.md).

## Rules that hold for every stage

- One physical engine owns the crank, combustion and gas exchange. One description of it feeds
  the sound, the controls, saving and the 3D view, and one event clock drives sound and
  visuals. JavaScript never duplicates the engine's geometry or physics.
- The stack: a C11 core, native measurement tools, a C real-time host, and an Electron app with a
  Svelte interface and procedural 3D, on Windows, macOS and Linux (docs/app-design.md). The
  WebAssembly build is kept for portability. The engine is validated natively before it goes
  into the app. No physics or audio ever runs in JavaScript.
- Windows first: until stage 126 every app stage is finished and tested on Windows, keeps
  Linux CI green and keeps the core's tests passing on macOS. System-specific code lives only in
  the platform files (app-design §1). From stage 126 every stage builds and tests on all three
  systems; a feature that works on one only is not done.
- Realism work goes: research, a stated hypothesis, an independent native test, a controlled
  engine comparison, native runs and the app, then Bernardo's listening. Keep findings apart
  from hypotheses, and research results apart from what goes into the engine.
- Measurements use unfiltered float audio. Generated and reference audio stay out of git.
- Query the code graph (graphify) before exploring code, and refresh it after code changes.
- Never silently lower the sound's quality for speed. A faster tier says what it changes.

## What stays open from earlier plans

- **The old sound tracks:** S1 (cycle variation, deceleration) closes at 058, 059 and 063. S2 (the
  exhaust's acoustics) closes at 050–061 and 063. S4 (the listener's scenes) closes at 056 and
  063, and in the app at 069 and 077.
- **Speed:** the old targets (4× real time natively, 2× in WebAssembly) are unmet. 118 gets a
  real-time tier for today's engine natively; 062 does it for the promoted renderer.
  WebAssembly is recorded, not gating, since the app runs natively.
- **The audit's 27 findings** each have an owner stage and closure evidence in
  [docs/roadmap/audit-coverage.md](docs/roadmap/audit-coverage.md).
- **Release gates:**
  - first playable app: 069;
  - base sound acceptance: 063;
  - full 3D: 077;
  - turbo: 084;
  - full catalog: 094;
  - final listening: 098;
  - systems and hardware qualification: 099;
  - public release: 102.

## Documentation map

- [Stage protocol, gates and architecture](docs/roadmap/README.md); the stage specifications are
  in `docs/roadmap/01-…14-*.md`.
- [Every old stage number → today's](docs/roadmap/legacy-map.md) and the
  [archived first plan](docs/roadmap/legacy-plan.md). Earlier versions of this file are in git's
  history: Codex's checkpoint-by-checkpoint log is at commit 011b3cf.
- [The audit's findings and their owners](docs/roadmap/audit-coverage.md),
  [roadmap decision](docs/adr/0002-canonical-roadmap-and-evidence-gates.md).
- [The app's design](docs/app-design.md) and [its decision](docs/adr/0004-desktop-application.md).
- [Glossary](GLOSSARY.md), [research](RESEARCH.md), [Codex's handoff log](CODEX.md),
  [notes for Claude](CLAUDE.md), [where the project is worked on](docs/workspace.md).
- [Results workflow](docs/render-workflow.md), [Bernardo's references](docs/user-sound-references.md).
