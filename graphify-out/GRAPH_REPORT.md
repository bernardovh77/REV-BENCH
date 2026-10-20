# Graph Report - REV-BENCH  (2026-10-19)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 959 nodes · 2942 edges · 29 communities (22 shown, 7 thin omitted)
- Extraction: 73% EXTRACTED · 27% INFERRED · 0% AMBIGUOUS · INFERRED: 801 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `5059f246`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- test_audio.c
- math
- rb_engine_step
- pipe.c
- rb_engine_api.c
- rb_cli.c
- rb-core.ts
- wav.c
- main.ts
- presets.c
- package.json
- titles
- RbExports
- test_dyno.c
- RbSim
- style
- label
- plot_fire.py
- engine-node.ts
- plot_dyno.py
- RbCore
- matplotlib_pyplot
- compilerOptions
- throttle.c
- plot_breathe.py
- AudioWorkletProcessor
- .render

## God Nodes (most connected - your core abstractions)
1. `rb_engine_step()` - 73 edges
2. `rb_engine_init()` - 56 edges
3. `RbExports` - 49 edges
4. `titles()` - 30 edges
5. `style()` - 30 edges
6. `rb_engine_settle()` - 30 edges
7. `read_csv()` - 29 edges
8. `rb_engine_desc_default()` - 27 edges
9. `RbSim` - 26 edges
10. `label()` - 26 edges

## Surprising Connections (you probably didn't know these)
- `cmd_sound()` --calls--> `rb_listener_distance()`  [INFERRED]
  native/rb_cli.c → core/src/audio.c
- `test_core_host_helpers()` --calls--> `rb_alloc_floats()`  [INFERRED]
  tests/test_core.c → core/src/rb_alloc.c
- `cmd_sound()` --calls--> `rb_create()`  [INFERRED]
  native/rb_cli.c → core/src/rb_alloc.c
- `cmd_tone()` --calls--> `rb_create()`  [INFERRED]
  native/rb_cli.c → core/src/rb_alloc.c
- `print_engine()` --calls--> `rb_create()`  [INFERRED]
  native/rb_cli.c → core/src/rb_alloc.c

## Import Cycles
- None detected.

## Communities (29 total, 7 thin omitted)

### Community 0 - "test_audio.c"
Cohesion: 0.05
Nodes (88): arriving(), bessel_i0(), distance(), rb_dc_blocker_init(), rb_dc_blocker_reset(), rb_dc_blocker_step(), rb_decimator_init(), rb_decimator_out() (+80 more)

### Community 1 - "math"
Cohesion: 0.05
Nodes (69): derive(), normalise(), rb_chamber_init(), rb_chamber_integrate(), rb_chamber_set_volume(), rb_chamber_update(), remix(), volume_step() (+61 more)

### Community 2 - "rb_engine_step"
Cohesion: 0.07
Nodes (86): add_share(), after_firing_tdc(), burn_duration(), burn_step(), close_intake(), computer_spark(), crank_turn(), cycle_angle() (+78 more)

### Community 3 - "pipe.c"
Cohesion: 0.10
Nodes (63): rb_chamber_add_flow(), rb_chamber_add_heat(), rb_chamber_add_mass(), rb_chamber_clear_rates(), rb_chamber_init_reservoir(), rb_chamber_pressure_now(), cell_heat(), cell_is_fast() (+55 more)

### Community 4 - "rb_engine_api.c"
Cohesion: 0.10
Nodes (42): describe_pipe(), field_ok(), rb_cylinder_field_count(), rb_cylinder_field_name(), rb_cylinder_field_unit(), rb_engine_field_count(), rb_engine_field_find(), rb_engine_field_flags() (+34 more)

### Community 5 - "rb_cli.c"
Cohesion: 0.16
Nodes (35): rb_engine_displacement(), rb_preset_name(), csv_close(), csv_open(), csv_row(), cmd_blowdown(), cmd_breathe(), cmd_dyno() (+27 more)

### Community 6 - "rb-core.ts"
Cohesion: 0.07
Nodes (40): EngineDescription, FieldInfo, makeImports(), PipeDescription, PresetInfo, RB_CLAMPED, RB_EF_ARRAY, RB_EF_IN_ORDER (+32 more)

### Community 7 - "wav.c"
Cohesion: 0.13
Nodes (27): build_header(), bytes_per_sample(), get_u16(), get_u32(), parse_wav(), put_u16(), put_u32(), uniform() (+19 more)

