"""Stage 053: the flow noise's booking, its reverse flows and its statistics can fail their tests.

python tools/plant_bugs.py 053 WORKDIR all
A valve's share of the stream power doubled (stage 115's test of the valves' law); reversed flows heard with the
forward law (their sign and their upstream state wrong); the hiss's amplitude not the square root
of its power (so twice the flow isn't twice the power); a floor that never lets a still gap
fall silent.
"""
REVERSE = ["test_flow_noise_reverse_jets_measured"]
STATS = ["test_flow_noise_statistics_over_seeds"]

BUGS = {
    'eta-doubled': ("core/src/noise.c",
        "#define VALVE_ETA      5e-4",
        "#define VALVE_ETA      1e-3",
        ["test_noise_valve_jets"], 300),
    'reverse-jets-heard': ("core/src/noise.c",
        "    if (!(o->area > 0.0 && o->mdot > 0.0)) return 0.0;   /* shut, or flowing back */",
        "    if (!(o->area > 0.0 && o->mdot != 0.0)) return 0.0;",
        REVERSE, 300),
    'hiss-amplitude-not-power': ("core/src/noise.c",
        "    const double hiss = band_step(&n->hiss, sqrt(radiate * w * PASS_AIRBOX) * white(&n->state));",
        "    const double hiss = band_step(&n->hiss, sqrt(radiate * PASS_AIRBOX) * sqrt(sqrt(w) * 1e-2) * white(&n->state));",
        STATS, 300),
    'a-still-gap-hisses': ("core/src/noise.c",
        "    const double hiss = band_step(&n->hiss, sqrt(radiate * w * PASS_AIRBOX) * white(&n->state));",
        "    const double hiss = band_step(&n->hiss, sqrt(radiate * (w + 1e-9) * PASS_AIRBOX) * white(&n->state));",
        STATS, 300),
}
