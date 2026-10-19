# REV-BENCH — notes for Claude

## Workflow
- The roadmap lives in `PLAN.md`. When asked to "do stage N" or "next stage", read `PLAN.md` first,
  do only that stage, then update its tracker row (status + one-line note) and write `docs/stage-NN.md`.
- Validate physics natively (tests, CSV plots, WAV renders) before wiring anything into the browser.
  Use `rb_cli ... --float` for analysis renders (16-bit files carry quantisation spurs near -97 dB).
- Every stage ends with all tests green: `make test`, `npm test` (in web/), and for browser work the
  Playwright test in both modes (dev and `E2E_PREVIEW=1`).
- Check that new physics tests can fail: plant deliberate bugs in a copy and note in the stage doc
  which tests catch them. `$HOME/rb-mut9/m9.py WORKDIR NAME...` applies each named bug to a copy of
  the sources in WORKDIR, rebuilds only the test binary, runs a filtered set of tests and puts the
  file back; two work dirs run two bugs at once. The whole suite takes 60-80 s (~140 s with the
  sanitizers, so run that in three filtered parts), and processes started in the background die
  with each 3-minute call: run a few bugs per call under `timeout`, re-copy the sources before each
  batch (a killed run can leave a bug in place), and never `pkill -f` a pattern that also matches
  your own command line.

## Core (C)
- C11, SI units, double precision, no allocation anywhere in the core (malloc only in rb_alloc.c,
  for hosts), NaN/inf guards.
- Inputs are parameter ids + doubles (enum in `core/include/rb.h`, metadata row in
  `core/src/rb_params.c`); outputs are telemetry ids + doubles. New C tests go in `tests/test_main.c`.
- Internal physics modules: `gas.c` (species RB_AIR/RB_FUEL/RB_BURNED, two-range cp fits, u(T), T(u)),
  `chamber.c` (0D control volume: clear rates, add flows/heat, integrate; `rb_chamber_set_volume` does
  piston work with the time-symmetric trapezoidal rule and splits changes over 2 %), `flow.c` (nozzle
  formula, orifices with the equalisation limit, `rb_network_step`), `cylinder.c` (slider-crank
  geometry; `rb_cylinder` = chamber + geometry + crank angle, `rb_cylinder_move` returns the work),
  `valve.c` (cam lift `(1 - u²)²`; effective area = curtain in series with the port), `throttle.c`
  (butterfly plate + shaft), `combustion.c` (heat release per kg of fuel with the constant-volume
  correction, Wiebe burn, `rb_burnable_fuel` unmixedness model), `heat.c` (Woschni h and w, wall
  area), `engine.c` (N cylinders + plenum + throttle + idle air valve + exhaust manifold → catalyst
  orifice → muffler → tailpipe + two air reservoirs; fuel at IVC, spark, burn, walls; the crank,
  held or free; friction, starter, the engine computer; per-cycle IMEP, BMEP, torque etc.).
  The cp fits were generated with Cantera (`pip install cantera`, data file `nasa_gas.yaml`) in
  Claude's cloud workspace; see docs/stage-03.md. Since stage 6, RB_BURNED above 1000 K uses the
  equilibrium *apparent* cp at 40 bar (dissociation); see docs/stage-06.md.
- Stage 9 added `dyno.c` (an absorbing dyno: a PI controller on the free crank's `load_torque`, a
  reading per cycle with the inertia correction; internal, not in rb.h yet) and `flow.c`'s
  `rb_duct` (an inertial duct: `rb_duct_flow` moves gas with the current ṁ, `rb_duct_accelerate`
  updates ṁ once the chambers are integrated). In `engine.c` it added:
  - the intake runners: plenum → duct → a port chamber a quarter of the runner's volume → intake
    valve; `runner_length = 0` means none, as in stages 5–8;
  - the burn's length from the charge at IVC and the speed (`burn_duration`, 0.5–2 times
    `desc.burn_duration`, which is the four's at 3000 rpm wide open);
  - the engine computer's spark (`computer_spark`): the best torque's, with half the charge burned
    8° after TDC; at idle 15° plus 0.1° per rpm below idle, capped at the best.

  `spark_map = 1` (the default) uses that spark and `spark_map = 0` the fixed `e->spark` of stages
  6–8, so tests that set `e.spark` must also clear `spark_map`.
