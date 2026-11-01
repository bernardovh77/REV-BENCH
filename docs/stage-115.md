# Stage 115 — Review repairs: the valves' jet noise and the cycles' variation 🎧

**Status:** done (2026-10-31)

Claude's review of Codex's work (stages 037–049 and 114) found the sound bugs Codex's own audit had
confirmed on 2026-10-22 ([D02–D05](sound-physics-audit-2026-10-22.md)) still in `core/src/noise.c`,
and the cycles' variation under the real engines' range since stage 041. Bernardo: "yes start with
1 and then pause". Item 1 of the review was:

> Fix D02–D05 and bring the cycle variation back to 1–3%, with the test's lower limit restored.
> Then render 12c-vs-now comparisons you can listen to.

This stage does that and nothing else. The engine's gas, its pipes, the presets and their exhausts
are as Codex left them.

## What was wrong

- **D02, the jets' choking.** The noise of a jet through a gap (an exhaust valve blowing down, the
  throttle at idle, the intake valves, a chambered can's baffles) goes as M⁵ of its stream power.
  `inner_jet` worked out the gas's speed with the density of the gas *upstream*, not in the gap,
  and its Mach number with the upstream gas's speed of sound. A choked gap read M = 0.58 instead of
  1: 15.8 dB too little power, and the wrong shape between choked and not.
- **D03, the X and Y junctions.** Each cylinder's valve noise went to one tailpipe, chosen by its
  collector. The cross-plane V8's X and the boxer's Y share the gas between the tailpipes, but not
  this noise: the boxer's tailpipe 1 heard none of it.
- **D04, the path.** The noise's trip down the exhaust left out the centre pipes and links:
  1.25–1.45 m on four presets. The four's first cylinder heard its noise 3.9 ms after the valve
  instead of 6.1.
- **D05, the catalyst.** The catalyst's half of the power was taken even without a catalyst (its
  bypass, `catalyst_laminar` 0). No preset bypasses its catalyst now, so nothing heard changed.
- **The cycles' variation.** Stage 040 made the flame's start causal: it can't start before the
  spark. It did that by cutting off the early half of stage 12c's random start, `max(z, 0)`. Half
  the cycles then started at exactly the same moment, and the start's spread fell from 0.99 to
  0.58 of its RMS. The cylinders' work varied by 0.6–1.2 % (the COV of IMEP) wide open, under the
  real engines' 1–3 %. Stage 041 recorded this as a "target miss" and dropped the test's floor
  (`cov > 0.008` became `cov > 10*cov0`, about 0.012 %).

## What changed

**`noise.c`, the jets (D02).** The gap's speed and Mach number now come from the pressures either
side, as the nozzle's flow does (`flow.c`). The gas expands from the upstream chamber's p0 and T0
to the pressure downstream, or only to the critical pressure if that's lower: then the gap is
choked and M = 1. With x = (p / p0)^((γ−1)/γ):

    U² = 2γ/(γ−1) R T0 (1 − x)        M² = 2/(γ−1) (1/x − 1)        choked: x = 2/(γ+1)

There is no jet unless the pressure falls across the gap. A NaN gives none either.

**Recalibrated, because the law changed.** The efficiencies were set when a choked jet read
M = 0.58:
- **The exhaust valves and the baffles: 2e-3 became 5e-4.** That is within IEC 60534-8-3's range
  for valves (1e-4 as a gap begins to choke, about 1e-3 well beyond). It puts the energy above
  2 kHz, from behind the car, inside the real engines' range both idling and at 3000 rpm wide open
  (the tables below). At 2e-3 the loaded renders read −6 to −15 dB above 2 kHz, a hiss above
  every revving recording but two.
- **The throttle, the idle valve and the intake valves: 2e-3 became 5e-5.** Their noise is heard
  at the intake's mouth. It is made downstream of the gap and has to come back up through it,
  against the flow (a choked gap lets no sound back at all), and then through the air box. At
  5e-4, the four's highs over the open bonnet at idle rose 7 dB above where stage 12b calibrated
  them on the Honda V6 recorded in front. At 5e-5 they are back: −17.0 dB above 2 kHz, as in
  Codex's build (stage 12c −18.2; the Honda −23.8). The cross-plane V8 in front reads −22.1. The
  intake valves' roar (Codex's, stage S2) had never been calibrated, as S2 says.

