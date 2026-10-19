# REV-BENCH

A browser-based combustion engine test bench: pick an engine, change displacement,
compression, cams, headers and turbo size, and hear and see the difference in real time,
with a 3D view of the engine running. The sound is generated from a physics simulation of
the gas moving through the engine, not from recordings.

**Status:** stage 11 of 26 done. The C core plays in the browser through an AudioWorklet
(still a test tone); natively it renders the engine's own sound to WAV files, as a microphone
behind the tailpipe hears it. The physics so far: gas properties, chambers and flow through openings,
a cylinder with a crank and piston, an engine breathing through its cams, valves, throttle
and intake plenum, burning fuel with heat lost to the walls, and a crank that turns on its
own: friction, a starter motor, and an engine computer that idles it, cuts the fuel when
coasting and stops it at the rev limit. Its intake runners, exhaust headers, exhaust pipes
and tailpipes are 1D pipes, where pressure waves run at the speed of sound and tune the
engine's breathing. Engines can be inline, V or boxer with any crank and firing order, and
five come ready-made: a 2.0 L four, a 3.0 L six, a cross-plane and a flat-plane V8 (each
bank its own exhaust), and a 2.0 L flat four with unequal headers. On the dyno, the four, the
six and the flat four make close to the torque and power of the real engines they stand for
(Toyota's 1AZ-FE, BMW's M54B30 and Subaru's FB20). See [PLAN.md](PLAN.md) for the roadmap and progress,
[RESEARCH.md](RESEARCH.md) for background, and `docs/` for notes and equations per stage.

## Try it in the browser

You need Node.js 22.12 or newer (`node --version`; in WSL, `nvm install 22` is an easy
way to get it).

```bash
cd /mnt/c/Users/berna/Personal-Projects/REV-BENCH/web
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
cd /mnt/c/Users/berna/Personal-Projects/REV-BENCH
make            # builds build/rb_cli
make test       # runs the C unit tests
make render     # the engine's sound (about a minute): renders/i4-idle.wav, i4-sweep.wav,
                # v8-cross-3000.wav and v8-flat-3000.wav, and the test tone (test.wav)
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
                           # --preset v8-cross, --sweep, --hold --rpm 3000 --throttle 0.3, --help
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
python3 tools/plot_sound.py spectrograms renders/i4-idle.wav renders/v8-cross-3000.wav
python3 tools/plot_sound.py sweep renders/i4-sweep.wav renders/i4-sweep.csv
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
docs/      One note per stage: what was built and the equations used.
renders/   Output of make render, the rb_cli demos and tools/ (git-ignored).
tools/     Python scripts that chart the CSV output.
```

Stack: C11 physics core → WebAssembly in an AudioWorklet · Vite + TypeScript UI ·
static hosting (Vercel).
