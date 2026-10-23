# Stage 036 — complete project roadmap and evidence protocol

2026-10-23. **Documentation checkpoint.** Baseline commit
`99ba54e4f5e210b3f72ea7be3b2ba3d0afc26529`. Bernardo asks for a definitive,
structured, renumbered plan covering prototypes, tests, product features and
the complete 3D experience. This work changes planning/docs only, not sound,
physical algorithms, browser behavior or deployment.

## Scope decisions

The invoked `grill-with-docs` wrapper at the user-supplied local
`.agents/skills/grill-with-docs/SKILL.md` requires grilling and domain-modeling. Those companion skills are absent
locally; their upstream primary instructions were read at
[grilling](https://github.com/mattpocock/skills/blob/main/skills/productivity/grilling/SKILL.md)
and [domain-modeling](https://github.com/mattpocock/skills/blob/main/skills/engineering/domain-modeling/SKILL.md),
with their document formats. Their fact-finding rule authorizes the read-only
parallel roadmap, physics/evidence and runtime/interface reviews used here.
No reviewer changed files. The session's scope/order frontier was presented
together and Bernardo explicitly answered **yes** to both:

1. First full release retains the original bench/editor, gauges/dyno, complete
   3D cutaway/gas/motion, turbo, knock/limits/damage, expanded layouts,
   save/share and public launch. Earlier playable checkpoints remain useful.
   Gearbox/two-stroke/export/fuels/supercharger/ITBs/anti-lag/garage/video are
   separately optional expansions.
2. Define shared contracts, finish necessary sound-physics/runtime gates, then
   implement browser and 3D. Learned synthesis has its own conditional research
   and integration gates; it is not automatically required before those features.

This settles the planning-level frontier. Future evidence-dependent decisions
are assigned to explicit stages rather than silently chosen now. The task
does not authorize executing those future feature stages or deploying now.

## Deliverables

- [PLAN.md](../PLAN.md): one current status/scheduling authority, **001–111**,
  grouped in thirteen readable phases. **037** is the next default stage;
  independent matched-reference work is **039**.
- [Thirteen phase catalogs](roadmap/README.md): every stage has a deliverable,
  dependency list, independent exit evidence, rejection rule and execution
  record convention. The protocol adds preflight research, frozen thresholds,
  ownership, failure detection, affected integration, costs and user checkpoints.
- [All 62 legacy rows mapped](roadmap/legacy-map.md), with original implementation
  evidence, partial parents, S3/12b alias and A1-audit/prototype-arm ambiguity
  retained. The old plan is [archived](roadmap/legacy-plan.md); only Markdown
  link locations were adjusted. Git retains the unchanged original.
- [All 27 D/R findings owned](roadmap/audit-coverage.md), with primary and
  integration/acceptance checks. No finding is closed by renumbering.
- [ADR 0002](adr/0002-canonical-roadmap-and-evidence-gates.md), updated glossary,
  public README and current CODEX handoff. Older technical sound plans point
  to the new scheduling authority while preserving their research/specifications.

Historical 001–035 are bounded evidence, not new acceptance of unresolved
sound or performance requirements. Candidate rejection can be a completed
experiment without yielding a production artifact. Legacy S1/S1e/S2/S4/full13b
remain open under their explicit new owners. Old 4x-native/2x-WASM goals,
rejected implicit-valve convergence and sound targets were not silently retired.

## Review findings incorporated

- Reuse existing core/TS engine, pipe and stereo bindings. Current product
  worklet/application still plays tone; do not report it as a browser engine.
- Define schema/semantic/spatial identity and actual event/snapshot clocks
  before graphics, controls and persistence. Avoid numeric pipe IDs as identity.
- Distinguish actual spark discharge, later kernel development and heat;
  do not define “ignition” as heat onset and pass causality by construction.
- Give stochastic sources one explicit energy/work owner before they feed a
  reflected-load graph. Common source/component/thermal gates survive candidate
  rejection; a failed hybrid need not be made to pass to qualify a conservative arm.
- Preserve physical gas/listener flight time while correcting DSP, buffering
  and device/display delay. Require unwrapped cycles/time at every RPM.
- A 0D/1D state does not provide spatial CFD flame fronts/pathlines or metal
  temperature. Diagnostic slow motion has an explicit engine/audio time policy.
- Include listener/scene/layer audition UI; front/side/cutaway/follow-exhaust
  cameras; at least two generically named inspired-by catalog builds.
- Preserve real Chrome/Firefox delivery, genuine stereo, actual callback tails,
  combined 3D/audio budget, lifecycle, supported hardware and gauges-only fallback.
  Reuse existing publication authorization; ask only if absent at execution.

## Validation and limits

The documentation checker verifies the **111 unique consecutive IDs**, all
card fields and matching dependency lists, no dependency cycle, unique target
anchors, local links/fences, **62/62** legacy mappings and **27/27** audit owners.
It also checks preserved measurement/reference/current-audio/core/WASM/Claude
hashes against the preceding checkpoint: **158 files and 48 embedded audio
payloads remain identical**. Detailed local validation and document hashes
are under ignored `renders/evidence/roadmap-2026-10-23/`.

No native/Python/web physics or playback suite is rerun for these documentation
changes. The previous checkpoint's test counts remain historical, not fresh
proof attributed to this plan. Future-stage tests are specifications; unbuilt
software cannot be honestly certified now, nor can zero rework be guaranteed.
The plan reduces that risk through explicit ownership, independent tests,
scope limits, rejection and user acceptance before promotion.

Graphify was queried before code inspection and AST-refreshed to **2478 nodes /
6655 edges / 140 communities**, with the known `core/include/rb.h:415`
partial-extraction warning retained. Its log is kept with the local evidence;
no semantic-document extraction is claimed. Claude's
active setup, accepted sound selection and local recordings are preserved.
User-added `.agents/` and `skills-lock.json` stay untouched and unstaged.
