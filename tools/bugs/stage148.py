"""Stage 148: the studio quality (the pipes at 192 kHz, cells a quarter of high's) can fail its tests.

python tools/plant_bugs.py 148 WORKDIR all

Studio's pipes at ultra's rate; the cell pool, the cells a pipe can have or the pipes' rate
left at their old limits; the grid not refined past half of high's cells. Candidate 7 (the
valves' noise above a finer grid's band only) heard on high's grid too, with one high-pass, or
never.
"""
STUDIO = ["test_core_quality", "test_studio_cells_a_quarter", "test_studio_plays_the_largest_engines",
          "test_studio_carries_the_gas_higher"]
REBALANCE = ["test_studio_rebalance_only_where_the_gas_carries"]

BUGS = {
    'studio-at-ultras-rate': ("core/src/rb_sim.c",
        "    cfg->pipe_rate = quality == RB_QUALITY_STUDIO ? 4.0 * RB_PIPE_RATE",
        "    cfg->pipe_rate = quality == RB_QUALITY_STUDIO ? 2.0 * RB_PIPE_RATE",
        STUDIO, 600),
    'pool-not-raised': ("core/src/engine.h",
        "#define RB_ENGINE_CELLS  2048",
        "#define RB_ENGINE_CELLS  512",
        STUDIO, 600),
    'pipe-cells-not-raised': ("core/src/pipe.h",
        "#define RB_PIPE_MAX_CELLS 256",
        "#define RB_PIPE_MAX_CELLS 64",
        STUDIO, 600),
    'pipe-rate-limit-not-raised': ("core/src/engine.h",
        "#define RB_PIPE_RATE_MAX 192000.0   /* the studio quality's (stage 148) */",
        "#define RB_PIPE_RATE_MAX 96000.0",
        STUDIO, 600),
    'grid-refined-only-to-half': ("core/src/engine_build.c",
        "    const double grid_scale = fmin(1.0, RB_PIPE_RATE * e->pipe_dt);",
        "    const double grid_scale = fmax(0.5, fmin(1.0, RB_PIPE_RATE * e->pipe_dt));",
        STUDIO, 600),
    'rebalance-on-highs-grid-too': ("core/src/noise.c",
        "    if (!(rate > RATE[0] * 1.001)) return 0.0;",
        "    if (!(rate > RATE[0] * 1.001)) return EDGE[0];",
        REBALANCE, 600),
    'rebalance-one-high-pass': ("core/src/noise.c",
        "            for (int k = 0; k < 2; k++) {\n                n->grid_lp[s][k] += n->grid_hp * (blow - n->grid_lp[s][k]);",
        "            for (int k = 0; k < 1; k++) {\n                n->grid_lp[s][k] += n->grid_hp * (blow - n->grid_lp[s][k]);",
        REBALANCE, 600),
    'rebalance-never-heard': ("core/src/rb_sim.c",
        "        sim->noise.rebalance = sim->variant == 7;   /* 148's */",
        "        sim->noise.rebalance = sim->variant == 8;   /* 148's */",
        REBALANCE, 600),
}
