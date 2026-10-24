# Stage039 closeout review and Stage041 readiness

**Superseded completion decision:** Bernardo clarified that the full039 matched
reference acquisition is still required. The [subsequent source-search review](stage-039-reference-search.md)
reopens039 and records the remaining gaps. This document preserves the valid
protocol checks and the earlier bounded decision; it no longer closes the whole
stage. PLAN owns current status. No041 work starts from this historical closeout.

Reviewed2026-10-24 at Bernardo's request, against baseline769cacf and the
[original039 specification](roadmap/03-contracts-and-sources.md#stage-039).
No engine, renderer, reference-lock assignment or audio is changed here.
PLAN remains the sole status tracker.

## Decision and completion boundary

039 was the only unfinished numbered stage among001–040. Its acquisition,
annotation, stereo measurement and comparison-protocol deliverable now closes
within the available evidence. The calibration outcome is explicitly negative:
**zero sources qualify for calibrated SPL or matched-engine response.**

The039 exit evidence requires coverage/uncertainty, processing/provenance,
sample intervals, powers, session separation, audition controls and limits on
each recording's claims. These are present and reviewed below. Its stop rule
forbids promoting unknown viewpoint/RPM into calibration; that prohibition
continues. Completion of the protocol is not completion of an engine-calibration
claim. No failed numerical, calibration, rights, runtime or listening gate is
waived, and no simulated result is made more realistic by changing this status.

The initial awaiting-reference decision remains in the historical039 execution
record. The newly supplied, curated E46/GR86 sources improve practical NA I6 and
related-boxer coverage, while their missing metadata remains a documented result.
Later qualification stages must acquire better evidence when their claims
require it. No additional recording request is pending now.

## Exit-evidence review

| Required item | Reviewed evidence and result |
| --- | --- |
| Acquisition, provenance and rights | Eleven source fingerprints and source locators in [lockv2](../tools/reference_sessions_v2.json); supplied recordings are local references, with no redistribution/training permission inferred. Public GM/Subaru source pages identify CC0 previews. The two supplied rear-V8 files have original source IDs and hashes but no verified public page URL. |
| Coverage and uncertainty | The matrix below and source metadata retain induction/model mismatches, unknown RPM/load/microphone/gain, partial exhaust descriptions, motion and cabin ASC uncertainty. No null is converted into a measured value. |
| Exact source intervals |34 raw excerpts,488.205646s total, with48kHz decoded-audio frame boundaries. The [video intake](video-sound-references-2026-10-24.md) records14 new intervals and visual/speech QA; the accepted E46 closed-valve candidate is198–200s, excluding the later truck. |
| Raw measurement and processing ledger | All34 WAVs are unfiltered float stereo. Per-channel time/band powers, peaks and RMS are recomputed and agree with the saved report. No waveform folding, EQ, gain, fade, distance correction or denoising enters measurement samples. Compressed sources remain compressed-source evidence. |
| Frozen source/session separation | Parentv1's eight sessions/20 intervals are unchanged. v2 adds three development sources. Nine development sessions/32 intervals, one validation session/one interval and one quarantine session/one interval remain distinct. All17 historically used Freesound catalog IDs remain development. |
| Common gain and level matching | Four development-only controls reproduce their stored gains and embedded preview hashes. The common-gain arm preserves recorded level ratios; equal-RMS explicitly removes them. Existing anti-phase and unequal-amplitude controls pass. |
| Claim limits and failure detection | Unsupported SPL/matched-engine qualification is rejected by schema1. Seen-validation, family/fingerprint leakage, unsafe intervals, clipping conversion, changed parent, FA24-as-FB20, either-channel speech and wrong audio-track selection are covered by the existing behavior tests. No held-out audio is introduced into listening previews. |
| Integrated checkpoint preservation | Original recordings and hardlink paths, previous20 raw excerpts, engine/core/WASM/Claude files and the55-player RESULTS page are preserved. Its existing offline playback evidence applies to the identical page hash. |

The validation partition is a **workflow partition**, not a certificate of
matched engine or microphone conditions. Its metadata and power QA have been
inspected; its audio has not been used for model fitting or listening-based
tuning. The quarantined Subaru source is not promoted at all. Previously heard
development recordings cannot later become fresh held-out validation.

## Current coverage and permitted comparisons

The versioned lock remains the exact interval/metadata authority. These groups
describe usefulness and remaining limits; they do not infer microphone position
from brightness or establish quantitative engine agreement.

| Source | Raw excerpts / partition | Useful evidence | Restriction |
| --- | --- | --- | --- |
| Ferrari458 |4 / development | NA flat-plane V8 texture and transitions | Exhaust/RPM/load/position/gain unknown; not a matched high-RPM or SPL target. |
| BMW M4 |4 / development | Turbo I6 texture | Turbo induction and uncertain generation; cannot qualify the NA S54-inspired preset. |
| Golf GTI |4 / development | Turbo I4 texture | Cannot qualify the NA I4 configuration or its load map. |
| Mustang GT |4 / development | Cross-plane V8 texture | Exact exhaust, state and selected-window microphone geometry unverified. |
| rcf.lorax |1 / development | User-classified rear cross-V8 observation | Bernardo reports about3m behind tailpipes; vehicle/hardware/mic gain unverified. |
| viper_ws9 |1 / development | User-classified rear cross-V8 observation | Same distance/context limitation; conservatively grouped with uncertain rear-V8 family. |
| GM/Holden race six |1 / validation | Reserved independent session; uploader describes open rear headers | RPM, measured load, microphone distance/calibration and exact engine match missing. No tuning audition. |
| Subaru-labelled preview |1 / quarantine | Provenance/uncertainty example only | Model/year/induction/exhaust/state not verified; no FB20 qualification. |
| GR86 mountain |4 / development | Cabin driving transitions in a related NA boxer | FA24 differs from FB20; exhaust and ASC/mic/window/operating state unknown. Music may remain. |
| GR86 AWE Touring |4 / development | Related NA boxer cabin response with uploader-named cat-back | FA24 differs from FB20; upstream hardware/tune, ASC and recording transfer unknown. |
| E46 Valvetronic |6 / development | NA I6 exterior revs, valve-caption controls and roadside passes | Section3+headers is a partial system description; captions are not actuator telemetry. RPM/load/mic/distance unknown. |

Speech screening and frame review reduce obvious presenter contamination; they
do not isolate engine stems or prove no wind, traffic, music, edits or added
sound. A camera view is not microphone telemetry. GR86 cabin energy may include
Active Sound Control; it cannot all be attributed to the physical tailpipe.
The current library supports scoped qualitative texture/transition observations,
not synchronized waveform error, calibrated Pa/SPL, exact engine maps or a
controlled spectral difference between different vehicle configurations.

## Open claims and their existing owners

| Remaining requirement | Owner and stop condition |
| --- | --- |
| Listener/cabin/mic transfer and absolute level | [056](roadmap/05-engine-qualification.md#stage-056): establish geometry, transfer and gain for the particular claim. Unknown gain cannot certify SPL. |
| Deceleration mechanism and state matching | [058](roadmap/05-engine-qualification.md#stage-058): qualify RPM/load/thermal/flow context before attributing a recording transition to a modeled mechanism. Independent causal controls may proceed. |
| Five-engine real physical/acoustic agreement | [059](roadmap/05-engine-qualification.md#stage-059): obtain claim-appropriate engine/exhaust/state and dyno evidence, freeze validation before fitting, and retain unsupported ranges. No matched source currently qualifies. |
| Base-engine listening acceptance | [063](roadmap/05-engine-qualification.md#stage-063): preserve reference claim limits, complete physical/runtime gates and record Bernardo's per-scenario verdict. This closeout supplies no new engine listening acceptance. |
| Conditional learned synthesis | [064](roadmap/06-learned-research.md#stage-064): verify training/evaluation rights, independence, bandwidth and configuration coverage. Local reference permission does not grant model-training permission. |
| Exact FB20/full-exhaust and expanded catalog coverage | [094](roadmap/11-engine-catalog.md#stage-094): acquire missing model/system evidence. Related FA24 cabin audio and partial exhaust titles cannot qualify a complete model. |
| Final catalog realism acceptance | [098](roadmap/12-release.md#stage-098): matched-reference and per-preset listening evidence remain required. Missing decisive evidence keeps its claim open. |

Acquisition can resume within these stages using public sources or a targeted
request to Bernardo; new source/processing/split revisions must preserve prior
history. Do not relabel a development recording as unseen or tune against the
held-out session to make coverage appear complete. Neither closing039 nor
Bernardo's positive reference-intake feedback closes S1/S1e/S2/S4/full13b.

## Review findings and Stage040 handoff

No blocking defect was found in the scoped039 pipeline. Three documentation gaps
are repaired: the roadmap protocol still named039 as the next default, and
future qualification could misread a completed acquisition row as calibrated
data;040's note also pointed to telemetry068 instead of browser integration066.
PLAN/protocol now identify the reviewed checkpoint, and059/063/094/098
explicitly retain the reference entry gates. Source locks and thresholds are
unchanged.

040's unchanged measured source fingerprints, six recorded deliberate-fault
results, causal/energy checks and declared limits are reviewed. Its nonnegative
projection repairs pre-spark heat; it is still an uncalibrated kernel surrogate.
The existing ECU mean-CA50 estimate, warm-map validity and state-dependent
failure need further work. The I4 early-coast rise remains a058 target. These
are declared follow-ups, not new claims that040 solved all combustion realism.

The next [041](roadmap/03-contracts-and-sources.md#stage-041) preflight should:

1. Freeze the mean-model/variation contract and continuity checks near zero;
   preserve040's actual discharge timing and nonnegative development.
2. Research and state residual/dilution/kernel validity, distinguish out-of-fit
   states and unresolved late chemistry, and document mean-CA50/ECU coupling.
3. Prove mass/species/fuel/energy/event consistency, multiple-seed gross/net
   IMEP/COV/repetition and stable idle across the supported five-preset map.
   Do not substitute a universal repetition score or arbitrary randomness.
4. Detect deliberate continuity/state/event faults, then perform the required
   native/WASM/full-sound regression and listening handoff for any core change.

These are entry requirements, not completed041 experiments.041 depends on
038/040; both retain their bounded evidence. It is ready to begin on Bernardo's
next instruction. No041 file, implementation, model tuning or render is added.

## Checks and reproducibility

Fresh review checks: source/lock/raw hashes, all34 stereo metrics, audition
payloads/gains, session ancestry, source-bound speech screens, preservation,
roadmap identifiers/dependencies/links and nine focused reference behavior tests
with no skips. Reproduce the focused tests with:

```sh
PYTHONPATH=tools RB_REFERENCE_FFMPEG=/path/to/working/ffmpeg \
  .venv-analysis/bin/python -m unittest -v test_reference_protocol test_video_references
```

The source equality proof from intake is retained; unchanged source and excerpt
fingerprints are checked now without decoding all videos again. Intermediate
full-decode WAVs were removed after the original proof. The identical published
page retains SHA256
`83e4046b875316fad3e176e635afbf49135196e73b9babe60630f8486256b7a5`.
Earlier native206/Python67/web17 and040 WASM results are historical evidence,
not freshly rerun suites. No code/core change requires a rebuild or new sound
checkpoint. Graphify is queried and refreshed for the updated documentation.

Local audit commands, gates, output, coverage and preservation hashes are under
ignored `renders/evidence/stage-039-close-2026-10-24/`. The audit's initial
assumption of a public URL for every supplied file was corrected to allow the
documented original-ID/hash provenance; its fingerprint reader was corrected
to distinguish source entries from nested executable/build metadata. These
were audit-fixture errors, not changes to references or passing product tests.

Commit/sync this reviewed checkpoint, then hold. Stage041 stays unstarted.
