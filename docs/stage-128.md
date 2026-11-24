# Stage 128: the complaints list (measured, candidates tested)

Worked on 2026-11-23 by Claude (Opus 5.5), overnight, after 060. The card is
[docs/roadmap/14-review-fixes.md](roadmap/14-review-fixes.md#stage-128).

**Status: 🟡.** Each complaint is measured. For "repeats too exactly", three candidate causes
are tested and rejected by measurement, and the remaining explanation is the reference
recording's own setting. For the dull idle, an existing candidate (variant 3) is measured,
promising on the four and too strong on the cross-plane V8. The crackle is found in the
pipes' layer at the limiter. The four's level waits on a recording at a known distance. Nothing
is promoted, and the core is unchanged: the two engine candidates built tonight lost on
measurement and were deleted, as the card asks. Bernardo's blind A/B verdicts, the card's exit
evidence, are still to come.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 066 (live play, the A/B key) | in the app; this stage's candidates are 054's and 055's variants 1–3, rendered offline with `rb_cli sound --variant` (stage 058) |
| 039 | the F20C is the one matched-ish recording; a stationary one is what the first complaint now needs |
| 058 | its overrun findings feed the crackle and the robotic coast |
| 059 | its COV (in Heywood's bands, corrected tonight) rules out the burn as the repeat's cause |

## 1. The cycle repeats too exactly

**The measurement** (stage 127, RESEARCH.md): the four held at 2850 rpm wide open, against the
Honda F20C cruising at 2850 rpm (engine bay, Wikimedia Commons). The tool is
`tools/compare_real.py compare --kind steady`: the best correlation of each half second with
the same one cycle later, by time lag, as 10 log10(r / (1 − r)).

**Repeat below 500 Hz:** the F20C reads **+4.6 dB**; ours as built **+20.5 dB**.

**The candidates, each switched in and measured:**

| Candidate | Repeat below 500 Hz | Verdict |
| --- | ---: | --- |
| The burn's variation off / as built / doubled / four times | 29.3 / 20.5 / 14.9 / 10.7 | **rejected as the cause.** Four times the spread (COV well past a real engine's) still leaves 6 dB to the F20C, and the variation as built is in Heywood's band (059) |
| A dilution-dependent spread (the burn's spread grown with the residual gas: variant 4, first try) | (not measured for repeat) | **deleted before it was committed:** it took the COV near idle to 16–153 %, and its premise (059's first light-load point, which wasn't light) was wrong |
| Turbulent wall friction (each exhaust pipe's friction times 1 + σn(t), a seeded 40 Hz process: variant 4, second try) | σ 0.3: 20.1; σ 1.0: 16.9 | **rejected and deleted:** even ±100 % friction moves it 3.6 dB of the 16 |
| The rpm wandering as a cruising car's does (±0.5 % and ±1 %, 0.4 and 1.1 Hz) | 21.1 / 19.9 | **rejected:** the measure finds each half second's best lag, so a slow wander costs it nothing |

**What remains is the recording's own setting.** A car driven on a road, heard from its engine
bay, has road, tyre and wind noise and the car's other sources under 500 Hz, none of it
periodic with the engine, and all of it lowering the repeat. Our renders have none of these.
The F20C gap is therefore not yet evidence against the model. A stationary recording of a
known engine (039, the review's problem 2) is what can tell.

**Not tried (named for a later stage):** the review's other lead, the wall temperature's slow
modulation. Its effect is through the sound speed, about 0.1 rad at 300 Hz over a 2 m path for 1
%: smaller than friction's.

## 2. Dull at idle from behind

**Variant 3** (stage 054: the reversed jets heard, the noise's paths from the live gas) at idle,
heard from behind (`rb_cli sound --idle --variant 3`). The share of energy above each frequency:

| | above 1 kHz | above 2 kHz | above 4 kHz | slope |
| --- | ---: | ---: | ---: | ---: |
| i4, accepted | −19.3 | −38.4 | −44.0 | −6.9 |
| i4, variant 3 | −19.1 | **−32.6** | **−37.9** | −5.0 |
| v8-cross, accepted | −13.6 | −27.7 | −46.2 | −9.6 |
| v8-cross, variant 3 | −5.8 | **−7.7** | **−11.8** | **+1.7** |

**What it says:**
- **The four:** 6 dB more above 2 and 4 kHz, the lows untouched. That is the kind of brightening
  "dull" asks for: **a fair candidate for Bernardo's ears on the four.**
- **The cross-plane V8: 20–34 dB more**, its slope rising. Its long overlap at idle makes strong
  reversed exhaust jets, heard here at the forward jets' full efficiency (5e-4).
  - **Too much to put before Bernardo as it is.** A sized version needs the reversed exhaust jet's
    efficiency (the gas blowing back into the cylinder: where does its sound go?).
  - **Open in 054's own record.**

## 3. The four is quiet

- **The trim:** the app trims it +10 dB since 066 (`PRESET_TRIM`, the app's, not the core's).
- **The card's measurement first,** against recordings at a known distance: **none exists** (039).
  The F20C was recorded in its engine bay, its distance unknown.
- **Open,** with the app's trim standing in.

## 4. The V8's crackle

**Rendered with each layer alone** (the cross-plane's rev in neutral, `--layers`). Measured above
1 kHz: kurtosis (3 for noise) and the events that pass five times the local RMS.

| Layer | At the limiter | On the overrun |
| --- | --- | --- |
| pipes | **kurtosis 7.4, 48 events** | 3.1, 0 |
| flow | 4.4, 2 | 4.3, 51 (but 3 dB under the pipes there) |
| engine | 2.6, 0 | 2.7, 0 |
| all | 4.1, 5 | 3.1, 0 |

**The crackle is the pipes' layer at the limiter: the soft cut's skipped firings.**
- **Each gap:** a cylinder that skips leaves a gap, and the next one's pulse is a hard edge.
- **In the full mix:** the flow's noise half-masks it. On the overrun nothing crackles.
- **Is it wrong?** A real limiter crackles too. Bernardo's ears decide whether this one is wrong
  in kind or only in level, so this stage names it and doesn't change it.
- **If it's to change:** the soft cut's band (`limiter_band`, live, 150 rpm by default) spreads
  the cuts. A wider band is a setting, not a code change, to try first on the A/B.

## What is left (why 🟡)

1. **Bernardo's blind A/B verdicts:**
   - variant 3 on the four's idle;
   - the limiter's band on the V8;
   - 055's variants 1 and 2.
2. **A stationary recording of a known engine (039):** the repeat and the four's level both wait
   on it.
3. **Variant 3's reversed exhaust jets sized** before it goes before Bernardo on the V8s.

## Files

- **New:** this doc.
- **Changed:** PLAN.md, CODEX.md.
- **Renders:** `renders/evidence/stage-128/` (git-ignored).
- **The core:** unchanged; both of tonight's candidates were deleted, never committed.
