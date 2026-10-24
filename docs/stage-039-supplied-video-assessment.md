# Stage039 — six supplied candidate videos reviewed

2026-10-24; baseline8affe42. Bernardo supplies six local videos and asks whether
they fill the reference gap. This is an assessment, not authorization for041,
sound tuning, a new listening checkpoint or automatic closure of039.

## Decision

**They substantially improve the missing engine/exhaust coverage and provide
some approximate RPM labels. They do not yet complete the matched dynamic
recording requirement in039.** The strongest candidates are the Subaru
stock/muffler-delete comparison, BMW stock/straight-pipe comparison and BMW
330i hardware progression. Existing M3/S54 and GR86/FA24 recordings remain
related-family references; these new candidates are closer to the intended
M54B30 and FB20 families, subject to the identity limits below.

No continuous independently observed RPM trace through a loaded pull and long
coast was established. The exterior views help classify the scene; exact audio
microphone placement, gain/processing and complete operating conditions remain
unknown. These limitations prevent precise waveform/dynamic calibration. They
do not make the recordings useless for qualitative engine/exhaust comparisons.
Absolute SPL is a separate stronger claim requiring a calibrated signal chain.

## Source findings

Times below are approximate navigation windows, not admitted sample intervals.
On-screen setup/RPM labels and owner descriptions are attributed claims rather
than engine inspection, actuator telemetry or synchronized instrumentation.

