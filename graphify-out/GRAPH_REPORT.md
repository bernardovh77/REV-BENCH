# Graph Report - rev-bench  (2026-10-21)

## Corpus Check
- 143 files · ~473,353 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1671 nodes · 4565 edges · 87 communities (69 shown, 18 thin omitted)
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 1288 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `f6ec673e`
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
- test_noise.c
- cmd_info
- throttle.c
- RbCore
- cmd_sound
- compilerOptions
- Stage S1b — Finite-time partial burns
- Stage S1d — Burn timing follows the trapped mixture
- math
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- engine.c
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- audio.c
- rb_engine_init
- Stage 8 — Engine layouts & presets
- prepare_user_references.py
- rb_decimator_init
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- rb_engine_desc_check
- Stage 3 — Gas & flow primitives
- Phase 4 — 3D engine view
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- Phase 2 — Sound (the realistic part)
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- rb_destroy
- REV-BENCH
- bench_wasm.mjs
- argparse
- Stages
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)
- rb_fuzz_run
- RevBenchProcessor
- audit_s1.py
- sys
- style
- numpy
- subprocess
- Stage S1c — Reproducible sound and combustion baselines
- rb_engine_init
- AGENTS.md
- rb_create
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- read_wav
- test_overrun.c
- Results
- setup
- checkpoint_13b.py
- REV-BENCH — Feedback checkpoint before stage 13b

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 98 edges
2. `rb_engine_init()` - 53 edges
3. `RbExports` - 53 edges
4. `Core (C)` - 41 edges
5. `rb_engine_init_at()` - 36 edges
6. `rb_engine_settle()` - 36 edges
7. `style()` - 33 edges
8. `titles()` - 33 edges
9. `cmd_sound()` - 31 edges
10. `rb_engine_desc_default()` - 30 edges

## Surprising Connections (you probably didn't know these)
- `Equations used` --references--> `rb_chamber_set_volume()`  [INFERRED]
  docs/stage-04.md → core/src/chamber.c
- `Stage 4 — Single cylinder: slider-crank + motoring` --references--> `rb_chamber_set_volume()`  [INFERRED]
  PLAN.md → core/src/chamber.c
- `Unequal headers (`core/src/presets.c`)` --references--> `rb_engine_desc_default()`  [INFERRED]
  docs/stage-12c.md → core/src/engine.c
- `Stage 8 — Engine layouts & presets` --references--> `rb_engine_layout()`  [INFERRED]
  PLAN.md → core/src/engine.c
- `The layout` --references--> `rb_engine_desc_check()`  [INFERRED]
  docs/stage-08.md → core/src/engine.c

## Import Cycles
- None detected.

