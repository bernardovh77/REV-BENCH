---
status: proposed
---

# No replacement for today's pipes yet; a bounded experiment instead

**Proposed by Claude, 2026-11-23 (stage 060), for Bernardo's acceptance.**

**Decision.** Keep A0, today's finite-volume pipes, as the production renderer. Reject every
current replacement as a whole renderer. Open one bounded experiment, stage 136: the coupled
arm as a component, in the cans and the band above about 1.5 kHz only. Nothing is promoted, so
061's promotion stays blocked until an arm passes 136.

## The arms, at the evidence they have

| Arm | Accuracy | Full engine | Cost | Verdict |
| --- | --- | --- | --- | --- |
| **A0**, finite volumes on 7.5 cm cells | the plain pipes lose under 0.3 dB to 1.5 kHz along every path, then 1.4–4.3 dB at 2 kHz and 5–11 at 2.4 (057); the cans hold plane-wave theory only to 450–750 Hz (050) | the five presets' whole drive with books to 1e-14 (057); dyno within 5–11 % of the inspirations' ratings for three presets (059) | 0.7–2.6× real time, whole engine, per preset and tier (118, 135) | **kept** |
| **A1**, the uncoupled implicit orifice | (043's record) | none | | rejected (043) |
| **A2**, the improved conservative solver with the production gas | at 1 m, magnitude and phase pass at 200 Hz in 72 of 72 rows; at 2 kHz in 12 of 64; at 4 kHz in 0 of 40, up to 98 dB of loss; false pressure excursions of 117–155 Pa at moving contacts (043) | none | | rejected (043; 044 and 045 kept it rejected) |
| **B**, the coupled duct | correct for its bounded proof: one conserved state per cell, characteristic streaming, the books closed (045, 046, 048) | **none:** only a uniform, constant-γ, sample-aligned duct from a still background; no adapter for the production gas, area changes, chambers, a moving background or reversal (057) | **0.002–0.015× real time for a single duct** (046, 048: a correctness reference, "far below real time"); whole-engine cost unknown | **not promoted;** the only viable research direction, as a component (136) |

## Why no comparison at equal fidelity was run

The card asks for one, and none was possible:
- **Only A0 runs the engine.** A comparison needs the arms in the five-preset engine, and only
  A0 can enter it.
- **The card's rule:** "If only one survives earlier gates, document the rejected arm rather than
  fabricate an equal comparison" (057), and a no-winner outcome "blocks production promotion and
  creates a new bounded experiment stage" (060).
- **No paper score:** nothing here is promoted on static speed, brightness or a score.

## What A0 supports (stated)

- **The engines:** the five presets, idle to their rated power speeds.
  - **Within 12 % of their ratings:** the four, the six and the boxer.
  - **Short:** the V8s at high rpm (059).
- **The plain pipes:** to about 1.5 kHz.
- **The cans:** to 450–750 Hz.
- **The upper band** is carried by the noise layer's calibrated passes (054).
- **Not supported:** the overrun's aperiodic content (058), sound against matched recordings
  (039).

## Why 136 is the right experiment

- **Where it aims:** the weak band (above 1.5 kHz in the pipes, above about 500 Hz in the cans)
  is exactly where B's characteristic streaming carries waves without numerical loss.
- **What stays:** the low band, where A0 is already within 0.3 dB, needs nothing new.
- **Why only components:** a component-sized B (a can, a tailpipe) avoids the whole-network
  adapter.
- **What it must still do:**
  - meet the engine's gas, mean flow and reversal at its two ports;
  - close the books;
  - cost what the tier allows (062's budget).

  That is 136's gate.

## Consequences

- **061 is blocked:** there is nothing to promote.
- **062** still works on A0 (stage 118's leads), not on a promoted renderer.
- **063,** Bernardo's acceptance, listens to A0 plus whichever A/B candidates win 128.
- **If 136 fails:** A0 stays and the cans' upper band stays the noise layer's, as today.
