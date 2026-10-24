# Stage 038 — Numerical guard and flux observability

Date: 2026-10-23. Dependency: 037. Execution authorized by Bernardo: review037,
then038. Scope: observe the existing native gas solver after successful preparation and detect hidden
interventions. No acoustic retuning, new propagation model, production browser
controls or completion of S1/S1e/S2/S4/full13b is claimed.

## Predecessor review

Stage037's bounded contracts are suitable for this step. Two errors were fixed:
negative/fractional native kind/place codes could be coerced into valid enums;
`prepare_build` discarded accepted annotations, transforms and centerlines.
Native enum codes now require exact nonnegative integers in range; preparation
returns an independent copy of the validated original build. Thirteen contract
unit tests and the five-preset native preparation matrix exercise these fixes.
This does not implement the future production geometry compiler or event API.

## Model, ownership and limits

Baseline: `e608b36b91a4094c4de4bccda5e3700019c4c100`, initially clean tracked
files; user-added `.agents/` and `skills-lock.json` excluded. Native GCC15.2.0,
`-O3 -ffp-contract=off`, no LTO; 192kHz engine/48kHz pipes. Fixed seed and accepted
preset geometry. Conservation follows finite-volume interface cancellation;
see [LeVeque's primary text/materials](https://www.clawpack.org/fvmhp_materials/).
That principle motivates independent books, not a claim this staggered solver
is already a production conservative Euler solver.

Each physical face/orifice owns one signed transfer: positive a→b. Slots are
identified by semantic role, component number, kind and cell/face part, without
address or engine pipe-array identity. Face parts -1/-2 identify inlet/outlet.
Role/number is a native diagnostic identity; later contract adapters must map
it explicitly to persisted build IDs. Unrelated runner omission cannot renumber
exhaust roles. Report quantities are kg, species kg and transported total
enthalpy (including donor kinetic energy) J, before/after transport caps.
`rejected_abs` is foregone transfer, not energy destroyed inside the engine.

Momentum entries are **staggered-column impulses N·s**, requested after pressure,
advective and drag forces but before the sonic/reset guard, and actually
accepted. Δmdot×effectiveLength supplies that impulse. Radiation correction is
an additional incremental impulse relative to the preceding push. The existing
0D junction stores mass/species/internal energy, not vector momentum; it has no
invented globally conservative momentum account. This exposes that architectural
limit for043. Global total energy includes pipe kinetic energy, piston work,
chemical heat, fuel sensible energy, wall heat and radiation work.

Requested/accepted movement, numerical caps, physical nozzle choking, donor/
limited reconstruction, density/length floors, invalid flow/heat/radiation resets,
mass/energy repairs, temperature floors, property-range excursions and scalar/
Newton fallbacks are separate reasons. Property extrapolation outside200–4000K
is the existing gas model's extension, not newly calibrated chemistry. Nonfinite
repair deltas increment `invalid`; they cannot truthfully be assigned finite J.
Temperature floors retain the existing stored-energy policy; a counted floor
is not proof of thermodynamic consistency. Zero repairs never means zero guards.

## Lifetime and bounded cost contract

`core/src/diagnostics.h` is an internal optional caller-owned native interface;
public rb.h/TS exports are unchanged. Prepare the engine first, initialize a
context and bind it at a quiescent boundary. No allocation, file I/O, locks or
unbounded loops occur during stepping. Capacity is checked before binding;
insufficient capacity leaves an existing binding intact. Reinitializing/rebuilding
an engine/chamber/radiation object clears its taps: rebind and advance the
caller's generation. Do not copy a self-referential engine or mutate/bind taps
concurrently with stepping. NULL unbinds all taps.

At most2048slots and256trace records; all events accumulate counters even when
trace capacity is zero/full. A full trace drops new records and increments
`dropped`, never overwrites old evidence or blocks stepping. Stride0 captures
guard events; N also samples ordinary events. Callers must drain/rearm between
bounded stepping blocks for a long trace. Aggregate counters remain authoritative
when the initial trace saturates. Records use binding-relative physics tick and slot index; the caller must
record the bind-time offset for an engine already running. This is not yet the
actual-event clock producer planned in040–042.
Context generation and pipe phase belong to the enclosing report. A zero-valued
trace field after a nonfinite input is invalid data if its slot's invalid counter
is positive; it is not evidence of zero physical impact.

Prepared geometry records physical length/area, actual dx/cell count and pre-pool
cell request. Dynamic faces record greatest effective momentum length and lowest
used density; pipes record maximum `(c+|u|) dt/dx` on the rate actually stepped.
Pool requests already obey per-pipe nominal timestep/cell limits; the separately
reported actual count exposes additional engine-pool coarsening. This is
observability, not mesh convergence or proof that every CFL<1 state is accurate.

## Gates and reproduction

Tolerances derive from existing native books, not audio matching: over a held
3000rpm .7→.01 throttle fixture, mass and each species error <2e-11×initial mass;
energy error <2e-9×transport/heat energy scale. Independently accumulated external
face taps must agree with engine books at complete pipe steps (radiation work
removed from the transported boundary-energy account). All five presets must
have exactly identical observed/unobserved final physical states and no
unquantifiable values. These short cold-state stress fixtures do not establish
a warm engine's full operating envelope or calibrated lift-off realism.

Adverse tests force reversal, empty donors, vacuum, length/density floors,
transport and sonic caps, corrupt junction species/energy, temperature floor,
physical choke/equalisation, extreme radiation input and bounded-capacity failure.
An independent small pressure drop verifies impulse `A Δp dt`, avoiding a test
that merely repeats the instrumented result. Existing whole-engine tests retain
full mass/species/energy books, now counting junction repairs too. Fuzz explicitly
inspects junction state and repaired chambers.

```sh
make BUILD=/tmp/rev-bench-s2d-next LTO=0 all \
  /tmp/rev-bench-s2d-next/rb_tests \
  /tmp/rev-bench-s2d-next/numerical_diagnostics_probe build-contract-check
/tmp/rev-bench-s2d-next/rb_tests
python3 tools/check_numerical_diagnostics.py \
  --probe /tmp/rev-bench-s2d-next/numerical_diagnostics_probe \
  --output renders/evidence/stage-038/measurements.json
```

Complete float-regression commands (the comparison folder is temporary evidence):

```sh
make render-standard BUILD=/tmp/rev-bench-s2d-next LTO=0 \
  RENDER_DIR=renders/evidence/stage-038/sound/standard RENDER_FLAGS=--float
python3 -S tools/exhaust_feedback.py --cli /tmp/rev-bench-s2d-next/rb_cli \
  --output renders/evidence/stage-038/sound/feedback
python3 -S tools/render_results.py --publish-only
```

Detailed measurements, full per-component records, command/build/test logs,
source hashes, fault results and preservation/render comparisons live in ignored
`renders/evidence/stage-038/`. Essential numerical results are summarized below.

## Measured numerical results

Five cold held-crank stress cases:0.25s at3000rpm, throttle .7→.01 halfway;
three CPU repetitions per monitoring mode. All mass/species and energy gates
pass. External diagnostic face books independently match the engine's books.
No unquantifiable values, species/energy repairs, invalid flow/heat/radiation
resets, pipe sonic/mass caps, density/length floors or CFL>1 occur in these
ordinary cases. Adverse fixtures separately force and detect those interventions.

| Preset | Slots | Cells requested/actual | Max runtime CFL | Mass residual kg | Total energy residual J |
| --- | --- | --- | --- | --- | --- |
| I4 |269|111/111|0.466|1.24e-16|-3.35e-10|
| I6 |412|170/170|0.507|2.54e-16|2.10e-10|
| Cross V8 |539|226/226|0.509|-9.69e-17|-4.00e-10|
| Flat V8 |454|188/188|0.418|2.16e-16|8.94e-10|
| Boxer4 |306|127/127|0.470|1.57e-16|-1.22e-10|

Zero repairs does hide substantial intervention in these fixtures:

| Preset | Equalisation caps | Scalar fallbacks | Property extrapolations |
| --- | --- | --- | --- |
| I4 |26915|138717|2089|
| I6 |30718|93532|2159|
| Cross V8 |55703|287057|2017|
| Flat V8 |76859|267006|2043|
| Boxer4 |22809|139976|2145|

These are call/event counts, not independent engine cycles or probabilities.
Scalar fallback restarts the existing gas inversion; it is not necessarily a
repair. The counts expose where040/043/047 and qualification must investigate;
this stage does not abolish protective caps or certify their acoustic neutrality.
Native paired runs preserve theta/heat/random cycle state at every tick and
all pipe p/U plus junction U at the endpoint. Cross-mode probe final states are
identical; native/WASM regression checks cover the same optional binding path.

## Cost and memory

Intel i7-1355U,12 logical CPUs, WSL/Linux, GCC15.2.0 `-O3`, LTO disabled;
CPU time excludes report serialization. Development runs shared the machine
with rendering and validation; these are observed costs, not deadline or worst-
case guarantees. Median of three repetitions:

| Preset | Aggregate taps / disabled | Bounded trace / disabled | Default renderer / baseline |
| --- | --- | --- | --- |
| I4 |1.595×|1.575×|1.036×|
| I6 |1.500×|1.491×|1.076×|
| Cross V8 |1.583×|1.399×|1.044×|
| Flat V8 |1.496×|1.616×|1.030×|
| Boxer4 |1.547×|1.529×|1.074×|

Trace ratios below aggregate ratios reflect timing variability, not a claim
tracing improves performance. An earlier concurrent sample had greater jitter;
the final three-repeat runs and all raw costs are retained. Default renderer
comparisons alternate old/new order over three1s held3000rpm recordings per
preset; all15 old/new float pairs are byte-identical. Default-path overhead was
3.0–7.6% in this test. Aggregate monitoring adds roughly50–60% engine CPU cost;
keep it opt-in and batch/drain diagnostic work off the audio callback. No
real-time promotion or hardware tier is established here.

Actual slot storage128–257KiB plus88B context; trace payload max26KiB. The
caller can allocate exactly the prepared slot count; the declared2048-slot
maximum is976KiB. All traces saturate at256 records while dropped-record counts
continue; they retain the earliest events, not an unbiased whole-run sample.

## Qualification and handoff

**Outcome: complete for the stated numerical-observation scope.** Successful
coverage: complete native suite200tests/38789checks
plus two added cases/25checks (202distinct tests/38814checks combined); affected
sanitizer9tests/31051checks; WASM15tests/31227checks; Python58 and web17. The
WASM test target now includes experiment sources/headers needed by the complete
native test runner, repairing its stale build recipe. The public WASM core is
rebuilt with unchanged exported API; its source fingerprint matches.

Six compiled, isolated faulty copies all fail behavioral checks: hide junction
repair, omit junction inspection, hide sonic cap, overwrite requested flow with
accepted flow, use incorrect impulse units, and hide radiation reset. Active
sources are never mutated. No all-fault or full operating-envelope claim.

Accepted audio/reference originals and Claude setup remain protected. The
single `renders/RESULTS.html` stays the listening checkpoint, including all long
coasts/research sections. Stage039 matched-reference work is the next default
after038; no later stage starts automatically. Undoing this checkpoint should
remove the diagnostic hooks as a unit and rebuild native/WASM artifacts; do not
mix objects compiled against different internal structure layouts.

The complete34-recording float checkpoint (337.248s), including all five long
coasts and the packed cross-V8 control, is byte-identical to accepted originals.
All engine renders report zero limited/nonfinite samples. The rebuilt single
results page passes offline playback for48players, exclusive playback, four
jump buttons and eight plots; all48 embedded audio payloads remain identical.
All116 protected audio/reference/Claude files match the prior checkpoint.
Verified duplicate comparison WAVs are removed after preserving hashes, logs,
CSV traces and reports; accepted measurement originals remain in `renders/current`.

Graphify was queried before exploration and refreshed after implementation:
2657nodes/7206edges/153communities. The known rb.h:415 parse warning remains;
AST updates do not claim semantic paper/document extraction. Native build has
the existing CLI long-string portability warning. An initial scratch link ran
while a new test file was still being created; the complete rebuild includes it
and passes. The first sandboxed Windows compiler invocation could not access
interop; the existing compiler succeeded through the approved execution path.
Neither event is a failed physics acceptance result or a relaxed tolerance.

Checkpoint after commit/sync. Parent S1/S1e/S2/S4/full13b, physical model
qualification, source causality and actual browser-engine/3D integration remain
open at their canonical stages. No listening retune or reference reprocessing
was needed for these byte-identical results.