**`noise.c`, where the noise comes out (D03–D05).** Each cylinder now has a share for each tailpipe
(`blow_share`):
- **No junction:** 1 for its own system's tailpipe.
- **Through an X or a Y:** 1/√n for each of the n tailpipes beyond the junction. That is the same
  noise, in the same phase, at each one. The paths beyond a junction are equally long, so it
  arrives at all of them at once, its power shared equally.

The path now includes the centre pipe and the link, still at 580 m/s. The catalyst takes its share
only when there is one.

**`combustion.c` and `engine.c`, the variation.** The flame's development delay after the spark is
now `rb_kernel_delay_unit(z)` = −ln Φ(−z), drawn from the same normal number z (kept within ±3) as
before:
- **What it is:** a unit exponential, never negative, its own every cycle.
- **Its tail:** the slow starts are the long one, as real engines' slow cycles are.
- **Its spread:** 0.991, stage 12c's (the symmetric start's was 0.986).
- **The burn's length:** still drawn alike with z, so a slow start still means a slow burn.

The engine computer's spark allows for the delay's mean, `rb_kernel_delay_mean()` = 1 − 2q −
ln(1 − q) with q = Φ(−3), 0.998651. That includes the idle spark, which before ignored it: with
variation 1 the burns at idle sat 1.8° later than without it, and would now have sat 4.6° later.

## What it does

The cylinders' work from cycle to cycle, each preset held wide open at 3000 rpm with variation 1,
100 cycles (COV of IMEP, per cylinder; real engines 1–3 %):

| Engine | Codex (stage 041) | Stage 115 |
| --- | --- | --- |
| I4 | 0.66–1.11 % | 0.99–1.99 % |
| I6 | 0.63–1.22 % | 0.87–2.30 % |
| Cross V8 | 0.78–1.10 % | 1.02–2.15 % |
| Flat V8 | 0.82–1.16 % | 1.06–2.20 % |
| Boxer | 0.65–1.19 % | 1.00–2.10 % |

The four still varies by 3–8 % idling. Its idle spark less the delay's mean is what it is with no
variation: 17.10° against 17.12°.

The sound, measured as `tools/compare_real.py` measures the recordings of real engines
(`tools/real_engines.csv`). Each preset is held on the dyno at 3000 rpm wide open and heard from
3 m behind; the stretch from 2 to 5 s. "Highs" is the share of the energy above 2 kHz. "Repeats"
is how much of each cycle below 500 Hz repeats in the next: the "piano note", +40 dB for an exact
loop.

| Engine | Highs: Codex → 115 (dB) | Repeats: 12c / Codex → 115 (dB) |
| --- | --- | --- |
| I4 | −27.1 → −18.7 | 19.5 / 22.5 → 21.1 |
| I6 | −28.2 → −20.0 | 17.0 / 20.0 → 18.7 |
| Cross V8 | −20.0 → −11.4 | 17.9 / 22.6 → 18.6 |
| Flat V8 | −21.0 → −14.3 | 12.0 / 21.9 → 19.7 |
| Boxer | −29.5 → −20.4 | 18.6 / 25.5 → 23.4 |

The ten revving recordings read −42 to −6 dB above 2 kHz, with a median of −20. The one recording
held at 3000 rpm, a Corvette LS6 V8, repeats at +13.

Idling from behind:
- **The four:** its highs are unchanged, −38.4 dB (Codex −38.3). Its repeats fall from +21.6 to
  +18.6.
- **The cross-plane V8:** −27.5 dB (Codex −27.0). It repeats at +4.0 against Codex's +7.0. Real
  engines repeat at about +5 idling.

What this does and doesn't do:
- **Under load:** the exhaust's rasp is now physical and in the real range. The engines repeat
  1.3–4 dB less than in Codex's build.