### Community 8 - "main.ts"
Cohesion: 0.09
Nodes (26): RevBenchNode, CoreInfo, ParamInfo, boot(), clearReadouts(), drawScope(), errorText(), findParam() (+18 more)

### Community 9 - "presets.c"
Cohesion: 0.18
Nodes (28): airbox(), boxer4(), exhaust(), inline4(), inline6(), intake(), layout(), muffler() (+20 more)

### Community 10 - "package.json"
Cohesion: 0.06
Nodes (26): @playwright/test, @types/node, typescript, vite, vitest, description, devDependencies, @playwright/test (+18 more)

### Community 11 - "titles"
Cohesion: 0.13
Nodes (11): main(), textbook_line(), plot_spark(), first(), main(), plot(), spans(), window() (+3 more)

### Community 13 - "test_dyno.c"
Cohesion: 0.18
Nodes (12): mark(), rb_dyno_init(), rb_dyno_step(), rb_dyno_sweep(), best_spark(), run_held(), test_calibration_cams(), test_calibration_compression() (+4 more)

### Community 15 - "style"
Cohesion: 0.20
Nodes (13): a_weighting(), check(), highpass(), levels(), levels_chart(), main(), orders(), read_wav() (+5 more)

### Community 16 - "label"
Cohesion: 0.21
Nodes (10): colorbar(), compare(), engine(), grid(), main(), pulse(), runs(), save() (+2 more)

### Community 17 - "plot_fire.py"
Cohesion: 0.17
Nodes (10): at_volume(), degrees(), half_cycle_axis(), imep(), log_axis(), main(), plot_cycle(), plot_power() (+2 more)

### Community 18 - "engine-node.ts"
Cohesion: 0.20
Nodes (6): EngineOptions, FromWorklet, PROCESSOR_NAME, ProcessorOptions, ToWorklet, RevBenchProcessor

### Community 19 - "plot_dyno.py"
Cohesion: 0.22
Nodes (10): curve_panel(), end_labels(), main(), peak(), plot_changes(), plot_curves(), plot_spark(), preset_of() (+2 more)

### Community 21 - "matplotlib_pyplot"
Cohesion: 0.25
Nodes (9): between(), firing_panel(), gaps(), main(), plot_firing(), plot_idle(), plot_runs(), preset_of() (+1 more)

### Community 22 - "compilerOptions"
Cohesion: 0.12
Nodes (15): compilerOptions, isolatedModules, lib, module, moduleResolution, noEmit, noFallthroughCasesInSwitch, noUnusedLocals (+7 more)

### Community 23 - "throttle.c"
Cohesion: 0.31
Nodes (9): rb_throttle_angle(), rb_throttle_area(), rb_throttle_check(), rb_throttle_open_area(), strip_in_ellipse(), body(), sliced_area(), test_throttle_geometry() (+1 more)

### Community 24 - "plot_breathe.py"
Cohesion: 0.24
Nodes (6): describe(), log_axis(), main(), plot_cycle(), plot_map(), pumping_mep()

## Knowledge Gaps
- **3 isolated node(s):** `@types/node`, `typescript`, `vite`
  These have ≤1 connection - possible missing edges. (Counts symbols only; 138 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **7 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `main()` connect `titles` to `math`, `style`, `label`, `plot_fire.py`, `matplotlib_pyplot`?**
  _High betweenness centrality (0.075) - this node is a cross-community bridge._
- **Are the 54 inferred relationships involving `rb_engine_step()` (e.g. with `rb_chamber_add_heat()` and `rb_chamber_clear_rates()`) actually correct?**
  _`rb_engine_step()` has 54 INFERRED edges - model-reasoned connections that need verification._
- **What connects `@types/node`, `typescript`, `vite` to the rest of the system?**
  _3 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `test_audio.c` be split into smaller, more focused modules?**
  _Cohesion score 0.0548790658882402 - nodes in this community are weakly interconnected._
- **Why does `rb_engine_step()` connect `rb_engine_step` to `test_audio.c`, `math`, `pipe.c`, `rb_engine_api.c`, `rb_cli.c`, `presets.c`, `test_dyno.c`, `throttle.c`?**
  _High betweenness centrality (0.050) - this node is a cross-community bridge._
- **Are the 46 inferred relationships involving `rb_engine_init()` (e.g. with `rb_chamber_init()` and `rb_chamber_init_reservoir()`) actually correct?**
  _`rb_engine_init()` has 46 INFERRED edges - model-reasoned connections that need verification._
- **Should `math` be split into smaller, more focused modules?**
  _Cohesion score 0.05380333951762523 - nodes in this community are weakly interconnected._