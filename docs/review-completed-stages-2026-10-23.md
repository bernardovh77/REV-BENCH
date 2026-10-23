# Brief review of completed Stages 001–036

2026-10-23. Baseline `62910fa05bed75cc303a94eb41584d2c7785a9c1`.
Bernardo requests a brief retrospective for mistakes or problems that could
affect future development, followed by a hold. This is a review, not permission
to start Stage 037 or apply fixes. Accepted sound, core/prototype source,
references and Claude setup are preserved.

## Findings

The completed work remains useful within its documented bounds. Its greatest
future risk is carrying an approximation, isolated proof or historical test
result into a broader feature as if it were already validated. Six issue
groups below carry forward known findings; **C07 and C08 are additionally
reproduced implementation gaps** found in this review. Priority P1 means a
gate before dependent physics/production promotion; P2 means a bounded
hardening/calibration/integration obligation, not an observed current crash.

The [original 27-finding audit](sound-physics-audit-2026-10-22.md) and its
[current ownership map](roadmap/audit-coverage.md) remain the detailed source
for the known findings; this review does not relabel those as new discoveries.

| ID | Priority / evidence | Future-facing problem | Required follow-up |
| --- | --- | --- | --- |
| C01 | P1 · existing D01/D06/D07, R02–R04; current source rechecked | Burn-start variation conflates commanded spark and actual ignition; variation zero switches the underlying burn model; injector sound is not driven by actual fuel delivery. Stable RPM does not resolve V8 COV/residual or artificial repetition. | 040–042, 055, 058–059 and listening 063/098. Fix causal events/model continuity before calibrating replacement sound or learned sources. |
| C02 | P1 · retained numerical evidence and current model | Mean torque convergence does not establish acoustic phase/bandwidth. Pipe guards and donor transport alter transients; burned-gas apparent cp uses a 40-bar approximation, not arbitrary composition/pressure chemistry. The refined-network growth failure remains relevant. | 038, 043, 047–052. Separate gas/wall temperatures and thermal/reversal effects from numerical damping; keep independent phase, conservation and intervention checks. |
| C03 | P1 · existing D02–D05/D09, R08/R10–R13; routing source rechecked | Added noise uses different paths/losses from pipe pulses: collector-selected outlet, omitted center/link delays, constant assumed propagation speed and unconditional catalyst attenuation. Choked-source conditions and listener interpolation also need correction. A better pipe alone cannot fix those layers. | 044, 049–056 and integrated 057–059. One physical routing/energy contract, supported components and separately validated listener transfer. |
| C04 | P2 · reference/exhaust/rollback records | Recordings have unmatched hardware, operating state and perspective; some supplied I4/I6 examples are turbocharged while base models are NA. Estimated exhaust internals and raised RPM limits are not calibrated components or durability evidence. Legacy mono folding can cancel stereo measurement energy. | 039, 047, 059, 087, 094. Keep raw stereo, match references where possible, label uncertainty and seek targeted NA-I6/boxer material. |
| C05 | P1 · retained experiment/cost evidence | Initial conservative A1 fails acoustic accuracy. B0 is frozen/quiescent; the newer open duct is a different implementation and is not the production-mixture/engine-to-B0 adapter. Its sampled leaky-domain growth screen does not prove trapped-loop stability. Neither demonstrates deployed real-time delivery. | 043–063, 069/077/099. Promote only surviving qualified artifacts; retain all common physics gates. Previous 39.4 ms for 2.33 ms of modeled duct time is a correctness-prototype measurement, not a product speedup. |
| C06 | P2 · current API/worklet and roadmap | The browser remains tone/mono, despite existing core/TS engine/stereo bindings. Scalar descriptions and pipe indices are insufficient for atomic builds, stable spatial identity and coherent 3D snapshots. Rebuilds reset engine/listener state and detach the dyno; that needs explicit transactions. | 037 then 066–077/095. Specify actual/unwrapped event time, generations, physical versus presentation delay and truthful 0D/1D visualizations before product implementation. |
| C07 | P2 · **new C reproduction** | B0 validates finite inputs but not every derived coefficient or energy/work quantity. An accepted `wave=1e200 Pa` returns `RB_AC_OK` with infinite energy/work and zero failures. An accepted positive `area=1e-310 m²` produces infinite impedance. | Extend prepared-state/domain and finite-book guards in 037/038/045 before general adapter use. These deliberately extreme inputs are outside credible engine operation; the finding does not invalidate the ordinary qualified B0 cases or show a current-render failure. |
| C08 | P2 · **new temporary-directory fault injection** | The section publisher overwrites a registered fragment before entering its rollback `try`. A partial write plus I/O exception preserves the old playable page/manifest but leaves a damaged fragment, so the next composition fails its hash check. | Bounded Stage 033 follow-up before the next affected sound publication. Define/install/recover the fragment and manifest transaction together; allocate the implementation checkpoint when development resumes. No fix or new execution stage is started here. |

