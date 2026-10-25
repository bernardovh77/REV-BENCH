# Graph Report - rev-bench  (2026-10-25)

## Corpus Check
- 346 files · ~730,863 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 3220 nodes · 9091 edges · 176 communities (146 shown, 30 thin omitted)
- Extraction: 77% EXTRACTED · 23% INFERRED · 0% AMBIGUOUS · INFERRED: 2072 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `1f406107`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_variation.c
- PLAN.md
- Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧
- rb_engine_step
- SimpleWave
- test_acoustic_candidates.c
- rb-core.ts
- Stages
- main.ts
- presets.c
- package.json
- rb_engine_api.c
- RbExports
- audit_s1.py
- RbSim
- style
- json
- audio.c
- rb_preset_desc
- Model limitations and validation still requiring attention
- coupled_duct.c
- audio_source_probe.c
- compilerOptions
- test_scene.c
- examples
- read_wav
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- combustion.c
- rb_pipe_init
- rb_network_step
- Stage 9 — Dyno + validation against real engines
- Brief review of completed Stages 001–036
- Stage 13 — Performance & stability pass
- render_results.py
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- cmd_info
- REV-BENCH — Research notes (stage 0)
- S2d design — coupled exhaust acoustics and learned-DSP benchmarks
- test_pipe.c
- audio_source_probe.c
- throttle.c
- .process
- numpy
- Test-Driven Development
- render_results.py
- Stage 13b-p — researched exhaust configurations
- Should REV-BENCH change its sound-generation architecture?
- RevBenchNode
- Process
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Current sound results
- read_csv
- REV-BENCH - Codex handoff
- build_contract_probe.c
- test_noise.c
- plot_run.py
- test_audio.c
- Canonical stage tracker
- audio_experiment_report.py
- rb_engine_init_at
- Existing sound, measurements and bounded prototypes
- Stage 036 — complete project roadmap and evidence protocol
- rb_destroy
- AGENTS.md
- Stage 13b-r — high-RPM exhaust character
- engine.h
- run_held
- rb_create
- Stage 3 — Gas & flow primitives
- Stage 038 — Numerical guard and flux observability
- string
- REV-BENCH — Feedback checkpoint before stage 13b
- contracts/README.md
- acoustic_transport_probe.c
- rb_engine_desc_check
- Historical foundations and engine core
- REV-BENCH
- exhaust_geometry.py
- combustion_repetition.py
- string
- Initial audio bottleneck diagnosis — 2026-10-22
- high_rpm_report.py
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- rb_chamber_gamma
- test_injection_trace_capacity_and_transparency
- setup
- check_combustion_variation.py
- diagnostics.c
- Engine integration, architecture selection and base acceptance
- Propagation prototypes and physical components
- prepare_video_references.py
- rb_gas_init
- matplotlib_pyplot
- Roadmap execution and evidence protocol
- REV-BENCH — notes for Claude
- Stage S1c — Reproducible sound and combustion baselines
- presets.c
- build_contract.py
- Optional expansions
- Complete 3D engine experience
- Persistence, final acceptance and release
- Roadmap, shared contracts and trustworthy excitation
- Turbocharged engine development
- plot_fire.py
- Knock, mechanical limits and damage
- Expanded engine and exhaust catalog
- Stage S1a — Reliable sound analysis before further tuning
- check_build_contract.py
- Selective sound rollback — 2026-10-22
- Playable browser bench
- check_injection_events.py
- analyze_acoustic_candidates.py
- Conditional learned-synthesis research
- Stage039 — Matched references and calibration protocol
- rb_cylinder_move
- Stage039 — expanded reference search and qualification review
- Structural findings that change the next step
- Stage039 — six supplied candidate videos reviewed
- test_cylinder.c
- style
- contracts/README.md
- compare_real.py
- test_overrun.c
- Stage040 — Spark, ignition and flame-development causality
- Stage041 — continuous combustion variation and residual validity
- test_diagnostic_orifice_thermal_and_radiation
- exact_riemann
- rb_cylinder_init
- Stage 5 — Valvetrain, throttle & intake plenum
- Phase 2b — Sound realism (top priority) 🎧
- test_motored_loss_angle
- 02-existing-sound.md
- Stage 042 — Actual fuel-injection events and acoustic triggers
- Phase 2 — Sound (the realistic part)
- Stage039 closeout review and Stage041 readiness
- S2b — Loaded exhaust detail and the lift-off contrast
- Phase 6 — Polish & ship
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- GR86 and E46 video references — 2026-10-24
- The workspace
- Stage 113 — Acoustic derived-state and numerical failure guards

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 123 edges
2. `rb_engine_init_at()` - 59 edges
3. `rb_engine_init()` - 58 edges
4. `RbExports` - 53 edges
5. `rb_preset_desc()` - 48 edges
6. `read_wav()` - 44 edges
7. `rb_chamber_init()` - 41 edges
8. `Core (C)` - 41 edges
9. `REV-BENCH - Codex handoff` - 41 edges
10. `style()` - 38 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/roadmap/legacy-plan.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-05.md → core/src/cylinder.c
- `Unequal headers (`core/src/presets.c`)` --references--> `rb_engine_desc_default()`  [INFERRED]
  docs/stage-12c.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (176 total, 30 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.09
Nodes (58): rb_engine_combustion_model(), rb_engine_cylinder_angle(), rb_engine_init(), rb_engine_step(), rb_preset_count(), rb_preset_desc(), rb_preset_find(), rb_preset_key() (+50 more)

### Community 1 - "PLAN.md"
Cohesion: 0.21
Nodes (4): One canonical roadmap with evidence-gated promotion, Audit finding ownership and closure evidence, Legacy stage crosswalk, Required use

### Community 2 - "Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.08
Nodes (49): rb_wiebe_quantile(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark() (+41 more)

### Community 4 - "SimpleWave"
Cohesion: 0.13
Nodes (13): balance(), experiment(), evaluate(), run(), experiment(), evaluate(), evaluate(), cv_at() (+5 more)

### Community 5 - "test_acoustic_candidates.c"
Cohesion: 0.07
Nodes (78): Experiment and acceptance, Implemented scope, Measured outcome, Method and physical ownership, Reproduce, S2d-R, S2d-0b and S2d-B1a — first open coupling checkpoint, Geometry and approximations, Pass-by and room decay (+70 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.06
Nodes (46): wasi, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "Stages"
Cohesion: 0.10
Nodes (20): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism, Progress tracker, REV-BENCH — Plan of Action (+12 more)

### Community 8 - "main.ts"
Cohesion: 0.13
Nodes (24): CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam(), findTelemetry() (+16 more)

### Community 9 - "presets.c"
Cohesion: 0.07
Nodes (27): A muffler is a pipe, Calibration, Cost, Equal and unequal headers, Every source at its own distance, Going straight on, Packing, Review of stage 11 (+19 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "rb_engine_api.c"
Cohesion: 0.11
Nodes (41): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+33 more)

### Community 12 - "RbExports"
Cohesion: 0.07
Nodes (3): What was built, RbCore, RbExports

### Community 13 - "audit_s1.py"
Cohesion: 0.21
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 14 - "RbSim"
Cohesion: 0.06
Nodes (11): Idle tuning, Notes for later stages, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+3 more)

### Community 15 - "style"
Cohesion: 0.19
Nodes (15): a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main(), orders() (+7 more)

### Community 16 - "json"
Cohesion: 0.08
Nodes (36): analyze(), brief(), plot_summary(), read(), select(), self_test(), digest(), main() (+28 more)

### Community 17 - "audio.c"
Cohesion: 0.23
Nodes (20): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_listener_distance(), rb_listener_hear(), rb_listener_place() (+12 more)

### Community 18 - "rb_preset_desc"
Cohesion: 0.07
Nodes (45): pipe_role(), rb_diag_bind_engine(), rb_diag_init(), rb_diag_reason_name(), rb_engine_contents(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat() (+37 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (33): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+25 more)

### Community 20 - "coupled_duct.c"
Cohesion: 0.18
Nodes (29): add(), admissible(), book(), chars(), end_state(), linear_flux(), linear_rhs(), linear_tick() (+21 more)

### Community 21 - "audio_source_probe.c"
Cohesion: 0.06
Nodes (83): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+75 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "test_scene.c"
Cohesion: 0.18
Nodes (17): Consumer review before implementation, Engine build, Geometry, connections and frames, Shared contracts — version 1, Versioning, identity and limits, geometry(), paths(), rb_scene_flight_time() (+9 more)

### Community 24 - "examples"
Cohesion: 0.18
Nodes (4): ReferenceSession, examples(), SharedContracts, snapshot()

### Community 25 - "read_wav"
Cohesion: 0.12
Nodes (12): read_wav(), a_weighting(), measure(), thirds(), deceleration_report(), main(), record(), sha() (+4 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.13
Nodes (48): Core (C), rb_engine_desc_default(), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth() (+40 more)

### Community 29 - "combustion.c"
Cohesion: 0.13
Nodes (20): clamp(), flame_speed(), normal_cdf(), rb_burn_mean_duration(), rb_burnable_fuel(), rb_charge_burn_evaluate(), rb_fuel_heat(), rb_kernel_positive_mean() (+12 more)

### Community 30 - "rb_pipe_init"
Cohesion: 0.26
Nodes (17): rb_chamber_clear_rates(), face_init(), rb_pipe_clear(), rb_pipe_flows(), rb_pipe_init(), rb_pipe_join(), pipes_pulse(), pulse_extreme() (+9 more)

### Community 31 - "rb_network_step"
Cohesion: 0.20
Nodes (15): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), blowdown_time_to_2bar(), setup_blowdown() (+7 more)

### Community 32 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.10
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 33 - "Brief review of completed Stages 001–036"
Cohesion: 0.15
Nodes (10): Brief review of completed Stages 001–036, Checks, evidence and hold, Coverage of every completed stage, Exact new reproduction results, Findings, Bounded completed-stage review repairs, Stage 112 — Recoverable checkpoint section publication, Stage 113 — Acoustic derived-state and numerical failure guards (+2 more)

### Community 34 - "Stage 13 — Performance & stability pass"
Cohesion: 0.12
Nodes (16): Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls (+8 more)

### Community 35 - "render_results.py"
Cohesion: 0.15
Nodes (9): coast_markers(), expected_clips(), listening_wav(), replace_current(), write_page(), checkpoint(), float_wav(), Players (+1 more)

### Community 36 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 37 - "cmd_info"
Cohesion: 0.32
Nodes (16): rb_preset_name(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags(), rb_param_max() (+8 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.10
Nodes (20): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), After stage 12b: a piano note (stage 12c) (+12 more)

### Community 39 - "S2d design — coupled exhaust acoustics and learned-DSP benchmarks"
Cohesion: 0.12
Nodes (16): A1 — conservative 1D comparison, Architecture and ownership, B0 — fixed-background acoustic graph, B1 — gas coupling, mean flow and lift-off, Comparison framework and rejection gates, Components, radiation and usable band, Decision and experimental arms, Learned synthesis: experiments and deployment (+8 more)

### Community 41 - "test_pipe.c"
Cohesion: 0.18
Nodes (25): rb_chamber_init_reservoir(), rb_pipe_contents(), rb_pipe_pack(), Tests that can fail, count(), extreme(), packed_ring(), packet_energy_ratio() (+17 more)

### Community 42 - "audio_source_probe.c"
Cohesion: 0.32
Nodes (14): coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame(), path_name() (+6 more)

### Community 43 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 44 - ".process"
Cohesion: 0.10
Nodes (15): Audio, device and visual clocks, Clocks and actual events, Coherent live snapshots, Event, clock, snapshot and rebuild contract — version 1, Transactional prepare, rebuild, reset and lifetime, Equations used, Results, Sources (+7 more)

### Community 45 - "numpy"
Cohesion: 0.10
Nodes (16): experiment(), evaluate(), reference(), evaluate(), run(), publish(), fragment(), fragment() (+8 more)

### Community 46 - "Test-Driven Development"
Cohesion: 0.15
Nodes (10): Designing for Mockability, When to Mock, Anti-patterns, Rules of the loop, Seams: where tests go, Test-Driven Development, What a good test is, Bad Tests (+2 more)

### Community 47 - "render_results.py"
Cohesion: 0.09
Nodes (25): Scope, contract and method, main(), adopt_current(), carry_diagnostics(), checked(), compose(), diagnostic_names(), fingerprint() (+17 more)

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 50 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

### Community 51 - "RevBenchNode"
Cohesion: 0.19
Nodes (7): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, Running

### Community 52 - "Process"
Cohesion: 0.15
Nodes (12): 1. Gather context, 2. Explore the codebase (optional), 3. Draft vertical slices, 4. Quiz the user, 5. Publish the tickets to the configured tracker, Acceptance criteria, Blocked by, <NN>: <Ticket title> (+4 more)

### Community 54 - "Stage 13b — Performance feedback checkpoint"
Cohesion: 0.25
Nodes (8): 2026-10-22 continuation: coupled port experiment and researched exhausts, Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance and solver work, Validation and sound, What was built, Why the implicit probe was rejected

### Community 55 - "Bernardo's engine sound examples — 2026-10-21"
Cohesion: 0.17
Nodes (12): 2026-10-24 supplied-video update, Additional rear cross V8 recordings — 2026-10-23, Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and distance, Processing and reproduction, R1b: optional filtered listening copies — 2026-10-21, Reproduction and checkpoint (+4 more)

### Community 60 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 61 - "read_csv"
Cohesion: 0.15
Nodes (10): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+2 more)

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.05
Nodes (41): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+33 more)

### Community 64 - "build_contract_probe.c"
Cohesion: 0.60
Nodes (3): count(), import(), value()

### Community 66 - "test_noise.c"
Cohesion: 0.07
Nodes (55): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+47 more)

### Community 67 - "plot_run.py"
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 68 - "test_audio.c"
Cohesion: 0.19
Nodes (20): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), rb_soft_limit(), Tests that can fail, amplitude() (+12 more)

