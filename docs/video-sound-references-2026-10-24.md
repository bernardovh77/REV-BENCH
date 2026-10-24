# GR86 and E46 video references — 2026-10-24

Bernardo supplied three local WebM videos for reference and asked that their
images guide perspective annotation. He reports that most/all GR86 driving
sound is inside the cabin and warns that the E46 has substantial non-car content.
This work curates the recordings; it changes no engine, exhaust or sound model
and does not start Stage041. Stage039 calibration remains open.

## Source identity and usefulness

| Source | Known context | Intended use and remaining uncertainty |
| --- | --- | --- |
| [GR86 mountain run, DriveVibe](https://www.youtube.com/watch?v=ve9CSQ0d1xw) | GR86 identified by uploader and visible car; selected footage is cabin POV | Related NA boxer cabin texture and driving transitions. Exhaust hardware, microphone, gain, tune, RPM/load and ASC state unknown. The title's “full exhaust sounds” is not proof of an aftermarket full exhaust system. |
| [GR86 AWE Touring, AZAR](https://www.youtube.com/watch?v=AWZnjjcWIbk) | Uploader identifies a2025 GR86 Premium and AWE Touring cat-back with Diamond Black tips; selected footage is driver POV | Related NA boxer cabin response with a named rear system. Upstream exhaust, microphone/window state, gain, tune, RPM/load and ASC state remain unverified. |
| [E46 M3, Valvetronic Designs](https://www.youtube.com/watch?v=0vfvIaQLIS0) | Manufacturer's video identifies Section3+headers; visible E46 M3, separate open/closed valve captions and exterior/roadside/near-tailpipe views | Much closer NA I6 family reference than the earlier turbo M4. Header specification, sections1/2, tune, exact RPM/load, distance, microphone response and gain remain unknown. Valve captions are uploader claims, not independent actuator telemetry. |

The [Toyota specification](https://www.toyota.com/content/dam/toyota/brochures/pdf/2024/gr86_ebrochure.pdf)
identifies the GR86's2.4-litre FA24 naturally aspirated boxer. Our current boxer
preset is a2.0-litre FB20. Related architecture is useful; it does not establish
an exact engine/exhaust calibration target. The GR86
[owner manual's ASC section](https://assets.sia.toyota.com/publications/en/om-s/OM18155U/pdf/OM18155U.pdf)
also documents Active Sound Control contributing engine sound. Its state in these
recordings is unknown. Do not force our physical tailpipe model to reproduce all
of this cabin energy, or infer that a physical exhaust is responsible for every
high-frequency feature or pop. The AWE video's pops/bangs title does not identify
afterfire, fuel-cut logic or a verified stock ECU state.

All files are supplied for local reference. Their public availability and the
mountain video's separate music credit do not grant rights to redistribute the
videos or train a model. Original file paths/bytes are preserved. Same-volume
hardlinks under ignored `renders/video-references/sources/` provide the source
library without duplicating4.4GB. The three original root filenames are also
explicitly ignored; no media enters Git. Never write either hardlink name.

## Visual review and selected source windows

Sampled frames across each full video establish scene context; closer frames
inspect the selected passages. Images show camera viewpoint. They do not prove
where an external microphone was attached or whether audio was replaced/edited.
Bernardo's GR86 cabin-audio description is retained independently of the images.
Do not infer distance from brightness or undo cabin response/Doppler with EQ.

| Recording | Source seconds | Observed camera context / exhaust caption |
| --- | --- | --- |
| GR86 mountain |130–142;172–184;232–244;296–308| Cabin POV, with differing camera angles across the source. No exterior audio class is inferred. |
| GR86 AWE |82–94;174–186;306–318;530–542| Driver cabin POV. Intro garage/startup/crossfade is excluded. |
| E46 |184–192| Stationary rear-quarter; uploader label valves OPEN. |
| E46 |198–200| Brief stationary rear-quarter; uploader label valves CLOSED. Shortened before the truck visible at203s; ambient traffic may still be present. |
| E46 |232–244| Roadside pass-by; uploader label valves CLOSED. Motion and Doppler are retained. |
| E46 |252–264| Roadside pass-by; uploader label valves OPEN. Not matched RPM/load to the closed take. |
| E46 |480–492;520–532| Exterior view mounted beside/down toward tailpipes. Microphone attachment/distance and valve state remain unknown; wind/road contamination is possible. |

The mountain video's early car-wash sequence and cinematic shots are excluded;
its uploader credits music, so absence of music in any specific passage is not
independently certified by a speech detector. The E46 presenter-led workshop
and in-cabin commentary sections are excluded. The E46 tailpipe view is a
separate source class from its stationary rear-quarter and roadside recordings.
No “engine-only stem” or perfectly clean background is claimed.

All source times are exact sample selections on the decoded48kHz audio clock.
Video-frame times are observational context, not synchronized RPM/load labels.
The containers report video start0 and Opus start−7ms; decoder priming and first
frame metadata are recorded. No invented timing correction is applied. These
small codec/container offsets preclude a sample-exact audiovisual timing claim.
Visible gauges are not promoted to a verified RPM/load trace.

## Audio screening and measurement authority

Each source is decoded without EQ, denoising, normalization, gating, pitch or
channel mixing. Measurement excerpts retain both original channels as48kHz
float32. This is decoded Opus, not recovered lossless master audio. Per-channel
time/band powers are computed before averaging powers, using the existing039
protocol. Waveform-averaged mono is only a cancellation diagnostic.

A separate16kHz stereo QA copy is screened with the official
[Silero model](https://github.com/snakers4/silero-vad/tree/60b7ffa243625ebdc1070275a29f18c87843786a)
(MIT, pinned commit and model SHA recorded locally). CPU inference processes L/R
separately in a two-channel batch with separate recurrent states,512-sample
windows and64-sample context. Source state resets between videos. Any channel
at/above0.35 rejects a candidate, with0.55s padding. The model catches the E46's
known speech control. False negatives and non-speech music remain possible;
visual exclusion supplements the screen. QA resampling never changes the raw
measurement files. Model/runtime are isolated analysis tools, not app dependencies.

The v2 source lock freezes the new development references and preserves all
v1 sessions/intervals/partitions unchanged. Both GR86 sources share one conservative
source family; none is claimed as an unseen validation recording. The GM
validation and uncertain Subaru quarantine remain untouched by listening-based
engine tuning. Our current action performs no such tuning.

## Reproduction and results

- [Version2 lock](../tools/reference_sessions_v2.json):11sessions/34intervals,
  including the prior8sessions/20intervals and these3sources/14new excerpts.
- [Video producer](../tools/prepare_video_references.py): verifies ancestor lock,
  source-bound per-channel speech checks, raw preparation and listening previews.
- [Reference protocol](../tools/reference_protocol.py): explicitly selects audio
  track0 from a video container; retains channels and rejects changed hashes.
- Local evidence: `renders/evidence/video-references-2026-10-24/` contains source
  probes/hashes, frame review, decoder/model/runtime provenance, speech scores,
  selection corrections, commands and preservation/test/playback reports.
- Authoritative new raw files/report: `renders/reference-protocol-v2/`.
  The old v1 raw files and accepted engine checkpoint are preserved.

```sh
PYTHONPATH=tools python tools/prepare_video_references.py \
  --source-dir /path/to/original-mp3-folder --ffmpeg /path/to/ffmpeg --publish
python3 -S tools/render_results.py --publish-only
```

The producer consumes the locally recorded source-bound speech QA files; no
model or private recording is silently downloaded. Exact screen/frame commands
and pinned model files are retained in ignored evidence. A fresh machine must
obtain the supplied videos and repeat the screen before preparing the locked
intervals. The local isolated Python/FFmpeg library paths are machine details,
not new Claude or application setup requirements.

Each source has one stereo listening compilation in the existing single
`renders/RESULTS.html`, under the video references section. Excerpts keep source
order, with0.7s gaps and10ms endpoint fades; one fixed gain per source maintains
its internal dynamics and headroom.192kbps MP3 is a listening convenience.
Different source compilations have independent gains and cannot establish
absolute or relative engine loudness. No audio separation, distance compensation
or corrective EQ has been applied.


## Verified intake checkpoint

Native206tests/694149checks, Python67tests and web17tests pass. Three new
behavior tests cover ancestor/session preservation and FA24/FB20 distinction,
padded speech in either channel, and a video with two audio tracks whose default
program would be wrong. Four temporary faulty copies are rejected: changed
parent, false exact-engine mapping, ignored right-channel speech and automatic
wrong-track decoding. No working-tree fault is retained.

All14selected measurement excerpts match the corresponding full original-channel
source decode exactly, including the shortened closed-valve prefix. All20prior
raw reference hashes and52old embedded audio payloads remain unchanged. The
single-page rebuild and all55offline playback controls, exclusive playback,
four seek buttons, eight previous plots and three new viewpoint panels pass.
The protected176raw/reference/core/WASM/Claude files are unchanged; only the
expected generated results manifest is refreshed. Originals remain at all three
supplied paths. Full-length intermediate WAVs/16kHz QA WAVs can be regenerated
from the recorded commands; they are removed after proof to avoid duplicate audio.
Selected raw excerpts, source videos, scores/hashes/models and visual evidence
remain local. The3new previews total161.7s.

Graphify was queried and AST-refreshed to2753nodes/7515edges/161communities;
its known rb.h parse warning remains. Source, documentation and small metadata
are committed/synced; no video/audio/frame/model is uploaded to GitHub. Hold
for instruction after this intake. Stage039 matched calibration,041 onward and
the parent realism/listening gates remain open.
