"""S1b's finite-time partial burns: each mutation must fail a physics test."""
PARTIAL = ["test_variation_partial_burns"]
RESIDUAL = ["test_variation_partial_burn_residual"]

BUGS = {
    "demand-still-capped": ("core/src/engine.c",
        "const double base = (v > 0.0) ? b->demand_duration : expected;",
        "const double base = expected;", PARTIAL, 120),
    "life-off-uncapped": ("core/src/engine.c",
        "const double base = (v > 0.0) ? b->demand_duration : expected;",
        "const double base = b->demand_duration;", PARTIAL, 120),
    "dilution-ignored": ("core/src/engine.c",
        "double dilution = 1.0 - 2.1 * x;",
        "double dilution = 1.0;", RESIDUAL, 120),
    "burn-past-evo": ("core/src/engine.c",
        "b->burning = 0;        /* whatever hasn't burned by now leaves unburned */",
        "/* BUG: burn continues through exhaust valve opening */", PARTIAL, 120),
    "partial-fuel-discarded": ("core/src/engine.c",
        "b->burning = 0;        /* whatever hasn't burned by now leaves unburned */",
        "b->burning = 0; e->cyl[i].gas.m[RB_FUEL] = 0.0;",
        ["test_variation_partial_burn_books"], 120),
}
