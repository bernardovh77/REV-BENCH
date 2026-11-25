# Phase B3 — a sound identical to a real car (stages 145–152)

Opened 2026-11-25 at Bernardo's request: "The objective in the end would be to feel like I'm
listening to a car in real life, for the sound to be identical. I want the app to take all the
necessary measures to make that possible", and: "consider all present variables included on the
sound when it reaches the ear", "the variables considered and changeable as well". Processing
power is secondary to quality (2026-11-24).

What reaches the ear is a chain: the burn, the valves' blowdown, the pipes, the intake, the
mechanical sources, the turbulent noise; how the car radiates and what its body does; distance,
the ground's reflection, the air's absorption, the surroundings and their echoes, wind and
background; two ears; the microphone when comparing with a recording; the headphones or
speakers and the level. Stage 144 measured the pipes' part; these stages take the rest, each
link checked, measured and made a control where it is a condition of the listening rather than a
fact of the car, with a physically right default.

**The order** (147 deferred to the PC on 2026-11-25: no speed-up on the laptop, docs/stage-147.md):
145 (the Polo 1.0 TSI
with its turbine) → 146 (Bernardo's recording of it, when it comes) → 148 (finer cells, the noise
layer rebalanced) → 152 (the losses above the plane wave, opened by 148's finding) → 149 (the
source-to-ear chain and its controls) → 150 (two ears and the level) → 151 (junctions with their
size and angles). 146 waits on the recording, and 152 is judged against it; the others don't.

<a id="stage-145"></a>
## Stage 145 — The Polo 1.0 TSI as a preset, with its turbine

