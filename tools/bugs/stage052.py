"""Stage 052: the mixed-rate scheduler's tests can fail.

python tools/plant_bugs.py 052 WORKDIR all
The card's "inserted feedback sample": the tailpipe's flow held a pipe step too long (one more
step in the loop to the sound); the valves' fast cells seeing the slow cells' pressures of the
last slow step instead of as they are by now (stage 10's old coupling, 0.3 % apart); an extra
physics step at each block's start, so that the block size changes the sound.
"""
GRID = ["test_scheduler_pipe_rates_agree_on_one_grid"]
BLOCKS = ["test_scheduler_blocks_and_transitions_are_seamless"]
TWO_RATES = ["test_engine_pipes_two_rates"]

BUGS = {
    'tail-flow-a-step-late': ("core/src/engine.c",
        "        e->tail_flow[s] = (out->a->rho > 0.0) ? out->moved / out->a->rho : 0.0;",
        "        { static double held[RB_MAX_COLLECTORS]; const double now = (out->a->rho > 0.0) ? out->moved / out->a->rho : 0.0;\n"
        "          e->tail_flow[s] = held[s]; held[s] = now; }",
        GRID, 300),
    'stale-slow-pressures': ("core/src/pipe.c",
        "    const double dp = (f->direct_a ? rb_chamber_pressure_now(f->a, since) : f->a->p)\n"
        "                    - (f->direct_b ? rb_chamber_pressure_now(f->b, since) : f->b->p);",
        "    const double dp = f->a->p - f->b->p + 0.0 * since;",
        GRID + TWO_RATES, 300),
    'a-step-each-block': ("core/src/rb_sim.c",
        "    rb_sim_before_block(sim);\n",
        "    rb_sim_before_block(sim);\n    model_step(sim);\n",
        BLOCKS, 300),
}
