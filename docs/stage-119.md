# Stage 119 — The review's leftovers

**Status:** done (2026-11-03)

The last item of Claude's review of Codex's work (2026-10-31), the parts stages 115–117 didn't
fix ([specification](roadmap/14-review-fixes.md#stage-119)). No physics changed: the engine, its
sound and every render are exactly stage 117's.

## Readable C, and proof that nothing changed

Codex's code in `core/` was written compressed: `if(x) {a=b;c=d;}`, no spaces around operators,
several statements to a line, few comments. It is now spaced and commented like the rest of the
core:
- **Rewritten whole, with comments:** `diagnostics.c` and `diagnostics.h` (what each reason, role
  and function is), `ignition.c`/`.h` and `injection.c`/`.h` (each event kind explained).
- **Reformatted line by line:** Codex's lines in `engine.c`, `combustion.c`, `pipe.c`,
  `rb_sim.c`, `chamber.c`, `gas.c`, `noise.c`, `flow.c` and `radiation.c`. clang-format 23 did
  only the lines in Codex's style (the hand-aligned code around them was left alone), and the
  rest was done by hand. The two event records are built with designated initializers
  (`.tick = ...`), so each field is named.
- **New comments:** `combustion.c`'s flame speed, the burn's length for a charge, the Wiebe
  quantile and the mean burn length; `scene.c`'s coordinates, paths and per-sample step;
  `radiation.c`; `rb_sim.c`'s stereo scene; `exhaust_models.h`; the research controls in
  `engine.h`.

**The evidence that behaviour is unchanged**, compared with stage 117's sources (commit 8cbd660):

| What | How | Result |
| --- | --- | --- |
| WebAssembly core | `make wasm` (zig 0.17), both trees | byte-identical, 150,931 bytes (the committed one reproduces exactly) |
| Native core objects | all 24 `core/src/*.o`, `-O2 -g0` and `-O3 -g0`, LTO off | byte-identical, at both levels |
| Renders | the four's idle (and from the cabin), sweep; the six's and cross-plane V8's revs; the flat-plane V8 held at 3000 rpm; the boxer's rev in the asphalt stereo scene; WAVs and CSVs | 10 files, all bit-identical |
| Native tests | `make test` | 250 tests, 4,950,077 checks, the same list of passes |

Building the objects is the strongest test: one `-O2` object did differ at first. Moving a
diagnostic's reason into its own variable in `pipe.c`'s `push_end` changed GCC's instruction
order, though not the arithmetic. It went back inside the call, with a comment, and the object
matched again. Only the WebAssembly fingerprint (`rb_core.sources.sha256`) changes, because the
sources' text did.

**The sound check** (PLAN.md, step 6): nothing to listen to, since every render is the same file.

**Planted-bug tables:** the bugs that patch the reformatted lines in `tools/bugs/stage040.py`,
`stage041.py`, `stage042.py`, `stage115.py` and `stage116.py` (20 of them) now match the new text,
with the same changes. All 245 bugs that applied to stage 117's sources still apply (28 older ones
had already stopped applying, as before).

## The reference tools measure in stereo

- **`compare_real.py`** read every WAV as the mean of its channels. A stereo file is now measured
  channel by channel, the channels' powers (and the repeats' correlations) then averaged, as the
  protocol in [docs/user-sound-references.md](user-sound-references.md) asks. A mono file gives
  exactly the numbers it did. On a truly stereo stretch of the rear V8 reference (30–40 s of
  `user-rcf-lorax-rear-3m-0`), the fold had read the share above 4 kHz 0.84 dB low (−27.12 instead
  of −26.28 dB), above 2 kHz 0.30 dB low, and the slope 0.5 dB/octave steep: what is in antiphase
  between the microphones cancels in a fold. Its first 15 s are the same in both channels, so they
  measure the same either way.
- **`orders.py`** takes its spectrum the same way.
- **`prepare_user_references.py`** wrote mono measurement excerpts unless a recording asked for
  stereo, while the page said the measurements were stereo. Stereo is the default now. The four
  legacy recordings of `user_engine_reference_sources.json` ask for mono explicitly
  (`"measurement_channels": 1`), so their 16 excerpts still reproduce.

## A failed publish no longer blocks `make results`

