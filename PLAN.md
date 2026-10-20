# REV-BENCH — Plan of Action

A browser-based internal combustion engine test bench. You pick an engine, change things like
displacement, compression, cams, headers and turbo size, and **hear and see** the result in real
time. Sound comes from a **physics simulation** (the realistic approach), not from samples or simple
synths.

**The sound comes first.** It is Bernardo's top priority: an engine that's hard to tell from a
real one by ear. After stage 12 he found the renders "dull and really artificial", so the plan
gained a sound-realism track (stages S1–S5). Its targets come from recordings of real engines,
and `tools/compare_real.py` measures them (RESEARCH.md, "Sound realism"). Every stage also keeps
a sound check (working agreement 6).

Each stage below is sized to fit in **one prompt**. To continue, just say *"do stage N"*
(or *"next stage"*). After each stage, its box in the tracker is ticked and the "Notes" column
gets a one-line summary of anything that changed from the plan.

---

## Progress tracker

| #  | Stage                                            | Phase            | Status | Notes |
|----|--------------------------------------------------|------------------|--------|-------|
| 0  | Research + plan                                  | Foundations      | ✅     | This file + RESEARCH.md |
| 1  | Native C scaffold, WAV writer, test runner       | Foundations      | ✅     | 14 tests pass (also ASan/UBSan); `make render` → clean sweep |
| 2  | Web scaffold: C→WASM tone playing in AudioWorklet| Foundations      | ✅     | 15.6 KB wasm, 0 imports; browser test measures the real output pitch |
| 3  | Gas & flow primitives                            | Physics core     | ✅     | 3 gases fitted to NASA data, first law, nozzle flow; blowdown within 0.07 % of exact |
| 4  | Single cylinder: slider-crank + motoring         | Physics core     | ✅     | Trapezoidal piston work: TDC within 1e-6 of the exact isentrope, no drift over 1000 revs |
| 5  | Valvetrain, throttle & intake plenum             | Physics core     | ✅     | Motored 2.0 L four: VE 0.79–0.93 wide open, 28 kPa at idle throttle, pumping loss = p_amb − MAP |
| 6  | Combustion (Wiebe) + heat transfer               | Physics core     | ✅     | 2.0 L four at 3000 rpm WOT: IMEP 12.0 bar, peak 58.8 bar at 12.7° ATDC at MBT (25°), 38.8 % indicated; low-speed VE 0.87 |
| 7  | Crankshaft dynamics, multi-cylinder, self-running| Physics core     | ✅     | Free crank with exact energy books; cranks at 238 rpm, idles at 800, limiter 6680–6830; brake 167 N m / 97 kW |
| 8  | Engine layouts & presets                         | Physics core     | ✅     | I4, I6, cross- and flat-plane V8, boxer-4 start, idle and rev; firings every 180/120/90°; engine and cylinder state in the API |
| 9  | Dyno + validation against real engines           | Physics core     | ✅     | Absorbing dyno on the free crank; the four, six and boxer within 5 % of the 1AZ-FE, M54B30 and FB20; lumped intake runners brought forward from stage 10 |
| 10 | 1D exhaust pipes (headers → collector → tailpipe)| Sound            | ✅     | Runners, headers, collectors, exhaust pipes and tailpipes are 1D pipes; a pulse runs at c (−0.16 %) and echoes inverted from an open end; the idle computer looks ahead |
| 11 | Audio extraction → first WAV renders 🎧          | Sound            | ✅     | A microphone hears the tailpipes' volume flow; anti-alias FIR, DC blocker, soft limiter, TPDF dither; four clean renders. Listening checkpoint |
| 12 | Mufflers, intake & listener 🎧                    | Sound            | ✅     | 1D mufflers (chambered, packed straight-through, straight pipe); snorkel + air box + filter, the mouth heard; a listener around the car, each source at its distance and off the ground. A pull gets louder; unequal headers rumble. Bernardo: "dull and really artificial" → S1–S5 |
| 13 | Performance & stability pass                     | Sound            | ⬜     | |
| S1 | Sound: life (cycle-to-cycle variation) 🎧         | Sound realism    | ⬜     | Real idles repeat at +5 dB per cycle, ours at +44 (RESEARCH.md) |
| S2 | Sound: air (flow noise, finer exhaust) 🎧         | Sound realism    | ⬜     | Ours falls off a cliff above 2 kHz: the pipes' grid |
| S3 | Sound: the engine's own sound 🎧                  | Sound realism    | ⬜     | Takes stage 12b's mechanical noise |
| S4 | Sound: stereo and the place 🎧                    | Sound realism    | ⬜     | Takes stage 12b's in-car listener |
| 13b| Performance II: vectorized pipes, implicit valves| Sound            | ⬜     | Split from stage 13: its 4× target |
| 14 | Real-time engine in the browser                  | Browser          | ⬜     | First playable 🎮 |
| 15 | Control panel (presets + parameters)             | Browser          | ⬜     | |
| 16 | Gauges + live dyno chart                         | Browser          | ⬜     | |
| 17 | 3D engine model: block + moving parts            | 3D view          | ⬜     | Built from the core's engine description |
| 18 | 3D engine in motion, in sync with the sound      | 3D view          | ⬜     | |
| 19 | 3D gas, combustion & exhaust flow                | 3D view          | ⬜     | |
| 20 | Turbocharger physics                             | Forced induction | ⬜     | |
| 21 | Turbo sound, turbo UI + 3D turbo 🎧               | Forced induction | ⬜     | |
| 22 | Knock, mechanical limits & engine damage         | Realism          | ⬜     | |
| 23 | More layouts & presets                           | Polish           | ⬜     | |
| 24 | UX polish, save/share builds, quality settings   | Polish           | ⬜     | |
| S5 | Sound: tuning by ear 🎧                           | Polish           | ⬜     | Once everything is built |
| 25 | Deploy to Vercel + README                        | Ship             | ⬜     | 🚀 |
| 26 | Optional extras (gearbox, 2-stroke, export, ...) | Extras           | ⬜     | Pick à la carte |

---

## Architecture (decided up front)

