# Stage 134: the inline and boxer eights

Done 2026-11-23 by Claude (Opus 5.5) overnight, with Bernardo asleep. The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-134).

**The question:** can the layouts offer the straight eight and the boxer eight Bernardo asked
for "eventually" (2026-11-18), built and running like the other 28 combinations?

## The gaps before it

PLAN.md's open rows: 069's and 066's real-device runs and listening, 118's misses, the fresh
engine's tone (phase B), the V12-V16's speed (→ 062), 039, 041, 042, 014. None touches what a
layout is built from: safe to leave.

## What was built

- **Offered** (`core/src/layouts.c`, `rb_layout_refusal`): inline 1-6 and 8 (a seven is still
  refused, with its own reason), boxer 2, 4, 6 and 8. rb.h's statement of the set follows.
- **The straight eight:** the core's table already had its order (1-6-2-5-8-3-7-4, the V16's
  banks use it), so the crank comes from the firing times as every inline's does. Its exhaust:
  two manifolds as real straight eights had, the outer four (1, 2, 7, 8) and the inner four,
  which that order feeds every 180 degrees each; one exhaust system.
- **The boxer eight:** even firing like the other boxers; a collector and an exhaust system per
  bank, as the six.
- **The tier:** eight cylinders are lean, the V8s' (`rb_layout_quality`).

The app needed no change: the editor's choices come from the add-on's `layouts()`.

## The evidence

- `test_layout_offers_the_stated_set`: the test's own statement of the set now has the eights;
  every family, count, crank and angle agrees with the core, the inline seven refused.
- `test_layout_keeps_the_presets_part`: the straight eight's manifolds (the outer and inner
  fours) each fed every 180 degrees, one exhaust; the boxer eight's collectors its banks, two
  exhausts.
- **The `layouts` suite** (every combination's whole drive, now 30):

  | | Start | Idle | Load | Limiter |
  | --- | --- | --- | --- | --- |
  | Inline 8 | 0.25 s | 700 rpm (696-700), valve 0.14 | holds 704 (sag 429) | 6810, back to 700 |
  | Boxer 8 | 0.25 s | 700 rpm (696-700), valve 0.14 | holds 705 (sag 434) | 6813, back to 702 |

- **Planted bugs:** stage 131's `inline-eight-not-refused` became `inline-seven-not-refused`
  (the eight's refusal is gone; the seven's is the one to plant): caught.
- **The presets unchanged:** `tools/cli_parity.py`, 11 of 11 byte-identical; the step is untouched
  (no speed check needed: `layouts.c` builds descriptions only).
- **All suites** pass (native, wasm, wasm-c, python, web, cmake, host, addon, app, app-e2e,
  layouts), after the app's tests learned the eights: the add-on contract's offered counts, the
  fake add-on's list, and a refused example now an inline seven and a boxer ten.

## What is left

- Bernardo's listening to the eights.
- They play at lean like the V8s; whether this laptop keeps up with them live is stage 069's
  question on the real device (the V8s at lean ran at about 66 % of real time here).

## Files

Changed: `core/src/layouts.c`, `core/include/rb.h` (the offered set's comment),
`tests/test_layouts.c`, `tools/bugs/stage131.py`, the wasm core, `app/src/engine/fake-addon.ts`,
`app/tests/addon-contract.ts`, `engine-layouts.test.ts`, docs (131, the roadmap's card,
PLAN.md).
