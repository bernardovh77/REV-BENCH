# Stage 068: live instruments, telemetry and the core's dyno

Done 2026-11-19 by Claude (Opus 5.5). The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-068).

**The question:** can the app show what the engine does, from the snapshots and the host's own
timing (speed, manifold pressure and boost, lambda and AFR, torque and power, the computer's
cuts, the output level, the render's speed), say when a build is outside what the model handles
well, and run the core's dyno with live, overlaid, exportable curves that are the core's own
readings?

## The gaps before it

PLAN.md's open rows: 066's device checks, 118's tier misses, 131's V12-V16 speed (→ 129), 039,
041, 042, 014. None touches the instruments or the dyno: all safe to leave.

## What was built

**The core:**
- **The model's own warning.** The pipes' faces count each time their sonic cap or mass cap
  steps in or their flow stops being a number, and the chambers already counted their repairs;
  `rb_engine_guards` sums them once a cycle and `RB_TLM_GUARD_RATE` is their rate over the
  cycle. **The orifices' equalisation limit is not counted:** measured, it acts on about a
  quarter of the valves' steps in a normal idle (806 452 in 384 000 steps of a four's eight
  valves) and on the throttle wide open (about 3600 a second), by design (a big opening between
  small volumes equalises faster than a step), so it tells nothing apart. What's left is still
  in normal running:

  | Drive (a second's average, lean tier) | Idle | Wide open | Lift-off |
  | --- | ---: | ---: | ---: |
  | The five presets, every tier | 0 | 0 | 16-180 (a cycle's burst up to 2200: the runners' ends on a closed throttle) |
  | The four with the narrowest headers allowed (choked) | 0 | **5123** | 95 |
  | The narrowest runners, exhaust, tailpipe, throttle | 0 | 0 | 16-180 |

  `RB_GUARD_RATE_WARN` is 1000 a second averaged over a second: five times the presets' worst
  second, a fifth of the choked build's. The counters cost nothing measurable (below).
- **The dyno in the telemetry:** attached, the speed it holds, its readings, and the last
  reading's speed, brake torque and power (`RB_TLM_DYNO_*`), so in every snapshot.
- **Snapshot version 3** (`contracts/snapshot-v3.md`, "RBS3"): the seven new instruments at the
  end, 31 in all; nothing before them moved. The checker (`tools/snapshot_v3.py`), its example,
  the generated decoder and its golden file follow; version 2's files are in git's history.

**The host:**
- **The output meter's input:** the render thread keeps the peak and the sum of squares of
  everything the device is given (both channels, after the swaps' fades) and publishes them
  with each snapshot (`rb_host_snapshot_info.out_peak`, `out_rms`), so no block between two
  snapshots is missed. A loop over the block, no allocation.
- `rb_trace_play_observed` (the engine after each block) and `rb_host_render --dyno-csv`, every
  reading the core's dyno makes in a replay.

**The add-on:** the host's dyno commands (`dynoAttach`, `dynoSweep`, `dynoClear`,
`dynoRelease`) and the meter beside each snapshot.

**The engine process:** the `dyno-*` commands (their numbers checked, refused during a
rebuild), recorded in the trace; each snapshot message carries the meter.

**The window:**
- **The instruments** (centre panel): speed, manifold pressure, boost or vacuum (against
  standard sea-level air, every preset's surroundings: the engine's own isn't a field), lambda
  and AFR (lambda times the core's stoichiometric default), torque, power, spark advance, the
  fuel cut and the rev limiter, the limiter's cuts, the output meter, render time, ring and
  underruns; the editor's units (N·m or lb·ft, kPa or psi, kW or hp).
- **The output meter** (`meter.ts`, `LevelMeter.svelte`): peak, held peak and RMS in dB below
  full scale, from the host's values. Defined ballistics: instant attack; the peak falls
  20 dB a second; the held peak stays 1.5 s, then falls; the RMS averages over 0.3 s; CLIP lit
  2 s after full scale. Time is the audio's (frames over the rate), never the window's clock.
- **The model's warning** (`guard-watch.ts`): a second's average of the guard rate, timed by
  the audio, over `RB_GUARD_RATE_WARN` (the app's test reads rb.h for it): a notice that the
  build is outside what the model handles well, kept 3 s after it falls back.
- **The dyno** (bottom panel): a sweep (from, to, rate) or a hold, wide open or as driven;
  Start, Release, Clear runs, Export CSV (through the main process's save dialog, the name only
  a suggestion); its live readouts; torque and power against speed, the run being read bold
  and the four before overlaid, in the editor's units. The runs (`dyno.svelte.ts`) are the
  core's readings: a point each time its count goes up, never a torque of the window's own.

## The evidence

| Card's evidence | How | Result |
| --- | --- | --- |
| App and native curves agree for the same engine and trajectory, aligned in time: at most 2 % pointwise torque error above 10 % of peak, 1 % of peak below | `app/e2e/dyno.spec.ts`: a 2000-5000 rpm sweep at 1000 rpm/s, wide open, run from the panel and exported through its button; the trace the engine process recorded replayed by `rb_host_render --dyno-csv`; matched by reading; each point's time since the run's first within a snapshot interval and a block of the native one's | 115 points, every one the native reading exactly (0.0 % of the allowed error), peak 180.6 N·m |
| Meter ballistics | `tests/instruments.test.ts`, the host's `test_host_snapshots_carry_the_output_meter` (a 0.5 tone reads 0.5 and 0.354; silence 0) | pass |
| Sweep, reset and overlay state | `tests/instruments.test.ts` (runs end on release and on a clear, the newest five kept, an empty run dropped, CSV in SI units), `tests/engine-dyno.test.ts`, the add-on's dyno test on the real core | pass |
| The model's warning, and its counters' cost | `test_api_guard_rate_warns_only_outside_the_model`; `tools/speed_check.py HEAD` (MSVC, 20 cells) | the presets under a quarter of the threshold, the choked build over twice it; median -0.3 %, inside the 3 % |
| No dyno in JavaScript; timing from the host | the runs take the core's readings only; every time is the snapshots' audio frames | by construction, and the e2e above |

**Planted bugs:** core (`tools/bugs/stage068.py`) 5 of 5; host (`stage068h.py`) 4 of 4;
TypeScript (`app_bugs_068.py`) 14 of 14, after one missed at first: the meter's fall rate,
whose test had computed its expectation from the same constant (it states 20 dB/s now).

**Found on the way:** stage 066's dead-man e2e read the throttle as the panel's second readout;
it now finds the readout by its label.

**All suites** (`tools/run_tests.py`, on mains power): native, wasm, wasm-c, python, web, cmake
(MSVC), host, addon, app, app-e2e and layouts pass. Twice, in full runs that took 213-224 s
(124 s normally), stage 124's message-passing measurement (`snapshot-path.spec.ts`, its
synthetic utility process) ran past its 120 s limit; it passed alone, beside the dyno's test and
in the next full run (19 s): the laptop was slow then. Named here in case it comes back.

## What is left

- **Time charts** (speed, torque, the warning over a drive): the card names the dyno's curves
  only; a scrolling chart of the instruments is for 069's qualification or later.
- **Boost against the engine's own air:** today against standard sea-level air; an ambient
  field (or the turbo stages, 078-084) changes it.

## Files

New: `app/src/ui/lib/meter.ts`, `guard-watch.ts`, `dyno.svelte.ts`,
`app/src/ui/components/LevelMeter.svelte`, `LineChart.svelte`, `app/tests/instruments.test.ts`,
`engine-dyno.test.ts`, `app/e2e/dyno.spec.ts`, `contracts/snapshot-v3.md` (from v2),
`tools/snapshot_v3.py` and its test (from v2), the v3 example and golden file,
`tools/bugs/stage068.py`, `stage068h.py`, `app_bugs_068.py`.
Changed: `core/include/rb.h`, `engine.c`/`.h`, `pipe.c`/`.h`, `rb_drive.c`, `rb_params.c`;
the host (`rb_host.h`, `rb_host_trace.h`, `host_internal.h`, `runner.c`, `host.c`, `trace.c`,
`host_render.c`, tests); the add-on (`rbn_host.c`, tests); `addon.ts`, `protocol.ts`,
`bridge.ts`, `control.ts`, `trace-recorder.ts`, `fake-addon.ts`, the preload, `main.ts`; the
store and client, `units.ts`, the centre and bottom panels, `App.svelte`, the tokens;
`tools/run_tests.py` (rb.h in the app's copy), `gen_snapshot_ts.py`; retargeted bugs of 12b
and 124.
