# S2 — Additional flow sources and optional spatial resolution

2026-10-21. Implementation advanced; numerical and listening acceptance are
reported separately in `docs/checkpoint-13b.md`. **S2 is not marked complete.**

## Sources

The existing internal-jet approximation now also drives intake-valve roar and
chambered-muffler baffle noise. Its acoustic power is

```text
W = eta * M^5 * (mdot * U^2 / 2),  M <= 1
p_rms(1 m) = sqrt(rho_ambient * c_ambient * W / (4 pi))
```

This uses actual modeled mass flow, upstream density/temperature and effective
opening area. It extends the simplified jet model already researched and
documented in `docs/stage-12b.md`; it is not a new validated valve-noise standard.
Intake-valve contributions add in power and pass through the airbox attenuation
and a 150 Hz/1.2 kHz/4.5 kHz shaped band. Their propagation back through the
intake is a lumped approximation, not resolved 3D turbulent acoustics.

Only chambered mufflers generate the baffle contribution. It enters downstream
of the catalyst and the main muffler attenuation, then travels along the
tailpipe before the existing output band. Reverse flow and zero opening/flow
do not create the positive-jet contribution. No fixed noise floor is added on
overrun. These sources have a separate seeded random stream, preserving the
existing sources' random draws. Coefficients remain provisional until listening
and comparison with suitable real recordings; no broad EQ retune was made.

## Optional ultra quality

`RB_QUALITY_ULTRA` / `rb_cli sound --quality ultra` uses approximately 384 kHz
physics and 96 kHz pipes at 48 kHz output. The nominal short/long grid spacing
halves from 5/7.5 cm to 2.5/3.75 cm, still bounded by the existing CFL sizing,
per-pipe maximum and fixed engine cell pool. The original low/medium/high
presets and default high quality are retained. Ultra is an optional convergence
and listening comparison, **not a declaration that its extra cost is audibly
justified**. See the paired high/ultra files and timings in the checkpoint.

The new stereo scene path includes an eight-point Rayleigh disk aperture
approximation for tailpipe directivity. Its angular high-frequency attenuation
is generated from source size and path angle, rather than a shared brightness
correction for the supplied recordings. Details and limits: `docs/stage-S4.md`.
The original mono listener retains its existing point-source radiation.

The valve-noise coefficient has not been reduced automatically for ultra:
finer 1D cells do not by themselves resolve 3D turbulent jet noise. Any reduction
needs evidence separating resolved pressure waves and unresolved turbulence.

## Validation

Native tests verify silence at no flow, increasing intake power and audible
output with increasing mass flow, and baffle noise only for the chambered path.
Quality tests include the new rates; existing conservation, source-isolation,
noise-power, spectral and browser checks remain required. Stage completion
still requires the planned spectral/repetition/overrun checks and Bernardo's ears.
