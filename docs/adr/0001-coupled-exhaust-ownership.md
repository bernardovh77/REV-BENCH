---
status: accepted
---

# Shared exhaust geometry with bidirectional physical loading

Bernardo accepted this **prototype scope**, not a production architecture,
on 2026-10-23: retain the physical engine as the authority for combustion and gas
exchange, and evaluate a downstream acoustic network derived from the same exhaust
description, returning its load through one conservative interface while
retaining the downstream mean gas inventories. This experiment tests whether
replacing uniform fine gas transport is worthwhile despite coupling cost and validity limits;
a one-way timbre filter or a full learned waveform would not preserve that
physical ownership automatically.

The static graph is promising, but the tested conservative control is too
dissipative and neither candidate validates engine integration. This decision
does not select a production replacement: acceptance depends on same-tick
feedback, conservation, mean-flow/thermal/reversal tests, real-engine listening
and measured deployed cost. Scope and promotion conditions are in the
[architecture review](../review-acoustic-architecture-2026-10-23.md).
