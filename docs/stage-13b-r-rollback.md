# Selective sound rollback — 2026-10-22

Bernardo requests the sound from the accepted checkpoint before 13b-r for
the flat V8 and I6, while retaining the I6's higher rev limit and all current
cross-V8 changes. Restore the selected exhaust definitions, regenerate the
single listening result with the long coasts, then hold for instructions.
This does not authorize implementing the sound-audit backlog.

## Configuration

| Engine | Current selection |
| --- | --- |
| I6 | Restore the Supersprint-inspired twin exhaust from `1d6c64a`: original unequal headers, 51.6-mm bores, .55/.70/.55/.52/.25-m front/center/link/rear/tail, catalysts and packed rear cores. Retain **8500 rpm** and its prepared-engine assumption. |
| Flat V8 | Restore the Tubi-inspired unvalved path from `1d6c64a`: 60-mm equivalent bores, .75/.35/.28-m front/rear/tail, retained catalysts, no added X or center/link. Retain **9000 rpm**. |
| Cross V8 | Keep the **7500-rpm** rear-delete, 3-inch-inspired X system and the separate packed-rear comparison. |
| I4 / boxer | Preserve current configuration and sound. |

Cases 1/3 of `core/src/exhaust_models.c` exactly match their pre-13b-r source
from `1d6c64a`. Cases 0/2/4 exactly match this task's starting commit `5254403`.
The rev-limit definitions in `presets.c` are unchanged. No EQ, pitch processing,
noise retune, controller change or numerical-quality reduction is introduced.
Raising I6 speed still raises its firing rate naturally; restoring its exhaust
does not require reusing a recording that stops at the old 7000-rpm limit.

The earlier geometry research, manufacturer sources and uncertainty ledger
remain in [13b-p](stage-13b-p.md). Cross V8 and the retained rev-limit assumptions
are in [13b-r](stage-13b-r.md). Dimensions and proprietary internals remain
engineering estimates. This is a preference rollback of those existing models,
not new evidence that their physics is exact.

The catalyst-resistance field and generic X/Y/radiation implementation remain
available. The topology regression assertions and browser preset expectations
are adjusted to the selected defaults; generic junction/coherence, conservation,
descriptor guards and high-RPM checks remain in place. No new physical model
is being validated through implementation-mirroring tests or new mutations.

## Listening result and verification

The deliverable remains `renders/RESULTS.html`, with all 34 examples and the
five eight-second loaded coasts. The I6 keeps the 20-second pull guard and the
others 12 seconds. User-facing labels identify the restored exhausts. Sources
remain unfiltered float WAVs in `renders/current/`; whole-clip scalar gain and
PCM16 conversion are confined to the embedded listening copies.

Before rendering, source/preview manifests and all 59 reference-file hashes
were saved under ignored `renders/evidence/13b-r-rollback/`. The publisher
retains the existing complete result until all regenerated recordings pass
validation, then replaces it and deletes the superseded generated audio.
Compare unaffected recordings with the starting source hashes, and restored
flat-V8/held-I6 recordings with `13b-r/accepted-recordings.json`. The I6's
free-rev/coast trajectory can change with the higher limiter; no exact identity
with a 7000-rpm drive is implied.

Native **175 tests / 4472 assertions** pass. SIMD WASM checks pass for the
core numerical kernels (**9/310**) and exhaust networks/high-RPM bookkeeping
(**6/1712**). ASan/UBSan exhaust checks pass (**6/1712**), with leak detection
disabled for the existing sandbox ptrace limitation. Python **29**, web unit
**17**, production build, and Chromium development/production checks (**two
tests in each mode**) pass. The browser checks verify the actual revised WASM
preset defaults; the main browser app still plays the stage-2 tone.

The first isolated web test caught an out-of-date copy of the C sources in
the validation workspace. Copying the current sources alongside the already
rebuilt WASM resolves the fingerprint mismatch; the project core was rebuilt
correctly. Current WASM is **158514 bytes**, SHA-256
`19451b12b3ae8f190cc1c3060065ad6a56bf7f178ee620ad4cc720cc424d4b68`.

Graphify was queried before source exploration and refreshed through AST:
**1890 nodes / 5111 edges / 89 communities**. Its known public-header extraction
limitation remains `core/include/rb.h:415`; native and WASM compilation pass.
Claude's configuration and hooks are unchanged.

Publication completed with all **34** float sources and embedded previews.
All **27** unaffected source recordings and previews are byte-identical to
the starting checkpoint, including all **10 cross-V8** examples. All **four
flat-V8** sources and the **held-3000-rpm I6** source exactly match the saved
accepted pre-13b-r hashes. The I6 free-rev/long-coast recordings reflect its
retained higher limit. Actual loaded peaks are **8425.70 / 7457.31 / 8956.16 rpm**
for I6/cross/flat, consistent with soft limiting below 8500/7500/9000.

All **33 engine recordings** report zero soft-limited/non-finite samples, and
the publisher verifies every long coast. All **59 reference files** are
unchanged. Superseded generated audio is removed by the completed atomic
replacement; only the current result is published. Exact hashes and numerical
checks are in `renders/evidence/13b-r-rollback/validation.json`.
Chromium plays all 34 embedded examples offline, verifies exclusive playback
and all four top-end jump buttons, and confirms the restored setup labels.

## Handoff

Hold after this scoped rollback and publication. The audit remains evidence
for its reviewed commit; its I6/flat-V8 bypass and flat-V8-X examples describe
the superseded settings. The underlying source/timing/combustion/validation
issues are not claimed fixed by choosing different hardware. S1/S1e/S2/S4/full
13b stay open, and stage 14 remains unstarted. Claude's setup stays active.
