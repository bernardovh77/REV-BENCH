# Stage 138: the exhaust as a network

Worked on 2026-11-24 by Claude (Opus 5.5), overnight, after 137 (the card,
[docs/roadmap/15-exhaust.md](roadmap/15-exhaust.md#stage-138)).

**Status: ✅** for the network; the parts' physics is 139's, the editor 140's, the real cars 141's.

## The gaps before it

| Gap | Could it influence this stage? |
| --- | --- |
| 050 (the cans' upper band on the engine's grid) | No: the network builds the same cans; 139 tests the new parts at fine cells, and the 458 (141) uses `muffler_resolution` |
| 051's second limit (the jet at the tailpipe's end) | No: each outlet radiates as before |
| 054 (the routing's components band) | Yes, the noise layer's routing: kept identical for the legacy engines, generalized for networks (below) |
| 136, 061–063 (the renderer) | No: the network is the production renderer's own pipes |

## What was built

**The description** (engine.h, `rb_exhaust_net` in `rb_engine_desc.net`):
- **pipes** (up to 40 past the headers), each with a role (an exhaust pipe, a can, a tailpipe, a
  header's secondary: its cells, wall and friction as the legacy parts'), its length, bore,
  packing, wall temperature, what it muffles as (`muffler`) and two ends;
- **volumes** (up to 12): collectors, X junctions, a silencer's boxes, merges;
- **outlets** (up to 4), each at its own place;
- **each end** closed; into a volume; into another pipe's end cell; tapped into another pipe's
  cell at a fraction of its length (side branches, H-pipes); straight on into the next pipe (with
  how it joins if it can't); or out of an outlet. Each end's face can be narrowed to an effective
  area with a laminar loss and an entry loss (catalysts, baffles, valves);
- **each header** from its exhaust valve to wherever `header_to` says.

**The legacy fields compile into a network** (`rb_exhaust_net_legacy`, exhaust_net.c): the same
pipes in the engine's old order (exhaust pipes, tailpipes, mufflers, the snorkel, centre pipes,
links), the same joins and losses. A description with `net.pipes == 0` uses it; one with a network
is checked (`rb_exhaust_net_check`: indexes, ranges, each outlet opened by one pipe end at its
end 1, every cylinder reaching an outlet) and built from it.

**The engine** (engine_build.c, engine.c): every pipe past the headers and every volume in generic
storage (`xpipe`, `xnode`), built from the network; the old names (`tailpipe`, `muffler`,
`collector`, `junction`, ...) are pointers into it, the legacy roles only for the legacy fields'
engines. The step loops run over the volumes and the pipe list (`pipe_list`).

**The paths** (`rb_exhaust_net_paths`, `rb_exhaust_net_route`): each cylinder's (or any pipe
end's) shortest way to each outlet through the network, and the pipes along it.

**The noise layer** (noise.c): for a network, each cylinder's valve noise to each outlet it
reaches, its power shared as through stage 115's junctions, delayed by its way's length at
`EXHAUST_C`, passed by the parts on the way; every narrowed face but a catalyst's a jet heard
past it. The legacy engines keep the legacy routing to the last bit.

**The listener and the stereo scene** (audio.c, scene.c, rb_sim.c): up to `RB_NET_OUTLETS`
outlets, each at its place and opening; the mouth and the bay after them. Every source index
moved to its name (`RB_LISTEN_MOUTH`, `RB_LISTEN_BAY`, `RB_SCENE_OUTLETS`...).

## The evidence

**Every preset bit-identical:**
- `cli_parity`: all 11 cases identical to the commit before: the WAVs, the CSVs and what `rb_cli` printed (once the mouth's number was fixed, below).
- `test_net_as_description_builds_the_same_engine`: each preset's legacy network handed over as
  its description builds the same engine as its fields (every cell's p and T, every volume, every
  cylinder, every outlet's flow, to the last bit after 0.1 s wide open at 4000 rpm), and each
  narrowed face's exit loss is the one its area says.
- `test_net_noise_routes_as_the_legacy_exhaust`: the flat V8's network routes the noise exactly as
  the legacy code (the sound identical to the last bit); the four's differs as 139 says.

**The new parts** (`test_net_new_parts_run_and_keep_the_books`, motored wide open, adiabatic
cylinders, the books as `test_exhaust_engine_books`'s): a Helmholtz resonator tapped into the
four's exhaust pipe (its volume breathing 9.5 kPa), a closed quarter-wave tube, three outlets, an
H-pipe between the flat V8's banks (each bank now reaching both outlets): mass and energy within
1e-14 of their flows, no repairs, every outlet flowing.

**Refused** (`test_net_check_refuses_bad_networks`): 16 bad networks of 16, and an engine won't
build from one.

**The listener** (`test_net_listener_hears_every_outlet`): three outlets each heard at its own
1/r, a fourth silent, more than four refused; the stereo scene the same.

**Two regressions the full suite found** (both fixed, both now planted bugs):
- the pool of cells was shared out from `rb_engine_pipe_count`, which since this stage counts
  the pipes only once they are built: on a first build it read 0, the one-cell-per-pipe reserve
  went missing, and an engine wanting more cells than the pool (`test_engine_description`'s
  sixteen cylinders with 10 m pipes) wrote its cells over the faces and crashed. The count is
  now made from the description. Ordinary presets were never near the pool's limit: unchanged.
- `rb_listener_source_distance`'s source 2 became the third tailpipe when the outlets went to
  four: `rb_cli sound` printed the intake's mouth at 0.00 m (the sound itself was right). The
  API keeps rb.h's numbering, the mouth 2, the third and fourth tailpipes 3 and 4
  (`test_api_settings`, with the 458's three tips).

**Planted bugs** (tools/bugs/stage138.py): 11, all caught. **Retargeted:** 21 bugs of eight other
stages whose code moved into the network (their faults the same, at their new text: overrides at
the end of each table), all caught.

**Speed** (`tools/speed_check.py` against the commit before, MSVC, the laptop's P cores, 15 cells):
the median after/before 1.007, no slower; the worst cell −0.5 % (v8-flat, high), within the noise.

## What changes for Bernardo

Nothing yet: every preset sounds the same. What the network makes possible is in 139 (the parts'
physics) and 141 (the real cars).

## What is left

- **The editor** (140): networks are built in C (presets, tests) for now.
- **A header's own diameter, stepped headers, a third collector:** the network carries them; the
  legacy fields don't, so a preset that wants them describes its network.

## Files

New: `core/src/exhaust_net.c`, `tests/test_exhaust_net.c`, `tools/bugs/stage138.py`, this doc.
Changed: engine.h, engine_build.c, engine.c, noise.c/h, audio.c/h, scene.c/h, rb_sim.c,
rb_internal.h, rb_engine_api.c, rb_drive.c, rb.h (the listener's source numbers), diagnostics.c,
native/fuzz.c, native/rb_cli.c; the probes in tools/ that read the engine's parts (the exhaust's
roles now pointers, `blow_delay` per outlet, the noise outputs' new numbers); the tests that
read the exhaust's parts or the sources by index; eight stages' bug tables; PLAN.md, CODEX.md,
docs/architecture.md. The graph needs a `graphify update .` (graphify isn't installed here).
