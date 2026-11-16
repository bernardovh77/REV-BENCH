# Stage 066: the playable engine and its audio lifecycle

Done 2026-11-16 by Claude (Opus 5.5), with Bernardo out; his checks on his own devices are
listed at the end, and the stage stays 🟡 until they are made. The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-066).

**The question:** can the app be driven like an engine (started, idled, blipped, pulled,
lifted, stopped) with the keyboard, the mouse, a controller or an analog key, through every
preset at its tier, in mono or a stereo scene, while the audio follows devices, sleep and
quit, and can it be proved that what the interface drives is exactly what the host renders?

## The gaps before it

PLAN.md's open rows that could touch it:
- **118: the V8s' speed.** Closed just before this stage by Bernardo's choice of the 19.2 kHz
  pipes (`RB_QUALITY_LEAN`, commit 6dd8f0f, docs/stage-118.md "Since: the V8s' lean tier").
  The app plays every preset at its tier and says so when the computer falls behind.
- **118: the four 1.05 dB off at idle.** Safe to leave: a sound tolerance, not something the
  app's driving touches.
- **039, 041, 042, 014.** Phase B's and old stages': none touches the app.

## What was built

**The engine process** (`app/src/engine/`):
- `throttle-guard.ts`: **the dead-man rule.** A live throttle (ramped key, analog key,
  trigger) is sent at a 50 ms beat; with no beat for 250 ms the engine process ramps it to
  zero over 100 ms, and at once when the window's port closes. On/off and latched throttles
  (and the old `set-param throttle`) hold.
