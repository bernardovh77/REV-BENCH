# Graph Report - rev-bench  (2026-10-23)

## Corpus Check
- 244 files · ~651,211 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 2524 nodes · 6770 edges · 151 communities (124 shown, 27 thin omitted)
- Extraction: 76% EXTRACTED · 24% INFERRED · 0% AMBIGUOUS · INFERRED: 1626 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `fba9bf04`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_variation.c
- PLAN.md
- rb_engine_step
- rb_engine_step
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- Stages
- main.ts
- presets.c
- package.json
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- RbExports
- compare_real.py
- RbSim
- style
- label
- audio.c
- noise.c
- Model limitations and validation still requiring attention
- coupled_duct.c
- rb_audit.c
- compilerOptions
- analyze_acoustic_candidates.py
- RevBenchNode
- gas.c
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- argparse
- rb_cylinder_move
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- review-completed-stages-2026-10-23.md
- test_audio.c
- checkpoint_sections.py
- Structural findings that change the next step
- engine.h
- REV-BENCH — Research notes (stage 0)
- S2d design — coupled exhaust acoustics and learned-DSP benchmarks
- Stage 13 — Performance & stability pass
- Stage 3 — Gas & flow primitives
- rb_engine_init_at
- REV-BENCH — notes for Claude
- numpy
- Test-Driven Development
- Selective sound rollback — 2026-10-22
- Stage 13b-p — researched exhaust configurations
- 02-existing-sound.md
- render_results.py
- Process
- Stage 13b — Performance feedback checkpoint
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Current sound results
- The workspace
- REV-BENCH - Codex handoff
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- string
- rand.h
- read_wav
- Canonical stage tracker
- csv
- test_pipe.c
- Existing sound, measurements and bounded prototypes
- Stage 036 — complete project roadmap and evidence protocol
- rb_engine_desc_default
- AGENTS.md
- Bounded completed-stage review repairs
- Stage 8 — Engine layouts & presets
- Stage 113 — Acoustic derived-state and numerical failure guards
- rb_create
- audio_source_probe.c
- cmd_info
- test_presets.c
- REV-BENCH — Feedback checkpoint before stage 13b
- string
- Legacy stage crosswalk
- S2b — Loaded exhaust detail and the lift-off contrast
- Historical foundations and engine core
- REV-BENCH
- pathlib
- rb_network_step
- test_acoustic_candidates.c
- Initial audio bottleneck diagnosis — 2026-10-22
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- Audio robustness: research and candidate roadmap — 2026-10-22
- Stage S1d — Burn timing follows the trapped mixture
- Stage S1b — Finite-time partial burns
- S2c — Source, transport and generation-architecture experiment
- Sound robustness and realism programme
- engine-node.ts
- hashlib
- setup
- Should REV-BENCH change its sound-generation architecture?
- math
- Engine integration, architecture selection and base acceptance
- Propagation prototypes and physical components
- test_audio_analysis.py
- rb_decimator_init
- matplotlib_pyplot
- Roadmap execution and evidence protocol
- Stage 1 — Native C scaffold, WAV writer, test runner
- Stage S1c — Reproducible sound and combustion baselines
- fuzz.c
- rb_engine_init
- Optional expansions
- rb_chamber_gamma
- Complete 3D engine experience
- Persistence, final acceptance and release
- Roadmap, shared contracts and trustworthy excitation
- Turbocharged engine development
- plot_fire.py
- Knock, mechanical limits and damage
- Expanded engine and exhaust catalog
- rb_chamber_init
- titles
- throttle.c
- Playable browser bench
- rb_pipe_integrate
- measure
- Conditional learned-synthesis research
- acoustic_transport_probe.c
- test_cylinder.c
- rb_cylinder_init
- Stage 5 — Valvetrain, throttle & intake plenum
- Stage 6 — Combustion + heat transfer
- plot_run.py
- rb_bench.c
- S2d — first native acoustic-renderer candidates
- test_main.c
- test_overrun.c
- bench_wasm.mjs

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

