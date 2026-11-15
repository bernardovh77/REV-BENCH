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

<a id="stage-127"></a>
## Stage 127 — The 2026-11-15 review's fixes

**Dependencies:** [118](07-app.md#stage-118), [123](07-app.md#stage-123), [124](07-app.md#stage-124).

**Deliverable:** What [the review](../review-2026-11-15.md) found that can be fixed before
stage 066, fixed; the rest given a stage or written into the stage that owns it.
- **The pipes' hand-off to the listener** (problem 4): with pipes slower than 48 kHz the
  listener reads their flows between the pipe steps (a polyphase windowed-sinc smoother),
  in mono and in the stereo scenes. Pipes at 48 kHz are untouched: renders at the high and
  balanced qualities stay bit for bit.
- **The A/B switch** (problem 1): `RB_PARAM_VARIANT`, a live integer setting.
- **macOS on every push** (problem 9): the core, the host on the null device and the add-on
  with Apple clang; the compiler-parity step stays weekly.
- **A weekly long fuzz** (problem 5) over the engine fields' whole ranges.
- **Rules as checks** (problem 10): a pre-push hook that runs the quick suites; CLAUDE.md
  split into rules and `docs/architecture.md`; planted bugs that survive re-indenting.
- **Housekeeping** (problem 11): the stray folders gone, with a test; the old browser page's
  end-to-end test on request only; the missing licence stated in the README until Bernardo
  chooses one.
- **References** (problem 2): a recording of a known, naturally aspirated engine in the
  catalog, and the first one-to-one comparison with a preset.

**Required exit evidence:** every suite green on Windows; CI green on Windows, Linux and macOS;
`tools/cli_parity.py` identical for every case at the high quality; `tools/speed_check.py`
within 3 %; `tools/tier_compare.py` for the declared tiers; planted bugs caught.

**Stop/reject rule:** No preset's declared tier changes without Bernardo: a cheaper tier that
the smoother makes possible is measured and put to him, not switched on. No licence is chosen
for him.

**Execution record:** `docs/stage-127.md`.

<a id="stage-128"></a>
## Stage 128 — The complaints list

**Dependencies:** [066](07-app.md#stage-066) (live play and the A/B key), [127](#stage-127).
Before [050](04-propagation.md#stage-050).

**Deliverable:** For each thing Bernardo has said is wrong with the sound, the shortest
physically honest candidate, behind `RB_PARAM_VARIANT`, listened to live and blind:
- **the cycle repeats too exactly** ("less like a piano note"): against the one matched
  recording so far (stage 127) the four repeats 18 dB more exactly below 500 Hz than a real
  Honda F20C. Stage 12c varied the burn; what the gas does on its way down the pipe
  (turbulence: slow, weakly correlated modulation of each pulse) is not modelled;
- **dull at idle from behind**;
- **the four is quiet** next to the others: its level measured against recordings at a known
  distance before anything is changed;
- **the V8's crackle** on the overrun: which layer makes it (`--layers`), then why.

Each candidate states what is physical in it and what is fitted, with the recording it was
fitted to. A candidate that wins is promoted to the accepted sound (variant 0) in a stage of
its own with the sound check; one that loses is deleted and its number retired.

**Required exit evidence:** per complaint: the measurement that shows it, the candidate's
effect on that measurement and on the engine's numbers (torque, VE within 1 %), and
Bernardo's verdict from blind A/B listening in the app, in his words.

**Stop/reject rule:** Nothing here replaces the pipes' solver (that stays stages 057–061).
No candidate becomes the default without his yes. A linear filter fitted to a recording is
named as fitted, never as physics.

**Execution record:** `docs/stage-128.md`.

<a id="stage-129"></a>
## Stage 129 — The cold end of the exhaust, linear

**Dependencies:** [127](#stage-127), [049](04-propagation.md#stage-049). Before
[062](05-engine-qualification.md#stage-062); it may run beside 050–056.

**Deliverable:** A quality tier in which the exhaust downstream of the point where its waves
are weak is carried by delay lines (the research graph of `experiments/acoustics/wave.c`,
joined to the last nonlinear cell by characteristics as `coupled_duct.c` proved), not by gas
cells: for speed first (stage 118: the cost is the thermodynamics of about 400 cells), and
for the band above the 7.5 cm grid's 2.4 kHz as a side effect.
1. **The premise, measured:** for the five presets at full load and on the overrun, each
   exhaust cell's pressure swing and Mach number; where the swing falls under 5 % of the mean.
   If that line is not near the catalyst the stage is replanned.
2. One pipe (the four's tailpipe), with the mean flow kept as a slow term so the back
   pressure stays right; then the muffler; then the exhaust pipes.
3. The tier declared like stage 118's, with its misses stated.

**Required exit evidence:** torque, VE and back pressure within 1 % of the high quality;
mass in equals mass out over a cycle; `tools/tier_compare.py` within 1 dB; the speed-up on
the baseline laptop, with `tools/speed_check.py`; live A/B by Bernardo.

**Stop/reject rule:** The old and the new copy of a pulse never both sound. If the premise
fails (step 1) the stage closes as a recorded negative result.

**Execution record:** `docs/stage-129.md`.

<a id="stage-130"></a>
## Stage 130 — `engine.c` split along the step order

**Dependencies:** [127](#stage-127). Before [078](09-turbo.md#stage-078) (turbo, knock and
limits each add to the engine's step).

**Deliverable:** `core/src/engine.c` (1,674 lines: the build, the computer, the intake, the
exhaust, the step and the books) in parts that follow the step order
`docs/architecture.md` describes: the engine computer; building and stepping the intake side;
the exhaust side; the cycle and its books. No behaviour change.

**Required exit evidence:** `tools/cli_parity.py` identical for every case;
`tools/speed_check.py` within 3 % on both compilers (functions that were inlined inside one
file must still be: measure, and keep them in one translation unit if that is what it
takes); `make wasm` rebuilt; every planted bug of every table still applies
(`plant_bugs.py --check`).

**Stop/reject rule:** A split that costs speed is not made: readability is not bought with
real time.

**Execution record:** `docs/stage-130.md`.
