# Graph Report - REV-BENCH  (2026-11-08)

## Corpus Check
- 398 files · ~793,420 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 11 file(s) not represented in the graph (top: (none) 5, .csv 2, .bin 1)

## Summary
- 3751 nodes · 10863 edges · 203 communities (159 shown, 44 thin omitted)
- Extraction: 78% EXTRACTED · 22% INFERRED · 0% AMBIGUOUS · INFERRED: 2427 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `a1c598e4`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_variation.c
- PLAN.md
- Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧
- csv
- audit_s1.py
- test_acoustic_candidates.c
- rb-core.ts
- Stages
- main.ts
- presets.c
- package.json
- rb_engine_api.c
- RbExports
- publish_section
- RbSim
- style
- rb_drive.c
- test_audio.c
- rb_destroy
- Model limitations and validation still requiring attention
- coupled_duct.c
- json
- compilerOptions
- combustion.c
- struct
- rb_gas_init
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- rb_events.c
- fixed_transport.c
- rb_network_step
- Stage 9 — Dyno + validation against real engines
- contracts/README.md
- Stage 13 — Performance & stability pass
- cmd_info
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- rb_gas_init
- high_rpm_report.py
- S2d design — coupled exhaust acoustics and learned-DSP benchmarks
- test_pipe.c
- audio_source_probe.c
- throttle.c
- engine-node.ts
- test_scene.c
- wav.c
- render_results.py
- REV-BENCH as a desktop application — the design
- Should REV-BENCH change its sound-generation architecture?
- RevBenchNode
- review-completed-stages-2026-10-23.md
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage 8 — Engine layouts & presets
- style
- REV-BENCH - Codex handoff
- string
- test_noise.c
- main
- The desktop application
- Canonical stage tracker
- numpy
- rb_engine_init_at
- Existing sound, measurements and bounded prototypes
- Stage 036 — complete project roadmap and evidence protocol
- pathlib
- AGENTS.md
- rb_tw_step
- math
- rb_create
- Stage 3 — Gas & flow primitives
- rb_cli.c
- string
- REV-BENCH — Feedback checkpoint before stage 13b
- check_combustion_variation.py
- acoustic_transport_probe.c
- rb_engine_layout
- Historical foundations and engine core
- REV-BENCH
- build_contract.py
- plot_run.py
- string
- Initial audio bottleneck diagnosis — 2026-10-22
- hashlib
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- preflight
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- rb_chamber_gamma
- engine.c
- setup
- rb_engine_init_at
- diagnostics.c
- Engine integration, architecture selection and base acceptance
- Propagation prototypes and physical components
- Stage 6 — Combustion + heat transfer
- material.c
- matplotlib_pyplot
- Roadmap execution and evidence protocol
- REV-BENCH — notes for Claude
- rb_cylinder_move
- Stage 045 — Production-mixture gas to actual B0 graph
- Optional expansions
- Complete 3D engine experience
- The port, persistence, final acceptance and release
- Roadmap, shared contracts and trustworthy excitation
- Turbocharged engine development
- plot_fire.py
- Knock, mechanical limits and damage
- Expanded engine and exhaust catalog
- Stage 4 — Single cylinder: slider-crank + motoring
- 04-propagation.md
- count
- Stage 039 — two supplied dashboard references
- Stage 113 — Acoustic derived-state and numerical failure guards
- csv
- Conditional learned-synthesis research
- Stage039 — Matched references and calibration protocol
- style
- user-sound-references.md
- rb_dyno_init
- Stage039 — six supplied candidate videos reviewed
- numpy
- cmd_presets
- rb_audit.c
- test_diagnostic_orifice_thermal_and_radiation
- Structural findings that change the next step
- Stage040 — Spark, ignition and flame-development causality
- Stage041 — continuous combustion variation and residual validity
- stddef
- exact_riemann
- diagnostics.h
- os
- .test_failed_experiment_publish_keeps_its_diagnostics
- test_injection_events.py
- 02-existing-sound.md
- Stage 042 — Actual fuel-injection events and acoustic triggers
- rb_engine_init_at
- Stage 5 — Valvetrain, throttle & intake plenum
- test_overrun.c
- stddef
- test_combustion.c
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- The workspace
- S2b — Loaded exhaust detail and the lift-off contrast
- bench_wasm.mjs
- Stage 049 — finite junctions and all-port transfer
- fake_program.py
- Stage 119 — The review's leftovers
- REV-BENCH — Research notes (stage 0)
- Stage 120 — Workspace and toolset upkeep
- Stage 13b-r — high-RPM exhaust character
- Bounded completed-stage review repairs
- S2d — first native acoustic-renderer candidates
- Stage 038 — Numerical guard and flux observability
- S2b — Loaded exhaust detail and the lift-off contrast
- test_presets.c
- dyno_average
- Current sound results
- Should REV-BENCH change its sound-generation architecture?
- test_main.c
- Stage S1c — Reproducible sound and combustion baselines
- OrderTests
- Stage S1a — Reliable sound analysis before further tuning
- S2 — Additional flow sources and optional spatial resolution
- Selective sound rollback — 2026-10-22
- S1e — Research and controlled overrun transitions

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 124 edges
2. `rb_engine_init()` - 60 edges
3. `rb_engine_init_at()` - 56 edges
4. `RbExports` - 53 edges
5. `REV-BENCH - Codex handoff` - 53 edges
6. `Core (C)` - 49 edges
7. `rb_preset_desc()` - 48 edges
8. `program()` - 48 edges
9. `rb_destroy()` - 44 edges
10. `read_wav()` - 44 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/roadmap/legacy-plan.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-04.md → core/src/cylinder.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-05.md → core/src/cylinder.c

