# Graph Report - rev-bench  (2026-11-01)

## Corpus Check
- 373 files · ~755,022 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 3452 nodes · 9941 edges · 201 communities (161 shown, 40 thin omitted)
- Extraction: 78% EXTRACTED · 22% INFERRED · 0% AMBIGUOUS · INFERRED: 2219 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `011b3cfa`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_audio.c
- rb_cylinder_move
- Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧
- rb_engine_step
- csv
- test_acoustic_candidates.c
- rb-core.ts
- Stages
- main.ts
- presets.c
- package.json
- titles
- RbExports
- publish_section
- RbSim
- style
- wav.c
- render
- rb_gas_init
- Model limitations and validation still requiring attention
- coupled_duct.c
- numpy
- compilerOptions
- rb_destroy
- audio.c
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- rb_engine_step
- rb_cylinder_move
- fixed_transport.c
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- rb_engine_init
- rb_destroy
- Stage 8 — Engine layouts & presets
- Stage 13 — Performance & stability pass
- check_injection_events.py
- examples
- REV-BENCH — notes for Claude
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Stage 3 — Gas & flow primitives
- Stages
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- style
- render_results.py
- test_cylinder.c
- 02-existing-sound.md
- REV-BENCH
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- Phase 3 — Browser
- presets.c
- graphify-guard.sh
- session-start.sh
- noise.c
- compare_real.py
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)
- rb_engine_init
- rb_cylinder_init
- Stage 5 — Valvetrain, throttle & intake plenum
- REV-BENCH — the plan
- audio_experiment_report.py
- rb_engine_init_at
- Existing sound, measurements and bounded prototypes
- Stage 036 — complete project roadmap and evidence protocol
- pathlib
- AGENTS.md
- test_diagnostic_orifice_thermal_and_radiation
- chamber.h
- rb_dyno_init
- rb_create
- Stage 3 — Gas & flow primitives
- rb_cli.c
- rb_chamber_init
- REV-BENCH — Feedback checkpoint before stage 13b
- 04-propagation.md
- acoustic_transport_probe.c
- test_presets.c
- Historical foundations and engine core
- REV-BENCH
- test_exhaust_geometry.py
- check_combustion_variation.py
- gas_graph.c
- Initial audio bottleneck diagnosis — 2026-10-22
- read_wav
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- Phase 2b — Sound realism (top priority) 🎧
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- rb_chamber_gamma
- combustion.c
- sys
- rb_engine_init
- diagnostics.c
- Engine integration, architecture selection and base acceptance
- Propagation prototypes and physical components
- combustion_repetition.py
- material.c
- read_csv
- Roadmap execution and evidence protocol
- REV-BENCH — notes for Claude
- audit_s1.py
- math
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
- 14-review-fixes.md
- setup
- Playable browser bench
- test_injection.c
- csv
- Conditional learned-synthesis research
- Stage039 — Matched references and calibration protocol
- build_contract.py
- Stage039 — expanded reference search and qualification review
- test_motored_loss_angle
- Stage039 — six supplied candidate videos reviewed
- prepare_video_references.py
- os
- combustion_variation_probe.c
- Phase 2 — Sound (the realistic part)
- test_main.c
- Stage040 — Spark, ignition and flame-development causality
- Stage041 — continuous combustion variation and residual validity
- SimpleWave
- exact_riemann
- Stage039 closeout review and Stage041 readiness
- Stage 8 — Engine layouts & presets
- rb_test.h
- Stage 114 — fixed-geometry thermal transport bridge
- Stage 13b-p — researched exhaust configurations
- Stage 042 — Actual fuel-injection events and acoustic triggers
- GR86 and E46 video references — 2026-10-24
- loads
- test_overrun.c
- The mufflers
- Should REV-BENCH change its sound-generation architecture?
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- The workspace
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- bench_wasm.mjs
- Stage 049 — finite junctions and all-port transfer
- Stage 1 — Native C scaffold, WAV writer, test runner
- Stage 038 — Numerical guard and flux observability
- REV-BENCH — Research notes (stage 0)
- cmd_presets
- Stage 13b-r — high-RPM exhaust character
- S2b — Loaded exhaust detail and the lift-off contrast
- rb_tw_step
- string
- build_contract_probe.c
- Stage 6 — Combustion + heat transfer
- test_combustion.c
- Phase 6 — Polish & ship
- Stage 048 — Thermal fronts, evolving properties and reversal reference
- S2d — first native acoustic-renderer candidates
- dyno_average
- Current sound results
- Stage S1c — Reproducible sound and combustion baselines
- S2 — Additional flow sources and optional spatial resolution
- S1e — Research and controlled overrun transitions

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 123 edges
2. `rb_engine_init()` - 60 edges
3. `rb_engine_init_at()` - 59 edges
4. `RbExports` - 53 edges
5. `rb_preset_desc()` - 48 edges
6. `REV-BENCH - Codex handoff` - 48 edges
7. `read_wav()` - 44 edges
8. `rb_chamber_init()` - 43 edges
9. `Core (C)` - 43 edges
10. `style()` - 38 edges

