# S2d — first native acoustic-renderer candidates

2026-10-23. Bernardo authorizes starting experiments and development from
[the implementation design](stage-S2d-design.md). This checkpoint builds the
isolated A1/B0 candidates and the initial topology/profiling contracts. It
does not replace production engine sound or complete S2d/S2e/S6, S1/S1e/S2/S4
or full 13b. Stage 14 remains unstarted; Claude's setup is preserved.

## Outcome

The quiescent acoustic graph is a viable **next coupling candidate**. Across
the tested 1-m, 200–4000-Hz paths, two frozen gas temperatures, three sample
rates and three segment counts, all-pass propagation meets the proposed
numerical magnitude/phase limits. Closed loops remain bounded for one second.
The initial conservative HLLC/MC control passes conservation and basic nonlinear
fixtures but loses too much upper-frequency detail. Do not adopt it as an
acoustic upgrade simply because its total-energy equation is conservative.

These results justify developing reflected-load/mean-flow coupling. They do
not show an audible realism improvement, physically calibrated exhausts,
resolved I6 lift-off behavior or a real-time engine. The current accepted
I6/flat/cross configurations and all listening examples remain the baseline.

## Implemented modules and contracts

`experiments/acoustics/wave.[ch]` is a preallocated C11 graph with independent
traveling-wave delays, weighted junction scattering, finite acoustic
compliance, wave/flow sources, constant attenuation and the existing causal
radiation load. It supports all-pass and linear fractional-delay controls.
Its book closes source work against delay/cavity storage, terminal work,
specified propagation loss and numerical interpolation loss. Terminal work
includes load storage; it is not a separately measured far-field radiation flux.

`experiments/acoustics/euler.[ch]` advances mass, momentum, total energy and
tagged gas together using limited reconstruction, HLLC/HLLE and SSP-RK2. It
records reconstruction/flux fallbacks and rejects invalid states or excessive
CFL. Gamma is frozen at each initial air state using production gas properties;
the variable-composition/heat-capacity engine adapter is not implemented.

Both are research implementations linked to the native probe/tests. They
are not dispatched by `rb_cli`, the public API or WASM. See their
[README](../experiments/acoustics/README.md) for units, ownership and reproduction.

The production snapshot resolves all five engines without unknown connections:

| Engine | Pipes | Cells | Current relevant rev limit |
| --- | ---: | ---: | ---: |
| I4 | 14 | 111 | Unchanged |
| I6 | 23 | 170 | 8500 rpm |
| Cross V8 | 27 | 226 | 7500 rpm |
| Flat V8 | 23 | 188 | 9000 rpm |
| Boxer | 16 | 127 | Unchanged |

Component IDs use semantic names, not the enumeration positions of pipes.
Removing intake runners in an isolated descriptor control preserves exhaust
identities/geometry even though legacy pipe indices move. Endpoint snapshots
include onward connections, finite junctions, restrictions and radiation.
This establishes navigation/identity data, not a complete acoustic compiler
or full production guard/cap trace. Those S2d-0 requirements remain open.

## Matrix and numerical results

The runner executes **344 cases** with exact commands and source hashes:

- **124 A1 rows:** 96 grid/temperature/flow/frequency combinations, six
  half-amplitude controls, 18 timestep controls, three longer growth screens
  and one one-second periodic energy control. Eighteen rows are excluded by
  spatial/setup guards, leaving 106 executed FV cases. No solver state failures
  or reconstruction/flux fallbacks occur in these executed rows.
- **220 B0 rows:** 216 transfer cases across 300/900 K, 48/96/192 kHz,
  1/4/16 segments, 200/800/2000/4000/12000/16000 Hz and two interpolators;
  plus four one-second closed-loop controls.

A1 uses a localized forward weak wave through a periodic 33.6-m duct at
mean velocities -70/0/+70 m/s. Its initial amplitude is 10 Pa, with 5-Pa
controls. Main observations last 2.5 ms, before the packet reaches the periodic
seam. Wave direction is forward; varying mean-flow sign does not establish
both characteristic directions in every flowing case. The one-second
9.6-m-domain control wraps around the periodic seam and is used for energy,
not centroid speed/phase. B0 uses a 1-m line at **zero mean flow**, coherent
sine projection after a smooth startup/settling interval, plus impulse loops.
Its gas remains homogeneous/frozen within each case.

