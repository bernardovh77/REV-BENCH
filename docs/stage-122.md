# Stage 122 — Native builds with MSVC and gcc, and continuous integration

**Status:** done (2026-11-09): local suites on Windows and WSL, and CI green on Windows
and Linux; the first macOS run is still to come (Mondays, or on demand)

The core, the tools and the C tests build with each system's own compiler through CMake,
and a GitHub Actions pipeline is set to prove it on every push
([specification](roadmap/07-app.md#stage-122)). Windows first: MSVC, the desktop app's
compiler, is the target; gcc builds and tests on Linux; Apple clang runs the core's tests
weekly. No physics changed.

## The CMake build

`CMakeLists.txt` builds what the Makefile builds, with the same rules:
- **the core** (`rb_core`), its internals for the tools and tests, the native helpers and
  the research solvers; `rb_cli`, `rb_bench`, `rb_audit`, `build_contract_probe`; the C
  tests as one CTest test;
- **strict doubles everywhere:** `-ffp-contract=off` for gcc and clang; `/fp:precise` for
  MSVC, which never contracts into a fused multiply-add without `/fp:contract`. No
  fast-math. Release is `-O3 -g` or `/O2 /Zi`, NDEBUG not defined, as the Makefile;
  link-time optimisation where the compiler has it;
- **warnings as errors in the core** (`RB_WERROR`, on): `/W3` for MSVC, gcc's `-Wall -Wextra
  -Wpedantic -Wshadow -Wstrict-prototypes`. The tests and research code keep warnings as
  warnings (gcc's `-Wmaybe-uninitialized` at `-O3` flags some there);
- **`rb_cli sound`** compiles as its own object with only the public headers (stage 121).

The Makefile stays: gcc, w64devkit and the WebAssembly build.

**MSVC on Windows** needs Visual Studio Build Tools 2026 with the C++ workload (here MSVC
14.51), and CMake 4.4.4 and Ninja 1.13.2, now in `tools/requirements-dev.txt` (Apache 2.0;
the wheels carry the programs, so nothing is installed system-wide). `tools/msvc_env.py`
finds the Build Tools through `vswhere` and runs a command in `vcvars64.bat`'s environment,
so no developer prompt is needed (docs/workspace.md).

**What MSVC needed:** one change. `rb_bench.c` timed itself with POSIX's
`clock_gettime(CLOCK_MONOTONIC)`, which MSVC lacks; under MSVC it now uses Windows'
performance counter. The core itself compiled at `/W3` without a single warning.

**MSVC passes every C test:** 262 tests and 4,968,415 checks, the same count as gcc, in
392 s on Bernardo's machine (w64devkit's gcc, without LTO: 462 s with the build). The
first try built in Debug without anyone asking: with MSVC, CMake's `project()` makes Debug
the default, before a later default can apply. An `/Od /RTC1` build ran the suite at about
an eighth of the speed. The default is now set before `project()`, and the flags checked
(`/O2 /Zi /GL /fp:precise /W3 /WX`).

## Do MSVC and gcc sound the same?

Within one compiler a change must keep every render bit-identical (stage 119's rule;
`tools/cli_parity.py`). Across compilers it can't: libm's `exp` and `pow` differ in their
last bits, and the engine carries them on. `tools/compiler_parity.py` renders five seeded
drives with two builds' `rb_cli` and compares them.

Measured on Bernardo's machine, MSVC 14.51 (Release, `/O2 /GL`) against w64devkit's gcc
16.1 (`-O3`), the RMS of the difference over the sound's RMS, by quarter of each drive:

| Drive | 1st quarter | 2nd | 3rd | 4th |
| --- | --- | --- | --- | --- |
| idle, the four, 6 s | 4.2e-10 | 1e-9 | 1.5e-9 | 8e-10 |
| dyno hold, cross-plane V8, 3 s | 1.6e-9 | 1.9e-9 | 2.6e-9 | 2.7e-9 |
| rev, the six, 12 s | 4.3e-10 | 1.3e-8 | 7.8e-7 | 1.2e-4 |
| sweep, the four, in gear | 1.9e-9 | 2.5e-9 | 7e-9 | 6.1e-9 |
| rev, the boxer, pass-by stereo | 1.1e-10 | 3e-9 | 1.2e-7 | 8.7e-7 |

The difference starts at the last bit, mostly stays near a billionth of the sound, and
grows where the engine magnifies it: in the six's rev, to 1.2e-4 by the idle after the
limiter (−78 dB, 0.08 dB of level). A first design froze per-quarter tolerances from this
(1e-3, 0.5 rpm, 0.5 dB per block).

**CI broke that design, usefully.** On a GitHub runner the same two compilers agreed as
closely on four drives, but the boxer's rev came apart in its last quarter: 0.35 of the
sound, 2.6 rpm, blocks 10 dB apart. Near the rev limiter the soft cut drops sparks at random
against a share worked out from the speed (stage 12c). A last-bit difference in the speed
can flip one decision, and from there each render is its own run, both valid. The runner's
CPU takes different paths in the C libraries' maths than this machine's, so it flipped there
and not here. No sample-by-sample tolerance can hold across that.

**So the check has two parts** (`compiler_parity.py`, tested in `test_compiler_parity.py`):
- **before anything can flip,** each drive's first quarter must agree to 1e-6 of the sound
  (measured: at most 2e-9, here and on the runner). Wrong code shows here at once;
- **over the whole drive,** it must be the same engine: the level within 0.5 dB, every
  octave band (31.5 Hz–16 kHz, within 60 dB of the loudest) within 2 dB, the speed within
  40 rpm on average. These are sized by a far bigger change than one flipped cut: the same
  build with another variation seed stays within 0.23 dB, 1.2 dB and 24 rpm.

On the runner (run #7) the boxer's rev, flipped again, came in at 8e-11 for its first
quarter, 0.001 dB of level, 0.61 dB in its worst band and 0.32 rpm on average: within the
tolerances. CI checks MSVC against gcc on every Windows push, and Apple clang against
Homebrew's gcc in the macOS run.

## Continuous integration

`.github/workflows/ci.yml`, every action pinned to a commit and every tool to a version
(Python 3.14.8, Node 22.23.4, w64devkit 2.8.0, CMake 4.4.4, Ninja 1.13.2):
- **Windows** (`windows-2025`), every push to `main`, pull requests and on demand: every
  suite but the browser test, through `run_tests.py`. That covers the Makefile's suites with
  w64devkit's gcc, the WebAssembly check, the Python and web unit tests, and `cmake` with
  MSVC. Then MSVC against gcc. About 23 minutes.
- **Linux** (`ubuntu-24.04`), the same pushes: every suite but the browser test with gcc,
  `cmake` included (about 9–15 minutes); and a second job, the C tests under
  AddressSanitizer and UBSan (about 18–33 minutes; it passed in every run that finished it,
  and run #7's was still going when this was written).
- **macOS** (`macos-15`), Mondays and on demand (`workflow_dispatch` with `macos`): the
  `cmake` suite with Apple clang, then Apple clang against gcc.
- **What failed shows on the run's page:** a failed test step puts its results table, its
  FAIL lines and its last lines into an annotation (`tools/ci_summary.sh`), and the parity
  check posts its table as a notice. The job logs and summaries didn't load in the desktop
  app's browser; annotations do.

Getting it green took seven runs, each a lesson:
1. w64devkit on `GITHUB_PATH` put its BusyBox `bash` ahead of Git's, and BusyBox refuses
   the runner's `bash --noprofile` (exit code 2). Now each step adds w64devkit itself.
2. A suite failed out of sight: the annotations were added (runs 3–6).
3. The parity check looked for the CMake build in `~/rb-build/cmake`; `run_tests.py` puts it
   in `~/rb-build/tests/cmake`.
4. The boxer's flipped soft cut, above.

**The repository is public** (GitHub's API answers without signing in), though the plan's
decision was to keep it private for now. On a public repository GitHub's standard runners
are free, macOS included, so the 2,000-minute budget behind running macOS weekly doesn't
apply. Bernardo decided to keep it public (2026-11-09, app-design §15); macOS stays weekly
and on demand.

## The test runner

`run_tests.py` gains the `cmake` suite (configure, build and CTest; MSVC through
`msvc_env.py` on Windows). Its pre-flight now also checks CMake (3.25 or newer) and Ninja,
and on Windows that there's a Visual Studio with the C++ tools. It finds the venv's CMake
and Ninja without them on PATH.

## A planted portability fault

`__builtin_expect` (a GNU builtin) in `rb_sim.c`'s `render`, in a copy of the sources:
w64devkit's gcc builds and links it without a word (`make ... rb_tests`, exit 0); MSVC
refuses it ("C4013: '__builtin_expect' undefined", an error under `RB_WERROR`). That's
the MSVC job's point: gcc alone would let it through.

`tools/bugs/stage122_python.py` plants five faults in the new tools, all caught: the
pre-flight accepting an old CMake; the parity check ignoring the level, misaligned renders
or the bands, or measuring its strict check over the whole drive (a late flip would then
fail it). `bands-not-compared` was missed at first: no test fed the comparison renders that
differ in one band alone; `test_one_band_louder_is_caught` does now (an 8 kHz whistle on a
100 Hz note, the level moved 0.01 dB, the band 17).

## Tests

`python tools/run_tests.py --keep-going`, every suite, on both systems:

| Suite | Windows | WSL |
| --- | --- | --- |
| native (Makefile) | 262 tests, 4,968,415 checks (w64devkit gcc 16.1) | the same (gcc 15.2) |
| cmake | the same 262 tests with MSVC 14.51 | the same with gcc 15.2 |
| wasm | the committed core is current | the same |
| wasm-c | 18 tests, 5,743 checks | the same |
| python | 119 tests (114 + 5 new), none skipped | the same |
| web | 17 tests | the same |
| e2e | 2 tests, both modes | the same |

`plant_bugs.py --check`: 0 stale. `compiler_parity.py check` MSVC against gcc: within the
tolerances.
