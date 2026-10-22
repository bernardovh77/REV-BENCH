# Graph Report - rev-bench  (2026-10-22)

## Corpus Check
- 161 files · ~546,993 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1765 nodes · 4818 edges · 97 communities (74 shown, 23 thin omitted)
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 1353 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `b9227429`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_engine_step
- rb_cylinder_move
- rb_engine_step
- math
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- wav.c
- main.ts
- Stage 12 — Mufflers, the intake and the listener 🎧
- package.json
- read_csv
- RbExports
- compare_real.py
- RbSim
- style
- label
- rb_engine_init
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- throttle.c
- RbCore
- rb_create
- compilerOptions
- Stage S1b — Finite-time partial burns
- Stage S1d — Burn timing follows the trapped mixture
- math
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- audio.c
- plot_fire.py
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- audit_s1.py
- test_audio.c
- Stage 8 — Engine layouts & presets
- prepare_user_references.py
- Stages
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- measure_lift_feedback.py
- Stage 3 — Gas & flow primitives
- argparse
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- test_core.c
- presets.c
- Stage 13b-p — researched exhaust configurations
- README.md
- S2 — Additional flow sources and optional spatial resolution
- bench_wasm.mjs
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)
- cmd_info
- string
- json
- sys
- setup
- test_wav.c
- REV-BENCH — notes for Claude
- Stage S1c — Reproducible sound and combustion baselines
- rb_engine_desc_default
- AGENTS.md
- S2b — Loaded exhaust detail and the lift-off contrast
- subprocess
- read_wav
- engine.h
- REV-BENCH — Feedback checkpoint before stage 13b
- S4 — Separate sources, stereo listeners and acoustic scenes
- render
- checkpoint-13b.md
- start
- fuzz.c
- Stage 6 — Combustion + heat transfer
- rb_audit.c
- test_engine_pipes_two_rates
- stdlib
- Stage 1 — Native C scaffold, WAV writer, test runner
- Down to the audio rate

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