Historical A0 pipe measurements are retained with their original CSV hash.
All 41 production C/header hashes match the source identities saved in S2c.
A0 uses production variable gas properties and staggered velocity; A1 freezes
gamma at the same background air state and uses collocated total energy.
Their normalized perturbation-energy ratios can be compared as diagnostic
weak-wave measures; neither is a replacement for the full nonlinear energy
book. The packet path is `(c+U)*2.5 ms`, not exactly the B0 1-m path. Historical
paired base/packet wall timings are not a candidate speedup benchmark.

At 300 K, zero flow, 2 kHz and 192-kHz stepping:

| Grid | A0 retained perturbation energy | A0 phase error | A1 retained perturbation energy | A1 carrier magnitude |
| --- | ---: | ---: | ---: | ---: |
| 37.5 mm | 99.953% | 134.65° | 0.311% | 0.00105 |
| 18.75 mm | 99.978% | 34.01° | 7.323% | 0.191 |
| 9.375 mm | 99.992% | 8.30° | 56.198% | 0.744 |

The ordinary shock-robust HLLC/MC choice is highly dissipative for these
weak packets. At the finest tested grid only 15/24 guarded main-matrix cases
meet both the ≤1-dB carrier and ≤10° phase screen. At coarser grids, 6/13,
8/18 and 11/23 pass. This is an acoustic rejection of the initial candidate,
not a failed conservation harness. Phase is not emphasized when the carrier
has fallen below 10% of its initial magnitude; every raw row remains retained.
No assertion that an all-speed correction alone fixes this is justified yet.

Maximum relative errors in the periodic FV totals are approximately
3.60e-16 mass, 8.22e-16 momentum, 1.15e-15 total energy and 3.37e-16 tagged gas.
The tested hot/+70-m/s/4-kHz FV packet decays to energy ratios 0.0355,
0.00159 and 0.000513 at 10/50/100 ms. The separate one-second periodic
control retains about 0.0000526. Absence of growth here does not explain or
repair the partition-dependent growth in the original staggered network.

For **B0 all-pass, zero flow, 1 m and 200–4000 Hz**:

- Maximum magnitude error is below 0.001 dB at CSV reporting precision.
- Maximum wrapped phase error is **4.189°**, within the proposed 10° screen.
- The four one-second loops show no measured growth. All-pass loops retain
  their injected energy to reporting precision; linear interpolation retains
  only about 1.52% at 300 K / 1.21% at 900 K, with the rest booked as numerical
  dissipation. This is not physical exhaust absorption.
- At 300 K/48 kHz/4 kHz/16 segments, all-pass magnitude is 1 with -4.189°
  phase error; linear interpolation gives magnitude 0.6004 (about -4.43 dB)
  with -1.445° phase error. Flat magnitude and accurate phase must both be checked.

The 12/16-kHz rows expose large phase errors in some segmented all-pass
paths. They are interpolation stress tests, **not** a validated physical
high-frequency exhaust band. Higher transverse modes, real wide-can behavior
and material losses remain outside this homogeneous plane-mode fixture.
Time-varying delay/impedance passivity is also untested.

An independent z-domain reference matches the compiled transfer responses
within 1.28e-7 complex amplitude (using CSV-rounded sound speeds). An
additional analytic sweep uses 1025 frequencies from 200 to 4000 Hz for each
of 18 all-pass configurations and gives a 4.189° maximum phase error. This
checks the static filter response between sampled native measurements; it is
not another engine experiment or a mean-flow validation.

## Production cost and profiling

The uninstrumented native production-stereo control remains approximately
0.56x real time in this host/compiler setup (cross V8, 6500 rpm, asphalt,
gain .1, 48-kHz output, 128-frame blocks, 1-s warmup and 2-s captures, three
repeats). It remains below the callback/runtime target. No whole-engine
speedup is attributed to the new graph, which is not integrated.