### Exact new reproduction results

For C07, a standalone native probe uses the current `wave.c` and production
`radiation.c` with a normal two-boundary duct:

| Input | Prepare / step | Finite impedance | Finite energy / source work | Failures |
| --- | --- | --- | --- | --- |
| area .001 m², drive 1 Pa | OK / OK | yes | yes / yes | 0 |
| area .001 m², drive 1e200 Pa | OK / OK | yes | **no / no** | 0 |
| area 1e-310 m², drive 1 Pa | OK / OK | **no** | yes / yes | 0 |

Relevant paths: `experiments/acoustics/wave.c:116,132,215,235–241`.
Input-domain rejection and finite derived books are different requirements.
Do not broaden this result into a claim that normal engine pressures overflow.

For C08, a temporary registered checkpoint is published successfully, then the
next `audio-experiment` fragment write is deliberately truncated and raises
`OSError`. Observed: page preserved **true**, manifest preserved **true**,
fragment preserved **false**, subsequent rebuild succeeds **false**.
`tools/checkpoint_sections.py:145` performs that write before the `try` below it.
The real `renders/RESULTS.html` and registered assets are never fault-injected.

## Coverage of every completed stage

“No additional issue” below means none identified in this bounded review,
not a new full scientific certification. Historical reports remain the
evidence for their original tests; unresolved parent acceptance stays open.