```
REV-BENCH/
├── core/          C11 simulation library. All physics + sound. No deps besides libm.
│   ├── include/   Public header: rb.h
│   └── src/       gas.c, flow.c, chamber.c, cylinder.c, valve.c, crank.c,
│                  pipe.c, combustion.c, turbo.c, dyno.c, audio.c, presets.c …
├── native/        Command-line harness: run the sim offline → WAV + CSV.
├── tests/         Small assert-based C test runner (make test).
├── tools/         Python helpers: plot CSV traces, spectrograms of WAVs.
├── web/           Vite + TypeScript app (AudioWorklet, UI, charts, 3D engine view).
├── docs/          Physics notes per stage (equations used, sources).
├── renders/       Regenerated output: WAVs, CSVs, plots (git-ignored; doc images go in docs/).
├── PLAN.md        This file.
└── RESEARCH.md    Background research.
```

**Key decisions and why:**

1. **The physics core is written in C.** It's Bernardo's strongest language, it's fast, and it compiles both
   natively (for testing and offline WAV renders) and to WebAssembly (for the browser) from the same code.
2. **Everything runs in the visitor's browser.** The C core is compiled to a *standalone* `.wasm`
   (no Emscripten JS glue) and instantiated **inside the AudioWorklet**, so the simulation runs on
   the real-time audio thread. Controls go in, and telemetry comes out at about 30 Hz over the worklet's
   `MessagePort`. Because this avoids `SharedArrayBuffer`, **no COOP/COEP headers are needed**, so
   hosting is plain static files.
3. **Sound comes from the gas.** Each cylinder is a thermodynamic control volume that breathes
   through valves into 1D pipes. The sound is taken from the pressure/flow leaving the tailpipe (and
   intake mouth), the way a microphone would pick it up. That's why firing order, crank type and header
   lengths automatically change the sound.
4. **The sim runs faster than audio.** Physics substeps run at 2–8× the 48 kHz audio rate (this is
   configurable as a quality setting), and the result is anti-alias filtered and decimated down to 48 kHz.
5. **Double precision, SI units everywhere, and no allocation in the hot loop.** Every state has a NaN/inf guard
   with a clamp, so a bad slider value can't blow up the audio.
6. **Native first, browser second.** Each physics stage is validated offline, using pressure traces,
   dyno numbers and WAV files, before it goes anywhere near the UI. That keeps debugging sane.
7. **Two kinds of parameter changes:**
   - *Live* (throttle, spark advance, AFR, boost target, rev limit, volume) change instantly.
   - *Rebuild* (bore, stroke, cylinder count, layout, pipe lengths, turbo size) re-initialize
     the engine with a short audio crossfade.
8. **The 3D view is built from the simulation, not drawn separately.** The 3D model (Three.js) is
   generated from the engine description the core exports, and it moves with the simulation's own
   crank angle and cylinder states, streamed from the worklet at display rate. The picture can't
   disagree with the sound, and any engine you build can be shown. That's why the physics stages
   expose geometry and per-cylinder state through the API, not only sound.

**Toolchain:** `gcc` + `make` for native builds and tests. For the WASM build: `zig cc` 0.17 (any `zig` on
PATH, or `pip install ziglang`). Emscripten is out because its SDK downloads from Google storage,
which isn't reachable here (found in stage 1).
The web side uses Vite 8 + TypeScript 7 with Vitest and Playwright for tests, and later Tweakpane
(controls), uPlot (charts) and Three.js (3D view: WebGPU with automatic WebGL 2 fallback). npm versions are pinned exactly and are at least
two weeks old when added.

**Git:** `Personal-Projects` is already a git repo, so REV-BENCH lives inside it as a subfolder.
Work is committed on the `rev-bench` branch at the end of each stage (stages 0–5 went in as one
commit), and Bernardo merges it into `main` when he wants. Commit messages carry no co-author
lines. `git commit` has to delete its lock files, which Claude's access to the folder can't do
without Bernardo's permission, so Claude asks for it in each session that commits.

---

## Stages

Each stage lists what gets built, then **Done when** (the acceptance check) and, where relevant,
**Bernardo checks** (something to look at or listen to).

### Phase 0 — Foundations

#### Stage 1 — Native C scaffold, WAV writer, test runner
- Folder structure above, `Makefile` with `make`, `make test`, `make render`.
- `core/` with a stub `rb_sim` API (`rb_create`, `rb_step`, `rb_render_block`, `rb_destroy`).
- 16-bit/float WAV writer, CSV logger, and a tiny test framework (`CHECK`, `CHECK_NEAR`).
- `native/rb_cli` renders a 2 s test signal to `renders/test.wav`.
- `.gitignore`, `README.md` (how to build on WSL).
- **Done when:** `make test` passes and `make render` produces a WAV that plays.

#### Stage 2 — Web scaffold: C→WASM tone in an AudioWorklet
- Install the WASM toolchain and add `make wasm`: `zig cc -target wasm32-wasi` (via `pip install ziglang`)
  or wasi-sdk, producing a standalone `.wasm` with libc/libm built in and no JS glue. The built
  `rb_core.wasm` is committed in `web/src/wasm/`, so the web app runs even on a machine without the toolchain.
- `web/` Vite + TS project. The main thread fetches `rb_core.wasm` and reads parameter metadata from its
  own instance; the AudioWorklet gets the bytes, compiles its own instance and calls `rb_render_block`.
- Unit tests (Vitest) for the WASM core, and a Playwright browser test that measures the pitch of the
  real audio output.
- A "Start" button (browsers need a user click before audio), a frequency slider sent via the port,
  and a telemetry echo back to the page.
- **Done when:** `npm run dev` plays a C-generated tone whose pitch follows the slider without glitches.

### Phase 1 — Physics core (native, offline, validated)

#### Stage 3 — Gas & flow primitives
- Gas properties (R, γ, cp, cv) for air and burned mixture, with blending by burned fraction.
- `chamber`: a 0D control volume (mass, energy → pressure, temperature) using the first law.
- `flow`: compressible isentropic flow through an orifice (subsonic + choked), with a discharge coefficient.
- Integrator choice (semi-implicit/RK2) and a fixed substep.
- **Done when:** tests pass for mass/energy conservation between two tanks, a tank blowdown
  that matches the expected choked → subsonic behavior, and equalization to the same pressure.
