# Stage 053: compressible and reversing turbulent-flow excitation

Worked on 2026-11-23 by Claude (Opus 5.5). The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-053).

**Status: 🟡.** The jets' noise is now evidenced on four counts:
- solved at the gap's own state;
- booked once, as declared source work;
- statistically sound over seeds;
- silent as its flow vanishes.

Reverse flow is measured: the model leaves out the reversed jets, and at idle they matter. Their
routing is stage 054's, and their listening Bernardo's (128's "dull idle"). The core is
unchanged.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 042 | Open for injector calibration, not the jets. |
| 050 | The cans' grid; the jets' noise uses fixed pass shares (`PASS_*`), not the cans' transmission loss: 054's. |
| 052, 051, 118, 066/069, 039, 041, 135, the fresh engine's tone | None bears on the jets' source. |

## The source today (noise.c)

**The internal jets** (exhaust valves; intake valves, throttle and idle valve; a chambered can's
baffles):
- **The law:** a share η M⁵ of the stream power ½ ṁ U².
- **M and U are the gap's own,** solved from the pressures either side:
  - x = (p/p₀)^((γ−1)/γ), held at the critical 2/(γ+1) when choked;
  - U² = 2γ/(γ−1) R T₀ (1 − x);
  - M² = 2/(γ−1) (1/x − 1).

  This has been the case since stage 115 (the audit's D02). Before it, a raw upstream velocity
  stood in and a choked gap read M = 0.58.
- **The efficiencies:** η is 5e-4 for the exhaust valves and baffles, and 5e-5 for the intake
  side, which is heard back up through the gap.

**The tailpipe's jet:**
- Lighthill's U⁸, with U the exit plane's own (subsonic, so the exit velocity is the jet's).
- Hot.
- Its spectrum peaks at Strouhal 0.2.

**Where the sound goes:** each source is white noise scaled to its power, banded, and delayed
down its path, with a fixed share passing the catalyst, muffler or air box (054's routing).

## The evidence

| Card's evidence | Test | Result |
| --- | --- | --- |
| Nozzle-to-noise integration, the sonic throat, upstream state | `test_noise_jets_at_the_gap` (stage 115) | the law with the gap's M and U to 1e-9; choked M = 1 and U = sqrt(2γRT₀/(γ+1)); continuous across the critical ratio (1e-5); none with the pressure rising, none from a NaN |
| Book the source work once; source-off control against the gas's energy | `test_flow_noise_is_external_source_work`: the four at 3000 rpm wide open with the flow's noise on and off | every instrument **bit-identical** after 0.5 s (the gas never feels it); only the sound differs. Over a cycle the jets' noise is 0.146 J, **5e-4 of their (M⁵-weighted) stream power** (the exhaust's choked blowdown makes nearly all of it) and **5.9e-5 of the indicated work** |
| Matched states forward and reverse | `test_flow_noise_reverse_jets_measured` | the roar is the forward valves' alone at every step (1e-9). The reversed jets left out are measured below |
| Zero-flow limits | `test_flow_noise_statistics_over_seeds` | a gap with no drop is exactly silent; a 1 % drop gives under 1e-6 of a choked gap's (M⁵) |
| Statistical expectations over seeds | the same test: a held, choked throttle gap, 8 seeds of 0.5 s | each seed within **2.5 %** of their mean (about 500 independent values a seed, 4.5 % expected); twice the flow gives **1.985×** the power (linear in ṁ) |
| A source strength's control | `test_noise_valve_jets`, `test_noise_throttle_hiss` (stage 115) | each source's level from its law, at 1 m, to 0.5 dB |

**The booking, declared.** The jets' noise is **external modelled source work**:
- it is made from the gas's state, and nothing is taken back;
- the gas's books never see it.

The alternative was to take it from the jets' dissipation. That would have to come out of the
Borda–Carnot loss at each gap, as heat not added, and change the gas's state, so it is not done.
The size of the inconsistency is the 5.9e-5 of the cycle's work above, under any book's
tolerance. It stays declared this way while the source is a statistical layer outside the
reflected-load propagation. **A source that enters the pipes' propagation (054's routing) must
come out of the gas's energy instead.**

**Reverse flow, measured** (the four, one cycle; each jet at its own η):

| | Reversed intake jets, against the forward intake jets | Reversed exhaust jets, against the forward exhaust jets | Reversed intake jets, against the forward exhaust jets |
| --- | ---: | ---: | ---: |
| Idle, 850 rpm, throttle 4 % | **7e5 times** | 54 % | **0.20** |
| 3000 rpm, wide open | 51 % | 0.01 % | 5e-6 |

**What the reversed jets are:**
- **Idling:** as the intake valve opens, the cylinder's leftover gas, at about 1 bar, blows
  back choked into the runner's vacuum. The forward intake jets cross only a small drop, so
  their M⁵ is tiny.
- **Wide open:** reversion near intake valve closing adds half again to the intake valves'
  jets.
- **The exhaust:** its reversals at overlap matter only at idle.

**What leaving them out means:**
- **At idle**, the reversed intake jets carry a fifth of the exhaust valves' source power.
- **Heard at the mouth** through the air box (`PASS_AIRBOX`, 0.003), they would arrive within
  a few dB of the exhaust valves' noise through a chambered can (0.004, after the catalyst's
  0.5).
- **So it may matter for 128's "dull idle".** It is a candidate for listening, not a change
  here: the card says to fix the transfer before tuning the loudness, and their routing (into
  the runner, heard at the mouth; into the cylinder, out through which path) is 054's.

**Planted bugs** (`tools/bugs/stage053.py`): 4 of 4 caught:

| Bug | Caught by |
| --- | --- |
| The valves' η doubled | `test_noise_valve_jets` |
| Reversed jets heard with the forward law | the reverse test |
| The hiss's amplitude not the root of its power | the seeds test |
| A floor that keeps a still gap hissing | the seeds test |

## What is left (why 🟡)

1. **The reversed jets:** their routing (054) and Bernardo's listening (128's idle).
2. **Calibration uncertainty:**
   - η (5e-4, within IEC 60534-8-3's 1e-4 to 1e-3 for valves) and Lighthill's K (4e-5) are
     calibrated against real engines' highs (stage 115), not measured for these engines.
   - Matched recordings (039) would bound them.
3. **The tailpipe's jet in reverse:** an inflow at the tailpipe makes no jet noise (by design:
   no jet forms going in). Its modulation by the pulses is the flow's own.

## Files

- **New:** `tests/test_flow_noise.c` (3 tests), `tools/bugs/stage053.py`, this doc.
- **Changed:** `tests/test_main.c`, PLAN.md, CODEX.md.
- **Unchanged:** the core.
