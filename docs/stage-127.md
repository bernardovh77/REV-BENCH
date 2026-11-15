# Stage 127: the 2026-11-15 review's fixes

Done 2026-11-15 by Claude (Fable 5.2, then Opus 5.5 after a usage reset), working alone while
Bernardo was out. The review is [docs/review-2026-11-15.md](review-2026-11-15.md); the stage
card is in [docs/roadmap/14-review-fixes.md](roadmap/14-review-fixes.md#stage-127).

**The question:** of the eleven problems the review found, which can be fixed before stage
066 without a decision that is Bernardo's, and does each fix hold up in the tests, the sound
check and the speed check? Every other problem needs a home in the plan, with what it must
prove.

## The gaps before it

PLAN.md's open rows: 118 (the four 1.05 dB off at idle at balanced; the V8s below 1.5 times
real time), 039 (no matched references), 041 and 042 (calibration, phase B), 014 (old speed
targets). The only one this stage could move is 118's V8 speed, through problem 4 below; the
rest are safe to leave, as each belongs to a later stage and none changes what this stage
touches. The four's balanced tier is untouched by anything here (pipes at 48 kHz).

## What each problem got

| # | Problem | What was done |
| --- | --- | --- |
| 1 | The sound is reached by the longest path | `RB_PARAM_VARIANT`, the live A/B switch, in the core; stage 128 (the complaints list) planned before 050; 066 gets the A/B key |
| 2 | No matched reference | A recording of a known naturally aspirated four (Honda F20C) in the catalog and compared with ours; the search for others recorded (RESEARCH.md); Bernardo's own recording still the one that would settle most |
| 3 | No speed headroom | Stage 129 planned (the cold end on delay lines), with its premise to be measured first |
| 4 | The pipes' hand-off makes false treble | **Confirmed and fixed**: the smoother. See below |
| 5 | Guards that fire unseen | A weekly long fuzz in CI; the guard counter and the warning written into 068's card |
| 6 | One release carries everything | Already in 069's card (the first pre-release, Bernardo's to publish); the licence is its precondition |
| 7 | Two fast cores for audio, the window and 3D | The heavy-window stress run written into 069's card |
| 8 | The throttle lives in the window | The dead-man rule written into 066's card, with its test |
| 9 | macOS arrives at the end | macOS on every push: the core, the host and the add-on with Apple clang. **Its first run ever found a real break** (below) |
| 10 | The process outweighs the code | The push hook; CLAUDE.md split (494 → 243 lines) into rules and `docs/architecture.md`; planted bugs that survive re-indenting; the review's own suggestion corrected |
| 11 | Housekeeping | The two stray folders removed, with a test; the old browser page's e2e test on request only; the licence's absence stated in the README; `engine.c`'s split planned as stage 130 |

## The pipes' hand-off (problem 4)

**The hypothesis held.** The listener got a new flow once a pipe step and took its change as
the pressure: one impulse per pipe step. A train of impulses carries a copy of the sound
around every multiple of the pipe rate, as strong as the sound itself. `test_audio_listener_smooths_slow_pipes`
shows it: a 1 kHz flow from pipes stepping at 16 kHz, heard at 96 kHz, has a line at 15 kHz
at 0.9 or more of the tone's amplitude. At 48 kHz pipes that copy sits above the output
filter's pass band and nobody hears it; at 24 kHz (the low quality) it is at 21–24 kHz; at
16 kHz it is at 13–16 kHz, which is what stage 118's search measured as +13 to +44 dB above
4 kHz.

**The fix** (`rb_smoother` in `core/src/audio.c`): a polyphase windowed-sinc interpolator
reads the flows between their pipe steps. Twelve pipe steps of taps under a Kaiser window
(β 8); each phase's taps add up to exactly 1, so a steady flow stays exactly steady and makes
no sound. It delays the flows by `rb_smoother_delay()` (6 pipe steps), and stage 12b's noise
sources are delayed as much so both still arrive together. The first version (8 taps, β 6)
left the 15 kHz line only 45 dB down; the test asks 50, and 12 taps give it.

