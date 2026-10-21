# Graph Report - rev-bench  (2026-10-21)

## Corpus Check
- 131 files · ~459,806 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 10 file(s) not represented in the graph (top: (none) 5, .csv 2, .css 1)

## Summary
- 1596 nodes · 4396 edges · 68 communities (55 shown, 13 thin omitted)
- Extraction: 71% EXTRACTED · 29% INFERRED · 0% AMBIGUOUS · INFERRED: 1254 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `62167948`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- rb_engine_step
- rb_cylinder_move
- rb_engine_step
- pipe.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- math
- main.ts
- Stage 12 — Mufflers, the intake and the listener 🎧
- package.json
- style
- RbExports
- compare_real.py
- RbSim
- style
- label
- test_noise.c
- .process
- presets.c
- RbCore
- throttle.c
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
- test_audio.c
- rb_engine_init
- Stage 8 — Engine layouts & presets
- prepare_user_references.py
- rb_decimator_init
- REV-BENCH — Research notes (stage 0)
- REV-BENCH — notes for Claude
- Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Stage 3 — Gas & flow primitives
- Phase 4 — 3D engine view
- Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet
- REV-BENCH — Plan of Action
- Phase 2 — Sound (the realistic part)
- Phase 2b — Sound realism (top priority) 🎧
- Phase 6 — Polish & ship
- REV-BENCH
- bench_wasm.mjs
- Stages
- Bernardo's engine sound examples — 2026-10-21
- graphify-guard.sh
- session-start.sh
- Stage S1a — Reliable sound analysis before further tuning
- The workspace
- REV-BENCH - Codex handoff
- Sound realism (October 2026, after stage 12)
- Stage 13 — Performance & stability pass
- sys
- Stage S1c — Reproducible sound and combustion baselines
- AGENTS.md

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 96 edges
2. `rb_engine_init()` - 51 edges
3. `RbExports` - 51 edges
4. `Core (C)` - 41 edges
5. `rb_engine_init_at()` - 36 edges
6. `rb_engine_settle()` - 36 edges
7. `style()` - 33 edges
8. `titles()` - 33 edges
9. `rb_engine_desc_default()` - 29 edges
10. `rb_create()` - 29 edges

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

## Communities (68 total, 13 thin omitted)

### Community 0 - "rb_engine_step"
Cohesion: 0.15
Nodes (31): rb_engine_cylinder_angle(), rb_engine_step(), rb_cycle_offset(), rb_valve_area(), rb_valve_check(), rb_valve_closes(), rb_valve_lift(), rb_valve_opens() (+23 more)

