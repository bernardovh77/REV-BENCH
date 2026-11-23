"""Stage 069: a stopped engine's sleep (RB_PARAM_REST) can fail its tests.

python tools/plant_bugs.py 069 WORKDIR all      (a few at a time, under `timeout`)
The rest on by default (offline renders change), sleep with no quiet stretch, a loud block
counted as quiet, a turning crank put to sleep, and a parameter, a field or a rebuild that
doesn't wake it, and a rest level too low to sleep before a fresh engine sings.
"""
SLEEPS = ["test_api_rest_sleeps_a_still_engine_only"]
WAKES = ["test_api_rest_wakes_on_anything_set"]
RUNNING = ["test_api_rest_never_touches_a_running_engine"]
SINGING = ["test_api_rest_keeps_a_fresh_engine_from_singing"]

BUGS = {
    'rest-on-by-default': ("core/src/rb_params.c",
        '    [RB_PARAM_REST]            = { "rest",            "",   0.0,   1.0,     0.0,    LIVE | INT },',
        '    [RB_PARAM_REST]            = { "rest",            "",   0.0,   1.0,     1.0,    LIVE | INT },',
        SLEEPS, 300),
    'sleeps-at-once': ("core/src/rb_sim.c",
        "    if ((double)sim->rest_quiet >= RB_REST_HOLD_S * sim->cfg.sample_rate) sim->resting = 1;",
        "    sim->resting = 1;",
        SLEEPS, 300),
    'loud-counts-as-quiet': ("core/src/rb_sim.c",
        "    if (!still || !(sim->out_peak < RB_REST_LEVEL) || !same) {",
        "    if (!still || !same) {",
        SLEEPS, 300),
    'turning-crank-sleeps': ("core/src/rb_sim.c",
        "                   && e->omega == 0.0 && sim->param[RB_PARAM_STARTER] == 0.0;",
        "                   && sim->param[RB_PARAM_STARTER] == 0.0;",
        RUNNING, 300),
    'param-doesnt-wake': ("core/src/rb_params.c",
        "    if (sim->param[id] != value) sim->touched++;   /* wakes a resting engine (stage 069) */\n",
        "",
        WAKES, 300),
    'field-doesnt-wake': ("core/src/rb_engine_api.c",
        "    sim->touched++;   /* wakes a resting engine (stage 069) */\n",
        "",
        WAKES, 300),
    'rest-level-too-low': ("core/include/rb.h",
        "#define RB_REST_LEVEL  1e-5",
        "#define RB_REST_LEVEL  1e-6",
        SINGING, 300),
    'rebuild-doesnt-wake': ("core/src/rb_sim.c",
        "    if (sim->touched == sim->rest_touched && sim->generation == sim->rest_generation) return 1;",
        "    if (sim->touched == sim->rest_touched) return 1;",
        WAKES, 300),
}
