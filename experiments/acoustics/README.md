# Native acoustic candidates — S2d

These C11 research modules implement the first fixtures from
[the S2d design](../../docs/stage-S2d-design.md). They are linked to the native
test/probe programs, **not** the production renderer or WASM. They allocate
nothing: callers own all workspaces and must keep prepared plans immutable.

- `wave.[ch]`: homogeneous, frozen, zero-mean-flow acoustic graph. Pressure
  waves are Pa; `Z=rho*c/A` is Pa s/m³; junction flow sources are m³/s.
  Ports point toward their node; outgoing pressure is `p_node-incoming`.
  Each edge has independent delay lines in both directions. Node IDs are
  local plan indices; the host must bind semantic physical component IDs.
- `euler.[ch]`: uniform-area constant-gamma finite volume control. MC-limited
  primitive reconstruction, HLLC with recorded HLLE fallback, SSP-RK2 and a
  passive tagged-gas density. Periodic or transmissive ends. Conserved
  quantities are densities per m³, not chamber contents. Gamma is frozen
  from the reference gas state; this is not the production mixture EOS.

The graph's all-pass fractional delay uses an orthogonal state update, so
delay storage has an explicit quadratic energy. Linear interpolation is a
comparison control with separately booked numerical dissipation. Fixed
propagation attenuation has a separate loss book. Finite-volume nodes use
implicit midpoint compliance with stored energy `C*p_state²/2`. Their port
pressure is the midpoint pressure, as tested against a bilinear frequency
response. `terminal_work` includes radiation-filter storage/work; it is not
instantaneous radiated energy. No extra terminal load is applied elsewhere.

The API rejects unsupported topology/capacity, an edge shorter than one
sample, nonfinite source values and active static gains. It does not round
short edges up, clip growing pressure or silently repair a failed state.
Graph coefficient updates, moving gas and nonlinear engine coupling remain
future work. Do not inject full graph output on top of the old exhaust sound.

## Open-port coupling checkpoint

`coupled_duct.[ch]` adds the bounded [S2d-B1a proof](../../docs/stage-S2d-0b-B1a.md):
one physical conserved state per cell, upstream nonlinear FV integration,
downstream characteristic streaming and distributed nonlinear/entropy transport.
It is a uniform-area constant-gamma duct experiment, not a production engine or
arbitrary B0 graph adapter. Density, momentum, total energy and passive tagged
gas cross one accepted interface. Total energy already owns pressure work;
do not add a separate acoustic energy reservoir. Actual physical pressure
includes kinetic-energy subtraction; reported characteristic amplitudes do not.

B0 now offers receive/incident/commit at the same tick; the legacy step wrapper
is retained. A received state can retry an invalid drive without consuming
delay storage. A duplicate receive or wrong-phase commit is an error. Plans
stay immutable. The coupled-duct experiment has its own encapsulated tick and
rejects non-sample-aligned geometry, inadmissible states and excessive residual
CFL without clipping/repairing them. Both remain allocation-free.

```sh
make BUILD=/tmp/rev-bench-s2d-next LTO=0 coupled-probe
PYTHONPATH=tools .venv-analysis/bin/python tools/run_coupled_duct.py
PYTHONPATH=tools .venv-analysis/bin/python tools/coupled_duct_report.py
```

The whole-engine gas/EOS adapter, arbitrary multiports, evolving backgrounds,
temperature/reversal and runtime/listening promotion remain open. The
downstream correction grid is retained at the common rate in this proof;
no inexpensive reduced mean model or real-time improvement is claimed.

## Reproduce

From the repository root, using an isolated build directory:

```sh
make BUILD=/tmp/rev-bench-s2d LTO=0 acoustic-probe
python3 tools/run_acoustic_candidates.py
MPLCONFIGDIR=/tmp/rev-bench-mpl .venv-analysis/bin/python tools/analyze_acoustic_candidates.py --publish
make BUILD=/tmp/rev-bench-s2d LTO=0 test
python3 tools/plant_bugs.py S2d /tmp/rev-bench-s2d-mutants all
```

The runner's `--quick` selects a small screen; `--reuse` accepts only exact
source/command matches with retained case output. The full matrix has 124 FV
rows (18 spatial exclusions) and 220 graph rows. Every case retains command,
source hashes and stderr, including rejected cases. Outputs stay in ignored
`renders/evidence/acoustic-candidates-2026-10-23/`; publication appends plots
to the single existing `renders/RESULTS.html`, preserving its audio previews.

`acoustic_candidate_probe topology` snapshots all five accepted engines, with
semantic IDs such as `i6/header-0`, endpoint connections and actual grid sizes.
`topology-no-runners` is an isolated identity control: removing intake runners
must not rename the exhaust. These are snapshots and contracts, not a full
compiler from engine geometry to validated acoustic components. Real packing,
perforation and material data cannot be inferred from IDs.

The native suite includes delay/reflection timing, weighted four-port and
cancellation, cavity frequency response/storage, causal radiation units/work,
loss accounting, one-second closed loops, contact/shock/rarefaction and FV
conservation. The analysis suite separates numerical harness integrity from
acoustic promotion and rejects missing/duplicate/nonfinite/faulted evidence.

Measurements and remaining gates: [stage notes](../../docs/stage-S2d.md).
References for the independently implemented methods: [Euler Riemann solvers](https://www.clawpack.org/riemann_book/html/Euler_approximate.html),
[waveguide scattering](https://www.di.univr.it/documenti/ArticoloRivista/allegato/allegato979612.pdf),
[fractional all-pass delays](https://www.dsprelated.com/freebooks/pasp/Thiran_Allpass_Interpolators.html)
and [causal pipe radiation](https://arxiv.org/abs/0811.3625).
