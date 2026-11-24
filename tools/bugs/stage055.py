"""Stage 055: the engine's structure and accessories (noise.c's candidate) can fail their tests.

python tools/plant_bugs.py 055 WORKDIR all
The first bending mode's beta L wrong; the shapes not normalized over the cylinders; a boxer's
banks bending the block the same way; the shear correction dropped; a slap striking with the
force's weights; the chain's order wrong; the variant never reaching the noise; the candidate
drawing more random numbers than the accepted sound.
"""
MODES = ["test_structure_modes_from_geometry"]
PATHS = ["test_structure_paths_per_cylinder"]
EVENTS = ["test_structure_events_and_energy"]
ACC = ["test_structure_accessories"]
SIM = ["test_structure_variant_reaches_the_noise"]
STATS = ["test_structure_order_statistics"]

BUGS = {
    'beta-l-wrong': ("core/src/noise.c",
        "static const double BETA_L[2] = { 4.73004074486, 7.85320462410 };",
        "static const double BETA_L[2] = { 4.5, 7.85320462410 };",
        MODES, 300),
    'weights-not-normalized': ("core/src/noise.c",
        "    for (int m = 0; m < 2; m++) rms[m] = sqrt(rms[m] / d->cylinders);",
        "    for (int m = 0; m < 2; m++) rms[m] = 1.0 + 0.0 * rms[m];",
        MODES, 300),
    'boxer-banks-alike': ("core/src/noise.c",
        "            case CM_L1: w[0] = v1 * sn; w[1] = v1 * fabs(c);  break;",
        "            case CM_L1: w[0] = v1 * fabs(sn); w[1] = v1 * fabs(c);  break;",
        MODES + PATHS, 300),
    'no-shear': ("core/src/noise.c",
        "#define TIMOSHENKO     ((1.0 + 2.0 * 1.33 / (5.0 / 6.0)) / 12.0)",
        "#define TIMOSHENKO     0.0",
        MODES, 300),
    'slap-with-force-weights': ("core/src/noise.c",
        "                           : n->cweight[i][k][by == BY_SLAP];",
        "                           : n->cweight[i][k][0];",
        PATHS, 300),
    'chain-order': ("core/src/noise.c",
        "#define CHAIN_TEETH       21.0",
        "#define CHAIN_TEETH       22.0",
        ACC, 300),
    'variant-never-reaches': ("core/src/rb_sim.c",
        "        sim->noise.variant = (sim->variant == 1 || sim->variant == 2) ? sim->variant : 0;   /* 055's */",
        "        sim->noise.variant = 0;",
        SIM, 300),
    'candidate-draws-more': ("core/src/noise.c",
        "    if (n->variant) cstrike(n, cin, by, i, size);",
        "    if (n->variant) { cstrike(n, cin, by, i, size); (void)white(&n->state); }",
        EVENTS, 300),
    # 2026-11-24: the candidate's block 5 dB quieter (Bernardo: its ringing heard as a whine)
    'trim-missing': ("core/src/noise.c",
        "        bay *= STRUCTURE_TRIM;",
        "        bay *= 1.0;",
        ["test_structure_events_and_energy"], 300),
}