### Community 1 - "rb_cylinder_move"
Cohesion: 0.05
Nodes (49): rb_cyl_dvolume(), rb_cyl_geom_init(), rb_cyl_piston_dx(), rb_cyl_piston_x(), rb_cyl_volume(), rb_cylinder_init(), rb_cylinder_move(), rb_wall_area() (+41 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.10
Nodes (19): Calibration, Cost, No two cycles alike (`core/src/engine.c`, `core/src/rand.h`), Notes for later stages, Sources, Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧, Tests that can fail, Tests that changed (+11 more)

### Community 3 - "pipe.c"
Cohesion: 0.06
Nodes (99): derive(), normalise(), rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_gamma(), rb_chamber_init() (+91 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.08
Nodes (62): rb_preset_count(), rb_preset_desc(), describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count() (+54 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.12
Nodes (46): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), rb_engine_desc_check(), rb_engine_displacement(), rb_preset_key(), rb_preset_name() (+38 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.08
Nodes (40): EngineDescription, PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER, RB_EF_INTEGER, RB_EF_LIVE (+32 more)

### Community 7 - "math"
Cohesion: 0.06
Nodes (40): Dither, The fuzzer, check_chamber(), check_engine(), fail(), seed_state(), uniform(), add() (+32 more)

### Community 8 - "main.ts"
Cohesion: 0.08
Nodes (31): EngineOptions, RevBenchNode, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, CoreInfo, ParamInfo (+23 more)

### Community 9 - "Stage 12 — Mufflers, the intake and the listener 🎧"
Cohesion: 0.07
Nodes (27): A muffler is a pipe, Calibration, Cost, Equal and unequal headers, Every source at its own distance, Going straight on, Packing, Review of stage 11 (+19 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "style"
Cohesion: 0.07
Nodes (44): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep(), main(), textbook_line() (+36 more)

### Community 13 - "compare_real.py"
Cohesion: 0.09
Nodes (18): read_wav(), a_weighting(), as_db(), band(), chart(), compare(), fetch(), main() (+10 more)

### Community 14 - "RbSim"
Cohesion: 0.08
Nodes (3): Notes for later stages, What was built, RbSim

### Community 15 - "style"
Cohesion: 0.19
Nodes (15): What was built, a_weighting(), bands(), check(), highpass(), levels(), levels_chart(), main() (+7 more)

### Community 16 - "label"
Cohesion: 0.23
Nodes (9): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+1 more)

### Community 17 - "test_noise.c"
Cohesion: 0.10
Nodes (37): band_init(), band_step(), engine_noise(), flow_noise(), impact(), inner_jet(), one_pole(), passes() (+29 more)

### Community 18 - ".process"
Cohesion: 0.15
Nodes (7): Equations used, Results, Sources, Stage 1 — Native C scaffold, WAV writer, test runner, What was built, RevBenchProcessor, renderSeconds()

### Community 19 - "presets.c"
Cohesion: 0.46
Nodes (13): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+5 more)

### Community 21 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "Stage S1b — Finite-time partial burns"
Cohesion: 0.25
Nodes (7): Completed before/after audit, Equations and behavior, Remaining work, Sound check and listening checkpoint, Sources and limits, Stage S1b — Finite-time partial burns, Why no fixed residual cutoff

### Community 24 - "Stage S1d — Burn timing follows the trapped mixture"
Cohesion: 0.25
Nodes (7): Change and limits, Idle and work variation, Matched full dyno sweeps, Remaining S1 work, Results, Sound and startup headroom, Stage S1d — Burn timing follows the trapped mixture

### Community 25 - "math"
Cohesion: 0.11
Nodes (34): poly(), rb_gas_cp(), rb_gas_cv(), rb_gas_gamma(), rb_gas_h(), rb_gas_init(), rb_gas_T_cv_from_u(), rb_gas_T_from_u() (+26 more)

### Community 26 - "Stage 10 — 1D pipes: runners, headers, collectors, tailpipes"
Cohesion: 0.07
Nodes (26): Against the real engines, At 96 kHz, Checks, Light load, Notes for later stages, Results, Review of stage 9, Sources (+18 more)

### Community 28 - "test_engine.c"
Cohesion: 0.13
Nodes (42): rb_engine_settle(), Results, Tests that changed, Tests that changed, ambient_density(), crank_books(), energy_books(), run_cycles() (+34 more)

### Community 29 - "Stage 9 — Dyno + validation against real engines"
Cohesion: 0.09
Nodes (21): Against the real engines, Calibration, Changes go the right way, Friction, Notes for later stages, Results, Review of stage 8, Sources (+13 more)

### Community 30 - "engine.c"
Cohesion: 0.10
Nodes (43): Core (C), rb_burnable_fuel(), rb_fuel_heat(), rb_wiebe(), accessory_torque(), add_share(), after_firing_tdc(), burn_duration() (+35 more)

### Community 31 - "Stage 11 — The sound: a microphone and the first WAV renders 🎧"
Cohesion: 0.10
Nodes (20): A pipe end is a monopole, Cost, DC blocker and soft limiter, Down to the audio rate, Notes for later stages, Review of stage 10, Sources, Stage 11 — The sound: a microphone and the first WAV renders 🎧 (+12 more)

### Community 32 - "Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧"
Cohesion: 0.09
Nodes (21): Calibration, Cost, Modal synthesis, Review of stage 13, Sources, Stage 12b — The flow's noise, the engine's own sound, and the cabin 🎧, Tests that changed, The cabin (+13 more)

### Community 33 - "test_audio.c"
Cohesion: 0.05
Nodes (97): arriving(), bessel_i0(), cabin(), cabin_design(), distance(), noise_arriving(), rb_dc_blocker_init(), rb_dc_blocker_reset() (+89 more)

### Community 34 - "rb_engine_init"
Cohesion: 0.17
Nodes (19): muffler_width(), rb_engine_desc_default(), rb_engine_init(), rb_engine_layout(), rb_engine_layout_inline(), tdc_angle(), test_audio_intake_mouth(), best_spark() (+11 more)

### Community 35 - "Stage 8 — Engine layouts & presets"
Cohesion: 0.20
Nodes (9): Idle tuning, Results, Review of stage 7, Sources, Stage 8 — Engine layouts & presets, The layout, The presets, Three things that surprised me (+1 more)

### Community 36 - "prepare_user_references.py"
Cohesion: 0.11
Nodes (15): core_fingerprint(), diagnostics(), main(), measurements(), record(), sha256(), validate_audio(), encode_listening() (+7 more)

### Community 37 - "rb_decimator_init"
Cohesion: 0.50
Nodes (4): Phase 5 — Forced induction & realism, Stage 20 — Turbocharger physics, Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧, Stage 22 — Knock, mechanical limits & engine damage

### Community 38 - "REV-BENCH — Research notes (stage 0)"
Cohesion: 0.33
Nodes (6): 3D in the browser (for stages 17–19), Bernardo's machine (checked in stage 0), Browser audio, Hosting (static site, free), How existing engine simulators make sound, REV-BENCH — Research notes (stage 0)

### Community 39 - "REV-BENCH — notes for Claude"
Cohesion: 0.18
Nodes (10): Bernardo, Claude's cloud sessions, Environment (device bridge), graphify, REV-BENCH — notes for Claude, Sound: Bernardo's top priority, The graph in this repo, Tools and charts (+2 more)

### Community 41 - "Stage 7 — Crankshaft dynamics, multi-cylinder, self-running"
Cohesion: 0.20
Nodes (9): Equations used, Notes for later stages, Results, Review of stage 6, Sources, Stage 7 — Crankshaft dynamics, multi-cylinder, self-running, Three things that surprised me, Try it (+1 more)

### Community 42 - "Stage 3 — Gas & flow primitives"
Cohesion: 0.25
Nodes (7): Equations used, Notes for later stages, Results, Sources, Stage 3 — Gas & flow primitives, The gases, What was built

### Community 43 - "Phase 4 — 3D engine view"
Cohesion: 0.25
Nodes (8): Phase 0 — Foundations, Phase 4 — 3D engine view, Stage 17 — 3D engine model: block + moving parts, Stage 18 — 3D engine in motion, in sync with the sound, Stage 19 — 3D gas, combustion & exhaust flow, Stage 1 — Native C scaffold, WAV writer, test runner, Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet, Stages

### Community 44 - "Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet"
Cohesion: 0.14
Nodes (9): Equations used, How the sound gets out, Notes for later stages, Results, Sources, Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet, What was built, makeImports() (+1 more)

### Community 45 - "REV-BENCH — Plan of Action"
Cohesion: 0.29
Nodes (4): Architecture (decided up front), Progress tracker, REV-BENCH — Plan of Action, Working agreement (for every stage)

### Community 46 - "Phase 2 — Sound (the realistic part)"
Cohesion: 0.29
Nodes (7): Phase 2 — Sound (the realistic part), Stage 10 — 1D exhaust pipes, Stage 11 — Audio extraction → first WAV renders 🎧, Stage 12 — Mufflers, intake & listener 🎧, Stage 12b — Flow noise, the engine's own sound & the cabin 🎧 (after stage 13), Stage 12c — Rough, not a piano note: variation, the limiter, the deceleration 🎧 (after 12b), Stage 13 — Performance & stability pass

### Community 47 - "Phase 2b — Sound realism (top priority) 🎧"
Cohesion: 0.33
Nodes (6): Phase 2b — Sound realism (top priority) 🎧, Stage 13b — Performance II: vectorized pipes, implicit valves, Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧, Stage S2 — Air: a finer exhaust 🎧, Stage S3 — The engine's own sound 🎧 (done in stage 12b), Stage S4 — Scenery: stereo and the place 🎧

### Community 48 - "Phase 6 — Polish & ship"
Cohesion: 0.33
Nodes (6): Phase 6 — Polish & ship, Stage 23 — More layouts & presets, Stage 24 — UX polish, save/share, quality settings, Stage 25 — Deploy, Stage 26 — Optional extras (one per prompt), Stage S5 — Tuning by ear 🎧 (once everything is built)

### Community 51 - "REV-BENCH"
Cohesion: 0.33
Nodes (6): Native tools (WSL / Linux), Project layout, Rebuilding the WebAssembly core, REV-BENCH, Sound-analysis checks, Try it in the browser

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
Cohesion: 0.33
Nodes (5): 2026-10-21 continuation: S1c and filtered references, 2026-10-21 continuation: S1d complete; S1 open, 2026-10-21: S1a, S1b and local references, Codex tools on this machine, REV-BENCH - Codex handoff

### Community 64 - "Sound realism (October 2026, after stage 12)"
Cohesion: 0.33
Nodes (6): After stage 12b: a piano note (stage 12c), How others get there, Measured against real engines, Sound realism (October 2026, after stage 12), The physics of what's missing, What this means for REV-BENCH

### Community 67 - "Stage 13 — Performance & stability pass"
Cohesion: 0.11
Nodes (17): Cost, Measuring, Notes for later stages, Review of stage 12, Sources, Stage 13 — Performance & stability pass, The chamber and the gas, The nozzle and the walls (+9 more)

### Community 72 - "sys"
Cohesion: 0.19
Nodes (7): load_table(), main(), run(), preset_of(), firing_panel(), gaps(), spacing()

### Community 73 - "Stage S1c — Reproducible sound and combustion baselines"
Cohesion: 0.33
Nodes (5): Float sound audit, Native cycle audit, Reference continuation and next S1 step, Results and validation, Stage S1c — Reproducible sound and combustion baselines

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 443 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **13 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Core (C)` connect `engine.c` to `test_audio.c`, `rb_engine_init`, `pipe.c`, `rb_cylinder_move`, `rb_cli.c`, `math`, `REV-BENCH — notes for Claude`, `RbExports`, `test_noise.c`, `math`, `test_engine.c`?**
  _High betweenness centrality (0.202) - this node is a cross-community bridge._
- **Are the 76 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 76 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `rb_cylinder_move` be split into smaller, more focused modules?**
  _Cohesion score 0.05357142857142857 - nodes in this community are weakly interconnected._
- **Why does `RbExports` connect `RbExports` to `rb-core.ts`, `Stage 2 — Web scaffold: C → WebAssembly → AudioWorklet`, `RbSim`, `.process`, `engine.c`?**
  _High betweenness centrality (0.169) - this node is a cross-community bridge._
- **Are the 47 inferred relationships involving `rb_engine_init()` (e.g. with `Try it` and `Three things that surprised me`) actually correct?**
  _`rb_engine_init()` has 47 INFERRED edges - model-reasoned connections that need verification._
- **Should `rb_engine_step` be split into smaller, more focused modules?**
  _Cohesion score 0.1 - nodes in this community are weakly interconnected._