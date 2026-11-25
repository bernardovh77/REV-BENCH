# Stage 148: finer cells (the studio quality), and what they showed

Worked on 2026-11-25 by Claude (Opus 5.5) (the card,
[docs/roadmap/16-realism.md](roadmap/16-realism.md#stage-148)).

**Status: 🟡** — the studio quality plays: pipes at 192 kHz, cells a quarter of high's, the
gas's own sound converged to about 3.2 kHz. Candidate 7 (the valves' noise taken away where the
finer grid now carries the gas) is on the A/B switch. But the stage's premise didn't hold: on the
finer grid the **gas itself** is about 10 dB brighter above 2 kHz than Bernardo's 458 clips,
where the high quality's mix matches them. The coarse grid's numerical damping had been standing
in for real losses. Turning the noise layer down can't fix that; stage 152 (new) takes the losses.

## The gaps before it

| Gap | Could it influence this stage? |
| --- | --- |
| 147 (several cores) | No: deferred to the PC (docs/stage-147.md); studio is an offline tier on one core |
| 146 (Bernardo's Polo recording) | Yes, for the judgement: a clean recording at a known place is what separates the missing losses (152). The tier and the candidate don't wait for it |
| 144's candidate 6 (the walls) | Measured here: it doesn't explain the brightness (below) |

## The tier

`RB_QUALITY_STUDIO` (appended: rb.h, rb_sim.c `rb_config_quality`): physics 384 kHz, pipes
192 kHz (every second physics step), so the grid (engine_build.c's `grid_scale`, already scaled
with the pipes' rate) makes the cells a quarter of high's: 1.25 and 1.9 cm. The limits raised for
it:

| Limit | Was | Now | Why |
| --- | ---: | ---: | --- |
| `RB_ENGINE_CELLS` (the pool) | 512 | 2048 | the cross-plane V8 at studio about 900 cells |
| `RB_PIPE_MAX_CELLS` | 64 | 256 | a 2.5 m pipe of studio's cells (the editor allows a 5 m exhaust pipe) |
| `RB_PIPE_RATE_MAX` | 96 kHz | 192 kHz | studio's pipes |

The sim grows from 2.0 to 3.1 MB (no engine is on any stack; the WebAssembly core's malloc grows
its memory). `rb_cli sound --quality studio`; `--pipe-rate` up to 192 kHz. Not offered in the
app: it is far slower than real time (the four 0.19× real time, the 458 about 0.1×, on the
laptop).

**Nothing else moved:** `tools/cli_parity.py` against 145's renders, 11 cases identical; every
preset's ultra render (2 s revving) bit-identical before and after (no preset was squeezed into
the old pool or a pipe's old cap); `tools/speed_check.py HEAD`, the median −1.0 % (one cell, the
six at low, read −15 % once and −0.2 % over 7 rounds: noise).

## The grid converged (the card's first evidence)

`tools/grid_convergence.py` (now `--coarse` and `--fine-cli`: any two grids, or a second build),
each preset held at 5000 rpm wide open, the pipes' sound alone from behind, dB per third-octave
band; the reference a scratch build whose studio has cells an eighth of high's (pipes 384 kHz,
physics 768 kHz):

| Against | 1.6 kHz | 2 kHz | 2.5 kHz | 3.2 kHz | 4 kHz | 5 kHz | 6.3 kHz |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| eighth cells over studio (i4, v8-cross, 458, Polo) | −0.2…1.9 | −0.1…1.6 | −0.2…1.5 | 0.0…2.6 | 0.0…5.3 | 1.7…9.8 | 4.6…14.4 |
| studio over ultra | 0.7…1.8 | −0.4…2.5 | 0.4…4.4 | 3.3…8.7 | 7.9…21.7 | 21…31 | 17…34 |
| studio over high | 2.8…5.5 | 3.7…16.2 | 18.6…34.7 | 34.4…48.5 | 42.7…48.8 | 43.5…62.1 | 42.2…61.2 |

Studio holds within about 2 dB of a grid twice as fine to 3.2 kHz (the card hoped for 5 kHz: at
4 kHz it is 1–5 dB short, at 5 kHz 2–10). Ultra's edge is about 2.5 kHz, high's 1.3 kHz (144).
Raw: `renders/evidence/stage-148/study.log`.

## The finding: the gas is too bright on a converged grid

The whole mix (`tools/compare_real.py`, as stage 115 and 141 measured it), from 3 m behind:

| Revving (0.5–6 s) | >1 kHz | >2 kHz | >4 kHz | slope | crest |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bernardo's 458 clips, brighter (stage 141) | −12.0 / −14.8 | −23.9 / −27.0 | −38.5 / −40.7 | −10.3 / −10.6 | 10.1 / 9.1 |
| ferrari-458, high | −16.5 | −25.2 | −36.6 | −8.6 | 18.0 |
| ferrari-458, studio | −12.3 | **−13.7** | −30.9 | −6.9 | 24.1 |
| ferrari-458, studio, candidate 7 | −12.6 | −13.8 | −31.5 | −7.1 | 24.2 |
| i4, high / studio / 7 | −14.3 / −8.2 / −8.3 | −28.4 / −15.2 / −15.4 | −34.3 / −28.5 / −29.1 | | |
| v8-cross, high / studio / 7 | −14.5 / −12.9 / −16.8 | −16.7 / −15.3 / −19.7 | −22.7 / −22.0 / −25.3 | | |
| polo-tsi, high / studio / 7 | −29.7 / −25.0 / −25.0 | −45.2 / −30.2 / −30.2 | −51.6 / −47.5 / −47.6 | | |

Held at 3000 rpm wide open the same: the 458's highs (>2 kHz) −18.0 at high, −9.2 at studio; the
four's −19.4 and −10.4. The ten revving recordings of the catalogue read −42 to −6 above 2 kHz,
median −20 (stage 115).

**Where it comes from,** each layer alone at studio (3000 rpm held, share of the whole mix's
energy, dB): the four's gas 2–4 kHz −11.2, its valves' noise −20.5; the 458's gas −9.8, its
noise −19.0. The gas carries it, not the noise: with no noise at all studio would still be about
10 dB over the clips.

**What it is not** (the 458 at studio, gas alone, dB against unchanged; a scratch build):

| Change | <250 Hz | 250–1k | 1–2k | 2–4k | 4–8k |
| --- | ---: | ---: | ---: | ---: | ---: |
| the walls' viscothermal loss (144's candidate 6) | | −0.8 | −1.7 | −2.0 | −2.3 |
| the packing twice as lossy | −2.3 | −0.1 | −1.0 | −1.3 | −2.1 |
| the packing four times | −3.8 | −2.3 | −3.7 | −2.6 | −3.4 |
| the catalyst's loss four times | +0.1 | −2.8 | −2.4 | −1.1 | −2.3 |
| the centre route narrowed to 0.3 | −6.5 | +0.6 | +0.9 | −0.7 | +0.6 |
| the three together | −3.5 | −10.3 | −9.2 | −9.7 | −10.2 |

