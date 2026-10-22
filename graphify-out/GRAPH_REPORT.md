# Graph Report - rev-bench  (2026-10-22)

## Corpus Check
- 171 files · ~571,871 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1883 nodes · 5097 edges · 97 communities (74 shown, 23 thin omitted)
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 1398 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `bd98ab94`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_engine_step
- rb_cylinder_move
- rb_engine_step
- wav.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- main.ts
- presets.c
- package.json
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- RbExports
- numpy
- RbSim
- style
- argparse
- test_scene.c
- Stage S1d — Burn timing follows the trapped mixture
- Model limitations and validation still requiring attention
- RbCore
- rb_engine_init
- compilerOptions
- S4 — Separate sources, stereo listeners and acoustic scenes
- engine.h
- rb_test.h
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- sys
- cmd_info
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- subprocess
- rb_destroy
- style
- plot_fire.py
- Stages
- REV-BENCH — Research notes (stage 0)
- engine-node.ts
- Stage 13 — Performance & stability pass
- Stage 3 — Gas & flow primitives
- json
- .process
- throttle.c
- label
- audit_s1.py
- Stage 13b-p — researched exhaust configurations
- README.md
- render_results.py
- bench_wasm.mjs
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)
- math
- start
- argparse
- compare_real.py
- pathlib
- math
- REV-BENCH — notes for Claude
- snapshot
- rb_engine_desc_default
- AGENTS.md
- rb_audit.c
- test_presets.c
- read_wav
- rb_create
- plot_run.py
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- test_engine_pipes_two_rates
- checkpoint-13b.md
- REV-BENCH — Feedback checkpoint before stage 13b
- Stage 1 — Native C scaffold, WAV writer, test runner
- S2b — Loaded exhaust detail and the lift-off contrast
- check_engine
- Current sound results
- test_overrun.c
- Stage 13b-r — high-RPM exhaust character

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 102 edges
2. `rb_engine_init()` - 57 edges
3. `RbExports` - 53 edges
4. `Core (C)` - 41 edges
5. `rb_engine_init_at()` - 40 edges
6. `style()` - 38 edges
7. `rb_engine_settle()` - 36 edges
8. `rb_chamber_init()` - 33 edges
9. `cmd_sound()` - 33 edges
10. `titles()` - 33 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  PLAN.md → core/src/chamber.c
- `Unequal headers (`core/src/presets.c`)` --references--> `rb_engine_desc_default()`  [INFERRED]
  docs/stage-12c.md → core/src/engine.c
