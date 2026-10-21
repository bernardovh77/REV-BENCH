"""Overrun must affect combustion, wait for entry, and reset on interruption."""
TESTS = ["test_overrun_transition", "test_overrun_reset_and_legacy",
         "test_overrun_retards_combustion"]
BUGS = {
    "instant-cut": ("core/src/engine.c",
        "e->fuel_cut = e->overrun_time >= transition;", "e->fuel_cut = 1;", TESTS, 30),
    "no-spark-transition": ("core/src/engine.c",
        "return best - e->overrun_retard * phase;", "return best;", TESTS, 30),
    "no-key-reset": ("core/src/engine.c",
        "e->overrun_time = 0.0;\n        e->overrun_armed = 0;\n        return;",
        "e->overrun_armed = 0;\n        return;", TESTS, 30),
}
