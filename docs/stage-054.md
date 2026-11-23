# Stage 054: unified pulse and noise routing

Worked on 2026-11-23 by Claude (Opus 5.5), after 055 and 056 (the plan's order), as the last of
the three stages Bernardo asked for. The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-054).

**Status: 🟡.** The resolved pulses and the valves' noise are measured through the same
running exhaust with same-state captures: two engines run identically, one gets a small pulse
in a cylinder's header, and their difference at each outlet is the pipes' response from that
port.

**What disagreed:**
1. **The noise's path times:** today's fixed 580 m/s is up to 0.6 ms off (late wide open, early
   idling).
2. **The reversed jets:** stage 053 measured them, but they are not heard.
3. **The components' losses** in the noise's band.

**The candidate** (`RB_PARAM_VARIANT` 3, for Bernardo's ears) fixes the first two:
- the noise's path times follow the cells' temperature and flow to every outlet: within 0.12 ms
  of the pulses on the four and the boxer, 0.25 on the V8's X junction, against today's
  0.57–0.71;
- the reversed jets are heard (idling, the four's mouth +1.1 dB, its tailpipe +1.0).

**What stays open:** the third, the components. There the two layers disagree by up to 18 dB,
and the pipes are the ones that are wrong: stage 050's grid limit. Fixing it is the grid's
decision (050, 062), not this routing's, so the stage stays open on it.

The accepted sound is bit-identical.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 050: the cans' grid | **Decisive:** on 7.5 cm cells the four's cans hold only to 450–750 Hz and the packing to 1.1 kHz, so in the noise's band (800 Hz up) the pipes' components are not the components. Measured here; stays open with 050. |
| 051: directivity, the mean jet | Both layers leave by the same outlets and the same listener; unchanged. |
| 053: the reversed jets' routing | Done here, as a candidate. |
| 052: the common clock | The captures run on one clock (192 kHz, pipes at 48 kHz): nothing waits on it. |
| 055, 056 (just done) | Their candidates are variants 1 and 2; this one is 3. 056's Lagrange reads are in the scene only. |
| 135, 118's misses, 066/069, the fresh engine's tone, 039 | None touches the routing. |

## The routing today (noise.c, stage 115)

**The exhaust valves' noise:**
- **Its path:** written ahead into a ring per outlet, at a fixed path time: the path's length
  (header, exhaust pipe, centre and link, muffler, tailpipe) over 580 m/s.
- **Its share:** shared out through an X or Y junction to every outlet beyond it, 1/√n of the
  pressure each.
- **Its pass:** the catalyst's and the muffler's fixed shares (`PASS_*`), each conditional on the
  component being there.

**The chambered baffles' noise:** down the tailpipe at 580 m/s.

**The jets that flow back:** none heard (053).

## The evidence (tests/test_routing.c)

**The capture:**
- **Two engines** are built alike and run to the same state.
- **The pulse:** the second gets a 300 Pa isentropic squeeze in a cylinder's first header cell,
  its exhaust valve shut (600° of its cycle).
- **The response:** the difference in each tailpipe's flow over 50 ms is the pipes' response
  from that port. It is exactly zero until the wave arrives, as the two runs are deterministic.
- **The arrival** is the front: when the response first reaches a tenth of its largest swing.
  - **Not much lower:** the solver's numerical diffusion sends a faint precursor ahead.
  - **Not its peak:** the cans smear the pulse's low end over a millisecond.
  - **Not its noise band alone:** there the cans ring with their grid-limited modes.

| Card's evidence | Test | Result |
| --- | --- | --- |
| All sources to all outlets, their timing | `test_routing_path_times_follow_the_gas`: every cylinder of the four (wide open and idling), four of the cross-plane V8's (its banks into an X) and two of the boxer's | the candidate's path times within **0.115 ms** (i4 wide open), **0.094** (i4 idling), **0.109** (boxer), **0.250** (V8, the X's mean path) of the pulses' fronts, against today's fixed 580 m/s: **0.672, 0.573, 0.646, 0.714 ms**. Every outlet a pulse reaches is one the noise is sent to, and no other. Opened from idle to wide open, its paths stay within 0 steps of a freshly worked-out set |
| Source sum | `test_routing_source_sum` | pulses in cylinders 1 and 4 together are the sum of each alone within **6e-4** of the peak (linear) |
| Geometry edits move both layers | `test_routing_geometry_edit`: 0.5 m more tailpipe | the pulse **0.771 ms** later, the candidate's noise **0.828** (within a tenth), today's 0.862 |
| Loss bypass controls | `test_routing_component_controls`: the four, each component as built and bypassed, its loss in the 800 Hz–3 kHz band, each layer against itself | **the catalyst: 0.7 dB of the pulses, 3.0 of the noise; the chambered can: 6.3 against 24.0; a straight-through: 13.4 against 11.0.** Both layers lose with each component, conditionally (a missing component takes nothing); by amounts that disagree, and in opposite order for the two mufflers (below) |
| No duplicated pulse | `test_routing_no_duplicated_pulse`: 0.25 s of the four wide open | the valves' noise at the tailpipe against the resolved flow's swings: **r = −0.006** (accepted), **−0.007** (candidate): the jets' turbulence, not a copy of their pulses |
| Stochastic work keeps 053's ownership | the candidate reads the gas and writes only the noise (`const rb_engine *`), as 053's `test_flow_noise_is_external_source_work` proved for the layer; the reversed jets add their own source work by the same law | the gas is unchanged by any variant |
| The reversed jets | `test_routing_reverse_jets_heard`: the four idling, a cycle | the reversed intake jets **7.4e5 times** the forward ones, heard at the mouth (**+1.1 dB**); the reversed exhaust jets down the headers (the tailpipe **+1.0 dB**); the accepted sound hears neither; the forward jets the same in both |
| The accepted sound unchanged | `tools/cli_parity.py`, 11 cases | **all identical** to 056's |

**Planted bugs** (`tools/bugs/stage054.py`): 8 of 8 caught:

| Bug | Caught by |
| --- | --- |
| The path times without the gas's own flow | the paths test |
| A cell's sound speed at the air's γ | the paths test |
| The centre pipes left out | the paths test |
| The routing worked out only once | the paths test (a first run missed it, as each test attached the noise fresh: the idle-to-wide-open check was added) |
| The tailpipe left out | the paths and geometry tests |
| The reversed intake jets unheard | the reverse test |
| The reversed exhaust jets unheard | the reverse test |
| Variant 3 never reaching the noise | the switch test |

**Kept in step:**
- `tools/bugs/stage055.py`: two bugs retargeted, as 055's modes now play at variants 1 and 2
  only (`structured()`).
- `tools/bugs/stage115.py`: `junction-uncorrelated`, retargeted, as the ring's slot is
  `where`.
- All three are still caught.

**The components, what the disagreement is:**
- **The grid** (stage 050): on 7.5 cm cells the four's main can holds plane-wave theory only to
  450 Hz (690 Hz in hot gas), and the packing's loss rises spuriously above 1.1 kHz.
- **What that does here:** in the noise's band the pipes' chambered can is a single expansion
  chamber at its grid's mercy, 6.3 dB, the expansion chamber's band average (m = 8.4:
  1/√(1 + (m − 1/m)²/4) of the power, 6.3 dB). The straight-through's packing reads 13.4 dB
  where its stated loss is 5–7.
- **Real mufflers:** a production chambered muffler, with its baffles, perforates and several
  chambers, loses 20–30 dB at 1–3 kHz. The noise layer's calibrated 24 dB is that, and the
  pipes' 6.3 is not.
- **So:** making the noise follow the pipes here would make the four's exhaust noise 18 dB
  louder, and wrongly. The card's rule "parallel filters that disagree with engine breathing
  fail the stage" holds the stage open on this, and the fix is the pipes' (050's finer cans, 062,
  094's muffler topology), not the noise's.

## Speed

`tools/speed_check.py` (MSVC, interleaved, best of 3): **median −0.3 %**, inside the 3 %
tolerance, with the bench's sound unchanged in every cell. The candidate's work runs only when it
is selected. When it is, it costs a walk over the path's cells every 256 steps and one more jet
per cylinder.

## The candidate (variant 3)

**Path times:**
- **Each cylinder to each outlet:** worked out again every 256 physics steps (1.3 ms) from the
  live cells: Σ dx / (c + u) along the pipes it goes down, as engine_build.c joins them.
  - **Without a junction:** its header, its exhaust pipe, its centre pipe and link, its system's
    muffler and tailpipe.
  - **Through an X:** the centre pipes leading to each outlet (their mean when two go into one
    muffler).
  - **Through a Y:** its own centre and link, then each muffler.
- **The baffles' noise:** its tailpipe's live time.

**The reversed jets:**
- **The intake's:** the cylinder's residual gas blowing back into the runner, heard at the
  mouth with the intake's roar. Same law, its sides swapped: γ 1.32, η 5e-5, the intake side's
  calibration, as 053 measured them.
- **The exhaust's:** the header's gas back into the cylinder at overlap, its gap's noise down
  the header as a forward jet's.

**Unchanged:** the shares at junctions, the components' passes, the band and the random draws
per cylinder (but for the reversed exhaust jets' own).

**The accepted sound** (variant 0) is bit-identical (cli_parity).

## What is left (why 🟡)

1. **The components in the noise's band.** The two layers disagree; the pipes' cans and packing
   are grid-limited there (050's numbers, measured again in context here). One transfer for both
   layers needs either:
   - finer cans (050's class-C candidate, Bernardo's call against the tier); or
   - a declared high-frequency transfer for each component that the pipes can't carry, which
     is 062's tier work and 094's muffler topology.

   Until then the noise keeps its calibrated passes, which are closer to real mufflers (a
   production chambered muffler loses 20–30 dB at 1–3 kHz) than the pipes' 6–9 dB.
2. **Bernardo's listening:** variant 3 against 0 (128): the idle with its reversed jets, the
   noise's timing against the pulses.
3. **The V8's X junction:** its path times are 0.25 ms from the pulses', against the four's
   0.12.
   - **The likely cause:** the junction is a volume (a 0D chamber), which the path sum crosses in
     no time, while the pulse fills it.
   - **The other case:** where two centre pipes feed one muffler, the candidate puts the noise
     at their mean.

   Both within the test's 0.3 ms, and half today's error.

## Files

- **New:** `tests/test_routing.c` (6 tests), `tools/bugs/stage054.py`, this doc.
- **Changed:**
  - `core/src/noise.c`, `noise.h`: the candidate;
  - `core/src/rb_sim.c`: variant 3 reaches the noise;
  - `core/include/rb.h`: the variants in use;
  - `tests/test_structure.c`: the switch test knows 3;
  - `tools/bugs/stage055.py`: retargeted;
  - `tests/test_main.c`, docs/architecture.md, PLAN.md, CODEX.md.
