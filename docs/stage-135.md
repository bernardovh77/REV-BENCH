# Stage 135: an optimization survey of today's architecture

Worked on 2026-11-23 by Claude (Opus 5.5), Bernardo's request of the same day. The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-135). Quality comes first: nothing here
changed the sound, and nothing that would is applied.

**Status: 🟡.** The map and the ranked list are done. No class A change cleared the card's bar
(below), so the code is unchanged. Two parts of the map are still open: the interface's own
work per frame, and the cache counters, for which no tool exists on this machine.

## The gaps before it

PLAN.md's open rows are 118's misses, 066/069's real-device runs and listening, the fresh
engine's tone, 039, 041, 051 and 014. An analysis that changes nothing touches none of them.

## How it was measured

- **Machine:** the baseline laptop (i7-1355U), on mains power.
- **Builds:** MSVC 19.52, Release with link-time optimization and `/Zi`, linked `/DEBUG`
  (`~/rb-build/s135/prof`), and w64devkit's gcc (`-O2 -ffp-contract=off`) for the counts.
- **Profile:** stage 118's `tools/win_sampler.c` (1 ms samples, main thread) on
  `rb_bench_render`, pinned to the performance cores (`--affinity 0xf`).
- **Counts:** a scratch copy of the core with counters in `remix()`, `rb_gas_init` and the
  scalar Newton step. It was built outside the repo and never committed.
- **Timings:** best-effort and interleaved. The laptop drifts 5–10 % between rounds, so a gain
  under about 3 % is not resolved.

## The map

### The core, by function (share of the render thread's samples)

