# Stage 050: catalysts, mufflers, perforates and packing

Worked on 2026-11-23 by Claude (Opus 5.5). Bernardo said "keep developing" after hearing that
050 was next, which is the instruction the card's hold after 049 waited for. The card is
[docs/roadmap/04-propagation.md](roadmap/04-propagation.md#stage-050).

**Status: 🟡.** The presets' components are measured against plane-wave theory. They are the
right physics, but today's exhaust grid limits them. The fix, finer cans, changes the sound and
costs the four its tier, so it waits for Bernardo. Three stated limits remain (below). The core
is unchanged.

## The gaps before it

PLAN.md's open rows are 118's misses, 066/069's real-device runs and listening, the fresh
engine's tone, 039, 041, 051, 135 and 014.

- **The fresh engine's tone** lives in the pipes' heat transfer. This stage's bench runs with
  the walls at the gas's own temperature, so the tone can't appear there.
- **039:** there are no measured recordings of these mufflers, so the controls are analytic
  (below), as the card allows.
- **051:** its ends' load is the next element after these components. It is not on this
  bench, where the ends are absorbing sponges.

None of the gaps changes what this stage measures.

## The components today (engine_build.c, exhaust_models.c)

| Component | Model | Presets |
| --- | --- | --- |
| Chambered can | A wide pipe (as wide as its volume over its length); the exhaust pipe opens into its first cell and the tailpipe leaves its last, joined as to volumes; the baffles a restriction (`muffler_area`) and a laminar loss (`BAFFLE_LAMINAR`, 30 Pa per m/s) | the four (12 L, 0.724 m); its centre silencer as a can (`center_volume` 2.6 L, 0.241 m) |
| Straight-through | A pipe packed with a laminar loss on the flow's swings (`PACKING` 1500 Pa per m/s per m, `PACKING_SETTLE` 0.25 ms; pipe.h) | the six (0.52 m), the boxer (and its centre, packed) |
| Catalyst | A restriction (`catalyst_area`) and a laminar loss (`CATALYST_LAMINAR`, 25 Pa per m/s) at the exhaust pipe's end | every preset |
| None | The V8s (rear delete; the flat-plane's estimate) | |

## The bench (tests/test_mufflers.c)

**How it measures:**
- **The path:** a 100 Pa pulse runs down a pipe into a component and on down another pipe.
- **No reflections:** both ends are soaked up by "sponges", three 3.2 m pipes each, packed
  harder outwards. Their round trip is under −40 dB from 50 Hz up.
- **Transmission loss:** what arrives 1 m past the component, divided by what arrives past a
  plain pipe of the same length in its place. Dividing by the plain pipe cancels the solver's
  own propagation.
- **Conditions:** air at 300 K, still, 48 kHz pipe steps (the engine's), linear acoustics.

**The control** is the plane-wave four-pole (transfer-matrix) result:
- the can is a duct of its own impedance, with the baffles' resistance in series;
- the catalyst is a series resistance;
- the packing is pipe.h's stated loss, R / (2ρc) nepers a metre above 1/τ.

The components are the presets' own values, not values fitted to the bench.

## The evidence

| Card's evidence | Test | Result |
| --- | --- | --- |
| Magnitude: the can against an expansion chamber's theory (its domes, its nulls kL = nπ, the baffles' resistance) | `test_muffler_can_is_an_expansion_chamber`, 2.5 cm cells | within **1.81 dB** from 50 Hz to 1.3 kHz (the nulls are where theory puts them, 240 Hz apart); the theory's own dome checked against 10 log10(1 + (m − 1/m)² / 4) |
| A held-out geometry | `test_muffler_centre_can_held_out`: the four's centre can, at the main can's tolerance, never tuned to it | within **1.55 dB** to 1.6 kHz |
| Packing's storage and loss | `test_muffler_packing_is_its_stated_loss` | within **0.75 dB** of its stated attenuation to 1.6 kHz; a steady flow sees none (`test_pipe_packing`, stage 12) |
| The catalyst | `test_muffler_catalyst_is_a_series_resistance` | within **0.081 dB** of a series resistance (0.26 dB, flat) to 1.6 kHz |
| Passivity | every test above | no component is ever louder past it (lowest TL −0.02 dB, the bench's noise) |
| On the engine's own grid (7.5 cm cells) | `test_muffler_engine_grid_limits` | within 2 dB to **450 Hz** in the main can (10 cells), **750 Hz** in the centre can (3 cells), **1.1 kHz** in the packing, **1.4 kHz** across the catalyst (300 K; in exhaust gas at 700 K about 1.5 times higher); above that limit the main can's error is real (the test checks it exceeds 2 dB) |
| Steady back pressure, energy books | stage 10/12's `test_pipe_*` (the joins' velocity heads, Borda-Carnot, the laminar losses), `test_engine_muffler_back_pressure`, `test_exhaust_engine_books` | unchanged |

**The catalyst's laminar constant, against a monolith** (an estimate: the presets' bricks
aren't published):
- **The brick assumed:** 400 cells per square inch, 1.1 mm channels, 75 % open, 0.12 m long,
  0.105 m across, behind a 48 mm pipe.
- **Laminar flow in square channels** (fRe = 56.9): Δp = 28.45 μ L u_ch / d_h².
- **With hot exhaust's viscosity at 700 K** (3.3e-5 Pa s), that is **26 Pa per m/s of the pipe's
  gas, against `CATALYST_LAMINAR`'s 25.**
- **Temperature dependence:** it scales with viscosity, so 18 at 400 K and 33 at 1000 K. The
  model keeps it constant.

**Planted bugs** (`tools/bugs/stage050.py`): all 5 caught:

| Bug | Caught by |
| --- | --- |
| End correction tripled | both can tests |
| Laminar loss dropped at a joined end | the catalyst and grid tests |
| Packing halved | the packing and grid tests |
| Packing never settles | the packing test |
| A pipe joined into a can at the wrong area | the can and catalyst tests |

## What it found

1. **The component models are right.** On fine enough cells, the can is the textbook
   expansion chamber, the packing does what pipe.h says, and the catalyst is the resistance its
   constant says.
2. **The engine's grid is what limits them.** At 7.5 cm the four's main can has 10 cells and
   its centre can 3.
   - **Main can:** the error grows above about 450 Hz (690 Hz in hot gas). The model's nulls
     drift low, and above 900 Hz it makes domes and nulls of its own: 25 dB where theory has
     10, and 0 where theory has 10.
   - **Packing:** its loss rises spuriously above 1.1 kHz (26 dB at 1.45 kHz where it should
     be 7).

   These are artefacts of too few cells per wavelength, not of the component models.
3. **Candidate, class C (stage 135's classes): the cans on finer cells** (a third of
   `LONG_CELL`).
   - **Gain:** it brings the four's cans within 2 dB to their cross mode.
   - **Cost:** the four's render is **10–22 % slower** (w64devkit gcc, interleaved, best of 3: balanced hold 1.73× → 1.36×, low hold 2.37× → 2.06×). That takes the four's
     balanced tier under stage 118's 1.5×.
   - **The sound:** it changes the four's sound above about 700 Hz.
   - **Why it isn't on the A/B switch:** the switch flips live, without a rebuild (stage 127),
     and the grid needs one.

   It is offered as renders for Bernardo's blind comparison, or for 062's tier work.

## Follow-up (2026-11-23, overnight): the finer cans as a build setting

**The decision it unblocks:** the class C candidate above couldn't go on the A/B switch, because
it needs a rebuild. So it is now an engine field:
- **`muffler_resolution`** (`RB_ENG_MUFFLER_RESOLUTION`, 1–4, default 1): the mufflers' and the
  centre can's cells are that many times shorter than the exhaust's.
- **Where it is:** the app's editor offers it in the Exhaust section. A change rebuilds the
  engine, which the host fades across (runtime-v1). Bernardo can try 3 against 1 by ear.
- **Never under the pipes' stability floor:** a wave's travel in a pipe step
  (`rb_pipe_cells_for`), 2.6 cm at 48 kHz pipes. So 3 gives the four's can 27 cells for its
  10, a little under three times.

**The evidence:**
- **Default 1:** all 11 `cli_parity` renders are bit-identical, and the speed check gives a
  median of +0.5 % (MSVC, 15 cells): within the laptop's noise.
- **At 3** (`test_muffler_resolution_builds_finer_cans`):
  - the four's muffler gets 10 → 27 cells and its centre can 3 → 9;
  - its other pipes are untouched;
  - its torque moves −0.00 % and its VE −0.01 % at 3000 rpm wide open: only the cans' upper band
    changes;
  - the description refuses under 1 and over 4.
- **The planted bugs,** in this stage's table, all caught: the setting ignored, the setting
  reaching the tailpipe, the range unchecked.

**What it costs:** the 10–22 % measured above, on the four at 3, when chosen. Bernardo's call:
quality against the tier.

## The limits, stated (what keeps it 🟡)

1. **The grid limit above.** It is real on today's tiers, it costs the four's sound above
   about 700 Hz, and the fix is Bernardo's call (quality against the tier).
2. **The catalyst is a point.** A real one has a brick of about 1 L between two cones, which
   acts as a short expansion chamber with its resistance spread along it: a few dB more loss
   above some hundreds of Hz. Its laminar constant is right at 700 K and moves ±35 % with the
   gas's temperature, which the model holds fixed.
3. **The packing's shape is the model's own, not a measured silencer's.** Real absorptive
   straight-throughs (glass fibre behind a perforate) have a loss that rises with frequency to
   a broad peak, set by the fibre's flow resistivity (Delany–Bazley) and the perforate's
   impedance. The model rises as (ωτ)² to a flat R / (2ρc) per metre, which here is 5–7 dB
   over 0.52 m at 1–1.6 kHz. Matching a real silencer needs a measured transmission loss of
   one like the six's or the boxer's (039's recordings, or a published one of the same
   construction). The internals stay marked as estimates (exhaust_models.c).

## Files

- **New:** `tests/test_mufflers.c` (5 tests), `tools/bugs/stage050.py`, this doc.
- **Changed:** `tests/test_main.c`, PLAN.md, CODEX.md.
- **Unchanged:** the core, the host and the app, so the renders are bit-identical. The speed
  check, `make wasm` and the parity tools have nothing to compare.
