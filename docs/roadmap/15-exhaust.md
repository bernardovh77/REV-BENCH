# Phase B2 — the exhaust, every part of it (stages 137–144)

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
<a id="stage-144"></a>
## Stage 144 — The solver's acoustic fidelity, audited against the six known culprits

**Top priority (Bernardo, 2026-11-25): the next stage after the push of 137–141, before 140.**
Source: Bernardo's guide "1D Engine Gas-Dynamics Solver: Acoustic Fidelity Guide & Codebase
Audit" (a copy in `renders/guides/`, his machine only), which names five reasons 1D solvers
sound synthetic and asks for an audit of six areas.

**Dependencies:** [138](#stage-138), [139](#stage-139); it reuses, and must not redo, 043–049
(the research solvers: the conservative scheme, finite junctions), 051 (the radiation load),
060 (the renderer decision: A1 and A2 rejected) and 129 (the cold end, a negative result).

**What is already known about each area** (to be confirmed with numbers, not taken as read):

| Area | Today (production core) | The guide's bar | The open question |
| --- | --- | --- | --- |
| 1. Scheme and cells | Staggered grid, symplectic Euler, donor-cell momentum with a limited reconstruction (pipe.c); cells 5 cm (short pipes) and 7.5 cm (long), 2.6 cm at the CFL floor at high quality; the noise layer supplies the band above | 10–15 cells a wavelength: 7 mm at 4 kHz cold | The solver's own cutoff by measurement (a pulse's dispersion and damping per cells a wavelength), hot and cold; which part of the sound above it is solver and which the noise layer |
| 2. Time step | Fixed: pipes at 48 kHz (24 kHz on the mixed-rate grid, 052), cylinders and valves at 192 kHz | dt from max(|u| + a), CFL 0.5–0.85; blowdown resolved | Whether EVO's first pressure rise (dP/dt) is clipped by the pipe rate; the header's first cells at the valve |
| 3. Junctions | Collectors and junctions are 0D volumes (chamber.c); pipes joined straight on keep their momentum | Momentum- and angle-aware junctions | What a 0D volume costs a firing order's interference, measured against 049's finite junction on the cross and flat V8s' collectors |
| 4. Valves | Cd 0.7 on the curtain in series with the port (valve.h), isentropic choked flow (flow.c) | Cd by L/D and pressure ratio, forward and reverse, fine at low lift | The low-lift blowdown spike's height and rise against a Cd(L/D, PR) map from published flow-bench data |
| 5. Open ends, audio | Silva et al.'s causal radiation load (radiation.h, 051); the listener hears each outlet as a monopole, rho0/(4 pi r) dQ/dt (audio.c) | Levine–Schwinger R(ka) with end correction; dQ/dt monopole | Largely in place: confirm the load is on for every preset and tier, and its R(ka) against Levine–Schwinger |
| 6. Walls, mufflers | Darcy friction and wall heat transfer in the gas; viscothermal (Kirchhoff) loss only in the noise layer (139); packing as a flow resistance; no perforate impedance | Viscothermal damping in the gas, perforates with porosity, wall thickness and grazing Mach | Whether the gas rings where real pipes don't (a decay-time measure per preset); 139's open extended-tube and cascade cases |

**Deliverable:**
1. The audit: for each area the files and functions, the measured behaviour (each claim above
   turned into a number) and its effect on the sound.
2. A ranked list of fixes, by audible effect for the work, each with its physics and its cost
   on the real-time tiers.
3. The first fixes from the top of that list, each as a candidate behind `RB_PARAM_VARIANT` for
   Bernardo's A/B, with planted bugs; or, where a fix was already tried (060's A1/A2), the
   reason it is or isn't worth trying again.

**Required exit evidence:** the measures behind every row; the fixes' renders against the
recordings (`tools/compare_real.py`) and their speed (`tools/speed_check.py`); cli_parity for
the accepted sound (unchanged unless a candidate is accepted); Bernardo's listening.

**Execution record:** `docs/stage-144.md`. **Started only with Bernardo's go-ahead.**

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
