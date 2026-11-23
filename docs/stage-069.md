# Stage 069: the first playable app, qualified on Windows (the part without Bernardo)

Worked on 2026-11-22/23 by Claude (Opus 5.5) overnight, with Bernardo asleep. The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-069). It stays 🟡: its real-device runs, his
listening and the pre-release are his (below, "What is left").

**The question:** does the app hold up on Windows (deadlines, underruns, memory) for a long
drive, with the window busy, and what are its defaults: latency, rates, tier, budgets? And
before that, what Bernardo found on 2026-11-22: a rising high-pitched tone with the engine off,
the vacuum gauge flicking to "Boost", and underruns on battery.

## The gaps before it

PLAN.md's open rows and notes:
- **Bernardo's report of 2026-11-22** (the tone, the gauge, the underruns): fixed here first,
  they are this stage's ground.
- **066's device checks:** Bernardo's, with this stage's real-device runs.
- **118's misses** (the four 1.05 dB at idle, the V8s' lean tier and its blind listening):
  sound tolerances, untouched; the listening is his.
- **131: V12-V16 can't play live** (→ 129): stated as a limit below.
- 039, 041, 042, 014: phase B's and old stages': none touches this.

## What Bernardo found, and why

**The tone with the engine off.** A freshly built engine (every rebuild while editing) starts
with its gas at 298 K against walls held at a warm engine's temperatures (the exhaust's 550 K,
the cylinders' 430 K: `EXHAUST_WALL`, `wall_temperature`). The pipes' heat transfer follows the
flow, St cp |mdot| / A (pipe.c): in still gas, the build's own small oscillations pump heat in
where the flow swings, the heated sections drive the oscillation (a Sondhauss tube: a pipe with
a hot section, closed at one end by a valve), and it grows until the gas nears the walls'
temperature, then dies:

| Still engine, never started (lean) | Quietest | Then | Peak | Gone (< -120 dB) |
| --- | ---: | --- | ---: | ---: |
| Inline 4 | -119 dB at 6.6 s | grows about 3 dB/s | -51 dBFS at 24 s, about 800 Hz | about 70 s |
| Inline 3 (from the four) | -138 dB at 10 s | grows about 13 dB/s | -54 dBFS at 24 s | about 42 s |

(dBFS here is the core's output before the app's level, about 10 dB under it.) After a real run
the gas is already warm: switched off after 6 s, the four, the three, the cross-plane V8 and the
boxer fell under -100 dB in 2.5-5 s and never came back above -101 dB in the next 40 s. It's the
core's (a still gas's heat transfer is natural convection, not a flow's), and changing it
changes every render: **left for phase B**, named in PLAN.md. In the app it no longer plays (the
sleep, below).

**The gauge flicking to "Boost".** A fresh build's manifold sits about 60 Pa above the
surroundings for its first seconds (101 385 Pa against 101 325), and the readout's label was the
difference's sign. Fixed below.

**The underruns.** On battery, the i7-1355U held at 1.7 GHz, his LS7 cross-plane V8 at the
lowest latency (10 ms) rendered at 66 % of real time with the engine off and underran; plugged
in, it stopped. Two causes: a stopped engine cost as much as a running one, and 10 ms left no
room for the system holding the render thread back. Both are answered below.

## What was built

**The core: a stopped engine sleeps (`RB_PARAM_REST`).** 0 by default (offline renders stay as
they were, bit for bit); the app's builds set 1. After each block the core counts it as quiet
when the crank is still, the starter off, no dyno on and the block's peak under
`RB_REST_LEVEL` (-100 dB); after `RB_REST_HOLD_S` (1 s) of quiet blocks the render writes
silence without stepping the model, until anything is set on the sim (every setter counts:
a parameter whose value changes, a field, the scene, the dyno) or it is rebuilt. The level is
set so that every engine measured falls asleep before the fresh build's tone starts (1-5 s
against 6 s or more): a sleeping engine never plays it. Asleep, it costs nothing; its slow
state waits as it was. `rb_resting()` says whether it sleeps.

