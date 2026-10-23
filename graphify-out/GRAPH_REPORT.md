# Graph Report - rev-bench  (2026-10-23)

## Corpus Check
- 199 files · ~608,138 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 2155 nodes · 5918 edges · 112 communities (89 shown, 23 thin omitted)
- Extraction: 74% EXTRACTED · 26% INFERRED · 0% AMBIGUOUS · INFERRED: 1568 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `832af05b`
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
- subprocess
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
- test_presets.c
- Model limitations and validation still requiring attention
- RbCore
- rb_engine_init
- compilerOptions
- math
- checkpoint_13b.py
- gas.c
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- argparse
- noise.c
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- hashlib
- rb_destroy
- csv
- Stage S1c — Reproducible sound and combustion baselines
- Stages
- REV-BENCH — Research notes (stage 0)
- S2d design — coupled exhaust acoustics and learned-DSP benchmarks
- Stage 13 — Performance & stability pass
- Stage 3 — Gas & flow primitives
- cmd_info
- .process
- numpy
- test_audio.c
- RESEARCH.md
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
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- math
- matplotlib_pyplot
- high_rpm_report.py
- rb_cylinder_init
- read_csv
- rb_engine_init_at
- REV-BENCH — notes for Claude
- argparse
- rb_engine_desc_default
- AGENTS.md
- style
- Stage 8 — Engine layouts & presets
- read_wav
- rb_create
- test_cylinder.c
- test_combustion.c
- rb_engine_init
- REV-BENCH — Feedback checkpoint before stage 13b
- test_scene.c
- S2b — Loaded exhaust detail and the lift-off contrast
- Stage 5 — Valvetrain, throttle & intake plenum
- Current sound results
- fuzz.c
- Stage 6 — Combustion + heat transfer
- Stage 13b-r — high-RPM exhaust character
- setup
- test_acoustic_candidates.c
- rb_bench.c
- Initial audio bottleneck diagnosis — 2026-10-22
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- Stage 1 — Native C scaffold, WAV writer, test runner
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- S2d — first native acoustic-renderer candidates
- rand.h
- Should REV-BENCH change its sound-generation architecture?

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
10. `cmd_sound()` - 33 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  PLAN.md → core/src/chamber.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-05.md → core/src/cylinder.c
- `Unequal headers (`core/src/presets.c`)` --references--> `rb_engine_desc_default()`  [INFERRED]
  docs/stage-12c.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (112 total, 23 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.14
Nodes (35): rb_engine_cylinder_angle(), rb_engine_step(), rb_get_cylinder(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift() (+27 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.12
Nodes (17): rb_cylinder_move(), Crank angle convention, Equations used, Notes for later stages, Results, Sources, Stage 4 — Single cylinder: slider-crank + motoring, Try it (+9 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.09
Nodes (42): rb_burnable_fuel(), rb_wiebe(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake() (+34 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.17
Nodes (29): describe_pipe(), field_ok(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags(), rb_engine_field_max(), rb_engine_field_min(), rb_engine_field_name() (+21 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.14
Nodes (39): rb_engine_displacement(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe(), cmd_dyno() (+31 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "subprocess"
Cohesion: 0.13
Nodes (11): cases(), check_topology_identity(), digest(), main(), read_row(), run(), write_csv(), build() (+3 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "presets.c"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "wav.c"
Cohesion: 0.06
Nodes (62): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+54 more)

### Community 13 - "compare_real.py"
Cohesion: 0.11
Nodes (17): a_weighting(), as_db(), band(), chart(), compare(), fetch(), main(), measure() (+9 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "rbplot.py"
Cohesion: 0.15
Nodes (11): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+3 more)

### Community 17 - "audio.c"
Cohesion: 0.18
Nodes (23): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step() (+15 more)

### Community 18 - "test_presets.c"
Cohesion: 0.62
Nodes (5): all_gaps(), count_sparks(), firing_gaps(), gaps_are(), test_presets_fire_where_the_layout_says()

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (31): Confirmed implementation and numerical issues, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events, D09 — P2: propagation interpolation itself darkens the sound (+23 more)

### Community 21 - "rb_engine_init"
Cohesion: 0.23
Nodes (24): rb_noise_init(), rb_noise_step(), Tests that can fail, cabin_expected(), db(), flow_rms(), impact_energy(), jet_expected() (+16 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "math"
Cohesion: 0.10
Nodes (25): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), at_rpm(), running(), test_overrun_reset_and_legacy() (+17 more)

### Community 24 - "checkpoint_13b.py"
Cohesion: 0.50
Nodes (6): analyze(), long_coast(), main(), record(), render(), sha()

### Community 25 - "gas.c"
Cohesion: 0.22
Nodes (19): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_T_cv_from_u(), rb_gas_T_from_u(), rb_gas_u() (+11 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (47): Core (C), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth(), ambient_density() (+39 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "argparse"
Cohesion: 0.20
Nodes (10): digest(), main(), deceleration_report(), main(), record(), sha(), bench(), compare() (+2 more)

### Community 31 - "noise.c"
Cohesion: 0.28
Nodes (10): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+2 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "hashlib"
Cohesion: 0.17
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 34 - "rb_destroy"
Cohesion: 0.19
Nodes (23): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_destroy(), rb_free_floats(), rb_set_param(), rb_get_telemetry() (+15 more)

### Community 35 - "csv"
Cohesion: 0.14
Nodes (15): analyze(), brief(), plot_summary(), read(), select(), self_test(), encode_listening(), main() (+7 more)

### Community 36 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

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
Cohesion: 0.09
Nodes (21): D08 — P1: randomized stability checks omit the new junction chamber, Tests that changed, Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass (+13 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "cmd_info"
Cohesion: 0.26
Nodes (18): rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find() (+10 more)

### Community 44 - ".process"
Cohesion: 0.15
Nodes (5): How the sound gets out, RevBenchProcessor, makeImports(), renderSeconds(), setup()

### Community 45 - "numpy"
Cohesion: 0.21
Nodes (15): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+7 more)

### Community 46 - "test_audio.c"
Cohesion: 0.19
Nodes (20): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), rb_soft_limit(), Tests that can fail, amplitude() (+12 more)

### Community 47 - "RESEARCH.md"
Cohesion: 0.29
Nodes (5): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22, REV-BENCH

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 51 - "render_results.py"
Cohesion: 0.12
Nodes (10): coast_markers(), expected_clips(), listening_wav(), main(), sha(), write_page(), checkpoint(), float_wav() (+2 more)

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
Cohesion: 0.10
Nodes (19): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+11 more)

### Community 64 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 66 - "math"
Cohesion: 0.11
Nodes (8): rb_exhaust_model(), add(), audit(), main(), number(), usage(), now(), output_rms()

### Community 67 - "matplotlib_pyplot"
Cohesion: 0.27
Nodes (8): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save()

### Community 68 - "high_rpm_report.py"
Cohesion: 0.16
Nodes (10): write_float(), main(), report(), window_metrics(), excerpt(), main(), spectral_metrics(), telemetry() (+2 more)

### Community 69 - "rb_cylinder_init"
Cohesion: 0.28
Nodes (12): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_wall_area(), rb_woschni_h() (+4 more)

### Community 70 - "read_csv"
Cohesion: 0.19
Nodes (12): comparison(), curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark() (+4 more)

### Community 71 - "rb_engine_init_at"
Cohesion: 0.05
Nodes (118): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+110 more)

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 73 - "argparse"
Cohesion: 0.15
Nodes (11): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+3 more)

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.23
Nodes (15): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), best_spark(), run_held(), test_calibration_burn_and_spark() (+7 more)

### Community 76 - "style"
Cohesion: 0.25
Nodes (9): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+1 more)

### Community 77 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 78 - "read_wav"
Cohesion: 0.18
Nodes (5): read_wav(), read_wav_bytes(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "rb_create"
Cohesion: 0.19
Nodes (24): config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+16 more)

### Community 80 - "test_cylinder.c"
Cohesion: 0.29
Nodes (8): compress(), engine_86x86(), isentrope_T(), test_big_volume_jump_is_split(), test_compression_converges_with_step_size(), test_compression_matches_isentrope(), test_cylinder_bad_input(), test_motoring_has_no_energy_drift()

### Community 81 - "test_combustion.c"
Cohesion: 0.25
Nodes (5): rb_fuel_heat(), rb_species_R(), Checked against chemical equilibrium, test_constant_volume_burn(), test_fuel_heat()

### Community 83 - "rb_engine_init"
Cohesion: 0.14
Nodes (36): rb_engine_contents(), rb_engine_init(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat(), slow_flows(), slow_step(), rb_preset_count() (+28 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.09
Nodes (19): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+11 more)

### Community 85 - "test_scene.c"
Cohesion: 0.32
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 91 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.25
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 92 - "Stage 5 — Valvetrain, throttle & intake plenum"
Cohesion: 0.20
Nodes (9): Equations used, How one step works, Notes for later stages, Results, Sources, Stage 5 — Valvetrain, throttle & intake plenum, Try it, Two things that surprised me (+1 more)

### Community 93 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 94 - "fuzz.c"
Cohesion: 0.31
Nodes (5): check_chamber(), check_engine(), fail(), seed_state(), uniform()

### Community 95 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.22
Nodes (8): Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it, What was built

### Community 96 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 98 - "setup"
Cohesion: 0.27
Nodes (6): first(), main(), plot(), spans(), window(), titles()

### Community 99 - "test_acoustic_candidates.c"
Cohesion: 0.08
Nodes (56): Validation and use, add(), conservative(), fluxes(), hllc(), mc(), neighbor(), physical_flux() (+48 more)

### Community 100 - "rb_bench.c"
Cohesion: 0.57
Nodes (7): rb_gas_init(), engine_cost(), main(), mixture_ns(), newton_ns(), now(), nozzle_ns()

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

### Community 105 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.33
Nodes (5): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

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

### Community 110 - "rand.h"
Cohesion: 0.70
Nodes (4): rb_rand_next(), rb_rand_normal(), rb_rand_uniform(), rb_rand_white()

### Community 112 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 657 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **23 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_cylinder_move`, `rb_engine_step`, `rb_engine_init_at`, `REV-BENCH — notes for Claude`, `Stage 13 — Performance & stability pass`, `rb_engine_desc_default`, `RbExports`, `rb_create`, `audio.c`, `rb_engine_init`, `rb_engine_init`, `gas.c`?**
  _High betweenness centrality (0.179) - this node is a cross-community bridge._
- **Are the 83 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 83 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_variation.c` be split into smaller, more focused modules?**
  _Cohesion score 0.1440185830429733 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `.process`, `Stage 8 — Engine layouts & presets`, `RbSim`, `RbCore`, `test_engine.c`?**
  _High betweenness centrality (0.160) - this node is a cross-community bridge._
- **Are the 53 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 53 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.11695906432748537 - nodes in this community are weakly interconnected._