## Surprising Connections (you probably didn't know these)
- `What was built` --references--> `rb_chamber_integrate_many()`  [INFERRED]
  docs/stage-13b.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/roadmap/legacy-plan.md → core/src/chamber.c
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `What changed` --references--> `rb_kernel_delay_mean()`  [INFERRED]
  docs/stage-115.md → core/src/combustion.c
- `Notes for later stages` --references--> `rb_cylinder_move()`  [INFERRED]
  docs/stage-05.md → core/src/cylinder.c

## Import Cycles
- None detected.

## Communities (201 total, 40 thin omitted)

### Community 0 - "test_audio.c"
Cohesion: 0.13
Nodes (34): rb_engine_combustion_model(), rb_engine_step(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+26 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.19
Nodes (4): One canonical roadmap with evidence-gated promotion, Audit finding ownership and closure evidence, Legacy stage crosswalk, Required use

### Community 2 - "Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧"
Cohesion: 0.09
Nodes (25): variation(), Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail (+17 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.12
Nodes (32): rb_burnable_fuel(), accessory_torque(), add_share(), after_firing_tdc(), burn_step(), computer_spark(), crank_turn(), cycle_angle() (+24 more)

### Community 4 - "csv"
Cohesion: 0.15
Nodes (15): analyze(), brief(), plot_summary(), read(), select(), self_test(), evaluate(), qualify_enthalpy() (+7 more)

### Community 5 - "test_acoustic_candidates.c"
Cohesion: 0.05
Nodes (92): 1. Same-tick return pressure needs an API boundary, 2. Port signs, pressure reference and work ownership must be explicit, 3. The background gas cannot be removed with its acoustic grid, 4. Shared geometry is currently a snapshot, not an executable compiler, 5. Stable output can conceal modified gas transport, 6. Native stereo and the browser host are different validation targets, 7. The single results file is not reproducibly rebuildable today, 8. Provenance and reference conditions still limit attribution (+84 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (45): PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE, RB_EF_PER_BANK (+37 more)

### Community 7 - "Stages"
Cohesion: 0.10
Nodes (20): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism, Progress tracker, REV-BENCH — Plan of Action (+12 more)

### Community 8 - "main.ts"
Cohesion: 0.13
Nodes (24): CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam(), findTelemetry() (+16 more)

### Community 9 - "presets.c"
Cohesion: 0.10
Nodes (38): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+30 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "titles"
Cohesion: 0.13
Nodes (37): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+29 more)

### Community 12 - "RbExports"
Cohesion: 0.06
Nodes (4): Consumer review before implementation, EngineDescription, RbCore, RbExports

### Community 13 - "publish_section"
Cohesion: 0.10
Nodes (20): Scope, contract and method, carry_diagnostics(), checked(), compose(), diagnostic_names(), fingerprint(), load(), publish_section() (+12 more)

### Community 15 - "style"
Cohesion: 0.19
Nodes (15): a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main(), orders() (+7 more)

### Community 16 - "wav.c"
Cohesion: 0.20
Nodes (23): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+15 more)

### Community 17 - "render"
Cohesion: 0.27
Nodes (11): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), rb_soft_limit(), render(), response() (+3 more)

### Community 18 - "rb_gas_init"
Cohesion: 0.18
Nodes (26): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_cv_from_u_diag() (+18 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (33): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+25 more)

### Community 20 - "coupled_duct.c"
Cohesion: 0.18
Nodes (29): add(), admissible(), book(), chars(), end_state(), linear_flux(), linear_rhs(), linear_tick() (+21 more)

