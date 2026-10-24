# Stage039 — expanded reference search and qualification review

2026-10-24; baseline81c456b. Bernardo requests extensive public-source research
before041, including different manufacturers, exhaust types and modifications,
then an attempt to finish039. If decisive references remain unavailable, he may
look for them later. This request does not start041 or authorize engine tuning.

## Outcome

The search produces a substantially broader, documented reference portfolio:
**58 selected source records,23 exhaust manufacturers/brands, all five engine
configurations,20 search batches/76 targeted queries.** These are catalogs,
measurement reports, documents and media leads, not58 acquired recordings.
The [candidate catalogv2](../tools/reference_candidates_v2.json) preserves the
earlier [v1 leads](../tools/reference_candidates_v1.json) by reference and hash;
the frozen recording/session locks are unchanged.

Several sources improve geometry, state and scalar-noise evidence. They do
**not** supply a complete matched acoustic-recording set with independent
RPM/operating state and microphone conditions. **Full039 remains awaiting
matched references.** Its existing processing/split/audition protocol is
implemented and reviewed. The previous bounded closure was too broad for the
full acquisition deliverable; it is superseded, not used to waive this gap or
move acquisition out of039. PLAN is the status authority.

No new candidate audio was acquired, auditioned or used to tune a model.
UUC explicitly offers downloadable330i videos, but both relevant links return404.
Two manufacturer-linked Akrapovič PDFs were acquired and visually checked as
documents. Existing eleven audio source sessions/34 excerpts,55-player RESULTS,
engine/exhaust choices, WASM and Claude setup remain unchanged.

## Search method and claim boundaries

Search covered English, Japanese and German primary manufacturer catalogs,
installation/approval documents, OEM archives, official YouTube demonstrations,
owner/installer reports, recording libraries and research datasets. It includes
positive and rejected leads; it is not a claim to have exhausted the internet.
Direct failures and index-only access remain labeled in the ledger. A returned
HTTP200 with an empty body is an unavailable source, not a successful read.

The qualification rules stay those established before the search:

| Evidence type | Permitted use | What it does not establish |
| --- | --- | --- |
| OEM/model or catalog/installation document | Engine-family identity, component choice, published dimensions and topology | Recorded timbre, surveyed proprietary internals, or sound calibration |
| Manufacturer scalar noise/dyno data | Attribute a reported number to its named system and stated test condition; retain uncertainty | A calibrated audio waveform, synchronized driving trace or another brand's setup |
| General test-method description | Design a corresponding future fixture and interpret the named metric | Proof that an online video or every product-table row followed that procedure |
| Official/owner sound demo | Candidate qualitative comparison after access, perspective and content review | Known microphone gain, independently synchronized RPM/load or isolated engine audio |
| Raw qualified recording | Claim-specific waveform/order/envelope comparison within documented conditions | Absolute SPL without the recording-chain calibration; durability beyond tested conditions |

No spectrum-derived RPM estimate becomes independent validation of that same
spectrum. A video tachometer can help with approximate state annotation after
frame review and alignment; it is not automatically a sample-accurate channel.
Known stationary neutral operation can support a no-load claim; road throttle
position alone does not supply crankshaft torque. Measurement needs scale with
the claim. Professional SPL calibration is not a prerequisite for every useful
timbre comparison, but it is required for an absolute-level claim.

All existing development/validation/quarantine partitions stay frozen. Catalog
inspection creates no new held-out acoustic dataset. Uncertain candidates are
not fitted, redistributed, added to the listening page or given invented rights.
The current raw-stereo protocol remains the measurement authority; manufacturer
numbers cannot be converted into dBFS gains or used to normalize unrelated clips.

## Correct engine identity before collecting more sound

| Preset and source count | Actual project inspiration | Important exclusions |
| --- | --- | --- |
| I4 —5 | Generic Toyota1AZ-FE/RAV4 NA2L; BRExhaust-inspired replacement route | Turbo Golf,1AZ-FSE direct injection,2AZ-FE and modern M20A RAV4 are distinct. |
| I6 —14 | Generic M54B30/E46 330i NA3L; Supersprint-inspired twin route; research8500rpm | Supplied E46 M3/S54 is related, not this engine. M4 turbo is more distant. Stock M54 data do not validate8500rpm. |
| Cross V8 —11 | Coyote/MustangGT-inspired5L;7500rpm; modified X route with rear silencers deleted | Current route is not the catalog Borla ATAK system. Active/non-active, GT/Dark Horse and S550/S650 remain distinct. |
| Flat V8 —13 | Ferrari458 Italia-inspired4.5L;9000rpm; Tubi-inspired unvalved rear | Speciale/race versions, valved systems, sport catalysts, catalyst bypass and ECU changes remain separate. |
| Boxer —11 | FB20/GP7 XV-inspired NA2L and HKS32018-AF012 rear | FA20/FA24 GR86/BRZ, EJ turbo, hybridGPE and later FB20 revisions are not automatic matches. |

Four further records cover research/recording-library scope. Counts describe
source records, not independent recording sessions or equally qualified evidence.

