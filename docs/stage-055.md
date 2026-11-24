# Stage 055: engine-specific structure and accessories

Worked on 2026-11-23 by Claude (Opus 5.5). Bernardo asked for the next three stages (055, 056,
054). The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-055).

**Status: 🟡.** The block's modes now come from each engine's own geometry, and each cylinder
strikes them where it sits on the block and as its bank leans. The cam chain and the alternator
have tones of their own. Both are candidates on the A/B switch: `RB_PARAM_VARIANT` 1 (the
structure) and 2 (the structure and the accessories). The accepted sound (0) is bit-identical in
every `cli_parity` case. The card's engine-bay, cabin and outside comparisons need matched
recordings (039), and the candidates need Bernardo's ears (128).

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 042: real injector calibration "→ 055" | The research below found no published level for a port injector's tick. The presets are all port-injected, the click is left at stage 12b's level, and the gap stays open, named. |
| 041: combustion's calibration (→ 059) | The force that strikes the modes is the cylinders' own pressure, whatever the burn. It doesn't change what this stage measures. |
| 053, 052, 050, 051, 135, 118's misses, 066/069, the fresh engine's tone | None touches the block's modes or the accessories. |
| 039: matched references | Needed for the card's bay, cabin and outside comparisons; stated below. |

## The structure before (noise.c, stage 12b)

- **Six modes**, fixed in frequency for an 86 mm bore and moved by √(0.086 / bore):
  - the block bending, 950 Hz;
  - the skirt, 1750 Hz;
  - the liners, 2700 Hz;
  - the head, 3500 Hz;
  - the cam cover, 5500 Hz;
  - the rail, 7800 Hz.
- **Every cylinder strikes every mode alike:**
  - combustion is the change of the pistons' total force;
  - piston slap at each dead centre;
  - the valves seating and lifting;
  - an injector's click.
- **What this means:** a V8 and a four with the same bore have the same block. A cylinder at
  the block's end sounds like one in its middle.

This is what the card's stop rule names: "generic bore-scaled modes".

## The research

**Block modes.**
- **What's published:** the block's bending modes are a beam's. Finite-element and
  experimental modal studies of four-cylinder blocks report them on whole powertrains, with the
  gearbox and mounts, so they sit far lower: the inline-four powertrain of
  [Rakenteiden Mekaniikka](https://rakenteidenmekaniikka.journal.fi/article/download/64943/26409)
  has 39 Hz bending and 48 Hz torsion on its base frame.
- **Bare blocks:** SAE 820438 (an idealized four-cylinder block, computed against measured)
  and SAE 2023-01-1153 (a four with balance shafts, resonant near 5000 rpm) give no bare-block
  numbers in what is public. So the shape of the model is a beam's, and its one fitted number
  is stage 12b's block mode (950 Hz, set then against a Honda V6's recorded share of energy
  above 1, 2 and 4 kHz).
- **Deep beams:** a block is as deep as half its length, so plain Euler–Bernoulli bending
  overstates its modes. Timoshenko's shear correction to first order lowers each by
  √(1 + T (βL)² (h/L)²), with T = (1 + E/(κG))/12 (aluminium, ν 0.33, κ 5/6). Without it the
  boxer's short, wide block rang laterally at 3.4 kHz; with it, 1.6 kHz.

