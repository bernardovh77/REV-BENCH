# Stage 139: the network's parts against four-pole theory

Worked on 2026-11-24 by Claude (Opus 5.5), overnight, after 138 (the card,
[docs/roadmap/15-exhaust.md](roadmap/15-exhaust.md#stage-139)).

**Status: 🟡** — the side branches and the plain chamber hold their theory; an extended-tube
chamber and two chambers in series hold it below 320 Hz and within 3 dB to 1 kHz respectively,
with the differences above stated and open.

## The gaps before it

| Gap | Could it influence this stage? |
| --- | --- |
| 050 (the engine's grid holds the cans to 450–750 Hz) | Yes, for the engine; the bench here runs at the pipes' finest stable cells (2.5 cm at 48 kHz pipes), as 050's did |
| 136 (a coupled renderer for the cans' upper band) | No: this is the production pipes' own accuracy |

## The bench

`tests/test_parts.c`, stage 050's transmission-loss bench made general: a 100 Pa pulse down a 2 in
pipe into the part and on down another, both soaked up by packed sponges, the part's TL the
spectrum past it over a plain pipe's. The parts are joined as the network joins them
(engine_build.c's `join_end`): a side branch's end into the tapped pipe's middle cell, a closed
end left alone, a pipe into a can's end cell. The theory is four-pole (`tests/fourpole.h`, now
shared with test_mufflers.c), of the geometry the solver is given: its end correction at a
joined end (0.3 diameters), and a can's pipes opening at its end cells' centres (half a cell in:
`can()`, a duct one cell shorter between two half-cell volumes).

**Compared by third-octave bands.** Each band's transmission energy-averaged over 5 Hz steps,
the bench's and theory's. A band of noise is what a listener hears go through; point by point, a
sharp peak a few hertz off is tens of dB off (the first version of these tests failed that way,
the bench and theory agreeing everywhere but at the peaks). **0.6 s records:** a cascade's low
resonance rings past 0.2 s, and a truncated ring moved the extended chamber's pocket resonance
from 440 to 495 Hz.

## The parts

| Part | Bands within | Its resonance | Lowest TL |
| --- | --- | --- | --- |
| Helmholtz resonator (1 L, 3 cm neck 6 cm long, tapped half-way) | **0.95 dB** to 1 kHz | 165 Hz, theory 165 | −0.26 dB |
| Quarter-wave tube (50 cm of 4 cm, closed) | **0.59 dB** to 1.2 kHz | 170 Hz, theory 170 | −0.25 dB |
| Expansion chamber (15 cm bore, 50 cm) | **1.54 dB** to 1 kHz (worst: 63 Hz, where the sponges absorb least) | — | 0.01 dB |
| Extended-tube chamber (inlet 10 cm in, outlet 15 cm) | **1.78 dB** 80–320 Hz; **8.65 dB** to 1 kHz (open) | the outlet's pocket 440 Hz, theory 450 | 1.47 dB |
| Two chambers in series (12 cm × 30 cm, 30 cm link, 15 cm × 45 cm) | **3.00 dB** to 1 kHz (open) | — | 2.68 dB |

**What the side branches say:** a pipe tapped into another's middle cell, ended by a volume or
closed, is the lumped or ducted side branch of theory: the Helmholtz resonator's and the tube's
peaks exactly where theory puts them. The solver's end correction (0.3 diameters a joined end)
is a little short of a real neck's (about 0.85 radii at each end, near flanged): a real resonator
of this geometry rings at 158 Hz, 4 % below the solver's.

**What is open:**
- **The extended-tube chamber's 400 and 800 Hz bands** are 6–9 dB off theory, where the inlet's
  pocket and the chamber's trough meet. Ruled out: the record's length (0.6 s), the end correction
  (0 to 0.3 diameters tried: the best, 0.2, still 5.8 dB) and the cells (2.5 cm is the floor at
  48 kHz pipes: finer needs faster pipes). Left: the junction cell, where three faces (the inlet,
  the pocket and the chamber's) meet in one cell volume.
- **The two chambers in series** have two sharp peaks on the bench (160 and 240 Hz) that the
  cascade theory doesn't; within 3 dB in bands.

## The noise layer's passes from geometry

A network's parts pass the valves' noise (the band above what the pipes resolve, 1–8 kHz) as
their geometry says, not as three constants:
- **a catalyst:** `PASS_CATALYST`, as before;
- **a can without a core:** an expansion chamber's plane-wave transmission averaged over kL,
  1 / √(1 + (m − 1/m)²/4), m its area over the pipes' either side: one can of m = 9 passes −7 dB;
- **a packed core** (a perforated tube through absorbent): a lined duct by Piening's law (Munjal
  2014; Beranek and Vér), 1.05 a^1.4 P/S dB a metre, the lining's absorption a = 0.9 (fibreglass
  or basalt wool, 2–8 kHz, estimated). The legacy 40 cm straight-through of 52 mm: 22 dB, where
  its constant `PASS_STRAIGHT` gave 11.

**The legacy constant `PASS_CHAMBERED` (−24 dB) stood for a whole multi-chamber muffler.** A
network gives its muffler's chambers and cores, and their passes multiply. A network that calls a
multi-chamber muffler one can is brighter than the legacy (the four's legacy network as its own
description: its valve noise 12.6 dB louder). The legacy engines keep the legacy routing and
constants, bit-identical.

`test_net_passes_from_geometry`: two bores' shares in the ratio of their transmissions (0.6737 and
0.6737); a straight-through twice as long takes Piening's dB again (to 1e-12).

**Why not the source.** The valves' noise efficiency `VALVE_ETA` (5e-4) is not fitted to a car:
stage 115 set it inside IEC 60534-8-3's range for valves and checked it against the whole
reference catalogue across presets whose passes ran from 0.002 to 0.5. It stays.

## Speed

`tools/speed_check.py` against 138 (MSVC, the P cores, 15 cells): the median after/before 0.994
(−0.6 %), within the 3 % tolerance; the worst cell −2.6 % (i6, balanced). The passes are worked
out once, when the noise layer is made: nothing more is done per sample.

## Planted bugs

tools/bugs/stage139.py: 5, all caught.

| Bug | Caught by |
| --- | --- |
| a joined end without its end correction (the Helmholtz neck too short) | `test_parts_helmholtz_side_branch` |
| the end correction doubled | `test_parts_helmholtz_side_branch` |
| an expansion chamber's pass not averaged over kL | `test_net_passes_from_geometry` |
| a packed core passing everything | `test_net_passes_from_geometry` |
| Piening's law with the area for the perimeter | `test_net_passes_from_geometry` |

## Files

New: `tests/test_parts.c`, `tests/fourpole.h`, `tools/bugs/stage139.py`, this doc. Changed:
noise.c (`part_pass`, `part_top`), tests/test_mufflers.c (fourpole.h), tests/test_exhaust_net.c,
tests/test_main.c, PLAN.md, CODEX.md, docs/architecture.md.