- **Repeats:** four of the five still repeat more than at stage 12c. With no variation at all,
  stage 12c's four repeated at +36 and stage 115's at +29: the variation now takes off less of a
  loop that was already less exact. Why is for the S1 work (stages 055/058/059).
- **Between the firings:** the energy between the firing orders is unchanged, measured with
  `tools/orders.py` (the four −17.7 dB, the cross-plane V8 −12.6, the flat-plane V8 −17.2, the
  boxer −14.4; stage 12c −14.6, +5.3, −7.4, −10.9). That comes from the exhausts Codex's stages
  13b-p and 13b-r gave the presets (the cross-plane V8's X merges its banks), not from these bugs.
  The exhausts are Bernardo's accepted choice, so they stay.
- **Idling from behind:** the highs stay where Codex had them, −38 dB for the four against the
  real engines' median of −25. At 5e-4 for the mouth they read −29, but only because the throttle's
  hiss from the front of the car, 6.8 m away, filled them in. The Honda recorded in front says that
  hiss shouldn't be there.

## The tests

New and rewritten (`tests/test_noise.c`, `tests/test_variation.c`,
`tests/test_combustion_variation.c`):
- **`test_noise_jets_at_the_gap` (new).** Air at 2 bar into 0.2 bar (the audit's fixture), choked,
  and at 1.2 and 1.05 bar into 1, not choked. The roar's power is the law's, to 1e-9, with M and U
  written the textbook way. It is continuous across the critical ratio. A gap with the pressure
  rising across it, or a NaN, gives no jet and doesn't silence the valve beside it.
- **`test_noise_valve_jets` (rewritten).** The old version forced flows of 2 and 4 times what a
  gap can pass, at equal pressures, which the corrected law rightly reads as no jet. It now blows
  down from 4 bar and 1100 K into 1 bar:
  - choked, the law's level within 0.5 dB;
  - twice the flow, 3 dB;
  - from 1.3 bar, the law's subsonic level;
  - the mufflers' passes;
  - the delay with the centre pipe and link, to the step;
  - 3 dB more without a catalyst.
- **`test_noise_valve_routing` (new).** The flat-plane V8 and the six send a bank-1 cylinder's
  noise to tailpipe 1 only. The cross-plane V8's X sends half its power to each tailpipe, the same
  sample at both, together as much as the same exhaust without its X sends to its own (within
  0.01 dB). The boxer's Y sends every cylinder's to both.
- **`test_noise_throttle_hiss` and `test_noise_intake_and_baffles`.** These now set real pressure
  drops: the plenum at 0.35 bar, the cylinders at 0.7 bar, the can at 1.1 bar.
- **`test_variation_cov_of_imep`.** Its floor is back: `cov > 0.008` for every cylinder.
- **`test_variation_cov_at_idle`.** It also checks that the idle spark less the delay's mean is the
  no-variation spark, within 1°.
- **`test_variation_off_is_exact`.** It bounds the delay by its new most, 4 × 0.07 × 180° ×
  6.608 = 333°.
- **`test_combustion_conditional_means`.** It checks the delay's mean against quadrature, its
  spread, that it grows with z and is never negative, and its value at z = 0, ln 2.
- **`test_audio_engine_bay`.** Over the open bonnet the engine's own sound must now be over 5 dB
  above the rest above 2 kHz (it was over 10). At 3000 rpm wide open the front also hears the
  exhaust valves' rasp from the tailpipe. That was 15.8 dB too weak when choked; corrected and
  calibrated from behind, it is 7.1 dB under the engine's own sound here. This is the one test
  whose limit moved, and the reason is the physics this stage corrected.

Planted bugs (`python3 tools/plant_bugs.py 115 WORKDIR all`, table `tools/bugs/stage115.py`). Every
one is caught:

| Bug | Caught by |
| --- | --- |
| `jet-never-chokes`: the gap expands past the critical ratio | `test_noise_jets_at_the_gap` |
| `jet-speed-of-upstream-gas`: U² from γ R T0 | `test_noise_jets_at_the_gap`, `test_noise_valve_jets` |
| `jet-against-the-pressure`: no check that the pressure falls | `test_noise_jets_at_the_gap` |
| `junction-ignored`: each cylinder to its own tailpipe only | `test_noise_valve_routing` |
| `junction-doubles-power`: each tailpipe gets it all | `test_noise_valve_routing` |
| `junction-uncorrelated`: one tailpipe's in opposite phase | `test_noise_valve_routing` |
| `path-without-centre`: the old path | `test_noise_valve_jets` |
| `catalyst-always`: the old catalyst share | `test_noise_valve_jets` |
| `half-normal-kernel`: stage 040's `max(z, 0)` | `test_variation_cov_of_imep` |
| `kernel-mean-wrong`: the computer expects 1 − q | `test_combustion_conditional_means` |
| `idle-spark-ignores-delay`: the old idle spark | `test_variation_cov_at_idle` |

Two older tables were updated so they still apply: stage 040's `allow-negative-delay` (now the
raw normal z as the delay) and stage 041's `omit-mean-kernel-delay` (the new mean's name). Both
are still caught.