**Timing drive.**
- **Where its whine comes from:** chain meshing at the sprockets: the polygon effect and the
  links' impacts (SAE 2002-01-0451, 951239, 2007-01-2526; see also
  [Graz, ISMA 2004](https://graz.elsevierpure.com/files/3015790/279-ISMA2004-schaffner-hhp-final.pdf)).
- **What it sounds like:** a narrow band at the meshing order, on top of the engine's own
  orders. It is loudest where the mesh frequency meets a mode of the structure.
- **The order:** the crank sprocket's teeth, typically 18–25.

**Alternator.**
- **The whine:** magnetic, at its stator slots' order (36 on a claw-pole machine), times the
  pulley ratio (2.5–3). Its level goes with the current.
- **What's published:** no levels found; an estimate.

**Injectors.**
- **What's published:** a direct injector's tick is the needle's impact, carried by the rail
  ([US 7827964](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/7827964),
  [US 9512798](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/9512798)).
  It is louder than a port injector's because of its rail pressure (about 120 bar against 3).
- **What's missing:** no level for either was found, and SAE 2011-01-1216 compares eight GDI
  injectors at idle without public numbers.
- **So:** the presets (all port-injected) keep stage 12b's click; 042's calibration stays open.

**Fan:** electric on these cars, broadband, and governed by the coolant's temperature, not the
engine's speed. Not modelled; stated.

## What was built (noise.c, the candidate)

**Variant 1: the block from its geometry.**

**The block's dimensions** (from `rb_engine_desc` alone):
- **Length:** the span of the cylinders' positions, plus 0.8 bores past each end cylinder.
- **Depth:** the deck height (stroke + rod) along the most upright bank, plus half a stroke of
  crankcase. Never less than the bore and 60 mm of jacket (a boxer's).
- **Width:** the bore and jacket, or the banks' spread (stroke + rod) × Σ|sin| of their angles.

**The modes:**
- **Three bending modes:** the first vertical, the first lateral (the width as its depth) and
  the second vertical. Each goes as h / L², lowered by the shear correction above.
- **The fit:** they are scaled so the four's first vertical is 950 Hz exactly.
- **The local modes** (skirt, liners, head, cam cover, rail) are panels the size of the bore,
  as before.

**How each cylinder strikes the bending modes:**
- **The shape:** Blevins's free-free shape at the cylinder's place, normalized to an RMS of 1
  over the cylinders.
- **The bank's projection:** its gas force along its axis, so cos of its bank angle for the
  vertical modes and sin for the lateral. A slap is across the bore, the other way round.
- **What follows:** a boxer's combustion bends its block only sideways, its banks opposite. An
  inline's slaps ring the lateral mode, never the vertical. The four's middle cylinders strike
  the first bending mode 1.9 times as hard as its end ones (they sit past its nodes, at 0.224
  and 0.776 of the length).
- **Unchanged:** the impacts themselves (their timing, sizes, spread and random draws) are the
  accepted sound's.

**Variant 2: that, and the accessories.** Both tones' phases follow the crank:
- **The chain:** 21 teeth, 0.0063 Pa at 1 m at 3000 rpm, as the rpm.
- **The alternator:** 36 slots × 2.7, 0.0019 Pa at 800 W of accessories, as their power.
- **Both 10 dB under their first levels** (0.02 and 0.006 Pa): Bernardo heard the whine far
  too loud at the front listener on every engine, and asked for it under half as loud
  (2026-11-23). 10 dB is about half the perceived loudness.

**The switch:** `rb_sim.c` copies `sim->variant` into `n->variant` before each step: 1 and 2
reach the candidate, and every other number plays the accepted sound.

**Each engine's block, as built:**

| Preset | Length | Depth | Width | 1st vertical | 1st lateral | 2nd vertical |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| i4 | 0.426 m | 0.272 m | 0.146 m | 950 Hz (fitted) | 754 Hz | 1713 Hz |
| i6 | 0.589 m | 0.280 m | 0.144 m | 628 Hz | 443 Hz | 1189 Hz |
| v8-cross | 0.502 m | 0.223 m | 0.352 m | 719 Hz | 822 Hz | 1378 Hz |
| v8-flat | 0.482 m | 0.193 m | 0.305 m | 716 Hz | 836 Hz | 1404 Hz |
| boxer4 | 0.287 m | 0.144 m | 0.440 m | 1312 Hz | 1570 Hz | 2459 Hz |

## The evidence (tests/test_structure.c)

| Card's evidence | Test | Result |
| --- | --- | --- |
| Bounded per-engine modes | `test_structure_modes_from_geometry` | every preset's three bending modes as the beam's law from its description (1e-6), its resonators at those frequencies (1e-9), the four's first at 950 Hz; the weights Blevins's shape (an independent copy of his table) times the bank's projection; the force's energy between the vertical and lateral modes kept (mean of cos² + sin² = 1, 1e-9); the four symmetric (first mode) and antisymmetric (second); the boxer's vertical force zero and its banks opposite laterally |
| Bank-aware paths, actual event timing | `test_structure_paths_per_cylinder` | a force step in the four's middle cylinder against its end one rings the first mode as their weights' ratio (1e-9), and the local modes alike (1e-12); the boxer's force never rings its vertical mode; an inline's slaps (the crank posed at TDC, four slaps) ring the lateral mode and never the vertical |
| Event and energy checks | `test_structure_events_and_energy` | on every preset, the candidate's impacts are the accepted sound's, counted and sized, and the generator's state is the same after 0.25 s; the bay's level within 3 dB (measured −0.17 to +0.13 dB at 3000 rpm wide open) |
| Order and modulation statistics | `test_structure_order_statistics` | each cylinder's firing interval in the bay, loudest against quietest: i4 0.53 → **0.72 dB**, i6 0.61 → **1.20 dB**, v8-cross 0.55 → **1.58 dB** (accepted → candidate): the cylinders now differ by where they sit, within 3 dB |
| Repeated-seed consistency | `test_structure_statistics_over_seeds` | 6 seeds of the V8, the bay's level within **0.161 dB** of their mean (the accepted sound's 0.145) |
| Accessories | `test_structure_accessories` | variant 2 minus variant 1 is exactly the two tones: the chain's amplitude at 21 × the crank and the alternator's at 97.2 × within 1 % at 3000 and 1500 rpm, and their energy all of the difference (2 %); a still engine makes none |
| The switch | `test_structure_variant_reaches_the_noise` | `RB_PARAM_VARIANT` 1 and 2 reach the noise through a render; 0 and 5 play the accepted sound |
| The accepted sound unchanged | `tools/cli_parity.py`, 11 cases | **all identical** to the last commit's |

**Planted bugs** (`tools/bugs/stage055.py`): 8 of 8 caught:

| Bug | Caught by |
| --- | --- |
| The first mode's βL wrong | the modes test |
| The shapes not normalized over the cylinders | the modes test |
| A boxer's banks bending the block the same way | the modes test |
| The shear correction dropped | the modes test |
| A slap striking with the force's weights | the paths test (after a first run missed it: the inline slap check was added for it) |
| The chain's order wrong | the accessories test |
| The variant never reaching the noise | the switch test |
| The candidate drawing more random numbers | the events test |

**Kept in step:**
- **Stage 12b's planted bugs:** `slaps-at-tdc-only` and `exhaust-seats-at-evo` were retargeted
  to `impact()`'s new arguments. Both are still caught.
- **Stage 053's reverse-jet check:** under MSVC it failed, measured against each step's forward
  jet. At idle those jets cross almost no drop, so they are rounding either way, and the
  compilers' libms round them differently. It is now measured against the cycle's loudest jet
  (1e-9), and the planted bug is still caught. Stage 053 had been checked with gcc only.

## The fitted constants and their uncertainty

| Constant | Value | Basis | Uncertainty |
| --- | --- | --- | --- |
| The four's first vertical bending | 950 Hz | stage 12b's block mode (fitted then to a recording's energy above 1–4 kHz) | unmeasured for this block; bare four-cylinder blocks' first bending in the open literature is not public |
| Block past the end cylinders | 0.8 bores | front and rear walls, the timing cover's face | ±0.3 bores: about ±15 % on the length's modes |
| Inline width past the bore | 60 mm | walls and jacket | ±20 mm |
| Shear constant T | 0.349 | aluminium, solid section | a ribbed hollow section's κ is lower: the deepest blocks' modes up to 20 % lower again |
| Modes' Q | 10–30 | stage 12b's | unchanged, unmeasured |
| Chain | 21 teeth, 0.0063 Pa at 1 m at 3000 rpm | typical sprocket; level an estimate, now about 33 dB under the bay (10 dB down at Bernardo's ear) | teeth 18–25; level ±10 dB |
| Alternator | 36 × 2.7, 0.0019 Pa at 800 W | claw-pole stator, typical pulleys | order ±15 %; level ±10 dB |
| Injector click | 0.05 Pa (stage 12b) | port injection | uncalibrated (042's gap) |

## What is left (why 🟡)

1. **Bernardo's listening:** variants 1 and 2 against 0 (128's A/B switch, live). The candidate
   that wins becomes the accepted sound, and the other is deleted with its number retired.
2. **Matched recordings (039):** the card's engine-bay, cabin and outside comparisons, and the
   modal constants above, need recordings of these engines with a known microphone. None exist
   yet.
3. **Injector calibration (042's gap):** a port injector's tick has no published level. It
   needs a recording close to the rail (039).
4. **Fan and the cam drive's lower orders:** the electric fan isn't modelled. The chain's
   low-order dynamics (its tensioner, the cam's torque) are the engine's own orders already.

## Speed

The candidate runs only when it is selected. The accepted sound's step gained only the force
it keeps for each cylinder.

`tools/speed_check.py` (MSVC, interleaved, best of 3, the laptop's performance cores, 15 cells)
gives a **median of −0.1 %**, within the 3 % tolerance. The worst cell is v8-cross high at
−1.6 %, the laptop's noise.

## Follow-up (2026-11-24): the block 5 dB quieter

Bernardo's A/B: candidate 1 is "decent", but its ringing is heard as a whine, too loud: "make it
5 dB quieter".
- **The change:** the candidate's block (its modes' sum, variants 1 and 2) times `STRUCTURE_TRIM`,
  0.5623 (−5 dB), in noise.c. The accessories' tones are added after it, at their own levels (10 dB
  under the stage's since 2026-11-23).
- **The test:** `test_structure_events_and_energy` now expects the bay 5 dB under the accepted
  sound, within 3 dB. Measured: −4.87 to −5.17 dB on every preset.
- **Planted bug:** `trim-missing` (the trim left out) is caught by that test.
- **The accepted sound:** unchanged; the trim is the candidate's only.

## Files

- **New:** `tests/test_structure.c` (7 tests), `tools/bugs/stage055.py`, this doc.
- **Kept in step:** `tools/bugs/stage12b.py` (two bugs retargeted), `tests/test_flow_noise.c`
  (its reverse check under MSVC).
- **Changed:**
  - `core/src/noise.c`, `noise.h`: the candidate;
  - `core/src/rb_sim.c`: the variant reaches the noise;
  - `core/include/rb.h`: the variants in use, in the comment;
  - `tests/test_main.c`, docs/architecture.md, PLAN.md, CODEX.md.
