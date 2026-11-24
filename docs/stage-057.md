# Stage 057: whole-engine candidate integration

Worked on 2026-11-23 by Claude (Opus 5.5), overnight, after 054. The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-057).

**Status: 🟡.** No research solver can enter the five-preset engine yet. As the card asks when
an arm hasn't survived, this records why each one fails rather than staging a comparison. What
the stage built is the baseline that any arm must beat once it can enter:
- **Where today's pipes are weak:** each plain exhaust pipe's own numerical loss, on its own
  cells and running gas.
- **The whole drive:** start, idle, a load, the pull to the limiter, the overrun's fuel cut, its
  resume and idle again, on every preset, with the books closing to about 1e-14.
- **Geometry edits:** an edit downstream returns its load to the cylinders.

The core is unchanged.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 050: the cans' grid | The weak-waves map below is the same limit, now for the plain pipes and in each engine's running gas. |
| 054: the components' band | The research arms would carry the cans' high band; none can yet (below). |
| 055, 056 | Their candidates are the noise's and the listener's, not the pipes'. They don't enter this stage. |
| 052: the common clock | The baseline runs on today's clocks. |
| 041, 042, 039, 118, 135, 066/069, the fresh engine's tone | None changes the gates measured here. |

## The arms

| Arm | What it is | Where it stands | Can it enter the engine? |
| --- | --- | --- | --- |
| A0 | today's pipes (finite volumes, 7.5 cm cells at 48 kHz) | the accepted sound | it is the engine: the baseline below |
| A1 | the uncoupled implicit-orifice variant | rejected (043's record) | no |
| A2 | the improved conservative solver with the production gas | rejected in 043 for its upper-band accuracy and cost; 044 and 045 kept it rejected | no |
| B (coupled duct, `experiments/acoustics/coupled_duct.c`) | one conserved state per cell, characteristic streaming downstream, distributed nonlinear and entropy corrections (S2d-B1a) | its bounded proof passes (045, 046, 048), but only for a **uniform, constant-γ duct, sample-aligned, from a still background** | **no:** every exhaust pipe of the five presets runs with a mean flow and meets a chamber or a junction; the arm has no adapter for the production gas's mixture, for area changes, for the engine's chambers, or for a moving background (its README's open list) |

**So no candidate is integrated, and no research selector is added:** a switch with nothing
behind it would be dead code. The arm's next step is the whole-engine adapter that its README
names:
- the production gas's mixture;
- multiports;
- evolving backgrounds;
- temperature and reversal.

That work is a research stage of its own (060's input), and it is gated by the baseline below.

## Where today's waves are weak

**The bench** (`test_baseline_where_the_waves_are_weak`):
- **The pipe:** each plain exhaust pipe of each preset (its headers, exhaust pipes, links,
  plain centre pipes and tailpipes; the cans and packing are components, stage 050), at 3000 rpm
  wide open.
- **Its conditions:** its own cell length, bore and running gas temperature (the mean of eight
  samples over a cycle).
- **The pipe laid out:** two 48-cell pipes between packed sponges.
- **The pulse:** a one-cell 100 Pa pulse, timed at two cells 72 apart, each in a window around
  its direct pass (the sponges' edges reflect a little).
- **Only the scheme:** friction and the walls' heat are off, so all the loss is the scheme's
  own.

**The scheme's own loss along each preset's worst plain path** (header, exhaust pipe, links,
tailpipe):

| Preset | 250 Hz | 500 Hz | 1 kHz | 1.5 kHz | 2 kHz | 2.4 kHz |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| i4 | 0.00 | 0.04 | 0.10 | 0.29 | **1.71** | **7.19** |
| i6 | 0.00 | 0.02 | 0.00 | 0.00 | **3.18** | **8.53** |
| v8-cross | 0.00 | 0.06 | 0.02 | 0.07 | **1.44** | **11.22** |
| v8-flat | 0.00 | 0.06 | 0.14 | 0.00 | 0.31 | **5.22** |
| boxer4 | 0.02 | 0.06 | 0.06 | 0.19 | **4.31** | **8.70** |

(dB; the bench resolves about ±0.2 dB per metre, so the small values are zero within it.)

**What it says:**
- **Up to about 1.5 kHz the plain pipes carry the engine's waves essentially without loss** (under
  0.3 dB along any path). The firing orders and the pulses' shape are not where today's solver
  is weak.
- **Between 1.5 and 2.4 kHz the scheme's own loss takes off:** 1.4–4.3 dB at 2 kHz, and 5–11 dB at
  2.4 kHz. Where in that band depends on each engine's gas temperature (hotter gas, longer
  waves): the flat-plane V8 holds to 2 kHz.
- **The 7.5 cm cells carry nothing past c / 2dx**, about 3.5 kHz in 700 K gas.
- **On cells a third as long** (2.5 cm), the loss at 2 kHz is a tenth (3.2 → 0.3 dB per metre).
  It is the grid's, as stage 050 found for the cans.

**Where a candidate would be valid** (the playbook's rule: integrate a candidate only where it
is valid): the band above about 1.5 kHz. That band is also where:
- the cans go wrong (050: above 450–750 Hz);
- the valves' noise lives (800 Hz up, 054);
- the treble the stereo scene restored sits (056).

The plain pipes below 1.5 kHz don't need replacing; the cans and the upper band do. That points
a coupled arm at the cans and the tailpipe's upper band, not at the whole network.

## The baseline (A0) on the whole drive

**The drive** (`test_baseline_whole_drive_books`): every preset at its real-time quality, on its
own crank, through the sim's public controls:
1. the key (the starter) for 1 s, then idle for 2 s;
2. throttle 0.4 with 40 N m on the crank for 1 s;
3. wide open for 1.5 s, into the limiter;
4. the lift, with 2 s to see the fuel cut, then 3 s of coast to its resume.

| Preset | Idle | Pull: top / limit | Limiter cuts | Fuel cut, resumed | Back at | Loudest sample | Mass book | Energy book |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| i4 | 796 | 6794 / 6800 | 208 | yes, yes | 779 | 0.15 | 1.4e-14 | 1.1e-14 |
| i6 | 714 | 8470 / 8500 | 319 | yes, yes | 699 | 0.98 | 4.5e-15 | 3.1e-14 |
| v8-cross | 660 | 7489 / 7500 | 464 | yes, yes | 658 | 1.00 | 1.2e-14 | 1.4e-14 |
| v8-flat | 1000 | 8999 / 9000 | 656 | yes, yes | 1008 | 1.00 | 9.9e-15 | 4.4e-14 |
| boxer4 | 757 | 6505 / 6500 | 213 | yes, yes | 740 | 0.52 | 1.2e-14 | 2.2e-14 |

**How to read the books:**
- **Mass:** against all that went through.
- **Energy:** the contents' change against what crossed: enthalpy in and out, the pistons'
  work, the walls' heat (cylinders and pipes), the fuel's sensible energy and the heat released.

The guards stayed under `RB_GUARD_RATE_WARN` throughout.

**The limiter:** the soft cut holds every engine at its limit (within 30 rpm over it). Above
the limit it already drops every spark (its share reaches all of them), so the hard cut never
changes the outcome. That is why "no hard cut" is not a planted bug here: it isn't a fault
(below).

**The loudest sample:** the V8s and the six reach full scale on their pulls. That is the soft
limiter at the sim's output, as designed (stage 12b/12c), not a clip. The drive checks that it
never exceeds full scale or becomes a NaN.

**Stems:** each preset's pull and long coast (`rb_cli sound --sweep --pull-seconds 4
--coast-seconds 6 --seconds 12 --float`), one file per layer (pipes, flow, engine) and one with
all three. They are in `renders/evidence/stage-057/` (git-ignored, 45 MB, 20 files).

All three layers together, over the 12 s:

| Preset | Peak | RMS (SPL at the listener) | Soft-limited samples | Non-finite |
| --- | ---: | ---: | ---: | ---: |
| i4 | −18.2 dBFS | 93.4 dB | 0 | 0 |
| i6 | −2.1 dBFS | 108.1 dB | 0 | 0 |
| v8-cross | −0.8 dBFS | 108.7 dB | 4 | 0 |
| v8-flat | −0.0 dBFS | 110.3 dB | 299 | 0 |
| boxer4 | −8.3 dBFS | 103.3 dB | 0 | 0 |

The flat-plane V8's 299 soft-limited samples are its pull at 9000 rpm.

## A geometry edit returns its load upstream

**The test** (`test_baseline_geometry_returns_upstream`): the four held at 4000 rpm wide open.

| Edit (behind the catalyst) | Collectors' back pressure | Pumping (PMEP) | VE |
| --- | ---: | ---: | ---: |
| As built | 18 025 Pa over ambient | 34 555 Pa | |
| No muffler | **−5 000 Pa** | **−121 Pa** | |
| Baffles half as open | **+1 483 Pa** | **+1 315 Pa** | |
| 0.6 m more tailpipe | **+227 Pa** | | **−0.083 %** |

The edits reach the cylinders the way the physics says.

## Planted bugs (`tools/bugs/stage057.py`): 6 of 6 caught

| Bug | Caught by |
| --- | --- |
| The fuel's mass not booked (0.1 % of it) | the drive's mass book |
| The fuel's energy not booked (10 %) | the drive's energy book |
| The engine's cells twice as long | the weak-waves map (its 1 kHz bound) |
| The baffles not narrowing the way out | the geometry test |
| The soft cut off | the drive's limit check (top within 30 rpm) |
| The fuel cut sticking after the foot is off | the drive's resume check |

**Two first tries were not faults, and were retired with a note in the table:**
- **"The pipes' momentum first order":** its reconstruction acts on ρu², which a small wave in
  still gas never feels. The bench measured the same loss to 0.01 dB.
- **"No hard cut":** the soft cut already drops every spark above the limit.

## What is left (why 🟡)

1. **The coupled arm's whole-engine adapter** (the production gas's mixture, area changes and
   chambers, a moving background, reversal). It is research of its own, aimed by the map above at
   the cans and the upper band.
2. **060's equal comparison** needs at least one arm that enters the engine. Until then the
   baseline here is the record.
3. **Bernardo's listening** of the stems is not needed for this stage (nothing changed), but the
   stems are there for 058's diagnosis of the overrun.

## Files

- **New:** `tests/test_baseline.c` (3 tests), `tools/bugs/stage057.py`, this doc.
- **Changed:** `tests/test_main.c`, PLAN.md, CODEX.md.
- **Unchanged:** the core, so the renders are bit-identical, and the speed check, `make wasm` and
  the parity tools have nothing to compare.
