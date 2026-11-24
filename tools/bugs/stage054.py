"""Stage 054: the noise's routing through the exhaust's own gas (noise.c's variant 3) can fail its tests.

python tools/plant_bugs.py 054 WORKDIR all
The path times without the gas's own flow; a cell's sound speed at the air's gamma; the centre pipes
left out of the path; the routing never worked out again; the X junction's centre pipes all into
one outlet; the reversed intake jets never heard; the reversed exhaust jets heard as well as the
forward ones; variant 3 never reaching the noise.
"""
PATHS = ["test_routing_path_times_follow_the_gas"]
EDIT = ["test_routing_geometry_edit"]
REVERSE = ["test_routing_reverse_jets_heard"]
SIM = ["test_structure_variant_reaches_the_noise"]

BUGS = {
    'no-flow-in-path': ("core/src/noise.c",
        "        const double v = sound + u;",
        "        const double v = sound + 0.0 * u;",
        PATHS, 300),
    'air-gamma': ("core/src/noise.c",
        "        const double sound = sqrt(rb_chamber_gamma(c) * c->gas.R * c->T);",
        "        const double sound = sqrt(1.4 * c->gas.R * c->T);",
        PATHS, 300),
    'centre-left-out': ("core/src/noise.c",
        "        middle[c] = extended ? pipe_time(&e->center[c]) + pipe_time(&e->link[c]) : 0.0;",
        "        middle[c] = 0.0 * extended;",
        PATHS, 300),
    'routed-once': ("core/src/noise.c",
        "            n->route_tick = RB_NOISE_ROUTE_EVERY;",
        "            n->route_tick = 1 << 30;",
        EDIT + PATHS, 300),
    'tailpipe-left-out': ("core/src/noise.c",
        "        rear[s] = pipe_time(&e->muffler[s]) + pipe_time(&e->tailpipe[s]);",
        "        rear[s] = pipe_time(&e->muffler[s]);",
        EDIT + PATHS, 300),
    'reverse-intake-unheard': ("core/src/noise.c",
        "            n->reverse_power += reverse_jet(&e->intake[i], GAMMA_BURNED, VALVE_ETA);",
        "            n->reverse_power += 0.0 * reverse_jet(&e->intake[i], GAMMA_BURNED, VALVE_ETA);",
        REVERSE, 300),
    'reverse-exhaust-unheard': ("core/src/noise.c",
        "        if (routed && !(w > 0.0)) w = reverse_jet(&e->exhaust[i], GAMMA_BURNED, THROTTLE_ETA);",
        "        if (routed && !(w > 0.0)) w = 0.0 * reverse_jet(&e->exhaust[i], GAMMA_BURNED, THROTTLE_ETA);",
        REVERSE, 300),
    'variant-3-never-reaches': ("core/src/rb_sim.c",
        "        sim->noise.variant = (sim->variant >= 1 && sim->variant <= 3) ? sim->variant : 0;   /* 055's, 054's */",
        "        sim->noise.variant = (sim->variant >= 1 && sim->variant <= 2) ? sim->variant : 0;",
        SIM, 300),
}