### Community 69 - "Canonical stage tracker"
Cohesion: 0.09
Nodes (22): 10. Knock, mechanical limits and damage, 11. Expanded engine and exhaust catalog, 12. Persistence, final acceptance and release, 13. Optional expansions, 14. Bounded review repairs (execute before 037), 1. Historical foundations and engine core, 2. Existing sound, measurements and bounded prototypes, 3. Roadmap, shared contracts and trustworthy excitation (+14 more)

### Community 70 - "audio_experiment_report.py"
Cohesion: 0.18
Nodes (15): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+7 more)

### Community 71 - "rb_engine_init_at"
Cohesion: 0.20
Nodes (27): cell_heat(), cell_is_fast(), end_flow(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux(), push() (+19 more)

### Community 72 - "Existing sound, measurements and bounded prototypes"
Cohesion: 0.10
Nodes (20): Existing sound, measurements and bounded prototypes, Stage 017 — Float-audio analysis and Windows paths, Stage 018 — Finite burns stop at exhaust opening, Stage 019 — Five-preset repetition and combustion audit, Stage 020 — Trapped-mixture burn feedback, Stage 021 — Raw user-reference catalog, Stage 022 — Filtered reference listening library, Stage 023 — Loaded exhaust detail and lift-transition correction (+12 more)

### Community 73 - "Stage 036 — complete project roadmap and evidence protocol"
Cohesion: 0.40
Nodes (5): Deliverables, Review findings incorporated, Scope decisions, Stage 036 — complete project roadmap and evidence protocol, Validation and limits

### Community 74 - "rb_destroy"
Cohesion: 0.17
Nodes (25): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_destroy(), rb_free_floats(), rb_set_param(), rb_get_telemetry() (+17 more)

### Community 76 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 77 - "engine.h"
Cohesion: 0.08
Nodes (16): rb_exhaust_model(), rb_version(), add(), audit(), main(), number(), usage(), engine_cost() (+8 more)

### Community 78 - "run_held"
Cohesion: 0.22
Nodes (14): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), best_spark(), run_held(), test_calibration_burn_and_spark(), test_calibration_cams() (+6 more)

