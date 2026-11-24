# Phase B2 — the exhaust, every part of it (stages 137–143)

Opened 2026-11-24 at Bernardo's request: "I want to be able to completely simulate every part of
the engines physics ... every style and size of muffler, if there's 1 or 2 or more mufflers,
X-pipes, junctions, all the different diameters, how many tailpipes, how many exits, their
diameters ... only then I'll be able to truly judge an engine sound." Research real cars'
exhausts, build presets of them, judge them against their recordings, fine-tune by recordings,
and make sure the physics is right in every detail; quality first, processing power second.

Before this phase the exhaust past the headers was one template: up to two collectors, an
exhaust pipe each, an optional centre section, one X or Y junction, one muffler per system of
one of three types for all, one tailpipe per system. These stages replace the template with a
network and validate each part on its own.

<a id="stage-137"></a>
## Stage 137 — The exhaust audit and the real cars' exhausts

**Dependencies:** [050](04-propagation.md#stage-050), [051](04-propagation.md#stage-051), [054](05-engine-qualification.md#stage-054)

**Deliverable:** What the exhaust model can and can't represent, part by part, against what real
exhausts have; the physics each part needs and the evidence that exists for it; the research
into the reference cars' exhausts (the 458 Italia's F136, the Mustang GT's Coyote, the E46 M3's
S54, the S2000's F20C), each dimension marked published, measured or estimated; the plan.

**Required exit evidence:** The gap table; the sources; the reference cars' layouts.

**Execution record:** `docs/stage-137.md`.

<a id="stage-138"></a>
## Stage 138 — The exhaust as a network

**Dependencies:** [137](#stage-137), [044](04-propagation.md#stage-044)

**Deliverable:** The exhaust past the headers as a network in the core (engine.h,
`rb_exhaust_net`): pipes of three roles (pipe, can, tailpipe) and header secondaries, volumes,
and up to four outlets; each pipe end closed, into a volume, into another pipe's end cell, tapped
into a pipe's middle (side branches, H-pipes), straight on into the next pipe, or out of an outlet
at its own place; each end's face narrowed, resisted and given its entry loss. The legacy fields
compile into a network in exactly the old build order. The noise layer routes each cylinder's
noise along the network's ways and every narrowed face's jet past it; the listener and the stereo
scene hear every outlet at its place and opening.

**Required exit evidence:** Every preset bit-identical (cli_parity, and an engine built from its
network as the description against the same from its fields); side branches, closed ends, H-pipes
and extra outlets keep the books; bad networks refused; the noise routing identical to the
legacy one where the parts are the legacy ones; planted bugs caught; the stale bugs retargeted;
speed within 3 %.

**Execution record:** `docs/stage-138.md`.

<a id="stage-139"></a>
## Stage 139 — The network's parts against four-pole theory

**Dependencies:** [138](#stage-138), [050](04-propagation.md#stage-050)

**Deliverable:** Each part on stage 050's transmission-loss bench, at a resolution that holds
plane-wave theory, against its four-pole (or lumped) theory: a Helmholtz side branch, a
quarter-wave tube, an expansion chamber with extended inlet and outlet, a reverse-flow
(three-pass) chambered muffler, an H-pipe and an X junction, a straight-through perforated core.
The noise layer's passes for each muffling part from its own geometry instead of three
constants.

**Required exit evidence:** Each part's TL within 050's tolerance to its cross mode; the passes'
derivation and their effect on the presets stated.

**Execution record:** `docs/stage-139.md`.

<a id="stage-140"></a>
## Stage 140 — The network in the API and the app's editor

**Dependencies:** [138](#stage-138), [067](07-app.md#stage-067)

**Deliverable:** The network read and written through the public API (pipes, volumes, ends,
outlets), checked as the core checks it; the app's editor shows and edits it (add a muffler, a
resonator, an X or H pipe, an outlet; change any diameter and length).

**Required exit evidence:** Round trips of every preset's network; the editor's tests; a network
edited in the app plays.

**Execution record:** `docs/stage-140.md`.

<a id="stage-141"></a>
## Stage 141 — Real cars' exhausts as presets

**Dependencies:** [138](#stage-138), [139](#stage-139), [039](03-contracts-and-sources.md#stage-039)

**Deliverable:** Presets of real cars with their researched exhausts as networks, every dimension
with its provenance: the Ferrari 458 Italia (F136 FB) first, then the Mustang GT (Coyote), the E46
M3 (S54) and the S2000 (F20C). Each judged against its recordings (Bernardo's and the public ones)
with tools/compare_real.py and tools/orders.py.

**Required exit evidence:** The presets' measures against their recordings, with what each gap is
attributed to; Bernardo's listening.

**Execution record:** `docs/stage-141.md`.

<a id="stage-142"></a>
## Stage 142 — Exhaust valves

**Dependencies:** [138](#stage-138), [141](#stage-141)

**Deliverable:** A bypass flap as a face whose effective area is a live control (by rpm, load or
the driver's mode, as the 458's, the GT350's and the M3's are), changing the network's flow and
sound without a rebuild.

**Required exit evidence:** The flap's area against its control; the books through a switch; no
click (the face's change is smooth); the sound's change measured.

**Execution record:** `docs/stage-142.md`.

<a id="stage-143"></a>
## Stage 143 — Tuning by recordings

**Dependencies:** [141](#stage-141), [039](03-contracts-and-sources.md#stage-039)

**Deliverable:** A calibration loop per real-car preset: its measures (orders, bands, repeat,
level by rpm) against its recordings, the parameters each measure answers to and the physical
range each may move in; adjustments only within those ranges, each stated.

**Required exit evidence:** The before and after measures; no parameter outside its physical
range; Bernardo's blind listening.

**Execution record:** `docs/stage-143.md`.