### Community 21 - "numpy"
Cohesion: 0.09
Nodes (27): acoustic_case(), compare(), flow_case(), main(), execute(), run(), transfer(), main() (+19 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "rb_destroy"
Cohesion: 0.19
Nodes (28): rb_destroy(), rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that can fail, Tests that changed, amplitude(), engine_sim() (+20 more)

### Community 25 - "audio.c"
Cohesion: 0.17
Nodes (25): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step() (+17 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "rb_engine_step"
Cohesion: 0.09
Nodes (56): Core (C), rb_engine_desc_default(), rb_engine_settle(), rb_exhaust_model(), Results, Tests that changed, Tests that can fail, Tests that changed (+48 more)

### Community 29 - "rb_cylinder_move"
Cohesion: 0.12
Nodes (17): rb_cylinder_move(), Phase 1 — Physics core (native, offline, validated), Stage 3 — Gas & flow primitives, Stage 4 — Single cylinder: slider-crank + motoring, Stage 5 — Valvetrain, throttle & intake plenum, Stage 6 — Combustion + heat transfer, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Stage 8 — Engine layouts & presets (+9 more)

### Community 30 - "fixed_transport.c"
Cohesion: 0.15
Nodes (39): add(), advance(), book(), clock_limit(), faces(), fail(), heat(), known_valid() (+31 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.21
Nodes (16): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), Tests that can fail, blowdown_time_to_2bar() (+8 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.10
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 33 - "rb_engine_init"
Cohesion: 0.19
Nodes (9): Shared exhaust geometry with bidirectional physical loading, One complete SI build and generation-tagged observations, Cost, preservation and reproducibility, Design, research and consumer review, Implementation and proof boundaries, Results and failure controls, Scope and frozen evidence gates, Stage 037 — Shared build, geometry, event and snapshot contracts (+1 more)

### Community 34 - "rb_destroy"
Cohesion: 0.10
Nodes (20): D08 — P1: randomized stability checks omit the new junction chamber, Tests that changed, Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass (+12 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.32
Nodes (15): param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags(), rb_param_max(), rb_param_min() (+7 more)

### Community 36 - "Stage 13 — Performance & stability pass"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 37 - "check_injection_events.py"
Cohesion: 0.27
Nodes (5): commands(), validate(), finite(), InjectionEventTests, valid()

### Community 38 - "examples"
Cohesion: 0.16
Nodes (5): dumps(), ReferenceSession, examples(), SharedContracts, snapshot()

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.12
Nodes (16): A1 — conservative 1D comparison, Architecture and ownership, B0 — fixed-background acoustic graph, B1 — gas coupling, mean flow and lift-off, Comparison framework and rejection gates, Components, radiation and usable band, Decision and experimental arms, Learned synthesis: experiments and deployment (+8 more)

### Community 41 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.15
Nodes (32): rb_chamber_init_reservoir(), face_init(), rb_pipe_cells_for(), rb_pipe_init(), rb_pipe_join(), rb_pipe_pack(), Packing, count() (+24 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.32
Nodes (14): coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame(), path_name() (+6 more)

### Community 43 - "Stages"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.25
Nodes (6): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.22
Nodes (15): Engine build, Geometry, connections and frames, Shared contracts — version 1, Versioning, identity and limits, geometry(), paths(), rb_scene_flight_time(), rb_scene_init() (+7 more)

### Community 46 - "style"
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 47 - "render_results.py"
Cohesion: 0.10
Nodes (19): read_wav_bytes(), adopt_current(), compare(), db_ratio(), deltas(), main(), measure(), coast_markers() (+11 more)

### Community 48 - "test_cylinder.c"
Cohesion: 0.29
Nodes (8): compress(), engine_86x86(), isentrope_T(), test_big_volume_jump_is_split(), test_compression_converges_with_step_size(), test_compression_matches_isentrope(), test_cylinder_bad_input(), test_motoring_has_no_energy_drift()

### Community 50 - "02-existing-sound.md"
Cohesion: 0.16
Nodes (8): Legacy roadmap — archived 2026-10-23, Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22, Native acoustic candidates — S2d, Open-port coupling checkpoint, Reproduce

### Community 52 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.08
Nodes (24): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+16 more)

### Community 54 - "Phase 3 — Browser"
Cohesion: 0.25
Nodes (8): 2026-10-22 continuation: coupled port experiment and researched exhausts, Arithmetic and research, Feedback checkpoint and handoff, Measured cost, Stage 13b — Performance and solver work, Validation and sound, What was built, Why the implicit probe was rejected

### Community 55 - "presets.c"
Cohesion: 0.17
Nodes (12): 2026-10-24 supplied-video update, Additional rear cross V8 recordings — 2026-10-23, Bernardo's engine sound examples — 2026-10-21, How these examples guide the next sound work, Processing and distance, Processing and reproduction, R1b: optional filtered listening copies — 2026-10-21, Reproduction and checkpoint (+4 more)

### Community 60 - "noise.c"
Cohesion: 0.13
Nodes (21): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+13 more)

### Community 61 - "compare_real.py"
Cohesion: 0.11
Nodes (12): chart(), compare(), fetch(), main(), read_list(), read_wav(), main(), orders() (+4 more)

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.04
Nodes (47): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+39 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.14
Nodes (33): add(), clock_state(), displacement(), dot(), evaluate(), failure(), flux(), gradients() (+25 more)

### Community 66 - "rb_engine_init"
Cohesion: 0.19
Nodes (32): rb_engine_init(), rb_noise_init(), rb_noise_step(), The tests, Tests that can fail, across(), at_a_metre(), blow() (+24 more)

### Community 67 - "rb_cylinder_init"
Cohesion: 0.56
Nodes (7): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), test_cylinder_geometry()

### Community 68 - "Stage 5 — Valvetrain, throttle & intake plenum"
Cohesion: 0.22
Nodes (9): Equations used, How one step works, Notes for later stages, Results, Sources, Stage 5 — Valvetrain, throttle & intake plenum, Try it, Two things that surprised me (+1 more)

### Community 69 - "REV-BENCH — the plan"
Cohesion: 0.09
Nodes (23): Documentation map, Done, Exhaust-acoustics research (043–049, 114), How a stage is done, Optional, after the release, Phase A — clean-up and the engine in the browser, Phase B — the sound to real-engine standard, Phase C — the 3D engine (+15 more)

### Community 70 - "audio_experiment_report.py"
Cohesion: 0.12
Nodes (23): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+15 more)

### Community 71 - "rb_engine_init_at"
Cohesion: 0.20
Nodes (29): cell_heat(), cell_is_fast(), end_flow(), fast_cells(), fast_flows(), kinetic_energy(), momentum_flux(), push() (+21 more)

### Community 72 - "Existing sound, measurements and bounded prototypes"
Cohesion: 0.10
Nodes (20): Existing sound, measurements and bounded prototypes, Stage 017 — Float-audio analysis and Windows paths, Stage 018 — Finite burns stop at exhaust opening, Stage 019 — Five-preset repetition and combustion audit, Stage 020 — Trapped-mixture burn feedback, Stage 021 — Raw user-reference catalog, Stage 022 — Filtered reference listening library, Stage 023 — Loaded exhaust detail and lift-transition correction (+12 more)

### Community 73 - "Stage 036 — complete project roadmap and evidence protocol"
Cohesion: 0.40
Nodes (5): Deliverables, Review findings incorporated, Scope decisions, Stage 036 — complete project roadmap and evidence protocol, Validation and limits

### Community 74 - "pathlib"
Cohesion: 0.07
Nodes (23): digest(), main(), main(), check(), bench(), compare(), main(), sha() (+15 more)

### Community 76 - "test_diagnostic_orifice_thermal_and_radiation"
Cohesion: 0.60
Nodes (5): rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), test_diagnostic_orifice_thermal_and_radiation(), test_radiation_causal_impedance()

