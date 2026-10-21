# REV-BENCH - Codex handoff

Read PLAN.md, CLAUDE.md and the latest stage notes before continuing. Claude's
instructions, hooks and settings remain active; this file supplements them.
CLAUDE.md and .claude/ have been preserved.

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
