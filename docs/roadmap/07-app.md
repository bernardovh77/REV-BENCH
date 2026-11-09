# The desktop application

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md) · [The app's design](../app-design.md) · [ADR 0004](../adr/0004-desktop-application.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

**Since 2026-11-08** REV-BENCH is a desktop application for Windows, macOS and Linux: Electron,
with the C core and a C real-time host running natively behind a Svelte interface
([ADR 0004](../adr/0004-desktop-application.md)). This file used to be `07-browser.md`, the
browser bench. Stages 066–069 keep their numbers with new specifications; 120–124 are new;
118 is retargeted from WebAssembly to the native render thread. Every stage here builds and
tests on all three systems; a feature that works on one only is not done.

The order: 120 → 121 → 122 → 118 → 123 → 124 → 066 → 067 → 068 → 069 (120 is specified in
[14-review-fixes.md](14-review-fixes.md#stage-120)).

<a id="stage-121"></a>
## Stage 121 — The core's public engine-control, telemetry and snapshot API

**Dependencies:** [037](03-contracts-and-sources.md#stage-037), [120](14-review-fixes.md#stage-120)

**Deliverable:** Everything a host needs to run and watch the engine, in `core/include/rb.h`,
with metadata like the existing parameters and fields (app-design §6):
- **The source:** engine or test tone, the engine the default for a host that asks for it.
- **Driver controls** as parameters with ranges and units:
  - throttle, ignition, starter, load torque;
  - held or free crank and held rpm;
  - clutch, car inertia and road load, moved from `rb_cli` into the core (`rb_vehicle`).
- **Settings:** variation, quality tier, listener place, stereo scene, layers. Changing one
  either applies live or reports that it needs a rebuild.
- **A dyno the sim owns:** attach, hold, sweep, its readings and their reset. It belongs to the
  engine generation it was attached to (runtime-v1).
- **Engine telemetry:** rpm, MAP, torque, power, lambda, spark advance, idle valve, fuel cut,
  limiter cuts, exhaust and tailpipe temperatures, mass flows, each with a unit and a name.
- **Snapshots:** `rb_get_snapshot(sim, buffer, size)` fills contracts/snapshot-v2's binary
  layout from one solver boundary (header, crank, cylinders, instruments; pipe cells by
  subscription later, stage 073), with its version and size checked.
- **Events:** runtime-v1's actual events in a bounded ring with a loss count.
- **Exports:** all new functions in `WASM_EXPORTS`; the WebAssembly build keeps compiling.

Then `rb_cli` is rewritten on these calls only: its idle, rev, sweep, hold and long-coast drives
use no internal header.

**Required exit evidence:**
- Every `rb_cli sound` drive renders bit-identically before and after (same compiler), which
  proves the API reproduces them.
- New C tests for each control, setting and telemetry value (range, unit, effect, invalid
  input), the dyno's lifecycle across rebuilds, snapshot coherence and size checks, and event
  loss counting.
- Planted bugs for each, caught (`tools/bugs/stage121.py`).
- contracts/snapshot-v2.md written, with an executable checker like runtime-v1's.
- `make test`, `run_tests.py` green on Windows and WSL.

**Stop/reject rule:** No behaviour change in the physics: renders identical. A host-side
re-implementation of driving logic (road load, clutch, dyno) is what this stage removes, not
adds. No allocation in the step path.

**Legacy coverage:** 7, 9 (the native engine and dyno exposed); 14 (API part).

**Starting evidence/specification:** [contracts/runtime-v1.md](../../contracts/runtime-v1.md),
native/rb_cli.c, [docs/stage-09.md](../stage-09.md), [docs/stage-12c.md](../stage-12c.md).

**Execution record:** `docs/stage-121.md`.

<a id="stage-122"></a>
## Stage 122 — Cross-platform native builds and continuous integration

**Dependencies:** [120](14-review-fixes.md#stage-120)

**Deliverable:** The core, the tools and the C tests build with each system's own compiler,
and a CI pipeline proves it on every push:
- **CMake** for the core and tests: MSVC 2022 on Windows, Apple clang on macOS (arm64 and x86-64,
  universal), gcc and clang on Linux. Strict double arithmetic everywhere (`/fp:precise`,
  `-ffp-contract=off`, no fast-math), warnings as errors at a stated level. The Makefile stays
  for the existing workflow.
- **The core compiles cleanly under MSVC:** anything C11 that MSVC lacks is fixed portably, with
  no behaviour change and renders bit-identical under gcc.
- **A compiler-parity test:** the same seeded drives rendered by each compiler, compared, and a
  tolerance frozen from the measurement (libm differs in its last bits).
- **GitHub Actions** on Windows, macOS and Linux:
  - the native suite, the Python suite, the WebAssembly build check and the web unit tests;
  - sanitizers (ASan, UBSan) on Linux and macOS;
  - artifacts kept for a week.
- **`tools/run_tests.py`** gains a `cmake` suite (the CMake build and its tests) and checks
  toolchain versions.
- **docs/workspace.md** gets each system's toolchain set-up, exact versions included (Visual
  Studio Build Tools, CMake, Xcode command-line tools).

**Required exit evidence:**
- Green CI on all three systems for a full push.
- The parity measurement and its frozen tolerance.
- A planted portability fault (e.g. a GNU-only construct) caught by the MSVC job.
- Bernardo's Windows machine builds with MSVC locally.

**Stop/reject rule:** "It builds" isn't parity: renders must be compared. No compiler flag that
changes results (fast-math, contraction) to make a build pass.

**Decided (2026-11-08):** the repository stays private for now. Linux and Windows run on every
push to `main`; macOS, whose minutes count ten times on a private repo, runs weekly and on
demand.

**Legacy coverage:** 13 (stability across builds); 25 (reproducible builds).

**Starting evidence/specification:** the Makefile, [docs/stage-13.md](../stage-13.md),
[docs/workspace.md](../workspace.md).

**Execution record:** `docs/stage-122.md`.

<a id="stage-118"></a>
## Stage 118 — A real-time tier for today's engine

**Dependencies:** [121](#stage-121), [122](#stage-122)

**Deliverable:**
1. **A benchmark of what the app will run:** the whole render path (`rb_render_stereo` with the
   noise, listener and scene) through the public API. It covers each preset and tier on idle, a
   held pull and a rev, with variation on. It is built natively with each system's compiler and
   LTO, and as WebAssembly for the record.
2. **A profile** of that path (gprof everywhere; callgrind and perf where available; Windows'
   sampling profiler), and a denormal-number check.
3. **Speed-ups that change nothing it computes,** where the profile points: renders at the
   default quality bit-identical before and after, per compiler.
4. **A declared playable tier** that runs every preset at least 1.5 times real time on the
   render thread on Bernardo's baseline machine. Measure what it changes against the default
   (torque, VE, `tools/compare_real.py` bands, `tools/orders.py`) and render comparisons for
   listening.

**Required exit evidence:**
- Interleaved timings, best of several runs, per preset, tier, drive and compiler, on the named
  baseline machine; also the CI runners' timings, for information.
- Bit-identical default renders after the speed-ups.
- The tier's measured differences within the tolerances in
  [app-design §15](../app-design.md#15-decisions) (torque, power, VE and idle speed within 1 %;
  each compare_real and orders measure and the loudness within 1 dB).
- Bernardo's listening approves the tier.
- All suites green on all three systems.

**Stop/reject rule:** No silent quality loss: the tier states what it changes. Below-real-time
correctness prototypes don't count. The promoted renderer's own tiers stay stage 062.
WebAssembly speed is recorded, not gating.

**Decided (2026-11-08):** sample rates 44.1 and 48 kHz, rendered natively; the tier's
tolerances as in app-design §15, with Bernardo's blind listening as the last word.

**Decision (Bernardo, preflight):** the baseline machine (CPU, GPU, OS).

**Why it's here (2026-11-01, retargeted 2026-11-08):** the app (066–069) comes before the
sound-physics chain 050–063, so Bernardo can play the engine and hear each sound improvement
live. It needs today's engine to run in real time first, natively now rather than as
WebAssembly.

**Execution record:** `docs/stage-118.md`.

<a id="stage-123"></a>
## Stage 123 — The real-time audio host

**Dependencies:** [121](#stage-121), [122](#stage-122)

**Deliverable:** `host/`, a C11 library that runs the core in real time (app-design §3–5),
without Node or JavaScript:
- **A render thread** at high priority:
  - Windows: MMCSS "Pro Audio";
  - macOS: time-constraint policy and the device's audio workgroup;
  - Linux: RTKit real-time, or a raised nice level.

  It renders blocks into an SPSC ring; it never allocates, locks or logs.
- **The audio device** through vendored, pinned miniaudio:
  - device listing, choosing and changes (unplugged headphones, the default changing);
  - sample rates: native rendering at 44.1 and 48 kHz; a device at any other rate is fed
    48 kHz and converted by miniaudio;
  - stop and restart on sleep and wake;
  - the null backend for tests.
- **Lock-free channels:** the command queue with acknowledgments, the snapshot triple buffer,
  the event ring with loss counting, and the retired-engine queue.
- **Rebuild transactions** (runtime-v1): prepare off the render thread, swap at a block boundary
  with a 10–20 ms fade out and in, acknowledge the new generation, free the old engine off the
  thread. A failed preparation leaves the running engine untouched.
- **The clock mapping** from physics ticks to audio frames to device output time (runtime-v1),
  with a new epoch on device or rate changes.
- **An offline mode** that renders a scripted command trace with no device, used by tests and
  WAV export.
- **Instrumentation:**
  - each block's render time, a deadline histogram and the ring's fill;
  - underruns, faded rather than clicked;
  - the thread priority actually obtained.

**Required exit evidence:**
- Unit tests for each channel, including concurrent stress runs under ThreadSanitizer (Linux,
  macOS).
- Swap tests, covering failure, stale generation, rapid successive swaps and fades without
  clicks (measured).
- The offline mode bit-identical to `rb_cli` for the same trace and compiler.
- A real-device run on each system Bernardo can reach: 15 minutes of a scripted drive with the
  deadline histogram, zero underruns at the default latency, and bounded memory.
- Planted bugs (a torn snapshot, a lost command, a swap without fade, a priority not set),
  caught.

**Stop/reject rule:** No physics on the device thread; no allocation or lock on the render
thread (checked by test hooks, not trust). An underrun is an error to report, never hidden by
looping old audio.

**Legacy coverage:** 14 (real-time delivery); 13b-c; R17 (runtime).

**Starting evidence/specification:** [contracts/runtime-v1.md](../../contracts/runtime-v1.md),
[docs/app-design.md](../app-design.md).

**Execution record:** `docs/stage-123.md`.

<a id="stage-124"></a>
## Stage 124 — The application's foundation

**Dependencies:** [122](#stage-122), [123](#stage-123)

**Deliverable:** The Electron app as a skeleton that plays sound, on all three systems:
- **`app/`**: pinned Electron, Vite, TypeScript (strict), Svelte 5, electron-builder.
- **The process model** (app-design §3):
  - the main process;
  - the engine utility process with the Node-API add-on (`app/native/`, CMake via cmake-js:
    MSVC, Apple clang universal, gcc);
  - the sandboxed renderer with context isolation, no Node, a strict content security policy
    and a typed preload API.
- **The MessagePort channel:** commands, acknowledgments, snapshots and events, with the
  snapshot-v2 TypeScript decoder generated from the C layout and checked against it.
- **Crash handling:** if the engine process dies, the window says so and restarts it with the
  autosaved build; logs go to the user-data folder.
- **The design system:** colour tokens (dark and light), Inter, type and spacing scales, the base
  components, and the window layout with its regions empty. The look is plain and provisional:
  the visual direction is chosen later (app-design §15), so every panel uses only the tokens and
  base components, and a new look changes them, not the panels.
- **Tests:** Vitest (UI, decoder, client) and Playwright's Electron driver (launch, play on the
  null device, quit, restart after a killed engine process). Both run in `tools/run_tests.py`
  (`addon`, `app`, `app-e2e`) and in CI on all three systems.
- **Unsigned installers** per system from CI: NSIS and zip, DMG and zip universal, AppImage,
  .deb and tar.gz. They install and launch on a clean machine.
- **The app's name and icon** drafted for Bernardo's choice.

**Required exit evidence:**
- CI green with installers on all three systems.
- The installed app plays the engine (a fixed preset idling) on Bernardo's machine.
- Measured MessagePort snapshot cost (bytes per second, latency) within runtime-v1's budget.
- The killed-engine-process restart works.
- The security settings checked by a test (Electron's security checklist items).
- A test or lint check that panels use no colours, fonts or sizes outside the tokens.

**Stop/reject rule:** No Node integration in the renderer; no add-on in the main process; no
physics or audio in JavaScript. Generated decoders, not hand-copied offsets.

**Decided (2026-11-08):** the app's name is Rev-Bench.

**Deferred (2026-11-08):** the visual direction, chosen later, at the latest before stage 101.

**Decision (Bernardo):** the icon (drafts here, final later).

**Legacy coverage:** 2 (the scaffold, as an app); 14; 24 (layout foundation).

**Starting evidence/specification:** [docs/app-design.md](../app-design.md), web/ (the binding
and test set-up to reuse).

**Execution record:** `docs/stage-124.md`.

<a id="stage-066"></a>
## Stage 066 — The playable engine and its audio lifecycle

**Dependencies:** [037](03-contracts-and-sources.md#stage-037), [118](#stage-118), [124](#stage-124). (Until 2026-11-08 this was the browser's AudioWorklet engine.)

**Deliverable:** In the app:
- **Driving:** start the engine (ignition, starter, idle), blip, pull, lift and coast, stop and
  restart, with the throttle on a key (held or latched), the mouse and wheel, or a game
  controller's analogue trigger (Gamepad API).
- **Presets and sound:** choose a preset (a rebuild with its fade), the playable tier from 118,
  the listener place and the stereo scene, with genuine stereo from `rb_render_stereo`.
- **The audio lifecycle:**
  - choose the output device, and follow device changes;
  - the latency setting;
  - sleep and wake;
  - the window minimised or hidden, with the sound going on;
  - quit with a fade.
- **A minimal engine readout:** rpm and the computer's state.

**Required exit evidence:**
- A seeded command trace driven through the app's UI produces, through the host's capture tap,
  the same audio as the host's offline render of that trace. It must match within the declared
  budget at 44.1 and 48 kHz on each system.
- Start, idle, blip, lift, stop, restart, preset changes, invalid commands, device changes and
  sleep and wake each work on Windows, macOS and Linux, as e2e tests on the null device plus a
  manual check on real devices.

**Stop/reject rule:** A tone, a copied mono channel or a benchmark is not the engine playing.
No JavaScript in the audio path. The tier is never changed silently.

**Legacy coverage:** 14.

**Starting evidence/specification:** [docs/stage-02.md](../stage-02.md),
[docs/review-acoustic-architecture-2026-10-23.md](../review-acoustic-architecture-2026-10-23.md),
[docs/app-design.md](../app-design.md).

**Execution record:** `docs/stage-066.md`.

<a id="stage-067"></a>
## Stage 067 — The engine, intake and exhaust editor

**Dependencies:** [037](03-contracts-and-sources.md#stage-037), [066](#stage-066)

**Deliverable:** The editor panel, generated from the core's metadata:
- **What it edits:**
  - presets;
  - bore, stroke and displacement, compression, cams;
  - intake runners, headers, mufflers;
  - AFR, the spark-map offset, limits;
  - volume and quality.
- **Each control:** units (metric or imperial), its range, and live or rebuild behaviour
  stated.
- **Listener, scene and layer audition** controls, kept apart from physical tuning.
- **Atomic changes:** rebuilds through the host's transaction with their fade; multi-field edits
  applied as one; undo and redo of edits.

**Required exit evidence:**
- Each control has a measured physical or audible effect, a valid range, reversible state and
  coherent geometry.
- Rapid mixed edits, stale commands and failed multi-field loads can't leave a half-applied
  build or double rendering.
- Changing scenes or layers resets the listener's histories as specified.
- e2e on all three systems.

**Stop/reject rule:** JavaScript never forks the physics description, and no slider combination
is claimed physically credible just because it was accepted.

**Legacy coverage:** 15.

**Starting evidence/specification:** [CLAUDE.md](../../CLAUDE.md), [docs/app-design.md](../app-design.md) §11.

**Execution record:** `docs/stage-067.md`.

<a id="stage-068"></a>
## Stage 068 — Live instruments, telemetry and the core's dyno

**Dependencies:** [066](#stage-066), [067](#stage-067)

**Deliverable:**
- **Instruments:** rpm, MAP and boost, lambda/AFR, torque and power, the computer's cuts, an
  output meter with defined ballistics, and the engine's render-speed and underrun status, all
  from snapshots.
- **The dyno** owned by the core (stage 121): hold, sweep, live curves, previous runs overlaid,
  units, and CSV export.

**Required exit evidence:**
- App and native curves agree for the same engine and trajectory, aligned in time. The initial
  gate: at most 2 % pointwise torque error above 10 % of peak, and an absolute error at most 1 %
  of peak below that, frozen in preflight.
- Meter ballistics and sweep, reset and overlay state tested.

**Stop/reject rule:** No separate dyno in JavaScript; timing comes from the host's
instrumentation, not from estimates in the interface.

**Legacy coverage:** 16.

**Starting evidence/specification:** [docs/stage-09.md](../stage-09.md),
[docs/review-acoustic-architecture-2026-10-23.md](../review-acoustic-architecture-2026-10-23.md).

**Execution record:** `docs/stage-068.md`.

<a id="stage-069"></a>
## Stage 069 — The first playable app, qualified on three systems

**Dependencies:** [066](#stage-066), [067](#stage-067), [068](#stage-068), [118](#stage-118). Base sound acceptance stays 063, and the final listening 098.

**Deliverable:** Qualify the engine, controls, stereo and instruments on Windows, macOS and
Linux, on named machines (Bernardo's baseline, plus whatever Mac and Linux machines are
available). Define and record:
- the supported systems and versions;
- the sample rates and the latency defaults;
- the tier and its hardware demand;
- the command and telemetry budgets.

Publish the first playable build: the unsigned installers as a GitHub pre-release, once
Bernardo authorises publishing.

**Required exit evidence:**
- On each system's real audio device, the declared 15-minute stress drive: the host's deadline
  histogram (tail and maximum), underruns (zero at the default latency), and memory.
- e2e on all three systems.
- Manual start, blip, pull and long-coast listening by Bernardo.

**Stop/reject rule:** A percentile alone can hide glitches; unmeasured timing keeps
qualification open. An untested system or machine gets an explicit limitation, never silently
reduced realism. A system qualified only in CI, without a real device, is listed as such.

**Decided (2026-11-08):** Bernardo's Mac access is limited and unreliable, so macOS is qualified
by CI on the null device plus real-Mac checks only when available, and listed as "CI-tested,
limited device testing" until then.

**Decisions (Bernardo):** whether a bare-metal Linux machine is available; publishing the
pre-release, and where, while the repository is private.

**Legacy coverage:** 14; 13b-c; R17.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../sound-robustness-plan.md).

**Execution record:** `docs/stage-069.md`.
