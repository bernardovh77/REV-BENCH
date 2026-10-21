"""Momentum reconstruction must retain real wave energy in either flow direction."""
TESTS = ["test_pipe_acoustic_transport", "test_pipe_conserves_everything"]
BUGS = {
    "donor-default": ("core/src/pipe.c", "p->momentum_order = 2;",
                       "p->momentum_order = 1;", TESTS, 45),
    "backward-slope": ("core/src/pipe.c",
        "((mdot >= 0.0) ? 0.5 : -0.5) * slope",
        "0.5 * slope", TESTS, 45),
    "excess-reconstruction": ("core/src/pipe.c",
        "((mdot >= 0.0) ? 0.5 : -0.5) * slope",
        "((mdot >= 0.0) ? 3.0 : -3.0) * slope", TESTS, 45),
}