### Community 79 - "rb_create"
Cohesion: 0.14
Nodes (29): rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step(), config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init() (+21 more)

### Community 80 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 81 - "Stage 038 — Numerical guard and flux observability"
Cohesion: 0.25
Nodes (8): Cost and memory, Gates and reproduction, Lifetime and bounded cost contract, Measured numerical results, Model, ownership and limits, Predecessor review, Qualification and handoff, Stage 038 — Numerical guard and flux observability

### Community 83 - "string"
Cohesion: 0.40
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.11
Nodes (15): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+7 more)

### Community 85 - "contracts/README.md"
Cohesion: 0.06
Nodes (32): Executed results and rejection, Preflight and frozen experiment, Stage 043 — Improved conservative solver and production-mixture control, Validation and review before 044, Valve/neighbor ownership and independent clocks, Frozen scope, model and gates, Implementation and bounded results, Regression, portability and fault detection (+24 more)

### Community 91 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 92 - "rb_engine_desc_check"
Cohesion: 0.53
Nodes (7): all_gaps(), count_sparks(), firing_gaps(), gaps_are(), refused(), test_layout_rules(), test_presets_fire_where_the_layout_says()

### Community 93 - "Historical foundations and engine core"
Cohesion: 0.12
Nodes (17): Historical foundations and engine core, Stage 001 — Initial research and native-first project setup, Stage 002 — Native scaffold, WAV/CSV output and C tests, Stage 003 — WASM and AudioWorklet tone scaffold, Stage 004 — Gas properties, chambers and orifice flow, Stage 005 — Slider-crank and motored cylinder, Stage 006 — Valvetrain, throttle and intake plenum, Stage 007 — Baseline combustion and wall heat (+9 more)