| Supplied video | Useful evidence | Qualification and limits |
| --- | --- | --- |
| [Subaru stock versus custom muffler delete](https://www.youtube.com/watch?v=wUGmbzjB9U0), AJ Garage | Same visible Impreza, stock and removed-muffler views; stock revs roughly0–19s, modified stationary section roughly49–69s; captions identify2K and4K revs. Roadside driving follows each setup; later garage footage is separate. | Highest-priority boxer candidate. Uploader identifies GP7/G4-era fitment, consistent with the intended FB20 family. Actual engine code/build is not independently inspected; this is an Impreza recording, not proof of an identical XV/HKS system. RPM captions identify approximate rev targets, not a continuous tachometer trace or steady hold. Camera positions and sessions differ; same gain/distance is unproved. |
| [BMW330Ci stock versus straight pipe](https://www.youtube.com/watch?v=ABubhIqrKm4), Python Don | Visible stock, warm and3–4K-rev captions; title card around56s specifies twin2.25in pipes downstream of the catalysts. Stock stationary material roughly11–46s; modified stationary material roughly58–87s. | Highest-priority I6 candidate. Correct uploader-identified330Ci family rather than M3. Modified pipes are substantially better described than a generic straight-pipe title. No visible RPM trace; the stock rev caption must not automatically label all modified passages. Garage reflections, changed camera position, temperature, editing and unknown gain prevent a calibrated before/after level claim. |
| [BMW330i exhaust configuration comparison](https://www.youtube.com/watch?v=gUf-FNlyuqM), Pyrenees BMW | Visible captions distinguish OEM catalysts/line with Eisenmann Race rear section (around9–24s), OEM catalysts with Supersprint X/line and Eisenmann rear (around27–45s), then catalyst-free Supersprint headers/X/line with the same named rear section (around48–66s). | Strong I6 hardware/timbre candidate; uploader explicitly identifiesM54. The opening title is not evidence that any segment has a fully stock rear exhaust. Several components and camera/environment conditions change. The Eisenmann2×76 caption describes the advertised rear configuration; it is not a measured main-pipe bore. No RPM/load trace or microphone transfer established. |
| [BMW330Ci straight pipe versus X-pipe](https://www.youtube.com/watch?v=S3DCHs8mJtg), Python Don | Underbody hardware, labeled cold starts, stationary revs, acceleration and roadside pass-bys; stationary rev comparison roughly61–106s and moving passages later. | Useful system comparison, **not an isolated X-pipe experiment**: uploader description and visible hardware identify an added Magnaflow muffler as well. Owner reports about performance/overrun are subjective observations, not verified power or causal measurements. Same channel and visible car as the stock/straight video: retain one conservative source family if admitted; check reused footage before any split. No tachometer trace established; moving passages include distance/directivity/Doppler effects. |
| [BMW330Ci straight pipe at an event](https://www.youtube.com/watch?v=GqnUq5Y_fjs), GEPFLANZT AUTOMOBILE | Exterior stationary rear-quarter views in a busy covered event space, roughly0–31s. Uploader reports an event sound-level result. | Secondary texture/context candidate. Crowds, multiple cameras/instrument-like equipment and reverberation complicate source isolation. The reported125.2dB lacks verified RPM, microphone geometry, weighting/time statistic and calibration. Neither this scalar nor the decoded sample level calibrates our model. |
| [Subaru XV custom valved exhaust](https://www.youtube.com/watch?v=G_PgZoWrebE), HENG HENG EXHAUST | Short stationary rear view of a first-generation-looking XV with custom quad outlets, roughly0–15s; uploader identifies a custom system. | Secondary candidate. Exact engine/displacement, upstream hardware, dimensions, valve position and a controlled open/closed comparison are unverified. Do not label it a proven FB20 valve comparison solely from the title or infer valve state from loudness. |

Original uploader descriptions were read directly without acquiring further
media. Search-index dates/descriptions can differ from directly retrieved
metadata; the local snapshots retain the retrieval result. No owner performance
claim is promoted to a dynamometer finding.

## Inspection and processing record

- All six supplied originals are fingerprinted and remain unchanged at their
  original paths. No media is committed, moved, deleted or overwritten. Generated
  inspection material stays in ignored
  `renders/evidence/stage-039-six-video-screen-2026-10-24/`.
-125 timestamped sampled frames cover the six videos; enlarged caption frames
  confirm the three BMW330i hardware labels. Camera position is recorded
  separately from the unknown audio microphone location. Sampling is not a
  claim to have inspected every frame or recovered a continuous tachometer.
- Full first-audio-program decodes total550.848646s at original48kHz, float32,
  with both channels retained. These are decoded lossy Opus recordings, not
  lossless microphone masters. No EQ, denoising, pitch correction, normalization,
  distance compensation or waveform channel folding is applied.
- Four recordings are effectively dual mono. A two-channel container does not
  establish independent stereo or a usable spatial recording reference.
- Raw channel powers, peaks and channel correlation are measured separately;
  spectrograms average channel powers rather than waveforms. These are
  recording-quality screens, not a simulator comparison or acoustic fit.
  Five sources have decoded float samples above full scale; lossy-codec
  overshoot alone does not prove microphone clipping. Preserve float headroom.
- The previously pinned Silero model runs on a separate16kHz per-channel QA
  copy, with independent recurrent state and a0.35 threshold. Event BMW and
  short XV have speech-risk flags; the BMW stock comparison has one brief
  intro flag. The other three have no threshold crossings. This is screening,
  not verified speech content, absence of music, engine purity or a subjective
  listening-quality judgment. Engine sound may cause false positives.
- The old FFmpeg decoder cannot read the two AV1 video tracks. An isolated
  imageio-ffmpeg0.6.0 wheel supplies FFmpeg7.0.2 under`/tmp/`; hashes and
  commands are retained. No project, application, Claude or Python-environment
  dependency is changed. The final screen is bound to the final raw hashes.
- Rights remain user-supplied local reference use. No redistribution or model
  training permission is inferred. The six are now inspected candidates, not
  an unseen validation set. No frozen session assignment is changed.

The [machine-readable assessment](../tools/reference_candidates_supplied_20261024.json)
records source identities/hashes, claim limits, frame counts and raw-channel
screening results. Its decoded whole-source powers include all content and
must not be used as engine-only level targets. Existing v1/v2 session locks,
eleven admitted sessions/34 excerpts, reserved/quarantine recordings and the
single55-player RESULTS page stay unchanged. No new reference is admitted by
this assessment, and no engine is tuned.

## Remaining gap and handoff

The next useful material is a named M54B30/330i or FB20 build with a visible
tachometer during an uninterrupted pull and several seconds of off-throttle
coast. Driving/gear state and disclosed microphone location/gain would make
the claims stronger. Fixed-camera stationary holds are also useful. RPM
estimated from the same audio is an analysis hypothesis, not independent
validation; an approaching/receding car's pitch drop is not by itself a
measured engine deceleration.

Bernardo need not own a car, make a complete recording or obtain professional
calibration equipment to supply useful videos. Existing clips may fill
complementary qualitative gaps while retaining separate sessions; unrelated
takes must not become a fabricated synchronized measurement. Exact SPL and
the research I6's8500rpm range remain unsupported by ordinary stock clips.

Full039 stays **Awaiting matched references**. Its protocol remains implemented;
its acquisition gap stays in039.041 remains unstarted. No further recording
question is issued in this assessment. Preserve the accepted engine/exhaust
choices, Claude setup and user-added skills; save the review and hold.

Nine existing reference behavior tests pass without skips using the temporary
decoder. All six original fingerprints and final raw/screen bindings pass;
194 protected source/reference/Claude/page files remain unchanged. All113
canonical stage rows and both session/candidate lock revisions are unchanged.
No new native/WASM/browser/render run is claimed for this metadata-only review.
Graphify was queried and AST-refreshed to2784nodes/7559edges/158communities;
the known rb.h parser warning remains. Validation and reproduction evidence
remain local alongside the frame, metadata and audio screens.