### Community 77 - "chamber.h"
Cohesion: 0.16
Nodes (3): mixture_ns(), now(), nozzle_ns()

### Community 78 - "rb_dyno_init"
Cohesion: 0.44
Nodes (7): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), test_dyno_holds_a_speed(), test_dyno_only_absorbs(), test_dyno_sweeps()

### Community 79 - "rb_create"
Cohesion: 0.14
Nodes (30): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), config_is_valid(), rb_config_default(), rb_config_quality() (+22 more)

### Community 80 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 81 - "rb_cli.c"
Cohesion: 0.25
Nodes (24): cmd_blowdown(), cmd_breathe(), cmd_dyno(), cmd_fire(), cmd_fuzz(), cmd_motor(), cmd_pipes(), cmd_run() (+16 more)

### Community 83 - "rb_chamber_init"
Cohesion: 0.33
Nodes (14): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+6 more)

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.29
Nodes (7): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed

### Community 85 - "04-propagation.md"
Cohesion: 0.07
Nodes (26): Decision, Inspection and processing, Remaining acquisition and qualification, Review and preservation, Stage 039 — two supplied dashboard references, What the footage establishes, Executed results and rejection, Preflight and frozen experiment (+18 more)

### Community 91 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 92 - "test_presets.c"
Cohesion: 0.26
Nodes (14): muffler_width(), rb_engine_desc_check(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), test_engine_description(), test_exhaust_descriptor_guards(), all_gaps() (+6 more)

