# Stage 060: renderer comparison and architecture decision

Worked on 2026-11-23 by Claude (Opus 5.5), overnight, after 059. The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-060).

**Status: 🟡.** The decision is written as [ADR 0005](adr/0005-no-renderer-replacement-yet.md),
proposed for Bernardo's acceptance:
- **No current replacement:** today's pipes (A0) stay.
- **A1 and A2:** stay rejected.
- **The coupled arm:** not promoted.
- **What the no-winner outcome does,** by the card: it blocks promotion (061) and opens a bounded
  experiment, stage 136, the coupled arm as one component in the band where A0 is weak.

The stage stays 🟡 until Bernardo accepts or changes the ADR.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 057 | No research arm could enter the engine: the reason no equal comparison could be run. |
| 059 | A0's supported envelope, which the ADR states. |
| 043 | A2's rejection record, carried into the ADR. |
| 045–048 | The coupled arm's bounded proofs and costs, carried in. |
| 039, 128 | The listening the card asks for ("exploratory listening"): there is nothing new to listen to, as no arm renders an engine. |

## The card's evidence, arm by arm

| Evidence | A0 | A1 | A2 | B (coupled) |
| --- | --- | --- | --- | --- |
| Predeclared accuracy | 050, 057 (bands measured) | 043 | 043: 12 of 64 rows pass at 2 kHz, 0 of 40 at 4 kHz | 045, 046, 048: its bounded duct proofs pass |
| Nonlinear, transient, thermal | the pipes' books, the guards, 048's references | | 117–155 Pa false contacts (043) | 046 (mean flow), 048 (thermal fronts): pass in its domain |
| Full-engine evidence | 057's drive on all five presets; 059's ratings | none | none | **none: can't enter** |
| Exploratory listening | the accepted sound | | | nothing to render |
| No-trace native cost | 0.7–2.6× real time, whole engine (118, 135) | | | **0.002–0.015× for one duct** (046, 048) |
| WASM cost | measured in 118 (recorded, not gating) | | | none |

**The decision:** no promotion. The full table and what A0 supports are in the ADR.

## What was done

- **The ADR,** `docs/adr/0005-no-renderer-replacement-yet.md` (proposed).
- **The new stage card,** 136, in docs/roadmap/05-engine-qualification.md: one component (the
  four's can or tailpipe) behind a research selector. Its gates:
  - four-pole theory on the engine's grid;
  - the books;
  - the load upstream;
  - the noise layer's pass removed in the same change;
  - the cost against the tier;
  - Bernardo's listening.
- **PLAN.md:** 060's row, 136's row, and 061 marked blocked on 136.
- **No code:** a decision stage. The renders are bit-identical.

## What is left (why 🟡)

1. **Bernardo's acceptance of the ADR,** or his change to it.
2. **136,** whose result decides whether 061 has anything to promote.
