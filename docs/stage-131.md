# Stage 131: engine layouts, any family and cylinder count

Done 2026-11-18 by Claude (Opus 5.5), at Bernardo's request of the same day. The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-131).

**The question:** can REV-BENCH build a coherent engine from a family (inline, V, boxer), a
cylinder count and a crank, instead of only its five presets, and does every one it offers
start, idle, rev and stop on its own?

## The gaps before it

PLAN.md's open rows: 066 waits on Bernardo's device and keyboard checks, 118 on the four's
1.05 dB at idle and the V8s' lean tier, 039, 041, 042 and 014 belong to phase B or old stages.
None touches how an engine is laid out: all are safe to leave. 118's tiers are what the layouts'
default tiers follow (below).

## What was built

**The core** (`core/src/layouts.c`, `layouts.h`; the public calls in `rb.h`):
- **What's offered:** inline 1 to 6 cylinders; V 2 to 16, even counts, banks 30 to 120 degrees
  apart; boxer 2, 4 and 6. Inline and boxer 8 are refused as "planned" (Bernardo, 2026-11-18).
  The cranks: even firing (every family); a V's common pins (each row's two cylinders on one
  pin, an inline crank: it fires as the bank angle makes it); the V8's cross-plane; a twin's
  180 or 270 degrees. 28 combinations at their usual angles.
- **The crank is designed, not looked up.** Every cylinder has its own pin in the description,
  so an even-firing engine is made from the times its cylinders should fire, each pin worked
  out so its piston is at TDC then. A V12 at 60, a V10 at 72 and a V8 at 90 come out with
  shared pins, as the real ones; a V6 at 60 with its pins split by 60, as GM's. A V fires as
  two inline engines of one bank, each bank evenly. Common pins give the real uneven ones: a
  90-degree V6 fires 90/150 (Buick's), a 45-degree V-twin 315/405 (Harley's), a 90-degree V10
  54/90 (the first Viper's). Where two cylinders would fire at once (a common-pin V12 at 120,
  a V16 at 90) the layout is refused with that reason. The boxer six alternates its banks
  (each fires every 240, as Porsche's), the four fires 1-3-2-4 (Subaru's).
- **The rest of the engine:** what doesn't depend on the layout comes from the preset it
  starts from (bore, stroke, rod, compression, cams, the burn, friction, rev limit, the kinds
  of muffler and pipes); what does is worked out: the headers into one collector, one per bank
  (a V of 4 or more, the boxer six) or the six's front and rear three, each fed evenly; two
  exhaust systems for a V of 6 or more and the boxer six; the air's sizes scaled by
  displacement; the running set from the size, fitted to the presets (flywheel 0.08 kg m^2 per
  litre, starter, accessories, idle speed by count from 1100 for a single to 600, the idle
  valve and gains keeping the four's loop). Each preset's own layout gives the preset back,
  whole, and its crank (pins, banks, firing order) is regenerated from any other preset.
- `rb_load_layout(sim, preset, family, cylinders, crank, bank_angle)` and its metadata; the sim
  keeps the layout through resets and quality changes; `rb_cli sound --layout F:N[:CRANK[:ANGLE]]`
  and `rb_cli fuzz --layouts`.

**The host:** `rb_host_build.layout_*` (none by default), applied after the preset and before
the edits; traces carry a `layout` line.

**The add-on:** a build's `layout`, its tier the layout's; `presets()` gives each preset's own
layout; `layouts()` lists what's offered (family, count, cranks with their angle ranges and
tiers).

**The engine process:** `set-layout` (the preset rebuilt as a layout, one transaction), and a
layout on `apply-edits` and `describe`. A new layout keeps the edits of whole-engine fields
(bore, cams, limits) and drops those indexed by the old layout's cylinders, banks or firing
places (`app/src/shared/layout-edits.ts`, shared with the window). The autosave and the trace
keep the layout; a saved layout the core no longer builds starts the preset as it comes, with a
warning in the log. A layout's level trim follows its size (four cylinders or fewer as the
four's +10 dB, more untrimmed).

**The window:** a Layout section at the top of the editor: the family, the cylinders (only the
counts offered), the crank and a V's bank angle. A change joins the draft: the preview shows
what it makes, or the core's reason it can't, and the draft bar names the edits it drops.
Undo and redo bring back a layout with its own edits. A generated engine is labelled "generic".

**Found on the way (fixed):**
- **The trace recorder** (stage 066's proof that the window's drive is the offline render) wrote
  neither the build's edits (stage 067's) nor, now, its layout: a drive with edits would have
  replayed without them.
