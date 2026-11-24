# Stage 137: the exhaust audit and the real cars' exhausts

Worked on 2026-11-24 by Claude (Opus 5.5), overnight, at Bernardo's request (the card,
[docs/roadmap/15-exhaust.md](roadmap/15-exhaust.md#stage-137)). Bernardo, after listening:
"Mainly the flat Vs, I think they all sound a bit artificial still. I want to be able to
completely simulate every part of the engine's physics ... every style and size of muffler, if
there's 1 or 2 or more mufflers, X-pipes, junctions, all the different diameters, how many
tailpipes, how many exits, their diameters ..."

**Status: ✅** (a record: the audit, the research and the plan; 138–143 do the work).

## What the exhaust could represent (before 138)

| Real exhausts have | The model had | Gap |
| --- | --- | --- |
| A header per cylinder, its own length | yes (`header_length[i]`, one diameter for all) | stepped headers, per-header diameters |
| 4-1, 3-1, 2-1 collectors | up to 2 collectors, 0D cones | 4-2-1 (tri-Y) secondaries; a third or fourth collector |
| A downpipe per bank | yes | — |
| Catalysts: one or two per bank, close-coupled and underfloor | one restriction at each exhaust pipe's end | a second catalyst; its place along the pipe |
| Resonators (inline chambered, glasspack, Helmholtz side branch, quarter-wave) | one optional centre section per bank (a can or packed pipe) | side branches; closed tubes; more than one in a row |
| Crossovers: X-pipe, H-pipe (balance tube), Y merge | one finite X or Y junction | an H-pipe; an X anywhere; more than one |
| Mufflers: chambered/turbo, reverse-flow 3-pass, straight-through packed, glasspack | one muffler per system, one type for all | a different muffler per bank; two in series; reverse flow |
| Bypass valves (flaps) by rpm or mode | none | stage 142 |
| 1–4 tips (twin, quad, the 458's triple centre exit), each its own diameter and place | one tailpipe per system (≤ 2), one diameter, two fixed places | stage 138 |

**The physics that was already there** (and stays): the pipes' 1-D finite-volume gas with
friction, heat and the burned gas's composition (stages 10–13, 047); the cans as wide pipe cells
(their plane-wave modes to 450–750 Hz on the 7.5 cm grid, to their cross mode at
`muffler_resolution` 3, stage 050); packed cores as a settling flow resistance (050, within 0.75
dB of their stated loss); catalysts as a series resistance (050, 0.08 dB); finite junctions that
conserve mass and energy (049); the tailpipes' radiation load against an unflanged pipe's (051);
the openings' directivity (051, accepted 2026-11-24); the noise layer's passes standing in for
the cans' upper band (054).

**What the network needs from the solver, checked in 138:** a pipe end joined to another pipe's
middle cell (the cells are chambers; their rates are cleared before any face flows, so a side
face's gas lands in the same step); closed ends (a pipe end with no join is a wall); more than
two outlets (the flows out are booked per outlet).

## The real cars' exhausts (research)

Factory exhaust drawings for these cars aren't public. What the public sources establish, and
what they don't:

**Ferrari 458 Italia, F136 FB (the flat V8 preset's inspiration).**
- Three tailpipes in a triangle at the centre of the rear (Ferrari's own page; contemporary
  launch reports). One report says the pipes "work in stages", all three only from mid-revs: a
  valve (the press release's text isn't in the results; unconfirmed).
- The rear silencer has vacuum-actuated bypass valves; the catalysts are 600-cell (aftermarket
  makers' descriptions of what they replace).
- Equal-length headers in the factory system (aftermarket "equal length" replacements and
  Capristo's 4-2-1 kit described as a change of routing).
- Aftermarket dimensions for scale (Kline): catalyst section dual 63.5 mm; silencer dual 63.5 mm
  with the valve open, single 50 mm with it closed; an X-pipe at 63.5 mm.
- **Not established:** primary lengths and diameters, the factory crossover, the silencer's
  volume and internals. These are estimates in the preset (141), marked so.

**Ford Mustang GT (2024, Coyote; the cross-plane V8's inspiration).** A retailer states 2.75 in
(70 mm) pipes for every 2024–2026 GT; active valves (the "Active Valve Performance Exhaust");
aftermarket systems with an X-pipe or an H-pipe; quad tips on the Dark Horse valance. The stock
resonators and mufflers' internals aren't established.

**Ford Shelby GT350 (Voodoo, a flat-plane V8 for comparison).** Active valves; aftermarket
3.0 in X-pipe systems and 2.75 in into the mufflers, 2.5 in out (Borla). Headers not established.

**BMW E46 M3 (S54, the six's naturally aspirated reference).** Two 3-into-1 headers; Euro cars'
catalysts in the first section, US cars' a single resonator there; a second-section resonator; a
rear muffler with four tips (Supersprint's and Turner's descriptions of the sections they
replace); twin 63.5 mm or single 76 mm mid sections in the aftermarket.

**Honda S2000 (F20C, the four's matched recording).** 4-2-1 headers (from my own knowledge:
not from these results; to confirm in 141).

Sources:
- [Ferrari 458 Italia (Ferrari)](https://www.ferrari.com/auto/458-italia)
- [The 458 Italia at Frankfurt (PistonHeads)](https://www.pistonheads.com/news/general-pistonheads/ferrari-458-italia/20627)
- [Kline Innovation's 458 exhaust](https://www.kline-innovation.com/exhaust-collection/ferrari/458-3/ferrari-458-exhaust/)
- [Capristo's 458 system (FerrariChat)](https://www.ferrarichat.com/forum/threads/introducing-capristos-458-italia-exhaust-system.354638/)
- [Fabspeed 458 Valvetronic package](https://www.fabspeed.com/fabspeed-ferrari-458-italia-valvetronic-performance-package-2010-2015/)
- [2024 Mustang exhaust (LMR)](https://lmr.com/products/2024-mustang-exhaust)
- [Borla 2024–2026 Mustang GT](https://www.borla.com/2024-2026-ford-mustang-gt-exhaust-systems)
- [Corsa GT350 3.0 in double X-pipe](https://www.corsaperformance.com/products/mustang-gt-350-3-0-x-pipe-14347)
- [Borla GT350 ATAK cat-back](https://www.borla.com/products/ford-mustang-shelby-gt350-cat-back-exhaust-system-140684)
- [Supersprint E46 M3 sections (Turner Motorsport)](https://www.turnermotorsport.com/BMW-E46-M3-Parts/c-5-bmw-exhaust-systems-and-parts/m-663-supersprint)
- [Supersprint E46 M3 63.5 mm system](https://www.supersprint.com/en-us/supersprint-performance-sport-exhaust-bmw-e46-m3-63-5-mm.aspx)
- [Holley: X-pipe vs H-pipe](https://holley.com/blog/post/x-pipe_vs_h-pipe_exhaust_which_is_better_/)

**The physics' references** (for 139): Munjal, *Acoustics of Ducts and Mufflers* (the four-pole
method; extended tubes; side branches); Sullivan and Crocker (1978), perforated-tube resonators;
the University of Kentucky Vibro-Acoustics Consortium's muffler design notes
([VAC Muffler Design](https://vac.engr.uky.edu/sites/vac/files/Webinars/06_VAC_Muffler_Design.pdf));
COMSOL's perforated-muffler benchmark against measured TL
([COMSOL](https://doc.comsol.com/6.3/doc/com.comsol.help.models.aco.perforated_muffler/models.aco.perforated_muffler.pdf)).

## The plan

1. **138:** the network (the template's parts compiled into it, every preset bit-identical; side
   branches, closed ends, crossovers, up to four outlets at their places).
2. **139:** each part against its four-pole theory; the noise layer's passes from each part's
   geometry.
3. **140:** the network in the API and the app's editor.
4. **141:** the real cars as presets, against their recordings.
5. **142:** exhaust valves.
6. **143:** tuning by recordings, within each parameter's physical range.

## Files

New: this doc, [docs/roadmap/15-exhaust.md](roadmap/15-exhaust.md). Changed: PLAN.md, CODEX.md.
