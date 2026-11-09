# Engine snapshot — version 2 (binary)

Stage 121. A snapshot is one coherent picture of the running engine, taken by
`rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, buffer, size)` between two physics
steps, so every value in it belongs to the same instant (contracts/runtime-v1.md's
"one solver boundary"). The desktop app's engine process copies one to the
window about 60 times a second (docs/app-design.md §4); stage 124 generates its
TypeScript decoder from these offsets and tests that the two agree.

Version 1 was runtime-v1's JSON reference shape; version 2 is the binary layout
the C core writes. [tools/snapshot_v2.py](../tools/snapshot_v2.py) is the
executable check of everything below (`python tools/snapshot_v2.py FILE`), and
[examples/snapshot-v2-i4-idle.bin](examples/snapshot-v2-i4-idle.bin) is a real
stream of seven, the four starting with the key (`rb_cli sound --preset i4
--idle --seconds 0.1 --snapshots FILE`).

## Layout

Little-endian, packed, doubles IEEE 754: the header, then the instruments,
then the cylinders.

| Bytes | Part |
| --- | --- |
| 0–111 | the header |
| 112 + 8 i | instrument i, a double (24 of them) |
| 304 + 40 c + 8 f | cylinder c's field f, a double (5 each) |

So a snapshot of an engine of n cylinders is 304 + 40 n bytes: 464 for a
four, 624 for a V8; `rb_snapshot_size` says so, and the header's `size` too.

### The header

| Offset | Field | Type | Meaning |
| --- | --- | --- | --- |
| 0 | `magic` | u32 | `0x32534252`, the bytes "RBS2" |
| 4 | `version` | u32 | 2 |
| 8 | `size` | u32 | bytes in the whole snapshot |
| 12 | `cylinders` | u32 | the engine's, 1–16 |
| 16 | `instruments` | u32 | 24 |
| 20 | `cylinder_fields` | u32 | 5 |
| 24 | `reserved0` | u32 | 0 |
| 28 | `reserved1` | u32 | 0 |
| 32 | `build_id` | u64 | a 64-bit FNV-1a hash of the engine description's every field value (the bits of `rb_get_engine`'s doubles): the same engine, the same id. Not contracts/README.md's string build namespace |
| 40 | `generation` | u64 | engine builds since the sim was made (`RB_TLM_GENERATION`) |
| 48 | `sequence` | u64 | +1 for every snapshot the sim takes, across generations |
| 56 | `physics_tick` | u64 | physics steps since this generation's build |
| 64 | `audio_frame` | u64 | audio frames rendered since the sim was made: it never goes back, rebuilds included |
| 72 | `cycle_index` | u64 | the crank's completed 720° cycles in this generation |
| 80 | `physics_hz` | f64 | physics steps per second |
| 88 | `audio_hz` | f64 | the sample rate |
| 96 | `phase_rad` | f64 | the crank in its cycle, [0, 4π): cylinder 1's firing TDC is 0 |
| 104 | `speed_rad_s` | f64 | the crank's speed, never negative |

### The instruments

The telemetry from `RB_TLM_RPM` on, in its order: instrument i is
`rb_get_telemetry(sim, RB_TLM_RPM + i)` at the same instant, bit for bit.

| Index | Name | Unit |
| --- | --- | --- |
| 0 | `rpm` | rpm |
| 1 | `rpm_smooth` | rpm |
| 2 | `cycle_rpm` | rpm |
| 3 | `map` | Pa |
| 4 | `torque` | N·m |
| 5 | `power` | W |
| 6 | `lambda` | 1 |
| 7 | `spark` | deg |
| 8 | `idle_valve` | 1 |
| 9 | `throttle` | 1 |
| 10 | `throttle_area` | m² |
| 11 | `ignition` | 1 |
| 12 | `starter` | 1 |
| 13 | `held` | 1 |
| 14 | `fuel_cut` | 1 |
| 15 | `spark_cut` | 1 |
| 16 | `limiter_cuts` | 1 |
| 17 | `overrun_time` | s |
| 18 | `load_torque` | N·m |
| 19 | `crank_inertia` | kg·m² |
| 20 | `air_flow` | kg/s |
| 21 | `fuel_flow` | kg/s |
| 22 | `exhaust_temperature` | K |
| 23 | `tailpipe_temperature` | K |

`torque`, `power`, `lambda`, `spark`, the flows and the temperatures are the
last complete cycle's; the rest are now. rb.h's comments define each.

### Each cylinder

Five doubles per cylinder, cylinder 0 (cylinder 1 on the engine) first:
`pressure` (Pa, > 0), `temperature` (K, > 0), `burn` (the burned fraction of
this cycle's fuel, [0, 1]), `intake_lift` and `exhaust_lift` (m, ≥ 0). They are
`rb_get_cylinder`'s `RB_CYL_PRESSURE`, `_TEMPERATURE`, `_BURN`, `_INTAKE_LIFT`
and `_EXHAUST_LIFT`.

## Rules

A reader refuses a snapshot whose magic, version, counts or size aren't these,
whose reserved words aren't 0, or whose numbers are out of range (any
instrument not finite; the phase outside [0, 4π); a negative speed; a cylinder's
pressure or temperature not positive, its burn outside [0, 1], a negative lift).

In a stream from one sim, the sequence goes up by exactly one each time and the
audio clock never goes back. Within one generation the build id stays the same
and the physics clock and the crank (cycle, then phase) never go back. A new
generation (a rebuild) starts the physics clock and the crank again; the
generation itself never goes back.

`rb_get_snapshot` returns the bytes it wrote, `RB_ERR_ID` for a version it
can't write, `RB_ERR_SIZE` for a buffer smaller than `rb_snapshot_size`, and
`RB_ERR_ARG` for a NULL sim or buffer. Taking one costs no allocation and
changes nothing in the engine; only the sequence counts it.

## What stays for later

Pipe-cell arrays by subscription are stage 073's, as a version 3 or an optional
trailing block announced in the header. Events travel apart, through
`rb_read_events` (runtime-v1's actual events).
