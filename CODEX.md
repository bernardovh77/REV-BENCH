# REV-BENCH - Codex handoff

Read PLAN.md, CLAUDE.md and the latest stage notes before continuing. Claude's
instructions, hooks and settings remain active; this file supplements them.
CLAUDE.md and .claude/ have been preserved.

## 2026-10-23: next checkpoint implemented — bounded open coupling proof

- Bernardo authorizes completing the next step, split as needed. Three bounded
  substages are complete: S2d-R publication, S2d-0b current-tick interface and
  S2d-B1a homogeneous open gas/characteristic duct proof. Full method, evidence,
  reproduction and limits: docs/stage-S2d-0b-B1a.md. Parent S2d-0/S2d/S1/full13b
  remain open; stage14 remains unstarted. Accepted engine sounds are preserved.
- Publication now uses tools/checkpoint_sections.py: hashed local sections and
  diagnostic inventory in ignored renders/checkpoint/manifest.json. All three
  research/reference producers share the composer. The one-time migration
  --adopt-current is already done. Ordinary --publish-only rebuilds all48
  players; fresh accepted renders carry42 registered diagnostic WAVs before
  atomic replacement. Unknown, missing or changed inputs fail before install.
- B0 wave.[ch] exposes receive/incident/commit so current returning pressure is
  available before the caller solves its boundary; no extra tick is inserted.
  Invalid drives can be retried without consuming storage twice. Legacy step
  matches the phased path exactly in the fractional-delay control. Source
  export into the graph is negative rb_ac_node_flow, per its port orientation.
- New experiments/acoustics/coupled_duct.[ch] retains one physical conserved
  state per cell: rho, momentum, total energy and passive tagged density.
  MC/RK2 gas-region stages share the same accepted flux with a characteristic
  streaming region. Conservative nonlinear/contact/entropy corrections remain
  spatially resolved downstream. No duplicate acoustic energy or added p'Q.
  Physical pressure subtracts kinetic energy; characteristic a+b does not.
- This is a constant-gamma, sample-aligned, homogeneous reference duct with
  explicit capacity/grid/state/CFL failure. It is NOT an engine adapter or
  arbitrary B0 multiport coupling. Production mixture EOS, compiled complete
  exhausts, moving backgrounds, heat/reversal, real components and deployed
  engine runtime/listening validation remain required.
- Fifteen physical cases pass: two interface positions plus all-FV control at
  three amplitudes, negative/matched/positive reflected-load controls and
  three one-second source-free screens. These have a matched left end, not
  trapped closed-loop growth tests. Worst split pressure waveform error1.30%,
  return-peak error2.5ticks, per-quantity relative region balance4.75e-12.
  Moving tagged contacts pass both directions against analytic advection.
- Whole-fixture median costs39.4/69.8/83.7ms for2.33ms modeled time at
  cuts168/280/560: below real time. No production speedup or higher hardware
  requirement is justified. Source-free energy and conserved books are
  separate checks. An initially rejected mixed-unit absolute-balance screen
  and80-bit roundoff control are retained, not hidden; final gate uses
  independently scaled kg/Ns/J/tagged-kg residuals.
- Validation: native193tests/7766checks, Python42/no skips, web17,
  ASan/UBSan18candidate/coupling tests/3294checks. All six deliberate faults
  are caught: stale return, added delay, unmatched flux, duplicate energy,
  lost tagged advection, kinetic energy misreported as pressure. Existing
  S2d wrong-return-sign mutation anchor updated and that individual control
  is caught; the older full matrix was not rerun. No core/WASM source changed.
- Offline result: all48players, exclusive playback, four jump buttons,
  two new/six retained plots pass with no network or page errors. All158
  preserved file hashes and48audio payloads match the starting snapshot.
  Ignored evidence: renders/evidence/coupling-proof-2026-10-23/, including
  raw fields/traces, source hashes, rejected screen, precision control,
  costs, validation logs and browser preview. Measurement sources match.
- Graphify AST refreshed:2298nodes/6295edges/126communities, knownrb.h:415
  warning retained; no semantic-document extraction claimed. User-added
  .agents/ and skills-lock.json remain untouched and unstaged. Claude setup
  remains active. Next dependent work: production-mixture/graph port adapter
  and complete geometry/cap observability, then hot/reversing/multiport
  coupling controls before changing engine sound.

## 2026-10-23: grill-with-docs review — next checkpoint agreed

- User requests analysis of progress, production/prototype structure and next
  step. docs/review-acoustic-architecture-2026-10-23.md records evidence,
  confirmed structural findings and the agreed next checkpoint. No source,
  physics, audio, preset or Claude setup changes.
- The explicitly invoked wrapper requires grilling/domain-modeling. Companion
  skills were absent locally and read from mattpocock/skills upstream. Their
  fact-finding instruction authorized read-only agent reviews; their interview
  requires waiting for scope answers before dependent implementation. Bernardo
  answered yes to both recommendations: interface/coupling-first physics scope
  and a complete-results-rebuild repair before the next audio checkpoint.
- B0's current drive-first monolithic step cannot expose this tick's returning
  waves for an external gas solve without an API change or duplicated internals.
  Its one-port flow sign also needs orientation at the gas boundary. These
  are integration hazards, not failures of the current static B0 fixtures.
- Geometry remains a snapshot; changing background gas and conservative
  coupling/guard observability remain unimplemented. A1's current rejection
  applies to that tested configuration. Production/native stereo and browser
  mono have separate validation/runtime requirements. The browser page remains
  the stage2 test-tone scaffold; interactive engine integration is stillstage14,
  unstarted. No learned engine exists.
- Confirmed publication regression introduced by accumulated experiment files:
  renders/current contains76WAVs; write_page expects34 and rejects42extra.
  Reproduced failure before writes; RESULTS.html unchanged. Three append
  publishers plus the replacing standard render do not provide a complete
  one-command48-player rebuild. Proposed fix: one declared checkpoint inventory
  and atomic composition; do not delete accepted/reference/evidence assets.
- Retained S2d344rows/18exclusions, aggregate CSVs and47source hashes match.
  Existing test counts are historical validation; no full suites/experiments
  rerun for this documentation review. Graphify queried; AST topology unchanged.
- GLOSSARY.md records resolved terms only. docs/adr/0001-coupled-exhaust-ownership.md
  records accepted prototype scope, not a production replacement. This review
  does not implement the agreed next checkpoint. Parent sound/13b stages
  remain open. Preserve user-added .agents/ and skills-lock.json without staging.

## 2026-10-23: additional rear cross V8 references — hold after import

