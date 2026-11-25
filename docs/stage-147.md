# Stage 147: the pipes on several cores — measured, deferred to the PC

Worked on 2026-11-25 by Claude (Opus 5.5) (the card,
[docs/roadmap/16-realism.md](roadmap/16-realism.md#stage-147)).

**Status: ⏸ deferred** (Bernardo, 2026-11-25: on to 145 and 148; 147 waits until it can be
measured on his PC). A prototype answered the card's question on the laptop before any code
went into the core: splitting a step's pipes over threads gives no speed-up here. Nothing in
the core changed.

## Where the time goes

`rb_engine_step` timed alone (the laptop, i7-1355U, gcc -O3), each preset at 5000 rpm wide open:

| Preset | Pipes' rate, cells | Engine (s per s simulated) | The pipes' slow phases | One pipe step |
| --- | --- | ---: | ---: | ---: |
| i4 | 48 kHz, 111 | 0.63 | 33 % (flows 0.03, integrate 0.13, accelerate 0.05) | 4.4 µs |
| i4 | 96 kHz, 217 | 1.62 | 63 % | 10.7 µs |
| v8-cross | 48 kHz, 226 | 1.22 | 42 % | 10.6 µs |
| v8-cross | 96 kHz, 446 | 3.25 | 68 % | 22.9 µs |
| ferrari-458 | 48 kHz, 217 | 1.27 | 35 % | 9.2 µs |
| ferrari-458 | 96 kHz, 432 | 3.39 | 62 % | 22.0 µs |

The integrate phase (the cells' thermodynamics, already eight cells at a time since 135) is
about two-thirds of the pipes' time. The noise layer and the listener are 2–13 % of a render
(`rb_cli sound` with and without `--layers pipes`): not worth a core of their own.

## The prototype

A scratch copy of the core whose integrate and accelerate phases ran their pipes over a pool of
spinning threads (the host's, Win32), each thread a contiguous run of pipes balanced by cells,
the whole engine timed and its state compared bit for bit with one thread:

| Grid | 2 threads | 3 threads | 4 threads |
| --- | ---: | ---: | ---: |
| today's (48 kHz pipes) | 0.94–0.97× | 0.78–0.79× | 0.83–0.87× |
| ultra's (96 kHz) | 0.94–0.97× | 0.79–0.82× | 0.83–0.88× |
| cells a quarter (192 kHz, limits raised) | 0.88–0.92× | 0.61–0.63× | 0.64–0.72× |
| the same, pinned to the two performance cores | 0.86–1.01× | | |

(Interleaving the pipes over the threads instead of contiguous runs was worse still: 0.81–0.95×,
the neighbouring pipes' data sharing cache lines.)

**Why not here:** a phase is a few microseconds of work, synchronised three times a pipe step;
the laptop has two performance cores and eight slower efficiency cores, and one 15 W budget: two
busy performance cores share it and each slows. Bernardo's PC (i9-13980HX: 8 performance
cores, 55 W and more) should scale far better; only it can show it.

**Bit-identical:** yes at 2 and 3 threads; at 4 a race showed: `push_onward` reads the next
pipe's first face's flow while that pipe's thread updates it (serially the list's order decides
which it sees). A real implementation snapshots what a pipe reads of its neighbours before the
phase, keeping today's order.

## What follows

- **148 doesn't wait for it:** the finer cells as an offline quality tier on one core; renders of
  different drives or presets run side by side, one a core, which does scale.
- **When 147 resumes (on the PC):** the prototype's design (a host-owned pool given to the core as
  a parallel-for, contiguous runs balanced by cells, the onward race snapshotted), measured at
  1–8 threads on the PC's performance cores, kept only if it pays.

## Files

None in the repo besides this doc and PLAN.md; the prototype is in the scratch build
(`rb-build/par-src/proto.c`), not kept.
