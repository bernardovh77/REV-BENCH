# Stage 130: `engine.c` split along the step order

Done 2026-11-23 by Claude (Opus 5.5) overnight, with Bernardo asleep. The card is
[docs/roadmap/14-review-fixes.md](roadmap/14-review-fixes.md#stage-130).

**The question:** can `core/src/engine.c` (1,759 lines by now: the description, the build, the
engine computer, the step and the books) be split into parts that follow the step's order, with
no behaviour change and no speed lost?

## The gaps before it

PLAN.md's open rows: 069's and 066's real-device runs and listening, 118's misses, the fresh
engine's tone (phase B), 039, 041, 042, 014. None touches the file's layout: safe to leave.
Stage 128 (the complaints list, next in phase B) was looked at first and left: every candidate
it asks for needs Bernardo's blind verdict (its first measurement is below, "Found on the way").

## What was done

Whole functions moved line for line (`tools/` had no part in it beyond the move); the only edits
are the files' headers and includes, `static` dropped from the six functions another part calls,
and their names given the core's `rb_` prefix:

| File | Lines | What |
| --- | ---: | --- |
| `engine_build.c` | 722 | the description (`rb_engine_desc_default`, the layout, `rb_engine_desc_check`), building the engine (`rb_engine_init_at`: cylinders, valves, pipes, junctions, the variation it starts with), and what the books read of it (`rb_engine_pipe`, `rb_engine_contents`, the guards) |
| `engine_computer.c` | 259 | the engine computer: the spark it chooses (`rb_computer_spark`, its map, MBT, the idle spark, `rb_computer_spark_advance`), the burn it expects (`rb_computer_burn_duration`), the limiter and the 2-step (`rb_computer_limiter_cut`), and `rb_computer_step` (the hard cut, the fuel cut on overrun, the idle air valve) once a step |
| `engine.c` | 759 | the step (`rb_engine_step`: intake closing, the spark, the burn, the crank, the pipes' fast and slow steps) and the cycle's books (`rb_engine_start_cycle`, `finish_cycle`) |
| `engine_parts.h` | 80 | what they share: `PI`, `AIR`, `FUEL`, `RB_SPARK_DEFAULT`, `RB_GAS_PACE_RPM`, the variation's constants and `variation()` (now `static inline`), and the six functions' declarations |

**Not split: the intake side's step from the exhaust side's.** The card names them as parts,
but they are one function, `rb_engine_step`, the physics' hot loop: its helpers are inlined
into it, and cutting it would put calls (and lost inlining) in every step. The card's stop rule
("readability is not bought with real time") decides: they stay together in `engine.c`, now a
file about the step alone.

## The evidence

| Card's evidence | Result |
| --- | --- |
| `tools/cli_parity.py` identical for every case | 11 of 11 byte-identical to the last commit (gcc) |
| `tools/speed_check.py` within 3 % on both compilers | MSVC: median +0.2 % (15 cells, the worst −2.3 %, the four at low); gcc (`tools/bench_render.py`, the same 15 cells interleaved, best of 3 on the fast cores): every cell within 1 % (worst −0.9 %, the boxer at high) |
| `make wasm` rebuilt | rebuilt (its output changes with the files, not its renders: the wasm suite and the web tests pass) |
| Every planted bug still applies (`plant_bugs.py --check`) | 41 bugs of 18 tables followed their code into the new files (by script: each to the one file that holds its text once); one, stage 133's `fuel-cut-cuts-nothing`, names the renamed `rb_computer_limiter_cut`; 0 stale |

`tools/run_audio_resolution.py`, which patches a line of the pipes' building into a copy,
patches `engine_build.c` now.

**All suites** (`tools/run_tests.py`, on mains): native, wasm, wasm-c, python, web, cmake (MSVC),
host, addon, app, app-e2e and layouts pass. app-e2e failed twice first on stage 124's
`snapshot-path.spec.ts` (the utility process sending snapshots flat out) running past its
120 s limit, then passed whole (that test in 19.8 s). It has done this in about four runs of ten
since stage 068, quiet machine or not: a hang, not slowness (it takes 19 s when it passes).
**Found and fixed after the commit:** a race in the test. It started the page's listener for the
port (`page.evaluate`, not awaited) and at once had the main process post the port; when the port
arrived first, the message was lost and the page waited forever. The listener is now installed,
and waited for, before the port is posted. It then passed the suite and 8 runs of its own in a
row (at four in ten, nine passes would happen about 1 % of the time).

## Found on the way

**Stage 128's first measurement** (the cycle repeating too exactly): the four held at 2850 rpm
wide open, from behind, against the Honda F20C (`tools/compare_real.py compare --kind steady`):

| | repeat below 500 Hz | above 1 kHz |
| --- | ---: | ---: |
| Honda F20C, cruising | +4.6 | −20.1 |
| ours, variation 0 | +29.5 | −15.5 |
| ours, variation 1 (today's) | +20.6 | −9.7 |
| variation 2 | +14.8 | −6.2 |
| variation 4 | +10.6 | −4.3 |

The burn's variation alone doesn't close it: four times today's (a 4-9 % COV of IMEP wide open,
past real engines' 1-3 %) leaves 6 dB, and brightens the sound 5 dB above 1 kHz. A correlation
of 0.74 (the F20C's +4.6 dB) means a quarter of the low band changes from one cycle to the next:
that wants a source of its own (the card's turbulence, or broadband flow noise in the low band,
where the tailpipe jet's model is cut below 150 Hz), and Bernardo's ears. For stage 128.

**CI's Windows job** failed from 2026-11-23 on `test_api_dyno_in_the_telemetry`: the runner's
image moved to MSVC 19.52 (from 19.51, which passes here and passed in CI before). Made to print
the values (commit 8c1afd5), CI showed the snapshot's torque and the telemetry's bit-identical
(`0x1.54a1da61d5310p+7` both, and every other instrument matching), and `torque == told` false
all the same: a miscompile of that comparison, not the core. The check compares the snapshot's
eight bytes with the reading's now (`memcmp`), which is the stricter statement anyway. Nothing
else in the core's 290 tests tripped on 19.52; if another comparison of a `memcpy`'d double
fails on it, this is why.

## What is left

Nothing of the card. The intake and exhaust sides' step stay one function (above).

## Files

New: `core/src/engine_build.c`, `engine_computer.c`, `engine_parts.h`, this doc. Changed:
`core/src/engine.c`; 18 bug tables (`tools/bugs/`), `tools/run_audio_resolution.py`, the wasm
core, `docs/architecture.md`.