## Import Cycles
- None detected.

## Communities (203 total, 44 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.11
Nodes (37): rb_engine_cylinder_angle(), rb_get_cylinder(), worked_out(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift() (+29 more)

### Community 1 - "PLAN.md"
Cohesion: 0.18
Nodes (6): One canonical roadmap with evidence-gated promotion, A desktop application, not a browser page, Audit finding ownership and closure evidence, Legacy stage crosswalk, Required use, REV-BENCH

### Community 2 - "Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧"
Cohesion: 0.11
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "csv"
Cohesion: 0.18
Nodes (8): balance(), experiment(), evaluate(), run(), experiment(), evaluate(), experiment(), run()

### Community 4 - "audit_s1.py"
Cohesion: 0.17
Nodes (10): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), a_weighting() (+2 more)

### Community 5 - "test_acoustic_candidates.c"
Cohesion: 0.08
Nodes (73): Experiment and acceptance, Implemented scope, Measured outcome, Method and physical ownership, Reproduce, S2d-R, S2d-0b and S2d-B1a — first open coupling checkpoint, Geometry and approximations, Pass-by and room decay (+65 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (45): PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE, RB_EF_PER_BANK (+37 more)

### Community 7 - "Stages"
Cohesion: 0.05
Nodes (41): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism (+33 more)

### Community 8 - "main.ts"
Cohesion: 0.13
Nodes (24): CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam(), findTelemetry() (+16 more)

### Community 9 - "presets.c"
Cohesion: 0.08
Nodes (45): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+37 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "rb_engine_api.c"
Cohesion: 0.12
Nodes (37): rb_preset_count(), describe_pipe(), field_ok(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags(), rb_engine_field_max(), rb_engine_field_min() (+29 more)

### Community 12 - "RbExports"
Cohesion: 0.06
Nodes (4): The header, What was built, RbCore, RbExports

### Community 13 - "publish_section"
Cohesion: 0.18
Nodes (17): Frozen checks and execution, Scope, contract and method, Stage 112 — Recoverable checkpoint section publication, A failed publish no longer blocks `make results`, adopt_current(), carry_diagnostics(), checked(), compose() (+9 more)

### Community 14 - "RbSim"
Cohesion: 0.07
Nodes (5): Consumer review before implementation, Notes for later stages, What was built, EngineDescription, RbSim

### Community 15 - "style"
Cohesion: 0.20
Nodes (14): a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main(), orders() (+6 more)

### Community 16 - "rb_drive.c"
Cohesion: 0.12
Nodes (35): Engine build, Geometry, connections and frames, Shared contracts — version 1, Versioning, identity and limits, Rules, apply_build_settings(), apply_load(), build_id() (+27 more)

### Community 17 - "test_audio.c"
Cohesion: 0.14
Nodes (27): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), rb_soft_limit(), Tests that can fail, Equal and unequal headers (+19 more)

### Community 18 - "rb_destroy"
Cohesion: 0.18
Nodes (33): rb_preset_find(), rb_destroy(), rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that changed, set(), tlm() (+25 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (33): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+25 more)

### Community 20 - "coupled_duct.c"
Cohesion: 0.20
Nodes (29): add(), admissible(), book(), chars(), end_state(), linear_flux(), linear_rhs(), linear_tick() (+21 more)

### Community 21 - "json"
Cohesion: 0.07
Nodes (37): execute(), main(), acoustic_case(), compare(), flow_case(), main(), execute(), run() (+29 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "combustion.c"
Cohesion: 0.10
Nodes (25): 2026-10-31: Claude's review and Stage 115; paused for Bernardo, clamp(), flame_speed(), normal_cdf(), rb_burn_mean_duration(), rb_charge_burn_evaluate(), rb_kernel_delay_mean(), rb_kernel_delay_unit() (+17 more)

### Community 24 - "struct"
Cohesion: 0.10
Nodes (11): check_stream(), fail(), main(), parse(), split(), AudioAnalysisTests, chunk(), wav_bytes() (+3 more)

### Community 25 - "rb_gas_init"
Cohesion: 0.18
Nodes (24): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step() (+16 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.11
Nodes (52): Core (C), rb_engine_desc_default(), rb_engine_settle(), Results, Tests that changed, The tests, Tests that can fail, Tests that changed (+44 more)

### Community 29 - "rb_events.c"
Cohesion: 0.17
Nodes (13): Each cylinder, Engine snapshot — version 2 (binary), Layout, The instruments, What stays for later, passed(), push(), rb_event_kind_name() (+5 more)

### Community 30 - "fixed_transport.c"
Cohesion: 0.26
Nodes (24): add(), advance(), book(), clock_limit(), faces(), fail(), heat(), known_valid() (+16 more)

### Community 31 - "rb_network_step"
Cohesion: 0.21
Nodes (14): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), blowdown_time_to_2bar(), setup_blowdown() (+6 more)

### Community 32 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.10
Nodes (20): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+12 more)

### Community 33 - "contracts/README.md"
Cohesion: 0.16
Nodes (9): Shared exhaust geometry with bidirectional physical loading, One complete SI build and generation-tagged observations, Cost, preservation and reproducibility, Design, research and consumer review, Implementation and proof boundaries, Results and failure controls, Scope and frozen evidence gates, Stage 037 — Shared build, geometry, event and snapshot contracts (+1 more)

### Community 34 - "Stage 13 — Performance & stability pass"
Cohesion: 0.10
Nodes (20): D08 — P1: randomized stability checks omit the new junction chamber, Tests that changed, Cost, Measuring, Notes for later stages, Sources, Stage 13 — Performance & stability pass, The chamber and the gas (+12 more)

### Community 35 - "cmd_info"
Cohesion: 0.23
Nodes (21): rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find() (+13 more)

### Community 36 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 37 - "rb_gas_init"
Cohesion: 0.17
Nodes (27): rb_fuel_heat(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u() (+19 more)

### Community 38 - "high_rpm_report.py"
Cohesion: 0.06
Nodes (36): 2026-11-03: Stage 119, the review's leftovers, "Lift time" is one instant, analyze(), long_coast(), main(), record(), render(), sha() (+28 more)

### Community 39 - "S2d design — coupled exhaust acoustics and learned-DSP benchmarks"
Cohesion: 0.12
Nodes (16): A1 — conservative 1D comparison, Architecture and ownership, B0 — fixed-background acoustic graph, B1 — gas coupling, mean flow and lift-off, Comparison framework and rejection gates, Components, radiation and usable band, Decision and experimental arms, Learned synthesis: experiments and deployment (+8 more)

### Community 41 - "test_pipe.c"
Cohesion: 0.19
Nodes (28): rb_chamber_init_reservoir(), rb_pipe_init(), rb_pipe_join(), rb_pipe_pack(), Tests that can fail, count(), extreme(), packed_ring() (+20 more)

### Community 42 - "audio_source_probe.c"
Cohesion: 0.35
Nodes (14): coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame(), path_name() (+6 more)

### Community 43 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 44 - "engine-node.ts"
Cohesion: 0.25
Nodes (6): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor

### Community 45 - "test_scene.c"
Cohesion: 0.32
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 46 - "wav.c"
Cohesion: 0.08
Nodes (47): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+39 more)

### Community 47 - "render_results.py"
Cohesion: 0.15
Nodes (8): expected_clips(), listening_wav(), replace_current(), write_page(), checkpoint(), float_wav(), Players, RenderResultsTests

### Community 48 - "REV-BENCH as a desktop application — the design"
Cohesion: 0.10
Nodes (20): 10. Testing, 11. The interface, 12. Files, settings and privacy, 13. Distribution, 14. What happens to the existing browser work, 15. Decisions, 1. What the app is, 2. The stack (+12 more)

### Community 50 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.21
Nodes (4): Legacy roadmap — archived 2026-10-23, Native acoustic candidates — S2d, Open-port coupling checkpoint, Reproduce

### Community 52 - "review-completed-stages-2026-10-23.md"
Cohesion: 0.17
Nodes (9): Brief review of completed Stages 001–036, Checks, evidence and hold, Coverage of every completed stage, Exact new reproduction results, Findings, Frozen checks and execution, Results, cost and regression evidence, Scope, model and public contract (+1 more)

### Community 54 - "Stage 13b — Performance feedback checkpoint"
Cohesion: 0.25
Nodes (8): 2026-10-22 continuation: coupled port experiment and researched exhausts, Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance and solver work, Validation and sound, What was built, Why the implicit probe was rejected

### Community 55 - "Bernardo's engine sound examples — 2026-10-21"
Cohesion: 0.17
Nodes (12): 2026-10-24 supplied-video update, Additional rear cross V8 recordings — 2026-10-23, Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and distance, Processing and reproduction, R1b: optional filtered listening copies — 2026-10-21, Reproduction and checkpoint (+4 more)

### Community 60 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.18
Nodes (12): rb_engine_desc_check(), rb_preset_name(), Idle tuning, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout (+4 more)

### Community 61 - "style"
Cohesion: 0.15
Nodes (12): channel_repeats(), chart(), compare(), fetch(), main(), read_list(), read_wav(), repeat() (+4 more)

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.04
Nodes (51): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+43 more)