### Community 94 - "REV-BENCH"
Cohesion: 0.29
Nodes (7): Delivered-fuel event, Fueling attempt, Language, Numerical observation (Stage038), REV-BENCH, Thermal/material reference (Stages047–048), Uniform-flow acoustic coupling (Stages045–046)

### Community 96 - "exhaust_geometry.py"
Cohesion: 0.16
Nodes (11): loads(), unique(), native(), check(), faults(), compile_exhaust(), routes(), execute() (+3 more)

### Community 98 - "combustion_repetition.py"
Cohesion: 0.18
Nodes (10): read_wav_bytes(), analyze(), digest(), execute(), one(), main(), as_db(), band() (+2 more)

### Community 99 - "string"
Cohesion: 0.08
Nodes (77): accumulate(), acoustic_energy(), add(), admissible(), apply_residual(), backward(), book(), coordinates() (+69 more)

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "high_rpm_report.py"
Cohesion: 0.12
Nodes (11): write_float(), main(), report(), window_metrics(), excerpt(), main(), spectral_metrics(), telemetry() (+3 more)

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
Cohesion: 0.32
Nodes (10): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), move_gas(), Tests that changed (+2 more)

### Community 110 - "test_injection_trace_capacity_and_transparency"
Cohesion: 0.26
Nodes (12): rb_engine_seed(), rb_injection_emit(), rb_injection_kind_name(), rb_injection_trace_init(), observe(), pose_ivc(), start(), test_injection_clock_mass_and_sound() (+4 more)

