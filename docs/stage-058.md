# Stage 058: robotic deceleration and lift-off, a causal diagnosis

Worked on 2026-11-23 by Claude (Opus 5.5), overnight, after 057. The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-058).

**Status: 🟡.** The card's controlled interventions are done. They identify what makes today's
overrun robotic and what its "boom" is, and they reject five other explanations. The stage
stays open by its own stop rule: the decisive references are missing. No reference recording
has a marked lift-off, loaded coast or neutral fall (docs/user-sound-references.md: "No idle,
load, lift-off or coast timestamps"), so the model's coast can't yet be held against a real one.
The core is unchanged.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 039: matched references | **Decisive and missing.** The card needs real loaded long-coast and neutral-fall references; none exists with the coast marked. Stays open. |
| 057 | Its stems and baseline were the starting renders. |
| 054, 055 | Their candidates (variants 3, 1, 2) were rendered here as interventions: `rb_cli sound --variant N`, new in this stage. |
| 041: combustion variation's calibration | Its variation is the one lever found here; its size on the overrun is uncalibrated. |
| 128 | The listening of any fix (below) is 128's A/B switch. |

## The method

- **The drive:** the sweep wide open from 1000 rpm, with the lift at 5 s, a 6 s loaded coast in
  gear (3 kg m² and the road), the clutch in at 11 s and the neutral fall to idle. The command:
  `rb_cli sound --sweep --pull-seconds 5 --coast-seconds 6 --seconds 15 --float --csv`.
- **The engines:** the six and the flat-plane V8 (Bernardo's boom and robotic texture) and the
  four (the control).
- **The interventions,** one at a time:
  - the layers apart (pipes only; pipes and engine);
  - the cycle variation off (0) and doubled (2);
  - an immediate fuel cut (`--overrun-transition 0`);
  - equal headers;
  - the old outlet end correction;
  - each muffler type;
  - 054's candidate (variant 3: the reversed jets heard).
- **The measures** (`tools/coast_diagnosis.py`): each window only while the fuel cut is on, so
  only the overrun is measured.
  - **Repetition:** successive 720° cycles' correlation, by crank angle, 60 Hz–1.5 kHz.
  - **The orders:** runs of 4 cycles by crank angle, bins an eighth of an order. Their outputs
    are the dominant half-order, its share of the power, and **the floor between the half-orders**
    below it.
  - **Crest and modulation.**
- **The first boom measure was rejected:** a fixed-frequency peak in the time-averaged spectrum.
  In a falling sweep it marks where the sweep dwells, not a resonance. Followed frame by frame,
  every strong low line moved with the rpm.

The renders are in `renders/evidence/stage-058/` (git-ignored; 35 WAVs at the same gain, each
with its CSV, rb_cli's text and the measures as JSON).

## What the interventions show

**The loaded coast** (in gear, fuel cut on; the floor is how far the content between the
half-orders sits under the dominant order):

| | i6 | v8-flat | i4 |
| --- | --- | --- | --- |
| As built | repetition **0.986**, floor **−41 dB** | **0.962**, **−36 dB** | **0.958**, **−43 dB** |
| Pipes only (no flow noise, no mechanical sound) | identical | identical | identical |
| Variation off | 0.998, −57 | 0.997, −52 | 0.998, −59 |
| Variation doubled | 0.960, −34 | 0.868, −30 | 0.853, −36 |
| Immediate fuel cut | 0.986, −40 | 0.962, −36 | 0.958, −42 |
| Equal headers | 0.984, −40 | 0.962, −36 | 0.966, −43 |
| Old outlet load | 0.981 | 0.955 | 0.955 |
| 054's reversed jets (variant 3) | | 0.906, **−27** | 0.949, −40 |
| Chambered muffler | 0.712, −28 | 0.809, −30 | (as built) |

**The neutral fall:** the dominant order's share of the power.

| | i6 | v8-flat | i4 |
| --- | --- | --- | --- |
| As built | order 3, **81 %** | order 4, **65 %** | order 2, **94 %** |
| Chambered muffler | order 3, 51 % | order 2, 43 % | |

## The diagnosis

1. **The robotic texture is an overrun with nothing aperiodic in it.**
   - **What is heard:** with the fuel cut on, everything heard is the pipes' pulse train. Taking
     away the flow's noise and the mechanical sound changes nothing measurable.
   - **How exact it is:** successive cycles correlate at 0.96–0.99, and the content between the
     orders sits 36–43 dB under the main one. Real engines firing at 3000 rpm leave 2–12 dB
     there (stage 12c's comparison); a real coast's figure is what 039 must supply.
   - **What moves it:** the one lever is the cycle-to-cycle variation, about 6–8 dB of floor per
     step, even with no burn (its draws still reach each cycle's charge).
   - **What adds to it:** 054's reversed jets add aperiodic content where it belongs, 9 dB of
     floor on the flat-plane V8's coast.
   - **Shown, as the card asks, by switching it off:** with the variation at 0, the coast is a loop
     to 0.998.
2. **The boom is not a resonance. It is one order carrying the sound as the rpm falls.**
   - **The share:** in the neutral fall a single order holds 65–94 % of the power (the six's 3rd,
     the V8's 4th, the four's 2nd).
   - **What it is not:** no frame-stable peak exists at any fixed frequency, the headers and the
     outlet load don't move it, and neither does the fuel cut's timing.
   - **What moves it:** the exhaust's components only. A chambered can spreads the power over the
     orders (43–51 %).
   - **What it means:** the six's and the V8's straight-through and open exhausts let their main
     order through nearly alone. With nothing between the orders (finding 1), that order is all
     there is: a drone.
3. **Rejected explanations,** each tested by switching it:
   - the flow noise's or the mechanical sound's part in the coast (nothing measurable);
   - the fuel cut's transition (0 against 0.15 s: within 1.5 dB and 0.001 of repetition);
   - the headers' inequality (equal headers: the same);
   - the outlet's radiation load (the old end correction: the same);
   - a fixed exhaust resonance (none frame-stable).

## What this points to (not done here)

**The candidates for 128's A/B switch, for Bernardo's ears:**
- **The overrun's own turbulence**, which the model doesn't hear:
  - the closed throttle's jet heard down the intake;
  - the reversed jets, which 054's variant 3 already adds;
  - the pumping's turbulence in the runners.
- **The gas exchange's variation on the overrun**, sized against a real coast.

**What would close it:** a coast recording with its lift marked. That is one of 039's asks (the
review's problem 2: a phone 3 m behind a car Bernardo can reach, its rpm logged).

**Stop rule kept:** no brighter render is offered as the cause. Each cause above is shown by
switching it, and the five rejections are kept.

## Evidence for the tool (tools/test_coast_diagnosis.py)

**The tests, on synthetic coasts with known answers:**
- **A pure set of orders:** order 2 found, cycles alike to over 0.99, the floor over 40 dB.
- **Noise added:** the floor rises by more than 10 dB, and the repetition falls.
- **Every half-order present:** the floor between them is still deep (over 60 dB); the
  half-orders are not the floor.
- **The windows:** follow the lift, the clutch and the fuel cut.

**Two mutants, both caught:**
- the crank angle taken without the rpm;
- the floor measured at the half-orders themselves (this needed the third test: a first version
  missed it).

**`rb_cli sound --variant N`** (new) selects an A/B candidate offline. With the default (0) all
11 `cli_parity` cases are identical.

## Files

- **New:**
  - `tools/coast_diagnosis.py`;
  - `tools/test_coast_diagnosis.py` (3 tests);
  - this doc.
- **Changed:**
  - `native/sound.c`, `native/rb_cli.c` (`--variant`);
  - PLAN.md, CODEX.md, docs/architecture.md (the tool).
- **Unchanged:** the core.
