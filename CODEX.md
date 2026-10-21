# REV-BENCH - Codex handoff

Read PLAN.md, CLAUDE.md and the latest stage notes before continuing. Claude's
instructions, hooks and settings remain active; this file supplements them.
CLAUDE.md and .claude/ have been preserved.

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
