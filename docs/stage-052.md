# Stage 052: the mixed-rate scheduler and its transitions

Worked on 2026-11-23 by Claude (Opus 5.5), after 050 and 051. The card is
[docs/roadmap/04-propagation.md](roadmap/04-propagation.md#stage-052).

**Status: 🟡.** Today's scheduler is measured, and its transitions, block sizes and delays are
tested. The test that catches an inserted feedback sample is in. Two parts of the card wait on
062's grid work (below). The core is unchanged.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 050 | Open for the cans' grid. This stage measures timing, not the cans. |
| 051 | Open for directivity and the jet. These come after the sound leaves the pipes. |
| 118's tier | The pipe rates compared here are the tier's own (48 kHz) and a slower one on the same grid. |
| 066/069, 039, 041, 135, the fresh engine's tone | None touches the scheduling. |

## The scheduler today (engine.c, pipe.h)

- **Two rates:**
  - **The physics rate** (192 kHz by default) carries the cylinders, valves and the pipes'
    end cells at the valves.
  - **The pipe rate** (48 kHz) carries every other pipe cell, the collectors and the junction,
    stepped once every `pipe_every` physics steps.
- **The coupling between them:**
  - Between slow steps, a fast face sees each slow cell's pressure as it is by now (its own
    rates running since the last slow step: `rb_chamber_pressure_now`).
  - Every slow-to-fast boundary has one owner, the face, so the load isn't split or doubled.
- **The grid follows the rate:** cells are `LONG_CELL × min(1, 48 kHz × pipe_dt)`. Pipes at
  48 kHz or slower share one grid; faster pipes have finer cells.
- **The sim around it:**
  - The listener reads the tailpipes' and the mouth's flows each pipe step.
  - The decimator brings the physics rate to the audio rate.
  - Live settings land between blocks.
  - A rebuild makes a new engine, and the host fades between the two (runtime-v1). No state
    is carried across.

## The evidence (tests/test_scheduler.c)

| Card's evidence | Test | Result |
| --- | --- | --- |
| Integrated flux and work, mixed rates against each other | `test_scheduler_pipe_rates_agree_on_one_grid`: the four at 3000 rpm wide open, pipes at 48 kHz (every 4 steps) and 24 kHz (every 8), the same cells | torque, VE (the flux per cycle) and net IMEP within **0.066 %** (gate 0.15 %) |
| Phase and delay | the same test: the tailpipe's volume flow, orders 2, 4, 8, 16 (50 to 400 Hz) by crank angle | within **0.39 dB** and **0.0207 ms** (gates 0.5 dB, 0.03 ms): a slow step's flows are its average, centred half a step back, so 24 kHz pipes run up to half a 48 kHz step later, and no further |
| Catch an inserted feedback sample | planted `tail-flow-a-step-late` (the tailpipe's flow held one more pipe step) | caught: one more step puts the 24 kHz run 0.021 ms further behind |
| Algebraic coupling, no undocumented delay | planted `stale-slow-pressures` (fast faces seeing the last slow step's pressures: stage 10's old coupling) | caught here and by `test_engine_pipes_two_rates` (192 against 384 kHz physics: VE within 0.05 %) |
| Block sizes and transitions | `test_scheduler_blocks_and_transitions_are_seamless`: the four on the sim, blocks of 128 and of 7 frames, through a throttle step and a spark change (live) and a rebuild (a longer tailpipe) | **bit-identical**, sample for sample; both changes take effect (the level falls) |
| A crossfade can't hide a wrong state transfer | the rebuild in the same test | there is no transfer to hide: a rebuild starts a new engine, which the test renders from its first sample |
| Long-run state consistency | `test_exhaust_engine_books`, `test_exhaust_high_rpm_fired_books`, `test_pipe_conserves_everything`, `test_pipe_two_rates_agree` (stages 10-115) | unchanged |
| The pipe rate's effect on the work | `test_engine_pipe_rate` (stage 118): 24 against 48 kHz pipes | within 0.5 % (now 0.07 % measured here, held) |
| No-trace full-fixture cost | stage 135's map (`rb_bench_render`, no taps, no files): every preset and tier | in docs/stage-135.md and docs/stage-118.md |

**Planted bugs** (`tools/bugs/stage052.py`): 3 of 3 caught:

| Bug | Caught by |
| --- | --- |
| `tail-flow-a-step-late` | the grid test |
| `stale-slow-pressures` | the grid test and `test_engine_pipes_two_rates` |
| `a-step-each-block` (an extra physics step at each block's start) | the block test |

## Moving preparation out of the sample loop

Stage 135's profile and the call sites show what the per-step code computes:
- **No preparation of fixed values runs per step,** except Woschni's `log(bore)` (one log per
  cylinder per step, under 1 %; 135's candidate 6). It stays.
- **Already worked out at build time:**
  - the filters' and smoothers' coefficients (`rb_onepole_coef`, the decimator, the scene's
    rooms);
  - the pipes' face constants (`per_length`, `per_area`);
  - the radiation load's discrete coefficients;
  - the gas's species constants (compile time).
- **What's left in the loop depends on the state:** the gas's mixture and temperature, the
  nozzles' pressure ratios, the jets.

So there is nothing of size to move, and nothing was moved.

## What is left (why 🟡)

1. **The common-clock control on the same grid.**
   - Pipes on the physics clock (192 kHz) get cells a quarter as long, so that comparison
     measures the grid as much as the scheduling.
   - Separating the two needs the cell size as a setting of its own: 062's grid work (where
     stage 050's finer cans also wait).
   - Until then, the control here is two mixed rates on one grid.
2. **Resampling at the listener** when the pipe rate isn't the audio rate.
   - The listener holds each pipe-rate value until the next. Stage 118 found that this images
     into the treble at slower pipes (062's lead).
   - At today's tiers the pipes run at the audio rate, so there is nothing to resample.

## Files

- **New:** `tests/test_scheduler.c` (2 tests), `tools/bugs/stage052.py`, this doc.
- **Changed:** `tests/test_main.c`, PLAN.md, CODEX.md.
- **Unchanged:** the core, so the renders are bit-identical.
