# Stage 133: the rev limiter's character and a 2-step

Done 2026-11-23 by Claude (Opus 5.5) overnight, with Bernardo asleep. The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-133).

**The question:** can the driver change how the revs bounce at the limit, slower or faster, and
hold a 2-step (a second, lower limit for launching) on a key, with the sound still the core's?
(Bernardo's request, 2026-11-22.)

## The gaps before it

PLAN.md's open rows: 069's and 066's real-device runs and listening (Bernardo's), 118's misses,
the fresh engine's tone (phase B), 039, 041, 042, 014. None touches the limiter: safe to leave.
069's dependency is on its app work (the editor, the driver's keys), which is done.

## What was built

**The core** (engine fields, live, in the editor's new "Rev limiter" section):

| Field | Default | Range | What it does |
| --- | ---: | --- | --- |
| `limiter_gap` | 100 rpm | 20-1500 | how far below the rev limit the crank falls before the hard cut lets go (`rev_limit - rev_limit_resume`, now a field; it follows the rev limit) |
| `limiter_band` | 150 rpm | 0-1000 | the soft cut's band (stage 12c's `LIMIT_SOFT`, now a field; 0: the hard cut alone) |
| `limiter_cut` | spark | spark, fuel | what the limiter cuts: the sparks (the charge leaves unburned) or the injectors (`RB_LIMITER_FUEL`: a cylinder already fuelled still fires, so it bites a cycle later) |
| `launch_rpm` | 4000 rpm | 1500-12000 | the 2-step's limit |
| `launch_gap` | 100 rpm | 20-1500 | its own gap ... |
| `launch_band` | 0 rpm | 0-1000 | ... and soft band (0: a hard stutter) |

`RB_PARAM_LAUNCH` (a driver's control, live): while 1, the engine computer holds the 2-step's
limit, gap and band instead of the rev limiter's, if its limit is lower. The fuel cut's skipped
injections count as the limiter's cuts and are its events (`RB_EVENT_LIMITER_CUT`).

**The app:** **L** holds the 2-step while held (let go on blur, like the starter); the editor's
"Rev limiter" section (the rev limit moved there from "Limits and load"), the cut a choice
("Spark cut", "Fuel cut").

## The evidence

**The bounce** (`tests/test_limiter.c`; the four at the low quality, wide open in neutral, its
speed read every millisecond for 2 s at the limit):

| Setting | Speed at the limit | Bounces a second |
| --- | --- | ---: |
| Gap 50 rpm, no soft band | 6730-6837 rpm | 36 |
| Gap 200 rpm | 6582-6812 rpm | 16.5 |
| Gap 600 rpm | 6186-6838 rpm | 8 |
| The default (gap 100, band 150), spark cut | 6660-6831 rpm, 339 cuts | |
| The same, fuel cut | 6654-6835 rpm, 337 cuts | |
| No band against a 400 rpm band | mean 6759 against 6699 rpm, top 6829 against 6811 | |
| The 2-step, armed (4000 rpm) | 3879-4081 rpm | 26 |

Monotonic as the card asks: a wider gap bounces deeper and slower. The 2-step at 3000 rpm with a
400 rpm gap bounces deeper than the default; set above the rev limit, the rev limiter holds; let
go, the engine revs on past 6000. Stage 067's `test_editor_every_field_has_an_effect` drives
the six new fields (the 2-step's armed) and measures an effect for each.

**Offline renders unchanged** with the defaults: `tools/cli_parity.py`, 11 of 11 cases
byte-identical to the last commit (gcc). **Speed:** `tools/speed_check.py HEAD`, median +0.2 %
(15 cells). **The 2-step's key, end to end:** `app/e2e/launch.spec.ts`
(the app on the real add-on, the null backend): L and W held, the four held under 4400 rpm; L
let go, past 5500.

**Planted bugs:** core (`tools/bugs/stage133.py`) 7 of 7: the gap not kept, the band fixed at
150, the 2-step never holding, holding above the rev limit, ignoring its gap, its parameter lost,
the fuel cut cutting nothing. TypeScript (`app_bugs_133.py`) 5 of 5: L never let go, held over
a blur, taken on auto-repeat, no Rev limiter section, the cut unnamed. Three older bugs whose
lines moved were retargeted and are still caught: stage 040's `bypass-hard-cut`, 116's
`soft-cut-too-early`, 12c's `soft-band-above-the-limit`.

**All suites** (`tools/run_tests.py`, on mains) pass: native, wasm, wasm-c, python, web, cmake
(MSVC), host, addon, app, app-e2e and layouts. **Found on the way:** the build contract's probe
(`tools/build_contract_probe.c`) zeroed every description field before reading a build file, so
the 2-step's limit read 0 rpm and every descriptor was refused; it starts from the core's
defaults now, every field the contract carries still required (the contract doesn't carry the
limiter: the app's saved builds do, as edits).

## What is left

- **Bernardo's listening:** the bounce at different gaps and bands, the fuel cut against the
  spark cut, and the 2-step (hold L, then the throttle).
- **The 2-step's bangs:** none, as the card's stop rule says: burning in the exhaust is stage
  109's chemistry (anti-lag). A real 2-step usually also retards the spark; not done (the card
  names the revs' rhythm only).
- The cut choice is per engine; a per-cylinder pattern (cutting fixed cylinders) is not offered.

## Files

New: `tests/test_limiter.c`, `app/e2e/launch.spec.ts`, `tools/bugs/stage133.py`,
`app_bugs_133.py`, this doc. Changed: `tools/build_contract_probe.c`, the bug tables of 040, 116 and 12c; `core/include/rb.h` (`RB_ENG_LIMITER_*`,
`RB_ENG_LAUNCH_*`, `rb_limiter_cut`, `RB_PARAM_LAUNCH`), `engine.c`/`.h`, `injection.h`,
`rb_engine_api.c`, `rb_params.c`, `rb_drive.c`, `rb_events.c`; `tests/test_main.c`,
`test_editor_fields.c`; the wasm core; `app/src/ui/lib/editor.svelte.ts`, `driver.svelte.ts`,
`app/src/engine/fake-addon.ts` and their tests; `docs/architecture.md`.