**The window:**
- **"Can't keep up" offers the right cure** (`keep-up.ts`): when the render takes 85 % of real
  time or more, a cheaper quality (as before); when it keeps up but blocks arrive late, a
  longer latency, the next one up. On battery it says so ("Can't keep up (on battery)", and
  plugging in in its tooltip): the main process's `powerMonitor`, through the bridge's new
  `onBattery`.
- **The boost-or-vacuum readout** (`manifold.ts`): "Boost" only 0.5 kPa or more above the
  surroundings; below, "Vacuum" (0 near the surroundings).

## The defaults, recorded

| | Declared | Where it comes from |
| --- | --- | --- |
| Windows | 10 and 11, 64-bit (x64) | Electron 45's own support (Windows 10 and later); qualified here on Windows 11 Home (build 26300), the i7-1355U laptop (Bernardo's baseline, `machines.md`); Windows 10 untested: a stated limit |
| Sample rates | 48 kHz, or 44.1 kHz when the device prefers it | the host (stage 123: `rb_host_pick_rate`); the null backend 48 kHz |
| Latency | 20 ms by default (the host's ring target), 10-80 ms offered; the warning offers the next one up when blocks arrive late | `control.ts` `LATENCY_MIN_MS`/`_MAX_MS`, the host's default; this stage's `keep-up.ts` |
| The tier | each preset's real-time default: the four balanced, the six and the boxer low, the V8s lean; a layout's from `rb_layout_quality` | `presets.c` (stage 118, the V8s Bernardo's choice of 2026-11-16); its demand on the baseline below |
| Snapshots | 60 a second, at most 64 KiB each (3.75 MiB/s); the app uses about 37 KB/s, 1 % of it | contracts/runtime-v1.md, measured in stage 124 |
| Commands | a live throttle every 50 ms (the dead-man beat), the rest on a user's action; a rebuild prepared off the thread, given up after 30 s | stage 066's `throttle-guard.ts`, `PREPARE_TIMEOUT_MS` |
| A stopped engine | asleep after 1 s of quiet: no render cost | this stage |

## The evidence

**The tone.** `tools/engine_off_probe.c` (`rb_engine_off_probe`) builds a stopped engine as the
app does with every editable field at either end of its range and renders 30 s. The inline 3
(from the four): **without the sleep, 61 of the 78 builds** were louder than -80 dBFS after the
first 3 s (-76 to -50 dBFS, all at 21-27 s: the tone); **with it, 1**: a 2 m header still ringing
down from its build (loudest at 0.95 s, falling from there), asleep later. In the tests:
`test_api_rest_keeps_a_fresh_engine_from_singing` (without the sleep the four sings over
-70 dBFS; with it, no preset nor the inline 3 gets back over the rest's level after 3 s), and
`test_api_rest_sleeps_a_still_engine_only`, `_wakes_on_anything_set`,
`_never_touches_a_running_engine` (asleep, it renders exactly what a sim without the rest does
until it sleeps, and a running engine, even turned down to silence, never sleeps).

**Offline renders unchanged:** `tools/cli_parity.py`, 11 of 11 cases byte-identical to the last
commit (gcc). **Speed:** `tools/speed_check.py HEAD`, median -0.1 % (15 cells), inside the 3 %.

**The long drives (the card's 15 minutes), on the i7-1355U on mains, Windows 11, nothing else
running, the null backend** (never a sound device: miniaudio's null device, paced in real time):

| Drive | Latency | Render, of real time | Longest block | Underruns |
| --- | ---: | --- | ---: | ---: |
| `rb_host_stress`, every preset at its tier, swapped each minute | 20 ms | mean 51 % | 18.5 ms | 116 (about 1 every 8 s, in every preset alike) |
| The same | **40 ms** | mean 50 % | 13.0 ms | **0** |
| The four at low only (the cheapest), 3 min | 20 ms | mean 33 % | 7.6 ms | 5 |
| The app (e2e, `stress.spec.ts`): the cross-plane V8 at lean, the 20 s cycle, **the window drawing a heavy animation** (3000 strokes and 6 ms of arithmetic a frame, 116-118 frames a second) | 20 ms | 66-74 % | | 506 |
| The same | **40 ms** | 66-73 % | | **3** |

Memory stayed flat (3.3 MB, growth after 30 s under 0.1 MB); the render thread made no
forbidden call. **What it says:** at 20 ms the null device itself starves the ring: the cheapest
engine, whose longest block (7.6 ms) is far inside 20 ms, still underran, about as often as the
V8s. Its consumer wakes on Windows's coarse timer and takes its periods in bursts, so its
underruns at 20 ms say nothing about the engine, and stage 123's 15 minutes on Bernardo's real
device at 20 ms had none. At 40 ms the host alone is clean. The busy window takes the render from
about 52 % to 66-74 % of real time (they share the laptop's two fast cores) and leaves 3
underruns in 15 minutes at 40 ms. **For stage 070** (the 3D view): a window drawing every frame
costs the render about a third more time; the measure the card names (the window's frame rate
capped while the ring is under half) isn't built, because what drives these runs' underruns
can't be told apart from the null device's own on this backend: it needs the real-device runs
below. **The default stays 20 ms** (the real device's evidence), and the warning now offers more
latency when the blocks arrive late.

**Planted bugs:** core (`tools/bugs/stage069.py`) 8 of 8: the rest on by default, sleep at once,
a loud block counted as quiet, a turning crank put to sleep, a parameter, a field or a rebuild
that doesn't wake it, a rest level too low to sleep before the tone (two missed at first, the
tests made sharper: the quiet stretch measured from the last loud block, and a rebuild with
nothing set). TypeScript (`app_bugs_069.py`) 8 of 8: the advice never "too slow", the latency
offered going down, the battery unsaid, the bar never offering latency, boost on a hair, a
negative vacuum, any truthy battery answer, builds without the rest.

**All suites** (`tools/run_tests.py`, on mains): native, wasm, wasm-c, python, web, cmake
(MSVC), host, addon, app, app-e2e and layouts pass (after `make wasm`, stage 124's planted bug
retargeted to the app's new build parameters, and the sandbox test's list of the bridge's
names).

## What is left (Bernardo, on his machine)

1. **The 15-minute drive on his real devices** (the speakers and his headphones), from a
   terminal in the repo with the add-on's build (`python tools/run_tests.py addon` makes it):
   `C:\Users\berna\rb-build\tests\cmake\rb_host_stress.exe --backend default --minutes 15 --quality tier`
   (it plays the engines: the volume low), then the same with `--target-ms 40`. The card asks
   zero underruns at the default latency.
2. **The app with the window busy, on the real device:** play the cross-plane V8 for 15 minutes
   with the dyno panel's curves running, the "Underruns" readout in sight; then the same on
   battery (the warning should say so and offer a longer latency).
3. **Listening:** a start, a blip, a pull and a long coast on each preset (and 066's checks:
   his Razer keyboard's analog key, unplugging headphones, sleep and wake).
4. **The pre-release:** his decision (the card's), the installer as a GitHub pre-release.
5. **The tone itself, in the core** (phase B): a still gas's heat transfer (natural
   convection, not St cp |mdot|), or walls that start at the gas's temperature on a fresh
   build. Either changes every render: his listening decides it.

Then 069 → ✅.

## Files

New: `tools/engine_off_probe.c` (`rb_engine_off_probe`, in the Makefile and CMakeLists.txt),
`app/src/ui/lib/keep-up.ts`, `manifold.ts`, `app/tests/keep-up.test.ts`,
`app/e2e/stress.spec.ts` (on request: `RB_E2E_STRESS_MINUTES`), `tools/bugs/stage069.py`,
`app_bugs_069.py`. Changed: `core/include/rb.h` (`RB_PARAM_REST`, `RB_REST_LEVEL`,
`RB_REST_HOLD_S`, `rb_resting`), `rb_internal.h`, `rb_sim.c`, `rb_params.c`, `rb_drive.c`,
`rb_engine_api.c`; `tests/test_api.c`, `test_main.c`; the Makefile's wasm exports and the wasm
core; `app/src/engine/control.ts` (`rest: 1`), `fake-addon.ts`, `main.ts`, the preload, the
bridge, the store, the top bar and the centre panel, their tests; `docs/architecture.md`;
stage 124's app bug table.