| Function | v8-cross lean hold | i4 balanced idle | i6 low rev | boxer4 low hold |
| --- | ---: | ---: | ---: | ---: |
| `rb_chamber_update` (holds the inlined Newton step and u(T)) | 21.4 | 20.4 | 23.5 | 22.0 |
| `push` (pipes' momentum) | 10.6 | 12.9 | 11.4 | 9.5 |
| `rb_gas_init` (a mixture's coefficients) | 8.5 | 11.9 | 9.3 | 9.1 |
| `rb_engine_step` | 6.8 | 5.9 | 5.5 | 4.5 |
| `volume_step` | 6.6 | 3.4 | 5.1 | 6.2 |
| `move_gas` | 4.1 | 6.8 | 5.3 | 5.7 |
| `finite` (a C-runtime symbol, see stage 118) | 4.4 | 3.3 | 4.1 | 3.8 |
| `rb_chamber_integrate_many` | 3.4 | 5.4 | 3.1 | 4.3 |
| `momentum_flux` | 2.7 | 4.4 | 4.9 | 3.8 |
| the rest, each under 3 % | 31.5 | 25.6 | 27.8 | 31.1 |

### The core, by part (the same samples grouped)

| Part | v8-cross lean | i4 balanced |
| --- | ---: | ---: |
| The gas and chambers: update, Newton, mixture, the cells' lanes | 35 % | 41 % |
| The pipes: momentum, fluxes, cell heat, side flows | 35 % | 38 % |
| Engine, cylinders, orifices | 12 % | 10 % |
| The C runtime's `exp`, `log` and the `finite` symbol | 7 % | 5 % |
| The sound's own layers: render, decimators, jet noise | 6 % | 3 % |

There is still no hot spot. Stage 118 found the same shape, and its fixes are in this profile:
`copysignl`, `fmin` and `fmax` are gone. What costs is the thermodynamics of a few hundred
cells, every physics step.

### What the counts say

Counted over each preset's 3 s hold window (gcc):

| | i4 balanced + v8-cross lean |
| --- | ---: |
| Chamber updates through `remix()` | 110.8 million |
| Of them, a mixture rebuilt (`rb_gas_init`) | 82.6 million (**75 %**) |
| Newton steps per solve (scalar path) | 1.24 |
| i6 low rev: rebuilt / Newton steps per solve | 78 % / 1.46 |

- **Newton is already near its floor:** a good guess and one step, sometimes two.
- **The mixture is rebuilt on three updates in four.** Remix's 1e-8 threshold rarely holds in
  the exhaust's cells, where burned gas and air mix every step.

### Data and memory

| Structure | Size |
| --- | ---: |
| `rb_sim` (the whole simulation) | 1.48 MB |
| `rb_engine` | 375 kB |
| `rb_chamber` (every pipe cell is one) | 392 B, of which `rb_gas` 184 B |
| `rb_pipe` (cells held apart) | 1.36 kB |

- **Working set:** a few hundred cells at 392 B each is about 150 kB per step. That is more
  than the 48 kB L1 and well inside the 1.25 MB L2.
- **No allocation anywhere,** and memory over a 15-minute drive is flat (stage 069: 3.3 MB,
  under 0.1 MB growth).
- **Not measured:** cache misses per sample. Windows' hardware counters need the Windows
  Performance Toolkit, which isn't installed here, and WSL has no `perf`.

### The host, the add-on and the app

These are taken from stages 069 and 123, not measured again:

- **The render thread:**
  - Already on the fast cores (CPU sets, `keep_to_fast_cores`) with MMCSS "Pro Audio".
  - An efficiency core would run it at 0.55× (stage 118). That is the biggest single factor,
    and it's handled.
- **Snapshots:**
  - Each one is a 112-byte header and about 50 values, posted about 60 times a second.
  - The decoder is generated code.
  - Their traffic is a few hundred kB a second, negligible next to the render.
- **The window:**
  - Stage 069 measured a window drawing every frame costing the render a third more time,
    because they share the two fast cores.
  - This is the largest cost outside the core. It belongs to stage 070 (the 3D view), whose
    card already asks to cap the frame rate while the ring is low.
- **Not measured:** the interface's own work per frame (Svelte updates, the instruments and the
  dyno's drawing) on today's panels.

## The candidates, ranked

Classes as the card defines them: **A** renders bit-identical; **B** within stage 122's
cross-compiler tolerance; **C** changes the sound.

| # | Candidate | Where | Measured or bounded gain | Cost to build | Class | Decision |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Rebuild a mixture only past a larger change** (remix's threshold 1e-8 → 1e-5) | `chamber.c` | rebuilds 75 % → 34 %; **+5 to +9 %** (i4 balanced, V8 lean), +3 % at 1e-6 | one constant | **C** (moves T and p by up to ~1e-5 of themselves, and every render's bits) | To Bernardo: behind `RB_PARAM_VARIANT` or 062, with the sound check and blind listening. Not a default. |
| 2 | **Build only the range of a mixture's coefficients that's used** (≤ 1000 K or above) | `gas.c`, `gas_inline.h`, `chamber.c` | about half of `rb_gas_init`, so roughly 4–6 % (a bound only; the honest build wasn't timed) | large: the gas becomes lazily built, so 48 callers that take `const rb_gas *` change, and a stale half is a silent wrong answer | A | Not taken: the risk and the readability cost are out of proportion. Revisit in 062's SoA rework. |
| 3 | **Two threads for one engine's pipes** (each pipe's arithmetic unchanged, joined every step) | `engine.c`, the host | up to about 1.6× on the V8s, if a barrier costs under a µs every 10–20 µs step | large; spin barriers on the render thread; the window already competes for the two fast cores (stage 069) | A | 062: needs an experiment first, and it fights the window for the same cores. |
| 4 | **Structure of arrays for the cells** (the hot fields together, `rb_gas` apart) | `chamber.h`, `pipe.c` | unknown without cache counters; the lanes in `rb_chamber_integrate_many` already gather into L1 | very large | A | 062 (its card names SoA and SIMD). |
| 5 | **Own `exp`/`log`/`pow`** for Woschni, the nozzles and the jets | `heat.c`, `flow.c`, `noise.c` | at most the runtime's 5–7 % | small | B (not the runtime's bits) | 062's cross-compiler work, with the sound check. |
| 6 | **Cache `log(bore)`** in Woschni | `heat.c` | one log per cylinder step: well under 1 % | small | A | Not worth it (card: under 2 % and a field more). |
| 7 | **Cap the window's frame rate while the ring is low** | the app | up to the third stage 069 measured, when the window is busy | medium | A (no sound change) | Stage 070, as its card says, with the real-device runs. |
| 8 | **Exempt the engine process from Windows' power throttling** (EcoQoS, `SetProcessInformation`) | `platform_win.c` | not measured: it matters only when Windows throttles a background app, which the null device can't show | small | A | For 069's real-device runs: measure with the app in the background, then decide. |

## What this stage applied

**Nothing.** Each class A candidate was either:
- below the card's bar (6),
- too risky for what it saves (2),
- an experiment that belongs to 062's rework (3, 4), or
- unmeasurable without a real device (7, 8).

No candidate that changes the sound is applied. The core, the host and the app are unchanged,
so the renders are bit-identical by construction. Since `core/` was not touched, the speed
check, `make wasm` and the parity tools have nothing to compare.

## What is left (why 🟡)

- The interface's own work per frame, on today's panels: needs the app driven with a profiler.
  Natural with stage 070's frame budget.
- Cache misses per sample: needs the Windows Performance Toolkit (a download) or `perf` in WSL.
  It would settle candidate 4.
- Candidate 1 is for Bernardo's ears. Candidates 3–5 are for 062. Candidate 8 is for 069's
  real-device runs.

## Files

New: this doc. Changed: PLAN.md (135's row), CODEX.md. The scratch builds and counters are in
`~/rb-build/s135`, outside the repo.