**Dependencies:** [138](15-exhaust.md#stage-138), [141](15-exhaust.md#stage-141); brings forward
the part of phase D's turbocharger ([078](09-turbo.md#stage-078)) that shapes the sound.

**Deliverable:** Bernardo's own car, a 2023 Volkswagen Polo 1.0 TSI, 110 PS (81 kW), stock
exhaust: the EA211 evo three-cylinder (999 cc; its code, bore, stroke, compression, cams and
turbocharger from VW's published data and public sources), the exhaust manifold in the head,
the turbine, the catalyst(s) and the Polo's silencers as a network, every dimension marked
published or estimated. The turbine as a part of the network: its flow by a map of its
published or estimated size, its housing's volume, and what it does to the pulses passing
through it (the reason these engines are quiet). Boost and the wastegate as far as idle, the
holds and light load need; full boost with phase D.

**Required exit evidence:** the preset's idle held for 30 s; its torque against VW's published
curve (200 N m at 2000–3000 rpm), stating the shortfall's cause if any; the turbine's books and
its four-pole check on the bench where it applies; planted bugs; speed.

**Execution record:** `docs/stage-145.md`.

<a id="stage-146"></a>
## Stage 146 — Bernardo's recording as ground truth

**Dependencies:** [145](#stage-145), [039](03-contracts-and-sources.md#stage-039). **Waits on the
recording.**

**Deliverable:** the recording protocol (outside, the phone 1 m from the tailpipe at its
height, about 45° off its axis; raw/unprocessed WAV; 10 s of background first; idle 30 s, holds
of about 10 s at stated rpm, revs, lift-offs; warm or cold stated; optionally the same 1 m to
the side), written in docs/user-sound-references.md. The analysis: each hold's rpm checked from
the firing frequency (rpm / 40 for a three-cylinder), the background's level by band, the
phone's response and processing stated (its low-frequency limit). The preset rendered in the
same situation (the listener at the same place, the same drive) and compared by
`tools/compare_real.py` and `tools/orders.py`, then blind on the A/B switch.

**Required exit evidence:** the measures, the preset's against the recording's, each difference
traced to a link of the chain or a part of the car; what was changed, within physical ranges.

**Execution record:** `docs/stage-146.md`.

<a id="stage-147"></a>
## Stage 147 — The pipes on several cores

**Dependencies:** [144](15-exhaust.md#stage-144).

**Deliverable:** the pipe solver's step split over a pool of threads owned by the host (the core
stays free of allocation and of threads of its own: it is given a way to run independent work).
Within a step pipes meet only at their ends, so each phase (flows, cells, faces) runs pipe by
pipe in parallel and the ends are joined in a fixed order: renders bit-identical whatever the
number of threads. The real-time host and the app use it; the offline tools take a thread count.

**Required exit evidence:** `cli_parity` and the host's parity bit-identical at 1, 2, 4 and all
threads; the speed-up on the laptop (2 performance and 8 efficiency cores) and on Bernardo's PC
(24 cores); the render thread's rules (no allocation, no locks that can wait on another thread
for long) kept; ThreadSanitizer clean in CI.

**Execution record:** `docs/stage-147.md`.

<a id="stage-148"></a>
## Stage 148 — Finer cells, the noise layer rebalanced

**Dependencies:** [147](#stage-147), [144](15-exhaust.md#stage-144).

**Deliverable:** a fidelity tier whose cells carry the gas's sound to about 5 kHz (a quarter of
today's, 144's measurement), the limits raised for it (cells an engine and a pipe, the pipes'
rate), and the noise layer turned down by band where the gas now carries the sound, so nothing is
counted twice: a candidate on the A/B switch. 043's higher-order scheme reconsidered once the
cores pay for it.

**Required exit evidence:** 144's grid study converged to the new band; the whole mix against
the recordings (bands, slope, crest, repeats) before and after; speed by tier on both machines;
Bernardo's A/B.

**Execution record:** `docs/stage-148.md`.

<a id="stage-149"></a>
## Stage 149 — The source-to-ear chain, every link a measure and a control

**Dependencies:** [146](#stage-146) (its recording checks the defaults), [056](05-engine-qualification.md#stage-056).

**Deliverable:** each link between the car and the listener checked in the code, measured and,
where it is a condition of the listening, made a parameter (rb.h) with a physically right
default and shown in the app:
- the tailpipe's directivity (louder along its axis at high frequencies), the intake mouth's
  radiation load;
- the car's body: what it blocks, what the bonnet lets out, the cabin;
- the listener's place (distance, angle, height; cabin, tailpipe, bonnet, pass-by);
- the ground (asphalt, grass, concrete) and its reflection;
- the air: temperature and humidity (its absorption, ISO 9613-1, and the speed of sound), wind;
- the surroundings: open road, a street between buildings, a car park, a garage, a tunnel,
  their early reflections and reverberation;
- background: traffic, wind on the microphone, none;
- "as a phone heard it", for fair comparison with recordings.

**Required exit evidence:** each link against its reference (a standard, a published
measurement or the recording), its cost; the controls in the app; Bernardo's listening.

**Execution record:** `docs/stage-149.md`.

<a id="stage-150"></a>
## Stage 150 — Two ears and the level

**Dependencies:** [149](#stage-149).

**Deliverable:** binaural rendering for headphones (a measured head-related transfer function,
licence noted, convolved in real time; the car placed outside the head, turning with the
listener's place), a speakers mode; the playback level: a calibration of the output (a tone at a
known level against the listener's ears or a phone's meter) so a car at 1 m plays as loud as it
is, and the app saying when it is too quiet to judge.

**Required exit evidence:** localisation and level against references; the render thread's cost;
Bernardo's blind listening with headphones.

**Execution record:** `docs/stage-150.md`.

<a id="stage-151"></a>
## Stage 151 — Junctions with their size and angles

**Dependencies:** [147](#stage-147), [049](04-propagation.md#stage-049).

**Deliverable:** collectors, X- and H-pipes and Y-junctions as 1D branches meeting in a small
volume, with their angles' losses (a pressure-loss junction model), checked against 049's
finite-volume junctions above the 100–200 Hz where a single volume holds; a candidate on the
A/B switch.

**Required exit evidence:** the junctions' transfer against 049's references by band; the firing
orders at the outlets before and after; the books; speed; Bernardo's A/B.

**Execution record:** `docs/stage-151.md`.

<a id="stage-152"></a>
## Stage 152 — The losses above the plane wave

**Dependencies:** [148](#stage-148), [146](#stage-146).

**Why (148's finding, docs/stage-148.md):** on studio's grid, converged to about 3.2 kHz, the
gas alone is about 10 dB brighter above 2 kHz than Bernardo's 458 clips, where the high quality's
mix matches them: the coarse grid's numerical damping had been standing in for real losses. No
one modelled part explains it (the walls, the packing four times as lossy, the catalyst four
times, the centre route narrowed: each 1–3 dB, and evenly across the bands).

**Deliverable:** the losses a real exhaust has at kilohertz that the plane-wave pipes leave out,
each from its physics and each a candidate on the A/B switch: a packed core's lining by
frequency (Delany–Bazley's fibrous material, Piening's lined duct, the noise layer's already),
the perforates' and a catalyst's narrow channels' viscothermal loss, the boxes above their first
cross mode (where a can stops being a plane-wave chamber), the mean flow's turbulent absorption,
and the source's own steepness (the blowdown's front against measured port pressures).

**Required exit evidence:** each part's transmission against published measurements to 5 kHz;
studio's mix against Bernardo's Polo recording at its known place (146) and his 458 clips, by
band, slope and crest, before and after; the noise layer recalibrated where it changes; the
books; speed; Bernardo's A/B.

**Execution record:** `docs/stage-152.md`.