**Where it acts:** only on pipes slower than 48 kHz, in mono and in the stereo scenes
(`rb_sim.c`, `smooth_every`). The high and balanced qualities are untouched:

- `tools/cli_parity.py`, the 11 drives against the last commit's build: 10 bit for bit; the
  eleventh, the four idling at the low quality, differs in the sound's columns only (peak,
  RMS, SPL; −26.3 → −26.4 dBFS peak) and is identical in every engine column (rpm, throttle,
  cuts, MAP, inertia).
- `tools/speed_check.py df1fc20`: median +0.1 %, within the 3 %; the sound changed in the five
  low-quality cells and no other.

**The declared tiers still hold** (`tools/tier_compare.py` against the high quality, three
drives each, with the noise floor of a second seed):

| Preset | Tier | Cells outside 1 dB |
| --- | --- | --- |
| six | low | none |
| boxer | low | none |
| cross-plane V8 | low | one: orders at idle, −1.04 dB against a floor of −2.31 (stage 118 had the same one) |
| flat-plane V8 | low | none |
| four | balanced | not affected (48 kHz pipes): bit for bit |

The four at low still misses as before (+1.3 dB above 1 kHz held, +1.24 dB loud); that is why
its tier is balanced.

### The V8s again

With the false treble gone, slower pipes come back as candidates for the V8s (stage 118:
16 kHz pipes ran them at 1.83 and 2.01 times real time, 19.2 kHz the flat-plane at 1.50–1.56).
Against the high quality, now:

| Pipes | Above 4 kHz, worst | Cells outside 1 dB (of 30) |
| --- | --- | --- |
| 16 kHz, before (stage 118) | +13 to +44 dB | most of the treble ones |
| 16 kHz, smoothed | 0.45 dB | 7: idle levels and bands (cross-plane −3.1 above 1 kHz, −12.0 above 2 kHz, −1.7 dB A; flat-plane −2.0, −1.2 dB A), repeat hi −5.3 (cross, idle), repeat mid −1.8 (flat, held) |
| 19.2 kHz, smoothed | 0.67 dB | 4: repeat lo +1.09 (inside its floor of 1.89), repeat hi −1.64 (cross, idle), above 1 kHz +1.03 (flat, held), repeat mid −2.55 (flat, held) |

