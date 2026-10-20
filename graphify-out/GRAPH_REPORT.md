# Graph Report - rev-bench  (2026-10-20)

## Corpus Check
- 114 files · ~439,895 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 9 file(s) not represented in the graph (top: (none) 5, .csv 1, .css 1)

## Summary
- 1470 nodes · 4107 edges · 63 communities (52 shown, 11 thin omitted)
- Extraction: 70% EXTRACTED · 30% INFERRED · 0% AMBIGUOUS · INFERRED: 1222 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9662bb59`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_audio.c
- rb_cylinder_move
- Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧
- pipe.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- wav.c
- main.ts
- presets.c
- package.json
- titles
- RbExports
- cmd_info
- RbSim
- style
- label
- plot_fire.py
- .process
- plot_dyno.py
- RbCore
- matplotlib_pyplot
- compilerOptions
- compare_real.py
- Stage 1 — Native C scaffold, WAV writer, test runner
- math
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- rb_engine_step
- Stage 9 — Dyno + validation against real engines
- test_noise.c
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- rb_engine_init
- rb_destroy
- Stage 8 — Engine layouts & presets
- Stage 13 — Performance & stability pass
- Phase 5 — Forced induction & realism
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- rb_create
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Stage 3 — Gas & flow primitives
- Stages
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- Phase 2 — Sound (the realistic part)
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- rbplot.py
- REV-BENCH
- bench_wasm.mjs
- throttle.c
- Phase 3 — Browser
- presets.c
- graphify-guard.sh
- session-start.sh
- The workspace
- Sound realism (October 2026, after stage 12)

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 91 edges
2. `rb_engine_init()` - 51 edges
3. `RbExports` - 51 edges
4. `Core (C)` - 41 edges
5. `rb_engine_settle()` - 36 edges
6. `rb_engine_init_at()` - 35 edges
7. `style()` - 33 edges
8. `titles()` - 33 edges
9. `rb_engine_desc_default()` - 29 edges
10. `rb_create()` - 29 edges

## Surprising Connections (you probably didn't know these)
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  PLAN.md → core/src/chamber.c
- `Unequal headers (`core/src/presets.c`)` --references--> `rb_engine_desc_default()`  [INFERRED]
  docs/stage-12c.md → core/src/engine.c
- `Stage 8 — Engine layouts & presets` --references--> `rb_engine_layout()`  [INFERRED]
  PLAN.md → core/src/engine.c
- `The layout` --references--> `rb_engine_desc_check()`  [INFERRED]
  docs/stage-08.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (63 total, 11 thin omitted)

### Community 0 - "test_audio.c"
Cohesion: 0.17
Nodes (23): config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+15 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (46): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+38 more)

### Community 2 - "Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "pipe.c"
Cohesion: 0.06
Nodes (93): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_init(), rb_chamber_init_reservoir() (+85 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.14
Nodes (35): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+27 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.08
Nodes (63): rb_chamber_gamma(), rb_engine_desc_check(), rb_engine_displacement(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift() (+55 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.08
Nodes (40): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+32 more)

### Community 7 - "wav.c"
Cohesion: 0.14
Nodes (28): Dither, build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32() (+20 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "presets.c"
Cohesion: 0.07
Nodes (27): A muffler is a pipe, Calibration, Cost, Equal and unequal headers, Every source at its own distance, Going straight on, Packing, Review of stage 11 (+19 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "titles"
Cohesion: 0.14
Nodes (14): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+6 more)

### Community 13 - "cmd_info"
Cohesion: 0.26
Nodes (17): rb_preset_key(), rb_preset_name(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags() (+9 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "label"
Cohesion: 0.24
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 17 - "plot_fire.py"
Cohesion: 0.17
Nodes (9): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), spark_text() (+1 more)

### Community 18 - ".process"
Cohesion: 0.15
Nodes (5): How the sound gets out, RevBenchProcessor, makeImports(), renderSeconds(), setup()

### Community 19 - "plot_dyno.py"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 21 - "matplotlib_pyplot"
Cohesion: 0.25
Nodes (9): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+1 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "compare_real.py"
Cohesion: 0.11
Nodes (17): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+9 more)

### Community 24 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.33
Nodes (5): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 25 - "math"
Cohesion: 0.08
Nodes (41): rb_fuel_heat(), rb_wiebe(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init() (+33 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.08
Nodes (24): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes (+16 more)

### Community 28 - "rb_engine_step"
Cohesion: 0.05
Nodes (107): Core (C), rb_burnable_fuel(), mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), accessory_torque(), add_share() (+99 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "test_noise.c"
Cohesion: 0.07
Nodes (45): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+37 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.10
Nodes (20): A pipe end is a monopole, Cost, DC blocker and soft limiter, Down to the audio rate, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧 (+12 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, Tests that changed, The cabin (+13 more)

### Community 33 - "rb_engine_init"
Cohesion: 0.07
Nodes (61): arriving(), bessel_i0(), cabin(), cabin_design(), distance(), noise_arriving(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+53 more)

### Community 34 - "rb_destroy"
Cohesion: 0.21
Nodes (24): rb_preset_find(), rb_destroy(), rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that can fail, Tests that changed, engine_sim() (+16 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 36 - "Stage 13 — Performance & stability pass"
Cohesion: 0.11
Nodes (17): Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls (+9 more)

### Community 37 - "Phase 5 — Forced induction & realism"
Cohesion: 0.50
Nodes (4): Phase 5 — Forced induction & realism, Stage 20 — Turbocharger physics, Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧, Stage 22 — Knock, mechanical limits & engine damage

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.33
Nodes (6): 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound, REV-BENCH — Research notes (stage 0)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 40 - "rb_create"
Cohesion: 0.42
Nodes (6): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), test_core_host_helpers()

### Community 41 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "Stages"
Cohesion: 0.25
Nodes (8): Phase 0 — Foundations, Phase 4 — 3D engine view, Stage 17 — 3D engine model: block + moving parts, Stage 18 — 3D engine in motion, in sync with the sound, Stage 19 — 3D gas, combustion & exhaust flow, Stage 1 — Native C scaffold, WAV writer, test runner, Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet, Stages

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.29
Nodes (6): Equations used, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.29
Nodes (4): Architecture (decided up front), Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 47 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.33
Nodes (6): Phase 2b — Sound realism (top priority) 🎧, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 48 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 50 - "rbplot.py"
Cohesion: 0.31
Nodes (5): first(), main(), plot(), spans(), window()

### Community 51 - "REV-BENCH"
Cohesion: 0.40
Nodes (5): Native tools (WSL / Linux), Project layout, Rebuilding the WebAssembly core, REV-BENCH, Try it in the browser

### Community 53 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 54 - "Phase 3 — Browser"
Cohesion: 0.50
Nodes (4): Phase 3 — Browser, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart

### Community 55 - "presets.c"
Cohesion: 0.46
Nodes (13): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+5 more)

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 387 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `rb_engine_step` to `test_audio.c`, `rb_engine_init`, `rb_cylinder_move`, `pipe.c`, `rb_cli.c`, `REV-BENCH — notes for Claude`, `RbExports`, `math`, `test_noise.c`?**
  _High betweenness centrality (0.207) - this node is a cross-community bridge._
- **Are the 71 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 71 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.058445353594389245 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `Stage 8 — Engine layouts & presets`, `rb-core.ts`, `RbSim`, `.process`, `RbCore`, `rb_engine_step`?**
  _High betweenness centrality (0.170) - this node is a cross-community bridge._
- **Are the 47 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 47 INFERRED edges - model-reasoned connections that need verification._
- **Should `Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧` be split into smaller, more focused modules?**
  _Cohesion score 0.1 - nodes in this community are weakly interconnected._