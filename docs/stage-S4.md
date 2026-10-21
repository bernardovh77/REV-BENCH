# S4 — Separate sources, stereo listeners and acoustic scenes

2026-10-21. Native and WASM implementation; listening acceptance pending.
The browser remains the stage-2 tone demo until stage 14.

## Signal path

`rb_set_scene(sim, scene)` enables an optional scene. `rb_render_stereo` returns
interleaved L/R frames; mono rendering with a selected scene returns their mean.
Without scene selection the original mono listener remains in use. The
configuration rejects unsupported scene audio rates outside 8–96 kHz before
changing the state. The existing mono API continues supporting its full range.

At every physics step each of the two tailpipes and intake mouth contributes
`rho/(4 pi) * dQ/dt`, plus its independently generated noise pressure at 1 m.
The bay and mount pressures stay separate. Five anti-alias filters reduce these
to audio rate; propagation then operates on individual sources. Each ear has
its own source distances, interpolated delays and filters. This is source
placement, not a stereo copy or pan of the final mono mix. All scene memory is
fixed in the simulation; no allocation occurs while rendering.

## Geometry and approximations

- Coordinates retain the existing front-engined car geometry. Ears are 18 cm
  apart, facing forward. Direct and ground-image paths use pressure gain `1/r`,
  with a 10 cm near-field floor. The ground reflection coefficients are 0.9
  for asphalt and 0.35 for grass. These are illustrative scalar coefficients,
  not frequency-dependent measured ground impedances.
- The wall scene adds the source image in the plane x = -3 m at coefficient
  0.7. Each reflection has its own delay. The grass/open/wall scenes share the
  same engine and source conditions.
- A first-order, frequency-dependent far-ear shadow approximates head
  obstruction; interaural delay comes from the actual ear coordinates. This
  captures broad cues only: it is not a measured HRTF, individualized head,
  pinna/elevation model or the full [Duda–Martens sphere solution](https://www.ece.ucdavis.edu/cipic/wp-content/uploads/sites/12/2015/04/cipic_JASA_Nov_1998.pdf).
- Tailpipe aperture delays use eight-node Gauss–Chebyshev quadrature of the
  uniform circular disk's Rayleigh integral. The analytic far-field benchmark
  is `2 J1(ka sin(theta))/(ka sin(theta))`; see
  [Penn State's acoustic-radiation explanation](https://www.acs.psu.edu/drussell/demos/baffledpiston/baffledpiston.html).
  An exhaust opening is not an infinite baffled piston: rear radiation/body
  diffraction and near-field behaviour remain approximations.
- The ISO scene places the centre 0.5 m from tailpipe 0 at 45 degrees and
  tailpipe height. It is a listening geometry, not a certified regulatory
  measurement or calibrated sound-level test.
- Cabin rendering reuses the existing front/rear body filters, mount path and
  55 Hz mode separately in each ear. It is not a measured vehicle interior.

## Pass-by and room decay

The pass-by travels at 15 m/s along y past a listener 7.5 m to its side, with
the car's rear origin abreast at reception time 6 s. For every source/image
and ear, emission time solves `|r(t) - v*tau| = c*tau`. With `r` the current
relative source position and velocity along y,

```text
a = c^2 - v^2
tau = (sqrt((r_y v)^2 + a |r|^2) - r_y v) / a
```

The source history is read at this retarded time. Geometry is evaluated every
64 audio frames and interpolated between evaluations. This follows source
motion rather than applying a pitch effect to a final mix; compare
[Smith's physical Doppler discussion](https://www.dsprelated.com/freebooks/pasp/Doppler_Simulation.html).
The finite history covers the intended 12-second pass. A path beyond the
available history yields silence instead of wrapping to unrelated audio.

Garage and dyno scenes add a four-line Householder feedback delay network:
`v - 0.5 sum(v)` is an orthogonal reflection, with per-line gain
`10^(-3 delay_seconds / RT60)` and high-frequency damping. The illustrative
RT60 settings are 1.2/0.45 s; actual band-dependent decay is shorter because of
damping. The network is not fitted to a measured room. Its design basis is the
orthogonal damped FDN described in
[Dal Santo et al., DAFx 2023](https://www.dafx.de/paper-archive/2023/DAFx23_paper_32.pdf).
Four lines are economical but can colour the tail; listening acceptance and
measured-room fitting remain future work.

## Validation and use

Native tests verify retarded propagation against the analytic static,
approaching and receding cases; impulse arrival; finite decaying reverberation;
stereo differences; invalid configurations; reset; and exact block-split
repeatability. Additional measured checks and validation totals are in
`docs/checkpoint-13b.md`. These tests establish implementation properties, not
audible realism approval.

```sh
build/rb_cli sound --preset v8-cross --rev --scene asphalt --gain 0.2 --float --out renders/cross-stereo.wav
build/rb_cli sound --preset v8-cross --hold --rpm 3000 --scene passby --gain 0.12 --float --out renders/passby.wav
```

Scene names: `asphalt`, `grass`, `wall`, `garage`, `dyno`, `iso`, `passby`,
`cabin`. References with uncertain cabin/exterior/flyby perspective must still
be kept separate; selecting a scene does not verify the recording's viewpoint.
