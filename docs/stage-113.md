# Stage 113 — Acoustic derived-state and numerical failure guards

2026-10-23. Baseline `fba9bf04c32b0a09801caf18abd70cabd5b9bcc7`;
user-added `.agents/` and `skills-lock.json` excluded. Repairs C07 from the
[completed-stage review](review-completed-stages-2026-10-23.md). Status: PLAN.md.

## Scope, model and public contract

This is numerical hardening of the native B0 research renderer, not a realism
retune or production promotion. Frozen homogeneous zero-flow linear acoustics,
SI units, source/port signs, timing, scattering and energy equations stay as
documented in [the prototype](stage-S2d.md) and
[coupling ownership decision](adr/0001-coupled-exhaust-ownership.md).
No new acoustic model/research claim is made. The seams authorized by the
reported reproductions are prepare, step/receive/commit and energy/work observers.
They test representability/error behavior, not arbitrary engine operation.

Preparation checks positive finite bulk modulus `rho*c*c`, impedance `rho*c/A`,
admittance, time/energy weights, cavity compliance/memory and radiation
coefficients. Unrepresentable plans return `RB_AC_ARGUMENT`; sample-count query
returns zero. There is no arbitrary minimum engine pipe size or pressure clamp.
The caller keeps the prepared plan immutable and owns storage, as before.

Runtime checks detect nonfinite wave arrivals, pressure, stored pressure,
work/loss books, delay-line squared storage, total energy and balance. A
numerical `RB_AC_STATE` marks the state failed: no further processing until
successful prepare resets it; observers return NaN. Partial progress must not
be retried as another normal tick. Invalid arguments still permit correction
and retry without consuming the current tick. Radiation velocity outside the
shared filter's existing ±1e6 m/s guard is reported as a failed prototype step
instead of silently invoking that filter's reset. Production radiation code
and its existing behavior are unchanged.

Finite arithmetic does not qualify high-amplitude linear acoustics, prove
passivity for arbitrary networks, or close the production gas/graph adapter.
The remaining C01–C06 owners and parent S1/S2/13b gates remain open.

## Frozen checks and execution

The new derived-domain test first failed against baseline for tiny positive
area, reciprocal-admittance and bulk/compliance cases. Ordinary finite controls
remain accepted, including a 1 MPa mathematical input (not physical calibration).
The huge-drive regression first returned success with invalid books; it now
must report failure and require prepare. Additional controls distinguish an
individually representable square from overflow of accumulated delay storage,
and detect the shared radiation filter's protective intervention.

## Results, cost and regression evidence

| Control | Observed repaired behavior |
| --- | --- |
| `area=1e-310 m²` | Prepare rejects with `RB_AC_ARGUMENT`; required samples zero. |
| Tiny density / unrepresentable bulk-compliance | Prepare rejects before installing a state. |
| `wave=1e200 Pa` | Step reports `RB_AC_STATE`, one failure, zero completed steps; further processing rejects until prepare. Failed observers return NaN. |
| `wave=1e154 Pa`, integer delay | Individual square fits; second step detects accumulated storage overflow. |
| High-amplitude pulse at radiation aperture | Existing filter guard is reported as a prototype failure rather than silently reset. |
| Corrected invalid drive on a received tick | Existing receive/commit retry and exact phased-versus-step test still pass. |

**48 ordinary before/after controls** cover transfer/closed loops, 300/900 K,
200/2000/4000 Hz, one/four segments and all-pass/linear interpolation, at
48 kHz, 1 m and 0.3 s. Every non-timing CSV field is **identical** against the
baseline implementation, including carrier magnitude/phase, energy and books.
These supplement the existing cavity, radiation, unequal-branch and passivity
fixtures; they do not extend the original qualified envelope.

The comparison command is `acoustic_candidate_probe wave|loop 48000 T F 1 N I .3`
for each Cartesian-product value above (`I=0` all-pass, `I=1` linear), linking
the same probe/production objects once with baseline B0 and once with repaired
B0. Only wall time, CPU time and CPU real-time factor are excluded from exact
CSV comparison; both source versions and the comparison script are retained.

Shared-host WSL2, Intel Core i7-1355U, GCC 15.2.0,
`-O3 -ffp-contract=off`, no LTO: median CPU cost ratio
**1.215** after/before across these fixtures. Median processing factors are
**207.3x before / 173.0x after real time**. These small ducts include diagnostic
energy checking and are not full-engine callback benchmarks. The roughly 22%
extra cost is explicit; no production engine or audio path runs these changes.
Future integration/performance stages must reassess whole-engine cost.

All **196 native tests / 7897 checks**, **45 Python tests**, **17 web unit tests**
pass. ASan/UBSan passes **18 candidate tests / 2593 checks**. Five Stage 113
deliberate faults are caught: missing derived-impedance guard; hidden work
overflow; reusable failed state; reset/hidden delay-storage overflow; silent
radiation reset. The existing extra-feedback-delay mutation also remains
caught after updating its source-text anchor. No core/WASM change or render
retune is made. All protected files and 48 preview payloads are preserved.

Reproduction from the repository root (use an existing analysis Python for
the complete Python suite):

```sh
make BUILD=/tmp/rev-bench-s2d-next LTO=0 test
PYTHONPATH=tools .venv-analysis/bin/python -m unittest discover -s tools -p 'test_*.py'
make BUILD=/tmp/rev-bench-s2d-next-san SAN=1 LTO=0 /tmp/rev-bench-s2d-next-san/rb_tests
ASAN_OPTIONS=detect_leaks=0 /tmp/rev-bench-s2d-next-san/rb_tests candidate_
python3 tools/plant_bugs.py 113 /tmp/rev-bench-fixes-mutants all
python3 tools/plant_bugs.py S2d0b /tmp/rev-bench-fixes-delay-mutant extra-feedback-delay
```

The full Python reference tests use the same existing FFmpeg environment as
the preceding checkpoint; no reference audio is reprocessed. Web tests run
against matching unchanged sources in the existing cached dependency checkout.
Raw logs, exact source/compiler/host metadata, comparison commands/results,
preservation and browser evidence live locally under ignored
`renders/evidence/fixes-112-113-2026-10-23/`. Graphify is refreshed after source
changes, retaining its known public-header parser limitation.

**Decision:** C07 is repaired within the representability/failure contract.
C01–C06 and their research/integration owners remain open. Save/sync and hold;
Stage 037 and later physical/model/product changes await Bernardo's instruction.