### Community 111 - "setup"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 112 - "check_combustion_variation.py"
Cohesion: 0.29
Nodes (5): one(), validate(), finite(), CombustionVariationTests, valid_row()

### Community 113 - "diagnostics.c"
Cohesion: 0.44
Nodes (10): add(), rb_diag_impulse(), rb_diag_inventory(), rb_diag_note(), rb_diag_repair(), rb_diag_role_name(), rb_diag_transfer(), reasons_add() (+2 more)

### Community 114 - "Engine integration, architecture selection and base acceptance"
Cohesion: 0.17
Nodes (12): Engine integration, architecture selection and base acceptance, Stage 053 — Compressible and reversing turbulent-flow excitation, Stage 054 — Unified pulse and noise routing, Stage 055 — Engine-specific structure and accessories, Stage 056 — Listener, vehicle and recording-transfer validation, Stage 057 — Whole-engine candidate integration, Stage 058 — Robotic deceleration and lift-off causal diagnosis, Stage 059 — Five-engine physical and acoustic qualification (+4 more)

### Community 115 - "Propagation prototypes and physical components"
Cohesion: 0.17
Nodes (12): Propagation prototypes and physical components, Stage 043 — Improved conservative solver and production-mixture control, Stage 044 — Executable shared exhaust geometry compiler, Stage 045 — General zero-flow open gas-to-graph coupling, Stage 046 — Uniform mean-flow coupling, Stage 047 — Gas-property and wall-thermal model characterization, Stage 048 — Thermal fronts, coefficient updates and reverse entrainment, Stage 049 — Branches, finite junctions and all-port transfer (+4 more)

### Community 116 - "prepare_video_references.py"
Cohesion: 0.12
Nodes (19): compare(), db_ratio(), deltas(), main(), measure(), auditions(), checked_lock(), main() (+11 more)

### Community 117 - "rb_gas_init"
Cohesion: 0.06
Nodes (77): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_cv_from_u_diag() (+69 more)

### Community 118 - "matplotlib_pyplot"
Cohesion: 0.19
Nodes (14): comparison(), plot_spark(), between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle() (+6 more)

### Community 119 - "Roadmap execution and evidence protocol"
Cohesion: 0.17
Nodes (12): Change control and evidence preservation, Dependency and milestone sketch, Integrated release scenario, Minimum execution record, Preserved sound and scenario obligations, Quantitative validation policy, Required validation commands for code stages, Roadmap execution and evidence protocol (+4 more)

### Community 120 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 121 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 122 - "presets.c"
Cohesion: 0.46
Nodes (13): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+5 more)