### Community 93 - "Historical foundations and engine core"
Cohesion: 0.12
Nodes (17): Historical foundations and engine core, Stage 001 — Initial research and native-first project setup, Stage 002 — Native scaffold, WAV/CSV output and C tests, Stage 003 — WASM and AudioWorklet tone scaffold, Stage 004 — Gas properties, chambers and orifice flow, Stage 005 — Slider-crank and motored cylinder, Stage 006 — Valvetrain, throttle and intake plenum, Stage 007 — Baseline combustion and wall heat (+9 more)

### Community 94 - "REV-BENCH"
Cohesion: 0.25
Nodes (8): Abbreviations in the research stages (043–049, 114), Delivered-fuel event, Fueling attempt, Language, Numerical observation (Stage 038), REV-BENCH, Thermal/material reference (Stages 047–048), Uniform-flow acoustic coupling (Stages 045–046)

### Community 96 - "test_exhaust_geometry.py"
Cohesion: 0.25
Nodes (7): check(), faults(), compile_exhaust(), routes(), execute(), native_input(), PreparedGeometry

### Community 98 - "check_combustion_variation.py"
Cohesion: 0.21
Nodes (9): commands(), digest(), execute(), one(), main(), validate(), finite(), CombustionVariationTests (+1 more)

### Community 99 - "gas_graph.c"
Cohesion: 0.08
Nodes (77): accumulate(), acoustic_energy(), add(), admissible(), apply_residual(), backward(), book(), coordinates() (+69 more)

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "read_wav"
Cohesion: 0.08
Nodes (25): read_wav(), analyze(), long_coast(), main(), record(), render(), sha(), write_float() (+17 more)

### Community 103 - "Audio robustness: research and candidate roadmap — 2026-10-22"
Cohesion: 0.22
Nodes (9): Audio robustness: research and candidate roadmap — 2026-10-22, Candidate small-stage sequence and audit coverage, Conditions for an exploratory neural residual, Proposed gates and reproducible experiment fixtures, Reference use and data still needed, Research scope and verification, Runtime budgets consistent with realism first, Two transport candidates, after the diagnostic experiment (+1 more)

### Community 104 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 105 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.25
Nodes (8): Phase 2b — Sound realism (top priority) 🎧, Stage 13b-p — Model-specific exhaust research and implementation, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage 13b-r — High-RPM exhaust character and revised limits, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

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
Cohesion: 0.21
Nodes (17): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), move_gas() (+9 more)

### Community 110 - "combustion.c"
Cohesion: 0.19
Nodes (16): 2026-10-31: Claude's review and Stage 115; paused for Bernardo, clamp(), flame_speed(), normal_cdf(), rb_burn_mean_duration(), rb_charge_burn_evaluate(), rb_kernel_delay_mean(), rb_kernel_delay_unit() (+8 more)

### Community 111 - "sys"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 112 - "rb_engine_init"
Cohesion: 0.08
Nodes (63): rb_fuel_heat(), pipe_role(), rb_diag_bind_engine(), rb_diag_init(), rb_diag_reason_name(), rb_engine_contents(), rb_engine_cylinder_angle(), rb_engine_init_at() (+55 more)

### Community 113 - "diagnostics.c"
Cohesion: 0.44
Nodes (10): add(), rb_diag_impulse(), rb_diag_inventory(), rb_diag_note(), rb_diag_repair(), rb_diag_role_name(), rb_diag_transfer(), reasons_add() (+2 more)