The I6 distinction corrects earlier conversational references to an
“S54-inspired preset.” [The core/exhaust research](stage-13b-p.md) already
identifies M54; [BMW's archive](https://www.bmwgroup-classic.com/en/models/bmw-classics/product-description-page.ad-867-1.bmw-330i-e46.content.html)
confirms330i/M54. The simulator remains a generic modified inspiration:
its84×90mm geometry, generic cams/compression and8500rpm assumption do not make
it an exact stock M54 replica. No frozen supplied-recording identity is changed.

## Manufacturers and exhaust variants covered

| Family | Inspected manufacturers and useful comparisons |
| --- | --- |
| I4 | Toyota OEM context; BRExhaust/MagnaFlow replacement route; Walker joints; HKS catalog exclusions. Public1AZ manifold-service footage is a layout lead, not a clean sound sample. Exact1AZ-FE aftermarket acoustic coverage remains the weakest. |
| I6 | Supersprint full/header/catalyst, resonated/unresonated and Sport/Race program; Eisenmann rear/center-delete and Sport/Race; UUC twin cat-back/reflection cancellation; Scorpion resonated cat-back versus rear-only; Dinan and Remus axle-back documentation. Owner leads add full systems, headers, underfloor catalysts and middle deletes. |
| Cross V8 | Borla S-Type/ATAK, active/non-active, cat-back/axle-back and X options; Corsa Sport/Xtreme and double-X; MagnaFlow modular xMOD/H/muffler-delete; AWE Touring/Track/SwitchPath/H; American Racing Headers primary-size/upstream modifications. |
| Flat V8 | Tubi unvalved/valved rear; Capristo rear/header/100- and250-cell/catalyst-bypass options; Akrapovič Slip-On/optional links/valves; Kline X/valve/catalyst/material variants; Novitec combined tuning; iPE official Speciale and installer Italia leads; Fabspeed header instructions. |
| Boxer | HKS GP7 rear system and retained upstream sections; Fujitsubo A-S; Ganador rear-only/woolless; Syms rear; owner stock FB20 cabin/tailpipe and HKS installations. Non-hybrid and hybrid records are kept separate. |

The complete58-row catalog records the exact URLs, access, claims, missing data,
variants and media links. Primary sources establish their own configuration
claims, not independent proof of their marketing descriptions or performance.
No rule equates Inconel/titanium, an “F1” label, tip diameter or a no-drone claim
with a quantified sound improvement in our model.

## Stronger quantitative leads, with limits

| Source | Reported observation | Remaining limitation |
| --- | --- | --- |
| [HKS GP7/FB20](https://www.hks-power.co.jp/en/product_db/muffler/db/15602) |32018-AF012; nominal50.8→45×2mm; proximity75dB stock/83dB HKS | Product-specific test RPM, recording-chain metadata and paired raw audio absent. A generic idle footnote is not substituted for the proximity metric. |
| [Fujitsubo GP7/FB20](https://www.fujitsubo.co.jp/products/detail/2486) |350-63093; idle55/59dB,2000rpm65/76dB,4000rpm75/78dB stock/modified | Proximity row77/84dB says4650rpm; remarks say4000rpm. Preserve the conflict; the linked video has no inspected metrology. |
| [Ganador GP7/FB20](https://ganador.co.jp/product/pae-050st/) |PAE-050ST; proximity80/81dB at4000rpm; idle58/64dB; manufacturer states same-condition internal comparisons | Published RPM is useful for the scalar result. No synchronized audio/mic-gain/calibration record was supplied. Different brands' stock baselines cannot be pooled into one target. |
| [Syms GP7/GPE](https://syms.jp/product/rearmuffler-xv/) |GP7 proximity76dB and acceleration70dB; hybrid82/71dB is separate | No per-recording RPM/microphone or signal chain; proximity and acceleration are different metrics. |
| [UUC TSE3](https://www.uucmotorwerks.com/catalog/uuc-twin-silencer-exhaust-tse3brfor-01-05-e46-330--325.htm) |M54-compatible true dual2.25in; manufacturer claims compliance with a95dB stationary-test limit | This is a claimed upper bound, not a measured95dB waveform target. Offered330i driving/neutral-rev downloads are both404. |
| [Corsa GT](https://www.corsaperformance.com/products/xtreme-sport-catback-mustang-gt-coupe-21250-21254) |Manufacturer claims+16dB Xtreme/+8dB Sport versus stock | No stated RPM, weighting or microphone/window in the inspected claim; no universal broadband multiplier follows. |
| [Akrapovič458](https://www.akrapovic.com/de/car/product/86/Ferrari/458-Italia-458-Spider/2013?brandId=20&modelId=70&yearId=264) |Primary approval sheet identifies stock manifolds/cats and replacement silencer topology; primary stock/Slip-On dyno plot spans engine speed | Visually checked documents supply component/dyno evidence. The approval sheet contains no measured sound-level table; neither PDF has synchronized acoustic data. |

[Fujitsubo's dated method explanation](https://www.fujitsubo.co.jp/about-exhaust-systems/regulation/)
describes warm stationary neutral operation, held speed followed by lift, a
microphone0.5m away/45° behind the outlet, A weighting and FAST maximum. This
suggests a useful future lift-off fixture. Its peak statistic is different from
our unweighted average raw power. The2018 explanation is method context, not
current legal advice or proof that another maker's video used those conditions.

Catalog errors to avoid: packaging L/W/H and gross weights are not pipe path
length, free gas volume or acoustic inertia. UUC's95dB limit is not a sample
level. Stock noise differing across manufacturers is not a simulator failure.
Fitment/category fields can conflict: Eisenmann's model field mentions AWD while
the actual center-pipe description excludes it. These remain uncertainties.

## Media and measurement acquisition outcomes

- Official Borla active/non-active stock/S-Type/ATAK, Akrapovič458, iPE Speciale,
  manufacturer-linked Fujitsubo/Kline/Capristo and owner clips are cataloged.
  Indexed titles/descriptions are distinct from actual audiovisual inspection.
  None of this turn's candidate videos is claimed to have been heard, measured
  or visually annotated. The earlier supplied GR86/E46 frame review remains valid.
- UUC's two explicitly offered330i MOV downloads were attempted from their
  source-page URLs; both404. The page remains readable and is snapshotted.
  No empty/error body is admitted as reference media.
- Several primary pages/PDFs are readable through web indexing but direct
  snapshots fail: HKS503, Borla403, Supersprint GB404 and US/WW empty200,
  Tubi404 and some YouTube cache/metadata failures. No login/cookie/access-control
  bypass is attempted, and unavailable media are not counted as acquired.
- The official Akrapovič-linked approval/dyno PDFs were downloaded, fingerprinted
  and rendered using the existing bundled PDF runtime after web screenshots
  failed. Both pages were visually inspected. No graph curve is digitized into
  an invented exact measurement; an approval sheet is not the full test report.
- Research corpora were checked for suitability. The procedural dataset is
  synthetic; IDMT electric motors have different source physics; traffic labels
  lack our engine/state controls. ANIR's calibrated cabin/IR work is a useful
  later listener-method lead, not a matching tailpipe set. A Pole Position458
  file list is a commercial lead, not an acquired licensed dataset.

Public availability and a media link do not establish redistribution/training
rights. No purchase or external contact was made. Source snapshots/documents
stay ignored; only curated metadata/links and notes enter Git.

## Final039 gates and practical remaining request

| Original039 obligation | Assessment after this search |
| --- | --- |
| Source/rights/interval/processing ledger and raw stereo powers | Implemented for the eleven actual sessions/34 excerpts; retained unchanged. Expanded public candidates are clearly separate. |
| Frozen development/validation separation and audition controls | Implemented; parent locks, held-out/quarantine and common/equal-RMS controls preserved. Nine focused behavior tests pass without skips. |
| Broader engine/exhaust configuration coverage | Improved for all five families;58 documented sources/23 exhaust brands and useful reported FB20 data. Coverage is uneven and access limitations remain explicit. |
| Decisive matched recording conditions, especially NA I6/FB20 | Still incomplete: no recovered raw recording set ties the named complete setup, independent RPM/operating condition and microphone geometry/transfer together. |
| Full039 completion | Does not pass yet. Preserve the completed protocol; retain the matched-acquisition gap in039 and hold before041. |

If Bernardo later looks for references, the most useful first additions are:

1. **M54B30/330i:** a named full exhaust configuration, steady-speed/no-load
   rev and loaded pull followed by a long lift/coast, with visible/logged RPM,
   known driving state and documented microphone position/distance.
2. **FB20/GP7:** stock or a named HKS/Fujitsubo/Ganador/Syms rear system with
   upstream hardware identified; exterior and cabin recorded separately, with
   the same state/RPM and microphone notes. Keep hybrid/GR86 sources distinct.

Original audio with fixed gain and disclosed processing would improve dynamic
comparisons. Absolute SPL needs sensitivity/gain/calibrator information; a
useful documented qualitative or relative comparison can have a narrower claim.
The modified8500rpm I6 needs evidence for that build/range; stock M54 clips do
not certify it. No immediate recording request is required by this checkpoint.

## Review, validation and handoff

Fresh checks cover catalog structure/unique IDs/URLs, all five configurations,
the23-brand count, parent catalog hash, reported scalar-versus-audio claim
separation, locked-session/raw/published/engine/Claude preservation, roadmap
IDs/dependencies/links and nine existing reference tests. No core/tool executable
code changed; expensive native/WASM/render suites are not claimed as fresh runs.
The existing RESULTS page and sound remain intact.

Evidence is ignored under `renders/evidence/stage-039-source-search-2026-10-24/`:
20 raw search records, primary page text/HTML snapshots, successful document and
failed media acquisition logs, PDF images/text/hashes, preservation/catalog/docs
checks and test logs. Graphify is queried and AST-refreshed for the new notes.
New audio is zero; new locked audio sessions are zero. Commit/sync the research
and corrected039 status, then hold.041 remains ready by038/040 dependencies and
unstarted; further development awaits Bernardo's instruction.
