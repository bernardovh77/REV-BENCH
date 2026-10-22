# Graph Report - rev-bench  (2026-10-22)

## Corpus Check
- 152 files · ~496,907 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1719 nodes · 4675 edges · 88 communities (69 shown, 19 thin omitted)
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 1304 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `4cb65467`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_engine_step
- rb_cylinder_move
- rb_engine_step
- test_pipe.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- wav.c
- main.ts
- Stage 12 — Mufflers, the intake and the listener 🎧
- package.json
- style
- RbExports
- compare_real.py
- RbSim
- style
- label
- rb_engine_init
- pipe.c
- throttle.c
- RbCore
- test_audio.c
- compilerOptions
- Stage S1b — Finite-time partial burns
- Stage S1d — Burn timing follows the trapped mixture
- gas.c
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- engine.c
- Tests that can fail
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- pipes_pulse
- Stage 13 — Performance & stability pass
- Stage 8 — Engine layouts & presets
- prepare_user_references.py
- Stages
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- rb_chamber_init
- Stage 3 — Gas & flow primitives
- math
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- Phase 2 — Sound (the realistic part)
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- rb_test.h
- Stage 6 — Combustion + heat transfer
- bench_wasm.mjs
- rb_bench.c
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)
- test_scene.c
- test_overrun.c
- argparse
- sys
- firing_panel
- rand.h
- REV-BENCH — notes for Claude
- Stage S1c — Reproducible sound and combustion baselines
- rb_engine_desc_default
- AGENTS.md
- S2b — Loaded exhaust detail and the lift-off contrast
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- read_wav
- Where the time goes
- REV-BENCH — Feedback checkpoint before stage 13b
- S4 — Separate sources, stereo listeners and acoustic scenes
- checkpoint-13b.md

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 98 edges
2. `rb_engine_init()` - 53 edges
3. `RbExports` - 53 edges
4. `Core (C)` - 41 edges
5. `rb_engine_init_at()` - 36 edges
6. `rb_engine_settle()` - 36 edges
7. `cmd_sound()` - 33 edges
8. `style()` - 33 edges
9. `titles()` - 33 edges
10. `rb_engine_desc_default()` - 30 edges

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

