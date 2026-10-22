# Graph Report - rev-bench  (2026-10-22)

## Corpus Check
- 169 files · ~566,689 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1842 nodes · 5027 edges · 77 communities (55 shown, 22 thin omitted)
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 1379 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `1d6c64a7`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_engine_step
- rb_cylinder_move
- rb_engine_step
- wav.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- Stage 11 — The sound: a microphone and the first WAV renders 🎧
- main.ts
- presets.c
- package.json
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- RbExports
- compare_real.py
- RbSim
- style
- style
- rb_engine_init
- Stage S1d — Burn timing follows the trapped mixture
- Stage S1b — Finite-time partial burns
- RbCore
- test_audio.c
- compilerOptions
- S4 — Separate sources, stereo listeners and acoustic scenes
- rb_test.h
- Stage 10 — 1D pipes: runners, headers, collectors, tailpipes
- AudioWorkletProcessor
- test_engine.c
- Stage 9 — Dyno + validation against real engines
- Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧
- audit_s1.py
- Stage 8 — Engine layouts & presets
- Stages
- REV-BENCH — Research notes (stage 0)
- engine-node.ts
- Stage 3 — Gas & flow primitives
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- throttle.c
- engine.c
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
- argparse
- math
- REV-BENCH — notes for Claude
- Stage S1c — Reproducible sound and combustion baselines
- rb_engine_desc_default
- AGENTS.md
- read_wav
- rb_engine_init
- checkpoint-13b.md
- REV-BENCH — Feedback checkpoint before stage 13b
- S2b — Loaded exhaust detail and the lift-off contrast
- Current sound results
- Stage 13b-r — high-RPM exhaust character

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 101 edges
2. `rb_engine_init()` - 56 edges
3. `RbExports` - 53 edges
4. `Core (C)` - 41 edges
5. `rb_engine_init_at()` - 39 edges
6. `style()` - 38 edges
7. `rb_engine_settle()` - 36 edges
8. `cmd_sound()` - 33 edges
9. `titles()` - 33 edges
10. `rb_chamber_init()` - 32 edges

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

## Communities (77 total, 22 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.15
Nodes (31): rb_engine_cylinder_angle(), rb_engine_step(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+23 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.05
Nodes (49): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+41 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "wav.c"
Cohesion: 0.15
Nodes (27): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+19 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.07
Nodes (66): rb_preset_count(), rb_preset_key(), rb_preset_name(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit() (+58 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.12
Nodes (42): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), csv_close(), csv_open(), csv_row(), cmd_blowdown() (+34 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (46): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+38 more)

### Community 7 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.09
Nodes (21): A pipe end is a monopole, Cost, DC blocker and soft limiter, Dither, Down to the audio rate, Notes for later stages, Review of stage 10, Sources (+13 more)

### Community 8 - "main.ts"
Cohesion: 0.09
Nodes (26): RevBenchNode, CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam() (+18 more)

### Community 9 - "presets.c"
Cohesion: 0.08
Nodes (42): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+34 more)

### Community 10 - "package.json"
Cohesion: 0.05
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 13 - "compare_real.py"
Cohesion: 0.10
Nodes (19): a_weighting(), as_db(), band(), compare(), fetch(), main(), measure(), read_list() (+11 more)

### Community 15 - "style"
Cohesion: 0.18
Nodes (15): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+7 more)

### Community 16 - "style"
Cohesion: 0.06
Nodes (51): chart(), describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main() (+43 more)

### Community 17 - "rb_engine_init"
Cohesion: 0.06
Nodes (55): rb_exhaust_model(), band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole() (+47 more)

### Community 18 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.22
Nodes (8): Bernardo's listening feedback — 2026-10-21, Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 19 - "Stage S1b — Finite-time partial burns"
Cohesion: 0.25
Nodes (7): Completed before/after audit, Equations and behavior, Remaining work, Sound check and listening checkpoint, Sources and limits, Stage S1b — Finite-time partial burns, Why no fixed residual cutoff