- Bernardo supplies rcf.lorax / DeztBYHSkCj and viper_ws9 / DewsrGHTcOZ
  as cross-plane V8 references. Both are recorded behind the car at roughly
  3 m from the tailpipes according to him. Preserve this user-reported context;
  exact geometry, vehicle/exhaust, motion and recording calibration are unknown.
- Source definitions: tools/rear_v8_reference_sources.json. Full decoded clips
  are approximately 52.013 s and 15.186 s, registered in the existing user
  reference catalog as user-reported-rear-3m. Unfiltered 48-kHz stereo float
  measurement WAVs remain source-level; do not fold channels before measuring
  power. Legacy compare_real.py still folds to mono; use audio_wav and the S2c
  per-channel analysis for these stereo references.
- Light listening processing uses the existing 20-Hz high-pass / 6-dB FFT
  noise-reduction chain and 10-ms end fades, without distance EQ/gain or pitch
  correction. Filtered copies and raw-then-filtered pairs are local under
  renders/user-reference-previews/rear-v8/; pairs are embedded in RESULTS.html.
  Each pair shares one fixed gain; source-to-source levels are uncalibrated.
- docs/user-sound-references.md records intervals, provenance, caveats,
  reproduction and validation. No new speech/isolation claim. Preserve original
  clips, previous references, accepted renders, core/WASM and Claude's setup.
- Hold for Bernardo's next instruction after this reference checkpoint.
  Do not continue acoustic development or close S1/S2d/full 13b acceptance.
- Validation: native189/4820, Python38 without skips, web17; all48 offline
  players, exclusive playback, four original seek buttons and six prior plots
  pass. Both measurement files equal direct source decode; filtered frame counts
  match. All108 older hashed files and46 embedded previews remain unchanged.
  Graphify AST refreshed to2159nodes/5933edges/112communities, knownrb.h:415
  limit. No semantic extraction is claimed. FFT end flushing now preserves
  fractional clip endings; the added integration check exercises this case.

## 2026-10-23: S2d initial native experiments and development

- Bernardo authorizes starting development from the design. Actual modules,
  limits/results/reproduction: docs/stage-S2d.md and
  experiments/acoustics/README.md. Parent S2d remains open. Initial homogeneous
  B0 fixtures are complete; S2d-0 and conservative A1 work remain partial.
- New preallocated native research modules: wave.[ch] (bidirectional graph,
  weighted junctions/cavity compliance, sources, radiation, loss/storage/work
  books) and euler.[ch] (constant-gamma HLLC/HLLE, MC, SSP-RK2, tagged gas).
  They link only into tests/probe, not production rb_cli/public API/WASM.
- Matrix: 124 FV rows with18 explicit setup exclusions, 220 graph rows.
  Initial A1 conserves total quantities to~1e-15 but is too dissipative for
  upper-band sound: at300K/zero-flow/2kHz/9.375mm only56.2% perturbation energy
  remains after2.5ms. Reject that configuration as an acoustic upgrade.
- B0 all-pass meets the tested quiescent1m/200–4000Hz screen: magnitude error
  below.001dB at reporting precision, worst phase4.189deg. One-second closed
  loops show no growth; linear interpolation's loss is measured/booked.
  Higher-band phase, transverse modes, mean flow, thermal fronts, real
  components and nonlinear reflected-engine coupling remain unvalidated.
- Five engine snapshots resolve all connections. Semantic component IDs survive
  intake-runner removal; legacy pipe positions are not physical identities.
  Historical A0 data match all41 current core C/header source hashes.
  Snapshot is not a complete acoustic compiler/full production cap trace.
- Production stereo baseline is still~.56x real time in this host. Separate
  instrumented profile points to gas/property/pipe costs; its overhead/self
  percentages are not a promised hybrid speedup. No engine audio is retuned.
- Native189/4820, Python37 without skips, web17 and candidate ASan/UBSan14/348
  pass. Nine deliberate physics/units faults are caught. Details and final
  publication/preservation/graph checks are in the ignored checkpoint evidence.
- RESULTS.html retains every accepted preview and long coast and adds two
  prototype plots. No new engine render, learned model, reference processing,
  core/WASM/preset or Claude setup changes. Do not close parent sound/full13b
  acceptance or start stage14. Next: complete interface observability/contracts,
  then common-clock nonlinear/load coupling and temperature/reversal controls.
- Independent z-domain transfer checks and1025-point analytic sweeps/18 static
  configurations agree with the native responses. This does not add mean-flow
  validation. Offline browser check passes on retry: all46clips, exclusive play,
  four original seek buttons and two new/four previous plots. All76 current
  WAVs and46preview payloads remain byte-identical.
- Graphify AST refreshed:2155nodes/5918edges/112communities; known rb.h:415
  limit retained. No semantic-document extraction or external training run.

## 2026-10-23: S2d implementation specification

- Bernardo requests a concrete technical design for improved conservative 1D
  versus coupled gas/acoustic rendering and engine-specific learned synthesis.
  Specification: docs/stage-S2d-design.md. Architecture sketches, equations,
  proposed C module/API boundaries, experiment arms and small implementation
  steps are documented; none is presented as an implemented prototype.
- A0 remains the accepted production control. A1 starts as an isolated coupled-
  variable conservative duct solver. B0 starts with a quiescent wave/scattering
  graph; B1 returns its load to a retained nonlinear gas region using one
  conservative boundary flux. Mean advection/thermal/species transport cannot
  disappear when acoustic propagation changes. Shared geometry owns all pulse/
  unresolved-source paths and exactly one radiation load.
- Mean-flow energy, entropy/front transport, coefficient-change work and
  short-component/direct-feedthrough solves are explicit gates. Test the known
  hot/refined growth and partition failures; separate temperature, reversal
  and mixing before assigning the I6 coast rise to numerical damping.
- L0 pulse and learned order/noise gain comparators remain outside physical
  feedback. Prefer offline fitting/export to conventional C DSP. Published
  16-kHz/synthetic limits do not establish 48-kHz or unseen-exhaust validity.
  Conditional L1 fits identified unresolved sources through the validated
  geometry graph; no duplicate resolved combustion pulses/resonance filters.
- Cost: current native stereo baseline is below real time. Adding a graph to
  all existing downstream gas work cannot establish a performance gain. Profile
  first; replace only validated degrees of freedom. Existing callback target
  remains proposed/unachieved; stronger hardware needs measured realism benefit.
- Current checkpoint edits documentation only. Graphify queried for coupling
  sites; source topology is unchanged, so no AST rebuild is needed. No new
  audio/model/training/core/WASM/preset/Claude changes. S2d/S2e/S6 and parent
  sound/13b acceptance stay open; stage14 unstarted. Next entry: S2d-0.
