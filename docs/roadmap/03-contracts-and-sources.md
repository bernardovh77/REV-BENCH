# Roadmap, shared contracts and trustworthy excitation

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

<a id="stage-036"></a>
## Stage 036 — Unified roadmap and evidence protocol

**Dependencies:** [035](../../docs/roadmap/02-existing-sound.md#stage-035)

**Deliverable:** Replace overlapping stage numbers with one canonical sequence, lossless legacy crosswalk, audit ownership, dependency graph and reusable evidence protocol. Preserve historical notes and Claude setup.

**Required exit evidence:** All 62 legacy rows and 27 audit findings have explicit owners; IDs/dependencies/links are consistent and acyclic; full scope and sound-before-browser order confirmed by Bernardo on 2026-10-23.

**Stop/reject rule:** No renumbering may close unresolved physics, listening or throughput requirements. This stage changes documentation only.

**Legacy coverage:** Roadmap reorganization.

**Starting evidence/specification:** [docs/roadmap/README.md](../../docs/roadmap/README.md), [docs/roadmap/legacy-map.md](../../docs/roadmap/legacy-map.md), [docs/roadmap/audit-coverage.md](../../docs/roadmap/audit-coverage.md).

**Execution record:** [docs/stage-036.md](../stage-036.md), with the scope answers, review findings and documentation/preservation checks.

<a id="stage-037"></a>
## Stage 037 — Shared build, geometry, event and snapshot contracts

**Dependencies:** [036](../../docs/roadmap/03-contracts-and-sources.md#stage-036)

**Deliverable:** Specify one versioned engine/build description: SI units, semantic component/port/source IDs, topology and spatial frames/centerlines. Define actual event times, unwrapped crank cycles, audio/display clocks, coherent snapshots, transactional rebuild/reset and serialization extension points for turbo/damage/layouts. Separate physical gas/listener flight time from DSP/FIR, buffering and device latency; specify the source event and expected audible arrival used for synchronization.

**Required exit evidence:** Round-trip representative five-preset descriptions; intake-runner removal preserves identities; incompatible generations/unsupported topology reject explicitly. Review schemas with physics, browser and 3D consumers before their implementation; preserve existing TS/core bindings rather than duplicate them.

**Stop/reject rule:** Do not choose the production renderer or promise a full geometry compiler here. Unsupported data/operating ranges and snapshot costs must be explicit.

**Legacy coverage:** S2d-0; 14-19; 24.

**Starting evidence/specification:** [docs/stage-S2d-design.md](../../docs/stage-S2d-design.md), [GLOSSARY.md](../../GLOSSARY.md).

**Execution record:** [Stage 037](../stage-037.md), with the [versioned contract](../../contracts/README.md), complete native round trips and bounded protocol/consumer review evidence.

<a id="stage-038"></a>
## Stage 038 — Complete numerical guard and flux observability

**Dependencies:** [037](../../docs/roadmap/03-contracts-and-sources.md#stage-037)

**Deliverable:** Instrument requested/accepted mass, species, momentum and total-energy transfer, junction inventories, clamps, sonic/cap/fallback reasons, effective geometry and CFL. Record physical effects per semantic component without callback file I/O.

**Required exit evidence:** Adverse vacuum/reversal/junction fixtures and deliberately hidden repairs must fail the checks. Reconcile whole-engine books and confirm diagnostic taps do not change the trajectory. Establish bounded trace and no-trace cost.

**Stop/reject rule:** Finite output or zero chamber repairs alone is insufficient; uninstrumented junction/pipe repairs keep this stage open.

**Legacy coverage:** S1f; S2d-0; D08; R06.

**Starting evidence/specification:** [docs/sound-physics-audit-2026-10-22.md](../../docs/sound-physics-audit-2026-10-22.md).

**Execution record:** [Stage038](../stage-038.md), with the predecessor review, native/WASM guard/flux proof, cost/trace limits and complete float checkpoint comparison.

<a id="stage-039"></a>
## Stage 039 — Matched references and calibration protocol

**Dependencies:** [036](../../docs/roadmap/03-contracts-and-sources.md#stage-036)

**Deliverable:** Acquire/annotate known engine/exhaust, RPM, load, microphone distance/perspective and recording sessions, prioritizing NA I6 and boxer gaps. Preserve raw stereo float and a locked validation split by session; ask Bernardo for targeted missing examples.

**Required exit evidence:** Publish coverage/uncertainty and processing ledger, sample intervals, rights/provenance and per-channel power measures. Demonstrate held-out/development separation and common-gain versus level-matched auditions. Set which claims each recording can support.

**Stop/reject rule:** Unknown MP3 viewpoint/RPM cannot become a calibrated SPL or waveform target. Missing decisive data blocks that calibration claim, while independent numerical fixtures may continue.

**Legacy coverage:** R2; R15; R16.

**Starting evidence/specification:** [docs/user-sound-references.md](../../docs/user-sound-references.md), [docs/sound-robustness-plan.md](../../docs/sound-robustness-plan.md).

**Execution record:** [Stage039](../stage-039.md), the [protocol review](../stage-039-review.md) and the superseding [public-source search](../stage-039-reference-search.md). The protocol is implemented; full039 still includes acquiring claim-appropriate matched recordings. The earlier bounded closure did not complete that acquisition and is superseded. Manufacturer scalar measurements and catalog/video leads do not certify missing recording metadata. Later056/059/063/064/094/098 gates remain separate. Status lives in PLAN.

<a id="stage-040"></a>
## Stage 040 — Spark, ignition and flame-development causality

**Dependencies:** [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038)

**Deliverable:** Separate commanded spark, actual spark discharge, subsequent kernel development and flame heat release; keep the kernel delay nonnegative relative to the actual discharge. Prevent pre-spark heat in the normal spark-ignited model and define misfire/cut/resume events using researched limits. Any later explicitly modeled autoignition has its own causal event.

**Required exit evidence:** Zero heat before the actual spark discharge within a predeclared numerical tolerance in the normal spark-ignited/autoignition-disabled control; closed fuel/energy/work books and deliberately early-heat fault caught. Test startup, idle, cut, overrun and retained fuel across the five presets.

**Stop/reject rule:** Do not add random pops or retime pulses merely to resemble a clip. Unsupported late-combustion/afterfire physics stays labeled.

**Legacy coverage:** S1f; D01; R03.

**Starting evidence/specification:** [docs/audio-robustness-research.md](../../docs/audio-robustness-research.md).

**Execution record:** [Stage040](../stage-040.md), with the predecessor review, baseline, frozen causal/energy gates and execution evidence.

<a id="stage-041"></a>
## Stage 041 — Continuous combustion variation and residual validity

**Dependencies:** [040](../../docs/roadmap/03-contracts-and-sources.md#stage-040), [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038)

**Deliverable:** Separate deterministic combustion-model selection from variation amplitude; characterize dilution/flame-fit validity, residual exchange, finite burn/kernel failure and gross/net IMEP statistics. Research whether missing late chemistry is consequential before adding it.

**Required exit evidence:** Variation approaching zero preserves the same mean model continuously; event/energy consistency, multiple-seed COV and repetition, idle stability and five-preset maps pass. Preserve evidence for V8 idle residual/COV and unmodeled chemistry.

**Stop/reject rule:** No arbitrary randomness or universal repetition score can substitute for engine-specific evidence. Out-of-fit states must be explicit.

**Legacy coverage:** S1; S1f; D06; R02-R04.

**Starting evidence/specification:** [docs/stage-S1c.md](../../docs/stage-S1c.md), [docs/audio-robustness-research.md](../../docs/audio-robustness-research.md).

**Execution record:** [Stage 041](../stage-041.md): continuous charge physics, explicit fit validity, conditional-mean phasing and aligned gross/net work; 120 seeded maps, 15 stereo repetition runs, independent/fault checks, native/WASM and full sound checkpoint. Real-engine COV/repetition/kernel/chemistry calibration and listening acceptance remain open, with high V8 idle dilution/COV and I4/boxer repetition retained as findings.

<a id="stage-042"></a>
## Stage 042 — Actual fuel-injection events and acoustic triggers

**Dependencies:** [040](../../docs/roadmap/03-contracts-and-sources.md#stage-040), [038](../../docs/roadmap/03-contracts-and-sources.md#stage-038)

**Deliverable:** Make the delivered-fuel event the authority for injector excitation; distinguish retained fuel, actual injection, disabled injection and cut/resume timing.

**Required exit evidence:** Zero injection clicks when no injection occurs; one-to-one event/fuel bookkeeping under startup, DFCO, restart and fuel retained in a cylinder. Deliberate detached-event fault is detected.

**Stop/reject rule:** Moving the click to a convenient crank angle without a matching fuel event is not a fix.

**Legacy coverage:** S1f; D07.

**Starting evidence/specification:** [docs/sound-physics-audit-2026-10-22.md](../../docs/sound-physics-audit-2026-10-22.md).

**Execution record:** [Stage 042](../stage-042.md): actual delivered-dose authority, explicit skip reasons, bounded source/trace separation and once-per-step mechanical consumption. Independent clocks/books, 105 seeded scenarios, retained/disabled/replay controls, eight compiled faults, native/WASM and full sound/page checks pass. Instantaneous IVC fueling and generic needle/modal amplitude remain uncalibrated; this is not a resolved real PFI/GDI schedule.