### Community 123 - "build_contract.py"
Cohesion: 0.40
Nodes (11): audible_performance_time(), counter(), identity(), keys(), number(), require(), validate_build(), validate_event() (+3 more)

### Community 124 - "Optional expansions"
Cohesion: 0.20
Nodes (10): Optional expansions, Stage 103 — Gearbox and vehicle-driving mode, Stage 104 — WAV recording and audio export, Stage 105 — Two-stroke engine cycle, Stage 106 — Alternative fuels, Stage 107 — Supercharger system, Stage 108 — Individual throttle bodies, Stage 109 — Anti-lag system (+2 more)

### Community 126 - "Complete 3D engine experience"
Cohesion: 0.22
Nodes (9): Complete 3D engine experience, Stage 070 — 3D scene, visual style and capability foundation, Stage 071 — Procedural bottom end and manual kinematics, Stage 072 — Procedural top end and valve motion, Stage 073 — Coherent snapshots and sound-synchronized live animation, Stage 074 — Diagnostic slow motion and four-stroke inspection, Stage 075 — Shared 3D intake and complete exhaust routing, Stage 076 — Gas, combustion, pulse and thermal visualization (+1 more)

### Community 127 - "Persistence, final acceptance and release"
Cohesion: 0.22
Nodes (9): Persistence, final acceptance and release, Stage 095 — Versioned local build persistence, Stage 096 — Shareable builds and validated imports, Stage 097 — Usability, accessibility and visual polish, Stage 098 — Final per-preset listening and realism acceptance, Stage 099 — Complete-browser and hardware release qualification, Stage 100 — Release documentation, evidence and asset review, Stage 101 — Static production deployment (+1 more)

### Community 128 - "Roadmap, shared contracts and trustworthy excitation"
Cohesion: 0.25
Nodes (8): Roadmap, shared contracts and trustworthy excitation, Stage 036 — Unified roadmap and evidence protocol, Stage 037 — Shared build, geometry, event and snapshot contracts, Stage 038 — Complete numerical guard and flux observability, Stage 039 — Matched references and calibration protocol, Stage 040 — Spark, ignition and flame-development causality, Stage 041 — Continuous combustion variation and residual validity, Stage 042 — Actual fuel-injection events and acoustic triggers

### Community 129 - "Turbocharged engine development"
Cohesion: 0.25
Nodes (8): Stage 078 — Turbo compressor and turbine map components, Stage 079 — Turbo shaft and engine gas-system integration, Stage 080 — Boost, wastegate, intercooler and blow-off control, Stage 081 — Turbo acoustic sources and turbine propagation, Stage 082 — Turbo controls, instrumentation and build integration, Stage 083 — Turbo geometry, shaft motion and valve visualization, Stage 084 — Turbo engine, audio, visual and runtime qualification, Turbocharged engine development

### Community 130 - "plot_fire.py"
Cohesion: 0.16
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 131 - "Knock, mechanical limits and damage"
Cohesion: 0.33
Nodes (6): Knock, mechanical limits and damage, Stage 085 — End-gas and knock physical model, Stage 086 — Knock-limited ECU, audible knock and feedback, Stage 087 — Piston speed, valve float and mechanical limits, Stage 088 — Optional damage state, warnings and reset behavior, Stage 089 — Knock and damage presentation effects

### Community 132 - "Expanded engine and exhaust catalog"
Cohesion: 0.33
Nodes (6): Expanded engine and exhaust catalog, Stage 090 — Additional inline and single-cylinder engines, Stage 091 — 60-degree and 90-degree V6 engines, Stage 092 — V10 and V12 engines, Stage 093 — Boxer-six engine, Stage 094 — Complete engine-specific exhaust and catalog calibration

### Community 133 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 134 - "check_build_contract.py"
Cohesion: 0.32
Nodes (6): catalog(), dumps(), from_native(), check(), main(), prepare_build()

### Community 135 - "Selective sound rollback — 2026-10-22"
Cohesion: 0.50
Nodes (4): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22

### Community 136 - "Playable browser bench"
Cohesion: 0.40
Nodes (5): Playable browser bench, Stage 066 — Playable browser engine and audio lifecycle, Stage 067 — Engine, intake and exhaust control panel, Stage 068 — Live instruments, engine telemetry and native-owned dyno, Stage 069 — First playable browser qualification

### Community 137 - "check_injection_events.py"
Cohesion: 0.27
Nodes (6): commands(), one(), validate(), finite(), InjectionEventTests, valid()

