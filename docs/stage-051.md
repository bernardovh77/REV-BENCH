# Stage 051: radiation load and audible-field consistency

Worked on 2026-11-23 by Claude (Opus 5.5) overnight, with Bernardo asleep. The card is
[docs/roadmap/04-propagation.md](roadmap/04-propagation.md#stage-051). It stays 🟡 for two
stated limits (below) that would change the sound, and so need Bernardo.

**The question:** is the load the open air puts on the tailpipes' ends (`core/src/radiation.c`)
right, over what range, and is the sound the listener hears the same account of the same
energy, heard once, from the right distance and in time?

## The gaps before it

PLAN.md's open rows: 069's and 066's real-device runs and listening, 118's misses, the fresh
engine's tone (phase B: it lives in the pipes' heat transfer, not the ends), 039, 041, 042, 014.
None touches the ends' load: safe to leave.

## What the core does

- **The load** (stage 121, `rb_radiation_*`): Silva et al.'s (2009) passive, causal rational
  fit of Levine and Schwinger's unflanged pipe, a second-order section from the end's velocity
  to its pressure, made discrete by the bilinear transform and taken implicitly in the pipe's
  step (`pipe.c`). The air outside is fixed; mean jets, flanges and the car's body are outside it
  (radiation.h says so).
- **The far field** (stage 11-12, `rb_listener_*`): each end a monopole of its volume flow,
  p = rho / (4 pi r) dQ/dt, straight and off the ground, each path delayed by its extra length
  over c from the nearest; it reads the flows and puts nothing back on the pipe.

## The evidence

| Card's evidence | Test | Result |
| --- | --- | --- |
| The terminal response against an independent control | `test_radiation_matches_an_unflanged_pipe` (new): the core's load driven at ka 0.02-1.5, its reflection and end correction against Norris and Sheng's (1989) fit of the same exact result, a different form from Silva's; their exact low end | \|R\| within 0.013, l/a within 0.026 (the worst at ka 1.5: 0.446 against 0.472); at ka 0.02 \|R\| 0.9998 (1 − ka²/2) and l/a 0.6129 (0.6133) |
| Work: what the load takes is what radiates | `test_radiation_takes_what_a_monopole_radiates` (new): at ka 0.02-0.1, the mean of p v S against a monopole's rho w² \|Q\|² / (8 pi c) | within 0.1 % (4.037e-5 against 4.041e-5 W at ka 0.02); only the real part works |
| Causality, passivity, no artificial back pressure | `test_radiation_causal_impedance` (stage 121) | the discrete load is the continuous one at the warped frequency to 1e-6; it absorbs (Re Z > 0); a steady flow sees no pressure |
| The pipe's books with the load on | `test_radiation_pipe_work` (stage 121) | mass and energy out of the end match what left the pipe; the end's outer mass belongs to the load alone |
| Far-field magnitude, phase, distance | `test_audio_listener_monopole` (stage 11) | rho w Q0 / (4 pi r) cos(w t) to 1e-4, half at twice the distance |
| Source distance and causality, the paths | `test_audio_listener_paths` (stage 12) | each path at its 1/r, (r − r_nearest)/c later, the ground's at its image's distance |
| No double load | by construction, and the two above: the load is the pipe's end's only (pipe.c's face), the listener only reads the flows | the same monopole in both accounts: the load's power is the far field's |

**The declared range:** the plane wave (ka under 1.84, the first cross mode), checked to 1.5;
for the presets' tailpipes (radius 2.2-3.8 cm, gas at 300-900 K) that is up to about 2-3 kHz,
above which the pipes' grid (2.4 kHz, the 7.5 cm cells) stops carrying waves anyway.

**Planted bugs** (`tools/bugs/stage051.py`): 3 of 3, the end's mass wrong, its resistance
wrong, the load doubled.

## The limits, stated (what keeps it 🟡)

1. **Directivity.** The listener hears each end as a monopole, the same in every direction. An
   unflanged pipe's radiation is that at low ka; by ka 1 it is several dB weaker towards its
   back (Levine and Schwinger). The "behind" listener stands near the pipe's axis, where the
   monopole is closest; "side" and "front" hear the highs (above about 2 kHz for a 2.5 cm
   tailpipe) a little too strong. A directivity filter changes the sound: for Bernardo's ears
   (and 056, the listener's validation).
2. **The mean flow at the end.** The load is the still air's. A jet leaving the end (Mach up to
   0.25-0.5 wide open, stage 129's measurement) absorbs low frequencies at the end (Munt's
   jet: |R| below 1 even as ka goes to 0) and moves the end correction. Not modelled; it would
   change the sound's low end: 054/056 with his ears.
3. **Flanges, flares and the body:** none; a tailpipe is a bare pipe in free air (with the
   ground's reflection).

## Files

New: `tools/bugs/stage051.py`, this doc. Changed: `tests/test_radiation.c` (two tests),
`tests/test_main.c`. The core is unchanged.
