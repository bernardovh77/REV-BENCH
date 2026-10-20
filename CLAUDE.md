# REV-BENCH — notes for Claude

## Workflow
- The roadmap lives in `PLAN.md`. When asked to "do stage N" or "next stage", read `PLAN.md` first,
  do only that stage, then update its tracker row (status + one-line note) and write `docs/stage-NN.md`.
- Validate physics natively (tests, CSV plots, WAV renders) before wiring anything into the browser.
  Use `rb_cli ... --float` for analysis renders (16-bit files carry quantisation spurs near -97 dB).
- Every stage ends with all tests green: `make test`, `npm test` (in web/), and for browser work the
  Playwright test in both modes (dev and `E2E_PREVIEW=1`). Stages that touch `core/` also do the
  sound check (PLAN.md, working agreement 6).
- Check that new physics tests can fail: plant deliberate bugs in a copy and note in the stage doc
  which tests catch them. `$HOME/rb-mut12/m12.py WORKDIR NAME...` (each stage's runner has its own
  table of bugs; copy the last one) applies each named bug to a copy of
  the sources in WORKDIR, rebuilds only the test binary, runs a filtered set of tests and puts the
  file back; two work dirs run two bugs at once. The whole suite takes 60-80 s (~140 s with the
  sanitizers, so run that in three filtered parts), and processes started in the background die
  with each 3-minute call: run a few bugs per call under `timeout`, re-copy the sources before each
  batch (a killed run can leave a bug in place), and never `pkill -f` a pattern that also matches
  your own command line.

## Sound: Bernardo's top priority
- **What Bernardo said:** after stage 12's renders, "dull and really artificial" (only the
  cross-plane V8's sweep came close). He asked for research and comparisons with real engines,
  and for the refining of the sound to be treated as very important.
- **Where it lives:** RESEARCH.md's "Sound realism" has the findings. PLAN.md's sound track
  (stages S1–S4 after stage 13, S5 at the end) acts on them, and working agreement 6 adds a sound
  check to every stage. Never trade sound for speed silently.
- **Measuring:** `tools/compare_real.py` measures renders like the recordings of real engines in
  `tools/real_engines.csv` (Freesound, CC0/CC BY). `fetch` puts the recordings in
  `renders/real/` once (needs ffmpeg), then `compare [--kind rev] WAV@T0-T1:LABEL...`.
- **The numbers that matter** (medians at idle):
  - **Repeating:** a real engine's cycle repeats at about +5 dB below 500 Hz and under 0 dB
    above. Ours repeats at +41 to +57 dB, an exact loop: that is the "artificial".
  - **High frequencies:** ours falls off a cliff above 2 kHz, the pipes' grid (it carries nothing
    above c/(πΔx): 2.4 kHz in the 7.5 cm exhaust cells). Real engines keep a broadband floor to
    16 kHz (−25 dB of their energy above 2 kHz, ours −59): that is the "dull".
  - **The highs are the gap, not the mids.** Under load our energy above 1 kHz is already the
    real engines' (−14 dB).
- **How engine-sim does it:** its physics stops at 1.9 kHz too. Its highs come from time jitter,
  noise multiplied into the flow and a convolution with an impulse response, with a leveler at
  the end.

## Core (C)
- C11, SI units, double precision, no allocation anywhere in the core (malloc only in rb_alloc.c,
  for hosts), NaN/inf guards.
- Inputs are parameter ids + doubles (enum in `core/include/rb.h`, metadata row in
  `core/src/rb_params.c`); outputs are telemetry ids + doubles. New C tests go in `tests/test_main.c`.
