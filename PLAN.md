# REV-BENCH — Plan of Action

A browser-based internal combustion engine test bench. You pick an engine, change things like
displacement, compression, cams, headers and turbo size, and **hear and see** the result in real
time. Sound comes from a **physics simulation** (the realistic approach), not from samples or simple
synths.

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
| 7  | Crankshaft dynamics, multi-cylinder, self-running| Physics core     | ⬜     | |
| 8  | Engine layouts & presets                         | Physics core     | ⬜     | |
| 9  | Dyno + validation against real engines           | Physics core     | ⬜     | |
| 10 | 1D exhaust pipes (headers → collector → tailpipe)| Sound            | ⬜     | |
| 11 | Audio extraction → first WAV renders 🎧          | Sound            | ⬜     | Listening checkpoint |
| 12 | Muffler, intake & mechanical sound, mix 🎧        | Sound            | ⬜     | Listening checkpoint |
| 13 | Performance & stability pass                     | Sound            | ⬜     | |
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

#### Stage 8 — Engine layouts & presets
- Layout description: inline / V (bank angle) / boxer, crank pin angles, firing order and
  per-cylinder offsets, including **cross-plane vs flat-plane V8**.
- Presets in C (`presets.c`) with exported getters/setters so the web UI can list and edit them.
  First set: 2.0 I4, 3.0 I6, 5.0 cross-plane V8, 4.5 flat-plane V8, 2.0 boxer-4.
- Export the engine description through the API (cylinder positions and bank angles, crank pin
  angles, bore, stroke, rod length, valve sizes, cam timing) plus per-cylinder state (pressure,
  temperature, burn fraction, valve lifts). The control panel and the 3D view (stages 17–19) build
  themselves from it, so no engine geometry is duplicated in JavaScript.
- **Done when:** every preset starts, idles and revs natively, and firing-event timing plots
  show correct spacing (for example, an even 90° spacing for a V8).

#### Stage 9 — Dyno + validation
- Absorbing dyno: hold RPM with a PI controller, cycle-averaged torque and power, and a sweep mode
  (e.g. 1500→redline) → CSV torque/power curve + plot.
- Compare 2–3 presets against published real-engine numbers and tune the model's free factors
  (discharge coefficients, friction, burn duration).
- Stage 6's knobs: burn duration and Wiebe shape (60° at every speed and load for now, so MBT
  only moves between 20° and 25°; real engines want more advance at high rpm and light load),
  mixture spread, combustion efficiency and wall temperature. Add the fuel's evaporation cooling
  the charge. Low-speed VE is 0.87, against 0.80–0.85 in real engines.
- **Done when:** peak torque and power are within about ±15% of the reference engines, at sensible RPMs,
  and changes behave in the right direction (more CR → more torque until knock; bigger cams → torque
  shifts up the rev range).

### Phase 2 — Sound (the realistic part)

#### Stage 10 — 1D exhaust pipes
- 1D compressible pipe model (finite-volume with a stable flux scheme, or a lossy waveguide if
  performance demands it). Open-end and closed-end boundaries, area changes, friction/damping.
- Per-cylinder primaries → collector(s) → tailpipe, with configurable lengths and diameters.
- Expose the pipe geometry (lengths, diameters, how pipes connect) and per-cell pressure, for the 3D
  exhaust view (stage 19).
- Use the same pipe model for the intake runners. Stage 5's quasi-steady valves give VE no
  mid-range hump: that comes from the inertia of the air in tuned runners ("ram" effect).
- The pipes replace stage 6's lumped exhaust manifold (2 L) and muffler (10 L). Check the
  air-fuel ratio at the lightest loads, where the lumped tailpipe still lets some air in (13.6
  instead of 14.6 at 2000 rpm and 3 % throttle).
- Mind the budget: the fired four already costs 1.8 µs per step natively (2.4 µs in WASM, 47 % of
  a core) at 192 kHz. Keep pipes cheap (few cells, a cheap flux scheme, or a lower update rate than
  the cylinders) and measure their cost from the start.
- **Done when:** a pressure pulse travels at the right speed of sound (the measured delay matches
  `L/c`), reflects correctly at an open end, and the engine still runs and holds dyno numbers with
  pipes attached.