(The walls on the four: −2.5 to −3.4 in every band above 250 Hz.) Each takes the level down about evenly; none
tilts the spectrum. The highs the gas carries on a converged grid are lost in a real exhaust by
something the plane-wave pipes don't have at all: a lining whose absorption rises with frequency,
the perforates' and the monolith's narrow channels, the boxes above their first cross mode,
turbulence. A probe of every pipe's ends (scratch `probe_parts.c`) found the port's pressure
itself 15 dB brighter at 2–4 kHz on studio's grid than on high's: the source's front is steep, and
nothing on the way takes it off. That is stage 152 (new, after this one), judged against
Bernardo's Polo recording at its known place (146): his 458 clips are a video's sound, placed
who knows where (stage 141).

## Candidate 7: the noise layer rebalanced (the card's deliverable)

The exhaust valves' noise (stage 115) was calibrated against real engines' energy above 2 kHz
on the high quality's grid, which carries nothing of the gas above about 1.3 kHz. A finer grid
carries more, and there the calibration counts that band twice. Candidate 7 (`RB_PARAM_VARIANT`
7; noise.c `grid_edge`, `rb_noise.rebalance`): on a grid finer than high's, the valves' noise
passes two first-order high-passes at the grid's measured edge (2.5 kHz at ultra, 3.2 kHz at
studio, straight in the rate's logarithm between): a hand-over as gradual as the grid's own loss
above its edge (2 dB off at twice the edge). On high's grid and coarser it is the accepted sound,
bit for bit. (In the app's candidate picker, 7 plays the accepted sound: the app's tiers are on
high's grid or coarser, and studio is the CLI's.)