### Community 138 - "analyze_acoustic_candidates.py"
Cohesion: 0.15
Nodes (13): accuracy(), analytic_line(), magnitude_db(), main(), plots(), publish(), read(), summarize() (+5 more)

### Community 139 - "Conditional learned-synthesis research"
Cohesion: 0.67
Nodes (3): Conditional learned-synthesis research, Stage 064 — Standalone learned pulse and DSP-parameter benchmark, Stage 065 — Conditional learned unresolved-source integration

### Community 140 - "Stage039 — Matched references and calibration protocol"
Cohesion: 0.17
Nodes (12): 2026-10-24 user-supplied video intake, Extended manufacturer/public-source research — 2026-10-24, Frozen reference rules, Gates, Implementation and acquired evidence, Initial bounded protocol closeout — 2026-10-24, superseded, Initial outcome and review handoff — 2026-10-23, Measurements and audition evidence (+4 more)

### Community 141 - "rb_cylinder_move"
Cohesion: 0.12
Nodes (17): rb_cylinder_move(), Phase 1 — Physics core (native, offline, validated), Stage 3 — Gas & flow primitives, Stage 4 — Single cylinder: slider-crank + motoring, Stage 5 — Valvetrain, throttle & intake plenum, Stage 6 — Combustion + heat transfer, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Stage 8 — Engine layouts & presets (+9 more)

### Community 142 - "Stage039 — expanded reference search and qualification review"
Cohesion: 0.22
Nodes (9): Correct engine identity before collecting more sound, Final039 gates and practical remaining request, Manufacturers and exhaust variants covered, Media and measurement acquisition outcomes, Outcome, Review, validation and handoff, Search method and claim boundaries, Stage039 — expanded reference search and qualification review (+1 more)

### Community 143 - "Structural findings that change the next step"
Cohesion: 0.14
Nodes (14): 1. Same-tick return pressure needs an API boundary, 2. Port signs, pressure reference and work ownership must be explicit, 3. The background gas cannot be removed with its acoustic grid, 4. Shared geometry is currently a snapshot, not an executable compiler, 5. Stable output can conceal modified gas transport, 6. Native stereo and the browser host are different validation targets, 7. The single results file is not reproducibly rebuildable today, 8. Provenance and reference conditions still limit attribution (+6 more)

### Community 144 - "Stage039 — six supplied candidate videos reviewed"
Cohesion: 0.33
Nodes (5): Decision, Inspection and processing record, Remaining gap and handoff, Source findings, Stage039 — six supplied candidate videos reviewed

### Community 145 - "test_cylinder.c"
Cohesion: 0.29
Nodes (8): compress(), engine_86x86(), isentrope_T(), test_big_volume_jump_is_split(), test_compression_converges_with_step_size(), test_compression_matches_isentrope(), test_cylinder_bad_input(), test_motoring_has_no_energy_drift()

### Community 146 - "style"
Cohesion: 0.27
Nodes (8): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save()

### Community 147 - "contracts/README.md"
Cohesion: 0.18
Nodes (9): One complete SI build and generation-tagged observations, Cost, preservation and reproducibility, Design, research and consumer review, Implementation and proof boundaries, Results and failure controls, Scope and frozen evidence gates, Stage038 predecessor review, Stage 037 — Shared build, geometry, event and snapshot contracts (+1 more)

### Community 148 - "compare_real.py"
Cohesion: 0.17
Nodes (11): chart(), compare(), fetch(), main(), read_list(), read_wav(), main(), orders() (+3 more)

### Community 149 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 151 - "Stage040 — Spark, ignition and flame-development causality"
Cohesion: 0.20
Nodes (10): Architecture and predeclared gates, Complete sound checkpoint comparison, Executed independent proof, Explicit limits and next handoff, Frozen physical question and research, Implemented causal state and ownership, Outcome — bounded completion and HOLD, Reproduction (+2 more)

### Community 152 - "Stage041 — continuous combustion variation and residual validity"
Cohesion: 0.14
Nodes (14): Complete listening checkpoint, Executed numerical evidence, Execution, Frozen evidence gates before candidate evaluation, Heat-release phasing, Late chemistry decision, Predecessor review and physical question, Processing cost and reproducibility (+6 more)

### Community 153 - "test_diagnostic_orifice_thermal_and_radiation"
Cohesion: 0.33
Nodes (5): rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), test_diagnostic_orifice_thermal_and_radiation(), test_radiation_causal_impedance()

