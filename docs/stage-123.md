# Stage 123 — The real-time audio host

**Status:** done (2026-11-09). Built and tested on Windows (MSVC and w64devkit's gcc) and in
WSL (gcc, also under ThreadSanitizer); CI green on Windows and Linux, the ThreadSanitizer job
included; the 15-minute real-device run passed with zero underruns ("The real-device run").

`host/` is a C11 library that runs the core in real time without Node or JavaScript
([specification](roadmap/07-app.md#stage-123), [contracts/runtime-v1.md](../contracts/runtime-v1.md),
[app-design §3–5](app-design.md)). The core is unchanged: nothing under `core/` was edited.

## What was built

```
host/include/rb_host.h        the public API (control thread, device, offline)
host/include/rb_host_trace.h  command traces for the offline mode
host/src/ring.c queue.c triple.c evring.c   the lock-free channels, one small module each
host/src/clock.c              the clock mapping (a sequence lock) and its arithmetic
host/src/runner.c             the render thread's work: commands, render, fades, snapshots, events
host/src/host.c               control side, rebuild transactions, the device's pull, sleep and wake
host/src/sim_build.c          an engine from a build description (first engine, prepare workers)
host/src/device.c             miniaudio: devices, rates, notifications
host/src/audit.c              the render thread's rules, counted
host/src/platform_win.c / platform_posix.c   threads, semaphore, mutex, clock, priority, memory
host/src/trace.c              the trace parser and player
host/tools/                   rb_host_render, rb_host_stress, rb_host_devices
host/tests/                   59 tests (58 without GNU ld's --wrap)
third_party/miniaudio/        0.11.25, pinned, with NOTICE.md (version, URL, SHA-256, licence)
tools/host_parity.py          rb_cli's drives against the host's offline renders
tools/bugs/stage123.py        39 planted bugs (and stage123_python.py: 4, for host_parity.py)
```

**Threads.** One control thread calls `rb_host_*`; the render thread is the only one that
touches a running engine; the device's thread (miniaudio's, or the caller of `rb_host_pull`)
only copies from the ring. The render thread asks the system for its priority when it starts
(Windows: MMCSS "Pro Audio", else the highest ordinary priority and the policy string says so;
POSIX: the normal class, said so, until stage 126), then loops: wait on its semaphore (the
device posts it after each callback, with a 4 ms timeout), and while the ring is under its fill
target (20 ms by default), render a block of 128 frames and write it to the ring.

**Channels** (all in `host/src`, all with behaviour tests and a stress test):
| Channel | Module | Direction | Size |
| --- | --- | --- | --- |
| audio | `ring` (SPSC, two growing counters) | render → device | 8192 frames, stereo |
| commands, acks, retired engines | `queue` (SPSC fixed records) | control → render, render → control | 256, 256, 4 |
| snapshots | `triple` (atomic slot word, dirty flag) | render → control | 3 × 64 KiB |
| events | `evring` (queue + loss counter) | render → control | 1024 |
| clock, device clock | sequence lock of atomic words | render / device → anyone | 8 words |

A full command queue refuses (`RB_HOST_ERR_FULL`) and hands out no request id. The render thread
takes a command only when there is room for its acknowledgment, so an ack is never lost.
Snapshots are 60 a second at the sim's own `rb_get_snapshot`, published whole; a slow reader
gets the newest and the drops are counted. Events carry a loss count (ring overflow, and the
core's own ring's).

**Commands** carry an id and an expected generation (0: any). Kinds: parameters, live engine
fields (only `RB_EF_LIVE`; any other would rebuild, which is a swap's job), the dyno's four
calls, the scene's clock, swap, epoch, rebase. Each is applied at the start of a block and
acknowledged with its outcome (ok, stale, refused with the core's code, busy), the generation,
epoch, engine tick and audio frame at which it took effect.

**Rebuild transactions.** `rb_host_prepare_begin` builds the engine on a worker thread, in the
order `rb_cli sound` uses (sim at the host's rate, preset, edits, parameters, scene, optional
settle); `rb_host_swap` hands it over. The render thread then fades the old engine out over a
whole number of blocks (6 blocks = 16 ms at 48 kHz, 17.4 ms at 44.1; always within 10–20 ms),
changes engines at that block boundary (generation + 1, a new epoch, the old engine on the
retired queue, a snapshot of the new engine published at once), acknowledges, and fades the new
engine in. Commands behind the swap wait until the fades are over and then apply to the new
engine; commands stamped with the old generation are stale. A failed build leaves the running
engine untouched and says why. A stale swap puts the prepared engine on the retired queue
(`STALE`); a swap when the retired queue (4) is full is not taken (`BUSY`): the build stays
with the control thread. `rb_host_poll` frees the retired engines off the render thread.

**Clock.** An epoch is `(physics_origin, audio_origin)` plus the two rates; the ack and the
snapshot carry the epoch. The audio frame is the ring's write counter and never restarts; the
engine's tick restarts with each engine. Epochs start at a swap, a rebase (a device at the other
44.1/48 kHz rate: the engine is rebuilt at that rate with the device stopped), a device change,
a reroute and a resume. `rb_host_clock_consistent` checks the line by integer
cross-multiplication; `rb_host_output_time_ns` maps an audio frame to when it reaches the
speakers from the last device callback (its time, the ring's position, the device's latency),
and refuses (`RB_HOST_ERR_NOCLOCK`) until a callback has run in the current epoch.

**The device.** miniaudio compiled apart (`ma_config.h`: only the device layer and its
converter). Listing, choosing, changing (`rb_host_select_device`), sleep and wake
(`rb_host_suspend`/`resume`: the device is closed, reopened on wake, the ring flushed, a new
epoch), a lost device (the notification sets a flag; `rb_host_poll` reopens the default one).
The rate: 48 kHz if the device takes it or anything, else 44.1 kHz if that is native, else 48 kHz
and miniaudio converts (`rb_host_pick_rate`). The device's first callback asks for its whole
buffer at once, so the render thread fills the ring that much higher while the device starts.
An **underrun** (the ring can't fill a callback) fades the last 2 ms of the real audio to zero,
plays silence, fades back in when audio returns, and is counted with its frames; nothing is
repeated.

**Offline mode** (`RB_HOST_BACKEND_OFFLINE`): no threads; `rb_host_offline_render` runs the same
`rbh_run_block` the render thread runs, taking commands and swaps from the same queues.
`rb_cli sound --trace FILE` writes the commands its drive gave the engine (the build, the frames
settled and recorded, every parameter at its frame), and `rb_host_render` plays the file and
writes a WAV.

**Instrumentation** (`rb_host_get_stats`): per-block render time (last, max, total), a deadline
histogram (tenths of the block's duration, then 1–2× and 2×+), the ring's fill (min, mean, max,
last), device callbacks, frames played, underruns and their frames, commands applied, refused and
stale, swaps, retired engines freed, events lost, the priority obtained (as the system reports
it, with the policy in words), the process's memory (private bytes now, peak working set), and
the render thread's forbidden calls.

**No physics on the device thread; no allocation, lock or logging on the render thread**,
checked, not trusted: all the host's allocation, mutexes and logging go through `audit.c` and
`platform.h`, which count a call made by a thread that marked itself the render thread; the
tests assert the counts are zero after the render thread has run commands, swaps, snapshots and
events; and the test binary built with GNU ld (Linux, MinGW) wraps `malloc`, `calloc`, `realloc`
and `free` to count the ones made on the render thread by anything, the core's code included.
`test_device_thread_never_runs_the_physics` pulls 200 callbacks with the render thread stopped
and checks the engine's step counter did not move.

## Decisions and open questions

Made without asking (nobody could answer); each is easy to reverse.

- **Stereo ring, mono engines duplicated.** The device is stereo; a mono engine plays the same
  samples in both ears. The offline mode writes the engine's own channels (one, or two with a
  scene), so it compares with `rb_cli`'s WAVs directly.
- **Fade = whole blocks, 4 to 6 of them** (512 to 768 frames: 10.7 to 16 ms at 48 kHz, 11.6 to
  17.4 ms at 44.1 kHz), chosen from `fade_ms` (default 15: 6 blocks, 16.0 ms). A fade that is a
  whole number of blocks lets the engines change exactly at a block boundary, as the roadmap
  says. The same frame count serves both rates, so a rate change never changes the table.
- **No other command is taken during a swap's two fades** (about 32 ms). The ones behind it
  apply to the new engine once the fade-in ends. The alternative (taking them during the
  fade-out, for the old engine) would let a `SET_PARAM` race a swap.
- **A command carries an expected generation, 0 meaning any**; a swap's default is the generation
  active when its build began, so a swap that lost a race is stale. `RB_HOST_ANY_GENERATION`
  skips the check (the rapid-swap tests use it).
- **Underruns fade out inside the last 2 ms of the data that exists, never by looping.** The
  ring is not given an extra 2 ms of latency for it. If fewer than 2 ms of real audio are left,
  the last real sample is held and faded (a decaying DC step, not repeated audio).
- **The device's first callback asks for its whole buffer at once** (three 10 ms periods, more
  than the ring's 20 ms target), so while the device starts the render thread fills the ring to
  the target plus the device's latency. Without this the first callback was an underrun.
- **Rate changes rebuild the engine** (`rebase`): the device is stopped, the active build is made
  again at the new rate, swapped in with no fade (nothing is playing), a new epoch and
  generation. The controls are not carried over: the engine is at rest, like any rebuild
  (contracts/runtime-v1.md's reset recipe).
- **Offline = same function, caller's thread.** `rbh_run_block` is the render thread's loop body;
  there is no second implementation of anything the offline mode checks.
- **`rb_cli sound --trace` records every parameter set before the first block as the build's
  `param` lines** and every later one as `at FRAME set`, and the host replays them in the same
  order, so the order of the core's calls is the same. A trace holds the drive as it happened:
  telemetry-dependent triggers (the starter letting go) are just frames in it.
- **The trace text has names, not numbers** (`param throttle`, `edit header_length 2 0.55`,
  `preset i4`), so it survives a new parameter being appended.
- **`rb_host_poll` is the only place engines are freed and acknowledgments are taken**; it
  stashes acks an internal wait (device change, resume) took off the queue, so no ack is lost.
  One control thread per host.
- **No test plays on a real device.** The default backend only exists in `rb_host_create`'s
  rate probe (`rb_host_devices` and `rb_host_stress --backend default`, never run here).
- **The tests accept at most 2 underruns on the null device** (two real-time tests): a shared CI
  runner can stall a thread. Exactly zero is the real-device run's evidence. The deterministic
  underrun tests do not depend on timing.
- **Windows MMCSS failure is a fallback, said so.** If MMCSS isn't available the render thread
  takes the highest ordinary priority, `boosted` is 0 and the policy says why; the priority test
  accepts either, because a CI VM may lack the service. On this machine it got MMCSS
  "Pro Audio" (OS priority 15).
- **miniaudio 0.11.25, not 0.11.26** (released 2026-10-31, nine days old): CLAUDE.md's two-week
  rule.
- **A planted-bug table may name its own test program** (`TARGET = "rb_host_tests"`), a small
  change to `tools/plant_bugs.py`, which also builds with `-j4`. The `host/src/build.c` first
  written was renamed `sim_build.c`: plant_bugs' copy skips anything called `build*`.

Open questions for the reviewer and Bernardo:
- The ring target (20 ms) and the device period (10 ms) are the app-design defaults; the real
  device's wake-up jitter decides whether they hold (see "What is left").
- Whether the app wants `rb_host_poll`'s recovery from a lost device to be automatic, as now, or
  reported first. Today a lost device reopens the default device inside `poll`.
- macOS and Linux scheduling classes are stage 126's; the POSIX platform file reports "normal".

## Tests

`python tools/run_tests.py host` (MSVC, with rb_cli parity): 58 tests, 5,239 checks, about 6 s;
w64devkit gcc: 59 (one more: the C library's allocator, wrapped by GNU ld), 5,241 checks; WSL
gcc: 59, 5,241, and the same 59 under **ThreadSanitizer: no report** (about 1 min). By seam:

| File | Tests | What they check |
| --- | --- | --- |
| `test_channels.c` | 10 | The ring, the queue, the triple buffer and the event ring each: FIFO and wrap, full and empty, bad sizes, flush; and a concurrent stress each (2 M audio frames in random chunks through a 256-frame ring, in order and whole; 1 M records through a queue of 8, untorn; 500 k snapshots read as fast as written, never torn or backwards, published = read + dropped; 300 k events, taken + lost = all) |
| `test_host.c` | 21 (20 without the allocator wrap) | The render thread fills the ring to 20 ms and keeps it there; the priority is asked for and reported; a command is applied at a block start and acknowledged with generation, epoch, tick and frame; acks in order with their ids; a full queue refuses at once with no id; refusals carry the core's code; every command kind; 5,000 commands through a queue of 256, none lost, in order; snapshots whole, on the clock's line, about 60 a second; a slow reader gets the newest and drops are counted; events in order, losses = the gaps; the clock (epoch, rates, cross-multiplication, the origin, sub-ticks); 44.1 kHz natively; the output time (needs a callback in the epoch, includes the device's latency); the underrun (2 ms fade out, silence, fade in, counted, frames exact); a short run of data; the audit counts only the render thread; **the render thread never allocates, locks or logs**; and never calls the C library's allocator; the deadline histogram bins; the statistics |
| `test_swap.c` | 9 | A swap installs the engine, acknowledges generation 2 / epoch 2 at a block boundary, frees the old one off the thread, publishes the new generation's snapshots, leaks nothing; **fades measured for clicks** (below); **failure** (bad preset, bad quality, impossible engine: FAILED with the core's code and words, the swap refused, the running engine, its generation, its sound and the live engines untouched); the prepare table is bounded; **stale generation** (refused, its engine freed, the first swap's tone is what plays); **rapid successive swaps** (one generation and one fade each, in order, no overlap, no step); a full retired queue (BUSY, the build stays with the control thread, then works); commands behind a swap wait and apply to the new engine, those for the old are stale; 40 swaps with a device thread pulling flat out and a reader of snapshots: no click, no torn snapshot, no leak, no forbidden call |
| `test_offline.c` | 6 | A scripted drive of a real engine through the offline mode equals the core driven directly through rb.h, bit for bit, mono and in a stereo scene (an independent reference: public calls in rb_cli sound's order); snapshots at any rate and events on change nothing; runs repeat exactly; what only a device host does is refused; a swap in the offline mode fades to silence at the switch and repeats exactly |
| `test_trace.c` | 4 | The format (build, commands, builds named for swaps); 17 kinds of error name their line; the player renders what a host rendered by hand, skipping the skip; a swap through a trace fades |
| `test_device.c` | 9 | The rate choice; the null backend lists one device; it plays in real time (about the frames real time asks for; priority; the output time maps a frame to the future); 44.1 kHz natively; a device change is a new epoch with the sound going on, and a missing device is refused without touching the playing one; a device at the other rate rebuilds the engine (generation + 1, new epoch, the old one freed); suspend stops it and resume restarts it in a new epoch; a lost device is noticed and the default reopened, a reroute is a new epoch; **the device thread never runs the physics** |

`tools/test_host_parity.py` (7 tests) checks the parity script's own decisions with fake programs.
`tools/run_tests.py` has a new `host` suite (configure, build, `ctest -L host`, parity) in the
cmake suite's build folder; the cmake suite's ctest now skips the host's label.

**Bit-identical to rb_cli**: `tools/host_parity.py` renders 9 drives with `rb_cli sound --trace`
(float WAV) and then plays each trace with `rb_host_render`: idle (low and high quality), rev of a
cross-plane V8 into its limiter, a sweep with the car in gear and the clutch, a dyno hold, a
closed-throttle hold, the boxer's rev in the pass-by stereo scene, a cabin idle with a load and
two layers, a build with edited headers, a chambered muffler, a seed and variation. **All 9 are
identical, byte for byte, with MSVC, with w64devkit's gcc and with gcc in WSL.** `rb_cli sound`
without `--trace` is unchanged (checked: the same WAV with and without it).

## Planted bugs

`tools/bugs/stage123.py`: **39 bugs, all caught** by `rb_host_tests` (`python tools/plant_bugs.py
123 WORKDIR NAME...`, two work dirs at a time, about 25 s a bug). One was missed at first:

- **deadline-never-missed** (a missed deadline counted in the 90 to 100 % bin) passed, because no
  test could make a block take longer than its duration. The binning is now `rbh_deadline_bin`,
  a pure function with `test_host_deadline_bins_are_tenths_then_missed`, which catches it.

Writing the Linux run also found a bug in a *test*, not planted: `test_ring_fifo_wrap_and_counts`
wrote 20 floats into an array of 16. It passed on Windows and under ThreadSanitizer and aborted
with "stack smashing detected" on the Linux gcc build (stack protector on); it is fixed.

The four the roadmap asks for: a torn snapshot (`torn-snapshot`, `snapshot-from-two-moments`), a
lost command (`lost-command`, `command-dropped-by-the-render-thread`), a swap without fade
(`swap-without-fade`, `swap-without-fade-out`, `swap-fade-too-short`) and a priority not set
(`priority-not-set`).

| Bug | Caught by |
| --- | --- |
| `ring-forgets-the-wrap` | `ring_fifo_wrap_and_counts`, `ring_stress_in_order_no_loss` |
| `ring-never-looks-full` | `ring_fifo_wrap_and_counts` |
| `lost-command` | `queue_fifo_refuses_when_full_and_wraps`, `queue_stress_no_loss_no_tear`, `host_no_command_is_lost_under_load` |
| `torn-snapshot` | `triple_stress_never_torn` |
| `snapshot-from-two-moments` | `host_snapshots_are_whole_and_published_about_60_times_a_second`, `host_clock_maps_physics_ticks_to_audio_frames` |
| `event-loss-not-counted` | `evring_orders_and_counts_losses`, `evring_stress_taken_plus_lost_is_everything`, `host_events_arrive_in_order_and_losses_are_the_gaps` |
| `command-dropped-by-the-render-thread` | `host_acks_come_in_order_with_their_ids`, `host_no_command_is_lost_under_load` |
| `stale-generation-applied` | `swap_stale_generation_is_refused_and_its_engine_freed`, `swap_commands_behind_it_wait_and_apply_to_the_new_engine` |
| `commands-applied-a-block-late` | `offline_mono_drive_is_the_cores_own_bit_for_bit`, `offline_stereo_scene_drive_is_the_cores_own_bit_for_bit` |
| `refusal-looks-like-success` | `host_refused_commands_say_why`, `host_every_command_kind_reaches_the_engine` |
| `swap-without-fade` | `swap_fades_without_a_click`, `offline_swap_fades_and_repeats_exactly`, `trace_swap_plays_through_a_fade` |
| `swap-without-fade-out` | `swap_fades_without_a_click`, `offline_swap_fades_and_repeats_exactly`, `trace_swap_plays_through_a_fade` |
| `swap-fade-too-short` | `swap_fades_without_a_click` |
| `swap-generation-not-advanced` | `swap_installs_the_new_engine_and_frees_the_old_one`, `swap_rapid_successive_swaps_each_get_their_own_fade_and_ack` |
| `swap-epoch-not-advanced` | `swap_installs_the_new_engine_and_frees_the_old_one`, `swap_rapid_successive_swaps_each_get_their_own_fade_and_ack` |
| `swap-leaves-the-audio-clock-behind` | `swap_installs_the_new_engine_and_frees_the_old_one` |
| `old-engine-never-freed` | `swap_installs_the_new_engine_and_frees_the_old_one`, `swap_stale_generation_is_refused_and_its_engine_freed`, `swap_busy_when_engines_are_not_being_freed_then_retried` |
| `stale-swap-leaks-its-engine` | `swap_stale_generation_is_refused_and_its_engine_freed` |
| `full-retired-queue-not-noticed` | `swap_busy_when_engines_are_not_being_freed_then_retried` |
| `failed-build-leaks` | `swap_failed_preparation_leaves_the_running_engine_untouched` |
| `failed-build-reported-ready` | `swap_installs_the_new_engine_and_frees_the_old_one`, `swap_fades_without_a_click`, `swap_stale_generation_is_refused_and_its_engine_freed` |
| `underrun-clicks` | `host_underrun_fades_out_in_2ms_then_silence_then_fades_in` |
| `underrun-repeats-old-audio` | `host_underrun_fades_out_in_2ms_then_silence_then_fades_in`, `host_short_data_before_an_underrun_still_fades_to_zero` |
| `underrun-not-counted` | `host_underrun_fades_out_in_2ms_then_silence_then_fades_in` |
| `audio-fades-in-never` | `host_underrun_fades_out_in_2ms_then_silence_then_fades_in` |
| `priority-not-set` | `host_render_thread_priority_is_set_and_reported` |
| `allocation-on-the-render-thread` | `render_thread_never_allocates_locks_or_logs`, `render_thread_never_calls_the_c_libraries_allocator` |
| `lock-on-the-render-thread` | `render_thread_never_allocates_locks_or_logs` |
| `log-on-the-render-thread` | `render_thread_never_allocates_locks_or_logs` |
| `physics-on-the-device-thread` | `device_thread_never_runs_the_physics` |
| `physics-rate-forgets-the-oversample` | `host_clock_maps_physics_ticks_to_audio_frames` |
| `device-latency-ignored` | `host_output_time_needs_a_device_callback_in_the_epoch` |
| `snapshots-twice-as-often` | `host_snapshots_are_whole_and_published_about_60_times_a_second` |
| `mono-engine-in-one-ear` | `offline_mono_drive_is_the_cores_own_bit_for_bit` |
| `trace-ignores-its-skip` | `trace_plays_what_a_host_renders_skipping_the_skip` |
| `prefers-44100` | `device_rate_choice` |
| `lost-device-ignored` | `device_lost_is_noticed_and_the_default_device_is_opened_again` |
| `resume-keeps-the-old-epoch` | `device_suspend_stops_the_device_and_resume_restarts_it_in_a_new_epoch` |
| `deadline-never-missed` | `host_deadline_bins_are_tenths_then_missed` |

`tools/bugs/stage123_python.py`: 4 bugs in `host_parity.py` (a truncated render passes, a failed
host passes, the host plays another trace, the comparison uses dithered files), all caught by
`test_host_parity.py`. `plant_bugs.py --check` reports 0 stale bugs.

## Measured

Bernardo's i7-1355U laptop, MSVC (Release `/O2 /GL`), the null device, the render thread in MMCSS
"Pro Audio" (OS priority 15). `rb_host_stress --backend null`, a 20 s scripted drive cycle of the
four (key, idle, blip, full throttle into the limiter, coast):

| Run | Render time per 2.67 ms block (mean / max) | Blocks over their duration | Underruns | Memory |
| --- | --- | --- | --- | --- |
| i4, **low** quality, 40 s, no swaps | 1.72 ms (64 % of real time) / 3.51 ms | 4.8 % | 5 (256 frames of silence) | 3.2 MB, growth after 30 s: 0.00 MB |
| i4, **high** quality, 15 s | 3.12 ms (117 %) / 9.71 ms | 89 % | 667 (104,416 frames) | 3.2 MB |
| low quality, a swap to the next preset every 20 s, 60 s | 2.25 ms / 6.08 ms | 20 % (after the first swap, to a heavier engine, most) | 383 | 3.2 MB, growth 0.03 MB |

The deadline histogram for the first run: 0 % of blocks under 30 % of their duration, 30 % at
40 to 50 %, 30 % at 50 to 60 %, 6 % at 60 to 70 %, 5 % at 70 to 80 %, 10 % at 80 to 90 %, 12.5 %
at 90 to 100 %, 4.8 % over. **The core does not render the default (high) quality of even the four
in real time on this machine: that is stage 118's work, not the host's.** What the host does about
it is what the roadmap asks: the underruns are counted, faded (2 ms) and never repeated, and
the interface can show them. The 15-minute real-device run with zero underruns therefore cannot
pass until the core is faster; the tool is ready (below). The render thread made no allocation,
lock or log in any run. The ring held 0 to 2,304 frames (the top is the start-up priming);
its mean fill was 662 of a 960 target.

**The fades, measured for clicks** (`test_swap_fades_without_a_click`, 12 swaps between a 220 Hz
and a 330 Hz tone at whatever phase each fell): fade 16.0 ms; the largest sample-to-sample step
across a swap is **0.999 times the tones' own largest step** (a click would be well above 1); at
the moment the engines change the level is 0.0026 and 0.0000 of the tones' own (out and in).
With the fades planted away (`swap-without-fade`) the same measure reads **22.6** and 1.33 / 0.006.

**Clock.** `rb_host_clock_consistent` holds for every ack and snapshot checked; across a swap
the audio frame goes on and the engine's tick restarts (origin 0 for a cold engine). The output
time is the last callback's time plus the ring's distance plus the device latency: a test checks a
callback's first frame arrives 10 ms (the set latency) after the callback ran.

**Build.** The host's own files under `/W3` and gcc's
`-Wall -Wextra -Wpedantic -Wshadow` with no warning (warnings are errors for the host in CMake).
A full host build (core, host, tests, tools) is about 16 s with Ninja and MSVC.

## The real-device run (2026-11-09)

On Bernardo's laptop, the default output (his Bluetooth earbuds, 48 kHz), MSVC build, the
render thread in MMCSS "Pro Audio": `rb_host_stress --backend default --minutes 15 --quality
tier --presets i4,i6,boxer4 --swap-every 60`, the drive's 20-second cycle (key, idle, blip,
full throttle into the limiter, foot off) on the four, the six and the boxer in turn, each at
its declared real-time quality (stage 118), 14 swaps.

| Measure | Result |
| --- | --- |
| Underruns | **0** (90,003 device callbacks, 43.2 million frames) |
| Render time per 2.67 ms block | mean 1.36 ms, max 8.81 ms |
| Blocks over their duration | 895 of 337,517 (0.27 %; 2 over twice it), all absorbed by the ring |
| Ring fill | min 0, mean 704, max 2304 frames (target 960) |
| Memory | 3.5 MB at the end, peak working set 14.3 MB, no growth after 30 s |
| Forbidden calls on the render thread | none (allocations, locks, logs 0) |

The ring touched empty once without an underrun: the margin is thin on this laptop's
heaviest blocks, which the deadline histogram's tail shows. Before the run, at the merge, the
tool learned `--quality tier|balanced` and `--presets`, and its verdict no longer counts a
slow block as a failure: the roadmap's evidence is the histogram and zero underruns, and a
slow block the ring absorbs is not heard. The V8s were left out: they miss real time at low
on this laptop (stage 118).

After the run the host suite crashed now and then: `rb_host_render` printed its summary from
the trace after freeing it (a use after free in the tool, not the host; its WAVs were already
written and closed, so parity always held). It reads the rate and channels first now; 30
renders of the three traces that crashed ran clean.

## What is left

- **The V8s on the real device**, once stage 118's tier reaches them (a faster machine or core).
- **No MSVC ThreadSanitizer.** The concurrent tests run under it on Linux only.
- **The clock's physics tick** is derived from the audio frame and the oversample (the core
  exposes the engine's tick only inside a snapshot, so it is read from one at each epoch's
  start). For the tone source, whose engine never steps, the tick the clock reports is the
  derived one, not a counted one.
- **Not done, on purpose:** macOS and Linux scheduling classes (stage 126); crossfading two
  engines (app-design §5 names it a later option); a capture tap for WAV export from the live
  host (the offline mode and `rb_host_render` write WAVs; the live tap is stage 104).
- **Core speed (stage 118)** and the deadline histogram's second look afterwards.
- **For the merge:** shared files touched are `CMakeLists.txt` (an appended block),
  `Makefile` (TSAN option, a host section, `comma` defined earlier), `.github/workflows/ci.yml`
  (one job and a comment), `tools/run_tests.py` (a `host` suite, `RB_TESTS_OUT`, the cmake
  suite skips the host label), `tools/test_run_tests.py` (one line), `tools/plant_bugs.py`
  (`TARGET`, `-j4`), `native/sound.c` and `native/rb_cli.c` (`--trace`). Nothing under `core/`.
