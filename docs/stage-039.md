# Stage039 — Matched references and calibration protocol

Execution begins2026-10-23 after review of038; baseline1f964f4.
Authorized scope continues through040, reviewing each predecessor before the
next completion. Claude setup and accepted engine examples remain protected.

## Review038

Reviewed the optional tap lifetime, capacity preflight, signed transfers,
impulse units, bounded traces, invalid-value accounting, junction inspection,
whole-engine books and native/WASM evidence. No blocking regression found
within its prepared-engine scope. Impulse accounts are not globally conserved
0D momentum; binding-relative ticks need an offset; saturated traces retain
early events; short cold-state checks do not qualify a warm operating map.
These declared limits remain future integration requirements. The existing
3–8% disabled-path timing overhead is retained as a performance follow-up.

## Frozen reference rules

Unmodified supplied compressed originals and unfiltered float channel samples
are measurement authority. Do not average L/R waveforms before measuring power;
report each channel and average powers. Legacy mono excerpts cannot establish
stereo or recover cancellations; preserve them and decode new stereo excerpts
from originals. Reference level is dBFS, never calibrated Pa/SPL without known
sensitivity, gain and geometry. Keep RPM/load/exhaust/viewpoint unknown where
unsupported. Brighter/darker is not a microphone position. No undoing cabin
filtering, distance EQ, codec damage or pass-by Doppler.

All existing tuning recordings are development data. Slicing the same source
or relabeling already-heard files cannot create an independent held-out set.
Group all excerpts and known reuploads by recording session/source family;
freeze assignment and content fingerprints before evaluating engine changes.
New sources with unknown cross-session independence may be quarantined. Unknown
hardware or operating state can support qualitative texture observations only.

Audition modes: one shared gain per pair preserves recorded level relationships;
a separate equal-RMS pair compares texture and explicitly discards level
relationships. Both use finite constant gains with a shared peak-headroom cap,
no limiting/EQ/denoising. Measurement WAVs never receive those gains. Filtered
historical previews remain listening aids, excluded from calibration.

## Gates

Require versioned source/session/split lock, exact sample intervals, rights and
processing ledger, per-channel time/band power, missing-metadata coverage, and
behavioral rejection of session leakage, changed fingerprints and unjustified
SPL/geometry claims. An anti-phase stereo control must retain channel power;
a same-sound unequal-level pair must retain its ratio under common gain and
lose that ratio only under labeled level matching. Validate real-source stereo
excerpts against source decode. No threshold can promote unknown hardware/RPM
into a matched calibration reference.

The currently missing decisive examples are naturally aspirated I6 and boxer
recordings with a known exhaust, independently stated RPM/load and microphone
position/distance. Asked Bernardo for targeted links/details; research and
independent040 fixtures continue while that evidence is pending.

## Implementation and acquired evidence

`tools/reference_sessions_v1.json` freezes eight source/session entries and all
17 historically used Freesound source IDs. Six supplied sessions/18 excerpts
remain development data. A previously unused GM inline-six session is validation;
the new Subaru-labelled session is quarantined, not a qualified FB20 target.
The two ambiguous rear V8 sources share the Mustang's conservative uncertainty
family; none is split into validation. Lock before any engine comparisons;
validation metadata/power QA is allowed, fitting/listening-based tuning is not.

