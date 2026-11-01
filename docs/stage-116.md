# Stage 116 — Tests that can fail again

**Status:** done (2026-11-01)

Item 2 of Claude's review of Codex's work: the tests whose limits had been loosened until they
could no longer catch what they were written for, and the Python test that failed. Bernardo, after
listening to stage 115: "Good the renders are decent so keep going."

No code in `core/` changed, so the engine and its sound are exactly stage 115's.

## The tests

Each row:
- what the test allowed before;
- what it checks now;
- a bug the new limit catches (`tools/bugs/stage116.py`). The old limit let three of them
  through: the junction's leak was measured (below), and the burn and fuel-cut bugs pass it by
  construction. The limiter bug moves the soft cut's band 120 rpm down, inside the old 150 rpm
  allowance.

The variation test's floor, the first finding in this list, came back in
[stage 115](stage-115.md).

**`test_exhaust_junction_scattering_conservation`** (`tests/test_exhaust.c`): a 10 Pa pulse
scattered by a junction of four pipes.
- **Before:** the energy's books had to close to 2e-7 of the gas's 25 J, which is 5.1e-6 J. The
  pulse itself carries 7.7e-7 J, so a junction could make or lose six times the whole wave and
  pass. The comment blamed the books for the slack, but they close to 1e-14 J.
- **Now:** the books close to 1e-5 of the wave's energy, worked out in the test from the pulse.
- **Bug:** `junction-makes-energy`, a chamber that gains a millionth of the energy flowing
  through it each step. That is 5.8e-10 J here: under the old 5.1e-6 J, and over the new 7.7e-12.

**`test_calibration_burn_and_spark`** (`tests/test_dyno.c`): a wild cam's charge, more than half
of it burned gas.
- **Before:** the burn had to last more than twice the reference and at most the whole cycle,
  720°.
- **Now:** each cylinder's burn is the charge model's for the charge it trapped, to 1e-9, and
  the mean is between 2.5 and 3.3 times the reference. It is 2.83 times, 170°.
- **Bug:** `burn-doubled-reference`, stage 9's cap at twice the reference back (2 × 60° + 1°).

**`test_engine_drives_itself`** (`tests/test_engine.c`): foot off after a blip.
- **Before:** the fuel cut had to come on at least once, with no fuel injected while it was on.
  A cut that let go again passed. So did no cut at all: stage 7's bound on the coast's fuel was
  5 % of an average second's, and a coast with the throttle shut takes 3.6 % with the cut and
  4.4 % without it.
- **Now:** the cut comes on 0.15 s after the lift, `overrun_transition`, to 2 steps. It holds,
  with no fuel injected, until the engine is back below 400 rpm above idle, as the engine
  computer is written to do.
- **Bug:** `fuel-cut-flickers`, the cut switching off every other step.

**`test_presets_run_on_their_own`** (`tests/test_presets.c`): each preset floored into its rev
limiter.
- **Before:** its top had to be above the limit less 150 rpm, which is the whole soft-cut band.
- **Measured:** the presets' tops reach from 6 rpm under the limit (the six, on the soft cut
  alone) to 22 over.
- **Now:** the top is within 50 rpm under the limit.
- **Bug:** `soft-cut-too-early`, the soft cut's band moved 120 rpm down.

**`test_radiation_causal_impedance`** (`tests/test_radiation.c`): two checks there couldn't fail.
- `|R| <= 1` follows from zr > 0, checked on the line before it, so it is gone.
- `zi / x ≈ 0.6133` checks the test's own reference, not the core: at low frequency that
  reference's load is an unflanged pipe's end correction, 0.6133 times its radius (Levine and
  Schwinger 1948). It stays, labelled as what it is.

Every bug is caught by its test (`python3 tools/plant_bugs.py 116 WORKDIR all`):

| Bug | Caught by |
| --- | --- |
| `junction-makes-energy` | `test_exhaust_junction_scattering_conservation` |
| `fuel-cut-flickers` | `test_engine_drives_itself` |
| `burn-doubled-reference` | `test_calibration_burn_and_spark` |
| `soft-cut-too-early` | `test_presets_run_on_their_own` |

## The Python test

`tools/test_render_results.py`'s full pipeline swaps in a fake `rb_cli` and asks make not to remake
it, `--assume-old`. It passed that flag in the `MAKEFLAGS` environment variable, which GNU make 4.3
ignores. So make compiled the whole core into the test's temporary folder, ran into the test's 60 s
limit, and left a stray make rendering in the background (seen in this session). Given more time,
it would have replaced the fake CLI and rendered for real.

`render_checkpoint` (`tools/render_results.py`) now puts `--assume-old` for the CLI on make's
command line. `make render` has built the CLI already by then, so this changes nothing there.

## Results

- **Native tests:** 250 tests and 4,950,077 checks, all pass.
- **Python tools:** 95 of 95 pass; before this stage, 94.
- **WebAssembly and web:** the core didn't change, so `web/src/wasm/` is stage 115's and its
  fingerprint still matches.

## Left for later

From the same review, outside this stage's tests:
- **The research solvers' tests** (`experiments/acoustics`, stages 045–049):
  - `gas_graph`'s accuracy has no test: a first-order version passes all 8 of its tests.
  - Two of its energy checks pass by construction.
  - `test_junction.c:172` asserts numerical damping, so a more accurate solver would fail it.
- **The tools' gates written as `assert`:** they disappear under `python -O`.
