# Graph Report - REV-BENCH  (2026-10-20)

## Corpus Check
- 109 files · ~394,573 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .zip 1, .csv 1)

## Summary
- 1412 nodes · 3932 edges · 66 communities (56 shown, 10 thin omitted)
- Extraction: 70% EXTRACTED · 30% INFERRED · 0% AMBIGUOUS · INFERRED: 1166 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9662bb59`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_sim.c
- rb_cylinder_move
- rb_engine_step
- pipe.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- string
- main.ts
- Stage 12 — Mufflers, the intake and the listener 🎧
- package.json
- setup
- RbExports
- rb_engine_init
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
- test_audio.c
- math
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- test_noise.c
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- audio.c
- rb_destroy
- Stage 8 — Engine layouts & presets
- Where the time goes
- rb_decimator_init
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- rb_create
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Stage 3 — Gas & flow primitives
- Phase 4 — 3D engine view
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- Phase 2 — Sound (the realistic part)
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- rb_engine_desc_check
- rbplot.py
- REV-BENCH
- bench_wasm.mjs
- throttle.c
- Stages
- presets.c
- graphify-guard.sh
- session-start.sh
- rb_valve_opens
- The workspace
- rb_dc_blocker_init
- Sound realism (October 2026, after stage 12)

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 81 edges
2. `RbExports` - 51 edges
3. `rb_engine_init()` - 44 edges
4. `Core (C)` - 39 edges
5. `rb_engine_settle()` - 36 edges
6. `rb_engine_init_at()` - 34 edges
7. `style()` - 33 edges
8. `titles()` - 33 edges
9. `read_csv()` - 29 edges
10. `rb_create()` - 28 edges

## Surprising Connections (you probably didn't know these)
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  PLAN.md → core/src/chamber.c
- `Stage 8 — Engine layouts & presets` --references--> `rb_engine_layout()`  [INFERRED]
  PLAN.md → core/src/engine.c
- `The layout` --references--> `rb_engine_desc_check()`  [INFERRED]
  docs/stage-08.md → core/src/engine.c
- `The intake runners` --references--> `rb_engine_desc_check()`  [INFERRED]
  docs/stage-09.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (66 total, 10 thin omitted)

### Community 0 - "rb_sim.c"
Cohesion: 0.18
Nodes (22): config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+14 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (48): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+40 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.12
Nodes (36): rb_burnable_fuel(), rb_fuel_heat(), rb_wiebe(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake() (+28 more)

### Community 3 - "pipe.c"
Cohesion: 0.06
Nodes (89): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init_reservoir(), rb_chamber_pressure_now(), rb_chamber_sound_speed() (+81 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.06
Nodes (72): rb_preset_count(), rb_preset_desc(), rb_preset_find(), rb_preset_name(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name() (+64 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.17
Nodes (34): rb_engine_displacement(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe(), cmd_dyno(), cmd_fire() (+26 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.08
Nodes (40): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+32 more)

### Community 7 - "string"
Cohesion: 0.13
Nodes (28): Dither, build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32() (+20 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "Stage 12 — Mufflers, the intake and the listener 🎧"
Cohesion: 0.07
Nodes (26): A muffler is a pipe, Calibration, Cost, Every source at its own distance, Going straight on, Packing, Review of stage 11, Snorkel, air box, filter (+18 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "setup"
Cohesion: 0.24
Nodes (7): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), setup()

### Community 13 - "rb_engine_init"
Cohesion: 0.15
Nodes (20): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), rb_engine_init(), Tests that can fail, test_audio_intake_mouth() (+12 more)

### Community 14 - "RbSim"
Cohesion: 0.08
Nodes (3): Notes for later stages, What was built, RbSim

### Community 15 - "style"
Cohesion: 0.19
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "label"
Cohesion: 0.16
Nodes (12): main(), textbook_line(), colorbar(), compare(), engine(), grid(), main(), pulse() (+4 more)

### Community 17 - "plot_fire.py"
Cohesion: 0.16
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 18 - ".process"
Cohesion: 0.15
Nodes (7): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built, RevBenchProcessor, renderSeconds()

### Community 19 - "plot_dyno.py"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 21 - "matplotlib_pyplot"
Cohesion: 0.20
Nodes (12): plot_spark(), between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs() (+4 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "compare_real.py"
Cohesion: 0.13
Nodes (15): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+7 more)

### Community 24 - "test_audio.c"
Cohesion: 0.23
Nodes (17): rb_soft_limit(), Tests that can fail, Equal and unequal headers, amplitude(), band_power(), engine_sim(), order_amplitude(), record_tailpipes() (+9 more)

### Community 25 - "math"
Cohesion: 0.06
Nodes (61): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_set_volume(), rb_chamber_update(), remix(), volume_step() (+53 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.13
Nodes (44): Core (C), rb_engine_cylinder_angle(), rb_engine_settle(), Results, Tests that changed, Tests that changed, ambient_density(), crank_books() (+36 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "test_noise.c"
Cohesion: 0.12
Nodes (35): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+27 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.10
Nodes (20): A pipe end is a monopole, Cost, DC blocker and soft limiter, Down to the audio rate, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧 (+12 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.10
Nodes (20): Calibration, Cost, Modal synthesis, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin, The engine's own sound (+12 more)

### Community 33 - "audio.c"
Cohesion: 0.23
Nodes (20): arriving(), cabin(), cabin_design(), distance(), noise_arriving(), rb_listener_distance(), rb_listener_hear(), rb_listener_place() (+12 more)

### Community 34 - "rb_destroy"
Cohesion: 0.31
Nodes (16): rb_destroy(), rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that changed, test_audio_sim_hears_every_source(), make_tone(), render() (+8 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.20
Nodes (9): Idle tuning, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+1 more)

### Community 36 - "Where the time goes"
Cohesion: 0.40
Nodes (5): The chamber and the gas, The nozzle and the walls, The pipes, What didn't help, Where the time goes

### Community 37 - "rb_decimator_init"
Cohesion: 0.33
Nodes (9): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), response(), test_audio_decimator_filter(), test_audio_decimator_tones() (+1 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.33
Nodes (6): 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound, REV-BENCH — Research notes (stage 0)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 40 - "rb_create"
Cohesion: 0.24
Nodes (9): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), Phase 0 — Foundations, Stage 1 — Native C scaffold, WAV writer, test runner, Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet (+1 more)

### Community 41 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "Phase 4 — 3D engine view"
Cohesion: 0.50
Nodes (4): Phase 4 — 3D engine view, Stage 17 — 3D engine model: block + moving parts, Stage 18 — 3D engine in motion, in sync with the sound, Stage 19 — 3D gas, combustion & exhaust flow

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.14
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.29
Nodes (4): Architecture (decided up front), Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.33
Nodes (6): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 13 — Performance & stability pass

### Community 47 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.33
Nodes (6): Phase 2b — Sound realism (top priority) 🎧, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 48 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 49 - "rb_engine_desc_check"
Cohesion: 0.52
Nodes (6): muffler_width(), rb_engine_desc_check(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), test_engine_description()

### Community 50 - "rbplot.py"
Cohesion: 0.19
Nodes (5): first(), main(), plot(), spans(), window()

### Community 51 - "REV-BENCH"
Cohesion: 0.40
Nodes (5): Native tools (WSL / Linux), Project layout, Rebuilding the WebAssembly core, REV-BENCH, Try it in the browser

### Community 53 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 54 - "Stages"
Cohesion: 0.22
Nodes (9): Phase 3 — Browser, Phase 5 — Forced induction & realism, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart, Stage 20 — Turbocharger physics, Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧, Stage 22 — Knock, mechanical limits & engine damage (+1 more)

### Community 55 - "presets.c"
Cohesion: 0.46
Nodes (13): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+5 more)

### Community 60 - "rb_valve_opens"
Cohesion: 0.30
Nodes (9): rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens(), dead_centre(), print_cam() (+1 more)

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 62 - "rb_dc_blocker_init"
Cohesion: 0.70
Nodes (4): rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step(), test_audio_dc_blocker()

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.40
Nodes (5): How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 368 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **10 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_sim.c`, `audio.c`, `rb_engine_step`, `pipe.c`, `rb_cylinder_move`, `REV-BENCH — notes for Claude`, `RbExports`, `rb_engine_init`, `rb_engine_desc_check`, `math`, `test_noise.c`?**
  _High betweenness centrality (0.242) - this node is a cross-community bridge._
- **Are the 62 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 62 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.05552617662612375 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `RbSim`, `.process`, `test_engine.c`?**
  _High betweenness centrality (0.191) - this node is a cross-community bridge._
- **Are the 40 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 40 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.12181616832779624 - nodes in this community are weakly interconnected._