## Communities (87 total, 18 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.15
Nodes (33): rb_engine_cylinder_angle(), rb_engine_step(), rb_get_cylinder(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift() (+25 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.06
Nodes (46): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+38 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "math"
Cohesion: 0.06
Nodes (92): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_init(), rb_chamber_init_reservoir() (+84 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.17
Nodes (28): describe_pipe(), field_ok(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags(), rb_engine_field_max(), rb_engine_field_min(), rb_engine_field_name() (+20 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.09
Nodes (49): rb_chamber_gamma(), rb_engine_displacement(), rb_preset_key(), rb_preset_name(), csv_close(), csv_open(), csv_row(), add() (+41 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.08
Nodes (41): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+33 more)

### Community 7 - "wav.c"
Cohesion: 0.20
Nodes (25): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+17 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "Stage 12 — Mufflers, the intake and the listener 🎧"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "read_csv"
Cohesion: 0.14
Nodes (10): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+2 more)

### Community 13 - "compare_real.py"
Cohesion: 0.16
Nodes (10): a_weighting(), as_db(), band(), compare(), fetch(), main(), measure(), read_list() (+2 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (16): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+8 more)

### Community 16 - "label"
Cohesion: 0.21
Nodes (11): chart(), plot_spark(), colorbar(), compare(), engine(), grid(), main(), pulse() (+3 more)

### Community 17 - "test_noise.c"
Cohesion: 0.10
Nodes (38): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+30 more)

### Community 18 - "cmd_info"
Cohesion: 0.26
Nodes (18): rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), param_ok(), rb_get_param(), rb_param_count(), rb_param_default(), rb_param_find() (+10 more)

### Community 19 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 21 - "cmd_sound"
Cohesion: 0.17
Nodes (25): rb_soft_limit(), config_is_valid(), model_step(), rb_config_default(), rb_config_quality(), rb_init(), rb_onepole_coef(), rb_oversample() (+17 more)

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
Nodes (52): rb_fuel_heat(), rb_wiebe(), poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init() (+44 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.09
Nodes (21): Checks, Light load, Notes for later stages, Sources, Stage 10 — 1D pipes: runners, headers, collectors, tailpipes, The ends, The engine computer looks ahead, The engine is unstable on its own (+13 more)

### Community 28 - "test_engine.c"
Cohesion: 0.12
Nodes (44): rb_engine_settle(), Results, Tests that changed, Tests that can fail, Tests that changed, ambient_density(), crank_books(), energy_books() (+36 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "engine.c"
Cohesion: 0.12
Nodes (39): Core (C), rb_burnable_fuel(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark() (+31 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.09
Nodes (21): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+13 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.05
Nodes (40): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, Tests that changed (+32 more)

### Community 33 - "audio.c"
Cohesion: 0.12
Nodes (32): arriving(), bessel_i0(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+24 more)

### Community 34 - "rb_engine_init"
Cohesion: 0.23
Nodes (15): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_default(), best_spark(), run_held(), test_calibration_burn_and_spark() (+7 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.13
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 36 - "prepare_user_references.py"
Cohesion: 0.18
Nodes (7): encode_listening(), main(), prepare(), prepare_filtered(), run(), sha256(), ReferenceFilteringTests

### Community 37 - "rb_decimator_init"
Cohesion: 0.29
Nodes (7): Phase 0 — Foundations, Phase 5 — Forced induction & realism, Stage 20 — Turbocharger physics, Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧, Stage 22 — Knock, mechanical limits & engine damage, Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet, Stages

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.29
Nodes (7): 2026-10-21 — Deceleration and spatial sound continuation, 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound, REV-BENCH — Research notes (stage 0)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.10
Nodes (16): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+8 more)

### Community 41 - "rb_engine_desc_check"
Cohesion: 0.27
Nodes (12): accessory_torque(), muffler_width(), rb_engine_desc_check(), rb_engine_fmep(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), What was built (+4 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "Phase 4 — 3D engine view"
Cohesion: 0.50
Nodes (4): Phase 4 — 3D engine view, Stage 17 — 3D engine model: block + moving parts, Stage 18 — 3D engine in motion, in sync with the sound, Stage 19 — 3D gas, combustion & exhaust flow

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.20
Nodes (8): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports()

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.25
Nodes (5): Architecture (decided up front), Latest listening feedback — 2026-10-21, Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 47 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.33
Nodes (6): Phase 2b — Sound realism (top priority) 🎧, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 48 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 50 - "rb_destroy"
Cohesion: 0.17
Nodes (27): rb_destroy(), rb_set_param(), rb_get_telemetry(), rb_render_block(), Tests that can fail, Tests that changed, Review of stage 12, amplitude() (+19 more)

### Community 51 - "REV-BENCH"
Cohesion: 0.33
Nodes (6): Native tools (WSL / Linux), Project layout, Rebuilding the WebAssembly core, REV-BENCH, Sound-analysis checks, Try it in the browser

### Community 53 - "argparse"
Cohesion: 0.16
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 54 - "Stages"
Cohesion: 0.50
Nodes (4): Phase 3 — Browser, Stage 14 — Real-time engine in the browser 🎮, Stage 15 — Control panel, Stage 16 — Gauges + live dyno chart

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
Cohesion: 0.25
Nodes (7): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, Codex tools on this machine, REV-BENCH - Codex handoff

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 66 - "rb_fuzz_run"
Cohesion: 0.35
Nodes (7): check_chamber(), check_engine(), fail(), rb_fuzz_run(), repairs(), seed_state(), uniform()

### Community 68 - "audit_s1.py"
Cohesion: 0.20
Nodes (8): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), S1AuditTests

### Community 69 - "sys"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 70 - "style"
Cohesion: 0.23
Nodes (10): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+2 more)

### Community 71 - "numpy"
Cohesion: 0.18
Nodes (4): read_wav(), main(), orders(), chunk()

### Community 72 - "subprocess"
Cohesion: 0.39
Nodes (3): load_table(), main(), run()

### Community 73 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 74 - "rb_engine_init"
Cohesion: 0.18
Nodes (26): rb_engine_init(), rb_preset_count(), rb_preset_desc(), rb_preset_find(), Tests that can fail, band_power(), order_amplitude(), record_tailpipes() (+18 more)

### Community 76 - "rb_create"
Cohesion: 0.36
Nodes (7): rb_alloc_floats(), rb_create(), rb_create_with(), rb_create_with_quality(), rb_free_floats(), Stage 1 — Native C scaffold, WAV writer, test runner, test_core_host_helpers()

### Community 77 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.22
Nodes (8): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it

### Community 78 - "read_wav"
Cohesion: 0.33
Nodes (3): read_wav(), AudioAnalysisTests, wav_bytes()

### Community 79 - "test_overrun.c"
Cohesion: 0.73
Nodes (5): at_rpm(), running(), test_overrun_reset_and_legacy(), test_overrun_retards_combustion(), test_overrun_transition()

### Community 80 - "Results"
Cohesion: 0.50
Nodes (4): Against the real engines, At 96 kHz, Results, What doesn't look like a real engine yet

### Community 82 - "setup"
Cohesion: 0.27
Nodes (6): first(), main(), plot(), spans(), window(), setup()

### Community 83 - "checkpoint_13b.py"
Cohesion: 0.44
Nodes (7): analyze(), long_coast(), main(), record(), render(), sha(), write_float()

### Community 84 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.08
Nodes (20): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed, Model (+12 more)

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 473 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **18 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `engine.c` to `Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧`, `audio.c`, `rb_cylinder_move`, `math`, `rb_engine_init`, `rb_cli.c`, `REV-BENCH — notes for Claude`, `rb_engine_desc_check`, `rb_engine_init`, `RbExports`, `test_noise.c`, `cmd_sound`, `math`, `test_engine.c`?**
  _High betweenness centrality (0.199) - this node is a cross-community bridge._
- **Are the 78 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 78 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.1475609756097561 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `Stage 8 — Engine layouts & presets`, `RevBenchProcessor`, `rb-core.ts`, `REV-BENCH — notes for Claude`, `RbSim`, `engine.c`?**
  _High betweenness centrality (0.163) - this node is a cross-community bridge._
- **Are the 49 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 49 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.06015037593984962 - nodes in this community are weakly interconnected._