- [Ears68's GM/Holden race six](https://freesound.org/people/Ears68/sounds/185151/):
  author describes carburettors, open rear headers, a hillclimb and a DR-05
  recorder. Carburettors suggest NA; induction is not independently verified.
  Preview decoded full interval0–61.137417s, stereo48kHz float. RPM, microphone
  distance and load trace are unknown. CC0 verified on the source page.
- [mario1298's Subaru-labelled recording](https://freesound.org/people/mario1298/sounds/181233/):
  sparse uploader metadata, including an unverified year/model combination;
  no verified induction/exhaust/state/position. Full preview0–13.869667s,
  stereo48kHz float, CC0. Quarantine it rather than call it a known NA boxer.
- [The previously catalogued Toyota1G-FE](https://freesound.org/people/qopsinonstudios/sounds/768643/)
  has author-reported vehicle/recorder/cold-start metadata, but still lacks
  matched exhaust, distance and RPM/load. It was already used historically;
  reclassifying it as fresh validation would leak development evidence.

No platform login or original-WAV access is implied: newly acquired public
previews are compressed MP3s. Their float decodes restore neither lost bandwidth
nor the original bit depth. Author descriptions are contextual evidence, not
metrology. Source pages and acquisition hashes are retained locally; rights,
source URLs, source intervals and source fingerprints are committed in the lock.
The synthetic [procedural dataset](https://rdoerfler.github.io/procedural-engine-sounds-page/)
is not imported as real reference audio: its annotated synthesis cannot close
these real-recording calibration gaps. Other located corpora/library descriptions
did not provide a verified matching NA I6/FB20 set with all required metadata.

## Measurements and audition evidence

`tools/reference_protocol.py` verifies every source fingerprint before decoding,
keeps original channels, decodes/resamples once, slices exact48kHz sample frames,
and verifies saved float samples against that decode. Twenty raw references are
now available in `renders/reference-protocol/raw/`, separate from unchanged
legacy mono files. Reports include both channel powers/peaks, RMS dBFS,
per-channel Hann/8192-bin band powers and averaged powers. Folded-mono power is
reported only as a cancellation diagnostic. Version1 refuses calibrated-SPL or
matched-engine qualification claims, even if a metadata field is populated.

| New reference | L RMS dBFS | R RMS dBFS | Mean channel power | Folded mono power |
| --- | ---: | ---: | ---: | ---: |
| GM six |−12.821|−11.923|0.0582295|0.0570933|
| Subaru candidate |−35.194|−35.194|0.000302424|0.000302424|

Those values describe decoded digital recordings, not engine SPL or relative
engine loudness. Anti-phase100Hz control retains0.02 power per channel and
0.02 averaged power while folded mono is zero. A pair differing by4× amplitude
retains16× power ratio under common gain; equal-RMS mode yields1×. Listening
conversion rejects peaks beyond full scale; measurement float retains±2 exactly.

Four4s **development-only** controls (two Ferrari excerpts, two gain modes) are
registered in the same `renders/RESULTS.html`, under Reference controls. Shared
gain preserves recorded relationships; equal-RMS mode intentionally removes
them. No new validation or quarantined recording is exposed as a tuning audition.
All earlier48 payloads, current34 engine/tone WAVs and original supplied/reference
files are preserved. There are now52 players in the one results page.

## Reproduction and gates

```sh
PYTHONPATH=tools .venv-analysis/bin/python tools/reference_protocol.py \
  --source-dir /mnt/c/Users/berna --ffmpeg /path/to/ffmpeg --publish
PYTHONPATH=tools .venv-analysis/bin/python -m unittest test_reference_protocol
python3 -S tools/render_results.py --publish-only
```

The checked source URLs may be downloaded into the lock's ignored `sources/`
paths; verify SHA-256 before preparation. No tool silently obtains unknown
private sources. FFmpeg on this machine uses the existing isolated library path;
no Claude tools/settings were installed, removed or upgraded.

Six behavioral tests cover anti-phase power, gain policies, source/family leakage,
seen-data rejection, unsupported SPL, safe sample intervals/IDs and float
preservation. Five compiled Python mutants are rejected: folding channels before
power, level-matching the common-gain arm, permitting previously used validation,
ignoring session/fingerprint leakage, and clipping measurement floats. An initial
fault-text replacement matched another expression and failed compilation; it
was corrected before the five behavioral mutation results were counted.

Final verification/evidence: `renders/evidence/stage-039/`, with coverage,
source pages/download fingerprints, reports, faults, preservation and logs.
All202 native tests/38814checks pass. Python64 and web17 pass; full offline
playback/rebuild and preservation are checked before the checkpoint commit.

## Outcome and review handoff

The reference acquisition/uncertainty ledger, locked session partition and
measurement/audition protocol are implemented. **Matched calibration remains
unqualified:** no known exhaust/RPM/load/microphone calibration set was supplied
or acquired. Stage039 stays awaiting matched references; its missing data is not
silently waived. Bernardo authorizes public-source research first and has no additional recordings
currently. The [public candidate ledger](../tools/reference_candidates_v1.json)
records YouTube/Instagram/owner/manufacturer leads, not qualified calibration data.
No request to supply recordings remains pending.

Review this protocol before040 completion. In particular: no retroactive held-out
claims, no Subaru-model inference from its title, no comparison using filtered
or waveform-averaged targets, and no use of the new validation session to retune
the engine. The roadmap explicitly allows independent numerical fixtures to
continue while decisive calibration data is missing;040 depends on038, not039.

## Public-source follow-up and review before040

The owner’s [FB20/factory-muffler post](https://minkara.carview.co.jp/userid/956048/car/1531576/6544040/parts.aspx)
links [YouTube](https://www.youtube.com/watch?v=CEjUwubjXPQ), but has descriptive
RPM observations rather than synchronized telemetry or microphone dimensions.
An E46 owner’s [replica-exhaust thread](https://forums.m3cutters.co.uk/threads/rep-supersprint-exhaust-ltw-race.232254/page-4)
links [Instagram](https://www.instagram.com/p/CJ4AAgUJ4ez/); a stated maximum rev
does not resolve load or microphone transfer. These remain qualitative leads.

The [S54 recorder’s primary post](https://nam3forum.com/forums/forum/main-forum/e46-2001-2006/161428-s54-engine-sound-recording-with-video)
provides a stronger acquisition lead: two SM11s and an SM57 into a Zoom F6,
with intake/engine/exhaust placements described. The author says channel levels
were changed and not remembered. A later dashboard-video announcement and a
current exhaust signature do not establish time-aligned RPM, original channel
gains or the exhaust fitted at recording time. The linked SoundCloud mix cannot
be treated as an isolated calibrated tailpipe. No media from these candidates
was downloaded, auditioned, redistributed or used for tuning/training. Rights
remain unknown. A manufacturer video link returned404 on direct repeat verification; its
[catalog](https://www.supersprint.com/en-gb/supersprint-performance-sport-exhaust-for-toyota-gt86.aspx)
still lists component-specific stock/full-system comparisons. These FA20
examples are a separate boxer architecture, not verified FB20 calibration.

Pre040 review: six protocol tests and five behavior faults support the locked
partition, stereo-power and safe-gain rules. Full preparation/rebuild preserves
all earlier recordings. No blocking protocol defect found; calibration remains
open for source metadata and rights. Continue039 on public sources when new
leads provide the missing information; ask Bernardo later only if it becomes
necessary. Independent040 never fits against these candidates or the untouched
new validation/quarantine excerpts.


## 2026-10-24 user-supplied video intake

Bernardo supplied two GR86 videos and a Valvetronic Designs E46 M3 video,
with explicit cabin-audio and non-car-content guidance. The
[intake record](video-sound-references-2026-10-24.md) documents visual scene
review, per-channel speech screening,14selected raw stereo excerpts and three
local listening compilations. [Lockv2](../tools/reference_sessions_v2.json)
preserves all eight v1 sessions unchanged and adds three development sources;
none becomes unseen validation. The default protocol now usesv2 and explicitly
selects the first audio track from video containers.

This supplies useful NA I6/exhaust-valve and related NA boxer/cabin references.
The GR86's FA24 differs from the FB20 preset, and cabin ASC state is unknown.
The E46 footage distinguishes source-reported open/closed valves and camera
views, while RPM/load, complete exhaust geometry and microphone calibration
remain unknown. Stage039 is still awaiting matched calibration, with better
usable references now available. No additional recording request is pending;
no engine sound or later numerical stage is changed by this intake.