#### Stage 11 — Audio extraction → first WAV renders 🎧
- Microphone model: tailpipe outflow → radiated pressure (∝ d(mass flow)/dt), distance attenuation.
- Decimation from sim rate to 48 kHz with an anti-alias filter, DC blocker and soft limiter.
- TPDF dither on 16-bit WAV export (stage 1 measured quantisation spurs at -97 dB on a pure tone).
- `make render` renders: I4 idle, an I4 rev sweep, and cross-plane vs flat-plane V8 at 3000 rpm.
- **Done when:** the WAVs are clean (no clicks, no DC, no aliasing whine), with spectrograms in `docs/`.
- **Bernardo checks:** listen. Does the V8 burble? Do the I4 and V8 sound obviously different?

#### Stage 12 — Muffler, intake & mechanical sound, mix 🎧
- Muffler/resonator (expansion chamber + perforated-pipe approximation, or a convolution
  with a synthesized muffler impulse response) and a "straight pipe" option.
- Intake mouth sound, valvetrain/mechanical noise tied to crank angle, flow turbulence noise.
- A mixer with listener position (behind the car, engine bay, in-car) and an optional room/outdoor
  reverb (done later with a Web Audio `ConvolverNode` in the browser).
- **Done when:** the new renders sound noticeably more like a real car than in stage 11, and the
  equal- vs unequal-length header renders sound different.
- **Bernardo checks:** listen and give feedback; the sound tuning knobs get adjusted from that.

#### Stage 13 — Performance & stability pass
- Profile the native build. Optimize memory layout and pipe cell counts, and add a quality setting (sim rate).
- Fuzz test: random parameter sets for 10 s each must produce no NaN/inf and no runaway.
- Measure the cost of the nozzle's `pow` calls and the Newton step for T in WebAssembly; cache
  gamma-dependent constants or tabulate them if they show up in the profile.
- Baselines (Node 22, i7-1355U): one cylinder's volume step costs about 150 ns natively and 96 ns
  in WASM (stage 4); one step of the whole breathing four, 1.0 µs natively and 1.4 µs in WASM
  (stage 5); the fired four with wall heat and the exhaust volumes, 1.8 µs natively and 2.4 µs in
  WASM (stage 6). That's 2.9× real time natively at 192 kHz, and 47 % of a core in the browser,
  before pipes. The target below likely needs the default physics rate dropped to 96 kHz (VE
  changes by 0.03–0.1 % between the two, IMEP by under 0.1 %) as well as the optimizations
  listed in `docs/stage-05.md` and `docs/stage-06.md`.
- **Done when:** the heaviest preset (V8 + full pipes) runs ≥ 4× real-time natively at the default
  quality, so it has headroom for WASM in the browser.

### Phase 3 — Browser

#### Stage 14 — Real-time engine in the browser 🎮
- Swap stage 2's test tone for the engine (the WASM build, worklet and messaging already exist).
- Messages: ignition, starter, throttle (keyboard + slider), preset load. Telemetry: rpm, torque,
  manifold pressure, AFR, and a CPU-load meter (check which timing APIs each browser's
  AudioWorkletGlobalScope offers; `performance.now()` may be missing).
- Detect underruns and auto-drop quality if the CPU can't keep up.
- **Done when:** in Chrome and Firefox you can start the engine, blip the throttle and hear it
  with no dropouts on Bernardo's machine.

#### Stage 15 — Control panel
- Tweakpane panel: preset picker, bore/stroke (→ displacement shown), compression ratio,
  cam duration/lift/centerline, header length/diameter, muffler type, spark advance, AFR,
  rev limit, volume, quality.
- Live vs rebuild parameters with a crossfade, input ranges kept sane, and units toggle (metric/imperial).
- **Done when:** every control audibly or measurably does something and nothing can crash the sim.

#### Stage 16 — Gauges + live dyno chart
- Tachometer, manifold pressure/boost, AFR, torque and HP readouts. Level meters get proper ballistics
  (stage 2's meters show a single 128-frame block, so the RMS wobbles about 1 dB).
- Dyno sweep button → torque/power curve drawn live in uPlot. Overlay the previous run to compare
  builds ("before vs after" is the whole point of a tester).
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