### Community 64 - "string"
Cohesion: 0.14
Nodes (33): add(), clock_state(), displacement(), dot(), evaluate(), failure(), flux(), gradients() (+25 more)

### Community 66 - "test_noise.c"
Cohesion: 0.13
Nodes (41): rb_engine_init(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), one_pole(), passes() (+33 more)

### Community 67 - "main"
Cohesion: 0.24
Nodes (15): rb_ft_inventory(), rb_ft_load(), rb_ft_make(), rb_ft_prepare(), rb_ft_step(), rb_ft_wall(), balanced(), setup() (+7 more)

### Community 68 - "The desktop application"
Cohesion: 0.20
Nodes (10): Stage 066 — The playable engine and its audio lifecycle, Stage 067 — The engine, intake and exhaust editor, Stage 068 — Live instruments, telemetry and the core's dyno, Stage 069 — The first playable app, qualified on Windows, Stage 118 — A real-time tier for today's engine, Stage 121 — The core's public engine-control, telemetry and snapshot API, Stage 122 — Cross-platform native builds and continuous integration, Stage 123 — The real-time audio host (+2 more)

### Community 69 - "Canonical stage tracker"
Cohesion: 0.09
Nodes (23): Documentation map, Done, Exhaust-acoustics research (043–049, 114), How a stage is done, Optional, after the release, Phase A — the desktop app and live play, Phase B — the sound to real-engine standard, Phase C — the 3D engine (+15 more)