- Internal physics modules: `gas.c` (species RB_AIR/RB_FUEL/RB_BURNED, two-range cp fits, u(T), T(u)),
  `chamber.c` (0D control volume: clear rates, add flows/heat, integrate; `rb_chamber_set_volume` does
  piston work with the time-symmetric trapezoidal rule and splits changes over 2 %), `flow.c` (nozzle
  formula, orifices with the equalisation limit, `rb_network_step`), `pipe.c` (stage 10's 1D pipe:
  cells = chambers on a staggered grid, faces whose ṁ is a state; ends closed, joined to a volume
  (or another pipe's cell) with the end correction, restricted, laminar loss, or on an orifice, or
  going straight on into another pipe (`rb_pipe_onward`, stage 12); two rates; stage 12's packing,
  a laminar loss on the flow's swings only), `cylinder.c` (slider-crank
  geometry; `rb_cylinder` = chamber + geometry + crank angle, `rb_cylinder_move` returns the work),
  `valve.c` (cam lift `(1 - u²)²`; effective area = curtain in series with the port), `throttle.c`
  (butterfly plate + shaft), `combustion.c` (heat release per kg of fuel with the constant-volume
  correction, Wiebe burn, `rb_burnable_fuel` unmixedness model), `heat.c` (Woschni h and w, wall
  area), `audio.c` (the listener: stage 11's monopoles, the tailpipes' and since stage 12 the
  intake mouth's volume flow, heard from a place around the car, each at its distance and delay
  and off the ground; the Kaiser-window anti-alias FIR and decimator; the DC blocker; the soft
  limiter), `engine.c` (N cylinders, the air → snorkel (pipe; the filter a laminar loss at its
  end) → air box (volume) → throttle + idle air valve → plenum, a runner per cylinder (pipe) →
  intake valve, exhaust valve → header (pipe) → one of up to two collectors (volumes) → each
  collector's exhaust pipe → catalyst (a restriction + laminar loss at its end) → muffler (pipe:
  a chambered can, a packed straight-through or a straight pipe) → tailpipe (pipe; a chambered
  muffler's baffles at its start) → the air, one or two systems; two air reservoirs; fuel at IVC,
  spark, burn, walls; the crank, held or free; friction, starter, the engine computer;
  per-cycle IMEP, BMEP, torque etc.).
  The cp fits were generated with Cantera (`pip install cantera`, data file `nasa_gas.yaml`) in
  Claude's cloud workspace; see docs/stage-03.md. Since stage 6, RB_BURNED above 1000 K uses the
  equilibrium *apparent* cp at 40 bar (dissociation); see docs/stage-06.md.
- Stage 9 added `dyno.c` (an absorbing dyno: a PI controller on the free crank's `load_torque`, a
  reading per cycle with the inertia correction; internal, not in rb.h yet). In `engine.c` it added:
  - the burn's length from the charge at IVC and the speed (`burn_duration`, 0.5–2 times
    `desc.burn_duration`, which is the four's at 3000 rpm wide open);
  - the engine computer's spark (`computer_spark`): the best torque's, with half the charge burned
    8° after TDC; at idle 15° at the idle speed, more below, less above, capped at the best.

  `spark_map = 1` (the default) uses that spark and `spark_map = 0` the fixed `e->spark` of stages
  6–8, so tests that set `e.spark` must also clear `spark_map`.
- Stage 10 made the runners (`runner_length = 0`: none, as in stages 5–8) and the exhaust 1D pipes
  (`pipe.h` explains the scheme). Cells about every 5 cm in runners and headers, 7.5 cm in exhaust
  pipes and tailpipes, from a pool of 512 in the engine. The pipes step at 48 kHz (`pipe_every`
  physics steps), but the cells at the valves are fast, stepped every physics step; fast faces see a
  slow neighbour's pressure as it is by now (`rb_chamber_pressure_now`: the gas it took meanwhile and
  its rates since the block began). Get this wrong and the engine's VE moves up to 0.9 % between
  384 and 96 kHz (docs/stage-10.md, "Two rates"; `test_engine_pipes_two_rates`). The engine
  computer's idle valve and idle spark act on the speed half a second ahead (`idle_heading`), the
  spark 0.3° per rpm: with runners an idling engine is unstable on its own (less exhaust pushed
  back into them the faster it turns). `heat_transfer = 0` means adiabatic cylinders; the pipes'
  walls always trade heat. At 96 kHz the valves' 5 cm cells are too stiff for the flow cap (10 %
  more pumping work motored); 192 kHz is converged.
- Stage 11 put the sound in `rb_render_block`. Every physics step the sim's source (`sim->source`,
  internal: stage 1's tone by default, which the browser plays until stage 14, or
  `RB_SOURCE_ENGINE`, which `rb_cli sound` and the tests set) gives the microphone's signal in
  full-scale units (1 = 40 Pa, `RB_FULL_SCALE_PA`); a NaN becomes 0 there, before the filters. Then
  the anti-alias FIR (157 taps at oversample 4: 0.4 ms), the 10 Hz DC blocker, the gain and the soft
  limiter (knee 0.8: a full-level tone peaks at 0.952, so level tests use 0.7). The engine's
  `tail_flow` is the tailpipes' volume flow at the end cell's density, whichever way it flows: the
  outside air's density for a reversing flow kinks it, a click every firing. A rebuilt engine
  resets the microphone. 16-bit WAVs are TPDF-dithered by default (`wav_set_dither(w, 0)` for exact
  values).