- `control.ts`, new commands: `throttle {value, live}`, `volume`, `set-latency`,
  `capture-dump` (tests only), and `load-preset` with a `quality` and a `scene`. The listener's
  place and the A/B variant are kept for every later build (a rebuild used to put them back to
  the core's defaults). The greeting carries the build and these settings.
- **Levels:** every build's `master_gain` is `APP_HEADROOM` (0.3) times the volume (0..2). The
  core's output is the pressure at the listener, so the engines keep their real relative
  loudness; the headroom puts the loudest rev under the limiter (below, "Levels").
- **Devices:** the host already moves to the system's default when a device is lost; the
  engine process now notices (`deviceEvents`, checked every 500 ms) and tells the window,
  which names the new device in a notice (app-design §15). Choosing a device says so too.
- **Latency:** the host takes its ring target only when made, so a change makes the host
  again with it, the engine rebuilt as it was, and greets the window again (the sound stops
  for a moment; commands in flight are answered `RESTARTED`). Kept with the autosave.
- **Sleep and wake:** Electron's `powerMonitor` → the supervisor → `host.suspend()` /
  `resume()` (stage 123's flush and new epoch).
- **Quit with a fade:** the first quit waits for the engine: `master_gain` to 0 (the core
  smooths it with 10 ms), 90 ms, then the host stops and the process exits (or the
  supervisor's kill after 2 s). No click.
- **Minimised or hidden:** the sound goes on (the engine is its own process); the window's
  throttle lets go (below).

**The window** (`app/src/ui/`):
- `throttle-input.ts`: Bernardo's throttle styles: on/off (W snaps 0 ↔ 100 %), ramped (hold
  W: 400 ms up, 250 ms down), analog key, controller trigger, and the mouse (slider and
  wheel, latched). Blur lets go of held keys and analog travel.
- `gamepad.ts`: the right trigger (standard mapping) and the **analog key, learned**: the next
  controller input pressed past half-way becomes the throttle, whichever button or axis Razer
  Synapse's emulation maps it to (an axis resting at −1 is read over its whole span).
- `driver.svelte.ts`: the keys (W/Up throttle, I ignition, S starter held, B the A/B switch;
  ignored in text fields and drop-downs and with Ctrl/Alt/Meta), blur and hide, the controller
  every frame, and the A/B switch: a candidate (1..7) from the developer section, and a blind
  mode where a coin decides which side is the candidate, revealed after.
- The top bar gained the scene (mono or one of the eight stereo scenes, a rebuild) and the
  listener's place (live, mono only). The centre panel holds the throttle style, the quality
  (the preset's tier by default, any other only when chosen), volume, output device and
  latency, a minimal readout (rpm, throttle, render time against real time, ring, underruns,
  the device playing) and the developer section.
- **No hidden quality change** (app-design §11): when blocks miss their deadline or the device
  runs dry between two statistics, a notice says the computer isn't keeping up and offers the
  next cheaper quality as a button.

**The host's capture tap** (`host/`): with `rb_host_config.capture_frames`, the render thread
copies every block it gives the device into a second lock-free ring, from frame 0 on;
`rb_host_capture_read` takes it, and a block with no room is dropped whole and counted
(`capture_dropped`). Nothing is allocated or waited on in the render thread. The add-on
exposes `captureFrames` and `captureRead`; the engine process's `trace-recorder.ts` writes
every parameter and swap the render thread acknowledged, at the frame its ack gives, as an
`rb_host_trace.h` trace. `RB_CAPTURE_SECONDS` (tests only) turns both on; `capture-dump`
writes them to the user-data folder. The trace format also reads `balanced` and `lean`.

## Levels

`rb_cli sound --rev` at each preset's tier, three seeds, unlimited peaks at `master_gain` 1:
the flat-plane V8 2.17 (+6.7 dBFS), the cross-plane 1.46, the six 1.07; the boxer and the four
far below (the four's whole rev peaks at −15.6 dBFS, about 20 dB under the flat-plane's RMS:
real relative loudness, as asked; its own level is phase B's to check against recordings).
At 0.3 the flat-plane peaks about 0.65, 1.8 dB under the soft limiter's knee (0.8). At the
1 m tailpipe place and in some scenes the loudest engines can still reach the limiter; the
volume (up to 2) is the user's.

## The evidence

| Card's evidence | How | Result |
| --- | --- | --- |
| A drive through the app's UI = the host's offline render of its trace, at 44.1 and 48 kHz on Windows | `app/e2e/drive-parity.spec.ts`: the built app, real add-on, null backend; keys (I, S held, W held), a preset change (a swap with fades), a stereo scene (another swap), the slider; capture-dump; `rb_host_render` (built in the add-on's own build) plays the trace; budget: bit for bit | **Bit for bit at 44.1 and 48 kHz** (about 5 s, 88 commands, two swaps). The first run differed from 3.13 s on, 768 frames before the first swap: a swap is acknowledged when the new engine goes in, after its fade-out, while a trace's `at F swap` is when the fade-out starts. The recorder now dates a swap from the fade's start, sized as the host sizes it (5 blocks at 44.1 kHz, 6 at 48) |
| Start, idle, blip, lift, stop, restart, preset changes, invalid commands, device changes, sleep and wake | vitest (engine control, store, driver, supervisor, process) and the e2e tests on the null device | automated parts pass; Bernardo's real devices: to do |
| The dead-man rule end to end | `drive-parity.spec.ts`: W held and the window hidden → throttle 0; W held and the page's thread blocked 1.5 s → the engine's trace shows the ramp to 0 and the beat taking it back; on/off holds through a hidden window | Passes: hidden → 0; blocked → the trace shows the ramp (values between 1 and 0, then 0) and the beat taking it back to 1, W held throughout; on/off stays at 100 % |
| C and add-on suites green on Linux and macOS | CI | Not yet: runs on the push (Windows, Linux, the sanitizers, macOS) |

**Planted bugs.** C (`tools/bugs/stage066.py`, rb_host_tests, the capture tap): all 6 caught (a block never written, written twice,
drops not counted, the first frame misnumbered, any size accepted, a read without a tap
answering 0). One more, a partial block, was dropped from the table: the tap's ring fills in
whole blocks, so no test could see it.
TypeScript, planted one at a time in the test copy and each run against its vitest file (`python
tools/bugs/app_bugs_066.py APP_COPY`), 16 bugs: the guard never ticked from `pump()`, its
timeout reversed, the ramp a cut, `detach()` not releasing; the ramped key's first tick lost
(also found for real while writing it), blur keeping analog travel, no beat; the sticky
parameters and the volume left out of builds; the recorder keeping stale acks, a swap dated at
its ack; a lost device not told; no quit fade; hiding not letting go; blind mode not blind; the
analog learn taking a key already pressed. **15 caught at first; "hiding not letting go" was
missed** (the test hid a window whose trigger was already at 0): the test now pulls the
trigger to 0.6 first, and catches it: 16 of 16. Two more were found for real by the e2e run rather than
planted: the App's effect re-running on a style change (it reset the throttle), and the
swap's date above.

**Every suite** (`python tools/run_tests.py`, Windows): native, wasm, wasm-c, python, web,
cmake, app and app-e2e passed in one run; host and addon failed in it and passed when rerun.
The add-on's was real: its list of the host's methods lacked `captureRead` (fixed). The
host's was `test_device_null_backend_plays_the_render_threads_audio_in_real_time` (more than
two underruns, the render thread behind the device) right after 50 minutes of native and
cmake builds: a timing test on a loaded laptop, with the tap off in it; it passed before and
after. Worth watching in CI.

## What is left (Bernardo, on his machine)

1. **Listen and drive** on his real devices: every preset at its tier, the six and the V8s
   at lean included (his blind A/B of low against lean for the V8s: the quality picker plays
   either; app-design §15's tolerances ask for it).
2. **His Razer analog keyboard:** Synapse's controller emulation on, "Analog key", "Learn the
   analog key", press it. No public SDK for reading the key travel directly turned up (Razer's
   SDK is for lighting; a developer's request on Razer Insider, December 2024, is
   unanswered), so the Gamepad route stays.
3. **Devices:** unplug headphones while it plays (the notice names the new device); choose a
   device; change the latency; sleep and wake the laptop; quit while revving (no click).
4. Then 066 → ✅ and 067.

## Files

New: `app/src/engine/throttle-guard.ts`, `trace-recorder.ts`; `app/src/ui/lib/throttle-input.ts`,
`gamepad.ts`, `driver.svelte.ts`; tests for each; `app/e2e/drive-parity.spec.ts`;
`tools/bugs/stage066.py`. Changed: the protocol (`throttle`, `volume`, `set-latency`,
`capture-dump`, `load-preset` scene; `device` and `capture` messages; the greeting's `build`
and `settings`), the add-on contract (`deviceEvents`, `deadlineMissed`, `captureFrames`,
`captureRead`), the host (`capture_frames`, `rb_host_capture_read`, `capture_dropped`), the
supervisor (`suspend`, `resume`, `stop(stopped)`), main (powerMonitor, the quit's wait),
`tools/run_tests.py` (app-e2e builds `rb_host_render` in the add-on's build), the e2e
helpers (the engine's drop-down named, now that the top bar has three).