### Community 70 - "numpy"
Cohesion: 0.08
Nodes (28): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+20 more)

### Community 71 - "rb_engine_init_at"
Cohesion: 0.18
Nodes (36): rb_chamber_clear_rates(), cell_heat(), cell_is_fast(), end_flow(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux() (+28 more)

### Community 72 - "Existing sound, measurements and bounded prototypes"
Cohesion: 0.10
Nodes (20): Existing sound, measurements and bounded prototypes, Stage 017 — Float-audio analysis and Windows paths, Stage 018 — Finite burns stop at exhaust opening, Stage 019 — Five-preset repetition and combustion audit, Stage 020 — Trapped-mixture burn feedback, Stage 021 — Raw user-reference catalog, Stage 022 — Filtered reference listening library, Stage 023 — Loaded exhaust detail and lift-transition correction (+12 more)

### Community 73 - "Stage 036 — complete project roadmap and evidence protocol"
Cohesion: 0.40
Nodes (5): Deliverables, Review findings incorporated, Scope decisions, Stage 036 — complete project roadmap and evidence protocol, Validation and limits

### Community 74 - "pathlib"
Cohesion: 0.06
Nodes (25): run(), run(), main(), render(), find_ffmpeg(), runs(), check(), load_file() (+17 more)

### Community 76 - "rb_tw_step"
Cohesion: 0.32
Nodes (12): positive(), rb_tw_balance(), rb_tw_capacity(), rb_tw_prepare(), rb_tw_step(), rb_tw_temperature(), temperature(), test_thermal_capacity_and_closed_books() (+4 more)

### Community 79 - "rb_create"
Cohesion: 0.14
Nodes (34): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), config_is_valid(), model_step(), rb_config_default() (+26 more)

### Community 80 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 81 - "rb_cli.c"
Cohesion: 0.23
Nodes (23): rb_engine_displacement(), cli_quality_from(), cli_take_value(), cli_to_double(), cli_usage(), cmd_blowdown(), cmd_breathe(), cmd_dyno() (+15 more)

### Community 83 - "string"
Cohesion: 0.40
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.29
Nodes (7): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed

### Community 85 - "check_combustion_variation.py"
Cohesion: 0.21
Nodes (9): commands(), digest(), execute(), one(), main(), validate(), finite(), CombustionVariationTests (+1 more)

### Community 91 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 92 - "rb_engine_layout"
Cohesion: 0.39
Nodes (7): muffler_width(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), Stage 8 — Engine layouts & presets, refused(), test_layout_rules()

### Community 93 - "Historical foundations and engine core"
Cohesion: 0.12
Nodes (17): Historical foundations and engine core, Stage 001 — Initial research and native-first project setup, Stage 002 — Native scaffold, WAV/CSV output and C tests, Stage 003 — WASM and AudioWorklet tone scaffold, Stage 004 — Gas properties, chambers and orifice flow, Stage 005 — Slider-crank and motored cylinder, Stage 006 — Valvetrain, throttle and intake plenum, Stage 007 — Baseline combustion and wall heat (+9 more)

### Community 94 - "REV-BENCH"
Cohesion: 0.25
Nodes (8): Abbreviations in the research stages (043–049, 114), Delivered-fuel event, Fueling attempt, Language, Numerical observation (Stage 038), REV-BENCH, Thermal/material reference (Stages 047–048), Uniform-flow acoustic coupling (Stages 045–046)

### Community 96 - "build_contract.py"
Cohesion: 0.08
Nodes (32): audible_performance_time(), catalog(), counter(), dumps(), from_native(), identity(), keys(), loads() (+24 more)

### Community 98 - "plot_run.py"
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 99 - "string"
Cohesion: 0.08
Nodes (80): The research solvers' tests can fail, accumulate(), acoustic_energy(), add(), admissible(), apply_residual(), backward(), book() (+72 more)

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "hashlib"
Cohesion: 0.06
Nodes (34): analyze(), brief(), plot_summary(), read(), select(), self_test(), digest(), main() (+26 more)

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

