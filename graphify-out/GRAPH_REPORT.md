# Graph Report - rev-bench  (2026-10-23)

## Corpus Check
- 219 files · ~622,006 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 2298 nodes · 6295 edges · 126 communities (105 shown, 21 thin omitted)
- Extraction: 75% EXTRACTED · 25% INFERRED · 0% AMBIGUOUS · INFERRED: 1602 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `83b6697b`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_variation.c
- Stage 4 — Single cylinder: slider-crank + motoring
- rb_engine_step
- rb_engine_step
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- subprocess
- main.ts
- presets.c
- package.json
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- RbExports
- compare_real.py
- RbSim
- style
- rbplot.py
- audio.c
- test_presets.c
- Model limitations and validation still requiring attention
- coupled_duct.c
- coupled_duct_report.py
- compilerOptions
- math
- RevBenchNode
- gas.c
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- argparse
- rb_cylinder_init
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- hashlib
- rb_destroy
- csv
- Structural findings that change the next step
- Stages
- REV-BENCH — Research notes (stage 0)
- S2d design — coupled exhaust acoustics and learned-DSP benchmarks
- Stage 13 — Performance & stability pass
- Stage 3 — Gas & flow primitives
- wav.c
- REV-BENCH — notes for Claude
- numpy
- Test-Driven Development
- RESEARCH.md
- Stage 13b-p — researched exhaust configurations
- README.md
- render_results.py
- Process
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- string
- matplotlib_pyplot
- high_rpm_report.py
- test_combustion.c
- read_csv
- pipe.c
- test_cylinder.c
- argparse
- rb_engine_desc_default
- AGENTS.md
- style
- Stage 8 — Engine layouts & presets
- read_wav
- rb_create
- audio_source_probe.c
- Stage 5 — Valvetrain, throttle & intake plenum
- rb_engine_init
- REV-BENCH — Feedback checkpoint before stage 13b
- Stage 6 — Combustion + heat transfer
- test_pipe.c
- S2b — Loaded exhaust detail and the lift-off contrast
- Current sound results
- rb_chamber_gamma
- rb_bench.c
- Stage 13b-r — high-RPM exhaust character
- setup
- test_acoustic_candidates.c
- Initial audio bottleneck diagnosis — 2026-10-22
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- S2d — first native acoustic-renderer candidates
- rb_network_step
- rb_chamber_init
- Should REV-BENCH change its sound-generation architecture?
- checkpoint_sections.py
- test_audio.c
- measure
- acoustic_transport_probe.c
- rb_decimator_init
- pipe.h
- Phase 1 — Physics core (native, offline, validated)
- Stage 1 — Native C scaffold, WAV writer, test runner
- Stage S1c — Reproducible sound and combustion baselines
- rb_dc_blocker_init
- S2d-R, S2d-0b and S2d-B1a — first open coupling checkpoint
- Results
- Native acoustic candidates — S2d

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 103 edges
2. `rb_engine_init()` - 57 edges
3. `RbExports` - 53 edges
4. `rb_engine_init_at()` - 43 edges
5. `Core (C)` - 41 edges
6. `style()` - 38 edges
7. `rb_engine_settle()` - 36 edges
8. `rb_chamber_init()` - 35 edges
9. `rb_preset_desc()` - 34 edges
10. `read_wav()` - 34 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  PLAN.md → core/src/chamber.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-04.md → core/src/cylinder.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-05.md → core/src/cylinder.c

## Import Cycles
- None detected.