- Documentation links/fences/whitespace and 1,000 static-junction equation
  checks pass; RESULTS.html hash matches S2c. These are design checks, not
  prototype acceptance. Native/web suites were not rerun for documentation.

## 2026-10-23: S2c experiment and sound-generation roadmap

- Bernardo authorizes the experiment, results and a robust method/plan, then
  explicitly requests research into changing sound generation. Complete
  evidence/method: docs/stage-S2c.md. Future stage gates:
  docs/sound-robustness-plan.md; primary sources and all 27 audit mappings:
  docs/audio-robustness-research.md. Architecture comparison:
  docs/sound-generation-architecture-review.md.
- Transport: 312 matrix configurations, 241 guarded/71 spatial exclusions.
  Fine space reduces phase/loss; time alone can worsen energy retention.
  At zero flow 2 kHz, 37.5→9.375 mm improves phase134.65→8.30 degrees despite
  ~100% perturbation energy throughout. Independent discrete dispersion
  predicts the errors. Refined hot-flow growth is scoped to the tested
  segmented network; partition sensitivity leaves interface/bulk attribution open.
- Source matrix: 30 trajectories/60 captures, three engines/two RPMs/five
  independent or combined rate/grid controls. Baseline mono production identity
  is exactly0 in six checks; simultaneous stems reconstruct within ~1e-15.
  Engine-rate doubling changes upper-band fraction≤.15dB; finer space at fixed
  pipe time changes I6/cross/flat by+5.93/.85/5.29dB. These are spectral changes,
  not listening realism acceptance. All repairs/limiter counts0.
- Held injection-off is not ECU overrun; residual heat and synthesized injector
  clicks are documented. Three additional ten-second actual-ECU coasts trace
  lift .5s, fuel cut .65s, clutch8.5s. I6 early coast rises6.06dB, dominated by
  pipe pulses while added flow noise falls. No idle checkpoint by10s is claimed.
- Actual early-coast tail end-cell gas T drops848→319/904→312/974→316K;
  this is local near-mouth gas, not wall/whole-exhaust cooling. Boundary
  entrainment, transient thermal behavior and numerical mixing need separation;
  do not attribute the transition to mean-flow damping alone.
- Native production-stereo cost control (cross6500,O3,LTO0,no trace output):
  median CPU real-time factor .563 baseline/.211 combined ultra. These are not
  WASM, engine-only or callback acceptance results. Uniform refinement is costly.
- Recommended next comparison: acoustic-preserving conservative1D vs coupled
  nonlinear gas/acoustic network with reflected source loading and common
  geometry-dependent pulse/noise transfer. Recent engine-specific pulse/learned
  parameter papers are relevant benchmarks, not proof of arbitrary hardware
  physics. Neural residual remains conditional on matched data and an unmet gap.
- Six raw stereo reference intervals retain unknown RPM/load/perspective;
  M4 turbo/NA-I6 mismatch remains. Current33 engine WAVs/59 reference files
  are preserved; all34 accepted examples/five full coasts stay in RESULTS.html,
  with appended plots/twelve common-gain diagnostic comparisons. Selected
  float diagnostics are grouped in renders/current/experiment/; temporary
  controls are cleaned after publication validation, numeric evidence retained.
- Native175/4472,Python29,web17,transport/source ASan/UBSan and meaningful
  analyzer/spectral self-checks pass. Offline result/graph details are in the
  final evidence manifest: all46clips play, original34previews/59references
  unchanged, original4jumpbuttons and4plots valid. Removed252temporary
  controls/1.75GB; kept42selectedfloats together, republication after cleanup
  passes. GraphifyAST2011nodes/5485edges/113communities, knownrb.h:415 limit;
  no document semantic extraction claimed. No engine/preset/WASM/Claude edits.
  S1/S1e/S2/S4/full13b remain open; stage14 unstarted. Future stages are planned,
  not silently implemented by this experimental checkpoint.

## 2026-10-22: initial audio bottleneck diagnosis

- Bernardo asks for an architecture review and initial bottleneck hypothesis,
  using common neural-audio failure modes as questions. Notes and proposed
  first experiment: docs/audio-bottleneck-diagnosis-2026-10-22.md.
- Current C renderer is physics/procedural, without a trained audio model,
  vocoder or training loss. Default output/engine/pipe rates are 48/192/48 kHz;
  the coarse CSV is telemetry, not audio-generation input. Native recordings
  contain the engine; interactive browser engine integration awaits stage 14.
- First suspect: spatial and mean-flow acoustic attenuation; the existing
  S2b frictionless 800-Hz moving-packet fixture retains about 59.9% energy.
  Next inspect procedural high-frequency source conditions/transmission,
  topology and excitation/state transitions. Audible importance remains a
  hypothesis; average torque convergence does not validate audio transport.
- Fresh hash-verified raw measurements: all 33 engine WAV peaks below the
  output limiter knee. Current logs report no limited samples. Long-coast
  crest I6/cross/flat is 16.11/19.99/17.93 dB. Whole-clip scalar preview gain
  preserves temporal dynamics but masks between-clip absolute levels.
  Evidence: renders/evidence/audio-bottleneck-2026-10-22/measurements.json.
- Primary DDSP/HiFi-GAN/DiffWave papers inform the conditional neural advice;
  no GAN/diffusion implementation is authorized. Current I6/flat rollback
  remains selected; historical bypass/flat-X examples are superseded.
- Graphify queried; documentation-only review, no code-graph refresh needed.
  No production or test sources/audio/Claude setup changed; no suite rerun.
  S1/S1e/S2/S4/full 13b remain open, stage 14 unstarted. Hold development.

## 2026-10-22: selective I6/flat-V8 sound rollback

- Bernardo requests the accepted exhaust sound from before 13b-r for flat V8
  and I6, preserving I6 8500 rpm and the complete current cross-V8 setup.
  Restore cases 1 and 3 of exhaust_models.c exactly from commit 1d6c64a.
  Cross V8 retains 7500 rpm, rear muffler delete, X and the packed control.
  Flat V8 remains 9000 rpm; I4/boxer are unchanged.
- The I6 returns to unequal headers and twin Supersprint-inspired 54-mm OD
  paths with catalysts/packed rears. Flat V8 returns to its Tubi-inspired
  unvalved 60-mm equivalent bores, original lengths and retained catalysts.
  Generic X/Y/radiation/catalyst APIs and the audit diagnostic remain available.
- Complete: rebuilt WASM and single RESULTS.html with all 34 examples and five
  long coasts. All 27 unaffected WAVs/previews, including all 10 cross-V8 clips,
  are byte-identical to the starting checkpoint. Four flat-V8 sources and the
  held I6 exactly match accepted pre-13b-r hashes. All 59 references unchanged.
