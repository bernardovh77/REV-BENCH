"""Stage 133: the rev limiter's character and the 2-step can fail their tests.

python tools/plant_bugs.py 133 WORKDIR all      (a few at a time, under `timeout`)
The gap not kept, the soft band fixed at its old 150 rpm, the 2-step never holding, holding
above the rev limit, ignoring its own gap, the parameter never reaching the engine, and the
fuel cut cutting nothing.
"""
DEFAULTS = ["test_limiter_defaults_are_the_old_limiter"]
GAP = ["test_limiter_gap_sets_the_bounce"]
BAND = ["test_limiter_band_softens_the_cut"]
FUEL = ["test_limiter_fuel_cut_holds_the_limit"]
LAUNCH = ["test_limiter_launch_holds_its_own_limit", "test_limiter_launch_lets_go"]

BUGS = {
    'gap-not-kept': ("core/src/rb_engine_api.c",
        "        if (field == RB_ENG_LIMITER_GAP) d.rev_limit_resume = d.rev_limit - value;\n",
        "",
        DEFAULTS + GAP, 300),
    'band-fixed': ("core/src/engine.c",
        "    const double band = l ? e->desc.launch_band : e->desc.limiter_band;",
        "    const double band = 150.0;",
        BAND + GAP, 300),
    'launch-never': ("core/src/engine.c",
        "    return e->launch && e->desc.launch_rpm < e->desc.rev_limit;",
        "    return 0;",
        LAUNCH, 300),
    'launch-above-the-limit': ("core/src/engine.c",
        "    return e->launch && e->desc.launch_rpm < e->desc.rev_limit;",
        "    return e->launch;",
        LAUNCH, 300),
    'launch-gap-ignored': ("core/src/engine.c",
        "    const double resume = l ? d->launch_rpm - d->launch_gap : d->rev_limit_resume;",
        "    const double resume = l ? d->launch_rpm - 100.0 : d->rev_limit_resume;",
        LAUNCH, 300),
    'launch-param-lost': ("core/src/rb_drive.c",
        "    case RB_PARAM_LAUNCH:     e->launch = (int)v; break;\n",
        "",
        LAUNCH, 300),
    'fuel-cut-cuts-nothing': ("core/src/engine.c",
        "    else if (e->desc.limiter_cut == RB_LIMITER_FUEL && limiter_cut(e) >= 0)\n        kind = RB_INJ_LIMITER;   /* stage 133: the limiter cuts the fuel */\n",
        "",
        FUEL, 300),
}