So at 16 kHz the treble is right but the idle is darker and quieter than the reference; at
19.2 kHz the treble, the slope and the loudness are inside 1 dB everywhere, and four cells
are outside: three repetition measures (+1.09, −1.64 and −2.55 dB; that kind of measure moves
by up to 2.7 dB between two seeds of the same engine, though in two of these cells the floor
is small) and the flat-plane's band above 1 kHz held, +1.03 dB. **This is Bernardo's decision, not taken
here** (the card's stop rule): whether the V8s get a cheaper tier with these misses stated,
or keep low and the "needs a faster machine" of stage 118.

The speed side, timed today with the gcc build (w64devkit, no LTO: slower than the MSVC
figures of stage 118), held at 3000 rpm wide open, best of three interleaved runs, pinned to
the fast cores:

| Preset | low (24 kHz pipes) | 19.2 kHz pipes | 16 kHz pipes |
| --- | ---: | ---: | ---: |
| cross-plane V8 | 1.36 | 1.52 | 1.51 |
| flat-plane V8 | 1.47 | 1.53 | 1.60 |

At 19.2 kHz both reach the 1.5 target on this machine, today. (The laptop's speed moves by
about 10 % from day to day: the same low cells read 1.28 and 1.37 times with MSVC in the
first speed check of this stage, and 1.46 and 1.60 in the second, an hour later. Only the
ratios inside one run mean anything.)

The smoother itself costs nothing measurable: `tools/speed_check.py df1fc20 --quality low
--rounds 5`, the machine otherwise idle, gives a median of −0.2 % (the worst cell −0.9 %). A
first run had read the six at low 13 % slower, with the machine busy; the second read 1.80
before and after.

## The A/B switch (problem 1)

`RB_PARAM_VARIANT`, 0 to `RB_VARIANT_MAX` (7): live, an integer, a setting (not a driver's
control), kept through a rebuild, no new generation of its own. It lands in `sim->variant`,
where code under listening reads it. Nothing reads it yet, so every number plays the accepted
sound bit for bit (`test_api_variant` renders 0 and 7 in fresh sims and compares them).
066's card adds the key and a blind mode; 128 is the first stage that puts candidates behind
it. It is appended to the parameters, so old ids hold; the snapshot is unchanged (parameters
aren't in it).

## macOS on every push (problem 9)

The workflow's macOS job had never run: GitHub's schedule hadn't fired. Run by hand, **it
failed at once**: `rb_bench_render` pinned itself to cores with `sched_setaffinity` and
`cpu_set_t`, which are Linux's, under a `#else` that also caught macOS. Now that code is for
Linux only; elsewhere `--cores` says it could not set the affinity (macOS has no call that
keeps a process to chosen cores).

The job now runs on every push: the `cmake`, `host` and `addon` suites with Apple clang (the
core and its tests, the real-time host on miniaudio's null device, the Node-API add-on). The
Apple-clang-against-gcc parity step stays weekly and on demand. Green on the trial branch.

## The long fuzz (problem 5)

A `fuzz` job, weekly and on demand: 120 random engines at the low quality, 120 at
oversample 2 and 60 at the high quality, 10 s each, from seeds that move with the run number,
every engine field drawn from its whole range (the ranges the editor will offer). The fuzzer
already drew from `rb_engine_field_min/max`; what was missing was enough of it. Its first run
(the trial branch, 300 engines) passed. A failure names its seed:
`rb_cli fuzz --sets 1 --seed N` replays it.

## Rules as checks (problem 10)

- **The push hook** (`.githooks/pre-push`): the quick suites (`native python host`) before any
  push that sends commits; `RB_SKIP_PREPUSH=1` skips it, `RB_PREPUSH_SUITES` chooses. It
  checks that the Python it finds really runs: on Windows `python3` can be the Store's stub.
  Enabled here with `git config core.hooksPath .githooks`; docs/workspace.md says how. Tested
  by `tools/test_pre_push_hook.py`, which runs the hook with a real `sh` against a stand-in
  runner.
- **CLAUDE.md split.** Its "Core (C)" section (257 lines of module reference and stage
  history), the tools' list and the sound's measured numbers moved verbatim to
  `docs/architecture.md`; CLAUDE.md keeps the rules (rewritten as a list of what holds
  everywhere) and points there. 494 lines → 243 (and 336 in docs/architecture.md); nothing was dropped.
- **Planted bugs survive re-indenting.** A bug's text is matched as written first; failing
  that, with any run of spaces and line breaks standing for any other (words still have to
  match, and the lines around keep their indent). The review had suggested anchoring bugs to
  functions; reading `plant_bugs.py` showed that a move inside a file never broke a bug (the
  text was already found anywhere in it), so that would have gained nothing. The review now
  says so.
- **The old browser page's e2e test** runs only when named (`run_tests.py e2e`), since the app
  replaced the page; CI never ran it anyway.

## The stray folders (problem 11)

Two empty folders at the repo's root since 2026-11-01, named like
`C:UsersbernaAppDataLocalTemp…build`. Windows forbids a colon in a name, so MSYS wrote it as
the private character U+F03A, which is why `git status` and `ls` showed them differently. Both
were empty and are gone. `tools/test_repo_root.py` fails if one comes back. The tool that made
them wasn't found: every tool in `tools/` today passes make forward-slash paths, so it was one
fixed since.

## The references (problem 2)

See RESEARCH.md, "2026-11-15: the first known engine to compare with". In short: the Honda
F20C's cycle repeats 16–19 dB less exactly below 500 Hz than our four's at the same speed, the
one difference the two set-ups can't explain; it is stage 128's starting point. No other
known, naturally aspirated engine with a free-flowing exhaust was found under a licence that
allows copying; Freesound's open-header V8 parade (CC0) went in for its brightness only.

## Tests

- **C:** `test_audio_smoother` (the interpolator alone: refusals, steadiness, a ramp, a missing
  pipe step), `test_audio_listener_smooths_slow_pipes` (the 15 kHz copy, with and without),
  `test_audio_sim_smooths_only_slow_pipes` (every quality, mono and stereo, a rebuild; the
  four at 16 kHz pipes, copies 20 dB weaker smoothed), `test_api_variant`,
  `test_audio_variant_reaches_the_sim`. 274 tests in all.
- **Python:** `test_repo_root.py`, `test_pre_push_hook.py`, `ReplaceOnceTests` in
  `test_plant_bugs.py`, `ChosenSuitesTests` in `test_run_tests.py`, `test_stage127_bugs.py`.
- **The whole run** (`python tools/run_tests.py`, Windows): every suite passed (native,
  wasm, wasm-c, python, web, cmake, host, addon, app, app-e2e; e2e on request, not run: no
  change touched `web/` but the rebuilt core, which the web suite checks).
- **CI:** the trial branch's run (before the last fixes) was green on Linux, the sanitizers,
  ThreadSanitizer, macOS and the fuzz, and failed on Windows only on the stale WebAssembly core,
  since rebuilt. The run on `main` after the push is checked after the commit (CODEX.md).

## Planted bugs

`tools/bugs/stage127.py`, 21 bugs in the core (`python tools/plant_bugs.py 127 WORKDIR all`):
all 21 caught. One was missed at first: `scene-never-feeds-its-smoothers` silenced tailpipe 0
in a stereo scene, and the test, which asked only that the treble copies be weaker, passed
(silence has no treble). `test_audio_sim_smooths_only_slow_pipes` now also asks that the
firing orders (100 Hz to 2 kHz) be as loud smoothed as not, within 1 dB, and catches it.

| Bug | Caught by |
| --- | --- |
| smoother-taps-not-normalised, -accepts-one-step, -accepts-too-many-steps, -delay-wrong | `test_audio_smoother` |
| smoother-phase-never-moves, -push-keeps-the-phase, -taps-read-backwards, -history-not-shifted | `test_audio_smoother`, `test_audio_listener_smooths_slow_pipes` |
| smoother-no-window, listener-smooth-ignores-a-refusal | `test_audio_listener_smooths_slow_pipes` |
| listener-hears-the-steps-anyway, listener-never-feeds-the-smoother | `test_audio_listener_smooths_slow_pipes`, `test_audio_sim_smooths_only_slow_pipes` |
| sim-smooths-48k-pipes-too, sim-never-smooths, scene-hears-the-steps-anyway, scene-never-feeds-its-smoothers, scene-smoothers-left-off | `test_audio_sim_smooths_only_slow_pipes` |
| variant-needs-a-rebuild, variant-takes-fractions | `test_api_variant` |
| variant-never-reaches-the-sim, variant-is-a-control | `test_audio_variant_reaches_the_sim` (and `test_api_variant`) |

`tools/bugs/stage127_python.py`, 14 bugs in the hook and the tools: all 14 caught. Not
planted: the hook's check that a found Python runs (only a machine with the Store's stub can
catch its removal).

## What is left

- **Bernardo's decisions:** the V8s' tier (above), and the project's licence (README).
- **Stages 128, 129 and 130** are planned (docs/roadmap/14-review-fixes.md), and 066, 068 and
  069's cards carry the review's items for them.
- **The F20C comparison is one engine** in a perspective no listener place matches; the preset
  wasn't given its geometry. Stage 128 or 039 does that.
- The macOS job runs the host on the null device only; a real Mac's device is stage 126's.
