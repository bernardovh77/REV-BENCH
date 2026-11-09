# Stage 118 — A real-time tier for today's engine

**Status:** built and measured on the baseline machine assumed below; **no tier yet meets both
the speed and the tolerances on every preset** (details under "The tier"). The timings are
provisional: another agent was building and testing on the same laptop during most runs, so
the reviewer takes the final baseline with the command given under "The benchmark". Bernardo's
blind listening and his confirmation of the baseline machine are still to come.

The roadmap asks ([specification](roadmap/07-app.md#stage-118)) for a benchmark of what the
app will run, a profile of it, speed-ups that change nothing the core computes, and a declared
playable tier that runs every preset at least 1.5 times real time on the render thread.
Nothing in the default quality's sound changed: every render is bit for bit what it was.

## The baseline machine (assumed)

The plan leaves the choice to Bernardo (app-design §15, open question 1). Until he decides,
his laptop is the baseline: **Intel Core i7-1355U** (2 performance and 8 efficiency cores,
12 threads, 16 GB), Intel UHD graphics, Windows 11 Home. MSVC 14.51 (`/O2 /GL`, LTO) is the
target compiler; w64devkit's gcc 16.1 (no LTO on Windows) and WSL's gcc are for comparison.

The CPU is hybrid, and that matters more than any speed-up here: **an efficiency core runs the
engine at 0.5 to 0.55 times a performance core's speed.** The render thread will have to be
kept on the performance cores (stage 123's priority and affinity work); every number below says
which it was run on.

## The benchmark

`native/bench_render.c` (`rb_bench_render`) times the app's whole render path, `rb_render_stereo`
in blocks of 128 frames at 48 kHz with the passby scene (the noise, the listener, the six
sources' decimators, the scene, the DC blockers and the limiter), through `rb.h` alone: the
object compiles with no header from `core/src`, as `rb_cli sound`'s does. Variation is on.

| Drive | Warm-up (not timed) | Timed window |
| --- | --- | --- |
| idle | the key, the starter, 3 s | 3 s idling |
| hold | held at 3000 rpm wide open, 1 s | 3 s on a dyno there |
| rev | the key, 3 s idling | 7 s: a blip, floored into the rev limiter, foot off, falling |

Each cell reports the best of N renders (each from scratch; the render is deterministic): the
window's seconds of sound per second of computing (x real time), the slowest block, the 99th
percentile block, and a **hash of every float rendered**. Two builds of one compiler must give
the same hash; that is the proof of "changes nothing it computes" for the whole table at
once, and `tools/cli_parity.py` is the second. Beside the timings, three more things in the
same program: `--ftz` (flush denormals to zero, for the denormal check), `--affinity MASK`
(pinning, for the hybrid CPU), `--wav-dir` (the listening files) and `--figures` (torque,
power, volumetric efficiency and the idle speed, for the tier's engine tolerances).
Besides the four named qualities it takes `os2` and `os3`: physics at 2 or 3 times the sample
rate with the pipes at their default 48 kHz, which no named quality has.

**One command makes the whole table, interleaved** (every build in turn for each cell, round
after round, so that whatever else the machine does falls on all builds alike; the best round
of each build is kept; cells whose hashes differ between builds of one compiler are flagged
and the exit code is 1):

```
python tools/bench_render.py --exe msvc:NAME=PATH/rb_bench_render.exe [--exe gcc:NAME=...] \
    --quality low,medium,high --drive idle,hold,rev --rounds 3 --cores p,e --md table.md
```

`--cores p,e` pins to the performance cores' logical processors and then to the efficiency
cores' (found through `GetLogicalProcessorInformationEx`; `--list-cores` shows them: here
`0xf` and `0xff0`). `make bench-render` runs it as a plain program, `make bench-render-wasm`
as WebAssembly under Node (selected cells: `BENCH_RENDER_ARGS="i4 --quality low --drive hold --runs 1"`).
Tested in `tools/test_bench_render.py` (the best round is kept, a changed sound is flagged,
different compilers may differ).

## The profile

`tools/win_sampler.c`: Windows' own sampler (WPR, xperf) comes with the Windows ADK, which this
machine does not have, so this is a 150-line sampling profiler on DbgHelp (it starts the
program, suspends its main thread every millisecond and reads where it was; the build needs
`/DEBUG` for the .pdb). It sees what LTO inlined into which function. gprof (gcc, WSL) agrees.
callgrind and perf: not installed in WSL here (valgrind and perf are absent; installing them
is a download I did not make), so instruction counts are not in this record.

The cross-plane V8 at the low quality, held at 3000 rpm (MSVC, before the speed-ups):

| Function | Share |
| --- | --- |
| `push` (the pipes' momentum step) | 10.9 % |
| `rb_gas_init` (a chamber's mixture, 21 million calls in 4 s) | 10.8 % |
| `u_and_cv` (u(T) and cv(T)) | 9.7 % |
| `rb_chamber_update` | 6.9 % |
| `rb_gas_T_cv_from_u_diag` (Newton's step for T) | 6.7 % |
| `rb_engine_step` | 6.2 % |
| `move_gas` | 5.7 % |
| `momentum_flux` | 5.2 % |
| `rb_chamber_integrate_many` | 4.1 % |
| `finite` (a symbol of the C runtime: see below) | 3.6 % |
| the rest (pipes, orifices, valves, noise, scene, decimators, each under 3 %) | 30 % |

and at the high quality the same list, `rb_gas_init` 11.5 %, `push` 11.5 %,
`rb_chamber_integrate_many` 8.7 %. The cost is the gas thermodynamics of some 400 cells and
the 27 fast chambers: no hot spot above 12 %, no allocation, no I/O. The sound's own layers
(noise, listener, scene, limiter) are under 10 % together. gprof on gcc: `rb_gas_init` 15 %,
`rb_chamber_integrate_many` 9.7 %, `rb_gas_T_cv_from_u_diag` 8.4 %, `push` 7.4 %.

**MSVC's library calls** were the cheap finding: `copysignl` 1.2 %, `fmin` 0.7 %, `fmax` and
`fmod` (0.6 %) are calls into the C runtime that gcc inlines, and none is in the profile after
the changes below. The `finite` entry is not ours: it stays at 3.5 % after every `isfinite` in
the core was replaced, so it is probably C-runtime code that DbgHelp names after the nearest
export (the sampler cannot tell). The gain of `rb_finite` is therefore unproven; it is exact,
tested, and kept.

After the speed-ups, the same run: `rb_chamber_update` 19.8 % (it now holds the inlined
Newton step and u(T), 16 % and 7 % before), `push` 11.9 %, `rb_gas_init` 8.4 % (10.8),
`rb_engine_step` 6.2 %, `move_gas` 6.0 %, `volume_step` 5.7 %, `momentum_flux` 4.3 %,
`rb_chamber_integrate_many` 3.6 %; `copysignl`, `fmin`, `fmax` gone.

**Denormal numbers: none matter.** `rb_bench_render --ftz` (flush to zero and denormals are zero)
renders 12 cells (the V8, the four and the boxer, low and high, idle and rev) with exactly the
same hashes as without: no subnormal number affects a result anywhere in the path, so
flushing them gains nothing and changes nothing. The timings with it were no faster.

## The speed-ups (render hashes unchanged)

| Change | Where |
| --- | --- |
| `rb_gas_init` does two coefficients at a time in SSE2 registers (the same additions and multiplications in the same order, never fused: each coefficient is independent) | `gas.c` |
| `rb_finite` (`fabs(x) < INFINITY`) for `isfinite` in the per-step code | `fastmath.h` and every core file |
| `u(T)`, `cv(T)` and Newton's step for T as always-inlined header functions, so the chamber's code inlines them across files (MSVC's LTO did not) | `gas_inline.h` |
| the second-order momentum flux's `copysign(fmin(...))` as two comparisons (neither slope is NaN or zero there) | `pipe.c` |
| `rb_clamp`, `rb_max` for `fmax(..., fmin(...))` in the render loop (every value finite by then) | `rb_sim.c` |
| `fmod(x, 720)` only when `x` is outside a cycle (inside, it returns `x`) | `valve.c`, `engine.c` |

Measured on the performance cores, MSVC, best of 3 interleaved rounds, the hold drive
(`before` is stage 122's code, `after` this stage's): the speed-ups bring **+3 % to +11 %**
(median +7 %) on every preset and quality, on the efficiency cores the same; with w64devkit's
gcc +4 % to +6 % (the boxer's high quality read -3 %, within the noise). The tables are below.
They cost nothing in sound. How much each of the six changes contributes was not measured
separately (the machine was too noisy to see 1 % steps).

What did **not** help, measured and dropped: profile-guided optimisation (MSVC `/LTCG:PGO`,
trained on the V8 and the four) gave +1 % to +4 %, within the noise; `/Ob3` and `/GS-` gave
nothing measurable. Skipping zero-weight species in `rb_gas_init` is useless, because the
fuel fraction is positive (a trace) in all but 335 of 21 million calls. `/arch:AVX2` or
`-march=native` would be a bet on the user's CPU, and are not used.

### Proof of "bit-identical"

* Every cell of `rb_bench_render` has the same hash before and after, for MSVC and for gcc
  (the matrix: `tools/bench_render.py` flags a difference; none).
* `tools/cli_parity.py render` and `compare`, 11 cases (idle, rev, sweep, hold, coast, stereo
  scene, the cabin, research switches, quality low, side listener...): **all identical** for
  gcc (w64devkit) and **all identical** for MSVC, WAV, CSV and printed text, before and after.
* `tests/test_speed.c` pins the parts a hash cannot tell apart: a golden hash of 4096 mixtures
  through `rb_gas_init` (the old code's, the same on gcc and on MSVC), `rb_finite` against
  `isfinite` at every edge, `rb_min`/`rb_max`/`rb_clamp` against the library, the valve's cycle
  offset against `fmod`, Newton's floor, and the pipe's minmod slope against its definition.

## Timings

All provisional (contaminated, as said). x real time, best of 3 rounds interleaved,
render path, 48 kHz, passby scene.

**MSVC, before and after the speed-ups** (stage 122's code and this stage's; hold drive, best of 3 rounds interleaved; x real time on the performance cores (P) and on the efficiency cores (E)):

| Preset | Quality | P before | P after | gain | E before | E after |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| i4 | low | 2.75 | 2.90 | +6 % | 1.53 | 1.65 |
| i4 | medium | 1.42 | 1.47 | +3 % | 1.01 | 1.06 |
| i4 | high | 1.02 | 1.12 | +10 % | 0.77 | 0.82 |
| i6 | low | 1.19 | 1.30 | +9 % | 0.99 | 1.06 |
| i6 | medium | 1.04 | 1.13 | +8 % | 0.67 | 0.71 |
| i6 | high | 0.76 | 0.80 | +5 % | 0.50 | 0.54 |
| v8-cross | low | 1.20 | 1.32 | +10 % | 0.78 | 0.83 |
| v8-cross | medium | 0.83 | 0.90 | +9 % | 0.52 | 0.55 |
| v8-cross | high | 0.61 | 0.65 | +6 % | 0.39 | 0.41 |
| v8-flat | low | 1.34 | 1.41 | +5 % | 0.86 | 0.91 |
| v8-flat | medium | 0.87 | 0.96 | +10 % | 0.55 | 0.59 |
| v8-flat | high | 0.67 | 0.73 | +9 % | 0.42 | 0.46 |
| boxer4 | low | 2.05 | 2.23 | +9 % | 1.36 | 1.47 |
| boxer4 | medium | 1.43 | 1.58 | +11 % | 0.93 | 0.99 |
| boxer4 | high | 1.09 | 1.18 | +8 % | 0.69 | 0.73 |

**MSVC after, every drive** (2 rounds; x real time; worst = the slowest block in ms of the 2.67 ms deadline, best round):

| Preset | Quality | P idle | P hold | P rev | P worst | E idle | E hold | E rev |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| i4 | low | 3.07 | 3.05 | 2.91 | 3.9 | 1.66 | 1.65 | 1.59 |
| i4 | os2 | 1.69 | 1.69 | 1.64 | 3.1 | 1.11 | 1.12 | 1.05 |
| i4 | high | 1.27 | 1.29 | 1.22 | 6.4 | 0.80 | 0.80 | 0.79 |
| i6 | low | 1.65 | 1.65 | 1.58 | 6.7 | 1.06 | 1.07 | 1.02 |
| i6 | os2 | 1.11 | 1.09 | 1.05 | 6.8 | 0.73 | 0.71 | 0.69 |
| i6 | high | 0.84 | 0.84 | 0.81 | 8.3 | 0.53 | 0.53 | 0.50 |
| v8-cross | low | 1.32 | 1.32 | 1.26 | 6.3 | 0.78 | 0.77 | 0.79 |
| v8-cross | os2 | 0.87 | 0.88 | 0.83 | 6.2 | 0.55 | 0.56 | 0.53 |
| v8-cross | high | 0.66 | 0.67 | 0.64 | 9.1 | 0.41 | 0.42 | 0.39 |
| v8-flat | low | 1.41 | 1.45 | 1.40 | 3.8 | 0.90 | 0.92 | 0.88 |
| v8-flat | os2 | 0.95 | 0.98 | 0.92 | 8.0 | 0.61 | 0.62 | 0.58 |
| v8-flat | high | 0.70 | 0.73 | 0.69 | 5.7 | 0.44 | 0.46 | 0.42 |
| boxer4 | low | 2.31 | 2.26 | 2.19 | 4.4 | 1.46 | 1.47 | 1.43 |
| boxer4 | os2 | 1.50 | 1.50 | 1.45 | 3.4 | 0.99 | 0.99 | 0.96 |
| boxer4 | high | 1.16 | 1.16 | 1.12 | 5.2 | 0.74 | 0.75 | 0.71 |

**gcc (w64devkit 16.1, no LTO on Windows), before and after**, performance cores, hold drive, 3 rounds:

| Preset | Quality | before | after | gain |
| --- | --- | ---: | ---: | ---: |
| i4 | low | 2.29 | 2.42 | +6 % |
| i4 | high | 1.02 | 1.06 | +4 % |
| i6 | low | 1.32 | 1.38 | +4 % |
| i6 | high | 0.67 | 0.71 | +5 % |
| v8-cross | low | 1.06 | 1.11 | +5 % |
| v8-cross | high | 0.55 | 0.58 | +5 % |
| v8-flat | low | 1.15 | 1.21 | +5 % |
| v8-flat | high | 0.58 | 0.61 | +5 % |
| boxer4 | low | 1.64 | 1.73 | +5 % |
| boxer4 | high | 0.84 | 0.81 | -3 % |

**gcc 15.2 in WSL (Linux, `-O3 -flto`), after, hold drive, best of 3 (not pinned):** i4 low 3.17, high 1.54; i6 1.57, 0.82;
cross-plane V8 1.28, 0.68; flat-plane V8 1.44, 0.72; boxer 2.16, 1.15 (x real time). The same numbers as MSVC's within 10 %.
Its gprof on the cross-plane V8 at low: `rb_chamber_update` 24 %, `rb_gas_init` 9.7 %, `move_gas` 8.9 %,
`rb_chamber_integrate_many` 8.0 %, `push` 6.6 %, `volume_step` 5.8 %, `rb_engine_step` 5.8 %, `momentum_flux` 5.3 %
(gcc inlines Newton's step into the update). Before the speed-ups: `rb_gas_init` 15 %.

**WebAssembly (zig cc, Node 22 WASI, SIMD), after, for the record, not gating:** the four at low 1.81, at high 0.93;
the cross-plane V8 at low 0.96, at high 0.49 (hold drive, best of 2). About 0.7 times the native speed.

The slowest block of a cell is often 2 to 4 times its mean: the operating system's scheduler,
and the first blocks of an engine event. A 128-frame block has 2.67 ms; the host's ring
(stage 123) has to absorb them, and its deadline histogram will say how often.

## The tier

### What the quality knobs do to the sound

The qualities differ in two rates: the physics step (`oversample` times 48 kHz: 96 kHz at
low, 192 kHz at medium and high) and the pipes' (24 kHz at low and medium, 48 kHz at high).
`tools/tier_compare.py` renders each preset on three drives (5 s of idling, 3000 rpm held
wide open on the dyno, the 12 s neutral rev) at the default quality, at the candidate, and
at the default again with another variation seed, and measures them as `compare_real.py` and
`orders.py` do, plus the A-weighted level. The second default is the **floor**: how much each
number moves between two renders of the same engine that differ only in their random draws.
The tolerances are app-design §15's, fixed before measuring: 1 dB on each compare_real
measure, the between-order level and the loudness; 1 % on torque, power, VE and idle speed.

**Engine figures, low against high** (`rb_bench_render --figures`, variation off, held wide
open at 1500, 3000, 4500 and 6000 rpm for every preset): torque and power within **0.34 %**,
volumetric efficiency within **0.33 %**, the idle's mean speed within **0.04 %** (5 presets).
The idle swing is not always "no larger": it reads 14.99 vs 15.10 rpm (the four), 11.5 vs 11.2
(the six), 6.3 vs 7.0 (the cross-plane V8), 12.5 vs 11.9 (the flat-plane V8), 18.6 vs 18.5
(the boxer): the differences are of the size of the random scatter. Medium is within 0.26 %.
The engine tolerances are not what stops the low quality.

**Sound measures, low against high** (15 cells per measure; "over" is more than 1 dB):

| Measure | Worst \|diff\| | Worst floor | Cells over |
| --- | --- | --- | --- |
| band share above 1 kHz | 1.28 dB | 0.54 | 2 of 15 |
| above 2 kHz | 0.49 | 0.74 | 0 |
| above 4 kHz | 0.83 | 0.87 | 0 |
| treble slope | 0.33 dB/oct | 0.57 | 0 |
| loudness (dB A) | 1.32 | 0.41 | 2 of 15 |
| repeat, low band | 0.72 | 2.71 | 0 of 10 |
| repeat, 0.5-2 kHz | 1.78 | 2.07 | 2 of 10 |
| repeat, 2-8 kHz | 0.77 | 0.69 | 0 |
| between-order level | 1.04 | 9.43 | 1 of 10 |

The cells over are: the four held at 3000 rpm (+1.28 dB above 1 kHz, +1.32 dB A, **real**:
the floor there is 0.02 and 0.09 dB) and revving (+1.04, +1.07 dB A, real); the four's mid
repeat held (−1.78 dB, the floor is −2.07, noise); the cross-plane V8's between-order level at
idle (−1.04, the floor −2.31, noise); the flat-plane V8's mid repeat held (−1.02, floor −0.17,
marginal). Everything else is inside 1 dB, and most inside 0.5. The 15 listed measures of the
other four engines are inside tolerance except those two marginal ones.

**Where the four's 1.3 dB comes from: the pipes' 24 kHz, not the physics rate.** Medium (192 kHz
physics, pipes 24 kHz) is worse than low on the four (+1.9 dB A held, +1.8 above 1 kHz); the
two variants with the pipes at 48 kHz are not:

| Four, against high | held 3000 rpm: >1 kHz, dB A | rev: >1 kHz, dB A | idle: dB A |
| --- | --- | --- | --- |
| low (96 / 24 kHz) | +1.28, +1.32 | +1.04, +1.07 | −0.39 |
| medium (192 / 24) | +1.80, +1.91 | +1.63, +1.32 | −0.08 |
| os2 (96 / 48) | −0.22, −0.15 | −0.29, −0.09 | **−1.05** (and mid repeat −1.47) |
| os3 (144 / 48) | −0.11, +0.06 | −0.09, −0.09 | −0.66 |

The pipes' step is the cause: at 24 kHz the cells are a little coarser (5.2 cm against 5.0 in
the runners and headers, the Courant limit) and the time step doubles, and the four's
exhaust, at 100 Hz firing, is the engine that notices. The cause is numerical accuracy,
not a bug; the other four engines do not move.

### Speed against the tolerances

x real time on a performance core, MSVC, after the speed-ups (idle / hold / rev are within
10 % of each other, so the hold figure stands for them; the table has all):

| Preset | high | os2 (96k/48k) | low (96k/24k) | Tolerances at low | Tolerances at os2 |
| --- | --- | --- | --- | --- | --- |
| four (i4) | 1.3 | 1.7 | 3.0 | **over** (+1.3 dB A at 3000 rpm and rev) | **over** at idle (−1.05 dB A, −1.47 mid repeat); within at 3000 rpm and rev |
| six (i6) | 0.8 | 1.1 | 1.65 | within | not measured |
| cross-plane V8 | 0.65 | 0.9 | **1.3** | within but one measure at the floor | not measured |
| flat-plane V8 | 0.7 | 1.0 | **1.4** | within but one measure marginal (−1.02) | not measured |
| boxer | 1.2 | 1.5 | 2.2 | within | not measured |

(On an efficiency core everything is about 0.55 times these.)

### Conclusion: no tier meets both, on this machine, yet

* **Low (96 kHz physics, pipes 24 kHz) is the closest.** It passes the engine tolerances
  everywhere, and the sound tolerances everywhere but the four at 3000 rpm and revving
  (+1.0 to +1.3 dB), and it runs at 3.0, 1.65, 2.2 times real time on the four, the six and
  the boxer, but only **1.3 and 1.4 times on the two V8s** on a performance core, 7 % and 14 %
  short of 1.5, and under 1 on an efficiency core.
* For the four, **os2** (physics 96 kHz, pipes 48 kHz) fixes the 3000 rpm and rev numbers at
  1.7 times real time but is 1.05 dB quieter at idle and 1.47 dB lower in the mid-band repeat
  there; **os3** (144 kHz, pipes 48) is inside 1 dB on every four cell (worst 0.66) but is
  slower than 1.5 times (1.2 times on the four).
* **No named quality, and none of the variants tried, gets the V8s to 1.5 times with the
  sound inside 1 dB.** The pipes at 24 kHz are the saving that makes the low quality fast and
  the cost that moves the four; the V8s need the saving but not the four's correction.
* What would close the gap, in the order I would try it: (1) another 15 % from the core, which
  the profile does not offer cheaply (the cost is spread over the gas thermodynamics and the
  pipes' momentum step; the next candidates are a lazy half of `rb_gas_init`, which only ever
  uses `lo/ulo` or `hi/uhi` at a time, and fusing `push` over a pipe's faces); (2) a per-preset
  quality (the four at os2, the others at low), which the app can choose from a table; (3) a
  look at the four's exhaust at 24 kHz, to see whether its 1.3 dB can be taken out
  (a correction for the coarser cells) so that low serves all five; (4) the machine:
  a faster baseline moves the V8s over 1.5 on its own.

I have **not declared a tier**. Declaring low would silently pass a +1.3 dB louder four; the
stop rule is "no silent quality loss". The data above is what the declaration needs.

## Listening files (Bernardo's blind A/B)

`C:\Users\berna\REV-BENCH\renders\stage-118\` (git-ignored; the app's path: the passby stereo
scene at 48 kHz, 32-bit float; the window of each drive as timed above, 3 s idling, 3 s held
at 3000 rpm, 7 s of the rev). `PRESET-QUALITY-DRIVE.wav` for every preset (`i4`, `i6`,
`v8-cross`, `v8-flat`, `boxer4`) at `high` (the default) and `low` (the candidate), on `idle`,
`hold` and `rev`: 30 files; and `i4-os2-*.wav`, the four with the pipes kept at 48 kHz, 3 more.
Listen for the four first: low should sound a touch louder and brighter at 3000 rpm.
The measured tables are `tools/tier_compare.py report`'s; its renders (mono, from behind) are in
`~/rb-build/s118/out/tier-low-high/` and are not kept.

## Tests run

* C tests: 268 tests, 4,969,191 checks, all passed with gcc (w64devkit, `make test`) and with
  MSVC (`ctest`, 292 s); the six new ones are `tests/test_speed.c`.
* `tools/test_bench_render.py` (4 tests) and `plant_bugs.py --check` (0 stale, all tables).
* `make wasm` rewrote `web/src/wasm/rb_core.wasm` and its checksum (matches the sources' bytes).
  The web and e2e suites, and the whole `tools/run_tests.py`, were left to the reviewer.

## Files

New: `native/bench_render.c`, `core/src/fastmath.h`, `core/src/gas_inline.h`,
`tests/test_speed.c`, `tools/bench_render.py`, `tools/test_bench_render.py`,
`tools/tier_compare.py`, `tools/win_sampler.c` (build line in its header),
`tools/bugs/stage118.py`. Changed: `core/src/{gas,chamber,pipe,valve,engine,rb_sim,cylinder,
noise,scene,combustion,radiation,rb_drive,rb_engine_api,rb_params}.c` (the helpers),
`Makefile`, `CMakeLists.txt`, `tests/test_main.c`, `web/src/wasm/rb_core.*`. The knowledge graph
(`graphify-out/`) needs a `graphify update .`.

## Planted bugs

`tools/bugs/stage118.py` (`python tools/plant_bugs.py 118 WORKDIR all`; `--check` passes with
the other tables). Each is a bug a speed-up could bring in; the tests that failed are listed.

| Bug | Caught by |
| --- | --- |
| the SSE mixture subtracts R from the wrong lane | `test_speed_gas_init_is_bit_exact`, the NASA-data, textbook-air and u/T tests |
| its integral (u's polynomial) is not applied | the same, bit-exact test and `test_gas_matches_nasa_data` |
| it drops the burned gas's term | the same bit-exact test, `test_gas_matches_nasa_data`, `test_gas_u_and_T_are_inverse` |
| it never writes the high range | the same, and `test_gas_mixing_is_mass_weighted` |
| `rb_finite` lets infinity through | `test_speed_finite_is_isfinite` |
| `rb_finite` lets NaN through | `test_speed_finite_is_isfinite` |
| `rb_clamp` swaps its limits | `test_speed_min_max_clamp_match_the_library` |
| `rb_min` returns the larger | `test_speed_min_max_clamp_match_the_library` |
| Newton stops after a loose step (1e-2) | `test_gas_u_and_T_are_inverse`, `test_gas_graph_contact_and_physical_pressure` |
| Newton forgets the 1 K floor | `test_speed_newton_floors_the_temperature` (the first try missed it: no test reached the floor) |
| the pipe's slope takes the wrong sign | `test_pipe_acoustic_transport` |
| the pipe's slope takes the larger side | `test_speed_momentum_flux_is_minmod` (missed by every earlier test: added) |
| the default (high) quality gets the low quality's pipe rate | `test_core_quality` |

One bug tried and **retired**: leaving out the scene source's clamp in `rb_sim.c`. The clamp
guards a pressure the engine never reaches, so no test can make it matter, and the render
hashes do not see it either; the lines are kept for safety and listed here as untested.
Three gas bugs first reported only the gas-graph tests (one by a timeout): the run ended there
before the bit-exact test's turn. The table now filters to the gas tests so that test runs,
and it fails each of them.
No existing table's bug needed retargeting (`plant_bugs.py --check`: 0 stale).

## Decisions and what is left

* **Decided:** the baseline is Bernardo's laptop, assumed; the tier's tolerances are
  app-design §15's, unchanged; speed-ups only where the render hash is unchanged; no compiler
  flag that touches results (no fast-math, no contraction, no `/arch` beyond SSE2); no
  `ci.yml` edit (the benchmark could run in CI for information; the runner's cores are not
  Bernardo's).
* **Not done:** the CI runners' timings (stage 123 or the reviewer can add the run to the
  workflows: `python tools/bench_render.py ...` prints a table and exits 1 on a changed
  hash); callgrind and perf; a declared tier (above); the full cells for the medium quality
  on idle and rev; the other presets' os2 sound tolerances.
* **Open, Bernardo:** confirm or change the baseline machine; listen to the files above
  (low against high on every preset, and os2 for the four); whether a per-preset quality is
  acceptable as the tier.
* **For stage 123:** the render thread has to run on a performance core. The same code is
  0.5 to 0.55 times as fast on an efficiency core, and a V8 cannot play there at any
  quality above low.
* **Shared files edited:** `Makefile` (`bench-render`, `bench-render-wasm`, `BENCH_RENDER_OBJ`
  in `ALL_OBJ`), `CMakeLists.txt` (`rb_bench_render`, built from public headers only),
  `tests/test_main.c` (the six new tests), `web/src/wasm/rb_core.*` (`make wasm`).
  No change to `ci.yml` or `run_tests.py`.
