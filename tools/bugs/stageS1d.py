"""The trapped mixture must drive the life-enabled burn, after injection."""
MIXTURE = ["test_variation_trapped_mixture"]
CAPTURE = ["test_variation_trapped_mixture_capture"]
BUGS = {
    "requested-mixture": ("core/src/engine.c",
        "? RB_AFR_STOICH * (g->m[RB_FUEL] / g->m[RB_AIR]) : 0.0;",
        "? RB_AFR_STOICH / e->afr : 0.0;", MIXTURE, 30),
    "inverse-mixture": ("core/src/engine.c",
        "RB_AFR_STOICH * (g->m[RB_FUEL] / g->m[RB_AIR])",
        "(g->m[RB_AIR] / g->m[RB_FUEL]) / RB_AFR_STOICH", MIXTURE + CAPTURE, 30),
    # 'life-off-changed' retired at stage 120: the trapped mixture now feeds the burn with
    # variation off too (Codex's charge-burn model, by design), so there is no variation-gated
    # path left to break.
    "trim-ignored": ("core/src/engine.c",
        "b->phi_ivc = isfinite(phi) ? phi : 0.0;",
        "b->phi_ivc = (e->afr > 0.0) ? RB_AFR_STOICH / e->afr : 0.0;", CAPTURE, 30),
}