### Community 109 - "rb_chamber_gamma"
Cohesion: 0.29
Nodes (11): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), move_gas(), Tests that changed (+3 more)

### Community 110 - "engine.c"
Cohesion: 0.11
Nodes (42): accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn() (+34 more)

### Community 111 - "setup"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 112 - "rb_engine_init_at"
Cohesion: 0.06
Nodes (56): pipe_role(), rb_diag_bind_engine(), rb_diag_init(), rb_diag_reason_name(), rb_engine_contents(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat() (+48 more)

### Community 113 - "diagnostics.c"
Cohesion: 0.44
Nodes (10): add(), rb_diag_impulse(), rb_diag_inventory(), rb_diag_note(), rb_diag_repair(), rb_diag_role_name(), rb_diag_transfer(), reasons_add() (+2 more)

### Community 114 - "Engine integration, architecture selection and base acceptance"
Cohesion: 0.17
Nodes (12): Engine integration, architecture selection and base acceptance, Stage 053 — Compressible and reversing turbulent-flow excitation, Stage 054 — Unified pulse and noise routing, Stage 055 — Engine-specific structure and accessories, Stage 056 — Listener, vehicle and recording-transfer validation, Stage 057 — Whole-engine candidate integration, Stage 058 — Robotic deceleration and lift-off causal diagnosis, Stage 059 — Five-engine physical and acoustic qualification (+4 more)

### Community 115 - "Propagation prototypes and physical components"
Cohesion: 0.17
Nodes (12): Propagation prototypes and physical components, Stage 043 — Improved conservative solver and production-mixture control, Stage 044 — Executable shared exhaust geometry compiler, Stage 045 — General zero-flow open gas-to-graph coupling, Stage 046 — Uniform mean-flow coupling, Stage 047 — Gas-property and wall-thermal model characterization, Stage 048 — Thermal fronts, coefficient updates and reverse entrainment, Stage 049 — Branches, finite junctions and all-port transfer (+4 more)

### Community 116 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.25
Nodes (8): Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it, What was built

### Community 117 - "material.c"
Cohesion: 0.21
Nodes (24): derive(), failure(), heat(), positive(), rb_mt_energy(), rb_mt_left_inventory(), rb_mt_load(), rb_mt_momentum() (+16 more)

### Community 118 - "matplotlib_pyplot"
Cohesion: 0.19
Nodes (14): plot_power(), plot_spark(), between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle() (+6 more)

### Community 119 - "Roadmap execution and evidence protocol"
Cohesion: 0.17
Nodes (12): Change control and evidence preservation, Dependency and milestone sketch, Integrated release scenario, Minimum execution record, Preserved sound and scenario obligations, Quantitative validation policy, Required validation commands for code stages, Roadmap execution and evidence protocol (+4 more)

### Community 120 - "REV-BENCH — notes for Claude"
Cohesion: 0.08
Nodes (22): Bernardo, Claude's cloud sessions, Environment, graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The app (since 2026-11-08), The graph in this repo (+14 more)

### Community 121 - "rb_cylinder_move"
Cohesion: 0.14
Nodes (24): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), Phase 1 — Physics core (native, offline, validated) (+16 more)

### Community 123 - "Stage 045 — Production-mixture gas to actual B0 graph"
Cohesion: 0.33
Nodes (6): Executed method and rejected controls, Gates frozen before implementation measurements, Numerical results and valid envelope, Preflight, research and ownership, Regression, failures, cost and handoff, Stage 045 — Production-mixture gas to actual B0 graph

### Community 124 - "Optional expansions"
Cohesion: 0.18
Nodes (11): Optional expansions, Stage 103 — Gearbox and vehicle-driving mode, Stage 104 — WAV recording and audio export, Stage 105 — Two-stroke engine cycle, Stage 106 — Alternative fuels, Stage 107 — Supercharger system, Stage 108 — Individual throttle bodies, Stage 109 — Anti-lag system (+3 more)

### Community 126 - "Complete 3D engine experience"
Cohesion: 0.22
Nodes (9): Complete 3D engine experience, Stage 070 — 3D scene, visual style and capability foundation, Stage 071 — Procedural bottom end and manual kinematics, Stage 072 — Procedural top end and valve motion, Stage 073 — Coherent snapshots and sound-synchronized live animation, Stage 074 — Diagnostic slow motion and four-stroke inspection, Stage 075 — Shared 3D intake and complete exhaust routing, Stage 076 — Gas, combustion, pulse and thermal visualization (+1 more)

### Community 127 - "The port, persistence, final acceptance and release"
Cohesion: 0.20
Nodes (10): Stage 095 — Versioned build files and the session, Stage 096 — Sharing builds, Stage 097 — Usability, accessibility and visual polish, Stage 098 — Final per-preset listening and realism acceptance, Stage 099 — Release qualification on the supported systems and hardware, Stage 100 — Release documentation, licences and asset review, Stage 101 — Signed installers and the update channel, Stage 102 — Public release and clean-machine acceptance (+2 more)

