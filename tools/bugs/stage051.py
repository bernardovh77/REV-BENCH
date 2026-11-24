"""Stage 051: the tailpipes' radiation load against independent results can fail its tests.

python tools/plant_bugs.py 051 WORKDIR all
The end's mass of air wrong (its reactive term), its resistance wrong, and the load twice the
air's impedance: each moves the reflection, the end correction or the power away from an
unflanged pipe's and a monopole's.
"""
PIPE = ["test_radiation_matches_an_unflanged_pipe"]
POWER = ["test_radiation_takes_what_a_monopole_radiates"]

BUGS = {
    'end-mass-wrong': ("core/src/radiation.c",
        "        const double k = 2.0 * r->tau / dt, square = 0.457 * k * k;",
        "        const double k = 2.0 * r->tau / dt, square = 0.300 * k * k;",
        PIPE, 300),
    'resistance-wrong': ("core/src/radiation.c",
        "        r->b0 = (1.226 * k + square) / den;",
        "        r->b0 = (1.000 * k + square) / den;",
        PIPE + POWER, 300),
    'load-doubled': ("core/src/radiation.c",
        "    r->impedance = rho * c;",
        "    r->impedance = 2.0 * rho * c;",
        PIPE + POWER, 300),
    # 2026-11-24: the mono listener's openings (variant 5, accepted the same day)
    'aperture-wrong-axis': ("core/src/audio.c",
        "            const double r = distance(p, at), along = at[1] - p[1];",
        "            const double r = distance(p, at), along = at[0] - p[0];",
        ["test_listener_mono_aperture"], 300),
    'aperture-reads-ahead': ("core/src/audio.c",
        "    l->lag = ceil(most / pipe_dt) + 1.0;",
        "    l->lag = 0.0 * most;",
        ["test_listener_mono_aperture"], 300),
    'aperture-never-placed': ("core/src/rb_sim.c",
        "                         e->pipe_dt, sim->dt);   /* the tailpipes' openings (stage 051) */",
        "                         e->pipe_dt, -1.0);",
        ["test_listener_mono_aperture"], 300),
}
