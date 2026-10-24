# Bernardo's engine sound examples — 2026-10-21

Six local MP3s supplied by Bernardo now provide 18 reference entries. The original
four contribute 16 selected engine-dominant excerpts; the two new cross V8 entries
retain full clips and unannotated operating states. They supplement the Freesound
collection; the simulator still generates its own audio from the physics.
Source audio is kept local in ignored renders/.

**2026-10-24 update:** [Three supplied videos](video-sound-references-2026-10-24.md)
add14selected stereo entries. There are now nine user-supplied recordings and
32user reference entries; the v2 protocol additionally includes the two separate
public preview sources. The original MP3 processing history below is preserved.

## What the references represent

| Recording | Preset relationship | Use and limits |
|---|---|---|
| Ferrari 458 Italia | v8-flat | Naturally aspirated flat-plane V8 character; different exact exhaust and calibration |
| 2024 Mustang GT | v8-cross | Naturally aspirated cross-plane V8 character; different exact exhaust and calibration |
| BMW M4 Akrapovic, 620 HP | i6 | Turbo inline-six texture; current I6 is naturally aspirated, so do not match its muffling or boost behavior yet |
| Golf 7 GTI, straight-piped | i4 | Turbo inline-four texture; current I4 is naturally aspirated |

The 2026-10-24 [video intake](video-sound-references-2026-10-24.md) adds two
GR86 FA24 boxer cabin sources and a naturally aspirated E46 M3 I6 source.
They supplement the six MP3s described here; the GR86 is not the FB20 preset.
For the original four recordings, the exact M4 generation, turbo setup,
microphone placement and recording processing are unverified. Titles establish
neither a calibrated RPM trace nor the microphone position. Manufacturer context:
[Ferrari 458](https://www.ferrari.com/en-PA/history/garage/2009/458-italia),
[Ferrari flat-plane V8 family](https://www.ferrari.com/en-GR/magazine/articles/bloodlines-special-series-V8s?amp=true),
[Volkswagen MkVII GTI](https://www.volkswagen-newsroom.com/en/new-golf-gti-international-press-presentation-2976),
[Ford 2024 Mustang specifications](https://media.ford.com/content/dam/fordmedia/North%20America/US/2022/09/14/2024-mustang-technical-specifications.pdf),
[BMW F82 M4 architecture](https://www.press.bmwgroup.com/usa/article/attachment/T0160684EN_US/391903).
The BMW document establishes that generation's architecture, not the identity
of the supplied recording.

## Selection and perspective

The full tracks were decoded without EQ, inspected with spectrograms, and
screened for speech with the official Silero ONNX VAD model (threshold 0.35,
approximately 0.55 seconds padding on either side). Selected 12-second windows
have no detected speech. Harmonic activity and changing engine pitch were used
to identify useful passages. This automated screen does not prove isolated
engine stems or exclude every other sound.

Each engine has two relatively brighter and two darker excerpts. These groups
describe the recording, not a confirmed front/rear/cabin location. Road noise,
wind, intake, turbo, exhaust and microphone filtering can all affect brightness.
Do not average these groups into a single EQ target or treat quieter/more
muffled sections as a defect in the engine. Stereo is retained in the previews.

| Source | Brighter source seconds | Darker source seconds |
|---|---|---|
| Ferrari | 212–224; 302–314 | 44–56; 125–137 |
| BMW | 152–164; 242–254 | 197–209; 659–671 |
| Golf | 53–65; 83–95 | 173–185; 239–251 |
| Mustang | 5–17; 197–209 | 74–86; 257–269 |

## Processing and reproduction

`tools/user_engine_reference_sources.json` stores the source IDs, exact windows,
groups and engine mapping. Original MP3s are unchanged.

`tools/prepare_user_references.py` creates:

- **Measurement excerpts:** 48 kHz mono float32 WAV, arithmetic L/R average,
  with no EQ, gain normalisation, denoising, gate or fade. These are the inputs
  for numerical comparisons, under renders/real/user-*.wav.
- **Listening previews:** four 12-second stereo excerpts per engine, a 0.7-second
  silence between excerpts, a gentle 12 Hz two-pole high-pass, 10 ms end fades,
  and one static peak gain per compilation. Each preview lasts 50.1 seconds.
  The encoded MP3 peak target is approximately -3 dBFS; MP3 encoding can shift
  the measured peak. No dynamic compression, pitch change or synthetic sound.
- **A separate CSV catalog:** tools/user_engine_references.csv, including source
  timestamps and inferred acoustic groups. The default Freesound catalog and
  its fetch command are unchanged. These local examples are not downloaded by
  the fetch command.

With FFmpeg installed, run from the repository root:

```sh
python tools/prepare_user_references.py --source-dir C:/Users/berna --ffmpeg /path/to/ffmpeg
python tools/compare_real.py compare --kind rev --catalog tools/user_engine_references.csv --engine v8-flat --place inferred-brighter 'renders/v8-flat-rev.wav@0-12:REV-BENCH flat V8'
```

Use `inferred-darker` separately; substitute the other preset keys as needed.
`--preview-dir` selects where the listening compilations and manifest are saved.
The supplied audio has not been added to Git. Its provenance stays in the
catalog rather than being given an invented open licence.

## How these examples guide the next sound work

The current comparisons are qualitative: RPM, throttle, modifications, camera
motion, microphone response and room/road conditions are not matched. Their
levels cannot establish absolute SPL, and the MP3s do not restore frequencies
removed by the original encoding.

- The Ferrari's brighter windows have -24 to -27 dB of their energy above
  2 kHz, the darker ones -33 to -35 dB. The flat-plane model's whole rev is
  about -27 dB: adding highs indiscriminately would miss the perspective effect.
- The Mustang's two groups span roughly -21 to -29 dB and -43 to -48 dB above
  2 kHz. The cross-plane model is about -23 dB. Again, the darker recording
  should not drive a universal low-pass on the model.
- The modeled whole-rev crest factors are roughly 21–25 dB versus 5–16 dB in
  the selected references. Their different drive envelopes and recording
  compression prevent calling this a calibrated error, but it supports checking
  isolated limiter/transient peaks by ear rather than only broad spectra.
- The BMW/Golf contain turbo and onboard filtering that the current naturally
  aspirated presets do not model. Use their pulse texture as a reference; defer
  exact turbo response, whistle and muffling to the forced-induction stages.

Preserve these group distinctions during S1/S2/S4 and later tuning. For a true
like-for-like test, obtain a known RPM/load stretch and verified listener position.
The outstanding S1 low-band repetition at idle still needs separate work; these
rev excerpts do not establish an idle target.

## R1b: optional filtered listening copies — 2026-10-21

Bernardo asked to try emphasizing the engine in these same four recordings,
accounting for possible cabin, exterior and passing-car sections. Sixteen
additional stereo float WAV excerpts and four filtered MP3 compilations are now
in `renders/user-reference-previews/filtered/`. The original MP3s, unfiltered
measurement excerpts, existing previews and comparison catalog are preserved.

This is conservative noise reduction, not source separation. Each excerpt uses
FFmpeg's two-pole 20 Hz high-pass followed by FFT denoising with a tracked noise
floor, a 6 dB reduction setting and initial noise floor of -50 dB:

```text
highpass=f=20:p=2,afftdn=nr=6:nf=-50:tn=1
```

The 6 dB setting is deliberately modest because an engine's exhaust, intake and
turbo also contain broadband noise. Road or wind overlapping the engine remains;
some engine texture can also be reduced. No speech-removal or perfectly isolated
engine claim is made. Existing speech-screened source intervals are reused.

Stereo and the original pitch/motion are retained. There is no Doppler correction,
attempt to undo cabin filtering, or shared brightness correction across engines.
The BMW title suggests a POV/onboard recording, but neither that title nor a
darker spectrum verifies the camera position for a selected window. The source
metadata now explicitly marks viewpoints as unverified; cabin/exterior/pass-by
are possible contexts, not assigned labels. Keep the brighter/darker groups
separate when comparing.

The float listening excerpts retain the filtered source level and use 10 ms end
fades. Compilation order is unchanged, with three 0.7-second silence gaps and
one static gain targeting -3 dBFS before 192 kbit/s MP3 encoding. There is no gate,
dynamic compressor or pitch shift. Encoding can move peaks slightly. The separate
`filtered-listening-manifest.json` records source SHA-256 hashes, source intervals,
viewpoint uncertainty, exact filter chains, compilation positions and static gain.
These filtered copies must **never** become inputs to spectrum/repetition targets
or engine calibration; continue using the unfiltered float files in `renders/real`.

Each engine also has a `<key>-raw-then-filtered.mp3` comparison. Its first
12 seconds are the first source interval with end fades only, followed by a
0.7-second gap and the same interval filtered. A single gain is shared across
both halves, preserving the effect on level. These 24.7-second pairs make the
tradeoff audible without comparing different revs or recording perspectives.

To create only these additional copies, without rewriting the measurement files:

```sh
python tools/prepare_user_references.py --source-dir C:/Users/berna --ffmpeg /path/to/ffmpeg --filtered-only
```

Use `--filtered-listening` during a full preparation to create both sets, or
`--filtered-dir` for another listening-output directory.

## Validation

All 16 WAVs decode as 12-second, mono, 48 kHz float32 files with finite samples.
All four previews decode as 50.1-second stereo files. Source ordering, manifest
positions and the three 0.7-second gaps were checked. The analysis suite now has
nine passing tests, including custom catalog filtering by engine and acoustic
group. Brighter/darker plots were generated separately for each preset family.

R1b was generated with native Linux FFmpeg 4.3.1 using isolated library links;
Claude's setup and installed tools were unchanged. All 16 filtered float excerpts
decode as finite 12-second stereo 48 kHz audio; all four filtered MP3s decode as
50.1 seconds of finite stereo audio. Original source hashes match the filtered
manifest. A real-FFmpeg integration test checks source/catalog/ordinary-preview
preservation, stereo channel differences, exact excerpt length, the retained
engine-like harmonic and attenuated subsonic rumble. Set `RB_REFERENCE_FFMPEG`
to a usable FFmpeg path when the default executable is unavailable; otherwise
that optional integration test is skipped.

Before compilation gain, the new excerpts' 1–20 Hz energy is reduced by about
4–10 dB. Their 20–500 Hz energy changes by less than 0.11 dB, while 2–8 kHz drops
by up to 2.05 dB. These are processing checks against the corresponding existing
unfiltered mono excerpts, not improved simulation scores or proof of engine
isolation. The four original MP3s, sixteen measurement WAVs, four ordinary MP3
previews and comparison catalog are SHA-256 checked before and after a filtered
preparation; all 25 remain byte-identical. Checks and exact hashes are retained
in the ignored filtered-output directory.

The previews are engine-dominant excerpts. Overlapping road/wind and other
mechanical sounds are not perfectly separated; the cleanup is not claimed to
produce studio-isolated engine stems.

## Additional rear cross V8 recordings — 2026-10-23

Bernardo supplies two further clips for **v8-cross**. He states that **both were
recorded outside behind the car, approximately 3 m from the tailpipes**. This
is stronger viewpoint evidence than the inferred brighter/darker groups above,
but is not a calibrated measurement of microphone geometry. Car/engine model,
exhaust, RPM/load, motion, microphone response, automatic gain, edits and ground
reflections remain unverified. The handles do not establish vehicle identity.

| Source MP3 | Preserved source interval | 48-kHz decoded frames | Catalog ID |
| --- | --- | ---: | --- |
| Video by rcf.lorax [DeztBYHSkCj].mp3 | 0–52.012708 s, full decoded clip | 2,496,610 | user-rcf-lorax-rear-3m-0 |
| Video by viper_ws9 [DewsrGHTcOZ].mp3 | 0–15.185854 s, full decoded clip | 728,921 | user-viper-ws9-rear-3m-0 |

Decoded duration excludes MP3 codec padding; interval endpoints are rounded to
microseconds. Source definitions, SHA-256 fingerprints and perspective evidence
are in `tools/rear_v8_reference_sources.json`. Both entries append to
`tools/user_engine_references.csv` with `place=user-reported-rear-3m`. Their
`kind=rev` is a broad catalog selection category, **not verified operating-state
annotation**. No idle, load, lift-off or coast timestamps or speech-screening
results are asserted for these new clips. Select and inspect intervals before
using them as phase-specific targets. Their hosting platform and source URLs
are unverified; the catalog does not invent a YouTube URL from the filenames.
The supplied originals remain unchanged and redistribution is not granted.

### Processing and distance

- Measurements: `renders/real/user-{rcf-lorax,viper-ws9}-rear-3m-0.wav`, full
  source-level **48-kHz stereo IEEE float32**. Decode/resample only; no EQ,
  denoising, gain, fade or channel averaging. Resampling the 44.1-kHz MP3s
  adds no original bandwidth or fidelity. The old 16 mono measurement excerpts
  are preserved; new stereo sources avoid cancellation from folding channels.
- Lightly filtered listening copies:
  `renders/user-reference-previews/rear-v8/filtered/`. Same conservative
  `highpass=f=20:p=2,afftdn=nr=6:nf=-50:tn=1` chain as R1b, followed by 10-ms
  end fades. A 100-ms zero pad flushes the FFT filter, then the output is
  trimmed to the exact source interval **before** fading. This prevents the
  incomplete final FFT block from truncating the recording; duration and
  unfiltered source samples are checked separately.
- Original-then-filtered pairs: each full original listening segment, 0.7 s of
  silence, then the corresponding filtered segment. The original half has
  only 10-ms end fades. Both halves share one static gain targeting −3 dBFS
  before 192-kbit/s MP3 encoding; there is no compression, gating, pitch shift
  or time stretching. Gains differ between recordings, so pair loudness is
  not a source-to-source or absolute-SPL comparison. Manifests record gains,
  exact processing and sequence times.
- **Distance is context, not a corrective EQ target.** Keep these in their
  rear/approximately-3-m group. No 3-m-to-1-m gain boost, brightness restoration,
  cabin inverse filter or Doppler correction is applied. Unknown mic gain,
  reflections, direction and motion prevent that calibration. For later
  comparisons, render the virtual listener with corresponding geometry and
  inspect the remaining recording uncertainty instead of forcing a match.
- Filtering is a modest listening aid, not an isolated engine stem. Overlapping
  wind, speech, road/ambient noise or music may remain; broadband engine texture
  can also be reduced. **Use only unfiltered float WAVs for measurements**, with
  channel powers measured separately before averaging. `compare_real.py` still
  folds stereo to mono; use `audio_wav.read_wav` and the S2c per-channel analysis
  for these new sources until that legacy analysis limitation is addressed.

### Reproduction and checkpoint

From the repository root, with an executable FFmpeg:

```sh
python tools/prepare_user_references.py --source-dir /mnt/c/Users/berna \
  --ffmpeg /path/to/ffmpeg --config tools/rear_v8_reference_sources.json \
  --append-catalog --preview-dir renders/user-reference-previews/rear-v8 \
  --filtered-listening --filtered-dir renders/user-reference-previews/rear-v8/filtered \
  --publish-filtered
```

The command verifies supplied source fingerprints before writing, preserves
existing catalog rows and adds no duplicate IDs on an identical rerun. Use
`--filtered-only` in place of `--filtered-listening` to regenerate only listening
artifacts. The results page must already exist: publication adds a marked
reference section and retains all engine examples, long coasts and research
plots. Both matched pairs are embedded in the existing `renders/RESULTS.html`
under **New references**, so no separate results page is introduced.

Preservation hashes and signal checks are retained locally in
`renders/user-reference-previews/rear-v8/validation.json` and
`preservation-before.json`; playback checks are in `browser-validation.json`.
Production sound, engine settings and acoustic-renderer development are unchanged.
**Hold for Bernardo's next instruction after this import.**

Validation: both raw stereo float WAVs equal a direct decode/resample of their
supplied MP3s sample for sample. Filtered WAVs retain the same complete frame
counts and are finite. The 108 previously hashed files (including accepted WAVs,
old reference audio and Claude setup) and all 46 existing embedded preview
payloads are unchanged; the page now has 48 playable recordings. Original MP3
fingerprints match. The catalog has 18 unique entries with its original rows
preserved.

Measured listening-filter changes, with channel powers averaged after spectral
analysis (8192-sample Hann blocks, first/last 100 ms omitted):

| Clip | 6–20 Hz | 20–500 Hz | 500–2000 Hz | 2–8 kHz | 8–16 kHz |
| --- | ---: | ---: | ---: | ---: | ---: |
| rcf.lorax | −4.260 dB | −0.008 dB | −0.001 dB | −0.020 dB | −0.040 dB |
| viper_ws9 | −4.655 dB | −0.004 dB | approximately 0 dB | −0.015 dB | −0.183 dB |

These quantify this light listening treatment, not engine isolation or calibrated
distance response. Native tests pass (189 tests / 4820 checks), Python analysis
tests pass (38, no skips, including real-FFmpeg stereo/catalog and fractional-end
regressions), and web unit tests pass (17). Offline Chromium checks all 48
players, exclusive playback, four original top-band seek buttons and six retained
research plots. Graphify's AST graph was refreshed to 2159 nodes / 5933 edges /
112 communities; the existing `rb.h:415` extraction limitation remains. No
semantic extraction of these documentation/data additions is claimed.

## Stage039 canonical reference protocol

[The session lock and measurement protocol](stage-039.md) now decode selected
source-level stereo float excerpts separately in `renders/reference-protocol/raw`.
Use those per-channel powers for future quantitative work. The16 legacy mono
files above stay preserved for historical reproduction; they cannot recover
stereo cancellation and are not stereo calibration authority. Existing supplied
recordings and all17 previously used Freesound sources are development data.
The new GM-I6 session is reserved validation; the sparse Subaru candidate stays
quarantined. Neither provides calibrated SPL or a measured RPM/load trace.
Four common-gain/equal-RMS development controls are in the same RESULTS page.


## 2026-10-24 supplied-video update

[Three reviewed videos](video-sound-references-2026-10-24.md) add14selected
stereo excerpts:8GR86 cabin passages and6E46 exterior passages. Workshop
presentation, car washing and intro/cinematic material are excluded from the
selected excerpts. E46 stationary open/closed captions, roadside pass-bys and
near-tailpipe camera views are separate source classes. Microphone placement
is not inferred from the camera. Dialogue screening is not a pure engine stem.

The authoritative [v2 source lock](../tools/reference_sessions_v2.json) retains
all prior assignments and reserves the new videos for development reference.
Unfiltered stereo float measurements live in `renders/reference-protocol-v2/`;
three source compilations and viewpoint images join the same RESULTS page.
The GR86 ASC setting, exact RPM/load and calibrated microphone data remain
unknown. These references are useful now for texture/dynamics/cabin observations;
Stage039's matched calibration remains open. No simulator tuning or041 work
is part of this intake.
