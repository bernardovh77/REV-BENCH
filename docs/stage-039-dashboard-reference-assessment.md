# Stage 039 — two supplied dashboard references

2026-10-25; baseline f7e6372. Bernardo supplies an XV acceleration/downshift
video and an M54B30 high-rev video and asks whether they are enough. This is
reference assessment only; development remains on hold after 048, before 114/049.

## Decision

**Useful and substantially closer: both show RPM during actual driving. They
do not yet close all of 039.** The Subaru adds the missing visible operating-state
sequence through acceleration, slowing and downshifts. The BMW adds visible-RPM
road pulls toward its claimed 7000 rpm range, but ends without a sustained coast.
Neither uploader describes the exhaust build or audio microphone setup.

These sources can support approximate RPM-linked pitch, order and qualitative
road-sound comparisons after excerpt curation. Unknown recording conditions
limit stronger claims; they do not invalidate that useful evidence. A calibrated
SPL recording or professional microphone equipment is not required for every
timbre comparison. No personal-car recording request is pending.

## What the footage establishes

The intervals below are navigation windows from sampled frames, not final
sample-accurate annotation or a digitized continuous RPM/load trace. Analog
gauge readings have perspective, resolution, instrument-lag and synchronization
uncertainty. Road motion establishes a driving example, not measured torque,
wide-open throttle, clutch/brake state or ECU deceleration fuel cut.

| Source | Useful coverage | Remaining limits |
| --- | --- | --- |
| [2012 Subaru XV/Crosstrek 2.0i manual](https://www.youtube.com/watch?v=WlRSgDQGiKA), Kacpa2, uploaded 2025-04-14; 63.68 s decoded audio | Dashboard RPM and speed visible. Approximately 2–34 s contains acceleration and gear changes; approximately 34–63 s contains slowing and downshifts. Much stronger FB20-family state coverage than the earlier exterior clips. | Title attributes model/engine; hardware is not inspected. The slowdown includes gear/clutch changes and cannot all be labeled uninterrupted single-gear off-throttle coast. Exhaust/intake/ECU configuration is undisclosed. Owner acceleration-time claims are not our measurements. |
| [E46 M54B30 Revving to 7,000 RPM](https://www.youtube.com/watch?v=LJsJTlYYGKw), Clapped M3, uploaded 2023-01-27; 16.37008 s decoded audio | Tachometer and nonzero road speed establish a moving-car example, not stationary neutral revving. Rising RPM spans multiple portions, with a likely shift between them; late gauge frames approach the high-six-thousand range approximately. | The terminal drop at roughly 15.3–16.37 s is brief; shift versus lift is not independently distinguished. It does not provide several seconds of I6 coast or prove an exact 7000 rpm trace. Empty description; exhaust/build unknown. Channel name does not turn the title-attributed M54 into an M3/S54. No validation of our 8500 rpm research limit follows. |

Both cameras are inside the cabin. **The audio microphone location is unknown.**
Cabin-view footage must not be relabeled a measured cabin transfer, a fixed
tailpipe distance or an exterior recording. We do not assume stock exhausts
from their omission in a title or description. The Subaru model attribution is
closer to the FB20/GP7-inspired preset; the BMW attribution is closer to M54B30
than the previously supplied M3/S54. Neither is an exact build match to our
modified exhaust selections.

## Remaining acquisition and qualification

The most valuable further I6 example is **one loaded M54-family pull followed
by several seconds of visible-tach lift/coast**, with the exhaust setup stated.
A source description or installation record identifying the exhaust on these
two videos would also improve their usefulness. The earlier exterior
stock/modified examples remain useful complementary evidence, but their
hardware and microphone conditions cannot be copied onto these different cars.

For a relative cabin timbre claim, identify appropriate excerpts, exclude shift
events where necessary, annotate approximate visible RPM/state and retain
recording uncertainty. For a controlled exhaust/propagation or lift-envelope
calibration claim, additional hardware, state and perspective information is
still decisive. Only absolute SPL needs a calibrated recording chain. Match
requirements to the claim instead of demanding laboratory precision universally.

Stage 039 remains **Awaiting matched references**; its implemented processing
protocol and previous eleven locked sessions/34 excerpts remain valid. This
assessment improves coverage without waiving later engine/propagation/listening
gates or admitting inspected candidates as unseen validation data.

## Inspection and processing

- Both supplied originals remain unchanged. Fingerprints, owner metadata,
  observations, intervals, limitations and permitted uses are recorded in the
  [candidate ledger](../tools/reference_candidates_supplied_20261025.json).
- 48 timestamped sampled frames cover the two videos; full contact sheets and
  enlarged instrument views were checked. This is visual/technical screening,
  not continuous tachometer digitization or subjective listening acceptance.
- Original 48 kHz stereo Opus tracks were decoded directly to unfiltered,
  unscaled 32-bit float WAV. No denoising, EQ, normalization, trimming, sample-rate
  conversion or channel folding was applied to the measurement copies. A float
  decode preserves the supplied signal; it does not undo lossy encoding.
- Per-channel full-source powers are retained separately below. They include
  all recorded content and are neither isolated engine power nor calibrated
  sound-pressure levels. BMW's channels are effectively dual mono, not two
  independent microphone observations. No full-scale decoded samples occur;
  this alone does not prove an unclipped original recording.
- A separate 16 kHz per-channel copy uses the existing pinned Silero model only
  to flag speech risk. Subaru flags 1.568–1.632 s; BMW flags 0–0.032 s and
  2.368–2.400 s. Engine transients can trigger this screen; it is not proof of
  speech, music absence, engine isolation or a listening judgment.
- Public uploader metadata was read with download disabled; no further media
  was acquired. Local reference use does not establish redistribution or model
  training rights. Originals, raw decodes, 48 frames, processing commands,
  hashes and screening outputs remain under ignored
  `renders/evidence/stage-039-two-new-videos/`.

| Source | Raw L/R mean-square power | L/R peak magnitude | Channel correlation |
| --- | --- | --- | --- |
| Subaru |0.04243077 /0.04211303 |0.91147822 /0.92305714 |0.91041956 |
| BMW |0.02199231 /0.02199235 |0.98736686 /0.98736686 |0.99999991 |

No engine/exhaust settings, production code, session locks, accepted clips or
the RESULTS page change. No new training, fitting or held-out assignment occurs.
Only this candidate ledger, assessment, handoff and graph maintenance are in
scope; hold for Bernardo's next instruction.

## Review and preservation

All nine focused reference-protocol/video tests pass, including independent
channel power, clipping policy, source-family leakage, unknown-geometry claim
rejection and stereo decode. Both originals, both raw float fingerprints,
48 frame files, two session locks, 74 protected engine/browser/Claude files,
106 current result files including the manifest, and the RESULTS page are
unchanged. The 114-stage dependency graph remains acyclic, with 114 still
unstarted and required before 049. No native/runtime/listening qualification is
claimed from this reference-only assessment.

Graphify was queried before inspection and its code graph refreshed:
3228 nodes/9100 edges/171 communities. The existing `rb.h:415` parser warning,
inferred edges and stale semantic/community labels remain limitations. AST
refresh does not re-extract Markdown semantics. Commands, screen outputs,
preservation checks and `validation.json` remain in the ignored evidence folder.
