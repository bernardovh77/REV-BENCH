"""Stage 115: the review's repairs (the audit's D02-D05, and the cycles'
variation back at the real engines' 1-3 % COV of IMEP wide open). Applied
only to isolated copies of the sources:

    python3 tools/plant_bugs.py 115 WORKDIR all
"""
BUGS = {
    # D02: the jet's speed and Mach number in the gap
    'jet-never-chokes': ("core/src/noise.c",
        "    if (x < choked) x = choked;   /* choked: sonic in the gap */\n",
        "",
        ['test_noise_jets_at_the_gap'], 300),
    'jet-speed-of-upstream-gas': ("core/src/noise.c",
        "const double u2 = 2.0 * gamma / (gamma - 1.0) * up->gas.R * up->T * (1.0 - x);",
        "const double u2 = gamma * up->gas.R * up->T * (1.0 - x);",
        ['test_noise_jets_at_the_gap', 'test_noise_valve_jets'], 300),
    'jet-against-the-pressure': ("core/src/noise.c",
        "down->p >= 0.0 && down->p < up->p",
        "down->p >= 0.0",
        ['test_noise_jets_at_the_gap'], 300),
    # D03: through an X or Y junction, on to every tailpipe beyond it
    'junction-ignored': ("core/src/noise.c",
        "if (d->junction_volume > 0.0) memcpy(reach, beyond, sizeof reach);",
        "if (0) memcpy(reach, beyond, sizeof reach);",
        ['test_noise_valve_routing'], 300),
    'junction-doubles-power': ("core/src/noise.c",
        "reach[s] ? 1.0 / sqrt((double)count) : 0.0;",
        "reach[s] ? 1.0 : 0.0;",
        ['test_noise_valve_routing'], 300),
    'junction-uncorrelated': ("core/src/noise.c",
        "n->blow_ring[s][at] += (float)(x * n->blow_share[i][s]);",
        "n->blow_ring[s][at] += (float)(x * n->blow_share[i][s] * (s ? -1.0 : 1.0));",
        ['test_noise_valve_routing'], 300),
    # D04: the path down the centre pipe and link
    'path-without-centre': ("core/src/noise.c",
        "const double middle = (d->center_length > 0.0) ? d->center_length + d->link_length : 0.0;",
        "const double middle = 0.0;",
        ['test_noise_valve_jets'], 300),
    # D05: the catalyst's share only with a catalyst
    'catalyst-always': ("core/src/noise.c",
        "(d->catalyst_laminar > 0.0 ? PASS_CATALYST : 1.0) * pass",
        "PASS_CATALYST * pass",
        ['test_noise_valve_jets'], 300),
    # the cycles' variation
    'half-normal-kernel': ("core/src/engine.c",
        "const double delay=v * VARY_START * expected * rb_kernel_delay_unit(start);",
        "const double delay=v * VARY_START * expected * fmax(start, 0.0);",
        ['test_variation_cov_of_imep'], 900),
    'kernel-mean-wrong': ("core/src/combustion.c",
        "return 1-2*q-log1p(-q);",
        "return 1-q;",
        ['test_combustion_conditional_means'], 300),
    'idle-spark-ignores-delay': ("core/src/engine.c",
        "const double s = IDLE_SPARK + kernel_delay + IDLE_SPARK_GAIN",
        "const double s = IDLE_SPARK + IDLE_SPARK_GAIN",
        ['test_variation_cov_at_idle'], 900),
}
