# Stage 141: real cars' exhausts as presets — the Ferrari 458 Italia first

Worked on 2026-11-24 by Claude (Opus 5.5), overnight, after 139 (the card,
[docs/roadmap/15-exhaust.md](roadmap/15-exhaust.md#stage-141)).

**Status: 🟡** — the 458 is a preset with its researched exhaust, measured against Bernardo's 458
recording and the stock 458's dyno sheet. The Mustang, the M3 and the S2000 are still to come,
and so are Bernardo's ears.

## The gaps before it

| Gap | Could it influence this stage? |
| --- | --- |
| 059 (every preset short of its rating; VE at or below 1.0) | Yes: the 458's power (below). Named, not fixed here: the intake's work |
| 039 (no recording at a known distance) | Yes: the comparison is with clips of unknown placement (below) |
| 139's open parts (extended tubes, cascades) | No: the 458's silencers are a reflective chamber and a packed tube |
| 142 (valves) | Yes: the centre tip's valve is open here, as at high revs |

## The preset

`ferrari-458` ("Ferrari 458 Italia"): the flat-plane V8's engine (the F136's 94 × 81 mm), its
exhaust a network (exhaust_models.c, `f136_458`):
- **published:** the catalysts in the manifolds; a left and a right rear silencer, each an oval
  clamshell 179 × 294.4 mm, 229 mm long, "reflection-absorption", one 70 mm inlet and outlets of
  70 and 45 mm (the EC type approval e24*70/157*2007/34*1364*00 of Akrapovič's replacement, which
  keeps the original's connections; kept in renders/evidence/stage-039-source-search-2026-10-24/);
  600-cell catalysts; three tips in a triangle, the upper with a valve;
- **estimated** (each marked in the code): every length, the collectors' and the merge's volumes,
  the silencers' insides (a reflective chamber, then a perforated tube through a packed chamber
  to the outer tip; the chamber's 45 mm outlet to the centre tip), the catalysts' loss (1.5 × the
  400-cell's), the tips' 95 mm bore and places;
- **the cans** at stage 050's finer cells (`muffler_resolution` 3).

Its tier is the flat V8's (lean); idling it holds 940–1031 rpm over 30 s (mean 1000).

## Against Bernardo's 458 recording

`tools/compare_real.py compare --kind rev`, against his four 458 clips (two "brighter", two
"darker": his recording, a video; placement unknown):

| | >1 kHz | >2 kHz | >4 kHz | slope | crest |
| --- | ---: | ---: | ---: | ---: | ---: |
| clips, brighter | −12.0 / −14.8 | −23.9 / −27.0 | −38.5 / −40.7 | −10.3 / −10.6 | 10.1 / 9.1 |
| clips, darker | −22.6 / −21.5 | −33.0 / −35.0 | −46.1 / −47.1 | −10.9 / −11.6 | 10.4 / 7.8 |
| **ferrari-458** (rev, 2–5.4 s) | −16.5 | −25.4 | **−37.1** | −8.9 | 18.3 |
| v8-flat (Tubi straight pipes) | −8.0 | −16.9 | −23.3 | −4.8 | 20.7 |

**How the gap closed, step by step, all of it physics:**

| Step | >4 kHz (rev) |
| --- | ---: |
| the straight-piped flat V8 | −23.3 |
| the 458's silencers, passes from geometry (139) | −32.8 |
| its packed tube by Piening's law, not the legacy packing constant | −35.4 |
| the walls' viscothermal loss along the way (Kirchhoff) | −37.1 |

**Ruled out, by measurement:** the cans' resolution (`muffler_resolution` 1 against 3: the same
numbers); the tips' own jets (40–50 dB under the valves' noise at every outlet, split with two
noise layers on one engine); the valve (shut, the centre route's 2 % leak made the mix brighter,
not darker: the level fell 4.5 dB in the low band).

**What is left:** above 4 kHz 1.4–3.6 dB over the brighter clips, the slope 1.5 dB/oct shallower.
The crest (18 against 8–10) can't be judged against a video's audio, which is usually compressed;
a clean recording (039) can. The darker clips are 6–8 dB darker again than the brighter: a
different place, inside or farther off; air absorption (ISO 9613-1) explains only 1–3 dB of that
at 30 m.

## Against the stock 458's dyno sheet

Akrapovič's SuperFlow sheet (renders/evidence/...akrapovic-458-dyno.png), the stock exhaust:
torque peaks 549 N m (405 lb ft) at 5900 rpm, 542 at 4000, 530 at 4900, dips 505 at 4400 and 5500,
468 at 7000; power about 406 kW (545 hp) at 8600.

`rb_cli dyno --preset ferrari-458`: 438 N m at 6000 rpm (−20 %), 328 kW at 8000 (−19 %); a peak at
6000 and a dip at 5500 where the real ones are; VE at most 0.99. The old flat V8's exhaust gave the
same within 1 %: **the shortfall is the engine's breathing, not its exhaust** — 059's open gap (a
real F136 reaches about 15 bar BMEP, VE near 1.1, by its intake's tuning). It is the next work.

## A bug MSVC found

The 458 is the first preset with a network, and MSVC's build failed two byte-for-byte comparisons
of its description (`test_api_engine_fields`, `test_layout_presets_are_their_own_layouts`) that gcc
passed. The network's structs had padding (seven ints before a double; one int between doubles),
and a compound literal leaves padding unspecified: MSVC left the stack's bytes in it. Their
fields are now ordered so there is none (engine.h), and static assertions refuse to build it
otherwise (tools/bugs/stage141.py plants the padding back: the build fails). Nothing is computed
differently: the renders are the same.

The web test that lists the presets now expects six.

**The suites:** all pass. Two app-e2e tests (`holding L holds the 2-step's limit`, `restarts after
its process is killed`) fail now and then under load, here and at the commit before 137 alike
(2 of 4 runs there): the app's timing, not this work.

## Speed

`tools/speed_check.py` against 139: the median after/before 1.004, the existing presets unchanged
(the 458 is a new cell, not in the comparison).

## What is left (why 🟡)

1. **Bernardo's ears:** the 458 against the flat V8 in the app.
2. **The other cars:** the Mustang GT (Coyote), the E46 M3 (S54), the S2000 (F20C).
3. **The engine's breathing** (059), for the 458's power and its torque curve's shape.
4. **The valve** (142): the centre tip's, moving with rpm.

## Files

Changed: core/src/exhaust_models.c (`f136_458`), core/src/presets.c, tests (six presets),
PLAN.md, CODEX.md. Renders: `renders/evidence/stage-141/`.