## Communities (88 total, 19 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.14
Nodes (34): rb_engine_cylinder_angle(), rb_engine_step(), rb_get_cylinder(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift() (+26 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (46): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+38 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "test_pipe.c"
Cohesion: 0.17
Nodes (28): rb_chamber_init_reservoir(), face_init(), rb_pipe_contents(), rb_pipe_end_cell(), rb_pipe_init(), rb_pipe_join(), count(), extreme() (+20 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.08
Nodes (53): rb_preset_count(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find() (+45 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.13
Nodes (41): rb_engine_desc_check(), rb_engine_displacement(), rb_preset_key(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown() (+33 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.08
Nodes (41): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+33 more)

### Community 7 - "wav.c"
Cohesion: 0.08
Nodes (46): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+38 more)

### Community 8 - "main.ts"
Cohesion: 0.09
Nodes (26): RevBenchNode, CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam() (+18 more)

### Community 9 - "Stage 12 — Mufflers, the intake and the listener 🎧"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "style"
Cohesion: 0.08
Nodes (37): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+29 more)

### Community 13 - "compare_real.py"
Cohesion: 0.14
Nodes (14): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+6 more)

### Community 15 - "style"
Cohesion: 0.19
Nodes (15): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+7 more)

### Community 16 - "label"
Cohesion: 0.27
Nodes (7): colorbar(), engine(), grid(), main(), pulse(), runs(), save()

### Community 17 - "rb_engine_init"
Cohesion: 0.07
Nodes (62): rb_engine_init(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole() (+54 more)

### Community 18 - "pipe.c"
Cohesion: 0.21
Nodes (28): cell_heat(), cell_is_fast(), end_flow(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux(), push() (+20 more)

### Community 19 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 21 - "test_audio.c"
Cohesion: 0.05
Nodes (97): arriving(), bessel_i0(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+89 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "Stage S1b — Finite-time partial burns"
Cohesion: 0.25
Nodes (7): Completed before/after audit, Equations and behavior, Remaining work, Sound check and listening checkpoint, Sources and limits, Stage S1b — Finite-time partial burns, Why no fixed residual cutoff

### Community 24 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 25 - "gas.c"
Cohesion: 0.25
Nodes (18): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_from_u() (+10 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.08
Nodes (25): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+17 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (47): Core (C), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth(), ambient_density() (+39 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "engine.c"
Cohesion: 0.11
Nodes (41): accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn() (+33 more)

### Community 31 - "Tests that can fail"
Cohesion: 0.20
Nodes (16): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), Tests that can fail, blowdown_time_to_2bar() (+8 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "pipes_pulse"
Cohesion: 0.20
Nodes (14): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), move_gas() (+6 more)

### Community 34 - "Stage 13 — Performance & stability pass"
Cohesion: 0.11
Nodes (18): new_pipe(), rb_engine_init_at(), rb_pipe_cells_for(), rb_pipe_onward(), rb_pipe_pack(), rb_pipe_resist(), Cost, Measuring (+10 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 36 - "prepare_user_references.py"
Cohesion: 0.09
Nodes (13): spectral_metrics(), load_table(), main(), run(), encode_listening(), main(), prepare(), prepare_filtered() (+5 more)

### Community 37 - "Stages"
Cohesion: 0.13
Nodes (15): Phase 0 — Foundations, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart, Stage 17 — 3D engine model: block + moving parts (+7 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.22
Nodes (9): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound (+1 more)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (9): Web, Sources, EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor (+1 more)

### Community 41 - "rb_chamber_init"
Cohesion: 0.33
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "math"
Cohesion: 0.21
Nodes (9): rb_burnable_fuel(), rb_fuel_heat(), rb_wiebe(), rb_species_R(), Checked against chemical equilibrium, test_burnable_fuel(), test_constant_volume_burn(), test_fuel_heat() (+1 more)

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.14
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.22
Nodes (6): Architecture (decided up front), Latest listening feedback — 2026-10-22, Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage), REV-BENCH

### Community 46 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 47 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.33
Nodes (6): Phase 2b — Sound realism (top priority) 🎧, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 48 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 50 - "rb_test.h"
Cohesion: 0.18
Nodes (4): Tests that changed, test_fuzz_random_engines(), main(), selected()

### Community 51 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.22
Nodes (8): Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it, What was built

### Community 53 - "rb_bench.c"
Cohesion: 0.54
Nodes (6): engine_cost(), main(), mixture_ns(), newton_ns(), now(), nozzle_ns()

### Community 54 - "Stage 13b — Performance feedback checkpoint"
Cohesion: 0.29
Nodes (7): Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance feedback checkpoint, Validation and sound, What was built, Why the implicit probe was rejected

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
Cohesion: 0.20
Nodes (9): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: stage 13b performance feedback checkpoint, Codex tools on this machine (+1 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 66 - "test_scene.c"
Cohesion: 0.32
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 67 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 68 - "argparse"
Cohesion: 0.10
Nodes (22): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), analyze() (+14 more)

### Community 69 - "sys"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 70 - "firing_panel"
Cohesion: 0.40
Nodes (3): firing_panel(), gaps(), spacing()

### Community 71 - "rand.h"
Cohesion: 0.70
Nodes (4): rb_rand_next(), rb_rand_normal(), rb_rand_uniform(), rb_rand_white()

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.20
Nodes (9): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+1 more)

### Community 73 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.23
Nodes (15): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), best_spark(), run_held(), test_calibration_burn_and_spark() (+7 more)

### Community 76 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 77 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 78 - "read_wav"
Cohesion: 0.20
Nodes (4): read_wav(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "Where the time goes"
Cohesion: 0.40
Nodes (5): The chamber and the gas, The nozzle and the walls, The pipes, What didn't help, Where the time goes

### Community 80 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.29
Nodes (7): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed

### Community 81 - "S4 — Separate sources, stereo listeners and acoustic scenes"
Cohesion: 0.40
Nodes (5): Geometry and approximations, Pass-by and room decay, S4 — Separate sources, stereo listeners and acoustic scenes, Signal path, Validation and use

### Community 84 - "checkpoint-13b.md"
Cohesion: 0.15
Nodes (8): Model, Real references, S1e — Research and controlled overrun transitions, Validation and remaining work, Optional ultra quality, S2 — Additional flow sources and optional spatial resolution, Sources, Validation

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 493 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **19 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `pipes_pulse`, `rb_cylinder_move`, `Stage 13 — Performance & stability pass`, `rb_cli.c`, `REV-BENCH — notes for Claude`, `rb_chamber_init`, `rb_engine_desc_default`, `math`, `RbExports`, `rb_engine_init`, `pipe.c`, `rb_test.h`, `test_audio.c`, `gas.c`, `engine.c`, `Tests that can fail`?**
  _High betweenness centrality (0.198) - this node is a cross-community bridge._
- **Are the 78 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 78 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.14285714285714285 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `Stage 8 — Engine layouts & presets`, `rb-core.ts`, `REV-BENCH — notes for Claude`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `RbSim`, `RbCore`, `test_engine.c`?**
  _High betweenness centrality (0.170) - this node is a cross-community bridge._
- **Are the 49 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 49 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.058445353594389245 - nodes in this community are weakly interconnected._