### Community 128 - "Roadmap, shared contracts and trustworthy excitation"
Cohesion: 0.25
Nodes (8): Roadmap, shared contracts and trustworthy excitation, Stage 036 — Unified roadmap and evidence protocol, Stage 037 — Shared build, geometry, event and snapshot contracts, Stage 038 — Complete numerical guard and flux observability, Stage 039 — Matched references and calibration protocol, Stage 040 — Spark, ignition and flame-development causality, Stage 041 — Continuous combustion variation and residual validity, Stage 042 — Actual fuel-injection events and acoustic triggers

### Community 129 - "Turbocharged engine development"
Cohesion: 0.25
Nodes (8): Stage 078 — Turbo compressor and turbine map components, Stage 079 — Turbo shaft and engine gas-system integration, Stage 080 — Boost, wastegate, intercooler and blow-off control, Stage 081 — Turbo acoustic sources and turbine propagation, Stage 082 — Turbo controls, instrumentation and build integration, Stage 083 — Turbo geometry, shaft motion and valve visualization, Stage 084 — Turbo engine, audio, visual and runtime qualification, Turbocharged engine development

### Community 130 - "plot_fire.py"
Cohesion: 0.18
Nodes (9): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), spark_text() (+1 more)

### Community 131 - "Knock, mechanical limits and damage"
Cohesion: 0.33
Nodes (6): Knock, mechanical limits and damage, Stage 085 — End-gas and knock physical model, Stage 086 — Knock-limited ECU, audible knock and feedback, Stage 087 — Piston speed, valve float and mechanical limits, Stage 088 — Optional damage state, warnings and reset behavior, Stage 089 — Knock and damage presentation effects

### Community 132 - "Expanded engine and exhaust catalog"
Cohesion: 0.33
Nodes (6): Expanded engine and exhaust catalog, Stage 090 — Additional inline and single-cylinder engines, Stage 091 — 60-degree and 90-degree V6 engines, Stage 092 — V10 and V12 engines, Stage 093 — Boxer-six engine, Stage 094 — Complete engine-specific exhaust and catalog calibration

### Community 133 - "Stage 4 — Single cylinder: slider-crank + motoring"
Cohesion: 0.25
Nodes (8): Crank angle convention, Equations used, Notes for later stages, Results, Sources, Stage 4 — Single cylinder: slider-crank + motoring, Try it, What was built

### Community 134 - "04-propagation.md"
Cohesion: 0.05
Nodes (37): Executed results and rejection, Preflight and frozen experiment, Stage 043 — Improved conservative solver and production-mixture control, Validation and review before 044, Valve/neighbor ownership and independent clocks, Frozen scope, model and gates, Implementation and bounded results, Regression, portability and fault detection (+29 more)

### Community 135 - "count"
Cohesion: 0.14
Nodes (10): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+2 more)

### Community 136 - "Stage 039 — two supplied dashboard references"
Cohesion: 0.29
Nodes (6): Decision, Inspection and processing, Remaining acquisition and qualification, Review and preservation, Stage 039 — two supplied dashboard references, What the footage establishes

### Community 137 - "Stage 113 — Acoustic derived-state and numerical failure guards"
Cohesion: 0.23
Nodes (11): rb_injection_emit(), rb_injection_kind_name(), rb_injection_trace_init(), observe(), pose_ivc(), start(), test_injection_clock_mass_and_sound(), test_injection_consumption_and_no_replay() (+3 more)

### Community 138 - "csv"
Cohesion: 0.18
Nodes (11): accuracy(), analytic_line(), magnitude_db(), main(), plots(), publish(), read(), summarize() (+3 more)

### Community 139 - "Conditional learned-synthesis research"
Cohesion: 0.67
Nodes (3): Conditional learned-synthesis research, Stage 064 — Standalone learned pulse and DSP-parameter benchmark, Stage 065 — Conditional learned unresolved-source integration

### Community 140 - "Stage039 — Matched references and calibration protocol"
Cohesion: 0.17
Nodes (12): 2026-10-24 user-supplied video intake, Extended manufacturer/public-source research — 2026-10-24, Frozen reference rules, Gates, Implementation and acquired evidence, Initial bounded protocol closeout — 2026-10-24, superseded, Initial outcome and review handoff — 2026-10-23, Measurements and audition evidence (+4 more)

### Community 141 - "style"
Cohesion: 0.24
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 142 - "user-sound-references.md"
Cohesion: 0.09
Nodes (22): Correct engine identity before collecting more sound, Final 039 gates and practical remaining request, Manufacturers and exhaust variants covered, Media and measurement acquisition outcomes, Outcome, Review, validation and handoff, Search method and claim boundaries, Stage 039 — expanded reference search and qualification review (+14 more)

### Community 143 - "rb_dyno_init"
Cohesion: 0.20
Nodes (13): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), best_spark(), run_held(), test_calibration_cams(), test_calibration_compression() (+5 more)