- Native 175/4472, SIMD WASM 15/2022, ASan/UBSan exhaust 6/1712, Python 29, web 17,
  production build and two Chromium checks per mode pass. Full validation:
  docs/stage-13b-r-rollback.md and ignored renders/evidence/13b-r-rollback/.
  Graphify queried/refreshed via AST: 1890 nodes/5111 edges/89 communities,
  known rb.h extraction limit 415. Actual I6 peak 8425.70 rpm confirms the
  retained 8500-rpm soft-limiter range. Generic solver/source physics unchanged.
- This is a requested preference rollback, not implementation of the audit
  fixes. Prior sound-physics findings describe commit bd98ab9; bypass and
  flat-V8-X examples there are historical after this rollback. S1/S1e/S2/S4/
  full 13b stay open, stage 14 unstarted. Hold now for Bernardo's instructions.

## 2026-10-22: complete sound/physics review — hold for instructions

- Bernardo requests an audit of all sound stages, a list of mistakes/gaps,
  then a hold. No sound fixes are authorized by this review request.
- Full findings and stage coverage: docs/sound-physics-audit-2026-10-22.md.
  It records 27 findings: nine confirmed implementation/numerical issues and
  eighteen model/validation items. P1 priorities include ignition causality,
  compressible jet source conditions, source routing/transmission, combustion
  model selection, deceleration diagnosis and acoustic convergence.
- New tools/sound_physics_audit.c is a read-only diagnostic linked to the
  unchanged production core. It reproduces heat before recorded spark,
  wrong sonic Mach inference, X/Y noise isolation, omitted center/link delay,
  bypass catalyst attenuation, the variation-zero physics discontinuity and
  interpolation coloration. The fuzzer also omits junction state/repairs.
- Fresh native 175/4444, Python 29 (no skips), web 17 and ASan/UBSan diagnostic
  pass. Existing tests do not establish physical correctness of these missing
  integrations. No new browser e2e/WASM-physics run or fresh complete sound/COV
  audit is claimed. Retained results are clearly labeled in the review.
- No production core, preset, web UI, audio, reference or Claude setup changes.
  Current RESULTS.html and its manifest hashes are preserved; no old audio
  sets were recreated. Logs/hashes: renders/evidence/sound-audit-2026-10-22/.
- Graphify queried and AST refreshed: 1883 nodes / 5097 edges / 97 communities;
  its known rb.h extraction limit remains line 415. This is the code graph
  refresh, not a new semantic extraction of all research documents.
- S1/S1e/S2/S4/full 13b stay open; stage 14 stays unstarted. Research fixes and
  validate realism/runtime only after Bernardo's next instruction. Hold here.

## 2026-10-22: stage 13b-r — high-rev race exhaust checkpoint

- Bernardo requests a sharper F1-style upper register for I6 and both V8s.
  Implemented: I6 limit 7000→8500, cross V8 7000→7500, flat V8 stays 9000.
  The I6 assumes mechanically prepared race hardware; kinematic valves and
  the gas solver do not certify mechanical durability. Sources and explicit
  geometry estimates: docs/stage-13b-r.md and RESEARCH.md.
- I6 now uses compensated equal effective paths, two branches into a finite
  Y and one 3-inch rear system inspired by BimmerWorld/MagnaFlow race parts.
  Flat V8 uses a Kline-inspired 63.5-mm X, fixed valve-open equivalent paths
  and catalyst bypass. Cross V8 promotes the previously auditioned physical
  rear-delete route; the separate current control is the packed rear option.
  This modified Borla-inspired route is not the catalog kit.
- Catalyst substrate resistance is now an explicit descriptor/API field:
  catalyst_laminar (Pa s/m), default25, selected bypasses0; effective throat
  areas still act separately. Existing public field indices are preserved.
  No pitch/EQ/extra oscillator or RPM-dependent output gain was introduced.
- The complete native 175-test suite passes; the final strengthened six-check
  coherence test passes separately. SIMD WASM18/2031, ASan/UBSan10/2144,
  Python29, web17, production build, two Chromium tests in each development
  and production mode pass. Five mutants caught after adding signed flow
  assertions to the initially pressure-only X-coherence test.
- 192→384→768 kHz high-rev convergence retains the 48-kHz pipe grid. First
  refinement changes brake torque <.22%, pumping loss <.24%; all runs have
  zero state repairs and conservation errors <2.2e-14kg/1.3e-7J. Native/WASM
  engine-only timing remains below the separate 4x/2x targets; see stage doc.
- All five start and complete 30-second idle with zero warm stalled steps;
  20 randomized ten-second runs pass. Flat-V8 idle COV20.59–29.49%, with135
  incomplete modeled burns. Existing S1/13b acceptance remains open.
- All34 current clips, five long coasts and four top-end jump buttons are
  embedded in renders/RESULTS.html. Actual loaded peaks:8429/7457/8957rpm.
  I6's pull guard is20s; others retain12s. Seventeen accepted recordings,
  including all I4/boxer examples, remain byte-identical. All59 reference
  library files are unchanged. Current raw float WAVs remain measurement inputs.
- Upper harmonics are more prominent, especially both V8s; these metrics do
  not establish listening acceptance. I6 1–4kHz relative energy is not a
  uniform increase. Coast-band transition contrast also increases in all
  three changed models; numerical details and remaining feedback are explicit
  in the stage doc. Do not conceal it or declare robotic deceleration solved.
- Source hashes, timings, original/final mutant logs, numerical reports and
  native/WASM binaries remain in ignored renders/evidence/13b-r/. Old/current
  comparison audio is deleted after preserving measurements. Claude's setup
  remains active and unchanged. Hold further development for listening feedback;
  stage14 and the unresolved persistent-SoA/implicit-valve work were not resumed.
- Graphify was queried before exploration and refreshed through AST:
  1842 nodes / 5027 edges / 77 communities. Its existing rb.h extraction
  limitation is now line415; native/WASM compilers validate the header.

## 2026-10-22: sound accepted; consolidated results and render cleanup

- Bernardo calls the current changes a success and "a lot better". Record
  acceptance of this exhaust listening checkpoint; S1 numerical/combustion
  checks and the remaining 13b solver/performance work stay open. This turn
  changes file management only. Hold further development for instruction.
- The single listening deliverable is now **renders/RESULTS.html**: a portable
  offline file with all 34 current examples embedded, grouped by engine with
  the five long loaded coasts first. It includes the separate cross-V8
  rear-delete modification and standard revs/positions/muffler examples.
  Browser listening copies use a constant whole-clip gain and PCM16; the
  original float WAVs are byte-identical and remain measurement inputs.
