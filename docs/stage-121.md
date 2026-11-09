# Stage 121 — The core's public engine-control, telemetry and snapshot API

**Status:** done (2026-11-08)

Everything a host needs to drive and watch the engine is now in `core/include/rb.h`
([specification](roadmap/07-app.md#stage-121)): the desktop app (stages 123–124) and
`rb_cli sound` use it, and nothing else. The physics is unchanged: every `rb_cli sound`
drive renders bit for bit what it rendered before, now that it sets parameters instead of
the engine's fields.

The work followed Bernardo's `tdd` skill (`.agents/skills/tdd`): seven seams, all of them
public, each with its tests written before its code. Slice 1's tests were written first
but not run until the code existed (they couldn't build); from slice 2 on each was seen
failing first. The seams, all through `rb.h` alone (`tests/test_api.c`, 14 tests):
1. **the driver's controls:** each one's name, unit, range, default, clamping and refusals,
   and what it does to the engine (the key starts it, the pedal revs it, a load slows it,
   held it turns at the held speed);
2. **the car and the clutch:** in gear the crank carries the car's inertia and the road's
   load; the clutch going in drops both at the same speed;
3. **settings:** applied at once, or `RB_PENDING` until the next rebuild; kept across
   rebuilds, while the controls go back to rest and the generation counts up; the quality;
4. **the dyno the sim owns:** hold, readings, clear, sweep, and off with a rebuild;
5. **snapshots:** layout, size and version refusals, values equal to the telemetry and
   `rb_get_cylinder` at the same moment, numbering, the build's identity; and the Python
   checker of the contract (`tools/test_snapshot_v2.py`);
6. **events:** in order and numbered without a gap, each cylinder's own order, the cuts,
   the ring's losses, cleared by a rebuild; and in order when several fall in one step;
7. **`rb_cli sound` on rb.h alone:** every drive bit-identical (below), and a build that
   can't see core/src.

## The API

- **The source:** `RB_PARAM_SOURCE`, the test tone (default, for the browser scaffold)
  or the engine.
- **The driver's controls**, parameters flagged `RB_PF_CONTROL`: ignition, starter,
  throttle, load torque, hold (the crank held by a motor, the default) and hold rpm, and
  the car: `gear_inertia` (the crank's inertia with the car in gear) and `clutch`. The
  car's road load (rolling resistance and drag) moved from rb_cli into the core
  (`rb_drive.c`); it's worked out before each rendered block, as rb_cli did.
- **The settings:** variation, listener, layers, overrun transition, events; the burn
  model, the seed and the pipes' momentum order are `RB_PF_REBUILD`: applied at once while
  the engine hasn't run, else `RB_PENDING` until the next rebuild. Switches are
  `RB_PF_INTEGER` (0.5 is refused). `rb_set_quality`/`rb_get_quality` rebuild the sim at
  another quality keeping everything else.
- **Rebuilds** all go through `rb_sim_build` (rb_reset, a preset, a field change, the
  quality): the engine at rest, the controls at their defaults (key off, crank held at
  0 rpm, no car), the settings reapplied, no dyno, no events, the listener placed, and
  `RB_TLM_GENERATION` one more. That's the reset recipe of contracts/runtime-v1.md. The
  description a host sees (`rb_get_engine`) never has the car's inertia in it.
- **Telemetry:** 24 engine values from `RB_TLM_RPM` on (rpm, MAP, torque, power, lambda,
  spark, idle valve, throttle, the cuts, the flows, the temperatures, ...), each with a
  unit (`rb_telemetry_unit`), plus the generation and the audio frames.
- **A dyno the sim owns:** `rb_dyno_attach`, `_release`, `_attached`, `_set_sweep`,
  `_clear`, `rb_get_dyno`.
- **Snapshots:** `rb_get_snapshot(sim, version, buffer, size)` writes
  [contracts/snapshot-v2.md](../contracts/snapshot-v2.md)'s binary layout from one moment
  between two steps: a 112-byte header (identity, generation, sequence, the physics and
  audio clocks, the crank's cycle, phase and speed), the 24 instruments (the telemetry,
  bit for bit), and five values per cylinder. `tools/snapshot_v2.py` is the executable
  check, and contracts/examples/snapshot-v2-i4-idle.bin a real stream of seven
  (`rb_cli sound --snapshots FILE`).
- **Events:** with `RB_PARAM_EVENTS` on (off by default, at no cost), the engine's actual
  spark commands and discharges, flame starts, fuel injections, intake and exhaust
  openings, fuel cuts and limiter cuts, in the order they happen, into a ring of 256;
  `rb_read_events` takes them, `rb_events_lost` counts what a full ring dropped (its
  sequence numbers are spent, so the gap shows).
- **Also:** `rb_variation_seed`, `rb_listener_source_distance`, `rb_set_scene_time` (what
  rb_cli's settling needed), the `outlet_radiation` engine field, and
  `RB_OUTPUT_FULL_SCALE_PA` (audio.h's full scale is now defined from it). All new
  functions are in `WASM_EXPORTS`; the WebAssembly core is rebuilt.

## rb_cli sound on the public API

`rb_cli sound` moved into `native/sound.c`, which includes only `rb.h` and rb_cli's own
`cli.h`, `wav.h` and `csv.h`. The Makefile compiles it without core/src on the include
path, so an internal header there is a build error (checked: adding `rb_internal.h`
fails with "No such file or directory"). The drives are a driver's script of
parameters between blocks; the core does the car and its road load.

**Bit-identical, by `tools/cli_parity.py`:** 11 cases rendered by stage 120's rb_cli and
by this one (w64devkit gcc both), as float WAVs with their CSVs and printed summaries:
the idle, the rev, the sweep with its car, the dyno hold, a closed-throttle hold, a long
coast in gear then clutch-in, the boxer's rev in the pass-by stereo scene, the cabin idle
with a load and two layers, the research switches (capped burn, a seed, first-order
momentum, constant outlet, chambered muffler, equal headers), low quality with set
headers and variation 0.5, and a straight-through rev from the side with the engine
layer alone. **All identical** — WAVs, CSVs, and every printed line but the CPU time.

One behaviour changed at the edges: `--inertia` below the engine's own crank inertia used
to set a lighter crank; now it means no car, as gear inertia is the
car on top of the engine.

## Planted bugs

`tools/bugs/stage121.py`: 28 bugs, **all caught** (`python tools/plant_bugs.py 121 WORKDIR
all`). Two were missed at first and taught something:
- **dyno-outlives-its-engine:** a rebuild takes the dyno off twice, directly and when the
  reset `hold` control puts the crank back on the motor, so one alone can't show. The bug
  now removes both.
- **events-out-of-order:** the sort of a step's events was never exercised: at normal
  rates no two events of different cylinders or sources share a step, and a speed sweep
  didn't change that (the angles are fixed by the cams). `test_api_events_share_a_step`
  now sets the intake cam so each intake opens 0.5° after another cylinder's exhaust,
  with 16 kHz physics: hundreds of shared steps, and the bug caught.

`tools/bugs/stage121_python.py`: 5 bugs in the snapshot checker, all caught; one
(phase-not-checked) only after the test checked single snapshots alone, as a broken
phase was also refused by the stream's "the crank turned back".

`plant_bugs.py --check` (stage 120) flagged 4 older bugs this stage's refactor had moved:
`no-listen-on-rebuild` and a pipe-rate bug now plant in `rb_sim_build` (caught); stage
12c's `dyno-kept-on-a-new-preset` and stage 13's `preset-forgets-pipe-rate` are retired,
the same lines now as other bugs.

## Tests

`python tools/run_tests.py --keep-going` on both systems, the same results on each:

| Suite | Windows (w64devkit) | WSL (gcc) |
| --- | --- | --- |
| native | 262 tests (14 new), 4,968,415 checks | the same |
| wasm | the committed core is current (rebuilt this stage) | the same |
| wasm-c | 18 tests, 5,743 checks | the same |
| python | 114 tests (3 new), none skipped | the same |
| web | 17 tests | the same |
| e2e | 2 tests, dev and production builds | the same |

`plant_bugs.py --check` reports 0 stale bugs. The parity renders were run again with the
finished rb_cli: all 11 identical.

No physics changed, so there was no sound check beyond the parity renders.
