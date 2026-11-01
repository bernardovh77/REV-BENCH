# Bounded completed-stage review repairs

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md)

Allocated after 111 to preserve the canonical IDs. Execution priority is
112 → 113 → hold; 037 remains the next planned feature stage. These repair
the two new C07/C08 reproductions, not the six broader retained risk groups.

<a id="stage-112"></a>
## Stage 112 — Recoverable checkpoint section publication

**Dependencies:** [033](02-existing-sound.md#stage-033), [036](03-contracts-and-sources.md#stage-036)

**Deliverable:** Repair C08 at the existing section-publisher/composer boundary
using complete immutable fragments and recoverable manifest/page installation.
Keep the single portable result, accepted examples and references.

**Required exit evidence:** Interrupted fragment/page/manifest writes and failed
installs preserve a playable, rebuildable checkpoint; retry works without losing
other sections. Full results rebuild and offline playback preserve all 48 clips.

**Stop/reject rule:** No success claim from page survival alone; verify registered
fragments and subsequent composition. Do not claim multi-file crash atomicity
or concurrent-writer support from ordinary exception recovery.

**Execution record:** [Stage 112](../stage-112.md).

<a id="stage-113"></a>
## Stage 113 — Acoustic derived-state and numerical failure guards

**Dependencies:** [032](02-existing-sound.md#stage-032), [034](02-existing-sound.md#stage-034), [036](03-contracts-and-sources.md#stage-036)

**Deliverable:** Repair C07 at B0 prepare/process/observer boundaries. Reject
unrepresentable derived coefficients and report failed numerical books/storage;
define reset/retry behavior without changing the qualified linear equations.

**Required exit evidence:** Reproductions fail safely, ordinary cases retain
their numerical results, invalid-argument retry still works, numerical failures
require reset, and regression/sanitizer/deliberate-fault checks pass. Record cost.

**Stop/reject rule:** Finite guards are not physical amplitude qualification,
stability proof, engine coupling or production promotion. No silent protective
intervention may be reported as an ordinary successful acoustic step.

**Execution record:** [Stage 113](../stage-113.md).

<a id="stage-115"></a>
## Stage 115 — Valve jet noise and the cycles' variation

**Dependencies:** [040](03-contracts-and-sources.md#stage-040), [041](03-contracts-and-sources.md#stage-041), [042](03-contracts-and-sources.md#stage-042), [113](#stage-113)

**Deliverable:** Fix the audit's D02–D05 in the production noise layer:
- the gap's Mach number and speed from the pressures either side;
- valve noise through X and Y junctions to every tailpipe beyond them;
- full path lengths;
- the catalyst's share only with a catalyst.

Recalibrate what the corrected law moves. Bring the cycles' variation back to the
real engines' 1–3 % COV of IMEP wide open with a causal delay, and restore the
test's floor.

**Required exit evidence:**
- an independent law check across choking;
- power-conserving, in-phase routing;
- per-preset COV;
- before and after measurements against the real recordings;
- planted bugs caught;
- native, sanitizer, WASM and web suites green;
- A/B renders for listening.

**Stop/reject rule:** Change no gas dynamics, preset or exhaust. Don't move a
test's limit unless the corrected physics explains it, and say so where it moves.

**Execution record:** [Stage 115](../stage-115.md).

<a id="stage-116"></a>
## Stage 116 — Tests that can fail again

**Dependencies:** [115](#stage-115)

**Deliverable:** Tighten the tests whose limits had been loosened until they
couldn't catch what they were written for:
- the junction's energy books;
- the wild cam's burn length;
- the coast's fuel cut;
- the rev limiter's top;
- two checks in the radiation test that couldn't fail.

Fix `test_render_results`, which fails under GNU make 4.3.

**Required exit evidence:** a planted bug for each tightened test that the
old limit passed and the new one catches; native and Python suites green.

**Stop/reject rule:** No `core/` change. Set a new limit from the measured
behaviour and the design's intent, not from what happens to pass.

**Execution record:** [Stage 116](../stage-116.md).