### Community 114 - "Engine integration, architecture selection and base acceptance"
Cohesion: 0.17
Nodes (12): Engine integration, architecture selection and base acceptance, Stage 053 — Compressible and reversing turbulent-flow excitation, Stage 054 — Unified pulse and noise routing, Stage 055 — Engine-specific structure and accessories, Stage 056 — Listener, vehicle and recording-transfer validation, Stage 057 — Whole-engine candidate integration, Stage 058 — Robotic deceleration and lift-off causal diagnosis, Stage 059 — Five-engine physical and acoustic qualification (+4 more)

### Community 115 - "Propagation prototypes and physical components"
Cohesion: 0.17
Nodes (12): Propagation prototypes and physical components, Stage 043 — Improved conservative solver and production-mixture control, Stage 044 — Executable shared exhaust geometry compiler, Stage 045 — General zero-flow open gas-to-graph coupling, Stage 046 — Uniform mean-flow coupling, Stage 047 — Gas-property and wall-thermal model characterization, Stage 048 — Thermal fronts, coefficient updates and reverse entrainment, Stage 049 — Branches, finite junctions and all-port transfer (+4 more)

### Community 116 - "combustion_repetition.py"
Cohesion: 0.14
Nodes (12): analyze(), digest(), execute(), one(), main(), a_weighting(), as_db(), band() (+4 more)

### Community 117 - "material.c"
Cohesion: 0.21
Nodes (24): derive(), failure(), heat(), positive(), rb_mt_energy(), rb_mt_left_inventory(), rb_mt_load(), rb_mt_momentum() (+16 more)

### Community 118 - "read_csv"
Cohesion: 0.26
Nodes (9): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+1 more)

### Community 119 - "Roadmap execution and evidence protocol"
Cohesion: 0.17
Nodes (12): Change control and evidence preservation, Dependency and milestone sketch, Integrated release scenario, Minimum execution record, Preserved sound and scenario obligations, Quantitative validation policy, Required validation commands for code stages, Roadmap execution and evidence protocol (+4 more)

### Community 120 - "REV-BENCH — notes for Claude"
Cohesion: 0.11
Nodes (17): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+9 more)

### Community 121 - "audit_s1.py"
Cohesion: 0.12
Nodes (14): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), execute() (+6 more)

### Community 122 - "math"
Cohesion: 0.23
Nodes (4): wrap(), now(), wrap(), array()

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
Cohesion: 0.15
Nodes (12): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+4 more)

### Community 131 - "Knock, mechanical limits and damage"
Cohesion: 0.33
Nodes (6): Knock, mechanical limits and damage, Stage 085 — End-gas and knock physical model, Stage 086 — Knock-limited ECU, audible knock and feedback, Stage 087 — Piston speed, valve float and mechanical limits, Stage 088 — Optional damage state, warnings and reset behavior, Stage 089 — Knock and damage presentation effects

### Community 132 - "Expanded engine and exhaust catalog"
Cohesion: 0.33
Nodes (6): Expanded engine and exhaust catalog, Stage 090 — Additional inline and single-cylinder engines, Stage 091 — 60-degree and 90-degree V6 engines, Stage 092 — V10 and V12 engines, Stage 093 — Boxer-six engine, Stage 094 — Complete engine-specific exhaust and catalog calibration

### Community 133 - "Stage S1a — Reliable sound analysis before further tuning"
Cohesion: 0.40
Nodes (4): Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning, Validation

### Community 134 - "14-review-fixes.md"
Cohesion: 0.07
Nodes (23): Brief review of completed Stages 001–036, Checks, evidence and hold, Coverage of every completed stage, Exact new reproduction results, Findings, Bounded completed-stage review repairs, Stage 112 — Recoverable checkpoint section publication, Stage 113 — Acoustic derived-state and numerical failure guards (+15 more)

### Community 135 - "setup"
Cohesion: 0.15
Nodes (11): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+3 more)

### Community 136 - "Playable browser bench"
Cohesion: 0.33
Nodes (6): Playable browser bench, Stage 066 — Playable browser engine and audio lifecycle, Stage 067 — Engine, intake and exhaust control panel, Stage 068 — Live instruments, engine telemetry and native-owned dyno, Stage 069 — First playable browser qualification, Stage 118 — A real-time tier for today's engine

### Community 137 - "test_injection.c"
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

### Community 141 - "build_contract.py"
Cohesion: 0.32
Nodes (11): audible_performance_time(), counter(), identity(), keys(), number(), require(), validate_build(), validate_event() (+3 more)