## Communities (151 total, 27 thin omitted)

### Community 0 - "test_variation.c"
Cohesion: 0.16
Nodes (31): rb_engine_cylinder_angle(), rb_engine_step(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+23 more)

### Community 1 - "PLAN.md"
Cohesion: 0.26
Nodes (3): One canonical roadmap with evidence-gated promotion, Audit finding ownership and closure evidence, REV-BENCH

### Community 2 - "rb_engine_step"
Cohesion: 0.11
Nodes (17): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed, The accessories (`accessory_power`, an engine field) (+9 more)

### Community 3 - "rb_engine_step"
Cohesion: 0.10
Nodes (36): rb_wiebe(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark() (+28 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.13
Nodes (38): rb_preset_count(), rb_preset_key(), rb_preset_name(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit() (+30 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.08
Nodes (65): rb_engine_desc_check(), rb_engine_displacement(), Dither, csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe() (+57 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "Stages"
Cohesion: 0.05
Nodes (42): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism (+34 more)

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

### Community 16 - "label"
Cohesion: 0.14
Nodes (10): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+2 more)

### Community 17 - "audio.c"
Cohesion: 0.18
Nodes (23): arriving(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step() (+15 more)

### Community 18 - "noise.c"
Cohesion: 0.28
Nodes (10): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+2 more)

### Community 19 - "Model limitations and validation still requiring attention"
Cohesion: 0.06
Nodes (33): Confirmed implementation and numerical issues, D01 — P1: modeled burning can precede its recorded spark, D02 — P1: the internal-jet noise model misidentifies choking, D03 — P1: added exhaust noise does not follow shared X/Y routing, D04 — P2: added-noise travel times omit center and link pipes, D05 — P1: synthetic noise retains hidden catalyst and obsolete losses, D06 — P1: turning variation off also switches combustion physics, D07 — P2: injector clicks are detached from injection events (+25 more)

### Community 20 - "coupled_duct.c"
Cohesion: 0.16
Nodes (29): add(), admissible(), book(), chars(), end_state(), linear_flux(), linear_rhs(), linear_tick() (+21 more)

### Community 21 - "rb_audit.c"
Cohesion: 0.33
Nodes (5): add(), audit(), main(), number(), usage()

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "analyze_acoustic_candidates.py"
Cohesion: 0.18
Nodes (11): accuracy(), analytic_line(), magnitude_db(), main(), plots(), publish(), read(), summarize() (+3 more)

### Community 25 - "gas.c"
Cohesion: 0.22
Nodes (20): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_from_u() (+12 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (46): Core (C), rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, ambient_density(), crank_books() (+38 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.10
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "argparse"
Cohesion: 0.11
Nodes (22): digest(), main(), analyze(), long_coast(), main(), record(), render(), sha() (+14 more)

### Community 31 - "rb_cylinder_move"
Cohesion: 0.12
Nodes (17): rb_cylinder_move(), Phase 1 — Physics core (native, offline, validated), Stage 3 — Gas & flow primitives, Stage 4 — Single cylinder: slider-crank + motoring, Stage 5 — Valvetrain, throttle & intake plenum, Stage 6 — Combustion + heat transfer, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Stage 8 — Engine layouts & presets (+9 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.10
Nodes (21): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, The cabin (+13 more)

### Community 33 - "review-completed-stages-2026-10-23.md"
Cohesion: 0.22
Nodes (7): Brief review of completed Stages 001–036, Checks, evidence and hold, Coverage of every completed stage, Exact new reproduction results, Findings, Frozen checks and execution, Stage 112 — Recoverable checkpoint section publication

### Community 34 - "test_audio.c"
Cohesion: 0.15
Nodes (33): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_destroy(), rb_free_floats(), rb_set_param(), rb_get_telemetry() (+25 more)

### Community 35 - "checkpoint_sections.py"
Cohesion: 0.18
Nodes (14): Scope, contract and method, adopt_current(), carry_diagnostics(), checked(), compose(), diagnostic_names(), fingerprint(), load() (+6 more)

### Community 36 - "Structural findings that change the next step"
Cohesion: 0.14
Nodes (14): 1. Same-tick return pressure needs an API boundary, 2. Port signs, pressure reference and work ownership must be explicit, 3. The background gas cannot be removed with its acoustic grid, 4. Shared geometry is currently a snapshot, not an executable compiler, 5. Stable output can conceal modified gas transport, 6. Native stereo and the browser host are different validation targets, 7. The single results file is not reproducibly rebuildable today, 8. Provenance and reference conditions still limit attribution (+6 more)

### Community 37 - "engine.h"
Cohesion: 0.14
Nodes (3): rb_exhaust_model(), now(), output_rms()

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.10
Nodes (20): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 2026-10-22 — selective exhaust rollback requested, 2026-10-22 — sound/physics audit, no tuning changes, 3D in the browser (for stages 17–19), After stage 12b: a piano note (stage 12c) (+12 more)

### Community 39 - "S2d design — coupled exhaust acoustics and learned-DSP benchmarks"
Cohesion: 0.12
Nodes (16): A1 — conservative 1D comparison, Architecture and ownership, B0 — fixed-background acoustic graph, B1 — gas coupling, mean flow and lift-off, Comparison framework and rejection gates, Components, radiation and usable band, Decision and experimental arms, Learned synthesis: experiments and deployment (+8 more)

### Community 41 - "Stage 13 — Performance & stability pass"
Cohesion: 0.10
Nodes (20): D08 — P1: randomized stability checks omit the new junction chamber, Tests that changed, Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass (+12 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "rb_engine_init_at"
Cohesion: 0.16
Nodes (30): new_pipe(), rb_engine_init_at(), cell_heat(), cell_is_fast(), end_flow(), fast_flows(), kinetic_energy(), momentum_flux() (+22 more)

### Community 44 - "REV-BENCH — notes for Claude"
Cohesion: 0.12
Nodes (12): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+4 more)

### Community 45 - "numpy"
Cohesion: 0.19
Nodes (16): analyze(), db(), main(), powers(), signal_metrics(), coast_features_and_plot(), db(), finish_plot() (+8 more)

### Community 46 - "Test-Driven Development"
Cohesion: 0.15
Nodes (10): Designing for Mockability, When to Mock, Anti-patterns, Rules of the loop, Seams: where tests go, Test-Driven Development, What a good test is, Bad Tests (+2 more)

### Community 47 - "Selective sound rollback — 2026-10-22"
Cohesion: 0.50
Nodes (4): Configuration, Handoff, Listening result and verification, Selective sound rollback — 2026-10-22

### Community 48 - "Stage 13b-p — researched exhaust configurations"
Cohesion: 0.25
Nodes (8): Final numerical and listening checks, Geometry and uncertainty ledger, Idle-control calibration, Physics review, Reproduce the checkpoint, Selected real systems, Stage 13b-p — researched exhaust configurations, Validation and feedback

### Community 50 - "02-existing-sound.md"
Cohesion: 0.13
Nodes (12): Shared exhaust geometry with bidirectional physical loading, Legacy roadmap — archived 2026-10-23, Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions (+4 more)

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

### Community 60 - "Current sound results"
Cohesion: 0.18
Nodes (9): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change, Changes, Next S1 work, Stage S1a — Reliable sound analysis before further tuning (+1 more)

### Community 61 - "The workspace"
Cohesion: 0.33
Nodes (5): Bernardo's machine (Windows + WSL), Claude's cloud sessions, .claude/settings.json, Moving it out of Personal-Projects (once), The workspace

### Community 62 - "REV-BENCH - Codex handoff"
Cohesion: 0.08
Nodes (25): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: complete sound/physics review — hold for instructions, 2026-10-22: full 13b and model-specific exhaust work resumed (+17 more)

### Community 64 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 66 - "string"
Cohesion: 0.23
Nodes (24): rb_noise_init(), rb_noise_step(), Tests that can fail, cabin_expected(), db(), flow_rms(), impact_energy(), jet_expected() (+16 more)

### Community 67 - "rand.h"
Cohesion: 0.70
Nodes (4): rb_rand_next(), rb_rand_normal(), rb_rand_uniform(), rb_rand_white()

### Community 68 - "read_wav"
Cohesion: 0.16
Nodes (10): write_float(), main(), report(), window_metrics(), excerpt(), main(), spectral_metrics(), telemetry() (+2 more)

### Community 69 - "Canonical stage tracker"
Cohesion: 0.09
Nodes (22): 10. Knock, mechanical limits and damage, 11. Expanded engine and exhaust catalog, 12. Persistence, final acceptance and release, 13. Optional expansions, 14. Bounded review repairs (execute before 037), 1. Historical foundations and engine core, 2. Existing sound, measurements and bounded prototypes, 3. Roadmap, shared contracts and trustworthy excitation (+14 more)

### Community 70 - "csv"
Cohesion: 0.14
Nodes (14): analyze(), brief(), plot_summary(), read(), select(), self_test(), encode_listening(), main() (+6 more)

### Community 71 - "test_pipe.c"
Cohesion: 0.18
Nodes (29): rb_chamber_init_reservoir(), face_init(), rb_pipe_contents(), rb_pipe_end_cell(), rb_pipe_init(), rb_pipe_join(), rb_pipe_onward(), Tests that can fail (+21 more)

### Community 72 - "Existing sound, measurements and bounded prototypes"
Cohesion: 0.10
Nodes (20): Existing sound, measurements and bounded prototypes, Stage 017 — Float-audio analysis and Windows paths, Stage 018 — Finite burns stop at exhaust opening, Stage 019 — Five-preset repetition and combustion audit, Stage 020 — Trapped-mixture burn feedback, Stage 021 — Raw user-reference catalog, Stage 022 — Filtered reference listening library, Stage 023 — Loaded exhaust detail and lift-transition correction (+12 more)

### Community 73 - "Stage 036 — complete project roadmap and evidence protocol"
Cohesion: 0.40
Nodes (5): Deliverables, Review findings incorporated, Scope decisions, Stage 036 — complete project roadmap and evidence protocol, Validation and limits

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.21
Nodes (16): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), test_audio_intake_mouth(), best_spark(), run_held() (+8 more)

### Community 76 - "Bounded completed-stage review repairs"
Cohesion: 0.67
Nodes (3): Bounded completed-stage review repairs, Stage 112 — Recoverable checkpoint section publication, Stage 113 — Acoustic derived-state and numerical failure guards

### Community 77 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.14
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 78 - "Stage 113 — Acoustic derived-state and numerical failure guards"
Cohesion: 0.50
Nodes (4): Frozen checks and execution, Results, cost and regression evidence, Scope, model and public contract, Stage 113 — Acoustic derived-state and numerical failure guards

### Community 79 - "rb_create"
Cohesion: 0.19
Nodes (24): config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample(), rb_physics_dt() (+16 more)

### Community 80 - "audio_source_probe.c"
Cohesion: 0.29
Nodes (15): rb_soft_limit(), coast_road_load(), main(), capture(), fail(), init_fixture(), main(), output_frame() (+7 more)

### Community 81 - "cmd_info"
Cohesion: 0.32
Nodes (15): param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find(), rb_param_flags(), rb_param_max(), rb_param_min() (+7 more)

### Community 83 - "test_presets.c"
Cohesion: 0.53
Nodes (7): all_gaps(), count_sparks(), firing_gaps(), gaps_are(), refused(), test_layout_rules(), test_presets_fire_where_the_layout_says()

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.11
Nodes (15): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+7 more)

### Community 85 - "string"
Cohesion: 0.32
Nodes (11): geometry(), paths(), rb_scene_flight_time(), rb_scene_init(), rb_scene_step(), read_source(), test_scene_aperture_radiation(), test_scene_doppler_tone() (+3 more)

### Community 92 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 93 - "Historical foundations and engine core"
Cohesion: 0.12
Nodes (17): Historical foundations and engine core, Stage 001 — Initial research and native-first project setup, Stage 002 — Native scaffold, WAV/CSV output and C tests, Stage 003 — WASM and AudioWorklet tone scaffold, Stage 004 — Gas properties, chambers and orifice flow, Stage 005 — Slider-crank and motored cylinder, Stage 006 — Valvetrain, throttle and intake plenum, Stage 007 — Baseline combustion and wall heat (+9 more)

### Community 96 - "pathlib"
Cohesion: 0.13
Nodes (9): publish(), assess(), exact_pressure(), initial(), read(), run(), CheckpointRenderTests, CoupledProofTests (+1 more)

### Community 98 - "rb_network_step"
Cohesion: 0.20
Nodes (15): equalising_mass(), rb_network_step(), rb_nozzle_critical_ratio(), rb_nozzle_mdot(), rb_orifice_flow(), rb_orifice_init(), blowdown_time_to_2bar(), setup_blowdown() (+7 more)

### Community 99 - "test_acoustic_candidates.c"
Cohesion: 0.06
Nodes (79): rb_radiation_init(), rb_radiation_linear(), rb_radiation_step(), Experiment and acceptance, Implemented scope, Measured outcome, Method and physical ownership, Reproduce (+71 more)

### Community 101 - "Initial audio bottleneck diagnosis — 2026-10-22"
Cohesion: 0.20
Nodes (10): 1. Numerical acoustic transport limits coherent detail, 2. Procedural highs are only partly coupled to the modeled exhaust, 3. Excitation and state transitions may be too approximate, 4. Current evidence does not implicate final compression, Architecture actually present, First proposed experiment, before choosing a remedy, How the suggested neural remedies apply, Initial audio bottleneck diagnosis — 2026-10-22 (+2 more)

### Community 102 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.22
Nodes (8): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports()

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

### Community 109 - "engine-node.ts"
Cohesion: 0.18
Nodes (7): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor, setup()

### Community 110 - "hashlib"
Cohesion: 0.17
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 111 - "setup"
Cohesion: 0.19
Nodes (12): comparison(), curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark() (+4 more)

### Community 112 - "Should REV-BENCH change its sound-generation architecture?"
Cohesion: 0.40
Nodes (5): Engine-specific neural research changes the comparison, Finer grids do not add missing acoustic modes, Should REV-BENCH change its sound-generation architecture?, The prototype and comparison method, Why changing the architecture may help

### Community 113 - "math"
Cohesion: 0.18
Nodes (10): rb_burnable_fuel(), rb_fuel_heat(), rb_species_R(), rb_wall_area(), rb_woschni_h(), rb_woschni_w(), test_burnable_fuel(), test_fuel_heat() (+2 more)

### Community 114 - "Engine integration, architecture selection and base acceptance"
Cohesion: 0.17
Nodes (12): Engine integration, architecture selection and base acceptance, Stage 053 — Compressible and reversing turbulent-flow excitation, Stage 054 — Unified pulse and noise routing, Stage 055 — Engine-specific structure and accessories, Stage 056 — Listener, vehicle and recording-transfer validation, Stage 057 — Whole-engine candidate integration, Stage 058 — Robotic deceleration and lift-off causal diagnosis, Stage 059 — Five-engine physical and acoustic qualification (+4 more)

### Community 115 - "Propagation prototypes and physical components"
Cohesion: 0.18
Nodes (11): Propagation prototypes and physical components, Stage 043 — Improved conservative solver and production-mixture control, Stage 044 — Executable shared exhaust geometry compiler, Stage 045 — General zero-flow open gas-to-graph coupling, Stage 046 — Uniform mean-flow coupling, Stage 047 — Gas-property and wall-thermal model characterization, Stage 048 — Thermal fronts, coefficient updates and reverse entrainment, Stage 049 — Branches, finite junctions and all-port transfer (+3 more)

### Community 116 - "test_audio_analysis.py"
Cohesion: 0.21
Nodes (4): AudioAnalysisTests, chunk(), wav_bytes(), failing_write()

### Community 117 - "rb_decimator_init"
Cohesion: 0.33
Nodes (9): bessel_i0(), rb_decimator_init(), rb_decimator_out(), rb_decimator_push(), rb_decimator_reset(), response(), test_audio_decimator_filter(), test_audio_decimator_tones() (+1 more)

### Community 118 - "matplotlib_pyplot"
Cohesion: 0.22
Nodes (11): plot_spark(), between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs() (+3 more)

### Community 119 - "Roadmap execution and evidence protocol"
Cohesion: 0.17
Nodes (12): Change control and evidence preservation, Dependency and milestone sketch, Integrated release scenario, Minimum execution record, Preserved sound and scenario obligations, Quantitative validation policy, Required validation commands for code stages, Roadmap execution and evidence protocol (+4 more)

### Community 120 - "Stage 1 — Native C scaffold, WAV writer, test runner"
Cohesion: 0.40
Nodes (4): Equations used, Results, Stage 1 — Native C scaffold, WAV writer, test runner, What was built

### Community 121 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 122 - "fuzz.c"
Cohesion: 0.31
Nodes (5): check_chamber(), check_engine(), fail(), seed_state(), uniform()

### Community 123 - "rb_engine_init"
Cohesion: 0.14
Nodes (33): rb_engine_contents(), rb_engine_init(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat(), slow_step(), rb_preset_desc(), rb_preset_find() (+25 more)

### Community 124 - "Optional expansions"
Cohesion: 0.20
Nodes (10): Optional expansions, Stage 103 — Gearbox and vehicle-driving mode, Stage 104 — WAV recording and audio export, Stage 105 — Two-stroke engine cycle, Stage 106 — Alternative fuels, Stage 107 — Supercharger system, Stage 108 — Individual throttle bodies, Stage 109 — Anti-lag system (+2 more)

### Community 125 - "rb_chamber_gamma"
Cohesion: 0.22
Nodes (11): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_gamma(), rb_chamber_pressure_now(), rb_chamber_sound_speed(), Tests that changed, What was built (+3 more)

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
Cohesion: 0.18
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 131 - "Knock, mechanical limits and damage"
Cohesion: 0.33
Nodes (6): Knock, mechanical limits and damage, Stage 085 — End-gas and knock physical model, Stage 086 — Knock-limited ECU, audible knock and feedback, Stage 087 — Piston speed, valve float and mechanical limits, Stage 088 — Optional damage state, warnings and reset behavior, Stage 089 — Knock and damage presentation effects

### Community 132 - "Expanded engine and exhaust catalog"
Cohesion: 0.33
Nodes (6): Expanded engine and exhaust catalog, Stage 090 — Additional inline and single-cylinder engines, Stage 091 — 60-degree and 90-degree V6 engines, Stage 092 — V10 and V12 engines, Stage 093 — Boxer-six engine, Stage 094 — Complete engine-specific exhaust and catalog calibration

### Community 133 - "rb_chamber_init"
Cohesion: 0.33
Nodes (12): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_integrate_many(), rb_chamber_set_volume(), rb_chamber_update(), remix() (+4 more)

### Community 134 - "titles"
Cohesion: 0.24
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 135 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 136 - "Playable browser bench"
Cohesion: 0.40
Nodes (5): Playable browser bench, Stage 066 — Playable browser engine and audio lifecycle, Stage 067 — Engine, intake and exhaust control panel, Stage 068 — Live instruments, engine telemetry and native-owned dyno, Stage 069 — First playable browser qualification

### Community 137 - "rb_pipe_integrate"
Cohesion: 0.37
Nodes (13): rb_chamber_clear_rates(), slow_flows(), fast_cells(), rb_pipe_clear(), rb_pipe_flows(), rb_pipe_integrate(), pipes_pulse(), test_exhaust_coherent_x_modes() (+5 more)

### Community 138 - "measure"
Cohesion: 0.23
Nodes (8): a_weighting(), as_db(), measure(), thirds(), deceleration_report(), main(), record(), sha()

### Community 139 - "Conditional learned-synthesis research"
Cohesion: 0.67
Nodes (3): Conditional learned-synthesis research, Stage 064 — Standalone learned pulse and DSP-parameter benchmark, Stage 065 — Conditional learned unresolved-source integration

### Community 140 - "acoustic_transport_probe.c"
Cohesion: 0.35
Nodes (9): excite(), init(), main(), measure(), packet(), release(), run(), seconds() (+1 more)

### Community 141 - "test_cylinder.c"
Cohesion: 0.29
Nodes (8): compress(), engine_86x86(), isentrope_T(), test_big_volume_jump_is_split(), test_compression_converges_with_step_size(), test_compression_matches_isentrope(), test_cylinder_bad_input(), test_motoring_has_no_energy_drift()

### Community 142 - "rb_cylinder_init"
Cohesion: 0.56
Nodes (7): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), test_cylinder_geometry()

### Community 143 - "Stage 5 — Valvetrain, throttle & intake plenum"
Cohesion: 0.22
Nodes (9): Equations used, How one step works, Notes for later stages, Results, Sources, Stage 5 — Valvetrain, throttle & intake plenum, Try it, Two things that surprised me (+1 more)

### Community 144 - "Stage 6 — Combustion + heat transfer"
Cohesion: 0.22
Nodes (9): Checked against chemical equilibrium, Equations used, How one step works now, Notes for later stages, Sources, Stage 6 — Combustion + heat transfer, Three things that surprised me, Try it (+1 more)

### Community 145 - "plot_run.py"
Cohesion: 0.33
Nodes (5): first(), main(), plot(), spans(), window()

### Community 146 - "rb_bench.c"
Cohesion: 0.67
Nodes (6): engine_cost(), main(), mixture_ns(), newton_ns(), now(), nozzle_ns()

### Community 147 - "S2d — first native acoustic-renderer candidates"
Cohesion: 0.33
Nodes (6): Implemented modules and contracts, Matrix and numerical results, Outcome, Reproduction and research, S2d — first native acoustic-renderer candidates, Validation, publication and remaining work

### Community 149 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 850 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **27 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `rb_engine_step`, `rb_chamber_init`, `rb_cli.c`, `rb_pipe_integrate`, `RbExports`, `audio.c`, `gas.c`, `rb_cylinder_move`, `Stage 13 — Performance & stability pass`, `rb_engine_init_at`, `REV-BENCH — notes for Claude`, `string`, `test_pipe.c`, `rb_engine_desc_default`, `rb_create`, `rb_network_step`, `math`, `rb_engine_init`, `rb_chamber_gamma`?**
  _High betweenness centrality (0.148) - this node is a cross-community bridge._
- **Are the 83 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 83 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.1111111111111111 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `REV-BENCH — notes for Claude`, `Stage 8 — Engine layouts & presets`, `RbSim`, `engine-node.ts`, `test_engine.c`?**
  _High betweenness centrality (0.138) - this node is a cross-community bridge._
- **Are the 53 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 53 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.10365853658536585 - nodes in this community are weakly interconnected._