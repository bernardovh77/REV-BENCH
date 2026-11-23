# Stage 056: listener, vehicle and recording-transfer validation

Worked on 2026-11-23 by Claude (Opus 5.5), after 055, as part of the three stages Bernardo
asked for. The card is
[docs/roadmap/05-engine-qualification.md](roadmap/05-engine-qualification.md#stage-056).

**Status: 🟡.** The stereo scene's listener is checked against analytic controls:
- its fractional delays and their band;
- the pass-by's Doppler and retarded distance, sample by sample;
- a history that stays in range;
- known-position stereo and the head's shadow;
- the cabin's transfer;
- all eight scenes.

One stop-rule failure was found and fixed. Reading the history between two samples lost up to
2.0 dB at 10 kHz, and the loss varied with each path's fraction of a sample, an unexplained
upper-band loss. The scene now reads by cubic Lagrange interpolation. That changes the stereo
scenes' renders above a few kHz (stated). The mono listener's renders are unchanged.

Two things stay open:
- **Directivity:** the listener hears the tailpipe as a monopole (051's stated limit).
- **Matched recordings (039):** the recording-perspective comparisons need them.

## The gaps before it

| Gap | What it means here |
| --- | --- |
| 051: directivity and the mean jet at the end | The scene's tailpipe is a Rayleigh disk on a monopole's level: a stated limit, carried here as the card's directivity item. |
| 039 | Without matched recordings, the perspective of a reference (exterior, cabin, flyby) can't be checked against the scene; the card forbids inferring the microphone's place from brightness. |
| 055 (just done) | Its candidates come out of the bay source; the scene places that source as before. |
| 053, 052, 050, 135, 118's misses, 066/069, the fresh engine's tone | None touches the listener. |

## The listener today

**The mono listener** (`audio.c`):
- **The flows:** each source's volume flow at its distance, read at the pipe rate between two
  pipe steps. The pipes carry almost nothing above about 2.4 kHz (7.5 cm cells), where reading
  between two samples at 48 kHz loses at most 0.1 dB.
- **The noise sources:** read at the physics rate (192 kHz), where the same loses 0.12 dB at
  10 kHz.
- **Both losses are explained and below any test's tolerance, so the mono listener is left as
  it is.**

**The stereo scene** (`scene.c`, stage 024/S4):
- **What it hears:** each source's pressure 1 m away at the audio rate.
- **The paths:** for each ear, each source and each path (direct, off the ground, off the
  wall), it reads the source's history at the path's own fractional delay.
- **The tailpipe's opening** spreads it over 8 Gauss–Chebyshev nodes across the Rayleigh disk.
- **The far ear** hears through the head's shadow.
- **The rooms:** a four-line FDN in the garage and on the dyno.
- **The cabin:** the body's filters.
- **Moving sources:** the pass-by works its paths out from the retarded flight time every 64
  samples, followed linearly in between.

## The evidence (tests/test_listener.c, with test_scene.c's)

| Card's evidence | Test | Result |
| --- | --- | --- |
| Fractional delays, their band | `test_listener_fractional_delay_band`: one path at half a sample past a whole one (the worst fraction) | **−0.000, −0.015, −0.226, −0.534 dB** at 0.5, 4, 8, 10 kHz (the linear read lost 0.0, −0.3, −1.25, −2.0); the phase is the delay's, 2πfd/fs, to 1e-6 rad; a whole-sample delay passes 15 kHz exactly (to the history's float, 1e-8) |
| Analytic delay and Doppler, moving histories | `test_listener_doppler_analytic`: the pass-by, a 600 Hz tone from the mouth, 10 s | every sample within **2.7e-5 of the loudest** of Σ g_k sin(2πf(t − τ_k)), direct and off the asphalt, τ_k solved independently by bisection, g_k = 1/(cτ_k) (0.9 for the ground); the Doppler factor 1.045 approaching, 0.959 going away; the tailpipe's aperture spread the retarded position's (1e-4), every second |
| Bounded history | `test_listener_bounded_history` and the pass-by | a path past the ring is silence, never a wrapped event; one just inside it is the source (1e-9); a negative delay silence; the pass-by's longest path stays inside the ring |
| Known-position stereo, magnitude and phase | `test_listener_stereo_known_position` | each ear's delay from the mouth is the documented geometry's (1e-8 samples); the far ear's shadow is its stated filter, (1 − 0.7h) + 0.7h H(f), within 0.05 dB at 4 kHz, each ear |
| Cabin and body transfer | `test_listener_cabin_transfer` | behind and in front, 20 Hz to 2 kHz: the body's one-poles at 100 and 200 Hz, 0.3, and the 55 Hz boom, within **0.02 dB** of those filters' own responses |
| All eight scenes | `test_listener_eight_scenes` | asphalt, grass, wall, garage, dyno, ISO, pass-by and cabin each start and answer an impulse from every source with finite sound; the ground in each, grass at 0.35/0.9 of asphalt's; the wall in the wall scene alone; a decaying room in the garage and on the dyno alone; the rest silent after |
| Earlier controls kept | `test_scene_*` (stage 024/S4) | the retarded time's closed form, the Rayleigh disk against Airy's, the room's decay, block-size repeatability: unchanged |
| The renders | `tools/cli_parity.py`, 11 cases | 10 identical; the stereo pass-by differs as stated: 0.00 dB under 2 kHz, +0.08 at 2–4, +0.28 at 4–8, +0.79 at 8–12, +1.21 above 12 kHz (the treble the linear read lost) |

**Planted bugs** (`tools/bugs/stage056.py`): 8 of 8 caught:

| Bug | Caught by |
| --- | --- |
| Reading between two samples again | the band test |
| A history that wraps | the history test |
| The source's place not retarded | the Doppler test's aperture check (a first run missed it: the mouth has no aperture and no shadow at that ear, so the check on the tailpipe's spread was added) |
| The Doppler from the listener's delay alone | the Doppler test |
| The head's shadow too strong | the stereo test |
| The cabin's boom twice as sharp | the cabin test |
| Grass as hard as asphalt | the scenes test |
| The ears swapped | the stereo test |

**Speed:** the bench renders through the pass-by scene, so the four-sample reads are measured: `tools/speed_check.py` (MSVC, interleaved, best of 3) gives a **median of −1.8 %**, inside the 3 % tolerance. The dearest cell is i6 low at −3.5 %, and the scene's sound changes in every cell. That is the price of the scenes' treble, stated here and not hidden. The app plays its stereo scenes through this listener, so the fix reaches the live sound in every scene: 0.1 to 1.2 dB more treble above 2 kHz, and nothing below.

## Distance and recording-perspective uncertainty

Each scene puts its listener at a documented point (`scene.c`, the shared frame: x to the car's
right, y forward from the rear bumper, z up). What a real recording's position does to a
comparison, stated rather than fitted:

| Scene | Listener | A recording's likely error | What it does |
| --- | --- | --- | --- |
| Asphalt, grass, wall | 3 m behind, 0.6 m right, 1.6 m up | ±1 m in distance, ±0.5 m in height | ±3 dB level (1/r); the ground's comb moves (its first null between 300 Hz and 1 kHz) |
| ISO | 0.5 m from the tailpipe at 45° | ±0.1 m, ±15° | ±1.8 dB; the opening's beaming above 3 kHz (aperture test) |
| Pass-by | 7.5 m from the line, 15 m/s | ±1 m, ±2 m/s | ±1.2 dB at the peak; Doppler factor ±0.6 % |
| Cabin | driver's head | seat and head position | the body's filters are the body's (below), not a microphone's |
| Garage, dyno | room | unknown room | RT60 1.2 s and 0.45 s are the scenes', not a measured room's |

- **The microphone's own processing** is not modelled anywhere: no microphone response,
  windscreen or compression. A reference recording's EQ and limiting belong to the recording.
- **A recording's place is never inferred from its brightness** (the card's rule).
- **Gas paths and room paths stay separate.** The pipes' sound is the engine's; the scene's
  paths and rooms are propagation in air after the sources.

## What is left (why 🟡)

1. **Directivity** (051's limit): the tailpipe is a monopole and a Rayleigh disk's beaming.
   A real tailpipe's jet refracts its sound.
2. **Matched recordings (039):** the scenes against real recordings at known positions.
3. **The cabin's filters are stage 12b's design** (one-pole corners at 100 and 200 Hz, 0.3, a
   55 Hz boom), checked here as built, not against a measured car body.

## Files

- **New:** `tests/test_listener.c` (6 tests), `tools/bugs/stage056.py`, this doc.
- **Changed:**
  - `core/src/scene.c`: cubic Lagrange reads;
  - `tests/test_main.c`, docs/architecture.md, PLAN.md, CODEX.md.