### Community 142 - "Stage039 — expanded reference search and qualification review"
Cohesion: 0.22
Nodes (9): Correct engine identity before collecting more sound, Final 039 gates and practical remaining request, Manufacturers and exhaust variants covered, Media and measurement acquisition outcomes, Outcome, Review, validation and handoff, Search method and claim boundaries, Stage 039 — expanded reference search and qualification review (+1 more)

### Community 143 - "test_motored_loss_angle"
Cohesion: 0.48
Nodes (5): rb_wall_area(), rb_woschni_h(), rb_woschni_w(), test_motored_loss_angle(), test_woschni()

### Community 144 - "Stage039 — six supplied candidate videos reviewed"
Cohesion: 0.33
Nodes (5): Decision, Inspection and processing record, Remaining gap and handoff, Source findings, Stage 039 — six supplied candidate videos reviewed

### Community 145 - "prepare_video_references.py"
Cohesion: 0.16
Nodes (14): auditions(), checked_lock(), main(), publish(), screen(), audition_gains(), digest(), measurements() (+6 more)

### Community 146 - "os"
Cohesion: 0.24
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 147 - "combustion_variation_probe.c"
Cohesion: 0.21
Nodes (7): add(), main(), number(), usage(), add(), number(), output_stats()

### Community 148 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 151 - "Stage040 — Spark, ignition and flame-development causality"
Cohesion: 0.20
Nodes (10): Architecture and predeclared gates, Complete sound checkpoint comparison, Executed independent proof, Explicit limits and next handoff, Frozen physical question and research, Implemented causal state and ownership, Outcome — bounded completion and HOLD, Reproduction (+2 more)

### Community 152 - "Stage041 — continuous combustion variation and residual validity"
Cohesion: 0.13
Nodes (14): Complete listening checkpoint, Executed numerical evidence, Execution, Frozen evidence gates before candidate evaluation, Heat-release phasing, Late chemistry decision, Predecessor review and physical question, Processing cost and reproducibility (+6 more)

### Community 153 - "SimpleWave"
Cohesion: 0.11
Nodes (15): balance(), experiment(), evaluate(), run(), experiment(), evaluate(), experiment(), run() (+7 more)

### Community 154 - "exact_riemann"
Cohesion: 0.29
Nodes (4): exact_riemann(), eq(), wave(), RiemannReferenceTests

### Community 155 - "Stage039 closeout review and Stage041 readiness"
Cohesion: 0.29
Nodes (7): Checks and reproducibility, Current coverage and permitted comparisons, Decision and completion boundary, Exit-evidence review, Open claims and their existing owners, Review findings and Stage 040 handoff, Stage 039 closeout review and Stage 041 readiness

### Community 156 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.14
Nodes (10): Idle tuning, Notes for later stages, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+2 more)

### Community 159 - "rb_test.h"
Cohesion: 0.21
Nodes (4): check_chamber(), fail(), rb_fuzz_inspect_engine(), test_diagnostic_junction_repair_and_fuzz()

### Community 160 - "Stage 114 — fixed-geometry thermal transport bridge"
Cohesion: 0.29
Nodes (7): Closeout and handoff, Executed method and results, Frozen gates and planned controls, Predecessor review and design, before candidate measurements, Review findings and failure detection, Stage 114 — fixed-geometry thermal transport bridge, Validation, cost and integration limits

### Community 161 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 162 - "Stage 042 — Actual fuel-injection events and acoustic triggers"
Cohesion: 0.18
Nodes (10): Complete audio checkpoint and preservation, Contract and ownership, Frozen gates before candidate evaluation, Implementation and predecessor review, Independent tests, faults and environment, Physical question and research, Processing cost and memory, Reproduction, final review and handoff (+2 more)

### Community 163 - "GR86 and E46 video references — 2026-10-24"
Cohesion: 0.33
Nodes (6): Audio screening and measurement authority, GR86 and E46 video references — 2026-10-24, Reproduction and results, Source identity and usefulness, Verified intake checkpoint, Visual review and selected source windows

### Community 167 - "loads"
Cohesion: 0.20
Nodes (9): catalog(), from_native(), loads(), unique(), check(), main(), native(), prepare_build() (+1 more)

### Community 168 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 169 - "The mufflers"
Cohesion: 0.29
Nodes (7): A muffler is a pipe, Going straight on, The boom, The chambered can, The mufflers, The rasp, What they do

