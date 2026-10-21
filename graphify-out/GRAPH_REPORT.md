# Graph Report - REV-BENCH  (2026-10-21)

## Corpus Check
- 123 files · ~446,559 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1534 nodes · 4242 edges · 66 communities (55 shown, 11 thin omitted)
- Extraction: 71% EXTRACTED · 29% INFERRED · 0% AMBIGUOUS · INFERRED: 1236 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `35240f25`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- engine.c
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
- test_audio_analysis.py
- RbSim
- style
- label
- rb_engine_step
- .process
- plot_dyno.py
- RbCore
- matplotlib_pyplot
- compilerOptions
- compare_real.py
- rb_engine_desc_default
- math
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- test_noise.c
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- test_audio.c
- rb_engine_init
- Stage 8 — Engine layouts & presets
- argparse
- rb_decimator_init
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- rb_engine_desc_check
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Stage 3 — Gas & flow primitives
- Phase 4 — 3D engine view
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- Phase 2 — Sound (the realistic part)
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- rbplot.py
- REV-BENCH
- bench_wasm.mjs
- throttle.c
- Stages
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 93 edges
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

## Communities (66 total, 11 thin omitted)

### Community 0 - "engine.c"
Cohesion: 0.13
Nodes (36): Core (C), rb_burnable_fuel(), add_share(), after_firing_tdc(), burn_duration(), close_intake(), computer_spark(), crank_turn() (+28 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (47): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+39 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "pipe.c"
Cohesion: 0.06
Nodes (96): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+88 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.08
Nodes (52): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+44 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.11
Nodes (44): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_preset_key(), rb_preset_name(), csv_close(), csv_open() (+36 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.08
Nodes (40): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+32 more)

### Community 7 - "string"
Cohesion: 0.14
Nodes (28): Dither, build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32() (+20 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "Stage 12 — Mufflers, the intake and the listener 🎧"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "setup"
Cohesion: 0.09
Nodes (23): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+15 more)

### Community 13 - "test_audio_analysis.py"
Cohesion: 0.17
Nodes (4): read_wav(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 14 - "RbSim"
Cohesion: 0.08
Nodes (3): Notes for later stages, What was built, RbSim

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "label"
Cohesion: 0.26
Nodes (8): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save()

### Community 17 - "rb_engine_step"
Cohesion: 0.22
Nodes (14): rb_wiebe(), burn_step(), cycle_angle(), rb_engine_displacement(), rb_engine_fmep(), rb_engine_layout_inline(), rb_engine_step(), test_wiebe_burn() (+6 more)

### Community 18 - ".process"
Cohesion: 0.15
Nodes (7): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built, RevBenchProcessor, renderSeconds()

### Community 19 - "plot_dyno.py"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 21 - "matplotlib_pyplot"
Cohesion: 0.23
Nodes (10): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+2 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "compare_real.py"
Cohesion: 0.11
Nodes (17): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+9 more)

### Community 24 - "rb_engine_desc_default"
Cohesion: 0.29
Nodes (11): rb_engine_desc_default(), test_audio_intake_mouth(), best_spark(), run_held(), test_calibration_burn_and_spark(), test_calibration_cams(), test_calibration_compression(), test_calibration_idle_spark() (+3 more)

### Community 25 - "math"
Cohesion: 0.05
Nodes (53): rb_fuel_heat(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u() (+45 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.08
Nodes (25): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes (+17 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (45): rb_engine_cylinder_angle(), rb_engine_settle(), Results, Tests that changed, Tests that changed, ambient_density(), crank_books(), energy_books() (+37 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "test_noise.c"
Cohesion: 0.06
Nodes (65): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+57 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.10
Nodes (20): A pipe end is a monopole, Cost, DC blocker and soft limiter, Down to the audio rate, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧 (+12 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "test_audio.c"
Cohesion: 0.05
Nodes (97): arriving(), bessel_i0(), cabin(), cabin_design(), distance(), noise_arriving(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+89 more)

### Community 34 - "rb_engine_init"
Cohesion: 0.21
Nodes (21): rb_engine_init(), rb_preset_count(), rb_preset_desc(), rb_preset_find(), Tests that can fail, Three things that surprised me, test_engine_pipe_rate(), test_engine_pipes_two_rates() (+13 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.20
Nodes (9): Idle tuning, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+1 more)

### Community 36 - "argparse"
Cohesion: 0.36
Nodes (3): main(), prepare(), run()

### Community 37 - "rb_decimator_init"
Cohesion: 0.50
Nodes (4): Phase 5 — Forced induction & realism, Stage 20 — Turbocharger physics, Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧, Stage 22 — Knock, mechanical limits & engine damage

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.33
Nodes (6): 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound, REV-BENCH — Research notes (stage 0)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 40 - "rb_engine_desc_check"
Cohesion: 0.48
Nodes (6): accessory_torque(), muffler_width(), rb_engine_desc_check(), rb_engine_layout(), tdc_angle(), rb_set_engine()

### Community 41 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "Phase 4 — 3D engine view"
Cohesion: 0.29
Nodes (7): Phase 0 — Foundations, Phase 4 — 3D engine view, Stage 17 — 3D engine model: block + moving parts, Stage 18 — 3D engine in motion, in sync with the sound, Stage 19 — 3D gas, combustion & exhaust flow, Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet, Stages

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.14
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

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
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 51 - "REV-BENCH"
Cohesion: 0.33
Nodes (6): Native tools (WSL / Linux), Project layout, Rebuilding the WebAssembly core, REV-BENCH, Sound-analysis checks, Try it in the browser

### Community 53 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 54 - "Stages"
Cohesion: 0.50
Nodes (4): Phase 3 — Browser, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart

### Community 55 - "Bernardo's engine sound examples — 2026-10-21"
Cohesion: 0.29
Nodes (6): Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and reproduction, Selection and perspective, Validation, What the references represent

### Community 60 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.50
Nodes (3): 2026-10-21: S1a, S1b and local references, Codex tools on this machine, REV-BENCH - Codex handoff

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 419 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `engine.c` to `test_audio.c`, `rb_cylinder_move`, `pipe.c`, `rb_engine_init`, `REV-BENCH — notes for Claude`, `rb_engine_desc_check`, `RbExports`, `rb_engine_desc_default`, `math`, `test_engine.c`, `test_noise.c`?**
  _High betweenness centrality (0.194) - this node is a cross-community bridge._
- **Are the 73 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 73 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `engine.c` be split into smaller, more focused modules?**
  _Cohesion score 0.12682926829268293 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `engine.c`, `rb-core.ts`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `RbSim`, `.process`?**
  _High betweenness centrality (0.149) - this node is a cross-community bridge._
- **Are the 47 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 47 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.05737704918032787 - nodes in this community are weakly interconnected._