- Engine step order: the crank's turn (held: rpm dt; free: ω dt + ½ α dt² from the torques now,
  after the engine computer runs) → events the crank passes during the step (an event counts if
  it's in [angle, angle + step)): IVC injects the fuel, notes the trapped charge and fixes the
  cycle's burn length and spark timing, the spark sets the burnable fuel, EVO stops the burn,
  overlap TDC closes the peak-pressure bookkeeping → set areas (throttle, valves) → all flows (the
  runners' with their current ṁ), Woschni wall heat and burning as rates from the current state,
  integrated together (explicit Euler), then the runners' ṁ from the new pressures → turn the
  crank and move the pistons (trapezoidal); a free
  crank's kinetic energy then changes by exactly the work done on it (gas, crankcase at ambient,
  starter, friction, load), so `crank_books()` in the tests balances to 1e-9.
  Per-cycle totals share the step that crosses 720° between both cycles. `rb_engine_settle` runs to
  a steady state of VE, plenum pressure and net work.
- `hold_speed = 1` (the default after init) is the dyno-held crank of stages 5–6: the engine
  computer (idle valve, fuel cut, rev limiter, cranking spark) only runs with `hold_speed = 0` and
  the ignition on, so held runs keep stage 6's numbers. Below 100 rpm the burn and Woschni's gas
  velocity keep a minimum pace (`RB_GAS_PACE_RPM`).
- `rb_cli breathe` stays motored with adiabatic walls by default (stage 5's numbers; `--walls` adds
  heat transfer); `rb_cli fire` has walls and fuel; `rb_cli run` is the free engine's scripted
  drive; `rb_cli dyno` puts the free engine on stage 9's dyno. In tests, `start()`
  (test_engine.c) is motored and adiabatic, `start_fired()` the full engine held, `start_free()`
  the free crank at rest, and test_dyno.c's `run_held()` a preset held wide open at 96 kHz. Loops
  that wait for the engine must be bounded (`run_cycles`, `run_to_angle`, `run_for`): a planted
  bug that stops the crank must fail a test, not hang the suite.
- Valve timing is given cam-card style: intake centerline in degrees after overlap TDC (cycle angle
  360 + ICL), exhaust centerline before it (360 - ECL).
- Layouts (stage 8): angles in the direction the crank turns; a piston is at TDC at crank angle
  `bank_angle[bank[i]] - pin[i]`; `rb_engine_layout` derives `firing[]` from the firing order (each
  cylinder fires at its first TDC after the previous firing, cylinder 1 first, all within 720°) and
  `rb_engine_desc_check` refuses a `firing[]` that disagrees. Presets live in `presets.c` (V8s:
  odd cylinders on bank 0, even on bank 1, cylinders 1 and 2 at the front). Each preset carries
  its own idle valve, sized so the warm engine idles at about 15 % open (where the computer starts
  it), and idle gains; tune new ones with a start, a 30 % blip and a 10 N m load at idle, keeping
  room for gains 1.5 times stronger (docs/stage-08.md, retuned in docs/stage-09.md). Watch the
  idle for 30 s: a slow swing can take 15 s to show (the six's does). Engine changes that move the
  idle's torque response (the burn, friction, pipes) mean retuning them.
- The public engine API (`rb_engine_api.c`): `rb_load_preset`, `rb_get_engine`/`rb_set_engine`
  over a field table (offsets into `rb_engine_desc`, a few worked out), `rb_get_cylinder`.
  `rb_set_engine` clamps a value outside the field's range (returning RB_CLAMPED; pin and bank
  angles wrap instead), changes a copy, checks it, then rebuilds the engine (or, for RB_EF_LIVE
  fields, changes it in place). New fields: append to `rb_engine_field` in rb.h and to `FIELDS[]`,
  with a range. The
  sim holds the engine, but until stage 14 `rb_step` still plays the test tone; tests that need it
  running step `sim->engine` directly (tests/test_presets.c).
- Crank angle convention: θ from TDC; over the 720° cycle 0 = firing TDC, 180 = BDC, 360 = overlap
  TDC, 540 = BDC. Charts run from -360 to 360 so firing TDC is in the middle.
- New public C functions must also be added to `WASM_EXPORTS` in the Makefile (and to `RbExports`
  in `web/src/core/rb-core.ts` if JS uses them).
- Bernardo wants a 3D view of the engine running (stages 17–19). Physics stages must expose what it
  needs through the API: the engine description (geometry, layout, crank pin angles, valve/cam data,
  pipe geometry) and per-cylinder / per-pipe-cell state. Never duplicate engine geometry in JavaScript.
- After any change in `core/`, run `make wasm`. It rewrites `web/src/wasm/rb_core.wasm` and
  `rb_core.sources.sha256`; the web unit tests fail on a stale core.

## Tools and charts
- `tools/plot_cycle.py` draws `rb_cli motor` CSVs, `tools/plot_breathe.py` the `rb_cli breathe`
  ones (cycle, map), `tools/plot_fire.py` the `rb_cli fire` ones (cycle, spark, power),
  `tools/plot_run.py` the `rb_cli run` one, `tools/plot_presets.py` the presets' (firing, runs,
  idle), `tools/plot_dyno.py` the `rb_cli dyno` sweeps and `fire --rpm-sweep` comparisons (curves,
  changes, spark). The shared style lives in `tools/rbplot.py`: surface #fcfcfb, ink #0b0b0b /
  #52514e, muted #898781, grid #e1e0d9, series blue #2a78d6 then orange #eb6834, an ordinal blue
  ramp for ordered categories, and the Inter font when installed. Doc images are rendered in
  Claude's cloud workspace (it has Inter) and saved in `docs/`; `renders/` is git-ignored.