### Community 144 - "Stage039 — six supplied candidate videos reviewed"
Cohesion: 0.33
Nodes (5): Decision, Inspection and processing record, Remaining gap and handoff, Source findings, Stage 039 — six supplied candidate videos reviewed

### Community 145 - "numpy"
Cohesion: 0.09
Nodes (28): read_wav(), read_wav_bytes(), analyze(), digest(), execute(), one(), main(), as_db() (+20 more)

### Community 146 - "cmd_presets"
Cohesion: 0.31
Nodes (11): csv_close(), csv_open(), csv_row(), cmd_presets(), fire_sweep(), pipes_engine(), print_firing(), settle_at() (+3 more)

### Community 147 - "rb_audit.c"
Cohesion: 0.23
Nodes (7): add(), audit(), main(), number(), usage(), add(), output_stats()

### Community 148 - "test_diagnostic_orifice_thermal_and_radiation"
Cohesion: 0.33
Nodes (5): rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), test_diagnostic_orifice_thermal_and_radiation(), test_radiation_causal_impedance()

### Community 149 - "Structural findings that change the next step"
Cohesion: 0.14
Nodes (14): 1. Same-tick return pressure needs an API boundary, 2. Port signs, pressure reference and work ownership must be explicit, 3. The background gas cannot be removed with its acoustic grid, 4. Shared geometry is currently a snapshot, not an executable compiler, 5. Stable output can conceal modified gas transport, 6. Native stereo and the browser host are different validation targets, 7. The single results file is not reproducibly rebuildable today, 8. Provenance and reference conditions still limit attribution (+6 more)

### Community 151 - "Stage040 — Spark, ignition and flame-development causality"
Cohesion: 0.20
Nodes (10): Architecture and predeclared gates, Complete sound checkpoint comparison, Executed independent proof, Explicit limits and next handoff, Frozen physical question and research, Implemented causal state and ownership, Outcome — bounded completion and HOLD, Reproduction (+2 more)

### Community 152 - "Stage041 — continuous combustion variation and residual validity"
Cohesion: 0.13
Nodes (14): Complete listening checkpoint, Executed numerical evidence, Execution, Frozen evidence gates before candidate evaluation, Heat-release phasing, Late chemistry decision, Predecessor review and physical question, Processing cost and reproducibility (+6 more)

### Community 154 - "exact_riemann"
Cohesion: 0.36
Nodes (4): exact_riemann(), eq(), wave(), RiemannReferenceTests

### Community 155 - "diagnostics.h"
Cohesion: 0.23
Nodes (4): check_chamber(), fail(), rb_fuzz_inspect_engine(), test_diagnostic_junction_repair_and_fuzz()

### Community 156 - "os"
Cohesion: 0.11
Nodes (14): The test runner, chromium(), kill_tree(), main(), make(), node_env(), remove_tree(), run_command() (+6 more)

### Community 159 - ".test_failed_experiment_publish_keeps_its_diagnostics"
Cohesion: 0.22
Nodes (4): CheckpointSectionTests, failing_replace(), failing_write(), section()

### Community 160 - "test_injection_events.py"
Cohesion: 0.27
Nodes (6): commands(), one(), validate(), finite(), InjectionEventTests, valid()

### Community 161 - "02-existing-sound.md"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 162 - "Stage 042 — Actual fuel-injection events and acoustic triggers"
Cohesion: 0.18
Nodes (10): Complete audio checkpoint and preservation, Contract and ownership, Frozen gates before candidate evaluation, Implementation and predecessor review, Independent tests, faults and environment, Physical question and research, Processing cost and memory, Reproduction, final review and handoff (+2 more)

### Community 163 - "rb_engine_init_at"
Cohesion: 0.27
Nodes (8): new_pipe(), rb_engine_init_at(), face_init(), rb_pipe_attach(), rb_pipe_cells_for(), rb_pipe_end_cell(), rb_pipe_onward(), test_engine_pipe_rate()

### Community 167 - "Stage 5 — Valvetrain, throttle & intake plenum"
Cohesion: 0.22
Nodes (9): Equations used, How one step works, Notes for later stages, Results, Sources, Stage 5 — Valvetrain, throttle & intake plenum, Try it, Two things that surprised me (+1 more)

### Community 168 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 169 - "stddef"
Cohesion: 0.27
Nodes (4): main(), mixture_ns(), now(), nozzle_ns()

### Community 170 - "test_combustion.c"
Cohesion: 0.22
Nodes (9): rb_burnable_fuel(), rb_wall_area(), rb_woschni_h(), rb_woschni_w(), test_burnable_fuel(), test_fuel_heat(), test_motored_loss_angle(), test_wiebe_burn() (+1 more)

### Community 172 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.15
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 173 - "The workspace"
Cohesion: 0.33
Nodes (6): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (done, 2026-10-20), Running every test, on Windows or in WSL (2026-11-06), The workspace

