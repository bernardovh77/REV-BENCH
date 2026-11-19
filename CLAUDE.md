# REV-BENCH — notes for Claude

## Workflow
- The roadmap lives in `PLAN.md`. When asked to "do stage N" or "next stage", read `PLAN.md` first,
  do only that stage, then update its tracker row (status + one-line note) and write `docs/stage-NN.md`.
- **Stage numbers:** since 2026-10-23 PLAN.md numbers the stages from 001 (Codex's roadmap,
  docs/roadmap/). Stage numbers in this file older than that are the old ones, mapped in
  docs/roadmap/legacy-map.md. For example, the old 14 (the engine in the browser) is now 066/069,
  the old 17–19 (the 3D view) are 070–077, and S1/S5 are spread over 040–063 and 098. "Stage 14"
  in PLAN.md is a finished early stage, not the browser.
- Codex (OpenAI's agent) also works on this repo: AGENTS.md is its instructions and CODEX.md its
  dated handoff log. Read the log's latest entry and PLAN.md's "Current state" before working,
  and add a short entry there for what you did.
- Validate physics natively (tests, CSV plots, WAV renders) before wiring anything into the browser.
  Use `rb_cli ... --float` for analysis renders (16-bit files carry quantisation spurs near -97 dB).
- **How to build a stage** (the method, and the approach to each remaining one) is
  docs/stage-playbook.md; what the 2026-11-15 review found is docs/review-2026-11-15.md.
- Every stage ends with all tests green: `python tools/run_tests.py` runs every suite, on
  Windows or WSL (docs/workspace.md). The old browser page's Playwright test (`e2e`, both
  modes) runs only when named, for work on `web/` itself (stage 127).
- **The push hook** (`.githooks/pre-push`; `git config core.hooksPath .githooks` once per
  clone) runs the quick suites and refuses a push that fails them. Don't skip it
  (`RB_SKIP_PREPUSH=1`) for a commit that goes to `main`. Stages that touch `core/` also do the
  sound check (PLAN.md, "How a stage is done", step 6).
- **Before a stage, check the open gaps** (PLAN.md's 🟡 rows, stage docs' "What is left") and
  say which could influence it: those are fixed first, the rest named as safe to leave
  (Bernardo, 2026-11-09). **A stage that touches `core/` runs `tools/speed_check.py`** (PLAN.md,
  "How a stage is done", 6b): a median slowdown over 3 % fails, and a deliberate one is stated
  with the tier it affects.
- Check that new physics tests can fail: plant deliberate bugs in a copy and note in the stage doc
  which tests catch them. `python tools/plant_bugs.py STAGE WORKDIR NAME...` (or `all`; `--list`)
  applies each bug of `tools/bugs/stageSTAGE.py` (a stage's own table: start from the last one's)
  to a fresh copy of the sources in WORKDIR, builds the test binary there and runs the tests named
  for it; two work dirs run two bugs at once. `python tools/plant_bugs.py --check` (in the Python
  suite) fails if any table's bug no longer matches today's sources: a change that moves a bug's
  text retargets the bug (or retires it with a note in its table) in the same stage. The whole suite takes 5–9 min in Claude's cloud
  workspace (3.5 at stage 12c; more with the sanitizers, so run that in filtered parts), and
  processes started in the background can die with a call: run a few bugs per call under
  `timeout`, and never `pkill -f` a pattern that also matches your own command line.
- make rebuilds every object whose headers changed (-MMD), if the object is in the Makefile's
  ALL_OBJ: a new program's objects go there too. Stage 13's segfault after a header change was
  rb_bench.o, left out of it.

## Sound: Bernardo's top priority
- **Bernardo's own references:** recordings and videos of the engines the presets are modelled on
  (docs/user-sound-references.md, docs/video-sound-references-2026-10-24.md) are on his machine
  only, in `renders/` (git-ignored). A cloud session can't see them; ask him to attach them, or
  for a session started on his machine (docs/workspace.md).
- **What Bernardo said:** after stage 12's renders, "dull and really artificial" (only the
  cross-plane V8's sweep came close). He asked for research and comparisons with real engines,
  and for the refining of the sound to be treated as very important. After stage 12b's: "the
  3000 on all engines feel like they should be the idle", "too smooth... less like a piano
  note", the limiter's "huge gap", the deceleration "very off" (stage 12c's work).
- **Where it lives:** RESEARCH.md's "Sound realism" has the findings. PLAN.md (remade
  2026-11-01) acts on them in phase B, stages 039 and 050–063, after the engine plays live in
  the desktop app (phase A: 120, 121, 122, 118, 123, 124, 066–069). Every stage that touches `core/` does the sound check
  (PLAN.md, "How a stage is done", step 6). Never trade sound for speed silently.