- Current originals: renders/current/{standard,feedback}/. Superseded
  generated audio, including the five old 3000-rpm before controls, is
  deleted at Bernardo's explicit request. All 59 files in renders/real/ and
  renders/user-reference-previews/ are hash-verified unchanged. Original
  user MP3s outside the repo are untouched.
- Current physics, dyno, timing, binaries, rejected solver evidence and
  historical before/after measurements moved to renders/evidence/13b-p/;
  older non-audio evidence is under renders/evidence/history/. Historical
  paths in the entries below describe prior runs, not retained audio.
  renders/evidence/retention.json records the relocation/retention checks.
- `make render` now stages standard float recordings plus all five extended
  coasts, held controls and the rear-delete coast, then atomically installs
  the complete result. Failed recording/validation leaves the previous set.
  `make results` republishes current files without rendering. Isolated
  research sets use `make render-standard RENDER_DIR=...`; see
  docs/render-workflow.md. No optional numerical packages are needed to
  render or publish. Analysis tools continue to use numpy/matplotlib.
- Full 13b and S1 remain incomplete. CLAUDE.md/.claude/ remain active and
  unchanged; no physics, engine tuning, or browser app changes in this pass.
- Cleanup removes 360 superseded generated audio files. Native 173/3252,
  Python 27 and web 17 tests pass. Chromium plays all 34 embedded clips
  offline, with one player at a time and no network requests. Seven new
  publishing tests cover retention, failure rollback, audio conversion and
  the full Makefile/scenario/publication sequence with a fake simulation.
- Successful replacement was also verified on the actual project drive,
  retaining identical current/reference hashes. Graphify queried first and
  refreshed via AST: 1808 nodes / 4932 edges / 92 communities. The known
  rb.h parser limitation remains at line 414; native compilation passes.

## 2026-10-22: researched exhaust implementation and listening checkpoint

- Bernardo authorizes large sound changes when research/physics support them.
  The five researched configurations and a causal unflanged outlet load are
  implemented; full 13b remains open. Do not call its persistent SoA refactor,
  coupled implicit accuracy requirement or original 4x/2x throughput complete.
  The latest coupled midpoint port/neighbor-face experiment still misses
  pumping convergence and stays local, unshipped. See docs/stage-13b.md.
- Full source/uncertainty ledger: docs/stage-13b-p.md. Primary manufacturer
  dimensions are distinguished from estimated lengths, bores/wall thickness,
  silencer cores/packing and junction volumes. Ferrari's two equivalent
  bank paths and the generic car's outlet locations are explicit limits.
- All presets now use their own full-path research configuration. Intermediate
  pipes/cans, twin I6 routing, a finite X and a one-collector/two-rear-path Y
  are supported. Pipe API indices retain the existing prefix; center/link
  kinds and places append to the enums. Original generic descriptors remain
  controls. The boxer idle-air equivalent is recalibrated to 46 mm² with
  air-area PI gain preserved; stability thresholds are unchanged.
- The radiation load changes gas momentum and books exterior pressure work;
  its inertance replaces the old end correction. Frequency response,
  passivity, pipe/engine conservation and deliberate-fault tests pass.
  --outlet-radiation constant|unflanged isolates it in native sound renders.
- Final native 173 tests/3252 checks; ASan/UBSan 9/659 (detect_leaks=0);
  direct SIMD WASM physics 16/843; eight mutants caught; 20 randomized
  ten-second runs with no failures/repairs. Python 20, web 17, production
  build and Chromium audio dev/preview checks pass. Standard float WAVs,
  five long coasts, equal-gain 3000 rpm controls and a cross-V8 rear-delete
  control are under ignored renders/13b-complete/. Current 39 WAVs are finite
  and below the limiter; two standard gains change to .2 (I6 rev, flat V8
  held 3000) to avoid clipping with the louder new systems.
- Fresh native five-model dyno steps and 30-second idle / 10-second held-WOT
  audit are complete. All start without warm stalls. Flat-V8 idle work COV
  remains 21.21–30.98%, with 162 incomplete modeled burns at EVO. S1 stays
  open. The exhaust/radiation changes do not solve that combustion issue.
- The selected packed cross-V8 kit is darker than the previous unfilled
  setup. A separately named rear-delete control retains new pipes/X/radiation
  with packing removed; it is not the catalog kit. I6's diagnostic overrun
  band step is smaller, flat V8's is larger. No universal perceived realism
  improvement or listening approval is claimed from those measurements.
- Local listening guide: renders/13b-complete/FEEDBACK.md. tools/exhaust_feedback.py
  reproduces controls, long coasts, spectra and lift-relative measurements.
  MP3s use one static whole-clip gain; measurements use raw float WAVs.
  Supplied reference recordings and perspective caveats are preserved.
- Final core WASM 158247 bytes; source SHA-256
  07ce3366e927c09cab6e97e8f8485d2782032662c76673d92149f47e635fe40b.
  Retained executables/hashes: renders/13b-complete/final-{cli,bench,bench.wasm,core.wasm}.
  /tmp/rev-bench-b922742 is an experimental modified source copy, not a clean
  baseline checkout. Rejected solver patches/results are in coupled-probe/.
- Final otherwise-idle interleaved timing (timing/benchmarks.json): cross V8
  .656x native and .472x WASM, with the physical exhaust changes. The 4x/2x
  targets remain unmet; this is not an optimization-only A/B. Other model
  costs, timing variance and stereo/browser limitations are in 13b-p notes.
- Graphify queried before exploration, AST refreshed: 1765 nodes, 4818 edges,
  97 communities. Its pre-existing rb.h parser limitation is now line 414;
  both C compilers accept the header. CLAUDE.md and .claude/ are unchanged.
  Stage 14 remains outside this request; keep unresolved acceptance explicit.

## 2026-10-22: full 13b and model-specific exhaust work resumed

Bernardo requests completion of all remaining 13b work plus new stage 13b-p:
real researched complete exhaust systems for every engine reference, with a
review of the underlying pipe physics. Large sound changes are authorized
when research/physics supports them; the b922742 renders are controls rather
than required targets. The former hold is lifted. Hold again only after this
work, without entering stage 14. Keep sources, assumptions, validation and
unmet acceptance explicit; do not promise exact 3D reality from a 1D model.

## 2026-10-22: stage 13b performance feedback checkpoint

- Bernardo said the S2b renders seem okay and authorized proceeding from them
  through 13b, followed by a hold for feedback. The entry hold in the older
  sections was lifted for this pass. **Hold now before further 13b or stage 14.**