- Stage 12 made the microphone a listener (`rb_listener`, `sim->hear`, `rb_sim_listen` after any
  rebuild): it stands at an `RB_HEAR_*` place (default `RB_HEAR_BEHIND`, 3 m behind the car: a loud
  V8 a metre from its tailpipe passes full scale) and hears tailpipe 0, tailpipe 1 and the intake's
  mouth (`mouth_flow`, < 0 drawing in) each at its 1/r and delay, and again off the ground (an image
  source, `RB_GROUND` 0.9). Their flows are kept at the pipe rate in a 2048-step ring. The muffler
  is a pipe (`muffler_type`: none, a straight pipe the tailpipe's bore; straight-through, that pipe
  packed, `PACKING` and `PACKING_SETTLE` in engine.c; chambered, a can as wide as `muffler_volume`
  over `muffler_length`, left through `muffler_area` with `BAFFLE_LAMINAR`). A can's exhaust pipes
  open into its first cell and its tailpipe starts from its last, joined as to volumes; a straight
  pipe or straight-through goes straight on (`rb_pipe_onward`) from its exhaust pipe, unless two
  meet there, and into its tailpipe. Joined through a volume, a straight pipe lost two velocity
  heads and cost more back pressure than the can. The intake: snorkel → air box (fast, with
  the plenum) → throttle; the engine's books (`mass_in` etc.) count at the snorkel's mouth, not the
  throttle. Pipes: runners, headers, exhaust pipes, tailpipes, then mufflers and the snorkel (new
  kinds and places appended, so old indices hold). Packing adds weight to slow swings
  (R τ per m³: keep it under the gas's density). The boom at 1500–2500 rpm in the four is the whole
  exhaust's lowest ring (~63 Hz), not the can's alone; the rasp above 2 kHz with a straight pipe or
  straight-through is made at the tailpipe's open end, where the flow turns round every firing.
- Engine step order: the crank's turn (held: rpm dt; free: ω dt + ½ α dt² from the torques now,
  after the engine computer runs) → events the crank passes during the step (an event counts if
  it's in [angle, angle + step)): IVC injects the fuel, notes the trapped charge and fixes the
  cycle's burn length and spark timing, the spark sets the burnable fuel, EVO stops the burn,
  overlap TDC closes the peak-pressure bookkeeping → set areas (throttle, valves) → when a block of
  `pipe_every` steps begins, the slow gas's rates (`slow_flows`: the pipes' slow faces and the
  collectors, all cleared before any flows) → all fast flows (throttle, valves, the pipes' fast
  faces with their current ṁ, the snorkel's into the air box), Woschni wall heat and burning as
  rates from the current state, integrated together (explicit Euler) → turn the crank and move the
  pistons (trapezoidal) → when the block ends, the slow gas integrated over it, the snorkel's
  inflow and the tailpipes' outflow counted, its faces' ṁ updated (`slow_step`) → the fast faces'
  ṁ from the new pressures. A free crank's kinetic energy changes by exactly the work done
  on it (gas, crankcase at ambient, starter, friction, load), so `crank_books()` in the tests
  balances to 1e-9.
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
  room for gains 1.5 times stronger (docs/stage-08.md, retuned in docs/stage-09.md and
  docs/stage-10.md). Watch the idle for 30 s: a slow swing can take 15 s to show. To tell the
  controller from the engine, hold the idle valve and spark still: since stage 10 the six and the
  cross-plane V8 swing on their own then. Engine changes that move the idle's torque response (the
  burn, friction, pipes, cams) mean retuning them.
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
  idle), `tools/plot_pipes.py` the `rb_cli pipes` ones (pulse, engine) and a stage-to-stage
  `fire --rpm-sweep` comparison, `tools/plot_dyno.py` the `rb_cli dyno` sweeps and
  `fire --rpm-sweep` comparisons (curves, changes, spark), `tools/plot_sound.py` the `rb_cli sound`
  WAVs (check: levels, DC, clicks and lines; spectrograms; a sweep with its orders; engines by
  order; pulls' dBA against rpm; 16-bit WAVs only, so render without `--float`),
  `tools/compare_real.py` the renders against recordings of real engines (see "Sound" above).
  The shared style lives in `tools/rbplot.py`: surface #fcfcfb, ink #0b0b0b / #52514e, muted
  #898781, grid #e1e0d9, series blue #2a78d6 then orange #eb6834, an ordinal blue ramp for ordered
  categories, and the Inter font when installed. Doc images are rendered in Claude's cloud
  workspace (it has Inter) and saved in `docs/`; `renders/` is git-ignored.

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

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- When graphify-out/graph.json exists, run `graphify query "<question>"` before your first grep, search or source-file read for a codebase question, and put this rule in every subagent prompt that explores code. Read raw files only for lines the graph or an earlier read already located. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
- A committed graphify-out/graph.json is rewritten whole on every build, so a large diff to it in a pull request is expected. Do not flag it as churn or suggest skipping the graph rebuild or its hooks.
