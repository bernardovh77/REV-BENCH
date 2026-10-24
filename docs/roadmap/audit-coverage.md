# Audit finding ownership and closure evidence

[Canonical tracker](../../PLAN.md) · [Original audit](../sound-physics-audit-2026-10-22.md) · [Stage protocol](README.md)

This assigns all **D01–D09 / R01–R18** findings to explicit stages. It does not close any finding or erase the original audit's superseded-configuration caveats. Completion requires a linked execution record and affected integration/acceptance evidence. Scoped limitations and deferred mechanisms remain labeled, not reported as implemented fixes.

The later [completed-stage review](../review-completed-stages-2026-10-23.md)
also records C07 (prototype derived-state/energy guards; owners 037/038/045)
and C08 (interrupted results-fragment write; bounded publication follow-up
needed before the next affected checkpoint). Those are additional findings,
not replacements for D01–D09/R01–R18. The bounded repairs are recorded in
[112](../stage-112.md)/[113](../stage-113.md). [040](../stage-040.md) supplies the
native spark/kernel/heat correction and independent D01 controls;057/059 still
own affected integration and operating-envelope cross-checks. R03's uncalibrated
kernel/extinction and late-chemistry limits remain explicit and open through041.

| Finding | Issue | Primary owner | Required cross-checks | Evidence needed for closure |
| --- | --- | --- | --- | --- |
| D01 | Pre-spark heat release | [040](03-contracts-and-sources.md#stage-040) | [057](05-engine-qualification.md#stage-057), [059](05-engine-qualification.md#stage-059) | Zero pre-actual-ignition heat, event/fuel/work books and an early-heat fault. |
| D02 | Incorrect choked source state | [053](05-engine-qualification.md#stage-053) | [054](05-engine-qualification.md#stage-054), [059](05-engine-qualification.md#stage-059) | Attainable nozzle-to-throat-to-noise controls across choking and reverse flow. |
| D03 | Missing X/Y source scattering | [054](05-engine-qualification.md#stage-054) | [049](04-propagation.md#stage-049), [057](05-engine-qualification.md#stage-057) | All-source/all-outlet complex transfer and upstream loading through actual geometry. |
| D04 | Incomplete/fixed source path delays | [044](04-propagation.md#stage-044), [054](05-engine-qualification.md#stage-054) | [048](04-propagation.md#stage-048), [057](05-engine-qualification.md#stage-057) | Complete remaining lengths and temperature/flow-dependent propagation, with path-identity tests. |
| D05 | Hidden/obsolete component loss | [050](04-propagation.md#stage-050), [054](05-engine-qualification.md#stage-054) | [057](05-engine-qualification.md#stage-057), [059](05-engine-qualification.md#stage-059) | Explicit component bypass/loss controls and measured or bounded magnitude/phase. |
| D06 | Variation switches deterministic model | [041](03-contracts-and-sources.md#stage-041) | [059](05-engine-qualification.md#stage-059), [063](05-engine-qualification.md#stage-063) | Continuity to zero variation at fixed mean model, multi-seed energy/COV controls. |
| D07 | Injector sound detached from fuel | [042](03-contracts-and-sources.md#stage-042) | [055](05-engine-qualification.md#stage-055), [057](05-engine-qualification.md#stage-057) | Actual injection drives excitation; disabled-injection and retained-fuel cases. |
| D08 | Incomplete guard/junction coverage | [038](03-contracts-and-sources.md#stage-038) | [057](05-engine-qualification.md#stage-057), [069](07-browser.md#stage-069), [099](12-release.md#stage-099) | All relevant requested/accepted transfers and interventions traced, adverse/integration faults caught. |
| D09 | Fractional-delay coloration | [056](05-engine-qualification.md#stage-056) | [069](07-browser.md#stage-069), [073](08-3d.md#stage-073), [099](12-release.md#stage-099) | Independent digital delay magnitude/phase and real moving-history tests. |
| R01 | Robotic coast / lift boom attribution | [058](05-engine-qualification.md#stage-058) | [040](03-contracts-and-sources.md#stage-040), [041](03-contracts-and-sources.md#stage-041), [048](04-propagation.md#stage-048), [053](05-engine-qualification.md#stage-053), [054](05-engine-qualification.md#stage-054), [059](05-engine-qualification.md#stage-059), [063](05-engine-qualification.md#stage-063) | Matched real phases, controlled same-trajectory layers and competing thermal/flow/transfer interventions. |
| R02 | V8 idle residual/COV and fit limits | [041](03-contracts-and-sources.md#stage-041) | [059](05-engine-qualification.md#stage-059), [063](05-engine-qualification.md#stage-063), [094](11-engine-catalog.md#stage-094) | Valid dilution/residual range, multi-seed per-engine COV and stable physical maps. |
| R03 | Kernel failure / late chemistry gaps | [040](03-contracts-and-sources.md#stage-040), [041](03-contracts-and-sources.md#stage-041) | [058](05-engine-qualification.md#stage-058), [059](05-engine-qualification.md#stage-059) | Research significance and actual-event limits; implement justified missing mechanisms or explicitly retain unsupported behavior. |
| R04 | Low-band repetition acceptance | [041](03-contracts-and-sources.md#stage-041), [059](05-engine-qualification.md#stage-059) | [063](05-engine-qualification.md#stage-063), [098](12-release.md#stage-098) | Reference-valid per-engine/multiple-seed repetition and Bernardo listening; no universal score substitution. |
| R05 | Acoustic convergence | [043](04-propagation.md#stage-043), [045](04-propagation.md#stage-045), [046](04-propagation.md#stage-046), [048](04-propagation.md#stage-048), [052](04-propagation.md#stage-052) | [057](05-engine-qualification.md#stage-057), [059](05-engine-qualification.md#stage-059), [060](05-engine-qualification.md#stage-060) | Independent spatial/pipe/engine-time controls, phase/magnitude/nonlinear/partition validity. |
| R06 | Unobserved protective interventions | [038](03-contracts-and-sources.md#stage-038) | [045](04-propagation.md#stage-045), [048](04-propagation.md#stage-048), [057](05-engine-qualification.md#stage-057), [099](12-release.md#stage-099) | Complete cap/repair/fallback observability and integrated physical consequences. |
| R07 | Estimated real exhaust geometry | [044](04-propagation.md#stage-044), [049](04-propagation.md#stage-049), [050](04-propagation.md#stage-050) | [075](08-3d.md#stage-075), [094](11-engine-catalog.md#stage-094) | Published versus estimated data and full physical component calibration; no exact-hardware claim from a name. |
| R08 | Muffler topology/impedance limits | [050](04-propagation.md#stage-050) | [054](05-engine-qualification.md#stage-054), [059](05-engine-qualification.md#stage-059), [094](11-engine-catalog.md#stage-094) | Causal impedance/storage/backpressure, magnitude and phase over a supported range. |
| R09 | Thermal/property assumptions | [047](04-propagation.md#stage-047), [048](04-propagation.md#stage-048) | [058](05-engine-qualification.md#stage-058), [059](05-engine-qualification.md#stage-059), [076](08-3d.md#stage-076) | Gas/property/wall sensitivity, evolved species/thermal fronts, reversal and coefficient-work books. |
| R10 | Radiation load / audible field consistency | [051](04-propagation.md#stage-051) | [056](05-engine-qualification.md#stage-056), [057](05-engine-qualification.md#stage-057) | Joint impedance/reactive/radiated-power/phase controls and single load ownership. |
| R11 | Generic structure/mechanical source | [055](05-engine-qualification.md#stage-055) | [059](05-engine-qualification.md#stage-059), [063](05-engine-qualification.md#stage-063), [098](12-release.md#stage-098) | Engine-specific event/force/modal/mount/accessory responses and reference uncertainty. |
| R12 | Cabin/body/source placement | [056](05-engine-qualification.md#stage-056) | [037](03-contracts-and-sources.md#stage-037), [075](08-3d.md#stage-075), [076](08-3d.md#stage-076) | Shared spatial frames and known-position/body/cabin transfer tests. |
| R13 | Pass-by / moving history limits | [056](05-engine-qualification.md#stage-056) | [069](07-browser.md#stage-069), [077](08-3d.md#stage-077), [099](12-release.md#stage-099) | Analytic Doppler/distance and bounded full trajectories, lifecycle and timing. |
| R14 | High-rev physical assumptions | [059](05-engine-qualification.md#stage-059), [087](10-limits.md#stage-087) | [086](10-limits.md#stage-086), [094](11-engine-catalog.md#stage-094), [098](12-release.md#stage-098) | Modified-engine maps and researched mechanical/valvetrain/knock limits; no inferred durability. |
| R15 | Unmatched reference conditions | [039](03-contracts-and-sources.md#stage-039) | [058](05-engine-qualification.md#stage-058), [059](05-engine-qualification.md#stage-059), [064](06-learned-research.md#stage-064), [094](11-engine-catalog.md#stage-094), [098](12-release.md#stage-098) | Hardware/state/viewpoint/session ledger, held-out data and explicit limits on identifiability. |
| R16 | Measurement and preview confounds | [036](03-contracts-and-sources.md#stage-036), [039](03-contracts-and-sources.md#stage-039) | [059](05-engine-qualification.md#stage-059), [063](05-engine-qualification.md#stage-063), [098](12-release.md#stage-098) | Unfiltered stereo power, fixed causal gain, separate level-matched timbre auditions and source/processing hashes. |
| R17 | Real-time delivery unresolved | [062](05-engine-qualification.md#stage-062), [069](07-browser.md#stage-069) | [077](08-3d.md#stage-077), [084](09-turbo.md#stage-084), [099](12-release.md#stage-099), [102](12-release.md#stage-102) | Actual full-engine callbacks, tails/max/observed misses/memory on named hardware and deployed product. |
| R18 | Isolated tests / ambiguous completion | [036](03-contracts-and-sources.md#stage-036) | [060](05-engine-qualification.md#stage-060), [061](05-engine-qualification.md#stage-061), [063](05-engine-qualification.md#stage-063), [069](07-browser.md#stage-069), [099](12-release.md#stage-099), [102](12-release.md#stage-102) | Independent fixtures, failure controls, production path and separate numerical/realism/runtime/user acceptance. |

The later [completed-stage review](../review-completed-stages-2026-10-23.md)
also reproduced C07/C08. Their bounded repairs are [113](../stage-113.md)
(derived-state/numerical guards) and [112](../stage-112.md) (section publication).
These do not close any broader D/R physics, realism or runtime obligation.

Every realism-changing stage also checks whether it reopens a previously accepted finding. Turbo, knock/damage, new layouts, learned integration and final tuning repeat affected physics, listening and runtime checks; base-engine acceptance is not inherited outside its envelope.

Stage038 supplies the [bounded native D08/R06 observation evidence](../stage-038.md):
junction inspection/repair counts, requested/accepted transfers and staggered
impulses, component geometry/CFL and explicitly costly optional monitoring.
Later integrated/browser/realism cross-checks in the table remain required;
observing an equalisation cap or property extrapolation does not remove its
physical limitation. C07/C08 bounded repairs remain112/113.
