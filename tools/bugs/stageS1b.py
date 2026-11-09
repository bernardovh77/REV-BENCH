"""S1b's finite-time partial burns: each mutation must fail a physics test."""
PARTIAL = ["test_variation_partial_burns"]
RESIDUAL = ["test_variation_partial_burn_residual"]

BUGS = {
    "demand-still-capped": ("core/src/engine.c",
        "    const double base = capped ? expected : b->demand_duration;",
        "    const double base = expected;", PARTIAL, 120),
    # 'life-off-uncapped' retired at stage 120: the cap is now Codex's RB_BURN_CAPPED model,
    # chosen apart from the variation, and no test pins that model's burn length to stage 9's
    # (docs/stage-120.md records the gap).
    "dilution-ignored": ("core/src/combustion.c",
        "    double dilution = 1.0 - 2.1 * x;",
        "    double dilution = 1.0;", RESIDUAL, 120),
    "burn-past-evo": ("core/src/engine.c",
        "b->burning = 0;        /* whatever hasn't burned by now leaves unburned */",
        "/* BUG: burn continues through exhaust valve opening */", PARTIAL, 120),
    "partial-fuel-discarded": ("core/src/engine.c",
        "b->burning = 0;        /* whatever hasn't burned by now leaves unburned */",
        "b->burning = 0; e->cyl[i].gas.m[RB_FUEL] = 0.0;",
        ["test_variation_partial_burn_books"], 120),
}
