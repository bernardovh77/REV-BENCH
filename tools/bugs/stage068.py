"""Stage 068: the model's own warning and the dyno's telemetry can fail their tests.

python tools/plant_bugs.py 068 WORKDIR all      (a few at a time, under `timeout`)
The guard count never summed, its rate per cycle rather than per second, the faces' sonic cap
or mass cap uncounted (the choked headers' guard), and the dyno's telemetry giving the wrong
reading or none. The host's meter and trace observer are stage068h.py.
"""
GUARD = ["test_api_guard_rate_warns_only_outside_the_model"]
DYNO = ["test_api_dyno_in_the_telemetry"]

BUGS = {
    'guards-never-summed': ("core/src/engine_build.c",
        "    for (int k = 0; k < rb_engine_pipe_count(e); k++) n += pipe_guards(rb_engine_pipe(e, k));\n",
        "",
        GUARD, 600),
    'guard-rate-per-cycle': ("core/src/engine.c",
        "    r->guard_rate = e->acc_time > 0.0 ? (double)(guards - e->guards_then) / e->acc_time : 0.0;",
        "    r->guard_rate = (double)(guards - e->guards_then) * 1e-3;",
        GUARD, 600),
    'mass-cap-uncounted': ("core/src/pipe.c",
        "        reasons |= RB_D_BIT(RB_D_MASS_CAP);\n        f->guards++;",
        "        reasons |= RB_D_BIT(RB_D_MASS_CAP);",
        GUARD, 600),
    'dyno-torque-absorbed': ("core/src/rb_drive.c",
        "    case RB_TLM_DYNO_TORQUE:   return sim->dyno ? sim->dyno->reading.brake_torque : 0.0;",
        "    case RB_TLM_DYNO_TORQUE:   return sim->dyno ? sim->dyno->reading.absorbed_torque : 0.0;",
        DYNO, 300),
    'dyno-readings-missing': ("core/src/rb_drive.c",
        "    case RB_TLM_DYNO_READINGS: return sim->dyno ? (double)sim->dyno->readings : 0.0;",
        "    case RB_TLM_DYNO_READINGS: return 0.0;",
        DYNO, 300),
}
