# Stage 120 — Workspace and toolset upkeep

**Status:** done (2026-11-08)

What the three reviews of 2026-11-06 found in the workspace and tools, closed before the app work
starts ([specification](roadmap/14-review-fixes.md#stage-120)). No `core/` change: the engine,
its sound and the WebAssembly core are stage 119's, byte for byte.

The new tool code was written test first, red then green, one slice at a time, following
Bernardo's `tdd` skill (`.agents/skills/tdd`). He chose the four seams under test:
1. `plant_bugs.py --check`: a bug whose text is gone is reported;
2. the test runner's command runner: a hang, and the child it started, are killed at the
   timeout;
3. `--keep-going`: by default the run stops at the first failed suite, and with the flag it runs
   the rest;
4. the pre-flight check: a missing make or Node, or a Node older than 22, is reported before any
   suite starts.

Each test was seen failing before its code existed (`tools/test_plant_bugs.py`,
`tools/test_run_tests.py`).

## Clutter

- `References.zip` (4.7 GB) and `stage-115-renders/` (28 MB, the stage-115 listening renders)
  moved into the git-ignored `renders/`, as `renders/References.zip` and
  `renders/stage-115-renders/`. Nothing deleted.
- `.gitignore` ignores archives and bulky media anywhere (`*.zip`, `*.webm`, `*.mkv`, `*.mp4`,
  `*.mov`, `*.mp3`, `*.wav`, `*.flac`, `*.m4a`; no tracked file matches). The three root-level
  video entries, for files long since moved, are gone.
- `.agents/` and `skills-lock.json` (three agent skills from mattpocock/skills: grill-with-docs,
  tdd, to-tickets) are git-ignored, Bernardo's choice: each machine's own.
- `plant_bugs.py` no longer copies `*.zip` or `*.flac` into its work folders (the 4.7 GB zip had
  been copied for every bug), and `run_tests.py`'s web copy leaves out WAV and video files too.

## Planted bugs

`python tools/plant_bugs.py --check` (or `STAGE --check`) loads every table in `tools/bugs/` and
reports each bug whose text no longer matches exactly once in today's sources. It exits 1 if
there is one, and the Python suite runs it (`test_the_repository_has_no_stale_bug`), so a change
that moves a bug's text has to retarget it in the same stage.

The committed tables had **29 stale bugs** of 281: 9 of stage 12b's, 7 of 12c's, 4 of 13's, 3 of
12's, 3 of S1b's, 2 of S1d's and stage 119's Python `make-backslashes`. Stage 119's reformat,
Codex's stages 037–049 and stage 115 had moved their text.
- **26 retargeted** to today's code, each planted and caught:

  | Bug | Caught by |
  | --- | --- |
  | 12 no-interpolation | test_audio_listener_paths |
  | 12 mouth-not-heard (now the per-step push, not the rebuild's prime) | test_audio_sim_hears_every_source |
  | 12 onward-as-volume | test_engine_muffler_back_pressure |
  | 12b second-tailpipe-silent | test_noise_jet_eighth_power |
  | 12b valve-mach-cubed, valve-never-chokes, catalyst-ignored | test_noise_valve_jets |
  | 12b every-cylinder-to-tail-0 | test_noise_valve_routing (added to its tests: stage 115 moved the routing there) |
  | 12b hiss-at-the-tailpipe | test_noise_throttle_hiss |
  | 12b injector-when-motored (now: a click for every dose event, delivered or not) | test_noise_impacts_per_cycle |
  | 12b cabin-without-mounts | test_audio_cabin |
  | 12b pipes-without-sonic-guard | test_pipe_most_sonic, test_engine_vacuum |
  | 12c start-not-varied (now the development delay held at its mean) | test_variation_cov_of_imep |
  | 12c length-not-varied | test_variation_cov_of_imep |
  | 12c fuel-trim-ignored, burn-trim-ignored, burn-trim-from-the-fuel | test_variation_cylinders_differ |
  | 12c no-soft-cut | test_variation_soft_cut |
  | 13 mixture-hi-weights | test_gas_matches_nasa_data, test_gas_u_and_T_are_inverse |
  | 13 cv-never-updated | five chamber tests |
  | 13 momentum-flux-lost | three pipe tests |
  | 13 low-quality-at-192kHz | test_core_quality |
  | S1b demand-still-capped | test_variation_partial_burns |
  | S1b dilution-ignored (now in combustion.c) | test_variation_partial_burn_residual |
  | S1d requested-mixture | test_variation_trapped_mixture, test_variation_trapped_mixture_capture |
  | 119 make-backslashes (Python) | test_make_gets_forward_slashes |

- **3 retired,** each with a note in its table:
  - 12c `start-unbounded`: stage 040 replaced the bounded start with a development delay that is
    never negative; stage 040's `discharge-at-random-kernel-angle` plants the unbounded one.
  - S1d `life-off-changed`: the trapped mixture now feeds the burn with variation off too (Codex's
    charge-burn model, by design), so no variation-gated path is left to break.
  - S1b `life-off-uncapped`: retargeted, it was **missed**. The cap is now Codex's
    `RB_BURN_CAPPED` model, chosen apart from the variation, and no test pins that model's burn
    length to stage 9's: `test_combustion_zero_variation_continuity` checks only that it stays
    under 180°. **A gap to close** when the capped model is next touched (it is the legacy
    model; the default is `RB_BURN_CHARGE`).

## The test runner

`tools/run_tests.py`:
- **Pre-flight:** make for native, wasm and wasm-c; Node for wasm-c, web and e2e (npm, npx too),
  and Node 22 or newer. Anything missing is listed and the run stops (exit 2) before any suite
  starts.
- **Time limits:** each suite has one (`TIMEOUTS`: 30 min native, 10–20 min the rest, far above
  their usual times). A suite past it is killed with every process it started (`taskkill /T` on
  Windows, its own process group on Linux) and reported as `timeout`. The planted hang, a
  command that starts a sleeping child and sleeps itself, is killed at 3 s with its child
  (`test_a_hang_and_the_child_it_started_are_killed`). Ctrl-C kills the tree too.
- **Order:** the run stops at the first failed suite; `--keep-going` runs the rest. Skipped
  suites show as `skipped` in the table.
- **Windows:** the web copy is removed with read-only files made writable and a few retries (npm
  leaves read-only files; a just-exited process can hold one for a moment); zig's Python path is
  quoted in case of spaces.

## Make

- **Python:** the Makefile uses the venv of `tools/requirements-dev.txt` when there is one,
  `~/rb-venv/bin/python` on Linux and `~/rb-venv/Scripts/python.exe` on Windows, and so its zig
  for `make wasm`; else `python3` or `python` as before.
- **`SAN=1`** first checks that the compiler can link the sanitizers' runtimes. With w64devkit,
  which has none, make stops with a sentence saying so (use WSL) instead of a link error.

## Hooks, docs and line endings

- **The graphify reminder hook** finds graphify on PATH, in `~/.local/bin` (WSL) or in the venv
  (`~/rb-venv/Scripts/graphify.exe` on Windows, `bin/` on Linux). Before, it found nothing on
  Windows and stayed silent.
- **The cloud set-up script** installs `tools/requirements-dev.txt` (imageio-ffmpeg was missing
  there) and reinstalls when the file changes.
- **Docs:** CLAUDE.md's test-tone notes ("until stage 14" is now stage 121's API), its "device
  bridge" section (now "Environment": no VM, the e2e test runs everywhere, the runner's new
  behaviour, `python` rather than `python3` on Windows), the `--check` rule;
  docs/workspace.md (the desktop app's Code tab on Windows, graphify in the venv, the runner).
  README.md had nothing stale; PLAN.md already had the 2026-11-06 parity.
- **Line endings:** four files that graphify and a tool had written with CRLF on Windows are LF
  again (`git ls-files --eol` shows none left), and `core.autocrlf` is `false` for this repo.
  `.gitattributes` already stores everything as LF; graphify rewrites CRLF on each update on
  Windows, which git normalises (noted in CLAUDE.md).

## Not done here, for Bernardo

Two changes the session's safety check rightly left to Bernardo:
- **Allow rules for the venv's graphify** in `.claude/settings.json` (Claude may not widen its own
  permissions). To stop the prompts on Windows, add `Bash(~/rb-venv/Scripts/graphify query:*)`,
  and the same for `path`, `explain` and `update`, to `permissions.allow`.
- **Node on WSL's login PATH** (a change to `~/.profile`, outside the repo). The runner adds
  `~/.local/node22/bin` itself, so nothing depends on it; for the shell, add to `~/.profile`:
  `[ -d "$HOME/.local/node22/bin" ] && PATH="$HOME/.local/node22/bin:$PATH"`.

## Tests

`python tools/run_tests.py --keep-going`, with the venv's Python, on both systems; the same
results on each:

| Suite | Windows (w64devkit) | WSL (gcc) |
| --- | --- | --- |
| native | 251 tests, 4,950,067 checks, pass (484 s) | the same (393 s) |
| wasm | the committed core is current, byte for byte | the same |
| wasm-c | 18 tests, 5,743 checks | the same |
| python | 111 tests (102 before, plus 9 new), none skipped | the same |
| web | 17 tests | the same |
| e2e | 2 tests, dev and production builds | the same |

`plant_bugs.py --check` reports 0 stale bugs, and `git status` shows only this stage's changes;
everything else is ignored. `SAN=1` builds in WSL and stops with its message on w64devkit.
There was no core change, so there was no sound check and no `make wasm`.