Its effect is small, as the finding says it must be: the gas is the bigger part. On the four, the
458 and the Polo at most 0.4 dB above 2 kHz; on the cross-plane V8, whose noise layer is a larger
share, 4.4 dB less above 2 kHz (from −15.3 to −19.7 revving, nearer the catalogue's median).
Kept as a candidate: once 152 brings the gas's highs down to the recordings, the noise above the
edge is what is left to calibrate, and this rule is how.

## Tests

tests/test_studio.c (new):
- `test_studio_cells_a_quarter`: every preset at studio has four times high's cells in every pipe
  (within 4), none capped, within the pool; a 3 m exhaust pipe has 160 cells (40 at high).
- `test_studio_plays_the_largest_engines`: the cross-plane V8 and the 458 run at studio, finite
  and audible.
- `test_studio_carries_the_gas_higher`: the four's pipes alone, 2.5–4 kHz against 100–500 Hz:
  high −51.0 dB, studio −18.4 (at least 10 dB more required).
- `test_studio_rebalance_only_where_the_gas_carries`: candidate 7 at studio takes 17.7 dB off the
  valves' noise at 1–1.6 kHz and 1.1 dB at 10–14 kHz (within 1.5); at high bit-identical.

`test_core_quality`'s table gains studio's rates. Two tests the raised limits outgrew:
`test_pipe_cells` asks a 30 m pipe for more cells than a pipe can have (10 m now fits in 200);
`test_engine_description`'s engine too big for the pool is built on studio's grid (on high's it
now fits, 1863 cells), squeezed to 2034, its runners 110 cells of the 256 they want.

**The checks:** every suite passes (`native` 1231 s, 352 tests); `tools/speed_check.py HEAD` on
the final core, the median −0.9 %; parity identical; the WebAssembly core rebuilt.

## Planted bugs

tools/bugs/stage148.py: 8, all caught.

| Bug | Caught by |
| --- | --- |
| studio's pipes at ultra's rate | `test_core_quality`, `test_studio_cells_a_quarter` |
| the pool left at 512 | `test_studio_cells_a_quarter` |
| a pipe's cells left at 64 (missed at first: no preset's pipe needs more at studio; the test now sets a 3 m exhaust pipe) | `test_studio_cells_a_quarter` |
| the pipes' rate limit left at 96 kHz | all four studio tests |
| the grid refined only to half of high's cells | `test_studio_cells_a_quarter` |
| candidate 7 on high's grid too | `test_studio_rebalance_only_where_the_gas_carries` |
| candidate 7 with one high-pass | the same |
| candidate 7 never heard | the same |

## What is left (why 🟡)

1. **The gas's highs on a converged grid** (stage 152, new): the losses above the plane wave,
   judged against Bernardo's Polo recording (146).
2. **Bernardo's A/B** of candidate 7 (`rb_cli sound --quality studio --variant 7`, against 0).
3. **Speed by tier on the PC** (the card asks for both machines): studio measured on the laptop
   only.

## Files

Changed: core/include/rb.h (`RB_QUALITY_STUDIO`, the variant's doc, the pipe rate's range),
core/src/rb_sim.c, core/src/engine.h (the pool, the rate's limit), core/src/pipe.h (a pipe's
cells), core/src/noise.c and noise.h (candidate 7), native/rb_cli.c and sound.c (the quality's
name, `--pipe-rate`, the variant's help; a help string split under C99's 4095 characters),
native/bench_render.c, host/src/trace.c (the quality's name), tests/test_core.c,
tests/test_main.c, tools/grid_convergence.py (`--coarse`, `--fine-cli`), docs/architecture.md, docs/roadmap/16-realism.md (stage 152),
PLAN.md, CODEX.md, the WebAssembly core. New: tests/test_studio.c, tools/bugs/stage148.py, this
doc. Renders: `renders/evidence/stage-148/`.
