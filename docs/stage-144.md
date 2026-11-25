# Stage 144: the solver's acoustic fidelity, audited against the six known culprits

Worked on 2026-11-25 by Claude (Opus 5.5), at Bernardo's go-ahead (the card,
[docs/roadmap/15-exhaust.md](roadmap/15-exhaust.md#stage-144)). Source: his guide "1D Engine
Gas-Dynamics Solver: Acoustic Fidelity Guide & Codebase Audit" (a copy in `renders/guides/`).

**Status: 🟡** — every area measured; the fixes ranked; the first, the walls' viscothermal loss,
built and tested as candidate 6 on the A/B switch, waiting for Bernardo's ears.

## The gaps before it

| Gap | Could it influence this stage? |
| --- | --- |
| 043–049, 051, 057, 060, 129 (the research solvers, the ends' load, the bands, the renderer decision, the cold end) | Yes: their numbers are reused here, not measured again |
| 039 (no recording at a known distance) | Yes: the candidate is judged by ear and by the measures that don't need a level |
| 059 (the intake's breathing) | No: the sound's fidelity, not the power |

## The audit, area by area

### 1. The scheme and the cells: the grid takes away the band above 1.3 kHz

The pipes (pipe.c) are a staggered grid, symplectic Euler, donor-cell momentum with a limited
reconstruction: cells of 5 cm (short pipes) and 7.5 cm (long ones) at the high quality, half that
at ultra. 057 measured the plain pipes' own loss: nothing to 1.5 kHz, 1.4–4.3 dB at 2 kHz along
a path, 5–11 dB at 2.4 kHz.

**Measured here on the whole engine** (`tools/grid_convergence.py`: each preset held wide open at
5000 rpm, the pipes' sound alone, `--layers pipes`, on two grids, third-octave bands):

| Band (Hz) | 500 | 1000 | 1600 | 2000 | 2500 | 3150 | 4000 | 6300 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ultra (cells halved) over high, dB, range over the six presets | −1.5…0.7 | −2.2…1.4 | 0.8…4.6 | 3.5…13.7 | 12.7…30.3 | 26…43 | 23…41 | 12…28 |
| cells a quarter (a scratch build: pipes at 192 kHz, 1024-cell pool) over ultra | −0.2…10 | −0.8…0.7 | −0.6…1.8 | −0.4…2.5 | 0.3…4.4 | 2.4…8.7 | 6…22 | 15…34 |

Each halving of the cells doubles the band the gas carries: about 1.3 kHz on today's grid,
2.5 kHz at ultra, about 5 kHz on the quarter cells. The guide's bound (12 cells a wavelength:
cells under a centimetre for 8 kHz in hot gas) is right for this scheme. Above its band the
sound is the noise layer's, tuned against recordings (050–056).

**The time step** (the guide's area 2) is not a separate limit: it is set by the cells' CFL
(dx > (c + |u|) dt), checked every step (the diagnostics' Courant number). The exhaust valves
open at the physics rate (96–384 kHz); what rounds the blowdown's front is the cells, above.

### 2. Junctions: single volumes, right to 100–200 Hz

Collectors, X- and Y-junctions are 0D volumes (chamber.c); pipes joined straight on keep their
momentum. 049's finite-volume junctions agree with a compact one only to 100 Hz (Y, H) and
200 Hz (X), so above that a junction's size and its branches' angles shape the pulses. The
finite-volume junction costs 10^3–10^4 times real time (049): not for the engine as it is.

### 3. Valves: Cd matters little to the timbre

The valves' area is the curtain (Cd 0.7) in series with the port (Cd 0.9), which already falls
with lift as Heywood's measured coefficients do (valve.h); choked flow is isentropic (flow.c).
**Measured:** the exhaust valves' Cd from 0.6 to 0.8 (the published spread at low lift) moves the
pipes' sound 1–2 dB, nearly evenly across 250 Hz–8 kHz, on the cross V8, the 458 and the four:
a level, not a timbre. A Cd by lift and pressure ratio ranks low.

### 4. The open ends and the listener: in place

Every exhaust outlet has Silva et al.'s causal radiation load (051: within 0.013 of Norris and
Sheng's |R| to ka 1.5), and the listener hears each outlet as a monopole of its volume flow,
rho / (4 pi r) dQ/dt (audio.c), as the guide asks. **One gap:** the intake's mouth has the old
fixed end correction, not the load (a small source behind the grille).

### 5. The walls: 100 to 1000 times too little damping

`experiments/ringdown/pipe_ringdown.c`: a 1 m pipe of 48.4 mm bore closed at both ends (only its
walls take its sound), a mode set up at 100 Pa and left to ring. The solver's decay against
Kirchhoff's boundary layers:

| Gas | Mode | Solver today (1/s) | Kirchhoff (1/s) |
| --- | --- | ---: | ---: |
| 300 K | 1 (174 Hz) | 0.03 | 5.6 |
| 300 K | 4 (694 Hz) | 0.07–0.22 | 11.3 |
| 800 K | 1 (279 Hz) | 0.04–0.05 | 15.7 |
| 800 K | 4 (1115 Hz) | 0.11–0.32 | 31.5 |

The friction is the mean flow's turbulent loss, quadratic in the speed, so it barely touches a
sound's small swings. With 50 m/s of mean flow it does damp them (about f U / D = 25 per second),
so at full throttle the gap closes; where the gas is slow — idle, the coast with the throttle
shut, the cans' wide bores, closed side branches — the pipes ring far longer than real ones.

## The candidate: the walls' viscothermal loss (variant 6)

**The physics** (pipe.h, `rb_wall_fit`; pipe.c, `wall_step`): Kirchhoff's wide-tube loss as a
drag on each face's flow, d(mdot)/dt = −(2/r) kappa sqrt(nu) sqrt(jw) mdot, kappa = 1 +
(gamma − 1)/sqrt(Pr) (the thermal layer's part lumped into the drag: the same damping of a plane
wave), the viscosity air's at the face's temperature (Sutherland). Its sqrt(jw) is seven
first-order sections a face, the corners from 3 Hz to a fifth of the pipes' rate, the weights
fitted at build for the pipes' step as the steps really make them; taken implicitly, like the
laminar loss. On the faces between cells and onward; the ends into volumes (half a cell, mostly
end correction) are left without. The momentum it takes the cells get back as heat, as they do
friction's. `RB_PARAM_VARIANT` 6 turns it on or off in every pipe while the engine runs; turned
on, each face's sections start at its flow, so there is no jump.

**The evidence** (tests/test_walls.c):

| Test | Result |
| --- | --- |
| `test_walls_fit_follows_kirchhoff` | the damping 0.97–1.13 of Kirchhoff's from 20 Hz to 10 kHz (or 0.35 of the rate) at the pipes' 19.2, 24, 48, 96 and 192 kHz; every weight positive |
| `test_walls_pipe_rings_down_as_kirchhoff_says` | modes 1 and 4 at 300 and 800 K: 0.94–0.98 of Kirchhoff's rate with it, under a fiftieth without |
| `test_walls_keep_the_books` | the four and the 458 motored: mass and energy within 1e-14 of their flows, as without |
| `test_walls_switch_live_without_a_jump` | on and off through the parameter; the sections start at the flows |

The probe beside it, at the engine's coarse cells too: within 3–8 % where a mode has ten cells a
wavelength, 15–20 % under where the grid itself is the limit (renders/evidence/stage-144/ringdown.txt).

**The accepted sound is unchanged:** `cli_parity`, all 11 cases bit-identical to the commit before
(variant 0 takes no new path).

**What it does to the sound** (the whole mix at the high quality, variant 6 against 0, dB):

| Drive | Total | 250 Hz–1.6 kHz | 2.5 kHz up |
| --- | ---: | ---: | ---: |
| idle | −1.5 to −2.7 | −0.5 to −4.1 | within ±1.4 |
| held wide open, 5000 rpm | −0.1 to −1.4 | +1.8 to −2.0 | within ±0.7 |
| coast (lifted off) | −1.3 to −3.4 | −0.6 to −4.6 | within ±0.5 |

Where the gas is slow the pipes' resonances lose 1–4 dB, as the ring-down predicts; at full
throttle the mean flow already damped them; from 2.5 kHz up, the noise layer's band, little moves. Against
the recordings' measures at idle (`compare_real`, no level needed): the share above 1 kHz falls
0.8–1.8 dB on the four, the six and the cross V8 (the boxer's rises 0.7), the crest moves 0.5 dB
or less.

## The fixes, ranked (audible effect for the work)

| Rank | Fix | Area | Effect | Cost | State |
| --- | --- | --- | --- | --- | --- |
| 1 | The walls' viscothermal loss | 5 | 1–4 dB off the slow gas's resonances; the "ringing" the guide names | about 7 states a face | **built: variant 6, for Bernardo's A/B** |
| 2 | The pipe solver on several cores (the pipes only meet at their ends each step) | 1 | pays for finer cells | none to the sound | stage 147 |
| 3 | Finer cells (a quarter: the gas to about 5 kHz), the noise layer turned down where the gas now carries the sound | 1 | 5–40 dB more of the real pulses above 2 kHz, in place of modelled noise | about 16 times the pipes' work | stage 148, after 147: a candidate with the noise rebalanced, else the band counts twice |
| 4 | Junctions with their size and angles (1D branches meeting in a small volume, checked against 049's finite volumes) | 2 | the firing orders' interference above 200 Hz | moderate | stage 151 |
| 5 | The intake's mouth with the radiation load | 4 | small (a minor source) | small | stage 149 |
| 6 | Cd by lift and pressure ratio, both ways | 3 | 1–2 dB level, not timbre | small | low |

Higher-order schemes (043's A2: third order with MC limiting, HLLC) were tried and rejected for
their upper band's accuracy and cost (060); finer cells are the measured way the band grows here,
and a better scheme is worth trying again only once the cores pay for it.

## Planted bugs

tools/bugs/stage144.py: 6, all caught.

| Bug | Caught by |
| --- | --- |
| the thermal layer's part left out | `test_walls_pipe_rings_down_as_kirchhoff_says` |
| the bore's diameter for its radius (half the loss) | `test_walls_pipe_rings_down_as_kirchhoff_says` |
| the sections never following the flow | `test_walls_pipe_rings_down_as_kirchhoff_says` |
| a section's corner a decade off | `test_walls_fit_follows_kirchhoff` |
| turned on without starting the sections at the flow | `test_walls_switch_live_without_a_jump` |
| the A/B switch never reaching the engine | `test_walls_switch_live_without_a_jump` |

## Speed

- **The accepted sound** (`tools/speed_check.py` against the commit before, MSVC, the P cores):
  the median after/before 0.987 (−1.3 %), within the 3 % tolerance; the worst cell −2.2 %. The
  faces carry seven more numbers each, and every pipe step asks whether the walls are on.
- **Variant 6** against 0, `rb_cli sound --hold --rpm 5000 --quality balanced`, two runs each:
  2–6 % more time (the four 1.85 → 1.74× real time, the cross V8 0.84 → 0.79×, the 458 0.81 →
  0.77×). A first version that worked out Sutherland's law with `pow` every face and step cost
  40–80 %; T sqrt(T), the radius from the pipe's bore and the weights' sum worked out once made
  it this.

## What is left (why 🟡)

1. **Bernardo's ears:** variant 6 against 0 (idle and lifting off show it most).
2. The ranked fixes 2–6: stages 147, 148, 151 and 149 (phase B3, docs/roadmap/16-realism.md); the
   Cd (6) stays low.

## Files

New: `tests/test_walls.c`, `tools/grid_convergence.py`, `experiments/ringdown/pipe_ringdown.c`,
`tools/bugs/stage144.py`, this doc. Changed: core/src/pipe.h, pipe.c (`rb_wall_fit`,
`rb_pipe_walls`, `wall_step`), engine.h, engine_build.c (`rb_engine_walls`), rb_sim.c (variant 6),
core/include/rb.h (the variant's list), native/rb_cli.c (its help), tests/test_main.c, PLAN.md,
CODEX.md, docs/roadmap/16-realism.md (phase B3, stages 145–151). Also a fix left from 141: the
app's fake add-on (app/src/engine/fake-addon.ts, which the app's tests check against the core)
now lists the 458 too; its two app tests had failed since 141 (pushed, so CI's Windows run of 141
fails on them). Evidence: `renders/evidence/stage-144/`. The graph needs a `graphify update .`.
