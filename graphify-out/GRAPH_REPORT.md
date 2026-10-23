# Graph Report - rev-bench  (2026-10-23)

## Corpus Check
- 186 files · ~593,335 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 2011 nodes · 5485 edges · 113 communities (90 shown, 23 thin omitted)
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 1469 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `3d79db33`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_variation.c
- rb_cylinder_move
- rb_engine_step
- rb_engine_step
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- main.ts
- presets.c
- package.json
- wav.c
- RbExports
- compare_real.py
- RbSim
- style
- rbplot.py
- audio.c
- Stage 8 — Engine layouts & presets
- Model limitations and validation still requiring attention
- RbCore
- rb_engine_init
- compilerOptions
- Phase 0 — Foundations
- json
- gas.c
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- rb_decimator_init
- pipe.c
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- hashlib
- rb_destroy
- csv
- Stage S1c — Reproducible sound and combustion baselines
- Stages
- REV-BENCH — Research notes (stage 0)
- rb_pipe_flows
- Stage 13 — Performance & stability pass
- Stage 3 — Gas & flow primitives
- Tests that can fail
- .process
- numpy
- test_audio.c
- Selective sound rollback — 2026-10-22
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
- matplotlib_pyplot
- high_rpm_report.py
- audio_source_probe.c
- read_csv
- rb_engine_init_at
- REV-BENCH — notes for Claude
- argparse
- rb_engine_desc_default
- AGENTS.md
- style
- rb_test.h
- read_wav
- rb_create
- rb_chamber_init
- presets.c
- rb_engine_init
- REV-BENCH — Feedback checkpoint before stage 13b
- test_scene.c
- Stage 1 — Native C scaffold, WAV writer, test runner
- S2b — Loaded exhaust detail and the lift-off contrast
- throttle.c
- Current sound results
- acoustic_transport_probe.c
- test_combustion.c
- Stage 13b-r — high-RPM exhaust character
- setup
- pipe.h
- rb_audit.c
- Initial audio bottleneck diagnosis — 2026-10-22
- Stage 6 — Combustion + heat transfer
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- sys
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- rb_bench.c
- test_overrun.c
- rb_dc_blocker_init
- Should REV-BENCH change its sound-generation architecture?

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 103 edges
2. `rb_engine_init()` - 57 edges
3. `RbExports` - 53 edges
4. `rb_engine_init_at()` - 42 edges
5. `Core (C)` - 41 edges
6. `style()` - 38 edges
7. `rb_engine_settle()` - 36 edges
8. `rb_chamber_init()` - 34 edges
9. `rb_preset_desc()` - 33 edges
10. `cmd_sound()` - 33 edges

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

