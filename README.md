# REV-BENCH

A browser-based combustion engine test bench: pick an engine, change displacement,
compression, cams, headers and turbo size, and hear and see the difference in real time,
with a 3D view of the engine running. The sound is generated from a physics simulation of
the gas moving through the engine, not from recordings.

**Status:** stages 0–13 of 26 done, and 12b and 12c. The C core plays in the browser through an
AudioWorklet (still a test tone); natively it renders the engine's own sound to WAV files, as a
listener around the car hears it: behind it, beside it, over the bonnet, a metre from a tailpipe
or in the driver's seat, each tailpipe and the intake's mouth at their own distance and off the
ground. On top of the pipes' sound come the noise of the gas's jets (at the tailpipes, the
exhaust valves and the throttle) and the engine's own mechanical sound (combustion, piston slap,
the valves, the injectors), struck into the block's resonances. No two cycles are alike: each
burn starts and runs its own way, the cylinders differ a little and their headers aren't all
alike, as a real engine's; the rev limiter stutters on a soft cut, and let go the engine falls
back to idle. The physics so
far: gas properties, chambers and flow through openings,
a cylinder with a crank and piston, an engine breathing through its cams, valves, throttle
and intake plenum, burning fuel with heat lost to the walls, and a crank that turns on its
own: friction, a starter motor, and an engine computer that idles it, cuts the fuel when
coasting and stops it at the rev limit. Its intake snorkel and runners, exhaust headers,
exhaust pipes, mufflers and tailpipes are 1D pipes, where pressure waves run at the speed of
sound and tune the engine's breathing and its sound. A muffler can be a chambered can, a
straight-through packed with wool, or a straight pipe. Engines can be inline, V or boxer
with any crank and firing order, and five come ready-made: a 2.0 L four, a 3.0 L six, a
cross-plane and a flat-plane V8 (each bank its own exhaust), and a 2.0 L flat four with
unequal headers. On the dyno, the four, the
six and the flat four make close to the torque and power of the real engines they stand for
(Toyota's 1AZ-FE, BMW's M54B30 and Subaru's FB20). A quality setting trades a little accuracy
for speed (the heaviest engine, the cross-plane V8, runs at about three quarters of real time
natively at full quality and nearly twice it at low), and a fuzzer drives random engines to find
what breaks. See [PLAN.md](PLAN.md) for the roadmap and progress,
[RESEARCH.md](RESEARCH.md) for background, and `docs/` for notes and equations per stage.

## Try it in the browser

You need Node.js 22.12 or newer (`node --version`; in WSL, `nvm install 22` is an easy
way to get it).

```bash
cd /mnt/c/Users/berna/REV-BENCH/web
npm install
npm run dev
```

Open <http://localhost:5173> in your normal Windows browser (WSL forwards it), turn the
volume down, press **Start** and move the pitch slider.

Run `npm install` and `npm run dev` in the same environment (both in WSL, or both in
Windows): some build tools are native programs built for one system.

Other web commands (inside `web/`):

```bash
npm test             # unit tests of the WebAssembly core (Node)
npm run build        # production build in web/dist
npm run test:e2e     # browser test; the first time run: npx playwright install chromium
```

## Native tools (WSL / Linux)

You need `gcc` and `make` (`sudo apt install build-essential`).

```bash
cd /mnt/c/Users/berna/REV-BENCH
make            # builds build/rb_cli
make test       # runs the C unit tests
make render     # the engine's sound (about six minutes) in renders/: the four idling,
                # pulling and revving in neutral (i4-idle.wav, i4-sweep.wav, i4-rev.wav), with
                # each muffler (i4-3000-*.wav), the cross-plane V8 from each place around the
                # car (v8-cross-3000-*.wav), pulling and revving, the flat-plane V8, the six
                # and the boxer revving (*-rev.wav), the boxer with unequal and equal headers
                # (boxer4-3000*.wav), and the test tone
```

Open the WAVs in `renders\` from Windows Explorer to listen, on headphones or speakers with
some bass: the four's idle is mostly 27 Hz.

More:

```bash
build/rb_cli info          # list the core's parameters, telemetry, engine fields and presets
build/rb_cli presets       # the five engines and how they fire; add a key (e.g. v8-cross)
                           # for every field of one, as the browser sees it
build/rb_cli tone --help   # options for the test render
build/rb_cli blowdown      # physics demo: a tank emptying (renders/blowdown.csv)
build/rb_cli motor         # physics demo: one cylinder turned by a motor (renders/motor.csv)
build/rb_cli breathe       # physics demo: the engine breathing (renders/breathe.csv);
                           # try --throttle 0.1, --rpm 800, --intake 280/11/105, --help
build/rb_cli breathe --sweep   # its volumetric efficiency map, ~35 s (renders/ve.csv)
build/rb_cli fire          # physics demo: the engine running on fuel (renders/fire.csv), the
                           # spark set by its engine computer; try --spark 40 (a fixed spark),
                           # --afr 13, --throttle 0.06, --cr 12, --motored, --help
build/rb_cli fire --spark-sweep    # torque against spark advance (renders/spark.csv)
build/rb_cli fire --rpm-sweep      # the wide-open power curve, brake and indicated (renders/power.csv)
build/rb_cli dyno          # the engine on a dyno, swept from 1500 rpm to the rev limit (renders/dyno.csv);
                           # try --preset i6, --step 500, --throttle 0.3, --intake 280/10.5/110, --help
build/rb_cli pipes --pulse # a pressure pulse in a 2 m pipe (renders/pulse.csv): its speed, its echo
build/rb_cli pipes         # one cycle of the four's runner, header and exhaust, cell by cell
                           # (renders/pipes.csv); --preset, --rpm, --throttle
build/rb_cli run           # the engine on its own: started, idling, revved, rev limiter, off
                           # (renders/run.csv); try --load 10, --inertia 0.08, --help
build/rb_cli run --preset v8-flat   # breathe, fire and run take any preset (rb_cli presets)
build/rb_cli sound         # the engine's sound, started and idling (renders/sound.wav); try
                           # --preset v8-cross, --sweep, --hold --rpm 3000 --throttle 0.3,
                           # --listener front, --muffler none, --equal-headers, --help,
                           # --quality low (faster: the physics and pipe rates halved)
build/rb_cli fuzz          # random engines started and driven for 10 s each: nothing may stop
                           # being a number or run away (also: make fuzz)
make bench                 # how fast the core runs, preset by preset
make bench-wasm            # the same as WebAssembly, run by node (needs zig)
make SAN=1 test            # tests with AddressSanitizer + UBSan
make help                  # all make targets
```

To chart the demos you need matplotlib (`sudo apt install python3-matplotlib`):

```bash
python3 tools/plot_cycle.py                  # renders/motor.csv → renders/motor.png
python3 tools/plot_breathe.py cycle          # renders/breathe.csv → renders/breathe.png
python3 tools/plot_breathe.py map            # renders/ve.csv → renders/ve.png
python3 tools/plot_fire.py cycle             # renders/fire.csv (+ fire-motored.csv) → renders/fire.png
python3 tools/plot_fire.py spark             # renders/spark.csv → renders/spark.png
python3 tools/plot_fire.py power             # renders/power.csv → renders/power.png
python3 tools/plot_run.py                    # renders/run.csv → renders/run.png
python3 tools/plot_presets.py firing         # rb_cli presets --csv renders/presets.csv first
python3 tools/plot_dyno.py curves renders/dyno-i4.csv ...   # dyno sweeps against the real engines
python3 tools/plot_pipes.py pulse renders/pulse.csv          # the pulse, where it is when
python3 tools/plot_pipes.py engine renders/pipes.csv         # the waves in a runner and a header
python3 tools/plot_sound.py check renders/*.wav              # levels, DC, clicks, lines
python3 tools/plot_sound.py spectrograms renders/i4-idle.wav renders/v8-flat-3000.wav
python3 tools/plot_sound.py sweep renders/i4-sweep.wav renders/i4-sweep.csv
python3 tools/plot_sound.py orders renders/boxer4-3000.wav renders/boxer4-3000-equal.wav --rpm 3000
python3 tools/plot_sound.py levels renders/i4-sweep.wav renders/i4-sweep.csv   # dBA against rpm
python3 tools/compare_real.py fetch          # recordings of real engines → renders/real/ (needs ffmpeg)
build/rb_cli sound --preset i4 --idle --seconds 16 --listener tailpipe --out renders/i4-idle-tailpipe.wav
python3 tools/compare_real.py compare renders/i4-idle-tailpipe.wav@6-16:"i4 idle"   # against them
```

## Rebuilding the WebAssembly core

The compiled core (`web/src/wasm/rb_core.wasm`) is committed, so the web app works without
a C-to-WebAssembly compiler. After changing anything in `core/`, rebuild it:

```bash
make wasm
```

This needs zig: either a `zig` on your PATH, or the pip package
(`python3 -m pip install --user ziglang`; on newer Ubuntu put it in a venv and run
`make wasm ZIG="path/to/venv/bin/python -m ziglang"`). `npm test` fails if the C code
changed since the last `make wasm`, so a stale core can't sneak into the browser.

## Project layout

```
core/      C11 simulation core (physics + sound). Only depends on libm.
native/    Command-line harness, WAV and CSV writers.
tests/     C unit tests (make test).
web/       Browser app (Vite + TypeScript): src/core wraps the WASM core, src/audio runs it
           in an AudioWorklet, tests/ has unit and browser tests.
docs/      One note per stage: what was built and the equations used. workspace.md: where
           REV-BENCH is worked on and how each place is set up.
renders/   Output of make render, the rb_cli demos and tools/ (git-ignored).
tools/     Python scripts that chart the CSV output.
```

Stack: C11 physics core → WebAssembly in an AudioWorklet · Vite + TypeScript UI ·
static hosting (Vercel).
