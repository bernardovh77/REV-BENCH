# Graph Report - REV-BENCH  (2026-10-20)

## Corpus Check
- 104 files · ~365,577 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 9 file(s) not represented in the graph (top: (none) 5, .csv 1, .css 1)

## Summary
- 1329 nodes · 3680 edges · 67 communities (54 shown, 13 thin omitted)
- Extraction: 71% EXTRACTED · 29% INFERRED · 0% AMBIGUOUS · INFERRED: 1078 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `8bdee2b4`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_create
- rb_cylinder_move
- rb_engine_step
- pipe.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- wav.c
- main.ts
- presets.c
- package.json
- label
- RbExports
- test_dyno.c
- RbSim
- style
- label
- plot_fire.py
- RevBenchProcessor
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
- rb_engine_init
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- cmd_info
- audio.c
- rb_destroy
- Stage 8 — Engine layouts & presets
- Stage 13 — Performance & stability pass
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
- rb_engine_init_at
- Stage 1 — Native C scaffold, WAV writer, test runner
- REV-BENCH
- bench_wasm.mjs
- throttle.c
- Stages
- RevBenchNode
- graphify-guard.sh
- session-start.sh
- engine-node.ts
- The workspace
- .loadSync
- Sound realism (October 2026, after stage 12)
- The listener

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 77 edges
2. `RbExports` - 51 edges
3. `rb_engine_init()` - 34 edges
4. `rb_engine_init_at()` - 34 edges
5. `rb_engine_settle()` - 33 edges
6. `Core (C)` - 33 edges
7. `style()` - 32 edges
8. `titles()` - 32 edges
9. `read_csv()` - 29 edges
10. `label()` - 28 edges

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

## Communities (67 total, 13 thin omitted)

### Community 0 - "rb_create"
Cohesion: 0.18
Nodes (22): config_is_valid(), model_step(), rb_config_default(), rb_get_telemetry(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+14 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (46): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+38 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.16
Nodes (32): add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn(), cycle_angle() (+24 more)

### Community 3 - "pipe.c"
Cohesion: 0.06
Nodes (92): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+84 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.06
Nodes (65): rb_preset_count(), rb_preset_desc(), rb_preset_find(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit() (+57 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.15
Nodes (37): rb_engine_displacement(), rb_preset_key(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe() (+29 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (42): CoreInfo, EngineDescription, ParamInfo, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER (+34 more)

### Community 7 - "wav.c"
Cohesion: 0.20
Nodes (25): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+17 more)

### Community 8 - "main.ts"
Cohesion: 0.15
Nodes (22): boot(), clearReadouts(), drawScope(), errorText(), findParam(), findTelemetry(), fitCanvas(), formatHz() (+14 more)

### Community 9 - "presets.c"
Cohesion: 0.10
Nodes (37): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+29 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "label"
Cohesion: 0.14
Nodes (13): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+5 more)

### Community 13 - "test_dyno.c"
Cohesion: 0.17
Nodes (12): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), dyno_average(), best_spark(), run_held(), test_calibration_cams() (+4 more)

### Community 15 - "style"
Cohesion: 0.21
Nodes (14): What was built, a_weighting(), check(), highpass(), levels(), levels_chart(), main(), orders() (+6 more)

### Community 16 - "label"
Cohesion: 0.24
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 17 - "plot_fire.py"
Cohesion: 0.18
Nodes (9): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), spark_text() (+1 more)

### Community 19 - "plot_dyno.py"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 21 - "matplotlib_pyplot"
Cohesion: 0.13
Nodes (15): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+7 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "compare_real.py"
Cohesion: 0.13
Nodes (15): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+7 more)

### Community 24 - "test_audio.c"
Cohesion: 0.22
Nodes (18): rb_soft_limit(), rb_destroy(), Tests that can fail, Equal and unequal headers, amplitude(), band_power(), engine_sim(), order_amplitude() (+10 more)