- *Built:* three pseudo-species (air, gasoline vapour, burned gas) with two-range cp(T) fits to the
  NASA Glenn data, explicit Euler with an equalisation limit on each connection, and a `blowdown`
  demo in the CLI (details in `docs/stage-03.md`).

#### Stage 4 — Single cylinder: slider-crank + motoring
- Geometry: bore, stroke, rod length, compression ratio → clearance volume, `V(θ)` and `dV/dθ`.
- The cylinder as a chamber with a moving piston (p·dV work). Valves closed, crank at fixed RPM.
- CSV of p–V and p–θ, plus a plot script in `tools/`.
- **Done when:** peak motoring pressure ≈ `p0·CR^γ` (within a few %), and the p–V loop closes
  with no energy drift over 100 cycles.
- **Bernardo checks:** the p–V plot looks like a textbook motoring loop.
- *Built:* `cylinder.c` (slider-crank) and `rb_chamber_set_volume()`, which applies the piston work
  with the time-symmetric trapezoidal rule so a closed cylinder can't drift. The crank angle
  convention (0° = firing TDC, 720° cycle) is fixed from here on. `rb_cli motor` writes the trace and
  `tools/plot_cycle.py` draws it (details in `docs/stage-04.md`).

#### Stage 5 — Valvetrain, throttle & intake plenum
- Cam lobe profile (duration, max lift, lobe centerline), lift → curtain area, separate
  intake and exhaust cams, overlap.
- Throttle body orifice (angle → area), intake plenum volume, runners as simple orifices for now,
  and an exhaust dumping into an ambient reservoir.
- Motored engine at fixed RPM: measure **volumetric efficiency** vs RPM and throttle.
- Check the per-connection equalisation limit (`RB_FLOW_LIMIT`) once the plenum feeds several
  runners; scale it by the number of strong connections if a small chamber's pressure oscillates.
- The cylinder becomes an open chamber: each step applies the valve flows first (explicit Euler),
  then the piston (`rb_cylinder_move`, trapezoidal).
- **Done when:** VE is roughly 0.8–0.95 at wide-open throttle, drops sharply when closed, and manifold vacuum looks
  plausible (about 30 kPa at idle-ish throttle).
- *Built:* `valve.c` (cam lift `(1 - u²)²`, valve curtain in series with the port), `throttle.c`
  (butterfly plate with its shaft), and `engine.c`: N cylinders on a plenum, motored at a fixed rpm,
  with per-cycle VE, plenum pressure and pumping loss. `rb_cli breathe` (and `--sweep`) and
  `tools/plot_breathe.py` show it (details in `docs/stage-05.md`). VE peaks too early (2500 rpm)
  because the runners have no inertia yet; see stage 10.

#### Stage 6 — Combustion + heat transfer
- Fuel (gasoline to start; data-driven so E100/methanol can be added later), AFR, and the trapped mass of fuel.
  Burning turns AIR + FUEL (14.596 : 1) into BURNED, the species stage 3 already tracks, and adds the
  lower heating value of the fuel burned as heat.
- Spark advance plus a Wiebe-function burn (duration, shape), combustion efficiency vs AFR.
- Simplified Woschni wall heat transfer. Check: with heat loss, a motored cylinder's peak pressure
  moves slightly before TDC (the thermodynamic loss angle, under a degree).
- Hot residual gas and warm walls should bring stage 5's low-speed VE (0.92, motored) down to a
  realistic 0.80–0.85.
- Outputs: IMEP, indicated torque and peak pressure location.
- **Done when:** a 2.0 L NA I4-sized cylinder at WOT gives IMEP ≈ 11–13 bar, peak pressure lands about
  10–20° ATDC at MBT timing, and too much advance visibly raises peak pressure while lowering torque.
- *Built:* `combustion.c` (heat release with the constant-volume correction, the Wiebe burn, and an
  unmixedness model for how much fuel can burn: best power at AFR 13), `heat.c` (Woschni), and in
  `engine.c` fuel injected at intake valve closing, the spark, the burn stopping at exhaust valve
  opening, and an exhaust manifold + muffler (lumped volumes until stage 10). The burned gas's cp
  above 1000 K now includes dissociation (within 2 % of Cantera's equilibrium). `rb_cli fire`
  (`--spark-sweep`, `--rpm-sweep`) and `tools/plot_fire.py` show it (details in
  `docs/stage-06.md`). All checks met except low-speed VE: 0.87 instead of 0.80–0.85.

#### Stage 7 — Crankshaft dynamics, multi-cylinder, self-running
- Crank and flywheel inertia, gas torque through the rod geometry, friction (a Chen-Flynn style FMEP),
  starter motor and an ignition switch. Take each cylinder's gas torque from the work of its step,
  `τ = -W / Δθ` (W from `rb_cylinder_move`), so the energy balance between gas and crank stays exact.
- N cylinders sharing a crank with a firing order and phase offsets (stage 5's `rb_engine` already
  has them, turned at a fixed rpm). Throttle, idle air control and a rev limiter (spark cut).
- Stage 6's `rb_engine_cycle.torque` is the indicated torque (the gas's net work over 4π); the
  brake torque is that minus friction.
- **Done when:** an I4 cranks, starts, idles stably (about 800 rpm), revs up and down with
  the throttle, bounces off the limiter, and shuts down when ignition is cut.