## Communities (126 total, 21 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.16
Nodes (32): rb_engine_cylinder_angle(), rb_engine_step(), rb_get_cylinder(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift() (+24 more)

### Community 1 - "Stage 4 — Single cylinder: slider-crank + motoring"
Cohesion: 0.25
Nodes (7): Crank angle convention, Equations used, Notes for later stages, Results, Sources, Stage 4 — Single cylinder: slider-crank + motoring, Try it

### Community 2 - "rb_engine_step"
Cohesion: 0.11
Nodes (18): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that changed, The accessories (`accessory_power`, an engine field) (+10 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.11
Nodes (36): rb_burnable_fuel(), rb_wiebe(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake() (+28 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.07
Nodes (57): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+49 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.14
Nodes (39): rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe(), cmd_dyno(), cmd_fire() (+31 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.06
Nodes (47): wasi, EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER (+39 more)

### Community 7 - "subprocess"
Cohesion: 0.18
Nodes (10): cases(), check_topology_identity(), digest(), main(), read_row(), run(), write_csv(), build() (+2 more)

### Community 8 - "main.ts"
Cohesion: 0.13
Nodes (24): CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam(), findTelemetry() (+16 more)

### Community 9 - "presets.c"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.10
Nodes (20): A pipe end is a monopole, Cost, DC blocker and soft limiter, Down to the audio rate, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧 (+12 more)

### Community 12 - "RbExports"
Cohesion: 0.07
Nodes (3): What was built, RbCore, RbExports

### Community 13 - "compare_real.py"
Cohesion: 0.14
Nodes (13): band(), chart(), compare(), fetch(), main(), read_list(), read_wav(), repeat() (+5 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "rbplot.py"
Cohesion: 0.14
Nodes (10): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+2 more)

### Community 17 - "audio.c"
Cohesion: 0.23
Nodes (20): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_listener_distance(), rb_listener_hear(), rb_listener_place() (+12 more)

### Community 18 - "test_presets.c"
Cohesion: 0.26
Nodes (14): muffler_width(), rb_engine_desc_check(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), Results, test_engine_description(), all_gaps() (+6 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (33): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+25 more)

### Community 20 - "coupled_duct.c"
Cohesion: 0.18
Nodes (29): add(), admissible(), book(), chars(), end_state(), linear_flux(), linear_rhs(), linear_tick() (+21 more)

### Community 21 - "coupled_duct_report.py"
Cohesion: 0.19
Nodes (8): publish(), assess(), exact_pressure(), initial(), read(), run(), CoupledProofTests, fixture()

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "math"
Cohesion: 0.10
Nodes (25): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), at_rpm(), running(), test_overrun_reset_and_legacy() (+17 more)

### Community 24 - "RevBenchNode"
Cohesion: 0.19
Nodes (7): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, Running

### Community 25 - "gas.c"
Cohesion: 0.20
Nodes (22): rb_fuel_heat(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u() (+14 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.09
Nodes (22): Checks, Light load, Notes for later stages, Review of stage 9, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes, The ends, The engine computer looks ahead (+14 more)

### Community 28 - "test_engine.c"
Cohesion: 0.11
Nodes (48): Core (C), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth(), ambient_density() (+40 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "argparse"
Cohesion: 0.16
Nodes (18): digest(), main(), analyze(), long_coast(), main(), record(), render(), sha() (+10 more)

### Community 31 - "rb_cylinder_init"
Cohesion: 0.28
Nodes (13): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), What was built (+5 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "hashlib"
Cohesion: 0.17
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 34 - "rb_destroy"
Cohesion: 0.17
Nodes (25): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_destroy(), rb_free_floats(), rb_set_param(), rb_get_telemetry() (+17 more)

### Community 35 - "csv"
Cohesion: 0.13
Nodes (14): analyze(), brief(), plot_summary(), read(), select(), self_test(), encode_listening(), main() (+6 more)

### Community 36 - "Structural findings that change the next step"
Cohesion: 0.14
Nodes (14): 1. Same-tick return pressure needs an API boundary, 2. Port signs, pressure reference and work ownership must be explicit, 3. The background gas cannot be removed with its acoustic grid, 4. Shared geometry is currently a snapshot, not an executable compiler, 5. Stable output can conceal modified gas transport, 6. Native stereo and the browser host are different validation targets, 7. The single results file is not reproducibly rebuildable today, 8. Provenance and reference conditions still limit attribution (+6 more)

### Community 37 - "Stages"
Cohesion: 0.05
Nodes (42): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism (+34 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.10
Nodes (20): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), After stage 12b: a piano note (stage 12c) (+12 more)

### Community 39 - "S2d design — coupled exhaust acoustics and learned-DSP benchmarks"
Cohesion: 0.12
Nodes (16): A1 — conservative 1D comparison, Architecture and ownership, B0 — fixed-background acoustic graph, B1 — gas coupling, mean flow and lift-off, Comparison framework and rejection gates, Components, radiation and usable band, Decision and experimental arms, Learned synthesis: experiments and deployment (+8 more)

### Community 41 - "Stage 13 — Performance & stability pass"
Cohesion: 0.12
Nodes (15): Cost, Measuring, Notes for later stages, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls, The pipes (+7 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "wav.c"
Cohesion: 0.14
Nodes (28): Dither, build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32() (+20 more)

### Community 44 - "REV-BENCH — notes for Claude"
Cohesion: 0.08
Nodes (15): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+7 more)

### Community 45 - "numpy"
Cohesion: 0.22
Nodes (15): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+7 more)

### Community 46 - "Test-Driven Development"
Cohesion: 0.15
Nodes (10): Designing for Mockability, When to Mock, Anti-patterns, Rules of the loop, Seams: where tests go, Test-Driven Development, What a good test is, Bad Tests (+2 more)

### Community 47 - "RESEARCH.md"
Cohesion: 0.29
Nodes (5): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22, REV-BENCH

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 50 - "README.md"
Cohesion: 0.25
Nodes (3): Shared exhaust geometry with bidirectional physical loading, Language, REV-BENCH

### Community 51 - "render_results.py"
Cohesion: 0.12
Nodes (13): read_wav_bytes(), coast_markers(), expected_clips(), listening_wav(), main(), render_checkpoint(), replace_current(), sha() (+5 more)

### Community 52 - "Process"
Cohesion: 0.15
Nodes (12): 1. Gather context, 2. Explore the codebase (optional), 3. Draft vertical slices, 4. Quiz the user, 5. Publish the tickets to the configured tracker, Acceptance criteria, Blocked by, <NN>: <Ticket title> (+4 more)

### Community 54 - "Stage 13b — Performance feedback checkpoint"
Cohesion: 0.25
Nodes (8): 2026-10-22 continuation: coupled port experiment and researched exhausts, Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance and solver work, Validation and sound, What was built, Why the implicit probe was rejected

### Community 55 - "Bernardo's engine sound examples — 2026-10-21"
Cohesion: 0.20
Nodes (10): Additional rear cross V8 recordings — 2026-10-23, Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and distance, Processing and reproduction, R1b: optional filtered listening copies — 2026-10-21, Reproduction and checkpoint, Selection and perspective (+2 more)

### Community 60 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.09
Nodes (22): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+14 more)

### Community 64 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 66 - "string"
Cohesion: 0.06
Nodes (56): rb_exhaust_model(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole() (+48 more)

### Community 67 - "matplotlib_pyplot"
Cohesion: 0.27
Nodes (8): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save()

### Community 68 - "high_rpm_report.py"
Cohesion: 0.14
Nodes (5): window_metrics(), spectral_metrics(), CheckpointRenderTests, HighRpmMetricsTests, LiftMetricsTests

### Community 69 - "test_combustion.c"
Cohesion: 0.21
Nodes (7): rb_wall_area(), rb_woschni_h(), rb_woschni_w(), test_burnable_fuel(), test_fuel_heat(), test_wiebe_burn(), test_woschni()

### Community 70 - "read_csv"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 71 - "pipe.c"
Cohesion: 0.18
Nodes (29): cell_heat(), cell_is_fast(), end_flow(), face_init(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux() (+21 more)

### Community 72 - "test_cylinder.c"
Cohesion: 0.43
Nodes (5): compress(), isentrope_T(), test_big_volume_jump_is_split(), test_compression_converges_with_step_size(), test_compression_matches_isentrope()

### Community 73 - "argparse"
Cohesion: 0.15
Nodes (12): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+4 more)

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.23
Nodes (15): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), best_spark(), run_held(), test_calibration_burn_and_spark() (+7 more)

### Community 76 - "style"
Cohesion: 0.21
Nodes (11): comparison(), between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs() (+3 more)

### Community 77 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.14
Nodes (9): Idle tuning, Notes for later stages, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+1 more)

### Community 78 - "read_wav"
Cohesion: 0.19
Nodes (4): read_wav(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "rb_create"
Cohesion: 0.19
Nodes (24): rb_soft_limit(), config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample() (+16 more)

### Community 80 - "audio_source_probe.c"
Cohesion: 0.32
Nodes (14): coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame(), path_name() (+6 more)

### Community 81 - "Stage 5 — Valvetrain, throttle & intake plenum"
Cohesion: 0.20
Nodes (9): Equations used, How one step works, Notes for later stages, Results, Sources, Stage 5 — Valvetrain, throttle & intake plenum, Try it, Two things that surprised me (+1 more)

### Community 83 - "rb_engine_init"
Cohesion: 0.12
Nodes (41): new_pipe(), rb_engine_contents(), rb_engine_init(), rb_engine_init_at(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat(), rb_pipe_cells_for() (+33 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.08
Nodes (20): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+12 more)

### Community 85 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.20
Nodes (9): Checked against chemical equilibrium, Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it (+1 more)

### Community 91 - "test_pipe.c"
Cohesion: 0.23
Nodes (26): rb_chamber_init_reservoir(), rb_pipe_init(), rb_pipe_join(), rb_pipe_pack(), Tests that can fail, extreme(), packed_ring(), packet_energy_ratio() (+18 more)

### Community 92 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.25
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 93 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 94 - "rb_chamber_gamma"
Cohesion: 0.17
Nodes (20): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), move_gas() (+12 more)

### Community 95 - "rb_bench.c"
Cohesion: 0.57
Nodes (6): rb_version(), main(), mixture_ns(), newton_ns(), now(), nozzle_ns()

### Community 96 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 98 - "setup"
Cohesion: 0.27
Nodes (6): first(), main(), plot(), spans(), window(), setup()

### Community 99 - "test_acoustic_candidates.c"
Cohesion: 0.09
Nodes (57): Implemented scope, add(), conservative(), fluxes(), hllc(), mc(), neighbor(), physical_flux() (+49 more)

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.29
Nodes (6): Equations used, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built

### Community 103 - "Audio robustness: research and candidate roadmap — 2026-10-22"
Cohesion: 0.22
Nodes (9): Audio robustness: research and candidate roadmap — 2026-10-22, Candidate small-stage sequence and audit coverage, Conditions for an exploratory neural residual, Proposed gates and reproducible experiment fixtures, Reference use and data still needed, Research scope and verification, Runtime budgets consistent with realism first, Two transport candidates, after the diagnostic experiment (+1 more)

### Community 104 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 106 - "Stage S1b — Finite-time partial burns"
Cohesion: 0.25
Nodes (7): Completed before/after audit, Equations and behavior, Remaining work, Sound check and listening checkpoint, Sources and limits, Stage S1b — Finite-time partial burns, Why no fixed residual cutoff

### Community 107 - "S2c — Source, transport and generation-architecture experiment"
Cohesion: 0.25
Nodes (8): Cost and architecture decision, Engine results, Experimental scope and controls, Outcome, Pipe results: magnitude is not enough, References and listening result, Reproduction, evidence and validation, S2c — Source, transport and generation-architecture experiment

### Community 108 - "Sound robustness and realism programme"
Cohesion: 0.29
Nodes (7): Architecture decision after S2d, Engineering direction, One method for every change, Reference and listening method, Research and audit coverage, Small stages and decision gates, Sound robustness and realism programme

### Community 109 - "S2d — first native acoustic-renderer candidates"
Cohesion: 0.33
Nodes (6): Implemented modules and contracts, Matrix and numerical results, Outcome, Reproduction and research, S2d — first native acoustic-renderer candidates, Validation, publication and remaining work

### Community 110 - "rb_network_step"
Cohesion: 0.20
Nodes (15): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), blowdown_time_to_2bar(), setup_blowdown() (+7 more)

### Community 111 - "rb_chamber_init"
Cohesion: 0.33
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 112 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

### Community 113 - "checkpoint_sections.py"
Cohesion: 0.33
Nodes (11): adopt_current(), carry_diagnostics(), checked(), compose(), diagnostic_names(), fingerprint(), load(), publish_section() (+3 more)

### Community 114 - "test_audio.c"
Cohesion: 0.36
Nodes (10): Tests that can fail, amplitude(), engine_sim(), render_engine(), test_audio_chain_blocks_dc(), test_audio_chain_survives_nan(), test_audio_engine_rebuilt_quietly(), test_audio_engine_reversal_quiet() (+2 more)

### Community 115 - "measure"
Cohesion: 0.23
Nodes (8): a_weighting(), as_db(), measure(), thirds(), deceleration_report(), main(), record(), sha()

### Community 116 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 117 - "rb_decimator_init"
Cohesion: 0.33
Nodes (9): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), response(), test_audio_decimator_filter(), test_audio_decimator_tones() (+1 more)

### Community 118 - "pipe.h"
Cohesion: 0.33
Nodes (4): rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), test_radiation_causal_impedance()

### Community 119 - "Phase 1 — Physics core (native, offline, validated)"
Cohesion: 0.25
Nodes (8): Phase 1 — Physics core (native, offline, validated), Stage 3 — Gas & flow primitives, Stage 4 — Single cylinder: slider-crank + motoring, Stage 5 — Valvetrain, throttle & intake plenum, Stage 6 — Combustion + heat transfer, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Stage 8 — Engine layouts & presets, Stage 9 — Dyno + validation

### Community 120 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.33
Nodes (5): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 121 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 122 - "rb_dc_blocker_init"
Cohesion: 0.70
Nodes (4): rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step(), test_audio_dc_blocker()

### Community 123 - "S2d-R, S2d-0b and S2d-B1a — first open coupling checkpoint"
Cohesion: 0.40
Nodes (5): Experiment and acceptance, Measured outcome, Method and physical ownership, Reproduce, S2d-R, S2d-0b and S2d-B1a — first open coupling checkpoint

### Community 124 - "Results"
Cohesion: 0.50
Nodes (4): Against the real engines, At 96 kHz, Results, What doesn't look like a real engine yet

### Community 125 - "Native acoustic candidates — S2d"
Cohesion: 0.67
Nodes (3): Native acoustic candidates — S2d, Open-port coupling checkpoint, Reproduce

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 706 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **21 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `string`, `rb_engine_step`, `rb_engine_api.c`, `pipe.c`, `rb_engine_desc_default`, `RbExports`, `REV-BENCH — notes for Claude`, `rb_network_step`, `rb_chamber_init`, `rb_create`, `audio.c`, `test_presets.c`, `rb_engine_init`, `gas.c`, `rb_chamber_gamma`, `rb_cylinder_init`?**
  _High betweenness centrality (0.191) - this node is a cross-community bridge._
- **Are the 83 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 83 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.10526315789473684 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `REV-BENCH — notes for Claude`, `Stage 8 — Engine layouts & presets`, `RbSim`, `test_engine.c`?**
  _High betweenness centrality (0.163) - this node is a cross-community bridge._
- **Are the 53 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 53 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.10569105691056911 - nodes in this community are weakly interconnected._