| Stage | Scope reviewed | Assessment / issue owner |
| --- | --- | --- |
| 001 | Research/workspace | Keep source/assumption provenance and current setup distinct from historical tool notes; no additional defect identified. |
| 002 | Native scaffold/audio files | Container/test foundation retained; later output preservation/transactions need C08. |
| 003 | WASM/worklet tone scaffold | Correctly bounded as tone, not engine application; C06. |
| 004 | Gas/chamber/orifice primitives | Original analytic controls are useful; property domain and protective interventions need C02. |
| 005 | Slider-crank/motoring | No additional kinematics defect identified; later visual transforms must use these conventions. |
| 006 | Valves/throttle/plenum | Quasi-steady/stiff-boundary limits must survive new solver comparison; C02. |
| 007 | Baseline combustion/heat | C01/C02: ignition and effective-property assumptions are not future knock/chemistry proof. |
| 008 | Free crank/controller | Retain energy/start/idle controls; engine-state and event transactions remain C01/C06. |
| 009 | Five layouts/description API | Existing bindings should be reused; spatial/semantic identity and atomic edits need C06. |
| 010 | Native dyno/calibration | Native calibration is not a browser dyno or arbitrary modified-engine validation; C04/C06. |
| 011 | Original 1D pipes | Main phase/convergence/guard obligation C02; successful mean breathing alone is insufficient. |
| 012 | Extraction/filter chain | Separate radiation load, audible output and digital interpolation response; C03. |
| 013 | Mufflers/intake/listener | Existing effective components need physical/calibrated impedance and field limits; C03/C04. |
| 014 | Initial optimization/quality/fuzz | Old speed/fuzz success does not qualify new geometry or current runtime; C02/C05. |
| 015 | Noise/structure/cabin | Largest source-path/event mismatch group: C01/C03. |
| 016 | Variation/limiter/decel | Implemented portion exists; robotic coast and repetition remain C01/C02/C04. |
| 017 | Float analysis/paths | Raw-float handling retained; stereo versus mono analysis remains C04. |
| 018 | Finite burns/EVO cutoff | Bounded conservation fix retained; ignition/late chemistry remains C01. |
| 019 | Repetition/COV audit | Correctly complete as an audit; its discovered failures remain C01. |
| 020 | Trapped-mixture feedback | Useful causal input, but variation/model discontinuity and out-of-fit residuals remain C01. |
| 021 | Raw reference catalog | Provenance retained; matched calibration gaps C04. |
| 022 | Filtered references | Correct listening-only distinction; filtered copies must not become measurement targets, C04. |
| 023 | Loaded-detail/lift correction | A limited reconstruction fix, not full transport or decel diagnosis; C01/C02/C03. |
| 024 | Native stereo scenes | Native implementation is not browser stereo/listener qualification; C03/C06. |
| 025 | 27-finding audit | Audit remains useful and mapped; completion does not mean its findings were fixed. |
| 026 | SIMD/batched thermodynamics | Bounded identity evidence retained; runtime and solver-convergence obligations C02/C05 remain. |
| 027 | Model-specific exhausts | Accepted tonal improvement retained; estimated dimensions/components and unequal layer routing remain C03/C04. |
| 028 | High-RPM changes/rollback | Current selection retained; raised revs are research assumptions, C04. |
| 029 | Source/space/time experiment | Independent controls remain useful; failures and attribution limits feed C02/C05. |
| 030 | Architecture specification | Sound direction, but proposed thresholds are not achieved engine acceptance; C05/C06. |
| 031 | Conservative prototype | Completed experiment with a rejected tested configuration; C05. |
| 032 | Quiescent acoustic graph | Ordinary fixture evidence retained; C05 and newly reproduced C07. |
| 033 | Complete results publisher | Normal rebuild/preservation controls pass; additional interrupted-write gap C08. |
| 034 | Receive/commit API | Current-tick behavior remains verified; finite derived-state/error contracts need C07 before broader use. |
| 035 | Open coupled duct | Valid bounded proof; gas/graph/thermal/runtime generalization remains C05. |
| 036 | Renumbered roadmap/protocol | All completed scopes and known obligations are mapped; retain explicit rejected-arm versus qualified-artifact semantics and the new C07/C08 follow-ups. |

## Checks, evidence and hold

Fresh checks are limited to the two reproductions above and targeted existing
candidate/coupling (**5 tests / 2957 checks**) and publisher (**8 tests**), all
passing. The native spot checks use the previous checkpoint's binary against
unchanged C sources; the C guard probe is compiled from the current sources.
The preceding global native/web/audio
test results are historical; this is not a full-suite rerun or a new listening
checkpoint. Graphify was queried for navigation; no production or prototype
source change requires an AST rebuild.

Reproduction sources, outputs, exact source hashes and focused test logs are
kept locally under ignored `renders/evidence/completed-review-2026-10-23/`.
The report includes the meaningful results above so a clean checkout does not
need those local files just to understand the conclusions. The C fixture can
be rebuilt with `gcc -std=c11 -O2 -Iexperiments/acoustics -Icore/src PROBE.c
experiments/acoustics/wave.c core/src/radiation.c -lm -o PROBE`.

**Hold for Bernardo's instruction.** No issue is repaired, no stage is started,
and no additional renders/reference processing are produced. Stage 037 remains
the planned next development item; C08 needs a separately bounded publication
follow-up before the next affected publishing operation.