Everything else:
- **Native tests:** 250 tests and 4,950,073 checks, all pass. Stage 041 had 248.
- **Sanitizers:** the noise, audio, combustion, variation and ignition tests are clean under the
  address and undefined-behaviour sanitizers: 50 tests, 655,680 checks.
- **Python tools:** 94 of 95 pass. `test_render_results`' full pipeline times out under GNU make 4.3,
  as it did before this stage (the review's item 2).
- **WebAssembly tests:** `make test-wasm` passes, 17 tests and 5,753 checks.
- **Web:** `npm test` passes, 17 tests, and the Playwright test passes in dev and with
  `E2E_PREVIEW=1`.
- **The core:** rebuilt with `make wasm` (zig 0.17), 150,931 bytes. Codex's last core was built by
  a script outside the repo with clang 16 (173,274 bytes); `make wasm` now reproduces the committed
  file.

## The cost

Each open gap now takes one `pow` a physics step. Interleaved on the same machine, 4 s held at
3000 rpm:
- **The cross-plane V8:** 10.7–11.1 s of CPU against Codex's 10.1–10.3, 4–8 % more (0.36–0.37×
  real time against 0.39×). It has eight gaps open at a time.
- **The four:** 5.2–5.5 s against 5.4–5.8, the same within the timing noise.
- **The engine alone (`make bench`):** unchanged.

## Left for later

- **The trip's speed:** the noise still travels at a fixed 580 m/s. The gas's own speed of sound
  and its flow would move it with temperature (D04's second half: stage 054).
- **Junction transfer:** a junction shares the noise equally and in phase. The real transfer
  depends on frequency and scatters back up the pipes (D03's research: stages 049 and 054).
- **Other silencers:** the centre silencers (the four's can, the boxer's packing) don't take a
  share of this noise yet. The mufflers' shares don't depend on frequency (D05's rest: stage 050).
- **The loop:** with the variation back, the engines still repeat more than at stage 12c under load
  (above).
- **Idling from behind:** still dull above 2 kHz.

## Listen

These are A/B renders of stage 12c, Codex's build (`d2ed9b0`) and stage 115. Every engine is heard
from 3 m behind, at the same gain in all three builds.

| File | What |
| --- | --- |
| `i4-idle`, `v8-cross-idle` | idling |
| `i4-rev`, `i6-rev`, `v8-cross-rev`, `v8-flat-rev`, `boxer4-rev` | revved in neutral: a blip, the limiter, back to idle |
| `*-3000` | held on the dyno at 3000 rpm wide open |

To make them again:

```
make BUILD=$HOME/rb-build rb_cli
$HOME/rb-build/rb_cli sound --preset i4 --rev --out renders/i4-rev.wav     # and so on
python3 tools/compare_real.py compare --kind steady renders/i4-3000.wav@2-5:i4
python3 tools/orders.py 4 renders/i4-3000.wav@2-5@2800-3200:i4
```