### Community 170 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

### Community 172 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.15
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 173 - "The workspace"
Cohesion: 0.40
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (done, 2026-10-20), The workspace

### Community 176 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.10
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 179 - "Stage 049 — finite junctions and all-port transfer"
Cohesion: 0.25
Nodes (8): 28 October continuation: rejected open-port extrapolation, Definitions needed by the next stage, Final review and bounded closeout — 28 October 2026, Frozen v3 results and validation, Independent references and gates, before candidate measurements, Near-unit reconstruction review — candidate v3, Predecessor review and frozen design, Stage 049 — finite junctions and all-port transfer

### Community 180 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.40
Nodes (4): Equations used, Results, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 181 - "Stage 038 — Numerical guard and flux observability"
Cohesion: 0.25
Nodes (8): Cost and memory, Gates and reproduction, Lifetime and bounded cost contract, Measured numerical results, Model, ownership and limits, Predecessor review, Qualification and handoff, Stage 038 — Numerical guard and flux observability

### Community 183 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.10
Nodes (20): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), After stage 12b: a piano note (stage 12c) (+12 more)

### Community 184 - "cmd_presets"
Cohesion: 0.28
Nodes (12): rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_presets(), fire_sweep(), pipes_engine(), print_firing() (+4 more)

### Community 185 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

### Community 186 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 187 - "rb_tw_step"
Cohesion: 0.29
Nodes (12): positive(), rb_tw_balance(), rb_tw_capacity(), rb_tw_prepare(), rb_tw_step(), rb_tw_temperature(), temperature(), test_thermal_capacity_and_closed_books() (+4 more)

### Community 189 - "build_contract_probe.c"
Cohesion: 0.60
Nodes (3): count(), import(), value()

### Community 190 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.22
Nodes (9): Checked against chemical equilibrium, Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it (+1 more)

### Community 191 - "test_combustion.c"
Cohesion: 0.40
Nodes (3): test_burnable_fuel(), test_fuel_heat(), test_wiebe_burn()

### Community 192 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 194 - "Stage 048 — Thermal fronts, evolving properties and reversal reference"
Cohesion: 0.33
Nodes (6): Design and frozen gates — before candidate measurements, Executed method and bounded results, Explicit split, decision and next prerequisite, Retained failures and improved numerical work, Stage 048 — Thermal fronts, evolving properties and reversal reference, Validation and whole-system limits

### Community 195 - "S2d — first native acoustic-renderer candidates"
Cohesion: 0.33
Nodes (6): Implemented modules and contracts, Matrix and numerical results, Outcome, Reproduction and research, S2d — first native acoustic-renderer candidates, Validation, publication and remaining work

### Community 197 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 198 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.40
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 199 - "S2 — Additional flow sources and optional spatial resolution"
Cohesion: 0.40
Nodes (4): Optional ultra quality, S2 — Additional flow sources and optional spatial resolution, Sources, Validation

### Community 200 - "S1e — Research and controlled overrun transitions"
Cohesion: 0.50
Nodes (4): Model, Real references, S1e — Research and controlled overrun transitions, Validation and remaining work

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 1106 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **40 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `rb_engine_step` to `rb_engine_init`, `rb_engine_step`, `rb_destroy`, `rb_engine_init_at`, `RbExports`, `rb_chamber_gamma`, `combustion.c`, `rb_create`, `rb_engine_init`, `rb_gas_init`, `rb_chamber_init`, `REV-BENCH — notes for Claude`, `audio.c`, `test_presets.c`, `rb_cylinder_move`, `Stage 11 — The sound: a microphone and the first WAV renders 🎧`?**
  _High betweenness centrality (0.123) - this node is a cross-community bridge._
- **Are the 101 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 101 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_audio.c` be split into smaller, more focused modules?**
  _Cohesion score 0.13472706155632985 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `rb_engine_step`, `RbSim`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `REV-BENCH — notes for Claude`, `Stage 8 — Engine layouts & presets`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **Should `Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧` be split into smaller, more focused modules?**
  _Cohesion score 0.08923076923076922 - nodes in this community are weakly interconnected._
- **Why does `REV-BENCH — notes for Claude` connect `REV-BENCH — notes for Claude` to `rb_cylinder_move`, `rb_engine_step`?**
  _High betweenness centrality (0.044) - this node is a cross-community bridge._