A separate `-pg`/LTO-disabled profile samples roughly 24.45% self time in
`rb_gas_T_cv_from_u` plus `rb_gas_init`, 9.54% in batched chamber integration,
and substantial additional pipe/flow work. This supports measuring how much
downstream thermodynamic work a reduced model could replace. These functions
also serve retained nonlinear regions; their aggregate cost is not all
removable. Instrumentation materially changes timing, inlining/attribution
and overhead. Do not use its absolute runtime or self-time percentages as a
promised speedup. Hardware, source hashes, commands and profile text accompany
the evidence; the host is shared, not a dedicated real-time test machine.

## Validation, publication and remaining work

Native **189 tests / 4820 checks**, Python **37** (including FFmpeg integration)
and web unit **17** pass. ASan/UBSan passes the **14 candidate tests / 348
checks** (leak detection disabled for the sandbox). Nine deliberate mutations are caught: wrong scattering, return
sign, half delay, active loop gain, dropped X branch, cavity growth, omitted
pressure work, reversed FV update and incorrect radiation area units. New
analysis tests reject missing/duplicate/faulted/nonfinite evidence, distinguish
failed acoustic accuracy from harness corruption, and verify repeatable
publication without altering embedded audio.

The existing single `renders/RESULTS.html` gains two diagnostic plots and a
renderer-prototype section. The accepted engine clips and long coasts remain
the listening baseline. Float measurements and local references stay in
ignored `renders/`; no reference processing or new engine audio is introduced.
Publication/preservation/browser and Graphify details are recorded in the
checkpoint validation manifest. All 46 audio previews and 76 current WAVs
remain byte-identical. Offline browser playback, exclusive play, four top-end
jump buttons and both new/four old plots pass. The browser check succeeded
on one approved retry after an automatic review timeout/sandbox IPC failure.
A new WASM binary is unnecessary because
production `core/` and its source fingerprint remain unchanged.
Graphify's AST refresh records 2155 nodes, 5918 edges and 112 communities;
the existing `rb.h:415` extraction limit remains. No document semantic
extraction or automatic label-training pass is claimed.

Next dependent work:

1. Finish the shared geometry compiler and full production cap/repair trace,
   keeping semantic source/port IDs and an explicit conservative flux contract.
2. Couple a retained nonlinear region to the graph at a common clock. Test
   upstream reflected load and one mass/species/enthalpy/work ledger before
   replacing any production propagation.
3. Add mean flow, entropy/thermal fronts and reversal. Separate temperature,
   velocity, wall transfer and near-mouth mixing in the I6 lift-off problem;
   the present zero-flow graph does not answer that question.
4. Validate real components, source insertion and engine-level trajectories;
   then publish native sound comparisons with full long coasts and measured
   runtime. Only then consider a default architecture change.
5. Keep R2/S1f/S6 data/causal-source gates: current MP3 perspectives and
   turbo/NA mismatches do not supply a calibrated training set. No learned
   model or synthetic-only realism claim is added in this checkpoint.

## Reproduction and research

Build/run commands and API limits are in the
[candidate README](../experiments/acoustics/README.md). Tools:
`acoustic_candidate_probe.c`, `run_acoustic_candidates.py` and
`analyze_acoustic_candidates.py`. The analyzer's `--publish` is repeatable and
preserves existing embedded audio; it adds no alternate results page.

Evidence: ignored `renders/evidence/acoustic-candidates-2026-10-23/` contains
source/command manifest, individual rows/stderr, topology/identity controls,
copied A0 data, aggregate FV/wave CSVs, summary, plots, native-stereo cost,
profile and validation logs. The successful numeric harness is not a claim
that every architecture-promotion gate passes.

Primary method references: [constant-gamma Euler Riemann formulations](https://www.clawpack.org/riemann_book/html/Euler_approximate.html),
[power/scattering waveguide networks](https://www.di.univr.it/documenti/ArticoloRivista/allegato/allegato979612.pdf),
[Thiran fractional delays](https://www.dsprelated.com/freebooks/pasp/Thiran_Allpass_Interpolators.html)
and [causal pipe radiation](https://arxiv.org/abs/0811.3625). The
[S2d design](stage-S2d-design.md) and [research companion](audio-robustness-research.md)
describe the mean-flow, modal and nonlinear limitations that these first
fixtures deliberately do not claim to resolve.