- `Stage 8 — Engine layouts & presets` --references--> `rb_engine_layout()`  [INFERRED]
  PLAN.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (97 total, 23 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.05
Nodes (92): rb_wiebe(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark() (+84 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (48): rb_burnable_fuel(), rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move() (+40 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "wav.c"
Cohesion: 0.18
Nodes (26): Dither, build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32() (+18 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.14
Nodes (35): rb_preset_count(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find() (+27 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.11
Nodes (48): rb_engine_desc_check(), rb_engine_displacement(), Idle tuning, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout (+40 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.10
Nodes (20): A pipe end is a monopole, Cost, DC blocker and soft limiter, Down to the audio rate, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧 (+12 more)

### Community 8 - "main.ts"
Cohesion: 0.09
Nodes (26): RevBenchNode, CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam() (+18 more)

### Community 9 - "presets.c"
Cohesion: 0.09
Nodes (39): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+31 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 13 - "numpy"
Cohesion: 0.12
Nodes (12): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines, a_weighting(), as_db(), measure() (+4 more)

### Community 14 - "RbSim"
Cohesion: 0.08
Nodes (3): Notes for later stages, What was built, RbSim

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "argparse"
Cohesion: 0.14
Nodes (9): describe(), log_axis(), main(), plot_cycle(), pumping_mep(), main(), textbook_line(), crank_axis() (+1 more)

### Community 17 - "test_scene.c"
Cohesion: 0.32
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 18 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (34): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+26 more)

### Community 21 - "rb_engine_init"
Cohesion: 0.05
Nodes (98): arriving(), bessel_i0(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+90 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "S4 — Separate sources, stereo listeners and acoustic scenes"
Cohesion: 0.40
Nodes (5): Geometry and approximations, Pass-by and room decay, S4 — Separate sources, stereo listeners and acoustic scenes, Signal path, Validation and use

### Community 24 - "engine.h"
Cohesion: 0.14
Nodes (7): rb_exhaust_model(), Tests that changed, The fuzzer, seed_state(), uniform(), test_fuzz_random_engines(), output_rms()

### Community 25 - "rb_test.h"
Cohesion: 0.09
Nodes (37): rb_fuel_heat(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u() (+29 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.10
Nodes (20): Against the real engines, At 96 kHz, Light load, Results, Review of stage 9, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes, The engine computer looks ahead (+12 more)

### Community 28 - "test_engine.c"
Cohesion: 0.21
Nodes (27): Core (C), rb_engine_settle(), Results, ambient_density(), crank_books(), run_cycles(), run_for(), run_to_angle() (+19 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "sys"
Cohesion: 0.20
Nodes (12): plot_map(), curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark() (+4 more)

### Community 31 - "cmd_info"
Cohesion: 0.25
Nodes (18): rb_preset_key(), rb_preset_name(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags() (+10 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "subprocess"
Cohesion: 0.17
Nodes (11): load_table(), main(), run(), encode_listening(), main(), prepare(), prepare_filtered(), run() (+3 more)

### Community 34 - "rb_destroy"
Cohesion: 0.27
Nodes (18): rb_destroy(), rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that changed, test_audio_chain_survives_nan(), test_audio_sim_hears_every_source(), make_tone() (+10 more)

### Community 35 - "style"
Cohesion: 0.22
Nodes (11): plot_spark(), between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs() (+3 more)

### Community 36 - "plot_fire.py"
Cohesion: 0.16
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 37 - "Stages"
Cohesion: 0.05
Nodes (41): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism (+33 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.17
Nodes (12): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio (+4 more)

### Community 39 - "engine-node.ts"
Cohesion: 0.44
Nodes (5): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet

### Community 41 - "Stage 13 — Performance & stability pass"
Cohesion: 0.12
Nodes (16): Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls (+8 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "json"
Cohesion: 0.24
Nodes (11): analyze(), long_coast(), main(), record(), render(), sha(), write_float(), bench() (+3 more)

### Community 44 - ".process"
Cohesion: 0.15
Nodes (5): How the sound gets out, RevBenchProcessor, makeImports(), renderSeconds(), setup()

### Community 45 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 46 - "label"
Cohesion: 0.24
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 47 - "audit_s1.py"
Cohesion: 0.23
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 51 - "render_results.py"
Cohesion: 0.11
Nodes (11): coast_markers(), expected_clips(), listening_wav(), main(), replace_current(), sha(), write_page(), checkpoint() (+3 more)

### Community 54 - "Stage 13b — Performance feedback checkpoint"
Cohesion: 0.25
Nodes (8): 2026-10-22 continuation: coupled port experiment and researched exhausts, Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance and solver work, Validation and sound, What was built, Why the implicit probe was rejected

### Community 55 - "Bernardo's engine sound examples — 2026-10-21"
Cohesion: 0.25
Nodes (7): Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and reproduction, R1b: optional filtered listening copies — 2026-10-21, Selection and perspective, Validation, What the references represent

### Community 60 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.13
Nodes (14): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+6 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 67 - "start"
Cohesion: 0.20
Nodes (11): Notes for later stages, Snorkel, air box, filter, Tests that changed, The intake, The mouth as a source, start(), test_engine_choked_throttle(), test_engine_intake_losses() (+3 more)

### Community 68 - "argparse"
Cohesion: 0.14
Nodes (14): deceleration_report(), main(), record(), sha(), comparison(), main(), report(), window_metrics() (+6 more)

### Community 69 - "compare_real.py"
Cohesion: 0.31
Nodes (6): band(), chart(), compare(), fetch(), main(), read_list()

### Community 71 - "math"
Cohesion: 0.05
Nodes (111): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+103 more)

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 73 - "snapshot"
Cohesion: 0.33
Nodes (7): energy_books(), inside(), snapshot(), species_unbalanced(), test_engine_conserves_mass_and_energy(), test_engine_crank_books(), test_engine_fired_conserves_everything()

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.22
Nodes (16): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), Tests that changed, best_spark(), run_held() (+8 more)

### Community 76 - "rb_audit.c"
Cohesion: 0.33
Nodes (5): add(), audit(), main(), number(), usage()

### Community 77 - "test_presets.c"
Cohesion: 0.53
Nodes (7): all_gaps(), count_sparks(), firing_gaps(), gaps_are(), refused(), test_layout_rules(), test_presets_fire_where_the_layout_says()

### Community 78 - "read_wav"
Cohesion: 0.16
Nodes (6): read_wav(), read_wav_bytes(), read_wav(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "rb_create"
Cohesion: 0.14
Nodes (29): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), config_is_valid(), rb_config_default(), rb_config_quality() (+21 more)

### Community 80 - "plot_run.py"
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 81 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.29
Nodes (6): Equations used, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built

### Community 83 - "test_engine_pipes_two_rates"
Cohesion: 0.29
Nodes (7): Checks, The ends, The pipe, The scheme, The walls, Two rates, test_engine_pipes_two_rates()

### Community 84 - "checkpoint-13b.md"
Cohesion: 0.15
Nodes (8): Model, Real references, S1e — Research and controlled overrun transitions, Validation and remaining work, Optional ultra quality, S2 — Additional flow sources and optional spatial resolution, Sources, Validation

### Community 85 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.29
Nodes (7): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed

### Community 87 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.33
Nodes (5): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 91 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 92 - "check_engine"
Cohesion: 0.53
Nodes (3): check_chamber(), check_engine(), fail()

### Community 93 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 94 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 96 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 578 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **23 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_engine_step`, `rb_cylinder_move`, `rb_cli.c`, `math`, `REV-BENCH — notes for Claude`, `rb_engine_desc_default`, `RbExports`, `rb_create`, `test_engine_pipes_two_rates`, `rb_engine_init`, `engine.h`, `rb_test.h`?**
  _High betweenness centrality (0.208) - this node is a cross-community bridge._
- **Are the 82 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 82 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.05079850492694529 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `.process`, `RbSim`, `RbCore`, `test_engine.c`?**
  _High betweenness centrality (0.187) - this node is a cross-community bridge._
- **Are the 53 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 53 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.05605499735589635 - nodes in this community are weakly interconnected._