- 13b-a is complete: eight-cell SoA thermodynamic scratch kernel, scalar
  repair/Newton fallback, vectorized mixture coefficient rebuild, native
  O3/LTO and WASM SIMD/LTO. Canonical chambers, the fast port cells, 1e-8
  remix threshold, default rates/grid and all sound/calibration settings stay.
  This is not the full persistent pipe/face SoA conversion.
- All 23 standard float WAVs and three extended stereo coasts are byte-identical
  to S2b. All 25 engine diagnostics are clean. The three current MP3 previews
  reuse verified encodings of the same float source. Raw audio, telemetry,
  hashes, plots and commands remain ignored in renders/stage-13b/.
- Three interleaved benchmark rounds: cross V8 native 0.546x -> 0.649x,
  WASM 0.455x -> 0.485x (about 19%/7% higher throughput). These time engine
  physics, not stereo audio or worklet deadlines. Host variation is retained
  in benchmarks.json; small gains on other presets need cautious interpretation.
- A research-only frozen-property backward-Euler orifice probe worsens
  96 kHz motored WOT pumping error vs 192 kHz: I4 10.78% -> 20.93%, cross V8
  12.66% -> 29.87%, with pipes fixed at 48 kHz. It omits port-face coupling,
  freezes thermal coefficients and applies to all orifices. Rejected: no
  probe code/option enters the core. Patch/source/results stay local.
- Full 13b remains open: persistent pipe/face SoA, properly coupled implicit
  valve accuracy, and 4x native/2x WASM at default quality are unresolved.
  PLAN.md records the small-stage split explicitly; do not mark the whole
  stage complete, lower default quality or proceed to 14 at this checkpoint.
- Native 167 tests/2709 checks; focused ASan/UBSan 5 tests/149 checks
  (detect_leaks=0 for sandbox ptrace); direct SIMD WASM C tests 9/310;
  all six final mutants caught; 20 randomized ten-second engines have
  zero failures/repairs (before the final overflow-only guard; final native
  randomized tests pass too). Python 20, web 17, build and browser audio dev/preview
  pass. make test-wasm exercises the same physics assertions in WASI.
- No fresh full dyno sweep, 30-second idle-COV or repetition audit is claimed.
  Native dyno/conservation tests pass and exact audio identity is established.
  S1's numerical/ignition requirements and broader S2/S4 acceptance remain
  open. The permission to proceed from the renders is not blanket completion.
- The final source fingerprint and SIMD WASM are committed. Native renders and
  WASM physics/browser checks include the specific-energy overflow guard.
  Emitted f64x2
  polynomial operations and native vectorization reports are retained locally.
- Graphify was queried before exploration and refreshed after the changes.
  CLAUDE.md/.claude/ and isolated environments are preserved. The checkpoint
  is committed/synced to main. See docs/stage-13b.md for measured limits,
  current long-coast examples and reproduction instructions.

## 2026-10-21: S2b — lift-off contrast and cross-plane redline

- Bernardo confirmed the boom is the louder, higher tone replacing the burble
  just after lift-off. Source/layer controls isolate it mainly to pipe flow;
  immediate/delayed cut and throttle-follower probes do not reliably fix it.
  Rejected follower code is not shipped; evidence is local in renders/feedback-13b/.
- Adopted limited linear reconstruction of transported pipe momentum, with
  minmod and donor fallback at extrema/boundaries. Mass/species/enthalpy and
  the physical loss models are retained. This is not a complete second-order
  Euler solver. `sound --momentum-order 1` selects the old donor control.
- Independent lossless 800 Hz packet test: ~39.0% energy retained with donor
  transport, ~59.9% with the revision at ~70 m/s, both directions; stationary
  retention ~100.1%. A genuine numerical contributor to lost loaded detail
  is demonstrated. It is not proof every remaining pumping resonance is false.
- Cross V8 now uses unfilled pipes rather than packed straight-through cans,
  consistent with the user's straight-piped reference description. The old
  hardware is available with `--muffler straight`; not a calibrated Mustang.
- Matched asphalt long sweeps at gain 0.15: 600–1600 Hz pre-lift/early-coast
  contrast changes I6 +9.84 to +6.96 dB, flat V8 +3.51 to -0.32 dB. This is
  less contrast through restored loaded detail, not a claim that all absolute
  overrun levels fell. Near the 6500-rpm crossing the cross V8 centroid rises
  393 to 790 Hz and above-2-kHz share -21.51 to -14.98 dB. Centroid is brightness,
  not perceived pitch. Listening approval remains open.
- Unfiltered float WAVs, raw CSV, references, measurements and rejected controls
  stay in ignored renders/feedback-13b/. tools/measure_lift_feedback.py writes
  mean-ear-power metrics and separate shared-gain listening pairs. Original
  recording viewpoints/RPM/load remain uncertain; do not infer positions from
  brightness. Full longer coasts remain eight seconds before clutch-in.
- Screenshot recovered through /mnt/c: missing matplotlib in checkpoint_13b's
  eager compare_real import. Rendering now uses only the standard library;
  analysis uses ignored .venv-analysis/ with tools/requirements-analysis.txt.
  Real --long-coast rendering was verified with Python -S and no PYTHONPATH.
  Fault-test copies exclude virtual environments and Graphify output.
- Fresh 30-second native physics audit: zero warm stalls in every preset;
  V8 idle IMEP COV still reaches 12.38% cross / 30.53% flat. Fresh 30-second
  audio repetition has not been remeasured; prior checkpoint values are historical.
- Native 165 tests/2583 checks; focused ASan/UBSan 3 tests/23 checks; all three
  S2b mutants caught; 20 randomized 10-second engines have zero failures.
  Five full life-enabled dyno sweeps show <0.66% torque/<0.88% power bin changes.
  23 float renders and new long sweeps report zero limited/non-finite samples.
  Twenty Python tests, 17 WASM unit tests, production build and browser audio
  dev/preview pass. The WASM preset test now checks the unfilled default and
  confirms the prior packed type remains selectable.
- Corrected the old checkpoint's all-zero limiter claim: its I4 cabin sweep
  is byte-identical to a control reporting 27 limited samples. Render gains
  now allow headroom. Fair lift comparisons retain gain 0.15 in both versions.
- Engine-only cross V8 benchmarks are about 0.52x native / 0.47x WASM;
  full-stereo cross clip CPU cost rises about 17% here. Default quality/grid
  stays unchanged; no speed gain or completed 13b target is claimed.
