# Graph Report - rev-bench  (2026-10-22)

## Corpus Check
- 164 files · ~550,513 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1808 nodes · 4932 edges · 92 communities (69 shown, 23 thin omitted)
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 1354 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `d7c7453b`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_engine_step
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
- read_csv
- RbExports
- compare_real.py
- RbSim
- style
- read_csv
- rb_engine_init
- chamber.h
- rb_network_step
- RbCore
- test_audio.c
- compilerOptions
- Stage S1b — Finite-time partial burns
- Stage S1d — Burn timing follows the trapped mixture
- math
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- rb_chamber_init
- plot_fire.py
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- audit_s1.py
- flow.h
- Stage 8 — Engine layouts & presets
- prepare_user_references.py
- Stages
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- measure_lift_feedback.py
- Stage 3 — Gas & flow primitives
- plot_run.py
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- rand.h
- engine.c
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
- argparse
- engine.h
- json
- sys
- setup
- test_pipe.c
- REV-BENCH — notes for Claude
- Stage S1c — Reproducible sound and combustion baselines
- rb_engine_desc_default
- AGENTS.md
- Stage 13 — Performance & stability pass
- sys
- read_wav
- rb_engine_pipe
- Where the time goes
- Phase 2 — Sound (the realistic part)
- Players
- REV-BENCH — Feedback checkpoint before stage 13b
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- Current sound results

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 99 edges
2. `rb_engine_init()` - 55 edges
3. `RbExports` - 53 edges
4. `Core (C)` - 41 edges
5. `rb_engine_init_at()` - 38 edges
6. `rb_engine_settle()` - 36 edges
7. `style()` - 36 edges
8. `cmd_sound()` - 33 edges
9. `titles()` - 33 edges
10. `rb_chamber_init()` - 31 edges

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

## Communities (92 total, 23 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.15
Nodes (32): rb_engine_cylinder_angle(), rb_engine_step(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+24 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (46): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+38 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.11
Nodes (18): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+10 more)