- **A pipe far wider than its headers.** The fuzz of random layouts (with random fields on top)
  found three twins whose 131-149 mm exhaust pipe, fed by two 36 mm headers (6.6 to 8.6 times
  their area), lost a pipe cell's state at the slow pipe rates (low and lean; balanced and high
  ran clean). The description check now refuses a collector's pipe over 6 times the area of the
  headers joining it: real collectors are 1 to 2 times; the presets reach 5.7 at most within the
  editor's ranges, so none of their accepted values changes
  (`test_layout_refuses_an_exhaust_far_wider_than_its_headers`). Seeds 131019, 132032 and 132055
  replay it.
- **Valves' padding under MSVC.** A compound literal (`(rb_valve){...}`) leaves the bytes between
  a valve's `count` and its `diameter` to the compiler; under MSVC they held the stack's
  leftovers, so two builds of the same preset differed there and `test_api_engine_fields`
  (setting every field to its own value changes nothing, byte for byte) failed for the four and
  the boxer. Valves are now made by `rb_valve_make`, which zeroes the padding; no value moved.

## The evidence

| Card's evidence | How | Result |
| --- | --- | --- |
| Every combination fires in the stated order and intervals, pins agreeing with the kinematics | `test_layout_fires_as_stated`: each of the 28 at its usual, smallest and largest angle; even ones every 720/n degrees (a V's banks and an odd-rowed boxer's every 1440/n, each collector fed evenly); the known uneven ones (90/150, 315/405, 54/90, the twins); each firing TDC found where the pin's direction meets the bank's, by turning the crank; running, 11 layouts' sparks in order at their angles | pass |
| Every combination starts, idles 30 s without hunting, takes a blip and a load, revs to its limit and stops | `test_layout_every_combination_runs` (the `layouts` suite, 12 parallel shares, 454 s on the laptop) | 28 of 28; every idle within 2 rpm over its last 20 s, the valve 12-19 % open; table below |
| The five presets unchanged | `test_layout_presets_are_their_own_layouts` (whole descriptions); `cli_parity` | pass; `cli_parity` 11 of 11 identical (after every core change, the valve's included) |
| Capacity at 16 cylinders | `test_layout_api`: a V16's 16 cylinders' state, a snapshot, sound, a quality change; the add-on's contract describes a V12 | pass |
| Speed per tier on the i7-1355U | `rb_cli sound --rev`, 8 s, one at a time | table below: V12, V14 and V16 can't play live |
| e2e on Windows | `app/e2e/layout.spec.ts`: the four rebuilt as a V6 from the editor (its header edit dropped and said so), autosaved, undone back to the four with its header, a common-pin V12 at 120 refused in the preview | pass (MSVC's and gcc's builds of the add-on alike) |
| `rb_cli fuzz` on each | `rb_cli fuzz --layouts --quality lean`: random layouts at any angle in range, with random fields on top; 200 sets | 3 failures in the first 120 (the pipe check above), then 80 of 80; every family drawn |
| The sound check | a rev of ten layouts at the high quality for Bernardo (`renders/stage-131/`: the single, three, five, 270-degree twin, 45-degree V-twin, both V6s, V10, V12, boxer six) | his listening informs; phase B and 094 calibrate |
| Planted bugs | `tools/bugs/stage131.py`, `stage131h.py`, `app_bugs_131.py` | core 10 of 10 (`idle-valve-not-scaled` with `RB_LAYOUT_SHARD=23/28`), host 3 of 3, TypeScript 14 of 14; the retargeted bugs of 066, 067, 118, 121, 124 and 127 still caught |

**The drive, every combination** (started from the four; idle speed by count; the limit the
four's 6800 rpm):

| Layout | Starts in | Idle | Lowest after the blip | Back from the limiter | Under load |
| --- | ---: | ---: | ---: | ---: | ---: |
| Inline 1 | 0.57 s | 1100 | 991 | 1093 | 1095 |
| Inline 2 (even, 180, 270) | 0.25-0.36 s | 1000 | 900-965 | 995 | 992-995 |
| Inline 3 | 0.30 s | 900 | 858 | 897 | 898 |
| Inline 4 | 0.27 s | 800 | 759 | 800 | 789 |
| Inline 5 | 0.23 s | 775 | 738 | 775 | 773 |
| Inline 6 | 0.29 s | 750 | 715 | 749 | 748 |
| V2 (even, common) | 0.30-0.36 s | 1000 | 931-965 | 995 | 992-995 |
| V4 | 0.27-0.31 s | 800 | 761-763 | 799-802 | 786-797 |
| V6 | 0.26-0.29 s | 750 | 711-714 | 748 | 748 |
| V8 (even, common, cross) | 0.25 s | 700 | 666-667 | 697-700 | 703 |
| V10 | 0.23 s | 650 | 613 | 649 | 652 |
| V12 | 0.25 s | 600 | 547 | 587 | 614 |
| V14 | 0.24 s | 600 | 551 | 586 | 612 |
| V16 | 0.23 s | 600 | 556 | 597 | 604 |
| Boxer 2 | 0.36 s | 1000 | 965 | 996 | 995 |
| Boxer 4 | 0.27 s | 800 | 759 | 800 | 788 |
| Boxer 6 | 0.29 s | 750 | 714 | 748 | 748 |

Three allowances, each found by the test and stated: a start within a second (the single takes
0.57: one compression to push through each turn and a half), the blip judged 4 s on (at 3 the
V12 is 30 rpm low, coming back), 4 s floored to reach the cut (the V12 needs 3), and dips
measured from the idle's own lowest instant (a single's speed swings 100 rpm inside each cycle).
The first runs also showed a test fault: the load thrown on just before the rev left the idle
valve open as the engine came back from the limiter; the load now comes last.

**Speed** (times real time, the rev drive, w64devkit's gcc build, one at a time):

| Layout | Its tier | At it | At lean |
| --- | --- | ---: | ---: |
| Inline 1 / 2 / 3 / 4 | balanced | 3.73 / 2.26-2.72 / 1.77 / 1.52 | 5.74 / 3.56-3.95 / 2.83 / 2.27 |
| Inline 5 / 6 | low | 1.71 / 1.34 | 1.91 / 1.64 |
| V2 / V4 | balanced | 2.38-2.40 / 1.38-1.39 | 3.60-3.65 / 2.13-2.14 |
| V6 | low | 1.39-1.40 | 1.56 |
| V8 | lean | 1.17-1.25 | |
| V10 | lean | 1.01-1.03 | |
| V12 / V14 / V16 | lean | **0.90 / 0.78 / 0.68** | |
| Boxer 2 / 4 / 6 | balanced, balanced, low | 2.34 / 1.43 / 1.35 | 3.59 / 2.24 / 1.52 |

The core's own speed is unchanged (`tools/speed_check.py HEAD`, MSVC, 20 cells: median +0.2 %).

The V12, V14 and V16 can't play live on the laptop: lean is the cheapest tier, and the app's
"can't keep up" warning will show. A faster tier for them is stage 129's (the cold end of the
exhaust on delay lines); the V10 is at the edge.

**All suites** (`tools/run_tests.py`, on mains power): native, wasm, wasm-c, python, web, cmake
(MSVC), host, addon, app, app-e2e and layouts pass; the graph needs a `graphify update .`
(graphify isn't installed on this machine).

## What is left

- **The V12, V14 and V16 in real time** (above): stage 129.
- **Inline 7-8 and boxer 8:** planned, Bernardo's choice; the core's tables already have the
  eight's firing order (the V16's banks use it).
- **Calibrated engines:** a generated engine is generic. 090-093 add researched V6s, V10s,
  V12s and a boxer six on top of these layouts, and 094 calibrates them.
- **Bernardo's listening** to the renders: phase B's question.

## Files

New: `core/src/layouts.c`, `layouts.h`, `tests/test_layouts.c`, `tools/layout_shards.py`,
`app/src/shared/layout-edits.ts`, `app/tests/engine-layouts.test.ts`, `app/e2e/layout.spec.ts`,
`tools/bugs/stage131.py`, `stage131h.py`, `app_bugs_131.py`.
Changed: `core/include/rb.h`, `presets.c` (each preset's layout), `rb_engine_api.c`,
`rb_internal.h`, `rb_sim.c`; `native/sound.c`, `fuzz.c`/`.h`, `rb_cli.c`; the host
(`rb_host.h`, `sim_build.c`, `trace.c`, tests); the add-on (`rbn_build.c`, `rbn_meta.c`,
`rbn_util.c`, `rbn_module.c`, `rbn.h`, `meta.test.mjs`); the contracts (`addon.ts`,
`protocol.ts`, `process-messages.ts`); `control.ts`, `trace-recorder.ts`, `fake-addon.ts`,
`autosave.ts`; the store, the editor and its panel, `App.svelte`; `tools/run_tests.py` (the
`layouts` suite), `.github/workflows/ci.yml` (weekly), the Makefile's wasm exports, retargeted
bugs of stages 118, 121, 124, 127, 066 and 067; `web/src/wasm`.
