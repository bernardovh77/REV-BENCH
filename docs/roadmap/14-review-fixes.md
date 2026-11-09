# Bounded completed-stage review repairs

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md)

Allocated after 111 to preserve the canonical IDs. Execution priority is
112 → 113 → hold; 037 remains the next planned feature stage. These repair
the two new C07/C08 reproductions, not the six broader retained risk groups.

<a id="stage-112"></a>
## Stage 112 — Recoverable checkpoint section publication

**Dependencies:** [033](02-existing-sound.md#stage-033), [036](03-contracts-and-sources.md#stage-036)

**Deliverable:** Repair C08 at the existing section-publisher/composer boundary
using complete immutable fragments and recoverable manifest/page installation.
Keep the single portable result, accepted examples and references.

**Required exit evidence:** Interrupted fragment/page/manifest writes and failed
installs preserve a playable, rebuildable checkpoint; retry works without losing
other sections. Full results rebuild and offline playback preserve all 48 clips.

**Stop/reject rule:** No success claim from page survival alone; verify registered
fragments and subsequent composition. Do not claim multi-file crash atomicity
or concurrent-writer support from ordinary exception recovery.

**Execution record:** [Stage 112](../stage-112.md).

<a id="stage-113"></a>
## Stage 113 — Acoustic derived-state and numerical failure guards

**Dependencies:** [032](02-existing-sound.md#stage-032), [034](02-existing-sound.md#stage-034), [036](03-contracts-and-sources.md#stage-036)

**Deliverable:** Repair C07 at B0 prepare/process/observer boundaries. Reject
unrepresentable derived coefficients and report failed numerical books/storage;
define reset/retry behavior without changing the qualified linear equations.

**Required exit evidence:** Reproductions fail safely, ordinary cases retain
their numerical results, invalid-argument retry still works, numerical failures
require reset, and regression/sanitizer/deliberate-fault checks pass. Record cost.

**Stop/reject rule:** Finite guards are not physical amplitude qualification,
stability proof, engine coupling or production promotion. No silent protective
intervention may be reported as an ordinary successful acoustic step.

**Execution record:** [Stage 113](../stage-113.md).

<a id="stage-115"></a>
## Stage 115 — Valve jet noise and the cycles' variation

**Dependencies:** [040](03-contracts-and-sources.md#stage-040), [041](03-contracts-and-sources.md#stage-041), [042](03-contracts-and-sources.md#stage-042), [113](#stage-113)

**Deliverable:** Fix the audit's D02–D05 in the production noise layer:
- the gap's Mach number and speed from the pressures either side;
- valve noise through X and Y junctions to every tailpipe beyond them;
- full path lengths;
- the catalyst's share only with a catalyst.

Recalibrate what the corrected law moves. Bring the cycles' variation back to the
real engines' 1–3 % COV of IMEP wide open with a causal delay, and restore the
test's floor.

**Required exit evidence:**
- an independent law check across choking;
- power-conserving, in-phase routing;
- per-preset COV;
- before and after measurements against the real recordings;
- planted bugs caught;
- native, sanitizer, WASM and web suites green;
- A/B renders for listening.

**Stop/reject rule:** Change no gas dynamics, preset or exhaust. Don't move a
test's limit unless the corrected physics explains it, and say so where it moves.

**Execution record:** [Stage 115](../stage-115.md).

<a id="stage-116"></a>
## Stage 116 — Tests that can fail again

**Dependencies:** [115](#stage-115)

**Deliverable:** Tighten the tests whose limits had been loosened until they
couldn't catch what they were written for:
- the junction's energy books;
- the wild cam's burn length;
- the coast's fuel cut;
- the rev limiter's top;
- two checks in the radiation test that couldn't fail.

Fix `test_render_results`, which fails under GNU make 4.3.

**Required exit evidence:** a planted bug for each tightened test that the
old limit passed and the new one catches; native and Python suites green.

**Stop/reject rule:** No `core/` change. Set a new limit from the measured
behaviour and the design's intent, not from what happens to pass.

**Execution record:** [Stage 116](../stage-116.md).

<a id="stage-117"></a>
## Stage 117 — Docs you can read, and up to date

**Dependencies:** [116](#stage-116)

**Deliverable:**
- restore the spaces Codex's docs lost between words and numbers, in prose only;
- bring CLAUDE.md, the README and docs/workspace.md up to date with the project as it is;
- add the research stages' abbreviations to the glossary;
- fix `rb_cli`'s help text and its over-long string literal.

**Required exit evidence:** a before/after count of the missing spaces; no code, link or
identifier changed by the pass; links checked; native, Python and web suites green.

**Stop/reject rule:** No `core/` change. Don't rewrite Codex's historical records beyond the
spacing; the cases a script can't decide safely stay listed.

**Execution record:** [Stage 117](../stage-117.md).

<a id="stage-119"></a>
## Stage 119 — The review's leftovers

**Dependencies:** [117](#stage-117)

**Deliverable:** Close what Claude's review of Codex's work (2026-10-31) found and stages
115–117 didn't fix:
- **Readable C:** Codex's compressed code in `core/` (engine.c, combustion.c, ignition.c,
  injection.c, scene.c, diagnostics.c, radiation.c, exhaust_models.c and the rest) is formatted
  and commented as CLAUDE.md asks, with no change in behaviour.
- **The reference tools measure in stereo as they say:** `prepare_user_references.py` folds to
  mono by default while the page says stereo.
- **Failed publishes:** a failed audio-experiment publish no longer blocks `make results` and
  `make render`.
- **Checks:** the tools' acceptance checks are explicit exceptions, not `assert`s (`python -O`
  drops those).
- **Paths and timing:** Windows paths and encodings work, and "lift time" means one instant in
  every report.
- **Prose numbers:** numbers in published prose come from the evidence.
- **The research solvers' tests can fail:**
  - `gas_graph` gets an accuracy test against an analytic wave;
  - its energy checks no longer pass by construction;
  - `test_junction.c`'s closed-box test checks bounded energy, not numerical damping.

**Required exit evidence:**
- Renders and test results before and after the reformatting are identical, bit for bit.
- A planted bug for each new or changed test.
- Native, Python and web suites green.

**Stop/reject rule:** No physics change. Reformatting that changes a single rendered sample is
a bug.

**Execution record:** [Stage 119](../stage-119.md).

<a id="stage-120"></a>
## Stage 120 — Workspace and toolset upkeep

**Dependencies:** [119](#stage-119)

**Deliverable:** Close what the three reviews of 2026-11-06 found in the workspace and tools,
before the app work starts:
- **Clutter:** `References.zip` (4.7 GB) and `stage-115-renders/` move into the git-ignored
  `renders/`. `.gitignore` and `plant_bugs.py`'s copy list ignore `*.zip` and other bulky media,
  and the stale root-level video entries go. `.agents/` and `skills-lock.json` are committed or
  ignored, as Bernardo chooses.
- **Planted bugs:** the 30 that no longer apply are retargeted to today's text, or retired with a
  note in their table, and `plant_bugs.py --check` reports any that stop applying. The check
  runs in the Python suite.
- **Docs:** README.md and CLAUDE.md are current (the test tone "until stage 14", the obsolete
  "device bridge" section, cloud-only e2e steps, `python3` on Windows). docs/workspace.md matches
  the desktop app's sessions. PLAN.md's "Where the project is" gains the 2026-11-06 parity.
- **Hooks:** the graphify reminder hook and the settings' allow rules find graphify in the
  Windows venv. The cloud set-up script installs `tools/requirements-dev.txt`.
- **Make:**
  - the Makefile finds the venv's Python (and so zig) on both systems;
  - `~/.local/node22` goes on WSL's login PATH;
  - `SAN=1` says plainly that it needs a sanitizer runtime (WSL and macOS), not just fail to
    link on w64devkit.
- **The test runner:** per-suite timeouts that kill the whole process tree, a check that make and
  Node exist (and Node's version), a sturdier removal of the web copy on Windows, quoted paths,
  and `--keep-going`.
- **Line endings:** files that graphify writes on Windows are renormalised, and `core.autocrlf`
  is turned off for this repo.

**Required exit evidence:** `plant_bugs.py --check` reports no stale bug; `git status` is clean
apart from what's ignored; `run_tests.py` passes on Windows and WSL with the new checks, and a
planted hang is killed by its timeout.

**Stop/reject rule:** No `core/` change. Bernardo's recordings are moved, never deleted.

**Execution record:** `docs/stage-120.md`.