### Community 25 - "math"
Cohesion: 0.06
Nodes (45): rb_burnable_fuel(), rb_fuel_heat(), rb_wiebe(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h() (+37 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.17
Nodes (28): Core (C), Results, crank_books(), energy_books(), inside(), run_cycles(), run_for(), run_to_angle() (+20 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "rb_engine_init"
Cohesion: 0.19
Nodes (23): rb_engine_desc_default(), rb_engine_init(), rb_engine_settle(), Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth(), test_calibration_burn_and_spark() (+15 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.09
Nodes (21): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+13 more)

### Community 32 - "cmd_info"
Cohesion: 0.35
Nodes (15): param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags(), rb_param_max(), rb_param_min() (+7 more)

### Community 33 - "audio.c"
Cohesion: 0.26
Nodes (16): arriving(), distance(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step(), rb_listener_distance(), rb_listener_hear(), rb_listener_place() (+8 more)

### Community 34 - "rb_destroy"
Cohesion: 0.44
Nodes (8): rb_set_param(), Tests that changed, make_tone(), render(), test_core_block_size_independent_and_reset(), test_core_level_smoothing(), test_core_no_clicks_on_frequency_change(), test_core_tone_frequency_and_level()

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 36 - "Stage 13 — Performance & stability pass"
Cohesion: 0.12
Nodes (16): Cost, Measuring, Notes for later stages, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls, The pipes (+8 more)

### Community 37 - "rb_decimator_init"
Cohesion: 0.33
Nodes (9): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), response(), test_audio_decimator_filter(), test_audio_decimator_tones() (+1 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.33
Nodes (6): 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound, REV-BENCH — Research notes (stage 0)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.12
Nodes (12): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+4 more)

### Community 40 - "rb_create"
Cohesion: 0.20
Nodes (12): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), rb_config_quality(), Phase 0 — Foundations, Stage 1 — Native C scaffold, WAV writer, test runner (+4 more)

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
Cohesion: 0.29
Nodes (6): Equations used, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.29
Nodes (4): Architecture (decided up front), Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.33
Nodes (6): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — folded into the sound-realism track, Stage 13 — Performance & stability pass

### Community 47 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.33
Nodes (6): Phase 2b — Sound realism (top priority) 🎧, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: flow noise and a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧, Stage S4 — Scenery: stereo and the place 🎧

### Community 48 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 49 - "rb_engine_init_at"
Cohesion: 0.24
Nodes (12): muffler_width(), new_pipe(), rb_engine_cylinder_angle(), rb_engine_desc_check(), rb_engine_init_at(), rb_engine_layout(), rb_engine_layout_inline(), system_of() (+4 more)

### Community 50 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.33
Nodes (5): Equations used, How the core runs, Results, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 51 - "REV-BENCH"
Cohesion: 0.40
Nodes (5): Native tools (WSL / Linux), Project layout, Rebuilding the WebAssembly core, REV-BENCH, Try it in the browser

### Community 53 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 54 - "Stages"
Cohesion: 0.22
Nodes (9): Phase 3 — Browser, Phase 5 — Forced induction & realism, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart, Stage 20 — Turbocharger physics, Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧, Stage 22 — Knock, mechanical limits & engine damage (+1 more)

### Community 60 - "engine-node.ts"
Cohesion: 0.44
Nodes (5): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, Optional: REV-BENCH in its own repo, Registering the hooks (once, by hand), The workspace

### Community 63 - ".loadSync"
Cohesion: 0.40
Nodes (3): How the sound gets out, makeImports(), setup()

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.40
Nodes (5): How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 65 - "The listener"
Cohesion: 0.50
Nodes (4): Every source at its own distance, The cross-plane V8's burble, The listener, Where it stands

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 345 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **13 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_create`, `rb_cylinder_move`, `rb_engine_step`, `pipe.c`, `REV-BENCH — notes for Claude`, `rb_create`, `RbExports`, `test_dyno.c`, `rb_engine_init_at`, `math`, `rb_engine_init`?**
  _High betweenness centrality (0.208) - this node is a cross-community bridge._
- **Are the 58 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 58 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.06015037593984962 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `Stage 8 — Engine layouts & presets`, `rb-core.ts`, `REV-BENCH — notes for Claude`, `RbSim`, `RevBenchProcessor`, `RbCore`, `test_engine.c`, `.createSim`?**
  _High betweenness centrality (0.185) - this node is a cross-community bridge._
- **Are the 30 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 30 INFERRED edges - model-reasoned connections that need verification._
- **Should `pipe.c` be split into smaller, more focused modules?**
  _Cohesion score 0.06226295828065739 - nodes in this community are weakly interconnected._