### Community 3 - "pipe.c"
Cohesion: 0.16
Nodes (34): new_pipe(), rb_engine_init_at(), cell_heat(), cell_is_fast(), end_flow(), face_init(), fast_cells(), fast_flows() (+26 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.06
Nodes (72): rb_preset_count(), rb_preset_key(), rb_preset_name(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit() (+64 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.13
Nodes (40): rb_engine_desc_check(), rb_engine_displacement(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe(), cmd_dyno() (+32 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "wav.c"
Cohesion: 0.08
Nodes (46): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+38 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "presets.c"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "read_csv"
Cohesion: 0.24
Nodes (6): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep()

### Community 13 - "compare_real.py"
Cohesion: 0.13
Nodes (14): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+6 more)

### Community 15 - "style"
Cohesion: 0.17
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "read_csv"
Cohesion: 0.14
Nodes (15): main(), textbook_line(), plot_power(), plot_spark(), colorbar(), compare(), engine(), grid() (+7 more)

### Community 17 - "rb_engine_init"
Cohesion: 0.08
Nodes (57): rb_engine_init(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole() (+49 more)

### Community 18 - "chamber.h"
Cohesion: 0.17
Nodes (18): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), rb_pipe_clear() (+10 more)

### Community 19 - "rb_network_step"
Cohesion: 0.21
Nodes (13): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), blowdown_time_to_2bar(), setup_blowdown() (+5 more)

### Community 21 - "test_audio.c"
Cohesion: 0.06
Nodes (92): arriving(), bessel_i0(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+84 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "Stage S1b — Finite-time partial burns"
Cohesion: 0.25
Nodes (7): Completed before/after audit, Equations and behavior, Remaining work, Sound check and listening checkpoint, Sources and limits, Stage S1b — Finite-time partial burns, Why no fixed residual cutoff

### Community 24 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 25 - "math"
Cohesion: 0.05
Nodes (54): rb_fuel_heat(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u() (+46 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.09
Nodes (21): Checks, Light load, Notes for later stages, Review of stage 9, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes, The ends, The engine computer looks ahead (+13 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (46): Core (C), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth(), ambient_density() (+38 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "rb_chamber_init"
Cohesion: 0.33
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 31 - "plot_fire.py"
Cohesion: 0.18
Nodes (9): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), spark_text() (+1 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "audit_s1.py"
Cohesion: 0.17
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 34 - "flow.h"
Cohesion: 0.29
Nodes (5): rb_pipe_radiate(), rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), test_radiation_causal_impedance()

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 36 - "prepare_user_references.py"
Cohesion: 0.24
Nodes (7): encode_listening(), main(), prepare(), prepare_filtered(), run(), sha256(), ReferenceFilteringTests

### Community 37 - "Stages"
Cohesion: 0.13
Nodes (15): Phase 0 — Foundations, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart, Stage 17 — 3D engine model: block + moving parts (+7 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.20
Nodes (10): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free) (+2 more)

### Community 41 - "measure_lift_feedback.py"
Cohesion: 0.15
Nodes (7): write_float(), excerpt(), main(), spectral_metrics(), telemetry(), CheckpointRenderTests, LiftMetricsTests

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "plot_run.py"
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.20
Nodes (8): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports()

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.33
Nodes (5): Architecture (decided up front), Latest listening feedback — 2026-10-22, Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "rand.h"
Cohesion: 0.27
Nodes (9): rb_burnable_fuel(), soft_cut(), spark(), rb_rand_next(), rb_rand_normal(), rb_rand_uniform(), rb_rand_white(), The rev limiter's soft cut (+1 more)

### Community 47 - "engine.c"
Cohesion: 0.11
Nodes (33): rb_wiebe(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark() (+25 more)

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 51 - "render_results.py"
Cohesion: 0.14
Nodes (11): coast_markers(), expected_clips(), listening_wav(), main(), render_checkpoint(), replace_current(), sha(), write_page() (+3 more)

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
Cohesion: 0.15
Nodes (12): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: full 13b and model-specific exhaust work resumed, 2026-10-22: researched exhaust implementation and listening checkpoint (+4 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 66 - "argparse"
Cohesion: 0.50
Nodes (4): deceleration_report(), main(), record(), sha()

### Community 67 - "engine.h"
Cohesion: 0.11
Nodes (17): rb_exhaust_model(), geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), add() (+9 more)

### Community 68 - "json"
Cohesion: 0.25
Nodes (10): analyze(), long_coast(), main(), record(), render(), sha(), bench(), compare() (+2 more)

### Community 69 - "sys"
Cohesion: 0.20
Nodes (11): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+3 more)

### Community 70 - "setup"
Cohesion: 0.23
Nodes (10): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+2 more)

### Community 71 - "test_pipe.c"
Cohesion: 0.24
Nodes (26): rb_chamber_init_reservoir(), rb_pipe_init(), rb_pipe_join(), Tests that can fail, extreme(), packed_ring(), packet_energy_ratio(), packing_fade() (+18 more)

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.10
Nodes (16): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+8 more)

### Community 73 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.23
Nodes (15): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), best_spark(), run_held(), test_calibration_burn_and_spark() (+7 more)

### Community 76 - "Stage 13 — Performance & stability pass"
Cohesion: 0.13
Nodes (14): Tests that changed, Cost, Measuring, Notes for later stages, Sources, Stage 13 — Performance & stability pass, The fuzzer, The quality setting (+6 more)

### Community 77 - "sys"
Cohesion: 0.43
Nodes (3): load_table(), main(), run()

### Community 78 - "read_wav"
Cohesion: 0.19
Nodes (5): read_wav(), read_wav_bytes(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "rb_engine_pipe"
Cohesion: 0.23
Nodes (13): rb_engine_contents(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat(), slow_flows(), slow_step(), inside(), repairs() (+5 more)

### Community 80 - "Where the time goes"
Cohesion: 0.40
Nodes (5): The chamber and the gas, The nozzle and the walls, The pipes, What didn't help, Where the time goes

### Community 81 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.06
Nodes (27): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+19 more)

### Community 85 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.29
Nodes (7): Phase 2b — Sound realism (top priority) 🎧, Stage 13b-p — Model-specific exhaust research and implementation, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 91 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 93 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 528 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **23 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_cylinder_move`, `pipe.c`, `rb_cli.c`, `REV-BENCH — notes for Claude`, `rb_engine_desc_default`, `Stage 13 — Performance & stability pass`, `RbExports`, `rand.h`, `engine.c`, `rb_engine_pipe`, `rb_engine_init`, `chamber.h`, `rb_network_step`, `test_audio.c`, `math`, `rb_chamber_init`?**
  _High betweenness centrality (0.194) - this node is a cross-community bridge._
- **Are the 79 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 79 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.14871794871794872 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `Stage 8 — Engine layouts & presets`, `rb-core.ts`, `REV-BENCH — notes for Claude`, `REV-BENCH — notes for Claude`, `RbSim`, `RbCore`, `test_engine.c`?**
  _High betweenness centrality (0.181) - this node is a cross-community bridge._
- **Are the 51 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 51 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.06015037593984962 - nodes in this community are weakly interconnected._