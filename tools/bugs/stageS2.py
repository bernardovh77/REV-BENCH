"""The extra flow sources must exist, and ultra must refine actual cells."""
BUGS = {
    "no-intake-roar": ("core/src/noise.c",
        "n->intake_power += inner_jet", "n->intake_power += 0.0 * inner_jet",
        ["test_noise_intake_and_baffles"], 30),
    "no-baffle-flow": ("core/src/noise.c",
        "hole.mdot = f->moved;", "hole.mdot = 0.0;",
        ["test_noise_intake_and_baffles"], 30),
    "no-grid-refinement": ("core/src/engine.c",
        "fmin(1.0, RB_PIPE_RATE * e->pipe_dt)", "1.0",
        ["test_ultra_spatial_resolution"], 30),
}
