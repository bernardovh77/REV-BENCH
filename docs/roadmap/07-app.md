# The desktop application

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md) · [The app's design](../app-design.md) · [ADR 0004](../adr/0004-desktop-application.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

**Since 2026-11-08** REV-BENCH is a desktop application for Windows, macOS and Linux: Electron,
with the C core and a C real-time host running natively behind a Svelte interface
([ADR 0004](../adr/0004-desktop-application.md)). This file used to be `07-browser.md`, the
browser bench. Stages 066–069 keep their numbers with new specifications; 120–124 are new;
118 is retargeted from WebAssembly to the native render thread.

**Windows first** ([app-design §1](../app-design.md#windows-first)): every stage here is finished and tested on Windows, keeps building
and passing its tests in Linux CI, and keeps the core's tests passing in macOS's weekly CI run.
The full macOS and Linux app is [stage 126](12-release.md#stage-126), before the release
stages.

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

**Deliverable:** The core, the tools and the C tests build with MSVC and gcc (and the core's
tests with Apple clang),
and a CI pipeline proves it on every push:
- **CMake** for the core and tests: MSVC 2022 on Windows (the target), gcc on Linux, and Apple
  clang on macOS for the core's tests (universal builds wait for stage 126). Strict double arithmetic everywhere (`/fp:precise`,
  `-ffp-contract=off`, no fast-math), warnings as errors at a stated level. The Makefile stays
  for the existing workflow.
- **The core compiles cleanly under MSVC:** anything C11 that MSVC lacks is fixed portably, with
  no behaviour change and renders bit-identical under gcc.
- **A compiler-parity test:** the same seeded drives rendered by MSVC and gcc, compared, and a
  tolerance frozen from the measurement (libm differs in its last bits); Apple clang's weekly
  renders are checked against it.
- **GitHub Actions:**
  - Windows, every push: the native suite (MSVC and w64devkit), the Python suite, the
    WebAssembly build check and the web unit tests;
  - Linux, every push: the same with gcc, plus the sanitizers (ASan, UBSan);
  - macOS, weekly and on demand: the core's tests with Apple clang;
  - artifacts kept for a week.
- **`tools/run_tests.py`** gains a `cmake` suite (the CMake build and its tests) and checks
  toolchain versions.
- **docs/workspace.md** gets Windows' toolchain set-up, exact versions included (Visual Studio
  Build Tools, CMake); macOS's and Linux's workstation set-ups come with stage 126.

**Required exit evidence:**
- Green CI: Windows and Linux on a full push, and a macOS core run.
- The parity measurement and its frozen tolerance.
- A planted portability fault (e.g. a GNU-only construct) caught by the MSVC job.
- Bernardo's Windows machine builds with MSVC locally.

**Stop/reject rule:** "It builds" isn't parity: renders must be compared. No compiler flag that
changes results (fast-math, contraction) to make a build pass.

**Decided (2026-11-08, revised 2026-11-09):** the repository is public (its runners are free).
Linux and Windows run on every push to `main`; macOS runs the core's tests weekly and on demand.
Windows first, so this stage builds no macOS or Linux app.

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
   held pull and a rev, with variation on. It is built with MSVC and LTO on Windows (the target),
   and with gcc in Linux CI and as WebAssembly for the record.
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
- All suites green on Windows and in Linux CI.

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
  - Windows: MMCSS "Pro Audio", in `host/src/platform_win.c`;
  - elsewhere `platform_posix.c` runs it at normal priority, for Linux CI; macOS's
    time-constraint policy and audio workgroup and Linux's RTKit come with stage 126.

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
- Unit tests for each channel, including concurrent stress runs under ThreadSanitizer (in Linux
  CI). The host builds and passes its tests on the null device on Windows and in Linux CI.
- Swap tests, covering failure, stale generation, rapid successive swaps and fades without
  clicks (measured).
- The offline mode bit-identical to `rb_cli` for the same trace and compiler.
- A real-device run on Bernardo's Windows machine: 15 minutes of a scripted drive with the
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

**Deliverable:** The Electron app as a skeleton that plays sound, on Windows:
- **`app/`**: pinned Electron, Vite, TypeScript (strict), Svelte 5, electron-builder.
- **The process model** (app-design §3):
  - the main process;
  - the engine utility process with the Node-API add-on (`app/native/`, CMake via cmake-js:
    MSVC; gcc in Linux CI);
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
  (`addon`, `app`, `app-e2e`) and in CI on Windows; the add-on's and Vitest's also on Linux.
- **An unsigned Windows installer** from CI (NSIS and a portable zip), which installs and
  launches on a clean Windows machine. macOS and Linux packages come with stage 126.
- **Platform code in one place:** `app/src/main/platform.ts` for anything system-specific in
  the shell (menus, paths, the user-data folder).
- **The app's name and icon** drafted for Bernardo's choice.

**Required exit evidence:**
- CI green: Windows with its installer, Linux's add-on and interface suites.
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
- **Added 2026-11-13 (Bernardo, after playing stage 124's build):**
  - **Throttle styles:** an on/off switch (a key snaps 0 ↔ 100 %), a ramped key (held: rises at
    a set rate; released: falls), analog key travel, and a controller's analog trigger.
  - **Analog keyboards:** Bernardo's is a Razer analog optical keyboard. Its key travel reaches
    the app through Razer Synapse's controller emulation (the Gamepad API); whether a native
    Razer SDK can read it directly is checked here, not assumed. Tested on his keyboard.
  - **Levels:** real relative loudness between engines (a V8 louder than a four), with the
    overall gain lowered so the loudest revs stay under full scale (today the six and V8s reach
    0 dBFS revving, the four −16), and a volume control. The four's own level is checked against
    real recordings in phase B.
  - **A lost device:** keep playing on the new default and show which (app-design §15).
- **Added 2026-11-15 (the review, docs/review-2026-11-15.md, problems 1 and 8):**
  - **A dead-man rule, in the engine process:** while a live throttle style is active (ramped
    key, analog key, trigger), the window sends the throttle at a steady beat. If none arrives
    for 250 ms the engine process ramps the throttle to zero over 100 ms. The on/off and
    latched styles hold by design and are exempt. On blur or hide the window sends "released"
    for every held key and trigger. Chromium slows a hidden window's timers and stops sending
    a page its key-ups and controller state once it loses focus, so without this an engine
    could stay at full throttle with nobody able to lift.
  - **The A/B key:** a key flips `RB_PARAM_VARIANT` (stage 127) between 0 and the candidate
    number chosen in a developer menu, while the engine runs; a blind mode picks at random
    which is A. With no candidate in the core the two are the same sound, and the menu says so.

**Required exit evidence:**
- A seeded command trace driven through the app's UI produces, through the host's capture tap,
  the same audio as the host's offline render of that trace. It must match within the declared
  budget at 44.1 and 48 kHz on Windows.
- Start, idle, blip, lift, stop, restart, preset changes, invalid commands, device changes and
  sleep and wake each work on Windows, as e2e tests on the null device plus a manual check on
  Bernardo's real devices. The C and add-on suites stay green in Linux CI and on macOS.
- The dead-man rule end to end: the window hidden with a ramped key held, and the engine
  process's messages stopped mid-pull; the snapshot's throttle reaches zero both times, and
  stays where it was for the on/off and latched styles.

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
- e2e on Windows.

**Stop/reject rule:** JavaScript never forks the physics description, and no slider combination
is claimed physically credible just because it was accepted.

**Legacy coverage:** 15.

**Starting evidence/specification:** [CLAUDE.md](../../CLAUDE.md), [docs/app-design.md](../app-design.md) §11.

**Execution record:** `docs/stage-067.md`.

<a id="stage-131"></a>
## Stage 131 — Engine layouts: any family and cylinder count

**Dependencies:** [067](#stage-067), [118](#stage-118), [123](#stage-123)

**Why:** Bernardo's request (2026-11-18): engines chosen by configuration (inline, V, boxer) and
cylinder count, not only by the five presets. The core already holds 1 to 16 cylinders in one
or two banks at any bank angle, with each cylinder's own pin, firing order, header and
collector; what's missing is something that designs a coherent engine from a family and a
count, and the tuning each preset got by hand. `cylinders` and `banks` are read-only today
because changing one alone leaves every per-cylinder list incoherent.

**Deliverable:**
- **The families and counts:**
  - inline: 1 to 6 cylinders (8 later, its own follow-up: a crank that long is a stated limit);
  - V: 2 to 16, even counts, with the bank angle in a range per count;
  - boxer: 2, 4 and 6 (8 later, as for the inline).
- **The crank where a count has several:** the V8's cross-plane and flat-plane, the V6's 60°
  even-firing, 90° odd-firing and 90° split-pin, the V-twin's angle; for the rest, the even
  firing rule (pins and firing order so the cylinders fire every 720/n degrees, primary and
  secondary balance stated), with a table of known real layouts taking precedence.
- **The core:** a new public function that fills a whole description from family, count, crank
  and bank angle: banks, pins, firing order, positions along the crank, headers grouped into
  collectors (one exhaust or one per bank), valve and runner sizes for the bore, and the
  running set (flywheel, starter, accessories, idle valve and gains, rev limit) worked out from
  the displacement and count instead of hand-tuned. Appended to rb.h, `WASM_EXPORTS` and the
  add-on; refusals stated with a reason, never a truncated engine.
- **The presets stay** as named starting points: each is a layout plus its own tuned values.
  Choosing another layout from one keeps the edits that still make sense (bore, stroke, cams,
  compression) and says which it drops.
- **The app:** a Layout control at the top of the editor (family, count, crank, bank angle),
  applied as one rebuild through 067's transaction, with the preview (the core's description
  before the build), undo, the autosave and BUSY rules; the real-time tier picked per layout
  from its measured cost.
- **Labelling:** a generated engine is generic ("V10, generic"), never presented as a real
  engine; 090–093 add researched, calibrated engines on top of this.

**Required exit evidence:**
- **Every supported combination** (each family, count and crank): the core's own events fire
  in the generated order at the stated intervals; pins, banks and firing order agree with the
  kinematics; it starts, idles 30 s without hunting (within the presets' idle tolerance), takes
  a blip and a 10 N m load, revs to its limit and stops, in bounded tests; `rb_cli fuzz` on each.
- **The five presets unchanged:** regenerating each preset's layout gives its own pins, banks
  and firing order, and the presets render bit for bit as before (`cli_parity`).
- **Capacity at 16 cylinders:** snapshots, telemetry, the add-on and the window handle every
  count up to 16, per-cylinder state included; stable IDs.
- **Speed:** each combination's cost per tier measured on the i7-1355U; the default tier for
  each plays live there, or the combination states that it can't.
- **The app:** e2e on Windows: a layout and count changed while playing, a refusal kept out,
  undo back to the preset; never half-applied.
- **The sound check:** idle and a rev of each family at a few counts rendered for Bernardo; his
  listening informs, it doesn't certify (phase B and 094 calibrate).
- Planted bugs in the generator (a wrong firing interval, a pin off, a bank swapped, the idle
  gains not scaled, a count silently clamped) caught by the tests.

**Stop/reject rule:** A generated engine is generic: no claim that it sounds like a real one.
No combination offered that the core can't build or run; no silent truncation or clamping of
the count. Hand-tuned presets aren't rewritten to fit the generator.

**Legacy coverage:** 23 (layouts, as a generator).

**Starting evidence/specification:** [docs/stage-067.md](../stage-067.md) ("What is left":
layout-fixed fields), `core/src/presets.c`, [docs/architecture.md](../architecture.md).

**Execution record:** `docs/stage-131.md`.

<a id="stage-132"></a>
## Stage 132 — The build saver

**Dependencies:** [067](#stage-067), [131](#stage-131)

**Why:** Bernardo's request (2026-11-18): keep the engines he makes and come back to them. It
pulls the saved-builds part of [095](12-release.md#stage-095) forward; 095 keeps import,
export, sharing and migration.

**Deliverable:** builds saved under a name (the preset, its layout and edits, the engine
computer's settings), listed, loaded back as one rebuild, replaced only when asked and deleted
after asking; the files the main process's alone, reached by id, never by path.

**Required exit evidence:** the files' and the window's behaviour tested; hand-edited and
foreign files neither listed nor deleted; e2e on Windows; planted bugs caught.

**Stop/reject rule:** the window never names a file path; a refused build leaves the engine
playing.

**Execution record:** `docs/stage-132.md`.

<a id="stage-068"></a>
## Stage 068 — Live instruments, telemetry and the core's dyno

**Dependencies:** [066](#stage-066), [067](#stage-067)

**Deliverable:**
- **Instruments:** rpm, MAP and boost, lambda/AFR, torque and power, the computer's cuts, an
  output meter with defined ballistics, and the engine's render-speed and underrun status, all
  from snapshots.
- **Added 2026-11-15 (the review, problem 5): the model's own warning.** The core counts
  every time one of its numerical guards steps in (the pipes' sonic guard and mass cap, the
  orifices' equalisation limit, a repaired chamber: counters in the engine, not the optional
  diagnostics taps), as a new telemetry value, guard interventions per second (a new snapshot
  version). Above a threshold set from the five presets' normal running, the readout says the
  build is outside what the model handles well. The counters must cost nothing measurable
  (`tools/speed_check.py`).
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
## Stage 069 — The first playable app, qualified on Windows

**Dependencies:** [066](#stage-066), [067](#stage-067), [068](#stage-068), [118](#stage-118). Base sound acceptance stays 063, and the final listening 098.

**Deliverable:** Qualify the engine, controls, stereo and instruments on Windows, on named
machines (Bernardo's baseline first). Define and record:
- the supported Windows versions;
- the sample rates and the latency defaults;
- the tier and its hardware demand;
- the command and telemetry budgets.

Publish the first playable build: the unsigned Windows installer as a GitHub pre-release, once
Bernardo authorises publishing.

**Required exit evidence:**
- On Bernardo's Windows machine's real audio devices (and a second Windows machine if one is
  available), the declared 15-minute stress drive: the host's deadline histogram (tail and
  maximum), underruns (zero at the default latency), and memory.
- e2e on Windows; Linux CI green; the latest macOS core run green.
- Manual start, blip, pull and long-coast listening by Bernardo.
- **Added 2026-11-15 (the review, problem 7):** the same 15-minute drive with the window
  drawing a deliberately heavy animation every frame (a stand-in for the 3D view, which shares
  the laptop's two fast cores with the render thread): zero underruns, or the measure that
  got there (the window's frame rate capped while the host's ring is under half) and its
  result. The ring's level reaches the window in what it already receives. Stage 070 starts
  from this result.

**Stop/reject rule:** A percentile alone can hide glitches; unmeasured timing keeps
qualification open. An untested system or machine gets an explicit limitation, never silently
reduced realism. A system qualified only in CI, without a real device, is listed as such.

**Decided (2026-11-08):** Windows first. macOS and Linux are qualified at stage 126 (macOS as
"CI-tested, limited device testing" while Bernardo's Mac access stays unreliable).

**Decision (Bernardo):** publishing the pre-release, and where (the public repository's GitHub
Releases being the obvious place).

**Legacy coverage:** 14; 13b-c; R17.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../sound-robustness-plan.md).

**Execution record:** `docs/stage-069.md`.