- Preserved and fast-forwarded Bernardo's newer GitHub README commit 5d97591;
  its personal project motivation remains intact. CLAUDE.md/.claude/ are unchanged.
  See docs/stage-S2b.md; S1/S2/S4 remain open and 13b/14 have not started.

## 2026-10-21: consolidated feedback checkpoint before 13b

- Stop here for Bernardo's feedback. His request resumed development toward
  stage 13b, with the consolidated checkpoint at its entry. S1/S2/S4 acceptance
  is still open; do not mark them complete or proceed to 13b/14 without the
  next instruction. Full progress and limits: `docs/checkpoint-13b.md`.
- S1e: actual pedal lifts now enter a timed ignition-retard transition before
  fuel cut; immediate-cut control is `--overrun-transition 0`. Defaults 0.15 s /
  12 degrees are illustrative. The timer is armed by pedal use to preserve
  startup calibration. This does not prove the robotic deceleration is fixed.
- S2: intake-valve and chambered-baffle noise use modeled flow and a separate
  random stream; ultra quality optionally doubles physics/pipe resolution.
  Default high remains. Valve-noise gain is not blindly reduced for a finer
  1D grid, which does not resolve 3D turbulence.
- S4: `rb_set_scene` and `rb_render_stereo` add separate-source/ear propagation,
  aperture directivity, head-shadow approximation, ground/wall images, a damped
  FDN and retarded-time moving-source Doppler. Eight scenes are available in
  the native CLI; WASM exports and type declarations are updated. The browser
  still plays the tone until stage 14. Scene audio supports 8–96 kHz.
- Bernardo additionally requested longer deceleration. `--coast-seconds 8`
  keeps the vehicle inertia attached for eight seconds; `--pull-seconds 12`
  bounds the pull if the limiter is not reached. The three stereo previews
  under ignored `renders/checkpoint-13b/long-coast/` are 20.66 s I6, 17.22 s
  cross V8 and 20.21 s flat V8. Their manifest/CSV identify lift and clutch-in.
  No time stretching, synthetic pops or pitch effects are used.
- Real falling-pitch candidates: BMW 250–253, Mustang 257–263, Ferrari 302–310 s.
  Do not promote these to verified neutral revs, loads, RPM or viewpoints.
  Reference/old/current pairs, scenes, full float renders and matched controls
  are local under `renders/checkpoint-13b/`; `tools/checkpoint_13b.py` reproduces
  them. Unfiltered float WAVs remain the measurement inputs.
- Validation: 164 native tests / 2574 checks; 11 focused ASan/UBSan tests / 50
  checks (leak detection disabled for sandbox ptrace); 16 Python tests; 17 WASM
  unit tests; production build; browser audio dev/preview. Ten planted faults
  are caught. The new independent ear-delay test catches a mutant that the
  first stereo level-difference check missed. Five full matched dyno CSVs are
  byte-identical to S1d's accepted after sweeps; 23 standard float renders and
  all new recorded scenarios report zero limited/non-finite samples.
- Numerical gaps remain: I4/boxer warm repeat +19.89/+17.71 dB, cross/flat V8
  idle COV up to 12.27/30.52%; no genuine ignition-kernel failure model. S2 also
  misses I6/flat full-rev slopes and cross V8 idle high-frequency share.
  Cross-plane physics is about 0.45x native / 0.39x WASM on this shared host;
  13b's 4x/2x targets are unmet. Ultra audio costs about 3x high here; its
  audible value remains unapproved. Realism stays above speed.
- Tools/dependencies/browser validation stay isolated; Claude's setup remains
  intact. The Graphify code graph is refreshed for the checkpoint. Commit and
  sync the coherent checkpoint to the existing main branch, without trailers;
  keep all source and generated audio ignored.

## 2026-10-21 listening feedback and resumed development

- Bernardo heard little noticeable difference in the recent S1d comparisons; no audible
  realism improvement or S1 listening approval has been established.
- Use the supplied real engine recordings as the primary sound reference. V8 and
  inline-six deceleration sound too robotic to Bernardo. Researching and testing the
  underlying cause is a priority before choosing a fix. The cause remains unknown;
  he did not specify which V8 layout. Keep proposed explanations labeled as hypotheses.
- The next authorized sound work should compare documented real deceleration intervals
  with corresponding simulated phases, separating neutral rev-down from loaded overrun
  and retaining uncertainty about cabin/exterior/flyby perspectives. Existing reference
  processing and measurement rules still apply. See PLAN.md and
  `docs/user-sound-references.md`.
- Realism is the absolute priority across the project. A heavier program and greater
  processing demands on the user's machine are acceptable when the demonstrated realism
  gain justifies them. Efficiency supports realism; performance targets may be revised
  with documented evidence of that gain and measured processing demands.
- Every realism problem requires a research-based, physically justified solution that
  prioritizes realism while pursuing efficiency. Cite primary sources, document assumptions
  and limits, validate against real evidence, and measure runtime cost
  (PLAN.md, working agreement 7).
- Seek additional reference recordings/sounds when needed, or ask Bernardo for targeted
  examples. He can provide sounds he finds useful for development. Preserve provenance,
  intervals, perspective uncertainty and processing notes (PLAN.md, working agreement 8).
- **Resumed:** Bernardo subsequently requested development through stage 13b, followed
  by a consolidated feedback checkpoint with progress, current state and listening
  examples. Continue the small stages; keep S1 open wherever acceptance remains unmet.

## 2026-10-21 continuation: S1d complete; S1 open

- Found that burn duration used requested AFR while heat release already used
  actual cylinder fuel/air. With life enabled the burn now uses post-injection
  trapped equivalence ratio, including cylinder fuel trim and retained fuel.
- `rb_burn.phi_ivc` captures the ratio at IVC; no new fuel reservoir, stochastic
  draw, misfire threshold or flame-speed coefficient. The life-off path remains
  unchanged. Missing reactants use a finite fallback with no burnable charge.
- Two new native tests cover matched rich charges under different injector
  targets, post-injection capture, frozen timing, absent reactants and legacy
  off/NaN/negative variation. Both pass; all four S1d mutants are caught.
- Full old/new float sets (23 WAVs each), five-preset 30-second idle/work audits
  and life-enabled dyno sweeps complete under ignored renders/s1d/. All presets
  have zero warm stalls. Maximum 500-rpm-bin torque/power changes are 0.0541% /
  0.0345%; matched 3000-rpm torque changes are below 0.0018%.
- Warm low-band repetition is now I4 +19.89, I6 +11.38, cross V8 +10.20,
  flat V8 +5.32, boxer +17.71 dB. The I4/boxer issue is essentially unchanged.
  Cross/flat V8 idle COV still reaches 12.27% / 30.52%; no global variation retune.
