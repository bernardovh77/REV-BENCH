# Graph Report - rev-bench  (2026-10-28)

## Corpus Check
- 374 files · ~745,993 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 3434 nodes · 9875 edges · 176 communities (140 shown, 36 thin omitted)
- Extraction: 78% EXTRACTED · 22% INFERRED · 0% AMBIGUOUS · INFERRED: 2181 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `ef08fe30`
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
- rb_cli.c
- audio.c
- rb_preset_desc
- Model limitations and validation still requiring attention
- coupled_duct.c
- json
- compilerOptions
- prepare_user_references.py
- engine.h
- rb_gas_init
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- rb_cylinder_move
- fixed_transport.c
- rb_network_step
- Stage 9 — Dyno + validation against real engines
- contracts/README.md
- Stage 13 — Performance & stability pass
- cmd_info
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- REV-BENCH — Research notes (stage 0)
- S2d design — coupled exhaust acoustics and learned-DSP benchmarks
- test_pipe.c
- audio_source_probe.c
- throttle.c
- engine-node.ts
- test_scene.c
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
- rb_destroy
- style
- REV-BENCH - Codex handoff
- string
- test_noise.c
- acoustic_candidate_probe.c
- sys
- Canonical stage tracker
- audio_experiment_report.py
- rb_engine_init_at
- Existing sound, measurements and bounded prototypes
- Stage 036 — complete project roadmap and evidence protocol
- test_audio.c
- AGENTS.md
- engine.h
- main
- run_held
- rb_create
- Stage 3 — Gas & flow primitives
- coupled_duct_report.py
- string
- REV-BENCH — Feedback checkpoint before stage 13b
- contracts/README.md
- acoustic_transport_probe.c
- rb_engine_desc_check
- Historical foundations and engine core
- REV-BENCH
- build_contract.py
- noise.c
- string
- Initial audio bottleneck diagnosis — 2026-10-22
- high_rpm_report.py
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- chamber.h
- combustion.c
- setup
- check_combustion_variation.py
- diagnostics.c
- Engine integration, architecture selection and base acceptance
- Propagation prototypes and physical components
- subprocess
- material.c
- matplotlib_pyplot
- Roadmap execution and evidence protocol
- REV-BENCH — notes for Claude
- Stage S1c — Reproducible sound and combustion baselines
- rb_tw_step
- Stage 045 — Production-mixture gas to actual B0 graph
- Optional expansions
- Complete 3D engine experience
- Persistence, final acceptance and release
- Roadmap, shared contracts and trustworthy excitation
- Turbocharged engine development
- plot_fire.py
- Knock, mechanical limits and damage
- Expanded engine and exhaust catalog
- Stage S1a — Reliable sound analysis before further tuning
- stage-113.md
- count
- Playable browser bench
- Stage 113 — Acoustic derived-state and numerical failure guards
- analyze_acoustic_candidates.py
- Conditional learned-synthesis research
- Stage039 — Matched references and calibration protocol
- rb_cli.c
- Stage039 — expanded reference search and qualification review
- Stage 8 — Engine layouts & presets
- Stage039 — six supplied candidate videos reviewed
- prepare_video_references.py
- style
- Stage 6 — Combustion + heat transfer
- Selective sound rollback — 2026-10-22
- rb_test.h
- Stage040 — Spark, ignition and flame-development causality
- Stage041 — continuous combustion variation and residual validity
- stddef
- exact_riemann
- Stage 114 — fixed-geometry thermal transport bridge
- 02-existing-sound.md
- Stage 042 — Actual fuel-injection events and acoustic triggers
- Mesh
- S2d — first native acoustic-renderer candidates
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- Stage 13b-r — high-RPM exhaust character
- The workspace
- S2b — Loaded exhaust detail and the lift-off contrast
- bench_wasm.mjs
- Stage 049 — finite junctions and all-port transfer
- Current sound results

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 123 edges
2. `rb_engine_init_at()` - 59 edges
3. `rb_engine_init()` - 58 edges
4. `RbExports` - 53 edges
5. `rb_preset_desc()` - 48 edges
6. `read_wav()` - 44 edges
7. `REV-BENCH - Codex handoff` - 44 edges
8. `rb_chamber_init()` - 41 edges
9. `Core (C)` - 41 edges
10. `style()` - 38 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/roadmap/legacy-plan.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Unequal headers (`core/src/presets.c`)` --references--> `rb_engine_desc_default()`  [INFERRED]
  docs/stage-12c.md → core/src/engine.c
- `Stage 8 — Engine layouts & presets` --references--> `rb_engine_layout()`  [INFERRED]
  docs/roadmap/legacy-plan.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (176 total, 36 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.13
Nodes (33): rb_engine_init(), rb_get_cylinder(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+25 more)

### Community 1 - "PLAN.md"
Cohesion: 0.21
Nodes (4): One canonical roadmap with evidence-gated promotion, Audit finding ownership and closure evidence, Legacy stage crosswalk, Required use

### Community 2 - "Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧"
Cohesion: 0.11
Nodes (18): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that changed, The accessories (`accessory_power`, an engine field) (+10 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.12
Nodes (40): accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn() (+32 more)

### Community 4 - "SimpleWave"
Cohesion: 0.07
Nodes (26): evaluate(), qualify_enthalpy(), run(), wave(), balance(), experiment(), evaluate(), run() (+18 more)

### Community 5 - "test_acoustic_candidates.c"
Cohesion: 0.07
Nodes (77): 1. Same-tick return pressure needs an API boundary, 2. Port signs, pressure reference and work ownership must be explicit, 3. The background gas cannot be removed with its acoustic grid, 4. Shared geometry is currently a snapshot, not an executable compiler, 5. Stable output can conceal modified gas transport, 6. Native stereo and the browser host are different validation targets, 7. The single results file is not reproducibly rebuildable today, 8. Provenance and reference conditions still limit attribution (+69 more)

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
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "rb_engine_api.c"
Cohesion: 0.12
Nodes (36): describe_pipe(), field_ok(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags(), rb_engine_field_max(), rb_engine_field_min(), rb_engine_field_name() (+28 more)

### Community 13 - "audit_s1.py"
Cohesion: 0.21
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): comparison(), a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "rb_cli.c"
Cohesion: 0.14
Nodes (39): rb_engine_displacement(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe(), cmd_dyno() (+31 more)

### Community 17 - "audio.c"
Cohesion: 0.17
Nodes (25): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step() (+17 more)

### Community 18 - "rb_preset_desc"
Cohesion: 0.07
Nodes (66): rb_fuel_heat(), rb_diag_reason_name(), new_pipe(), rb_engine_contents(), rb_engine_cylinder_angle(), rb_engine_init_at(), rb_engine_pipe(), rb_engine_pipe_count() (+58 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (33): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+25 more)

### Community 20 - "coupled_duct.c"
Cohesion: 0.18
Nodes (29): add(), admissible(), book(), chars(), end_state(), linear_flux(), linear_rhs(), linear_tick() (+21 more)

### Community 21 - "json"
Cohesion: 0.07
Nodes (35): digest(), main(), execute(), main(), acoustic_case(), compare(), flow_case(), main() (+27 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "prepare_user_references.py"
Cohesion: 0.27
Nodes (8): encode_listening(), main(), prepare(), prepare_filtered(), publish_filtered(), run(), sha256(), ReferenceFilteringTests

### Community 24 - "engine.h"
Cohesion: 0.11
Nodes (12): add(), main(), number(), usage(), add(), number(), output_stats(), wrap() (+4 more)

### Community 25 - "rb_gas_init"
Cohesion: 0.19
Nodes (25): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_cv_from_u_diag() (+17 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (49): Core (C), rb_engine_desc_default(), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, test_audio_intake_mouth() (+41 more)

### Community 29 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (48): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+40 more)

### Community 30 - "fixed_transport.c"
Cohesion: 0.26
Nodes (24): add(), advance(), book(), clock_limit(), faces(), fail(), heat(), known_valid() (+16 more)

### Community 31 - "rb_network_step"
Cohesion: 0.19
Nodes (18): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), Tests that can fail, Tests that changed (+10 more)

### Community 32 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.10
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 33 - "contracts/README.md"
Cohesion: 0.18
Nodes (9): One complete SI build and generation-tagged observations, Cost, preservation and reproducibility, Design, research and consumer review, Implementation and proof boundaries, Results and failure controls, Scope and frozen evidence gates, Stage038 predecessor review, Stage 037 — Shared build, geometry, event and snapshot contracts (+1 more)

### Community 34 - "Stage 13 — Performance & stability pass"
Cohesion: 0.10
Nodes (20): D08 — P1: randomized stability checks omit the new junction chamber, Tests that changed, Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass (+12 more)

### Community 35 - "cmd_info"
Cohesion: 0.24
Nodes (18): rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find() (+10 more)

### Community 36 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 37 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.10
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.10
Nodes (20): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), After stage 12b: a piano note (stage 12c) (+12 more)

### Community 39 - "S2d design — coupled exhaust acoustics and learned-DSP benchmarks"
Cohesion: 0.12
Nodes (16): A1 — conservative 1D comparison, Architecture and ownership, B0 — fixed-background acoustic graph, B1 — gas coupling, mean flow and lift-off, Comparison framework and rejection gates, Components, radiation and usable band, Decision and experimental arms, Learned synthesis: experiments and deployment (+8 more)

### Community 41 - "test_pipe.c"
Cohesion: 0.16
Nodes (40): rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init(), rb_chamber_init_reservoir(), rb_chamber_sound_speed(), rb_pipe_clear(), rb_pipe_contents(), rb_pipe_init() (+32 more)

### Community 42 - "audio_source_probe.c"
Cohesion: 0.32
Nodes (14): coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame(), path_name() (+6 more)

### Community 43 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 44 - "engine-node.ts"
Cohesion: 0.25
Nodes (6): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor

### Community 45 - "test_scene.c"
Cohesion: 0.19
Nodes (17): Consumer review before implementation, Engine build, Geometry, connections and frames, Shared contracts — version 1, Versioning, identity and limits, geometry(), paths(), rb_scene_flight_time() (+9 more)

### Community 46 - "Test-Driven Development"
Cohesion: 0.15
Nodes (10): Designing for Mockability, When to Mock, Anti-patterns, Rules of the loop, Seams: where tests go, Test-Driven Development, What a good test is, Bad Tests (+2 more)

### Community 47 - "render_results.py"
Cohesion: 0.08
Nodes (26): Scope, contract and method, adopt_current(), carry_diagnostics(), checked(), compose(), diagnostic_names(), fingerprint(), load() (+18 more)

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 50 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

### Community 52 - "Process"
Cohesion: 0.15
Nodes (12): 1. Gather context, 2. Explore the codebase (optional), 3. Draft vertical slices, 4. Quiz the user, 5. Publish the tickets to the configured tracker, Acceptance criteria, Blocked by, <NN>: <Ticket title> (+4 more)

### Community 54 - "Stage 13b — Performance feedback checkpoint"
Cohesion: 0.25
Nodes (8): 2026-10-22 continuation: coupled port experiment and researched exhausts, Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance and solver work, Validation and sound, What was built, Why the implicit probe was rejected

### Community 55 - "Bernardo's engine sound examples — 2026-10-21"
Cohesion: 0.17
Nodes (12): 2026-10-24 supplied-video update, Additional rear cross V8 recordings — 2026-10-23, Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and distance, Processing and reproduction, R1b: optional filtered listening copies — 2026-10-21, Reproduction and checkpoint (+4 more)

### Community 60 - "rb_destroy"
Cohesion: 0.19
Nodes (24): rb_alloc_floats(), rb_create(), rb_create_with(), rb_destroy(), rb_free_floats(), rb_set_param(), rb_get_telemetry(), rb_render_block() (+16 more)

### Community 61 - "style"
Cohesion: 0.15
Nodes (9): main(), textbook_line(), first(), main(), plot(), spans(), window(), crank_axis() (+1 more)

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.04
Nodes (45): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+37 more)

### Community 64 - "string"
Cohesion: 0.14
Nodes (33): add(), clock_state(), displacement(), dot(), evaluate(), failure(), flux(), gradients() (+25 more)

### Community 66 - "test_noise.c"
Cohesion: 0.22
Nodes (24): rb_noise_init(), rb_noise_step(), Tests that can fail, cabin_expected(), db(), flow_rms(), impact_energy(), jet_expected() (+16 more)

### Community 67 - "acoustic_candidate_probe.c"
Cohesion: 0.17
Nodes (15): Geometry and approximations, Pass-by and room decay, S4 — Separate sources, stereo listeners and acoustic scenes, Signal path, Validation and use, chamber_name(), component_name(), fv() (+7 more)

### Community 68 - "sys"
Cohesion: 0.17
Nodes (9): load_table(), main(), run(), describe(), log_axis(), main(), plot_cycle(), plot_map() (+1 more)

### Community 69 - "Canonical stage tracker"
Cohesion: 0.09
Nodes (22): 10. Knock, mechanical limits and damage, 11. Expanded engine and exhaust catalog, 12. Persistence, final acceptance and release, 13. Optional expansions, 14. Bounded review repairs (execute before 037), 1. Historical foundations and engine core, 2. Existing sound, measurements and bounded prototypes, 3. Roadmap, shared contracts and trustworthy excitation (+14 more)

### Community 70 - "audio_experiment_report.py"
Cohesion: 0.18
Nodes (15): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+7 more)

### Community 71 - "rb_engine_init_at"
Cohesion: 0.17
Nodes (32): cell_heat(), cell_is_fast(), end_flow(), face_init(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux() (+24 more)

### Community 72 - "Existing sound, measurements and bounded prototypes"
Cohesion: 0.10
Nodes (20): Existing sound, measurements and bounded prototypes, Stage 017 — Float-audio analysis and Windows paths, Stage 018 — Finite burns stop at exhaust opening, Stage 019 — Five-preset repetition and combustion audit, Stage 020 — Trapped-mixture burn feedback, Stage 021 — Raw user-reference catalog, Stage 022 — Filtered reference listening library, Stage 023 — Loaded exhaust detail and lift-transition correction (+12 more)

### Community 73 - "Stage 036 — complete project roadmap and evidence protocol"
Cohesion: 0.40
Nodes (5): Deliverables, Review findings incorporated, Scope decisions, Stage 036 — complete project roadmap and evidence protocol, Validation and limits

### Community 74 - "test_audio.c"
Cohesion: 0.19
Nodes (20): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), rb_soft_limit(), render(), Tests that can fail (+12 more)

### Community 76 - "engine.h"
Cohesion: 0.18
Nodes (8): rb_exhaust_model(), observe(), pose_ivc(), test_injection_clock_mass_and_sound(), test_injection_consumption_and_no_replay(), test_injection_retained_disabled_and_boundaries(), wrap(), output_rms()

### Community 77 - "main"
Cohesion: 0.24
Nodes (15): rb_ft_inventory(), rb_ft_load(), rb_ft_make(), rb_ft_prepare(), rb_ft_step(), rb_ft_wall(), balanced(), setup() (+7 more)

### Community 78 - "run_held"
Cohesion: 0.23
Nodes (13): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), best_spark(), run_held(), test_calibration_burn_and_spark(), test_calibration_cams() (+5 more)

### Community 79 - "rb_create"
Cohesion: 0.16
Nodes (26): rb_create_with_quality(), config_is_valid(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+18 more)

### Community 80 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 81 - "coupled_duct_report.py"
Cohesion: 0.22
Nodes (8): publish(), assess(), exact_pressure(), initial(), read(), run(), CoupledProofTests, fixture()

### Community 83 - "string"
Cohesion: 0.38
Nodes (10): derive(), normalise(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix(), volume_step() (+2 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.11
Nodes (15): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+7 more)

### Community 85 - "contracts/README.md"
Cohesion: 0.06
Nodes (32): Decision, Inspection and processing, Remaining acquisition and qualification, Review and preservation, Stage 039 — two supplied dashboard references, What the footage establishes, Executed results and rejection, Preflight and frozen experiment (+24 more)

### Community 91 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 92 - "rb_engine_desc_check"
Cohesion: 0.24
Nodes (15): muffler_width(), rb_engine_desc_check(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), Results, test_engine_description(), test_exhaust_descriptor_guards() (+7 more)

### Community 93 - "Historical foundations and engine core"
Cohesion: 0.12
Nodes (17): Historical foundations and engine core, Stage 001 — Initial research and native-first project setup, Stage 002 — Native scaffold, WAV/CSV output and C tests, Stage 003 — WASM and AudioWorklet tone scaffold, Stage 004 — Gas properties, chambers and orifice flow, Stage 005 — Slider-crank and motored cylinder, Stage 006 — Valvetrain, throttle and intake plenum, Stage 007 — Baseline combustion and wall heat (+9 more)

### Community 94 - "REV-BENCH"
Cohesion: 0.29
Nodes (7): Delivered-fuel event, Fueling attempt, Language, Numerical observation (Stage038), REV-BENCH, Thermal/material reference (Stages047–048), Uniform-flow acoustic coupling (Stages045–046)

### Community 96 - "build_contract.py"
Cohesion: 0.06
Nodes (46): Cost and memory, Gates and reproduction, Lifetime and bounded cost contract, Measured numerical results, Model, ownership and limits, Predecessor review, Qualification and handoff, Stage 038 — Numerical guard and flux observability (+38 more)

### Community 98 - "noise.c"
Cohesion: 0.30
Nodes (10): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+2 more)

### Community 99 - "string"
Cohesion: 0.08
Nodes (77): accumulate(), acoustic_energy(), add(), admissible(), apply_residual(), backward(), book(), coordinates() (+69 more)

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "high_rpm_report.py"
Cohesion: 0.10
Nodes (23): analyze(), long_coast(), main(), record(), render(), sha(), write_float(), deceleration_report() (+15 more)

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

### Community 109 - "chamber.h"
Cohesion: 0.53
Nodes (4): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_pressure_now()

### Community 110 - "combustion.c"
Cohesion: 0.16
Nodes (15): clamp(), flame_speed(), normal_cdf(), rb_burn_mean_duration(), rb_burnable_fuel(), rb_charge_burn_evaluate(), rb_kernel_positive_mean(), rb_wiebe() (+7 more)

### Community 111 - "setup"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 112 - "check_combustion_variation.py"
Cohesion: 0.21
Nodes (9): commands(), digest(), execute(), one(), main(), validate(), finite(), CombustionVariationTests (+1 more)

### Community 113 - "diagnostics.c"
Cohesion: 0.14
Nodes (23): add(), pipe_role(), rb_diag_bind_engine(), rb_diag_impulse(), rb_diag_init(), rb_diag_inventory(), rb_diag_note(), rb_diag_repair() (+15 more)

### Community 114 - "Engine integration, architecture selection and base acceptance"
Cohesion: 0.17
Nodes (12): Engine integration, architecture selection and base acceptance, Stage 053 — Compressible and reversing turbulent-flow excitation, Stage 054 — Unified pulse and noise routing, Stage 055 — Engine-specific structure and accessories, Stage 056 — Listener, vehicle and recording-transfer validation, Stage 057 — Whole-engine candidate integration, Stage 058 — Robotic deceleration and lift-off causal diagnosis, Stage 059 — Five-engine physical and acoustic qualification (+4 more)

### Community 115 - "Propagation prototypes and physical components"
Cohesion: 0.17
Nodes (12): Propagation prototypes and physical components, Stage 043 — Improved conservative solver and production-mixture control, Stage 044 — Executable shared exhaust geometry compiler, Stage 045 — General zero-flow open gas-to-graph coupling, Stage 046 — Uniform mean-flow coupling, Stage 047 — Gas-property and wall-thermal model characterization, Stage 048 — Thermal fronts, coefficient updates and reverse entrainment, Stage 049 — Branches, finite junctions and all-port transfer (+4 more)

### Community 116 - "subprocess"
Cohesion: 0.05
Nodes (27): read_wav(), read_wav_bytes(), main(), analyze(), digest(), execute(), one(), main() (+19 more)

### Community 117 - "material.c"
Cohesion: 0.21
Nodes (24): derive(), failure(), heat(), positive(), rb_mt_energy(), rb_mt_left_inventory(), rb_mt_load(), rb_mt_momentum() (+16 more)

### Community 118 - "matplotlib_pyplot"
Cohesion: 0.23
Nodes (10): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+2 more)

### Community 119 - "Roadmap execution and evidence protocol"
Cohesion: 0.17
Nodes (12): Change control and evidence preservation, Dependency and milestone sketch, Integrated release scenario, Minimum execution record, Preserved sound and scenario obligations, Quantitative validation policy, Required validation commands for code stages, Roadmap execution and evidence protocol (+4 more)

### Community 120 - "REV-BENCH — notes for Claude"
Cohesion: 0.08
Nodes (21): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+13 more)

### Community 121 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.40
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 122 - "rb_tw_step"
Cohesion: 0.29
Nodes (12): positive(), rb_tw_balance(), rb_tw_capacity(), rb_tw_prepare(), rb_tw_step(), rb_tw_temperature(), temperature(), test_thermal_capacity_and_closed_books() (+4 more)

### Community 123 - "Stage 045 — Production-mixture gas to actual B0 graph"
Cohesion: 0.33
Nodes (6): Executed method and rejected controls, Gates frozen before implementation measurements, Numerical results and valid envelope, Preflight, research and ownership, Regression, failures, cost and handoff, Stage 045 — Production-mixture gas to actual B0 graph

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
Cohesion: 0.17
Nodes (11): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+3 more)

### Community 131 - "Knock, mechanical limits and damage"
Cohesion: 0.33
Nodes (6): Knock, mechanical limits and damage, Stage 085 — End-gas and knock physical model, Stage 086 — Knock-limited ECU, audible knock and feedback, Stage 087 — Piston speed, valve float and mechanical limits, Stage 088 — Optional damage state, warnings and reset behavior, Stage 089 — Knock and damage presentation effects

### Community 132 - "Expanded engine and exhaust catalog"
Cohesion: 0.33
Nodes (6): Expanded engine and exhaust catalog, Stage 090 — Additional inline and single-cylinder engines, Stage 091 — 60-degree and 90-degree V6 engines, Stage 092 — V10 and V12 engines, Stage 093 — Boxer-six engine, Stage 094 — Complete engine-specific exhaust and catalog calibration

### Community 133 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 134 - "stage-113.md"
Cohesion: 0.15
Nodes (10): Brief review of completed Stages 001–036, Checks, evidence and hold, Coverage of every completed stage, Exact new reproduction results, Findings, Bounded completed-stage review repairs, Stage 112 — Recoverable checkpoint section publication, Stage 113 — Acoustic derived-state and numerical failure guards (+2 more)

### Community 135 - "count"
Cohesion: 0.60
Nodes (3): count(), import(), value()

### Community 136 - "Playable browser bench"
Cohesion: 0.40
Nodes (5): Playable browser bench, Stage 066 — Playable browser engine and audio lifecycle, Stage 067 — Engine, intake and exhaust control panel, Stage 068 — Live instruments, engine telemetry and native-owned dyno, Stage 069 — First playable browser qualification

### Community 137 - "Stage 113 — Acoustic derived-state and numerical failure guards"
Cohesion: 0.50
Nodes (4): Frozen checks and execution, Results, cost and regression evidence, Scope, model and public contract, Stage 113 — Acoustic derived-state and numerical failure guards

### Community 138 - "analyze_acoustic_candidates.py"
Cohesion: 0.09
Nodes (24): accuracy(), analytic_line(), magnitude_db(), main(), plots(), publish(), read(), summarize() (+16 more)

### Community 139 - "Conditional learned-synthesis research"
Cohesion: 0.67
Nodes (3): Conditional learned-synthesis research, Stage 064 — Standalone learned pulse and DSP-parameter benchmark, Stage 065 — Conditional learned unresolved-source integration

### Community 140 - "Stage039 — Matched references and calibration protocol"
Cohesion: 0.17
Nodes (12): 2026-10-24 user-supplied video intake, Extended manufacturer/public-source research — 2026-10-24, Frozen reference rules, Gates, Implementation and acquired evidence, Initial bounded protocol closeout — 2026-10-24, superseded, Initial outcome and review handoff — 2026-10-23, Measurements and audition evidence (+4 more)

### Community 141 - "rb_cli.c"
Cohesion: 0.08
Nodes (47): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+39 more)

### Community 142 - "Stage039 — expanded reference search and qualification review"
Cohesion: 0.08
Nodes (22): Correct engine identity before collecting more sound, Final039 gates and practical remaining request, Manufacturers and exhaust variants covered, Media and measurement acquisition outcomes, Outcome, Review, validation and handoff, Search method and claim boundaries, Stage039 — expanded reference search and qualification review (+14 more)

### Community 143 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.14
Nodes (10): Idle tuning, Notes for later stages, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+2 more)

### Community 144 - "Stage039 — six supplied candidate videos reviewed"
Cohesion: 0.33
Nodes (5): Decision, Inspection and processing record, Remaining gap and handoff, Source findings, Stage039 — six supplied candidate videos reviewed

### Community 145 - "prepare_video_references.py"
Cohesion: 0.10
Nodes (21): fragment(), fragment(), compare(), db_ratio(), deltas(), main(), measure(), auditions() (+13 more)

### Community 146 - "style"
Cohesion: 0.21
Nodes (11): chart(), colorbar(), compare(), engine(), grid(), main(), pulse(), runs() (+3 more)

### Community 147 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.22
Nodes (9): Checked against chemical equilibrium, Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it (+1 more)

### Community 148 - "Selective sound rollback — 2026-10-22"
Cohesion: 0.50
Nodes (4): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22

### Community 149 - "rb_test.h"
Cohesion: 0.18
Nodes (7): main(), selected(), at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 151 - "Stage040 — Spark, ignition and flame-development causality"
Cohesion: 0.20
Nodes (10): Architecture and predeclared gates, Complete sound checkpoint comparison, Executed independent proof, Explicit limits and next handoff, Frozen physical question and research, Implemented causal state and ownership, Outcome — bounded completion and HOLD, Reproduction (+2 more)

### Community 152 - "Stage041 — continuous combustion variation and residual validity"
Cohesion: 0.14
Nodes (14): Complete listening checkpoint, Executed numerical evidence, Execution, Frozen evidence gates before candidate evaluation, Heat-release phasing, Late chemistry decision, Predecessor review and physical question, Processing cost and reproducibility (+6 more)

### Community 153 - "stddef"
Cohesion: 0.17
Nodes (3): mixture_ns(), now(), nozzle_ns()

### Community 154 - "exact_riemann"
Cohesion: 0.36
Nodes (4): exact_riemann(), eq(), wave(), RiemannReferenceTests

### Community 160 - "Stage 114 — fixed-geometry thermal transport bridge"
Cohesion: 0.29
Nodes (7): Closeout and handoff, Executed method and results, Frozen gates and planned controls, Predecessor review and design, before candidate measurements, Review findings and failure detection, Stage 114 — fixed-geometry thermal transport bridge, Validation, cost and integration limits

### Community 161 - "02-existing-sound.md"
Cohesion: 0.18
Nodes (5): Shared exhaust geometry with bidirectional physical loading, Legacy roadmap — archived 2026-10-23, Native acoustic candidates — S2d, Open-port coupling checkpoint, Reproduce

### Community 162 - "Stage 042 — Actual fuel-injection events and acoustic triggers"
Cohesion: 0.20
Nodes (10): Complete audio checkpoint and preservation, Contract and ownership, Frozen gates before candidate evaluation, Implementation and predecessor review, Independent tests, faults and environment, Physical question and research, Processing cost and memory, Reproduction, final review and handoff (+2 more)

### Community 163 - "Mesh"
Cohesion: 0.33
Nodes (3): Mesh, half_area(), p()

### Community 167 - "S2d — first native acoustic-renderer candidates"
Cohesion: 0.33
Nodes (6): Implemented modules and contracts, Matrix and numerical results, Outcome, Reproduction and research, S2d — first native acoustic-renderer candidates, Validation, publication and remaining work

### Community 170 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.15
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 171 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 173 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 176 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 179 - "Stage 049 — finite junctions and all-port transfer"
Cohesion: 0.25
Nodes (7): 28 October continuation: rejected open-port extrapolation, Definitions needed by the next stage, Frozen v3 results and validation, Independent references and gates, before candidate measurements, Near-unit reconstruction review — candidatev3, Predecessor review and frozen design, Stage 049 — finite junctions and all-port transfer

### Community 180 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1099 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **36 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `test_noise.c`, `rb_engine_step`, `Stage 13 — Performance & stability pass`, `rb_engine_init_at`, `test_pipe.c`, `RbExports`, `chamber.h`, `combustion.c`, `rb_create`, `run_held`, `audio.c`, `rb_preset_desc`, `string`, `REV-BENCH — notes for Claude`, `rb_gas_init`, `rb_engine_desc_check`, `rb_cylinder_move`, `rb_network_step`?**
  _High betweenness centrality (0.105) - this node is a cross-community bridge._
- **Are the 101 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 101 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_variation.c` be split into smaller, more focused modules?**
  _Cohesion score 0.13090418353576247 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `engine-node.ts`, `RbSim`, `Stage 8 — Engine layouts & presets`, `REV-BENCH — notes for Claude`, `test_engine.c`?**
  _High betweenness centrality (0.080) - this node is a cross-community bridge._
- **Should `Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧` be split into smaller, more focused modules?**
  _Cohesion score 0.10526315789473684 - nodes in this community are weakly interconnected._
- **Why does `rb_engine_step()` connect `rb_engine_step` to `test_variation.c`, `rb_engine_api.c`, `rb_cli.c`, `rb_cli.c`, `audio.c`, `rb_preset_desc`, `rb_test.h`, `test_engine.c`, `rb_cylinder_move`, `rb_network_step`, `Stage 13 — Performance & stability pass`, `test_pipe.c`, `audio_source_probe.c`, `throttle.c`, `test_noise.c`, `rb_engine_init_at`, `test_audio.c`, `engine.h`, `run_held`, `string`, `rb_engine_desc_check`, `chamber.h`, `combustion.c`?**
  _High betweenness centrality (0.044) - this node is a cross-community bridge._