- **Measuring:** `tools/compare_real.py` measures renders like the recordings of real engines in
  `tools/real_engines.csv` (Freesound, CC0/CC BY/Sampling+). `fetch` puts the recordings in
  `renders/real/` once (needs ffmpeg), then `compare [--kind steady|rev] WAV@T0-T1:LABEL...`.
  `tools/orders.py CYL WAV@T0-T1@RPMLO-RPMHI:LABEL...` measures the orders between the firing
  ones: how differently the cylinders sound. Recordings made "at the exhaust" have the jet
  blowing on the microphone: their low band varies more than a listener's would.
- **The numbers so far** (how far each measure is from real engines', stage by stage) are in
  [docs/architecture.md](docs/architecture.md#the-sounds-numbers-so-far).

## Core (C): the rules

The reference (each module, the engine's step order, the conventions, what each stage added)
is [docs/architecture.md](docs/architecture.md): read the part about a module before changing
it, and update it in the same stage. The rules that hold everywhere:

- C11, SI units, double precision, no allocation anywhere in the core (malloc only in
  rb_alloc.c, for hosts), NaN/inf guards. No GNU-only constructs (`__builtin_*`, statement
  expressions, VLAs): MSVC builds it too.
- Readable C: `if (x) y;`, spaces round operators, one statement a line, a comment on what
  each function does. A pure reformat or move must leave `make wasm`'s output and the
  `-O2 -g0` objects byte-identical (docs/stage-119.md).
- Inputs are parameter ids + doubles, outputs telemetry ids + doubles (`core/include/rb.h`).
  New ones are appended: a parameter to `rb_param` and `PARAMS[]`, applied in `rb_sim_apply`;
  an engine field to `rb_engine_field` and `FIELDS[]` with a range; engine telemetry before
  `RB_TLM_COUNT`, which is a new snapshot version. New public functions also go in
  `WASM_EXPORTS` (Makefile) and, if JS uses them, `RbExports` (`web/src/core/rb-core.ts`).
- New C tests go in `tests/` and are listed in `tests/test_main.c`. Loops that wait for the
  engine are bounded (`run_cycles`, `run_to_angle`, `run_for`): a planted bug that stops the
  crank must fail a test, not hang the suite. Tests that set `e.spark` also clear `spark_map`.
- `variation` is 0 by default so tests and dyno numbers stay exact; `rb_cli sound` uses 1.
- After any change in `core/`: `make wasm` (the web tests fail on a stale core), the sound
  check and `tools/speed_check.py` (PLAN.md, "How a stage is done", 6 and 6b).
- Anything that moves the idle's torque response (the burn, friction, pipes, cams) means
  retuning the presets' idle; watch an idle for 30 s. A new part is fuzzed (`rb_cli fuzz`).
- Renders at the high quality are the reference and must not change unless the stage says so:
  `tools/cli_parity.py` (bit-identical within one compiler), `tools/compiler_parity.py`
  (across compilers).
- The 3D view (stages 070–077) draws from the API: the engine description and per-cylinder /
  per-pipe-cell state. Never duplicate engine geometry in JavaScript.
- `RB_PARAM_VARIANT` (stage 127) is the live A/B switch for a candidate sound: 0 is the
  accepted sound, and a candidate under listening reads `sim->variant`. A candidate that loses
  is deleted and its number retired.

## Tools and charts

Each tool, and the charts' shared style, are listed in
[docs/architecture.md](docs/architecture.md#tools-and-charts). `tools/plot_sound.py` reads
16-bit WAVs only (render without `--float`); every other analysis uses float renders.

## The app (since 2026-11-08)
- REV-BENCH is a desktop app for Windows, macOS and Linux, not a browser page: docs/app-design.md
  is the design and docs/adr/0004-desktop-application.md the decision. In short:
  - Electron's main process for windows, menus and updates;
  - an engine utility process that loads a Node-API add-on (C) wrapping `host/`, the real-time
    host, whose render thread runs the core and fills a ring that miniaudio plays;
  - a sandboxed Svelte 5 + TypeScript renderer for the interface and Three.js 3D, talking to the
    engine process through one MessagePort (commands, acks, snapshot-v3 snapshots).
- Stage 124 built it (docs/stage-124.md): `app/native/` (the add-on, `python tools/build_addon.py`),
  `app/src/{main,engine,preload,ui,shared}`; the contracts `app/src/shared/addon.ts` and
  `protocol.ts` change on both sides at once. Suites `addon`, `app`, `app-e2e` (never the real
  audio device); `tools/package_app.py` makes the installer. The engine process builds every
  engine with `hold` 0: the core's default holds the crank still.
- No physics or audio in JavaScript. The render thread never allocates, locks or logs. Rebuilds
  are prepared off the thread and swapped at a block boundary with a fade (runtime-v1).
- Windows first (app-design §1): until stage 126 every app stage is finished and tested on
  Windows (MSVC), keeps Linux CI green (gcc: the C, the add-on and the interface's tests) and
  the core's tests passing in macOS's weekly run (Apple clang). Strict doubles, no FMA
  contraction everywhere. System-specific code only in the platform files
  (`host/src/platform_*.c`, `app/src/main/platform.ts`), paths never with hard-coded
  separators. Stage 126 ports the app to macOS and Linux; from then on a feature that works on
  one system only is not done.
- Pin every dependency exactly, at versions at least two weeks old, with its licence noted for
  THIRD_PARTY_NOTICES.
- The CMake build (stage 122, `CMakeLists.txt`): the core, rb_cli and the tests with each
  system's compiler; Release by default (set before `project()`, or MSVC makes it Debug); the
  core's warnings are errors (`RB_WERROR`). On Windows `python tools/msvc_env.py cmake ...`
  runs it with MSVC from any shell (it loads `vcvars64.bat` through vswhere); CMake and Ninja
  come from tools/requirements-dev.txt. `run_tests.py cmake` configures, builds and tests.
  New sources under core/src, tests/ are picked up; a new program goes in both the Makefile
  and CMakeLists.txt. Keep the core free of GNU-only constructs (`__builtin_*`, statement
  expressions, VLAs): MSVC refuses them.
- Across compilers renders can't be bit-identical (libm's last bits), and a last bit can flip
  a random decision (the limiter's soft cut), after which each render is its own run.
  `tools/compiler_parity.py check A B DIR`: each drive's first quarter within 1e-6 of the
  sound, then the whole drive's level, octave bands and mean rpm (docs/stage-122.md). Within
  one compiler renders must be bit-identical (`tools/cli_parity.py`).
- CI job logs and summaries don't load in the desktop app's browser; annotations do: a failed
  test step raises its table and last lines as one (`tools/ci_summary.sh`).
- CI (`.github/workflows/ci.yml`): Windows (every suite but e2e with MSVC and w64devkit, then
  MSVC against gcc) and Linux (gcc, plus a sanitizer job) on every push to `main`; macOS
  (Apple clang) Mondays and on demand. Actions are pinned to commits.

## Web (the WebAssembly build and the old browser scaffold)
- `web/` keeps the WebAssembly core, its binding and tests as portability checks; the browser
  bench is no longer the product (an optional browser demo is stage 125).
- Vite 8 + TypeScript 7 + Vitest 5 + Playwright 1.63, pinned exactly (pick versions at least two
  weeks old). `web/.npmrc` sets `legacy-peer-deps=true` because npm 10 crashes on vitest's optional peers.
- The worklet gets the wasm bytes in `processorOptions` and compiles its own instance synchronously.
  Nothing may allocate inside `process()`.
- 3D view: Three.js `WebGPURenderer` (automatic WebGL 2 fallback, `await renderer.init()`), shaders in
  TSL, geometry generated procedurally; motion from a display-rate snapshot streamed by the worklet,
  delayed by the audio output latency.

## Environment
- Build outside the repo, `make BUILD=$HOME/rb-build ...` (`tools/run_tests.py` uses
  ~/rb-build/tests), and only write deliverables (source, docs, renders) into the project.
  Bernardo's recordings and other bulky media go in `renders/` (git-ignored, as are `*.zip`,
  `*.webm`, `*.wav` and the like anywhere): moved, never deleted.
- Commits: one per stage, at its end, on `main` (or the branch a session is told to use). Author as
  in Bernardo's history: `git -c user.name=bernardovh77 -c user.email=you@example.com commit ...`.
  Never add Co-Authored-By or session lines to commit messages (Bernardo's request).
  Read git state with `git --no-optional-locks ...`.
- Never `npm install` inside the repo. `tools/run_tests.py` copies it (without node_modules) to
  ~/rb-build/tests/web-copy and runs `npm ci` and the web tests there; for a dependency change,
  do the same by hand and copy `package-lock.json` back.
- Every suite runs on native Windows and in WSL alike, neither needing the other:
  `python tools/run_tests.py` (with the Python of `tools/requirements-dev.txt`; setup in
  docs/workspace.md). On Windows, w64devkit's gcc and make build without LTO (the Makefile
  checks). Tools pass make forward-slash paths, read and write text as UTF-8, find programs with
  `checks.program()` (Windows adds `.exe`) and FFmpeg with `ffmpeg_path.find_ffmpeg()`. Tests that
  fake a program use `tools/fake_program.py`. If WSL hangs, work on Windows rather than restart it.
- `tools/run_tests.py` checks first that make and Node 22+ are there (exit 2 if not), stops at
  the first failed suite unless `--keep-going`, and kills a suite (with everything it started)
  that runs past its limit (`TIMEOUTS`). On Windows, `python` alone may be the system Python:
  use the venv's, `C:\Users\berna\rb-venv\Scripts\python`.
- The e2e test runs wherever the others do (`python tools/run_tests.py e2e`, both modes): with the
  installed Chrome or Edge on Windows, Playwright's Chromium in WSL, and in Claude's cloud
  workspace with `PW_CHROMIUM=/opt/pw-browsers/chromium-1194/chrome-linux/chrome`.
- The desktop app's built-in browser can't open servers Claude starts, so Bernardo runs
  `npm run dev` himself to listen.
- REV-BENCH is its own git repo (github.com/bernardovh77/REV-BENCH, default branch `main`). Until
  stage 13 it was a folder of Bernardo's Personal-Projects repo, which its history was split out
  of. On his machine the clone is its own folder beside Personal-Projects,
  `/mnt/c/Users/berna/REV-BENCH` (inside Personal-Projects until stage 12b; docs/workspace.md).

## Claude's cloud sessions
- .claude/settings.json runs `.claude/hooks/session-start.sh` when a session starts (it prints
  "REV-BENCH tools ready"); if it didn't, run `bash .claude/hooks/session-start.sh` first. It
  installs what the workflow uses and skips what's there: ~/rb-venv with zig 0.17
  (`~/rb-venv/bin/python -m ziglang`), numpy, matplotlib and graphify 0.9.92 (linked into
  /usr/local/bin), and the web app's node_modules in ~/rb-copy/web. Outside the cloud
  (CLAUDE_CODE_REMOTE unset) it does nothing.
- The web tests run in that copy: from the repo's root, `tar --exclude=web/node_modules
  --exclude=web/dist --exclude=web/test-results --exclude=build --exclude=renders -cf - . |
  (cd ~/rb-copy && tar -xf -)`, then `npm test` in ~/rb-copy/web (the e2e test as above).

## Bernardo
- Windows + WSL. CS student, strongest in C, so keep the C readable and well commented.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- When graphify-out/graph.json exists, run `graphify query "<question>"` before your first grep, search or source-file read for a codebase question, and put this rule in every subagent prompt that explores code. Read raw files only for lines the graph or an earlier read already located. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
- A committed graphify-out/graph.json is rewritten whole on every build, so a large diff to it in a pull request is expected. Do not flag it as churn or suggest skipping the graph rebuild or its hooks.

## The graph in this repo

graphify's section above assumes graphify is installed. In this repo:
- Run graphify from the repo's root, where graphify-out/ is.
- In cloud sessions the setup script above installs graphify. Wherever it's missing
  (`command -v graphify`): GRAPH_REPORT.md first to find the files, then only those; don't read
  graph.json or graph.html (about 1 MB each); and say in the stage's report that the graph needs
  a `graphify update .`.
- .claude/hooks/graphify-guard.sh is graphify's reminder to query first, which .claude/settings.json
  runs before searches and reads (`sh .claude/hooks/graphify-guard.sh search|read`). It finds
  graphify on PATH or in ~/.local/bin and stays quiet in cloud sessions, where it would only
  start Python before every call.
- On Windows graphify writes CRLF line endings. Git stores LF whatever the working tree has
  (`.gitattributes`: `eol=lf`; `core.autocrlf` is off for this repo since stage 120), so the
  diff is unaffected; `git ls-files --eol | grep w/crlf` lists such files if a tool minds.
- GRAPH_REPORT.md names each community after one of its members ("math", "titles"), not a
  topic: look at what's in it.
- graphify can't parse core/include/rb.h (its `extern "C"` guard), and headers give it no
  symbols: the API's declarations aren't in the graph, its definitions (rb_sim.c, ...) are.
- The graph trails the code until someone runs `graphify update .`: where they disagree, the
  code is right.
- Committed: graph.json, graph.html, GRAPH_REPORT.md, .graphify_analysis.json and the
  .graphify_labels files. cache/, manifest.json, .graphify_root (the absolute path of the folder
  it scanned) and .cloud-cta-shown are each machine's own and git-ignored: graphify remakes them,
  and without a manifest the next update just re-checks every file. So are its dated backups
  (graphify-out/2026-10-20/) and .graphify_labels.pending.json, the names it gave new groups.