- *Built:* in `engine.c`, a free crank (`hold_speed = 0`) whose kinetic energy changes by exactly
  the work done on it: gas, crankcase, starter, friction and load. Also Chen-Flynn friction with
  brake torque, BMEP and brake power, a DC starter with a one-way clutch, and an engine computer:
  idle air valve, fuel cut on overrun, spark-cut rev limiter and a late cranking spark. The stage 6
  review fixed spark timing changing mid-burn (it's now set at intake valve closing). `rb_cli run`
  and `tools/plot_run.py` show a whole drive (details in `docs/stage-07.md`).

#### Stage 8 — Engine layouts & presets
- Layout description: inline / V (bank angle) / boxer, crank pin angles, firing order and
  per-cylinder offsets, including **cross-plane vs flat-plane V8**.
- Presets in C (`presets.c`) with exported getters/setters so the web UI can list and edit them.
  First set: 2.0 I4, 3.0 I6, 5.0 cross-plane V8, 4.5 flat-plane V8, 2.0 boxer-4.
- Export the engine description through the API (cylinder positions and bank angles, crank pin
  angles, bore, stroke, rod length, valve sizes, cam timing) plus per-cylinder state (pressure,
  temperature, burn fraction, valve lifts). The control panel and the 3D view (stages 17–19) build
  themselves from it, so no engine geometry is duplicated in JavaScript.
- Each preset needs its own crank inertia, starter, idle air valve size (roughly in proportion to
  displacement) and idle controller gains: stage 7 tuned them on the 2.0 I4, and an eight with the
  same settings hunts at idle.
- **Done when:** every preset starts, idles and revs natively, and firing-event timing plots
  show correct spacing (for example, an even 90° spacing for a V8).
- *Built:* layouts in `engine.c`: banks and bank angles, each cylinder's bank, crank pin and
  position, and a firing order, from which `rb_engine_layout` works out the firing angles (and
  refuses layouts that can't work). Five presets in `presets.c`, each with its own idle valve and
  idle gains (`idle_gain_p`, `idle_gain_i`, now part of the description; the four's proportional
  gain came down to 0.000375). The sim holds an engine, and the public API gives the presets, 34
  description fields with names, units and flags, and 8 numbers of each cylinder's state
  (`rb_engine_api.c`, wrapped in `rb-core.ts`). The stage 7 review fixed the engine computer's
  state when the crank switches between held and free. Details in `docs/stage-08.md`.

#### Stage 9 — Dyno + validation
- Absorbing dyno: hold RPM with a PI controller, cycle-averaged torque and power, and a sweep mode
  (e.g. 1500→redline) → CSV torque/power curve + plot. With stage 7 the controller acts on the free
  crank's `load_torque`; the ideal hold (`hold_speed = 1`) stays for quick sweeps.
- Calibrate friction (stage 7 uses Chen-Flynn's commonly used coefficients) against real engines:
  10.5 bar of brake mean effective pressure is low for a modern 2.0 L.
- Compare 2–3 presets against published real-engine numbers and tune the model's free factors
  (discharge coefficients, friction, burn duration).
- Stage 6's knobs: burn duration and Wiebe shape (60° at every speed and load for now, so MBT
  only moves between 20° and 25°; real engines want more advance at high rpm and light load),
  mixture spread, combustion efficiency and wall temperature. Add the fuel's evaporation cooling
  the charge. Low-speed VE is 0.87, against 0.80–0.85 in real engines.
- **Done when:** peak torque and power are within about ±15% of the reference engines, at sensible RPMs,
  and changes behave in the right direction (more CR → more torque until knock; bigger cams → torque
  shifts up the rev range).
- *Built:* `dyno.c`, an absorbing dyno: a PI controller on the free crank's `load_torque` (1.5 Hz,
  absorbing only) with a reading every cycle and the inertia correction. `rb_cli dyno` sweeps or
  holds each speed and writes a CSV, and `tools/plot_dyno.py` charts it. The calibration:
  - a burn that lengthens with speed and at light load (entrainment and burn-up, with Metghalchi
    and Keck's flame speed);
  - the engine computer's spark at the best torque (half burned 8° after TDC), held back at idle as
    a reserve;
  - modern friction (Knauder et al.);
  - lumped intake runners (`rb_duct`: a column of air with inertia, and a port chamber at each
    valve), brought forward from stage 10 because without a ram effect bigger cams only lost torque.

  The four, the six and the boxer make within 5 % of the 1AZ-FE's, M54B30's and FB20's peak torque
  and power. Bigger cams move the torque peak from 3500 to 5000 rpm, and each point of compression
  adds about 2.5 %. The idle valves and gains were retuned for the new engine, and below idle the
  computer now learns the idle valve's position even while the speed changes (a load at idle). The
  stage 8 review gave the API's fields ranges (clamped with `RB_CLAMPED`; angles wrap). The fuel's
  evaporation cooling was left out: it belongs with direct injection. Details in
  `docs/stage-09.md`.

### Phase 2 — Sound (the realistic part)

#### Stage 10 — 1D exhaust pipes
- 1D compressible pipe model (finite-volume with a stable flux scheme, or a lossy waveguide if
  performance demands it). Open-end and closed-end boundaries, area changes, friction/damping.
- Per-cylinder primaries → collector(s) → tailpipe, with configurable lengths and diameters.
  The V8s and the boxer need one collector per bank (the API's `bank` field): the cross-plane's
  burble comes from each bank's uneven pulses meeting there, and the boxer's rumble from
  unequal-length primaries.
- Expose the pipe geometry (lengths, diameters, how pipes connect) and per-cell pressure, for the 3D
  exhaust view (stage 19).
- Use the same pipe model for the intake runners. Stage 5's quasi-steady valves give VE no
  mid-range hump: that comes from the inertia of the air in tuned runners ("ram" effect). Stage 9
  added lumped runners (`rb_duct`, one column of air and a port chamber each); the pipes replace
  them, keeping the `runner_length` and `runner_diameter` fields.
- Then check the calibration again (`test_calibration_*` holds the four, six and boxer within
  ±15 % of their real engines) and retune the idle gains. With the lumped exhaust 28–55 % of the
  charge at idle is burned gas, which makes the idle burns 64–99°. The pipes will change that, and
  the idle gains depend on it. Watch each idle for 30 s: the six's ±2.5 rpm swing only shows after
  15 s. The V8s' idle spark (an absolute 15°) is 20–28° short of their best; a reserve counted from
  the best spark needs their gains retuned.
- The pipes replace stage 6's lumped exhaust manifold (2 L) and muffler (10 L). Check the
  air-fuel ratio at the lightest loads, where the lumped tailpipe still lets some air in (13.6
  instead of 14.6 at 2000 rpm and 3 % throttle).
- Mind the budget: the fired four already costs 1.8 µs per step natively (2.4 µs in WASM, 47 % of
  a core) at 192 kHz. Keep pipes cheap (few cells, a cheap flux scheme, or a lower update rate than
  the cylinders) and measure their cost from the start.
- **Done when:** a pressure pulse travels at the right speed of sound (the measured delay matches
  `L/c`), reflects correctly at an open end, and the engine still runs and holds dyno numbers with
  pipes attached.
- *Built:* `pipe.c`, a staggered-grid 1D pipe: cells (chambers) joined by faces whose mass flow is a
  state, with momentum, Darcy friction, kinetic energy paid for by the cells, Colburn's walls, open
  and closed ends with the end correction, restrictions, a laminar loss, and two rates (the cells at
  the valves every physics step, the rest at 48 kHz, the fast faces seeing the slow cells' pressure
  as it is by now). Every runner, header, collector's exhaust pipe (through a catalyst with a
  laminar loss into its muffler) and tailpipe is one; the V8s have a twin exhaust, the six two
  collectors of three, the boxer unequal headers. The API gives each pipe and every cell's state, and
  `rb_cli pipes` and `tools/plot_pipes.py` show them. The pulse runs at the speed of sound (0.16 %
  slow) and comes back from an open end inverted, 15 mm beyond it (the end correction: 12 mm). The
  four, the six and the boxer stay within 6 % of their real engines. The tailpipe no longer draws
  fresh air in at the lightest loads, as stage 6's lumped one did (the air-fuel ratio reads 14.6
  there, not 13.6). The runners made the idle unstable on its own (less exhaust pushed back into
  them the faster the engine turns), so the engine computer's idle valve and spark now act on the
  speed half a second ahead, the spark three times as strongly, and every preset's idle was
  retuned. The pipes cost 2.3 times stage 9 per step for the four, 2.7 for the cross-plane V8.
  Details in `docs/stage-10.md`.

#### Stage 11 — Audio extraction → first WAV renders 🎧
- Microphone model: tailpipe outflow → radiated pressure (∝ d(mass flow)/dt), distance attenuation.
- Decimation from sim rate to 48 kHz with an anti-alias filter, DC blocker and soft limiter.
- TPDF dither on 16-bit WAV export (stage 1 measured quantisation spurs at -97 dB on a pure tone).
- `make render` renders: I4 idle, an I4 rev sweep, and cross-plane vs flat-plane V8 at 3000 rpm.
- **Done when:** the WAVs are clean (no clicks, no DC, no aliasing whine), with spectrograms in `docs/`.
- **Bernardo checks:** listen. Does the V8 burble? Do the I4 and V8 sound obviously different?
- *Built:* `audio.c`: a microphone 1 m from the tailpipes hears what a monopole radiates,
  `ρ0/(4πr) dQ/dt`, from their volume flow (the plan said mass flow: the hot gas is twice the volume
  per kilogram). A Kaiser-window FIR takes it from the physics rate to the audio rate (flat to
  20 kHz, 100 dB down from 28 kHz), then a 10 Hz DC blocker and a soft limiter (a knee at 0.8, never
  past full scale, which is 40 Pa); 16-bit WAVs get TPDF dither. `rb_render_block` runs the chain on
  the tone or the engine (the browser keeps the tone until stage 14); `rb_cli sound` and
  `make render` render the four idling and pulling and the two V8s at 3000 rpm, and
  `tools/plot_sound.py` draws them. They're clean: no clicks, DC or whine. One click was in the
  physics: when a tailpipe's flow turned round, its volume was taken at the outside air's density,
  a kink at every firing; now it's the end cell's. At a microphone between its tailpipes the
  cross-plane V8's half orders cancel (its banks fire the same pattern 360° apart), leaving a lope
  at order 1. The single-volume muffler booms at 1500–2500 rpm and keeps a pull from getting louder
  above it. Details in `docs/stage-11.md`.

#### Stage 12 — Mufflers, intake & listener 🎧
- Muffler/resonator (expansion chamber + perforated-pipe approximation, or a convolution
  with a synthesized muffler impulse response) and a "straight pipe" option.
- Intake mouth sound.
- A listener with a position (behind the car, beside it, in the engine bay).
- From stage 11: the muffler is one volume, which booms near its ring (1500–2500 rpm in the four),
  filters the firing orders by 12 dB an octave above it (a pull doesn't get louder), and on the
  overrun leaves a sharp exhaust resonance (643 Hz in the four). Each tailpipe needs its own place
  and delay: at a microphone between them the cross-plane V8's half orders cancel. The intake
  mouth's flow changes every physics step, unlike the tailpipes', so the anti-alias filter will
  matter there.
- Split (working agreement 2): the mechanical and flow noise and the in-car listener moved to
  stage 12b.
- **Done when:** the new renders sound noticeably more like a real car than in stage 11, and the
  equal- vs unequal-length header renders sound different.
- **Bernardo checks:** listen and give feedback; the sound tuning knobs get adjusted from that.
- *Built:* each exhaust system's muffler is a pipe: a chambered can (an expansion chamber, left
  through baffles with a laminar loss), a straight-through (the tailpipe's bore, packed: a laminar
  loss on the flow's swings only, which takes nothing from the steady flow), or a straight pipe.
  Pipes can go straight on into each other (joined through a volume, a straight pipe lost two
  velocity heads and cost more back pressure than the can). The air comes in through a snorkel,
  the filter (a laminar loss) and an air box; the engine's books count at the snorkel's mouth, whose
  flow is a source too (the throttle's flow changes every physics step, the snorkel's mouth only once
  a pipe step, like the tailpipes'). A listener stands 3 m behind the car (the default), a metre
  behind a tailpipe, beside the car or over the bonnet, and hears each tailpipe and the mouth at its
  own distance and delay, and off the ground. A pull now gets louder: a metre from the four's
  tailpipe, 85–91 dBA below 1600 rpm and 99–109 above 5500 (stage 11's: 84–93 all the way). The
  boxer's unequal headers put every half order in its exhaust; equal ones leave only the firing's.
  Away from the middle of its twin exhaust, the cross-plane V8's half orders come through. The boom
  at 1500–2500 rpm is the whole exhaust's lowest ring (about 63 Hz), not the muffler's. The four,
  six and boxer lose about 3 % of their torque and power to the filter and mufflers, and stay
  within 9 % of their real engines. Details in `docs/stage-12.md`.

#### Stage 12b — folded into the sound-realism track
Its flow noise is now stage S2, its mechanical noise S3, and its in-car listener and reverb S4
(after stage 13).

#### Stage 13 — Performance & stability pass
- Profile the native build. Optimize memory layout and pipe cell counts, and add a quality setting (sim rate).
- Fuzz test: random parameter sets for 10 s each must produce no NaN/inf and no runaway.
- Measure the cost of the nozzle's `pow` calls and the Newton step for T in WebAssembly; cache
  gamma-dependent constants or tabulate them if they show up in the profile.
- Baselines (Node 22, i7-1355U): one cylinder's volume step costs about 150 ns natively and 96 ns
  in WASM (stage 4); one step of the whole breathing four, 1.0 µs natively and 1.4 µs in WASM
  (stage 5); the fired four with wall heat and the exhaust volumes, 1.8 µs natively and 2.4 µs in
  WASM (stage 6). That's 2.9× real time natively at 192 kHz, and 47 % of a core in the browser,
  before pipes. The free crank (stage 7) adds 5–15 %. The target below likely needs the default
  physics rate dropped to 96 kHz (VE changes by 0.03–0.1 % between the two, IMEP by under 0.1 %,
  and the free engine's idle not at all) as well as the optimizations listed in
  `docs/stage-05.md` and `docs/stage-06.md`. The eights (stage 8) cost 2.0 µs per step natively,
  1.8 times the four: about 40 % of a core at 192 kHz before pipes. Stage 9's lumped intake runners
  add 30–38 %: the four costs 1.42 µs per step and the cross-plane V8 2.64 µs, mostly in the port
  chambers' gas updates.
- Stage 10's pipes (96–212 cells, at 48 kHz but for the cells at the valves) multiply that by 2.3
  for the four and 2.7 for the cross-plane V8 (callgrind; on a cloud VM 4.1 and 9.2 µs per step, so
  only the fours keep up with real time there). The biggest items: rebuilding cells' gas mixtures
  (15 %), `u(T)` and `T(u)` (13 %), the pipes' faces and cells (28 %). Dropping to 96 kHz needs the
  valves' cells reworked first: a 5 cm cell at a wide-open valve takes two 96 kHz steps to equalise,
  so the flow cap gives 10 % more pumping work there (`docs/stage-10.md`, "At 96 kHz"). The engine
  struct is 317 KB, nearly all the pool of pipe cells.
- Benchmark with repeated runs: on this laptop (hybrid performance and efficiency cores) the same
  step measured 1.8 µs in one session and 1.1 µs in another.
- **Sound first.** At the default quality the renders must not change, or only inaudibly; check
  with `tools/plot_sound.py` and `tools/compare_real.py`.
  - **The quality setting covers the exhaust's cell size too,** not only the physics rate. The
    pipes carry nothing above c/(π Δx): 2.4 kHz in the 7.5 cm exhaust cells, the cliff in
    RESEARCH.md's "Sound realism".
  - **The headroom found here goes to the sound:** finer cells where the sound is made (stage S2),
    then the other sound stages.
- **Done when:** the heaviest preset (V8 + full pipes) runs ≥ 4× real-time natively at the default
  quality, so it has headroom for WASM in the browser.

### Phase 2b — Sound realism (top priority) 🎧

Stage 12's renders sounded "dull and really artificial" to Bernardo, and the recordings of real
engines show why (RESEARCH.md, "Sound realism"):
- **Too regular:** each cycle of ours repeats the last to +44 dB; a real idle repeats at about +5.
- **Too dull:** nothing above 2 kHz, the pipes' grid; real engines have broadband energy up to
  16 kHz.
- **Too much missing:** no mechanical sound, and no place.

Each stage adds a layer driven by what the sim already computes, not samples. Each one ends with
the numbers from `tools/compare_real.py` in the real engines' range, and with Bernardo's ears.

#### Stage S1 — Life: cycle-to-cycle and cylinder-to-cylinder variation 🎧
- **Each cycle's burn varies,** as the flame's early growth does with the turbulence and mixture
  at the plug.
  - A random ignition delay and burn length per cycle and cylinder, from a seeded generator in
    the core, so renders and tests stay reproducible.
  - Larger for slow burns (idle, part throttle, residual gas, lean or rich), smallest near the
    best spark.
  - Aim at the COV of IMEP real engines show: a few percent at load and more at idle. Driveability
    suffers above about 10 %.
- **Small fixed differences between cylinders** (fuel, air, spark), so even-firing engines get
  their small half orders.
- **Turbulence in the flow** at the valves' and throttle's jets, which the pipes then carry and
  filter.
- **The engine computer must still hold the idle,** so retune its gains if needed.
- **Done when:**
  - at idle the cycle repeats between 0 and +15 dB below 500 Hz (stage 12: +41 to +57);
  - the COV of IMEP reads a few percent at load and more at idle;
  - the dyno numbers move under 1 %;
  - every preset idles steadily for 30 s.
- **Bernardo checks:** the held and idling renders no longer sound like a loop.

#### Stage S2 — Air: flow noise and a finer exhaust 🎧
- **Broadband noise where the gas is fast:**
  - the tailpipe's jet (its power grows with about U⁸);
  - the throttle's hiss, following its pressure drop;
  - the intake's roar;
  - the muffler's baffles (1–8 kHz under hard acceleration).

  Each is driven by the velocities, densities and sizes the sim computes, with a spectrum scaled by
  its Strouhal number. So it bursts with each pulse (the rasp) and drops on the overrun.
- **The exhaust's highs:**
  - finer cells where the sound is made, through stage 13's quality setting (the cutoff c/(π Δx)
    is 2.4 kHz with today's 7.5 cm cells);
  - the tailpipe beaming its highs along its axis above ka ≈ 1.
- **Done when:**
  - at idle, the energy above 2 kHz is in the real engines' range (−45…−5 dB; stage 12: −57…−67);
  - above 500 Hz, the cycle repeats under 0 dB;
  - revving, the third-octave slope from 0.5 to 8 kHz is between −2 and −8 dB per octave
    (stage 12's pulls: −12 to −22);
  - on the overrun, the highs drop.
- **Bernardo checks:** does it still sound dull?

#### Stage S3 — The engine's own sound 🎧
- **The sources:**
  - combustion noise through the block: each cylinder's pressure rise through the structure's
    attenuation curve;
  - piston slap: the side thrust changes sign near TDC, from the gas force and the rod's angle;
  - the valves seating, the injectors ticking, the cam drive's whine.
- **How:** physically timed impulses into a few block and head resonances (modal synthesis),
  radiated from the engine bay. Strongest in front of the car, and heard inside it.
- **Done when:**
  - in front of the car, the energy above 1 kHz is well above the exhaust's (the recorded Honda
    V6: +13 dB);
  - the clatter follows the load, and the ticking follows the rpm.
- **Bernardo checks:** in front of the car, the idle sounds like an engine bay.

#### Stage S4 — Scenery: stereo and the place 🎧
- **A stereo listener:** two ears, or a spaced pair of microphones, each hearing every source at
  its own distance and delay, with the head's shadow. WAVs and the browser in stereo.
- **Places:**
  - an open lot (asphalt or grass);
  - beside a wall;
  - a garage or dyno cell, its reverb a feedback delay network in allocation-free C, so native
    renders and the browser sound the same;
  - ISO 5130's exhaust test spot (0.5 m, 45°);
  - a pass-by at 7.5 m;
  - inside the car: the body's paths, the engine's orders through the mounts, the cabin's boom
    (stage 12b's in-car listener).
- **Done when:** each place renders in stereo and sounds like itself, and the in-car render sounds
  like it's from inside.
- **Bernardo checks:** listen on headphones.

#### Stage 13b — Performance II: vectorized pipes, implicit valves
Split from stage 13 (working agreement 2), whose cheap wins and quality setting brought the
cross-plane V8 from 0.48× to about 0.8× real time natively at high quality (docs/stage-13.md). The
pipes are now most of the cost, about 70 ns per cell and step, and no single function dominates:
it's the per-cell work itself.
- **Structure of arrays for the pipes' cells.** Their state goes in arrays the loops run along, with
  no function calls and branch-free upwinding, so the compiler vectorizes them. Then build the
  WebAssembly core with SIMD (`-msimd128`, in every current browser) and link-time optimization.
- **The gas mixture without rebuilding it.** The rebuild is 18 % of the time, and the remix
  threshold can't be raised: it would make T and p jump by up to 1 Pa. Instead, evaluate the
  three species' u(T) and cv(T) weighted by mass (more arithmetic, all of it vectorizable), or
  vectorize the rebuild.
- **The valve's flow solved together with its port cell** (implicit), so a 96 kHz physics rate
  keeps 192 kHz's accuracy. Today's low quality has stage 10's error: 10 % more pumping work
  motored at 1500 rpm wide open.
- **After the sound stages S1–S4,** which change the hot code (S2's finer exhaust cells above all).
- **Done when:** the heaviest preset (V8 + full pipes) runs ≥ 4× real time natively and ≥ 2× as
  WebAssembly at the default quality, with the renders unchanged.

### Phase 3 — Browser

#### Stage 14 — Real-time engine in the browser 🎮
- Swap stage 2's test tone for the engine (the WASM build, worklet and messaging already exist).
- Messages: ignition, starter, throttle (keyboard + slider), preset load. Telemetry: rpm, torque,
  manifold pressure, AFR, and a CPU-load meter (check which timing APIs each browser's
  AudioWorkletGlobalScope offers; `performance.now()` may be missing).
- The engine runs with its crank free (`hold_speed = 0`). Ignition, starter and load map to the
  engine's own fields; telemetry can also show the idle valve, spark cut and fuel cut.
- Detect underruns and auto-drop quality if the CPU can't keep up.
- **Done when:** in Chrome and Firefox you can start the engine, blip the throttle and hear it
  with no dropouts on Bernardo's machine.

#### Stage 15 — Control panel
- Tweakpane panel: preset picker, bore/stroke (→ displacement shown), compression ratio,
  cam duration/lift/centerline, header length/diameter, muffler type, spark advance, AFR,
  rev limit, volume, quality.
- Live vs rebuild parameters with a crossfade, input ranges kept sane, and units toggle (metric/imperial).
  The API gives every field's range (`rb_engine_field_min/max`, stage 9) and clamps values outside
  it. The spark is the engine computer's since stage 9 (`spark_map`): offer an offset from it
  rather than a fixed advance. The intake runners (`runner_length`, `runner_diameter`) are fields too.
- **Done when:** every control audibly or measurably does something and nothing can crash the sim.

#### Stage 16 — Gauges + live dyno chart
- Tachometer, manifold pressure/boost, AFR, torque and HP readouts. Level meters get proper ballistics
  (stage 2's meters show a single 128-frame block, so the RMS wobbles about 1 dB).
- Dyno sweep button → torque/power curve drawn live in uPlot. Overlay the previous run to compare
  builds ("before vs after" is the whole point of a tester). Stage 9's dyno (`core/src/dyno.c`)
  isn't in the public API yet: expose it through `rb_sim`, so the browser sweeps with the same code.
- A load at idle: with stage 9's idle gains the fours take about 5 s to recover from a sudden
  10 N m. A derivative term on the idle valve or a feed-forward for known loads would help.
- **Done when:** a sweep in the browser matches the native dyno curve from stage 9 within a few %.

### Phase 4 — 3D engine view

The 3D view is generated **procedurally** from the engine description the core exports (stage 8):
change the bore, the cylinder count or the layout and the model rebuilds itself, so every engine the
simulator can run can also be shown. Its motion comes from the simulation itself, so the picture
can't disagree with the sound. It replaces the 2D cutaway that was planned here before.

#### Stage 17 — 3D engine model: block + moving parts
- Three.js scene: `WebGPURenderer` (falls back to WebGL 2 by itself), orbit camera, lighting and
  metal materials. Any custom shaders are written in TSL so they run on both backends.
- Procedural bottom end built from the engine description: block with cylinder bores, pistons, wrist
  pins, connecting rods, crankshaft with pins and counterweights, flywheel. Inline, V and boxer layouts.
- A crank-angle slider turns the engine by hand (it checks the kinematics before anything is
  connected to the sim), an x-ray/cutaway toggle for the block, and an automatic rebuild when a
  "rebuild" parameter changes.
- **Done when:** every preset is recognizable (screenshots of each in `docs/`), the parts move
  correctly when the crank is turned by hand, and changing bore/stroke visibly resizes them.
- **Bernardo checks:** the look (realistic metal or a technical x-ray style), decided before stage 18.

#### Stage 18 — 3D engine in motion, in sync with the sound
- Top end: heads, camshafts, valves and springs, spark plugs, cam drive.
- The worklet streams a compact visual snapshot (crank angle, rpm, per-cylinder pressure, burn
  fraction and valve lifts) at display rate. The main thread interpolates between snapshots and
  delays the picture by the audio output latency, so what you see lines up with what you hear.
- Slow-motion "time warp": the core runs slower than real time so you can watch the four strokes,
  with a stroke label on each cylinder.
- **Done when:** at 6000 rpm the visible crank angle stays within a few degrees of the sound, and
  slow motion shows intake → compression → power → exhaust in the right firing order for every preset.

#### Stage 19 — 3D gas, combustion & exhaust flow
- Intake manifold, headers, collector and tailpipe built from the pipe geometry (stage 10).
- Cylinder gas colored by pressure/temperature, spark and flame front from the burn fraction, fresh
  charge vs exhaust gas through the ports, exhaust pulses traveling down the headers (from the pipe
  model's cells), and headers glowing with exhaust temperature.
- Performance budget (60 fps on a laptop with the sound running), a 3D quality setting, and a
  fallback to gauges only when 3D isn't available.
- **Done when:** you can watch each cylinder's exhaust pulse leave in firing order, and the page
  holds 60 fps with sound on Bernardo's machine.

### Phase 5 — Forced induction & realism

#### Stage 20 — Turbocharger physics
- Compressor (simplified map: pressure ratio vs corrected speed and flow, with surge/choke limits),
  turbine driven by exhaust flow (A/R-like size parameter), shaft inertia → **spool lag**,
  wastegate with a boost target, intercooler, blow-off valve.
- A "turbo size" parameter that scales the maps (small = fast spool / low top-end, big = laggy / big power).
- **Done when:** native dyno runs show boost building with RPM, a clear lag on throttle steps, and
  a bigger turbo moving the torque curve up the rev range.

#### Stage 21 — Turbo sound, turbo UI + 3D turbo 🎧
- Turbine/compressor whine from shaft speed (blade-pass tone + broadband), BOV "pssh" on throttle
  lift, wastegate chatter, and the muffling effect of the turbine on exhaust pulses.
- UI: turbo on/off, size, boost target, BOV on/off, boost gauge.
- 3D: the turbo on the exhaust manifold, with compressor and turbine wheels spinning at the simulated
  shaft speed (blurred when fast), and the wastegate and blow-off valve moving.
- **Done when:** spool-up is clearly audible and matches both the boost gauge and the 3D turbo.

#### Stage 22 — Knock, mechanical limits & engine damage
- Knock model (end-gas pressure/temperature vs fuel octane, Livengood-Wu style) → audible knock + power
  loss; piston speed, valve float and over-rev limits; an optional "engine blows" state with a
  warning UI and reset. The end gas is the unburned share of stage 6's Wiebe burn. Stage 6 lets
  50° of advance push the peak pressure to 82 bar; a real engine would knock long before that.
  Since stage 9 the engine computer sets the spark for the best torque everywhere (18–37° on the
  four); knock should hold it back at full load and low speed, as real engines' maps do.
- 3D effects: knock flashes inside the cylinder, glowing parts, smoke, and a thrown rod when the
  engine blows.
- **Done when:** the silly combos (CR 16 + 3 bar boost on pump gas, 12k rpm on a long-stroke I6) are
  caught and shown instead of producing fantasy horsepower.

### Phase 6 — Polish & ship

#### Stage 23 — More layouts & presets
- I3, I5, 60° V6, 90° V6, V10, V12, boxer-6, single-cylinder motorcycle, plus a couple of iconic
  "inspired by" builds (described generically, not by brand).
- **Done when:** each one starts, sounds distinct, renders correctly in 3D and lands within sanity
  ranges on the dyno.

#### Stage 24 — UX polish, save/share, quality settings
- Layout and theming, keyboard shortcuts, first-time onboarding, builds saved locally and shareable
  via URL, mobile-friendly controls, an auto-quality fallback for slow devices, and 3D camera presets
  (front, side, cutaway, follow the exhaust).
- **Done when:** a fresh visitor can figure out what to do in under 30 seconds.

#### Stage S5 — Tuning by ear 🎧 (once everything is built)
- **Bernardo tunes each preset in the browser,** with live controls for each layer of the sound.
  He compares it with recordings of the real engine it's modelled on: at the exhaust, in front,
  inside and passing by.
- **`tools/compare_real.py` keeps the numbers honest.** The tuned values become the defaults.
- **Done when:** Bernardo is happy with every preset's sound.

#### Stage 25 — Deploy
- Vercel project (static build of `web/`), caching headers for `.wasm`, README with screenshots
  and a short "how it works" (written from the real equations used, not fluff), and a link from the portfolio.
- **Done when:** it's live at a public URL and works on a second device.

#### Stage 26 — Optional extras (one per prompt)
- Gearbox + vehicle mode (drive away, shift), WAV recording/export, 2-stroke engines, fuels (E85,
  methanol), supercharger, ITBs, anti-lag, a 3D garage/dyno-room scene, video capture of the 3D view.

---

## Working agreement (for every stage)

1. Read `PLAN.md` and the code from previous stages before starting.
2. Keep the stage small. If it grows, split it and add a row to the tracker instead of rushing.
3. All tests must pass before moving on: `make test`, and `npm test` in `web/`. After any change in
   `core/`, run `make wasm` (the web tests fail on a stale core). Stages that touch the browser extend
   `web/tests/e2e` and must pass in headless Chromium, on the dev server and the production build.
4. Write the equations used in `docs/stage-NN.md` with sources, so the physics stays explainable.
5. Finish by updating the tracker row and listing anything Bernardo should check.
6. **Sound check** (the sound is the top priority):
   - **Re-render and compare.** Every stage that changes `core/` runs `make render` and compares
     the sound with the last stage's, using `tools/plot_sound.py` and `tools/compare_real.py`.
   - **Say what changed.** The stage doc says what changed in the sound. A stage that isn't meant
     to change it shows that it didn't.
   - **Never trade sound for speed silently.** A lower default quality is checked by ear and by
     spectrum, not only by the dyno numbers.