### Community 154 - "exact_riemann"
Cohesion: 0.29
Nodes (4): exact_riemann(), eq(), wave(), RiemannReferenceTests

### Community 155 - "rb_cylinder_init"
Cohesion: 0.56
Nodes (7): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), test_cylinder_geometry()

### Community 156 - "Stage 5 — Valvetrain, throttle & intake plenum"
Cohesion: 0.22
Nodes (9): Equations used, How one step works, Notes for later stages, Results, Sources, Stage 5 — Valvetrain, throttle & intake plenum, Try it, Two things that surprised me (+1 more)

### Community 159 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.25
Nodes (8): Phase 2b — Sound realism (top priority) 🎧, Stage 13b-p — Model-specific exhaust research and implementation, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage 13b-r — High-RPM exhaust character and revised limits, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 160 - "test_motored_loss_angle"
Cohesion: 0.48
Nodes (5): rb_wall_area(), rb_woschni_h(), rb_woschni_w(), test_motored_loss_angle(), test_woschni()

### Community 161 - "02-existing-sound.md"
Cohesion: 0.19
Nodes (5): Shared exhaust geometry with bidirectional physical loading, Legacy roadmap — archived 2026-10-23, Native acoustic candidates — S2d, Open-port coupling checkpoint, Reproduce

### Community 162 - "Stage 042 — Actual fuel-injection events and acoustic triggers"
Cohesion: 0.20
Nodes (10): Complete audio checkpoint and preservation, Contract and ownership, Frozen gates before candidate evaluation, Implementation and predecessor review, Independent tests, faults and environment, Physical question and research, Processing cost and memory, Reproduction, final review and handoff (+2 more)

### Community 163 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 167 - "Stage039 closeout review and Stage041 readiness"
Cohesion: 0.29
Nodes (7): Checks and reproducibility, Current coverage and permitted comparisons, Decision and completion boundary, Exit-evidence review, Open claims and their existing owners, Review findings and Stage040 handoff, Stage039 closeout review and Stage041 readiness

### Community 168 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 169 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 170 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.33
Nodes (6): Equations used, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built

### Community 171 - "GR86 and E46 video references — 2026-10-24"
Cohesion: 0.33
Nodes (6): Audio screening and measurement authority, GR86 and E46 video references — 2026-10-24, Reproduction and results, Source identity and usefulness, Verified intake checkpoint, Visual review and selected source windows

### Community 172 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 173 - "Stage 113 — Acoustic derived-state and numerical failure guards"
Cohesion: 0.50
Nodes (4): Frozen checks and execution, Results, cost and regression evidence, Scope, model and public contract, Stage 113 — Acoustic derived-state and numerical failure guards

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1045 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **30 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `test_variation.c`, `test_noise.c`, `rb_engine_step`, `rb_engine_init_at`, `rb_engine_api.c`, `RbExports`, `rb_chamber_gamma`, `rb_cylinder_move`, `rb_create`, `run_held`, `audio.c`, `rb_preset_desc`, `string`, `rb_gas_init`, `REV-BENCH — notes for Claude`, `combustion.c`, `rb_network_step`?**
  _High betweenness centrality (0.073) - this node is a cross-community bridge._
- **Are the 101 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 101 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_variation.c` be split into smaller, more focused modules?**
  _Cohesion score 0.09471153846153846 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `test_engine.c`, `.process`, `rb-core.ts`, `RbSim`?**
  _High betweenness centrality (0.057) - this node is a cross-community bridge._
- **Should `Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧` be split into smaller, more focused modules?**
  _Cohesion score 0.1 - nodes in this community are weakly interconnected._
- **Why does `rb_engine_step()` connect `test_variation.c` to `rb_engine_step`, `rb_engine_api.c`, `rb_cylinder_move`, `rb_preset_desc`, `audio_source_probe.c`, `test_overrun.c`, `test_engine.c`, `combustion.c`, `rb_pipe_init`, `rb_network_step`, `test_motored_loss_angle`, `Stage 13 — Performance & stability pass`, `audio_source_probe.c`, `throttle.c`, `test_noise.c`, `test_audio.c`, `rb_engine_init_at`, `engine.h`, `run_held`, `rb_create`, `string`, `rb_engine_desc_check`, `rb_chamber_gamma`, `test_injection_trace_capacity_and_transparency`?**
  _High betweenness centrality (0.048) - this node is a cross-community bridge._