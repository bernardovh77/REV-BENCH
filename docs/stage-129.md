# Stage 129: the cold end of the exhaust, linear — a recorded negative result

Done 2026-11-23 by Claude (Opus 5.5) overnight, with Bernardo asleep. The card is
[docs/roadmap/14-review-fixes.md](roadmap/14-review-fixes.md#stage-129).

**The question:** downstream of some point the exhaust's waves should be weak enough to carry
linearly, on delay lines instead of gas cells, for speed (stage 118: the cost is the
thermodynamics of about 400 cells). The card's step 1 measures that premise first: where each
exhaust's pressure swing falls under 5 % of the mean. "If that line is not near the catalyst
the stage is replanned"; "if the premise fails the stage closes as a recorded negative result."

## The gaps before it

PLAN.md's open rows: 069's and 066's real-device runs and listening, 118's misses, the fresh
engine's tone (phase B), 039, 041, 042, 014. None touches the exhaust's wave amplitudes: safe
to leave.

## The measurement

`tools/exhaust_swing_probe.c` (`rb_exhaust_swing_probe`): each preset at the high quality, held
by the dyno wide open at 5000 rpm and with the throttle shut at 4000 rpm; after 1.5 s, 0.5 s
read at 12 kHz, every exhaust cell's pressure swing ((max − min) / 2 over its mean) and highest
Mach number. Per pipe, in the gas's order (headers 20-45 % everywhere, not shown):

| Preset | Drive | Exhaust pipe | Centre | Link | Muffler | Tailpipe |
| --- | --- | --- | --- | --- | --- | --- |
| Inline 4 | wide open | 8-17 % | 9-10 % | 10 → 1.4 % | 1.1-1.7 % | 0.7-1.4 % (Mach 0.25) |
| | shut | 7-19 % | 10-11 % | 11 → 2 % | 1.1-2.1 % | 0.7-2.0 % |
| Inline 6 | wide open | 18-25 % | 15-31 % | 32-36 % | 32-36 % | 22-29 % (Mach 0.50) |
| | shut | 11-24 % | 8-29 % | 31-34 % | 27-34 % | 14-24 % |
| V8 cross-plane | wide open | 13-17 % | 4.3-5.9 % | 4.3-5.6 % | 4.3-5.1 % | 2.8-4.7 % |
| | shut | 8-21 % | 2.8-4.5 % | 2.8-4.1 % | 4.2-4.8 % | 1.7-4.4 % |
| V8 flat-plane | wide open | 11-17 % | | | 14-17 % | 4.9-13 % |
| | shut | 7-15 % | | | 11-15 % | 2.7-10 % |
| Boxer 4 | wide open | 9-20 % | 12-21 % | 7-14 % | 6-15 % | 1.8-5.1 % |
| | shut | 10-32 % | 23-32 % | 11-22 % | 16-23 % | 6-14 % |

**The premise fails.** The catalyst sits at the end of the exhaust pipe, where the centre
section begins. Only the cross-plane V8 comes near the 5 % line there, and even it straddles it
at full load. The four reaches it 1.3 m further on, at the end of its link pipe, so only its
muffler and tailpipe qualify (14 of its 76 exhaust cells, about 18 %). The six, the flat-plane V8
and the boxer never reach it: their free-flowing systems carry 5-36 % swings to the open air,
which is what makes them loud. The tailpipes' Mach numbers (0.17-0.50 wide open) would also ask
the linear part to carry a strong mean flow. A tier built on this would save little on the
engines that need it most (the V8s, the six) and would linearise exactly the waves that make
their sound.

So, as the card says, **the stage closes as a recorded negative result**: no delay-line tier.

## What follows from it

- **The V12, V14 and V16 in real time** (stage 131 pointed them here) need their speed from
  elsewhere: stage 062 (fidelity tiers and optimisation for the promoted renderer), or a faster
  machine. Stage 131's limit stands until then.
- **The band above 2.4 kHz** (the 7.5 cm grid), the card's side effect, stays with stages
  050-056 and the noise model.
- The probe stays, for any later idea that needs the waves' strength (a muffler model, 050).

## Files

New: `tools/exhaust_swing_probe.c` (`rb_exhaust_swing_probe`, in the Makefile and
CMakeLists.txt), this doc. Changed: PLAN.md (129 closed, the V12-V16 limit pointed at 062).