`audio_experiment_report.py` wrote its diagnostic files into `renders/current/experiment/` before
publishing. If the publish then failed, the files no longer had the fingerprints the checkpoint's
manifest held, and both `make results` (`diagnostic_names`) and `make render`
(`carry_diagnostics`) refused to run until someone repaired the folder by hand.

Now the report writes them to a staging folder and hands it to `publish_section`, which builds the
next folder beside the current one and swaps it in only together with the manifest and the page.
On a failure at any step it puts back the files and the manifest.

## Checks that `python -O` can't drop

The 28 `assert`s that were acceptance checks in seven tools (`check_numerical_diagnostics.py`,
`check_thermal.py`, `check_material.py`, `audio_experiment_report.py`,
`analyze_acoustic_transport.py`, `audio_reference_probe.py`, `prepare_user_references.py`) are now
`require(condition, what)` from the new `tools/checks.py`. It raises `CheckFailed`, a
`ValueError` like the tools' other refusals, with a plain message.

## "Lift time" is one instant

`rb_cli sound` moves the pedal (and lets the clutch in) at the start of a 128-frame block and
writes the block's CSV row at its end. `measure_lift_feedback.py` took the lift as the previous
row's time, the instant itself; `render_results.py`, `exhaust_feedback.py`,
`ignition_checkpoint.py` and `checkpoint_13b.py` took the first row with the throttle shut, 2.67 ms
later. All five now use `checks.lift_at()` and `clutch_at()`: the start of the first block that
shows the change, with the speed at that instant. The measurement windows after a lift move 2.67 ms
earlier the next time those reports run; nothing published was recomputed. The render tests'
hand-written CSVs had a second between rows; they now have rows a block apart around each change,
as `rb_cli` writes them.

## Windows

Bernardo's machine runs the tools natively on Windows too. There, the Python suite had 9 failures
of 90:
- **Paths:** names relative to a folder came out with backslashes (`feedback\i4-long-coast.wav`),
  so `write_page` called every checkpoint incomplete. They are spelled with forward slashes
  everywhere now (`as_posix()`), and so are the paths handed to `make`: its shell ate the
  backslashes, and a backslashed `--assume-old` made `make render` rebuild the CLI.
- **Encodings:** about 220 text reads and writes in `tools/` had no encoding, which on Windows is
  the code page (cp1252), not UTF-8. They all say `encoding="utf-8"` now.
- **Temporary folders:** six defaults were `/tmp/...`; they are the system's temporary folder.
- **LTO:** w64devkit's gcc has no link-time optimisation. The Makefile now turns LTO on only where
  the compiler has it (WSL's gcc does, so nothing changes there).
- **Not fixable here:** two pipeline tests fake the CLI with a POSIX script, which Windows can't
  run directly; they skip on Windows. What they caught there, the backslashed make arguments, has
  its own test now (`test_make_gets_forward_slashes`), which runs everywhere.

The Python suite passes on Windows (102 tests, 5 skipped: those two, two that need FFmpeg, one as
before) and under WSL (102 tests).

## Numbers in published prose come from the evidence

Six publishers wrote counts into their prose by hand. Each now computes them from the evidence it
is given, and each, run on the existing evidence, prints what the page already says:
- **`fixed_transport_checkpoint.py`:** "All 60 primary waveform cases".
- **`junction_checkpoint.py`:** "30 source runs cover 441 complex frequency responses" (each source
  run gives every port's response at every frequency).
- **`thermal_checkpoint.py`:**
  - "gas response times are 0.075–0.30 seconds; wall response times are 16–133 seconds";
  - "all 60 primary ... profiles ... at 768 cells and 768 kHz" (the primary profiles are those at
    the first profile's grid, not the first 60 rows; `check_material.py` counts them the same way
    now);
  - "20.65 %" at 768 cells.
- **`geometry_checkpoint.py`:**
  - "128 of 248 representable wave cases", "40 additional grids", "117–155 Pa";
  - "All 42 connected ... pairs", "all 14 independent-bank pairs".
  - The "14 planted faults" were stage 043's six (in its `faults.log`) and stage 044's eight,
    which only its record mentions. The publisher counts the faults in the plant_bugs logs it is
    given (`--faults`), and says nothing without them.
- **`flow_checkpoint.py`:** the stability cases, temperatures, speeds and carrier frequencies.
- **`audio_experiment_report.py`:** the segments' length (0.75 s), the lift (0.5 s) and the coast
  (8 s), from the clips and their markers.
- **`prepare_user_references.py`:** the 0.7 s gap, from the pairs' manifest (and one `GAP_S` in
  the code instead of six literals).

Declared validity limits ("200–990 K") are design statements, not measurements, and stay as
written.

## The research solvers' tests can fail

- **`test_gas_graph_analytic_pulse`** (new): rb_gg_seed's weak Gaussian pulse (1e-3 of the
  pressure, 6 mm half-width) against linear acoustics, at rest and on ±70 m/s, once it has crossed
  into the delay graph. It checks the shape against the analytic pulse at x0 + (c + U) t, and the
  energy against the pulse's own,
  E = area (A p0)² w √(π/2) / (ρ c²), which is what the graph books for a passing right-going pulse.
  Measured: shape error 3.7–4.9 %, energy 0.964–0.975 E; limits 7 % and 0.94–1.00 E. The energy may
  never exceed E: the coupling must not make energy.