## Communities (113 total, 23 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.13
Nodes (32): rb_engine_cylinder_angle(), rb_get_cylinder(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+24 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (46): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+38 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.12
Nodes (37): accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn() (+29 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.08
Nodes (57): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+49 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.13
Nodes (42): rb_engine_displacement(), rb_preset_count(), rb_preset_key(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown() (+34 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.09
Nodes (21): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+13 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "presets.c"
Cohesion: 0.07
Nodes (29): A muffler is a pipe, Calibration, Cost, Equal and unequal headers, Every source at its own distance, Going straight on, Notes for later stages, Packing (+21 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "wav.c"
Cohesion: 0.14
Nodes (27): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+19 more)

### Community 13 - "compare_real.py"
Cohesion: 0.16
Nodes (10): a_weighting(), as_db(), band(), compare(), fetch(), main(), measure(), read_list() (+2 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "rbplot.py"
Cohesion: 0.14
Nodes (7): describe(), log_axis(), plot_cycle(), pumping_mep(), main(), textbook_line(), crank_axis()

### Community 17 - "audio.c"
Cohesion: 0.23
Nodes (20): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_listener_distance(), rb_listener_hear(), rb_listener_place() (+12 more)

### Community 18 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.16
Nodes (16): muffler_width(), rb_engine_desc_check(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), Idle tuning, Results, Review of stage 7 (+8 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (31): Confirmed implementation and numerical issues, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events, D09 — P2: propagation interpolation itself darkens the sound (+23 more)

### Community 21 - "rb_engine_init"
Cohesion: 0.10
Nodes (44): rb_engine_init(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole() (+36 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "Phase 0 — Foundations"
Cohesion: 0.67
Nodes (3): Phase 0 — Foundations, Stage 1 — Native C scaffold, WAV writer, test runner, Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet

### Community 24 - "json"
Cohesion: 0.13
Nodes (19): digest(), main(), analyze(), long_coast(), main(), record(), render(), sha() (+11 more)

### Community 25 - "gas.c"
Cohesion: 0.19
Nodes (22): rb_fuel_heat(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u() (+14 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (27): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+19 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (46): Core (C), rb_engine_settle(), Results, Tests that changed, Tests that changed, ambient_density(), crank_books(), energy_books() (+38 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.10
Nodes (20): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+12 more)

### Community 30 - "rb_decimator_init"
Cohesion: 0.33
Nodes (9): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), response(), test_audio_decimator_filter(), test_audio_decimator_tones() (+1 more)

### Community 31 - "pipe.c"
Cohesion: 0.20
Nodes (25): cell_heat(), cell_is_fast(), end_flow(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux(), push() (+17 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "hashlib"
Cohesion: 0.18
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 34 - "rb_destroy"
Cohesion: 0.18
Nodes (23): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_destroy(), rb_free_floats(), rb_set_param(), rb_get_telemetry() (+15 more)

### Community 35 - "csv"
Cohesion: 0.15
Nodes (13): analyze(), brief(), plot_summary(), read(), select(), self_test(), encode_listening(), main() (+5 more)

### Community 36 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 37 - "Stages"
Cohesion: 0.05
Nodes (39): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism, Phase 6 — Polish & ship (+31 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.14
Nodes (14): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0) (+6 more)

### Community 39 - "rb_pipe_flows"
Cohesion: 0.18
Nodes (21): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), slow_flows() (+13 more)

### Community 41 - "Stage 13 — Performance & stability pass"
Cohesion: 0.12
Nodes (16): Cost, Measuring, Notes for later stages, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls, The pipes (+8 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "Tests that can fail"
Cohesion: 0.18
Nodes (18): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), Tests that can fail, Tests that changed (+10 more)

### Community 44 - ".process"
Cohesion: 0.10
Nodes (11): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, RevBenchProcessor (+3 more)

### Community 45 - "numpy"
Cohesion: 0.21
Nodes (15): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+7 more)

### Community 46 - "test_audio.c"
Cohesion: 0.23
Nodes (20): rb_preset_find(), Tests that can fail, Tests that can fail, amplitude(), band_power(), engine_sim(), order_amplitude(), record_tailpipes() (+12 more)

### Community 47 - "Selective sound rollback — 2026-10-22"
Cohesion: 0.50
Nodes (4): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 51 - "render_results.py"
Cohesion: 0.11
Nodes (12): coast_markers(), expected_clips(), listening_wav(), main(), render_checkpoint(), replace_current(), sha(), write_page() (+4 more)

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
Cohesion: 0.11
Nodes (17): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+9 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 67 - "matplotlib_pyplot"
Cohesion: 0.21
Nodes (11): chart(), plot_spark(), colorbar(), compare(), engine(), grid(), main(), pulse() (+3 more)

### Community 68 - "high_rpm_report.py"
Cohesion: 0.12
Nodes (12): write_float(), comparison(), main(), report(), window_metrics(), excerpt(), main(), spectral_metrics() (+4 more)

### Community 69 - "audio_source_probe.c"
Cohesion: 0.29
Nodes (15): rb_soft_limit(), coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame() (+7 more)

### Community 70 - "read_csv"
Cohesion: 0.20
Nodes (11): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+3 more)

### Community 71 - "rb_engine_init_at"
Cohesion: 0.19
Nodes (30): rb_chamber_init_reservoir(), rb_engine_init_at(), face_init(), rb_pipe_cells_for(), rb_pipe_end_cell(), rb_pipe_init(), rb_pipe_join(), rb_pipe_onward() (+22 more)

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 73 - "argparse"
Cohesion: 0.17
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.21
Nodes (16): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), best_spark(), run_held(), test_calibration_burn_and_spark() (+8 more)

### Community 76 - "style"
Cohesion: 0.23
Nodes (10): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+2 more)

### Community 77 - "rb_test.h"
Cohesion: 0.38
Nodes (5): all_gaps(), count_sparks(), firing_gaps(), gaps_are(), test_presets_fire_where_the_layout_says()

### Community 78 - "read_wav"
Cohesion: 0.13
Nodes (8): read_wav(), read_wav_bytes(), read_wav(), main(), orders(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "rb_create"
Cohesion: 0.19
Nodes (24): config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+16 more)

### Community 80 - "rb_chamber_init"
Cohesion: 0.33
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 81 - "presets.c"
Cohesion: 0.46
Nodes (13): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+5 more)

### Community 83 - "rb_engine_init"
Cohesion: 0.20
Nodes (20): rb_engine_contents(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat(), rb_preset_desc(), Tests that can fail, Tests that can fail, repairs() (+12 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.08
Nodes (20): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+12 more)

### Community 85 - "test_scene.c"
Cohesion: 0.32
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 87 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.33
Nodes (5): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 91 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 92 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 93 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 94 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 95 - "test_combustion.c"
Cohesion: 0.25
Nodes (5): rb_burnable_fuel(), rb_wiebe(), test_burnable_fuel(), test_fuel_heat(), test_wiebe_burn()

### Community 96 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 98 - "setup"
Cohesion: 0.27
Nodes (6): first(), main(), plot(), spans(), window(), setup()

### Community 99 - "pipe.h"
Cohesion: 0.33
Nodes (4): rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), test_radiation_causal_impedance()

### Community 100 - "rb_audit.c"
Cohesion: 0.29
Nodes (6): rb_version(), add(), audit(), main(), number(), usage()

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.20
Nodes (9): Checked against chemical equilibrium, Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it (+1 more)

### Community 103 - "Audio robustness: research and candidate roadmap — 2026-10-22"
Cohesion: 0.22
Nodes (9): Audio robustness: research and candidate roadmap — 2026-10-22, Candidate small-stage sequence and audit coverage, Conditions for an exploratory neural residual, Proposed gates and reproducible experiment fixtures, Reference use and data still needed, Research scope and verification, Runtime budgets consistent with realism first, Two transport candidates, after the diagnostic experiment (+1 more)

### Community 104 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 105 - "sys"
Cohesion: 0.36
Nodes (5): load_table(), main(), run(), main(), plot_map()

### Community 106 - "Stage S1b — Finite-time partial burns"
Cohesion: 0.25
Nodes (7): Completed before/after audit, Equations and behavior, Remaining work, Sound check and listening checkpoint, Sources and limits, Stage S1b — Finite-time partial burns, Why no fixed residual cutoff

### Community 107 - "S2c — Source, transport and generation-architecture experiment"
Cohesion: 0.25
Nodes (8): Cost and architecture decision, Engine results, Experimental scope and controls, Outcome, Pipe results: magnitude is not enough, References and listening result, Reproduction, evidence and validation, S2c — Source, transport and generation-architecture experiment

### Community 108 - "Sound robustness and realism programme"
Cohesion: 0.29
Nodes (7): Architecture decision after S2d, Engineering direction, One method for every change, Reference and listening method, Research and audit coverage, Small stages and decision gates, Sound robustness and realism programme

### Community 109 - "rb_bench.c"
Cohesion: 0.67
Nodes (6): engine_cost(), main(), mixture_ns(), newton_ns(), now(), nozzle_ns()

### Community 110 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 111 - "rb_dc_blocker_init"
Cohesion: 0.70
Nodes (4): rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step(), test_audio_dc_blocker()

### Community 112 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 627 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **23 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_cylinder_move`, `rb_engine_step`, `rb_engine_api.c`, `rb_pipe_flows`, `rb_engine_init_at`, `REV-BENCH — notes for Claude`, `rb_engine_desc_default`, `Tests that can fail`, `RbExports`, `rb_create`, `rb_chamber_init`, `audio.c`, `Stage 8 — Engine layouts & presets`, `rb_engine_init`, `rb_engine_init`, `gas.c`, `pipe.c`, `test_combustion.c`?**
  _High betweenness centrality (0.193) - this node is a cross-community bridge._
- **Are the 83 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 83 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_variation.c` be split into smaller, more focused modules?**
  _Cohesion score 0.1349527665317139 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `.process`, `RbSim`, `RbCore`, `test_engine.c`?**
  _High betweenness centrality (0.172) - this node is a cross-community bridge._
- **Are the 53 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 53 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.06015037593984962 - nodes in this community are weakly interconnected._