### Community 176 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.10
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 179 - "Stage 049 — finite junctions and all-port transfer"
Cohesion: 0.25
Nodes (8): 28 October continuation: rejected open-port extrapolation, Definitions needed by the next stage, Final review and bounded closeout — 28 October 2026, Frozen v3 results and validation, Independent references and gates, before candidate measurements, Near-unit reconstruction review — candidate v3, Predecessor review and frozen design, Stage 049 — finite junctions and all-port transfer

### Community 180 - "fake_program.py"
Cohesion: 0.22
Nodes (4): available(), compiler(), make(), CheckpointRenderTests

### Community 181 - "Stage 119 — The review's leftovers"
Cohesion: 0.20
Nodes (7): Checks that `python -O` can't drop, Numbers in published prose come from the evidence, Readable C, and proof that nothing changed, Stage 119 — The review's leftovers, Suites, The reference tools measure in stereo, Windows

### Community 183 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.10
Nodes (20): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), After stage 12b: a piano note (stage 12c) (+12 more)

### Community 184 - "Stage 120 — Workspace and toolset upkeep"
Cohesion: 0.22
Nodes (8): Clutter, Hooks, docs and line endings, Make, Not done here, for Bernardo, Planted bugs, Stage 120 — Workspace and toolset upkeep, Tests, PlantBugsCheckTests

### Community 185 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 186 - "Bounded completed-stage review repairs"
Cohesion: 0.25
Nodes (8): Bounded completed-stage review repairs, Stage 112 — Recoverable checkpoint section publication, Stage 113 — Acoustic derived-state and numerical failure guards, Stage 115 — Valve jet noise and the cycles' variation, Stage 116 — Tests that can fail again, Stage 117 — Docs you can read, and up to date, Stage 119 — The review's leftovers, Stage 120 — Workspace and toolset upkeep

### Community 187 - "S2d — first native acoustic-renderer candidates"
Cohesion: 0.29
Nodes (7): Implemented modules and contracts, Matrix and numerical results, Outcome, Production cost and profiling, Reproduction and research, S2d — first native acoustic-renderer candidates, Validation, publication and remaining work

### Community 188 - "Stage 038 — Numerical guard and flux observability"
Cohesion: 0.25
Nodes (8): Cost and memory, Gates and reproduction, Lifetime and bounded cost contract, Measured numerical results, Model, ownership and limits, Predecessor review, Qualification and handoff, Stage 038 — Numerical guard and flux observability

### Community 189 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 190 - "test_presets.c"
Cohesion: 0.62
Nodes (5): all_gaps(), count_sparks(), firing_gaps(), gaps_are(), test_presets_fire_where_the_layout_says()

### Community 192 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 194 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

### Community 196 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 198 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 199 - "S2 — Additional flow sources and optional spatial resolution"
Cohesion: 0.40
Nodes (4): Optional ultra quality, S2 — Additional flow sources and optional spatial resolution, Sources, Validation

### Community 200 - "Selective sound rollback — 2026-10-22"
Cohesion: 0.50
Nodes (4): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22

### Community 201 - "S1e — Research and controlled overrun transitions"
Cohesion: 0.50
Nodes (4): Model, Real references, S1e — Research and controlled overrun transitions, Validation and remaining work

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1213 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **44 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `RbExports`, `rb_dyno_init`, `rb_drive.c`, `combustion.c`, `rb_gas_init`, `rb_events.c`, `rb_network_step`, `Stage 13 — Performance & stability pass`, `rb_engine_init_at`, `cmd_info`, `rb_gas_init`, `test_combustion.c`, `Stage 8 — Engine layouts & presets`, `test_noise.c`, `rb_create`, `string`, `rb_engine_layout`, `rb_chamber_gamma`, `engine.c`, `rb_engine_init_at`, `REV-BENCH — notes for Claude`, `rb_cylinder_move`?**
  _High betweenness centrality (0.110) - this node is a cross-community bridge._
- **Are the 102 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 102 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_variation.c` be split into smaller, more focused modules?**
  _Cohesion score 0.10821256038647344 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `engine-node.ts`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `RbSim`, `REV-BENCH — notes for Claude`, `test_engine.c`?**
  _High betweenness centrality (0.081) - this node is a cross-community bridge._
- **Should `Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧` be split into smaller, more focused modules?**
  _Cohesion score 0.10526315789473684 - nodes in this community are weakly interconnected._
- **Why does `rb_engine_step()` connect `engine.c` to `test_variation.c`, `Stage 113 — Acoustic derived-state and numerical failure guards`, `rb_engine_api.c`, `rb_dyno_init`, `test_audio.c`, `cmd_presets`, `rb_audit.c`, `combustion.c`, `test_engine.c`, `rb_network_step`, `Stage 13 — Performance & stability pass`, `rb_gas_init`, `test_overrun.c`, `test_combustion.c`, `throttle.c`, `audio_source_probe.c`, `wav.c`, `REV-BENCH as a desktop application — the design`, `test_presets.c`, `dyno_average`, `test_noise.c`, `rb_engine_init_at`, `rb_create`, `rb_cli.c`, `string`, `rb_chamber_gamma`, `rb_engine_init_at`, `rb_cylinder_move`?**
  _High betweenness centrality (0.048) - this node is a cross-community bridge._