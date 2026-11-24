# Stage 059: five-engine physical and acoustic qualification

Worked on 2026-11-23 by Claude (Opus 5.5), overnight, after 058. The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-059).

**Status: 🟡.** The physical half is qualified against the published ratings of the engines the
presets are modelled on, and the supported envelope is frozen in a test with its gaps stated.
The acoustic half can't be qualified: the card's reference entry gate is not met (stage 039's
matched recordings are still missing), and per the stop rule unknown reference hardware can't
certify a model. The core is unchanged.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 039: matched references | **The entry gate for the acoustic half, not met.** The physical half uses the manufacturers' scalar ratings, which 039's own table permits for "a reported number to its named system and stated test condition", not as audio. |
| 057 | Its whole-drive baseline (start, idle, load, limiter, cut, resume) and float stems are this stage's trajectories. |
| 058 | Its overrun measures (repetition, orders, floor) are the per-layer and order statistics; its finding (the overrun's floor 36–43 dB down) is an unmatched mechanism here. |
| 041: combustion's calibration | Measured here: the COV's load dependence is the gap. |
| 118's tiers, 135, 066/069 | The maps run at the presets' default quality; the tiers' misses stay theirs. |

## The engines and their identity (stage 039's table)

| Preset | Modelled on | Not the same engine because |
| --- | --- | --- |
| i4 | Toyota 1AZ-FE, 2.0 L, 86 × 86 | a replacement-route exhaust; generic, not one model year |
| i6 | BMW M54B30, 3.0 L | a Supersprint-style twin exhaust; **an 8500 rpm limit, a prepared-race assumption: the stock M54 rates power at 5900** |
| v8-cross | Ford Coyote 5.0 (2011) | a modified X with the rear silencers deleted; a 7500 rpm limit |
| v8-flat | Ferrari 458 Italia, F136, 4.5 L | a Tubi-style unvalved rear |
| boxer4 | Subaru FB20 (XV), 2.0 L, 84 × 90 | an HKS-style rear |

**The published ratings** (manufacturers' net figures; ranges where sources differ):

| Engine | Peak torque | Peak power | Sources |
| --- | --- | --- | --- |
| 1AZ-FE | 190–194 N m at 4000 rpm | 108–112 kW at 6000 | [Toyota AZ engine](https://wikiwand.com/en/Toyota_AZ_engine), [engine-specs](https://www.engine-specs.net/?p=7996) |
| M54B30 | 300 N m at 3500 | 170 kW (231 PS) at 5900 | [auto-data](https://www.auto-data.net/en/bmw-3-series-sedan-e46-330i-231hp-automatic-21146) |
| Coyote 5.0 (2011) | 529 N m (390 lb-ft) at 4250 | 307 kW (412 hp) at 6500 | [cars.com](https://www.cars.com/articles/2011-ford-mustang-whats-new-1420685626764), [mustangspecs](https://www.mustangspecs.com/?p=8393) |
| F136 (458) | 540 N m at 6000 | 419–425 kW (570 CV) at 9000 | [Ferrari](https://www.ferrari.com/auto/458-italia), [F136](https://en.wikipedia.com/wiki/Ferrari_F136_engine) |
| FB20 (XV) | 196 N m at 4200 | 110 kW at 6200 | [Subaru FB engine](https://en.wikipedia.com/wiki/Subaru_FB_engine), [CarExpert](https://www.carexpert.com.au/subaru/xv/2016/s/powertrain-and-performance) |

## The physical qualification

**The maps.** `rb_cli dyno --step 500` gives each preset wide open on the core's dyno, every
500 rpm from 1500 to just under its limit (`renders/evidence/stage-059/*-dyno.csv`, git-ignored).

| Preset | Peak torque (model) | Peak power (model) | Peak VE | Peak PMEP |
| --- | --- | --- | ---: | ---: |
| i4 | 178 N m at 4510 | 104 kW at 6506 | 0.974 at 5511 | 1.08 bar |
| i6 | 289 N m at 4513 | 157 kW at 5507 | 1.019 at 5011 | 1.17 bar |
| v8-cross | 465 N m at 5013 | 261 kW at 6004 | 1.002 at 5013 | 1.00 bar |
| v8-flat | 435 N m at 4519 | 329 kW at 8003 | 0.982 at 7511 | 1.23 bar |
| boxer4 | 181 N m at 4011 | 105 kW at 6010 | 0.961 at 5012 | 0.91 bar |

**At the ratings' own speeds** (`test_qualification_against_published_ratings`: the core's
dyno, a second's settling, six cycles' mean):

| Preset | Torque at the rated speed | Power at the rated speed | In the envelope? |
| --- | ---: | ---: | --- |
| i4 | 171 N m at 4000: **−11.0 %** | 104 kW at 6000: **−5.5 %** | yes (±12 %) |
| i6 | 268 N m at 3500: **−10.7 %** | 157 kW at 5900: **−7.9 %** | yes |
| v8-cross | 473 N m at 4250: **−10.7 %** | 257 kW at 6500: **−16.2 %** | **no:** its power falls short |
| v8-flat | 431 N m at 6000: **−20.3 %** | 328 kW at 8500: **−22.4 %** | **no** |
| boxer4 | 181 N m at 4200: **−7.7 %** | 105 kW at 6200: **−4.6 %** | yes |

**What it says:**
- **All five fall short, by 5–22 %.** That none is over says the shortfall is systematic, not
  noise.
- **The four, the six and the boxer** are 5–11 % under at both points: within the frozen ±12 %.
- **The V8s fall short most at high rpm, and that is their breathing.**
  - **The cause:** the model's VE never passes about 1.0 (0.96–1.02 at every preset's peak),
    while a tuned high-output engine reaches 1.05–1.15.
  - **The flat-plane V8:** its BMEP at 6000 rpm is 12.0 bar against the 458's 15.1.
  - **The likely mechanism:** the intake's and the exhaust's ram tuning at high rpm, which the
    runners, the plenum and the headers set. It is not tuned here: tuning against one rating
    is the "passing one trajectory" the stop rule rejects.
- **The common 5–11 % may also be friction and the accessories.** The model's FMEP (1.0–2.2 bar
  across the range) is a Chen–Flynn fit, not these engines' measured strip-down friction.
  Unseparated.

**COV of IMEP over seeds** (`test_qualification_cov_over_seeds`: three seeds, 40 cycles each,
variation 1):

| Preset | Wide open, 3000 rpm | Light load (idle speed, 8 % throttle) |
| --- | ---: | ---: |
| i4 | 1.0–2.0 % | 1.4–1.6 % |
| i6 | 1.3–2.0 % | 1.7–1.8 % |
| v8-cross | 1.6–2.0 % | 1.7–1.9 % |
| v8-flat | 1.6–2.0 % | 1.6–2.1 % |
| boxer4 | 0.9–2.1 % | 1.6–1.7 % |

- **Wide open:** within Heywood's 1–3 % (section 9.4) for every preset.
- **At light load: no higher, where real engines' rise to 3–10 %** (more residual gas, slower
  burns). The variation doesn't grow with a light load's dilution: an unmatched mechanism (041's
  calibration).
- **Related to 058:** it may be part of why the idle and the overrun sound too regular (058's
  floor, 128's "dull idle").

**Trajectories** (057's `test_baseline_whole_drive_books` and stems):
- **The drive:** start, idle, a load, the pull, the limiter, the fuel cut and its resume, on every
  preset.
- **The books:** they close to 1e-14.
- **The limiter:** each engine holds within 30 rpm of its limit.
- **The guards:** quiet.

## The supported envelope (frozen)

| | Supported | Stated, not validated |
| --- | --- | --- |
| Speeds | idle to the rated power speed: i4 6000, i6 5900, v8-cross 6500, v8-flat 9000, boxer 6200 | **above the stock rating: the i6's 8500 limit** (stock 5900 power, about 6500 redline), the cross V8's 7500 (stock 7000). Raised limits are not validated durability (the card's rule). |
| Wide-open torque and power | i4, i6, boxer within 12 % (5–11 % short) | the V8s 11–22 % short; both high-rpm breathing |
| Cycle variation | wide open, 1–2 % COV | light load and idle: too regular (1.4–2.1 % against a real 3–10 %) |
| The overrun | books, cut and resume | its sound's floor and repetition (058), unqualified |
| Sound | **not qualified:** no matched references (039's entry gate) | everything acoustic, by the stop rule |
| Exhausts | as modified (039's identity) | catalog systems, valved variants, other model years |

**Unresolved mechanisms:**
1. high-rpm ram tuning (VE above 1);
2. load-dependent cycle variation;
3. friction's share of the common shortfall;
4. the overrun's aperiodic content (058);
5. the cans' upper band (050, 054).

## Planted bugs (`tools/bugs/stage059.py`): 3 of 3 caught

| Bug | Caught by |
| --- | --- |
| Friction's constant part doubled | the ratings test |
| The burn's start varying three times as much | the COV test (out of Heywood's band) |
| The burn's start varying a seventh as much | the COV test (under its floor) |

## What is left (why 🟡)

1. **The acoustic qualification**: 039's matched references. The card's entry gate.
2. **The V8s' breathing** above about 5000 rpm, and the four mechanisms above, each a stage of
   its own or 062's tier work, with a reference to fit against rather than a single rating.
3. **Raised limits:** the i6's 8500 and the cross V8's 7500 stay a stated build assumption.

## Files

- **New:** `tests/test_qualification.c` (2 tests, about 5 minutes: the COV's 30 settled runs; the native suite now takes about 18), `tools/bugs/stage059.py`, this doc.
- **Changed:** `tests/test_main.c`, PLAN.md, CODEX.md.
- **Unchanged:** the core.
