"""Stage 144: the walls' viscothermal loss (pipe.h, rb_wall_fit; variant 6) can fail its tests.

python tools/plant_bugs.py 144 WORKDIR all

Its thermal part left out (only the viscous layer's loss); the bore's diameter taken for its
radius (half the loss); the sections' slow parts never following the flow (a drag on the whole
flow, not its swings); a section's corner a decade off; turned on without starting the
sections at the flow (a jump); the A/B switch never reaching the engine.
"""
FIT = ["test_walls_fit_follows_kirchhoff"]
RING = ["test_walls_pipe_rings_down_as_kirchhoff_says"]
SWITCH = ["test_walls_switch_live_without_a_jump"]

BUGS = {
    'thermal-part-missing': ("core/src/pipe.c",
        "    const double kappa = 1.0 + (gamma - 1.0) * WALL_PER_ROOT_PR;",
        "    const double kappa = 1.0 + 0.0 * (gamma - 1.0) * WALL_PER_ROOT_PR;",
        RING, 600),
    'diameter-for-radius': ("core/src/pipe.c",
        "    const double drag = 2.0 * per_radius * kappa * sqrt(nu) * w->dt;",
        "    const double drag = 1.0 * per_radius * kappa * sqrt(nu) * w->dt;",
        RING, 600),
    'sections-frozen': ("core/src/pipe.c",
        "    for (int k = 0; k < RB_WALL_SECTIONS; k++) f->wall[k] += w->a[k] * (mdot - f->wall[k]);",
        "    for (int k = 0; k < RB_WALL_SECTIONS; k++) f->wall[k] += 0.0 * w->a[k] * (mdot - f->wall[k]);",
        RING, 600),
    'corner-a-decade-off': ("core/src/pipe.c",
        "        w->a[k] = 1.0 - exp(-xi[k] * dt);",
        "        w->a[k] = 1.0 - exp(-(k == 3 ? 10.0 : 1.0) * xi[k] * dt);",
        FIT, 300),
    'switched-on-with-a-jump': ("core/src/pipe.c",
        "            for (int j = 0; j < RB_WALL_SECTIONS; j++) p->face[k].wall[j] = p->face[k].mdot;",
        "            for (int j = 0; j < RB_WALL_SECTIONS; j++) p->face[k].wall[j] = 0.0;",
        SWITCH, 600),
    'switch-never-reaches-the-engine': ("core/src/rb_sim.c",
        "        if (e->walls != (sim->variant == 6)) rb_engine_walls(&sim->engine, sim->variant == 6);   /* 144's */",
        "        if (0 && e->walls != (sim->variant == 6)) rb_engine_walls(&sim->engine, sim->variant == 6);",
        SWITCH, 600),
}