## Communities (97 total, 23 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.05
Nodes (100): Core (C), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark() (+92 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (41): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), Crank angle convention (+33 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "math"
Cohesion: 0.07
Nodes (93): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+85 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.14
Nodes (35): rb_preset_count(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find() (+27 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.13
Nodes (40): rb_engine_displacement(), rb_preset_key(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe() (+32 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "wav.c"
Cohesion: 0.25
Nodes (11): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+3 more)

### Community 8 - "main.ts"
Cohesion: 0.09
Nodes (26): RevBenchNode, CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam() (+18 more)

### Community 9 - "Stage 12 — Mufflers, the intake and the listener 🎧"
Cohesion: 0.07
Nodes (26): A muffler is a pipe, Calibration, Cost, Equal and unequal headers, Every source at its own distance, Going straight on, Notes for later stages, Packing (+18 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "read_csv"
Cohesion: 0.25
Nodes (7): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), read_csv()

### Community 13 - "compare_real.py"
Cohesion: 0.13
Nodes (14): a_weighting(), as_db(), band(), compare(), fetch(), main(), measure(), read_list() (+6 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "label"
Cohesion: 0.19
Nodes (11): chart(), colorbar(), compare(), engine(), grid(), main(), pulse(), runs() (+3 more)

### Community 17 - "rb_engine_init"
Cohesion: 0.10
Nodes (44): rb_engine_init(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole() (+36 more)

### Community 18 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.12
Nodes (16): A pipe end is a monopole, Cost, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧, The four, The microphone (+8 more)

### Community 19 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 21 - "rb_create"
Cohesion: 0.14
Nodes (29): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), config_is_valid(), rb_config_default(), rb_config_quality() (+21 more)

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
Cohesion: 0.06
Nodes (54): rb_burnable_fuel(), rb_fuel_heat(), rb_wiebe(), equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow() (+46 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.10
Nodes (20): Against the real engines, At 96 kHz, Light load, Results, Review of stage 9, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes, The engine computer looks ahead (+12 more)

### Community 28 - "test_engine.c"
Cohesion: 0.15
Nodes (33): rb_engine_settle(), Results, ambient_density(), crank_books(), energy_books(), inside(), run_cycles(), run_for() (+25 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "audio.c"
Cohesion: 0.17
Nodes (25): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step() (+17 more)

### Community 31 - "plot_fire.py"
Cohesion: 0.15
Nodes (12): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+4 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.04
Nodes (41): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, Tests that changed (+33 more)

### Community 33 - "audit_s1.py"
Cohesion: 0.20
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 34 - "test_audio.c"
Cohesion: 0.21
Nodes (22): rb_soft_limit(), rb_preset_find(), rb_destroy(), Tests that can fail, Tests that can fail, amplitude(), band_power(), engine_sim() (+14 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 36 - "prepare_user_references.py"
Cohesion: 0.14
Nodes (8): encode_listening(), main(), prepare(), prepare_filtered(), run(), sha256(), CheckpointRenderTests, ReferenceFilteringTests

### Community 37 - "Stages"
Cohesion: 0.06
Nodes (35): Phase 0 — Foundations, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism, Phase 6 — Polish & ship, Stage 10 — 1D exhaust pipes (+27 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.20
Nodes (10): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free) (+2 more)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.13
Nodes (10): Web, Sources, EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor (+2 more)

### Community 41 - "measure_lift_feedback.py"
Cohesion: 0.26
Nodes (6): write_float(), excerpt(), main(), spectral_metrics(), telemetry(), LiftMetricsTests

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "argparse"
Cohesion: 0.16
Nodes (8): main(), textbook_line(), first(), main(), plot(), spans(), window(), label()

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.20
Nodes (8): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports()

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.33
Nodes (5): Architecture (decided up front), Latest listening feedback — 2026-10-22, Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "test_core.c"
Cohesion: 0.27
Nodes (15): rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that changed, cmd_tone(), make_tone(), render(), rnd01() (+7 more)

### Community 47 - "presets.c"
Cohesion: 0.46
Nodes (13): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+5 more)

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 51 - "S2 — Additional flow sources and optional spatial resolution"
Cohesion: 0.40
Nodes (4): Optional ultra quality, S2 — Additional flow sources and optional spatial resolution, Sources, Validation

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
Cohesion: 0.17
Nodes (11): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: full 13b and model-specific exhaust work resumed, 2026-10-22: researched exhaust implementation and listening checkpoint (+3 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 66 - "cmd_info"
Cohesion: 0.35
Nodes (15): param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags(), rb_param_max(), rb_param_min() (+7 more)

### Community 67 - "string"
Cohesion: 0.30
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 68 - "json"
Cohesion: 0.50
Nodes (6): analyze(), long_coast(), main(), record(), render(), sha()

### Community 69 - "sys"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 70 - "setup"
Cohesion: 0.25
Nodes (9): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+1 more)

### Community 71 - "test_wav.c"
Cohesion: 0.36
Nodes (14): wav_close(), wav_open(), wav_set_dither(), rbt_tmp_path(), harmonic(), le16(), le32(), slurp() (+6 more)

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.20
Nodes (9): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+1 more)

### Community 73 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.20
Nodes (18): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), Tests that changed, best_spark(), run_held() (+10 more)

### Community 76 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 77 - "subprocess"
Cohesion: 0.22
Nodes (7): bench(), compare(), main(), sha(), load_table(), main(), run()

### Community 78 - "read_wav"
Cohesion: 0.15
Nodes (7): read_wav(), read_wav(), main(), orders(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 80 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.29
Nodes (7): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed

### Community 81 - "S4 — Separate sources, stereo listeners and acoustic scenes"
Cohesion: 0.40
Nodes (5): Geometry and approximations, Pass-by and room decay, S4 — Separate sources, stereo listeners and acoustic scenes, Signal path, Validation and use

### Community 83 - "render"
Cohesion: 0.30
Nodes (10): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), render(), response(), test_audio_decimator_filter() (+2 more)

### Community 84 - "checkpoint-13b.md"
Cohesion: 0.25
Nodes (4): Model, Real references, S1e — Research and controlled overrun transitions, Validation and remaining work

### Community 85 - "start"
Cohesion: 0.18
Nodes (12): Notes for later stages, Snorkel, air box, filter, Tests that changed, The intake, The mouth as a source, start(), test_engine_choked_throttle(), test_engine_intake_losses() (+4 more)

### Community 87 - "fuzz.c"
Cohesion: 0.27
Nodes (5): check_chamber(), check_engine(), fail(), seed_state(), uniform()

### Community 91 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.20
Nodes (9): Checked against chemical equilibrium, Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it (+1 more)

### Community 92 - "rb_audit.c"
Cohesion: 0.33
Nodes (5): add(), audit(), main(), number(), usage()

### Community 93 - "test_engine_pipes_two_rates"
Cohesion: 0.29
Nodes (7): Checks, The ends, The pipe, The scheme, The walls, Two rates, test_engine_pipes_two_rates()

### Community 95 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.40
Nodes (4): Equations used, Results, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 96 - "Down to the audio rate"
Cohesion: 0.40
Nodes (5): DC blocker and soft limiter, Dither, Down to the audio rate, The anti-alias filter, The staircase

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 510 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **23 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `rb_engine_step` to `Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧`, `rb_cylinder_move`, `math`, `REV-BENCH — notes for Claude`, `rb_engine_desc_default`, `RbExports`, `rb_engine_init`, `rb_create`, `math`, `test_engine.c`, `test_engine_pipes_two_rates`, `audio.c`?**
  _High betweenness centrality (0.208) - this node is a cross-community bridge._
- **Are the 79 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 79 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.0518444666001994 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb_engine_step`, `Stage 8 — Engine layouts & presets`, `rb-core.ts`, `REV-BENCH — notes for Claude`, `RbSim`, `RbCore`?**
  _High betweenness centrality (0.186) - this node is a cross-community bridge._
- **Are the 51 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 51 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.0641025641025641 - nodes in this community are weakly interconnected._