# Stage 122 — Native builds with MSVC and gcc, and continuous integration

**Status:** done locally (2026-11-09); CI_STATUS_PLACEHOLDER

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
drives with two builds' `rb_cli` and compares the float WAVs (the RMS of the difference
over the sound's RMS, per quarter of each drive) and the per-block CSVs (rpm, SPL).

Measured, MSVC 14.51 (Release, `/O2 /GL`) against w64devkit's gcc 16.1 (`-O3`):

| Drive | Sound, worst quarter | rpm | SPL dB |
| --- | --- | --- | --- |
| idle, the four, 6 s | 1.5e-9 | 0 | 1e-7 |
| dyno hold, cross-plane V8, 3 s | 2.7e-9 | 0 | 0 |
| rev, the six, 12 s | 1.2e-4 | 0.0014 | 0.078 |
| sweep, the four, in gear | 7e-9 | 1e-6 | 2e-7 |
| rev, the boxer, pass-by stereo | 8.5e-7 | 1e-5 | 3e-5 |

(A Debug MSVC build, measured first by mistake, gave the same picture.)

The first difference comes within milliseconds (at the last bit), and mostly stays near a
billionth of the sound. It grows where the engine magnifies it: the six's rev went from
4.5e-10 in its first quarter to 1.2e-4 in its last, the idle settling after the limiter.
That's still −78 dB below the sound, and 0.08 dB of level.

**Frozen tolerances** (`compiler_parity.py`), about 8 times the worst measured: the
difference's RMS at most 1e-3 of the sound's (−60 dB) in every quarter, 0.5 rpm and
0.5 dB SPL at every block. CI checks MSVC against gcc on every Windows push, and Apple clang
against Homebrew's gcc in the weekly macOS run. A compiler that goes beyond them needs a
look, not a looser number.

## Continuous integration

`.github/workflows/ci.yml`, every action pinned to a commit and every tool to a version
(Python 3.14.8, Node 22.23.4, w64devkit 2.8.0, CMake 4.4.4, Ninja 1.13.2):
- **Windows** (`windows-2025`), every push to `main`, pull requests and on demand: every
  suite but the browser test, through `run_tests.py`. That covers the Makefile's suites with
  w64devkit's gcc, the WebAssembly check, the Python and web unit tests, and `cmake` with
  MSVC. Then MSVC against gcc (`compiler_parity.py check`).
- **Linux** (`ubuntu-24.04`), the same pushes: every suite but the browser test with gcc,
  `cmake` included; and a second job, the C tests under AddressSanitizer and UBSan.
- **macOS** (`macos-15`), Mondays and on demand (`workflow_dispatch` with `macos`): the
  `cmake` suite with Apple clang, then Apple clang against gcc.

The private repository's free plan gives 2,000 minutes a month, Windows' counting twice
and macOS's ten times; a push costs about MINUTES_PLACEHOLDER.

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

`tools/bugs/stage122_python.py` plants faults in the new tools: the pre-flight not checking
CMake's version, and the parity check passing what it shouldn't.

## Tests

`python tools/run_tests.py --keep-going`, every suite, on both systems:

| Suite | Windows | WSL |
| --- | --- | --- |
| native (Makefile) | 262 tests, 4,968,415 checks (w64devkit gcc 16.1) | the same (gcc 15.2) |
| cmake | the same 262 tests with MSVC 14.51 | the same with gcc 15.2 |
| wasm | the committed core is current | the same |
| wasm-c | 18 tests, 5,743 checks | the same |
| python | 117 tests (114 + 3 new), none skipped | the same |
| web | 17 tests | the same |
| e2e | 2 tests, both modes | the same |

`plant_bugs.py --check`: 0 stale. `compiler_parity.py check` MSVC against gcc: within the
tolerances.