- **`test_gas_graph_fractional_flux_and_clock`:** its energy check was
  `rb_ac_balance + coordinate_work = 0`. That holds by construction, since `coordinate_work` is
  defined as whatever balances it. (It isn't even the energy that crosses the interface: measured,
  it is −0.0008 E; the pulse crosses in the linear tick.) Now the check is physical: the pulse never
  gains energy; it keeps over 0.95 E once it is in the graph; an end that reflects all of it keeps
  98.5 % of that until it returns; and it is gone (under 1e-3 E) once it has left the duct.
- **`test_gas_graph_mean_flux_and_energy`:** its books check stays, with a comment on what it can
  and can't see.
- **`test_junction_closed_source_free_second`:** it required the closed box's disturbance to die
  away to 1e-5 in a second. That is the scheme's numerical damping, which a more accurate scheme
  would rightly have less of. Now it requires the energy never to grow (as before) and total energy
  and mass to stay put.

**Planted bugs** (`tools/bugs/stage119.py`, run natively on Windows against these tests and against
stage 117's):

| Bug | Stage 119's tests | Stage 117's tests |
| --- | --- | --- |
| `first-order-gas`: first-order upwind in the gas region instead of WENO5 (every inventory exact, the pulse smeared) | caught (analytic pulse, fractional flux) | **missed** |
| `graph-sound-speed`: the graph's cells 1 % long | caught (analytic pulse and 4 others) | caught by the inventories |
| `coupling-makes-energy`: 3 % more wave handed to the graph | caught (analytic pulse and 4 others) | caught by the inventories |
| `coupling-loses-energy`: 5 % less | caught (analytic pulse and 4 others) | caught by the inventories |
| `closed-box-amplifies`: the HLLC star pressure's impedance sign flipped | caught | caught |

The inventories (mass, species, momentum) catch the coupling bugs too. The old energy checks
couldn't: a first-order solver, which keeps every inventory exact, passed every stage 117 test.

The Python tests' bugs are in `tools/bugs/stage119_python.py`, which applies each one to a copy of
`tools/` and runs its test (plant_bugs.py builds the C tests only). All six are caught:
- `publish-before-commit`: the old publish, diagnostics written first;
- `mono-fold` and `mono-by-default`: the old fold and the old default;
- `lift-at-row-end`: the old instant;
- `assert-check`: `require` as an `assert`, caught under `python -O`;
- `make-backslashes`: caught on Windows (only Windows paths have backslashes).

## Suites

- **Native:** `make test`, 251 tests and 4,950,067 checks, all passing (one test more; the
  fractional-flux test's by-construction identity checks, 42 of them, gave way to 11 physical ones).
- **Python:** 102 tests under WSL and on Windows. Under WSL, FFmpeg is a snap that can't see
  `/tmp`, so the three tests that run it need `TMPDIR` in the home folder; stage 117's tree fails
  them the same way without it.
- **Web:** `npm test`, 17 of 17 in a copy outside the repo.
- **Code graph:** `graphify update .` (3,538 nodes, 10,149 edges).

No browser work, so no Playwright run.