- The cross V8 startup at gain 1 hits six soft-limited samples near 0.339 s.
  The failed run is preserved under initial-gain1/. Matched audit recordings
  at gain 0.5 have zero limited/non-finite samples for every preset; audit gain
  now defaults to 0.5 and --source-root records the before build's actual source.
  Engine/browser gain and standard render gains remain unchanged. This transient
  and the changed boxer/flat V8 revs need listening review.
- Added dyno `--variation` (default 0) and optional Makefile RENDER_DIR /
  RENDER_FLAGS so validation can preserve paired float sets. Defaults are kept.
- Validation: 153 native tests / 2517 checks; 16 Python and 16 web unit tests;
  production build and Chrome dev/preview audio tests pass. ASan/UBSan passes
  five focused tests / 150 checks (leak detection disabled for sandbox ptrace).
- WASM rebuilt with existing isolated wasi-sdk: 121148 bytes. Claude's setup
  is unchanged. See docs/stage-S1d.md and the eight matched listening pairs in
  renders/s1d/listening/. S1 still requires a genuine ignition model, repetition
  work, V8 idle investigation and Bernardo's listening approval.

## 2026-10-21 continuation: S1c and filtered references

- S1c is complete: a native audit (`make audit`) and float idle sound baseline
  runner (`python3 tools/audit_s1.py`). All five presets finish 30-second idles
  without warm stalls. Core physics, calibration, quality and WASM are unchanged.
- Warm low-band repeat is I4 +19.91, I6 +11.36, cross V8 +10.13, flat V8 +5.36,
  boxer +17.71 dB. I4/boxer still miss the target. See docs/stage-S1c.md.
- Per-cylinder idle IMEP COV reaches 8.49–12.24% in the cross V8 and 21.40–30.22%
  in the flat V8. Direct EVO monitoring finds 132/1598 partial completions in
  the flat idle; a matched 96 kHz run confirms similar behavior. This is not a
  new regression or an ignition-misfire count. Increasing variation globally
  would be unjustified; the earlier sampled S1b table could miss these events.
- Validation: 151 native tests / 2455 checks, 15 Python tests (no skips), 16 web
  unit tests and production typecheck/build pass. New audits remain diagnostic;
  they do not replace matched full dyno sweeps or listening approval.
- R1b is complete: conservative filtered listening copies and matched raw/filtered
  comparisons of Bernardo's four MP3s. All 25 original/reference/catalog files
  remain byte-identical. Camera
  position, cabin/exterior transitions and flybys must not be inferred from
  brightness alone. Record source intervals and processing in the manifest.
- S1 remains open until low-band repetition, ignition failures, dyno acceptance
  and Bernardo's listening checks have been resolved.
- This continuation has direct WSL execution (GCC 15.2). The older Windows
  bridge limitations below describe the originating session. Here temporary
  Python packages and Linux Node 22.16 were isolated under /tmp for validation;
  nothing was installed into Claude's environment or the mounted project.
- `CLAUDE.md`, `.claude/`, `core/` and `web/` remain unchanged. The Graphify graph
  is rebuilt after these source/documentation changes and committed with S1c.

## 2026-10-21: S1a, S1b and local references

- S1a is complete: shared PCM16/float32 WAV reader, Windows path handling,
  meaningful analysis regressions. All three sound tools accept float renders.
- S1b is complete as a substage: life-enabled slow burns retain charge-model
  demand and are stopped at EVO; unburned species stay in the gas. No separate
  residual cutoff, ignition-misfire probability or fuel-carryover state.
- S1 itself remains unfinished: low-band repetition at idle still reaches
  +20.4 dB, and genuine spark-kernel failure is not modeled.
- Validation: make test passes 151 tests / 2455 checks. All five new planted
  bugs are caught. Python analysis suite passes nine tests. Web unit tests
  pass all 16, and the audio browser test passes dev and production preview.
- All five presets were started and run for 30 seconds. Warm idle is stable;
  mean torque at 3000 rpm wide open is unchanged to six decimal places.
- make render completed. Fifteen prior WAVs are byte-identical. Seven other
  rev/pull differences are spectrally negligible. The flat-plane rev has a
  larger limiter transient: see docs/stage-S1b.md and its listening checkpoint.
  Its example recording gain is now 0.38; final limited/non-finite counts are 0.
- WASM was rebuilt (120944 bytes); source fingerprint and import/export tests pass.
- Bernardo supplied four MP3s. Sixteen unfiltered float32 excerpts are in
  renders/real, and four stereo previews plus comparisons are in
  renders/user-reference-previews. Original MP3s are unchanged.
- Local-reference metadata and reproduction: tools/user_engine_reference_sources.json,
  tools/user_engine_references.csv, tools/prepare_user_references.py, and
  docs/user-sound-references.md. The default Freesound catalog stays separate.
- Brighter/darker groups describe acoustic differences, not verified camera
  positions. BMW/Golf are turbocharged; current I6/I4 presets are naturally
  aspirated. Do not force their muffling into an NA calibration. These are
  engine-dominant excerpts, not perfectly isolated stems or calibrated SPL.

## Codex tools on this machine

WSL returns E_ACCESSDENIED in this sandbox. Isolated Zig 0.15/0.16 compilers also
failed on cache I/O. Neither failure justified changing Claude's environment.
Portable GCC 16.2 and wasi-sdk 20 (Clang 16) resolved native/WASM validation.
No npm packages or system/Claude tools were installed or upgraded.

The isolated tools are under the originating chat's work/ folder:
C:/Users/berna/Documents/Codex/2026-10-21/hey-chat-i-have-this-project/work

- gcc/w64devkit/bin: add to PATH for make/gcc, and build to work/make-native.
- build_wasm.py: reproduces the Makefile's exports and fingerprint using wasi-sdk.
- python: numpy/matplotlib/scipy; audio-tools: FFmpeg decoder; graphify: 0.9.92.
- Set TEMP/TMP and MPLCONFIGDIR to writable work/tmp locations for this sandbox.
- Windows browser tests use PW_CHROMIUM=C:/Program Files/Google/Chrome/Application/chrome.exe.
  Running Vite in an independent terminal avoids Windows server-teardown hangs.
- graphify is available with PYTHONPATH pointing to work/graphify; use
  python -m graphify query/update from the repo root. Query before new code
  exploration as CLAUDE.md requests; refresh the graph after source changes.

The user authorized committing and syncing this work to bernardovh77/REV-BENCH,
as well as retaining it in C:/Users/berna/REV-BENCH. Follow the existing commit
author convention, without co-author/session trailers. Generated/private audio
stays in ignored renders/; commit the code, reference catalog and documentation.