### Community 21 - "test_audio.c"
Cohesion: 0.05
Nodes (98): arriving(), bessel_i0(), cabin_design(), distance(), noise_arriving(), rb_cabin_process(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+90 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "S4 — Separate sources, stereo listeners and acoustic scenes"
Cohesion: 0.40
Nodes (5): Geometry and approximations, Pass-by and room decay, S4 — Separate sources, stereo listeners and acoustic scenes, Signal path, Validation and use

### Community 25 - "rb_test.h"
Cohesion: 0.08
Nodes (41): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_from_u() (+33 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.11
Nodes (47): Core (C), rb_burnable_fuel(), rb_fuel_heat(), rb_wiebe(), rb_engine_settle(), Results, Tests that can fail, Tests that changed (+39 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.05
Nodes (39): Calibration, Cost, Modal synthesis, Notes for later stages, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, Tests that changed (+31 more)

### Community 33 - "audit_s1.py"
Cohesion: 0.08
Nodes (19): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), load_table() (+11 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.18
Nodes (10): Idle tuning, Notes for later stages, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets (+2 more)

### Community 37 - "Stages"
Cohesion: 0.05
Nodes (40): Architecture (decided up front), Latest listening feedback — 2026-10-22, Phase 0 — Foundations, Phase 2 — Sound (the realistic part), Phase 2b — Sound realism (top priority) 🎧, Phase 3 — Browser, Phase 4 — 3D engine view, Phase 5 — Forced induction & realism (+32 more)

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.18
Nodes (11): 2026-10-21 — Deceleration and spatial sound continuation, 2026-10-21 — Lift-off contrast and loaded pipe transport (S2b), 2026-10-22 — model-specific exhausts and outlet loading, 2026-10-22 — Preserve realism while batching pipe thermodynamics (13b), 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free) (+3 more)

### Community 39 - "engine-node.ts"
Cohesion: 0.44
Nodes (5): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.14
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 45 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 47 - "engine.c"
Cohesion: 0.12
Nodes (39): accessory_torque(), add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn() (+31 more)

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
Cohesion: 0.14
Nodes (13): 2026-10-21: consolidated feedback checkpoint before 13b, 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21 listening feedback and resumed development, 2026-10-21: S1a, S1b and local references, 2026-10-21: S2b — lift-off contrast and cross-plane redline, 2026-10-22: full 13b and model-specific exhaust work resumed, 2026-10-22: researched exhaust implementation and listening checkpoint (+5 more)

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 68 - "argparse"
Cohesion: 0.09
Nodes (25): analyze(), long_coast(), main(), record(), render(), sha(), write_float(), deceleration_report() (+17 more)

### Community 71 - "math"
Cohesion: 0.06
Nodes (106): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+98 more)

### Community 72 - "REV-BENCH — notes for Claude"
Cohesion: 0.11
Nodes (12): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+4 more)

### Community 73 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

### Community 74 - "rb_engine_desc_default"
Cohesion: 0.28
Nodes (12): rb_engine_desc_default(), Tests that changed, best_spark(), run_held(), test_calibration_burn_and_spark(), test_calibration_cams(), test_calibration_compression(), test_calibration_idle_spark() (+4 more)

### Community 78 - "read_wav"
Cohesion: 0.16
Nodes (6): read_wav(), read_wav_bytes(), read_wav(), AudioAnalysisTests, chunk(), wav_bytes()

### Community 79 - "rb_engine_init"
Cohesion: 0.15
Nodes (30): rb_engine_contents(), rb_engine_init(), rb_engine_pipe(), rb_engine_pipe_count(), rb_engine_pipe_heat(), slow_flows(), slow_step(), rb_preset_desc() (+22 more)

### Community 84 - "checkpoint-13b.md"
Cohesion: 0.15
Nodes (8): Model, Real references, S1e — Research and controlled overrun transitions, Validation and remaining work, Optional ultra quality, S2 — Additional flow sources and optional spatial resolution, Sources, Validation

### Community 85 - "REV-BENCH — Feedback checkpoint before stage 13b"
Cohesion: 0.29
Nodes (7): Acceptance that is still outstanding, Real references and other scenarios, Reproduction and feedback, REV-BENCH — Feedback checkpoint before stage 13b, Start with the longer deceleration, Validation and processing cost, What has progressed

### Community 91 - "S2b — Loaded exhaust detail and the lift-off contrast"
Cohesion: 0.29
Nodes (7): Cross-plane V8, Evidence and research, Measurement and listening, Model, Rejected controls, Render-helper error and validation, S2b — Loaded exhaust detail and the lift-off contrast

### Community 93 - "Current sound results"
Cohesion: 0.40
Nodes (5): Current sound results, Local layout and retention, One file to open, Produce the next checkpoint, Validation of the file-management change

### Community 96 - "Stage 13b-r — high-RPM exhaust character"
Cohesion: 0.29
Nodes (7): Deliverable and reproduction, Implemented geometry and limits, Measured sound and listening checkpoint, Numerical validation, Research and implementation choices, Rev limits and mechanical assumptions, Stage 13b-r — high-RPM exhaust character

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 546 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **22 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `test_engine.c` to `Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧`, `rb_cylinder_move`, `math`, `REV-BENCH — notes for Claude`, `rb_engine_desc_default`, `RbExports`, `engine.c`, `rb_engine_init`, `rb_engine_init`, `test_audio.c`, `rb_test.h`?**
  _High betweenness centrality (0.203) - this node is a cross-community bridge._
- **Are the 81 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 81 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.05357142857142857 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `REV-BENCH — notes for Claude`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `RbSim`, `test_engine.c`?**
  _High betweenness centrality (0.177) - this node is a cross-community bridge._
- **Are the 52 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 52 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.1 - nodes in this community are weakly interconnected._