## Web
- Vite 8 + TypeScript 7 + Vitest 5 + Playwright 1.63, pinned exactly (pick versions at least two
  weeks old). `web/.npmrc` sets `legacy-peer-deps=true` because npm 10 crashes on vitest's optional peers.
- The worklet gets the wasm bytes in `processorOptions` and compiles its own instance synchronously.
  Nothing may allocate inside `process()`.
- 3D view: Three.js `WebGPURenderer` (automatic WebGL 2 fallback, `await renderer.init()`), shaders in
  TSL, geometry generated procedurally; motion from a display-rate snapshot streamed by the worklet,
  delayed by the audio output latency.

## Environment (device bridge)
- Deleting files in the project folder is blocked by default, so build with
  `make BUILD=$HOME/rb-build ...` (outside the mounted folder) and only write deliverables
  (source, docs, renders) into the project. `git commit` also needs delete permission (lock files).
- Commits: one per stage, at its end, on the `rev-bench` branch (don't commit to `main`). Author as
  in Bernardo's history: `git -c user.name=bernardovh77 -c user.email=you@example.com commit ...`.
  Never add Co-Authored-By or session lines to commit messages (Bernardo's request).
  Read git state with `git --no-optional-locks ...`.
- Never `npm install` inside the mounted folder. Copy REV-BENCH (without node_modules) to $HOME,
  `npm ci` in its web/, run tests there, and copy `package-lock.json` back if it changed.
- The bridge VM has gcc 11, make, node 22, python3 + numpy, zig 0.17 (`python3 -m ziglang`), git;
  no sudo. GitHub, PyPI and npm are reachable; storage.googleapis.com is not (so no Emscripten SDK).
  Its clock trails the mounted folder slightly, so make prints harmless clock-skew warnings.
- The browser e2e test runs in Claude's cloud workspace: stage the web/ files there, `npm ci`, then
  `PW_CHROMIUM=/opt/pw-browsers/chromium-1194/chrome-linux/chrome npx playwright test`
  (add `E2E_PREVIEW=1` for the production build).
- The desktop app's built-in browser can't open servers Claude starts, so Bernardo runs
  `npm run dev` himself to listen.
- `REV-BENCH` is a subfolder of the `Personal-Projects` git repo, so don't run `git init` here.

## Bernardo
- Windows + WSL. CS student, strongest in C, so keep the C readable and well commented.
