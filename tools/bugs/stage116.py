"""Stage 116: the tests stage 041 and the stages around it had loosened,
tightened again. Each bug passed the loosened test and fails the tightened
one. Applied only to isolated copies of the sources:

    python3 tools/plant_bugs.py 116 WORKDIR all
"""
BUGS = {
    # test_exhaust_junction_scattering_conservation: the energy's books (a
    # chamber that gains a millionth of the energy flowing through it)
    'junction-makes-energy': ("core/src/chamber.c",
        "    ch->U += ch->dU * dt;",
        "    ch->U += ch->dU * dt + 1e-6 * fabs(ch->dU) * dt;",
        ['test_exhaust_junction_scattering_conservation'], 300),
    # test_engine_drives_itself: the fuel cut holds once it's on
    'fuel-cut-flickers': ("core/src/engine.c",
        "        e->fuel_cut = e->overrun_time >= transition;",
        "        e->fuel_cut = e->overrun_time >= transition && !e->fuel_cut;",
        ['test_engine_drives_itself'], 600),
    # test_calibration_burn_and_spark: the burn is the charge model's
    'burn-doubled-reference': ("core/src/engine.c",
        "const double base=capped ? expected:b->demand_duration;",
        "const double base=capped ? expected:fmin(b->demand_duration, 2.0 * e->desc.burn_duration + 1.0);",
        ['test_calibration_burn_and_spark'], 600),
    # test_presets_run_on_their_own: the soft cut keeps the engine at its limit
    'soft-cut-too-early': ("core/src/engine.c",
        "(e->desc.rev_limit - LIMIT_SOFT)) / LIMIT_SOFT;",
        "(e->desc.rev_limit - 120.0 - LIMIT_SOFT)) / LIMIT_SOFT;",
        ['test_presets_run_on_their_own'], 900),
}
