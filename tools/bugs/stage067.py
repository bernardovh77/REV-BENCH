"""Stage 067: the editor's core changes can fail their tests.

python tools/plant_bugs.py 067 WORKDIR all      (a few at a time, under `timeout`)
The two engine-computer settings (an air-fuel ratio that never reaches the injectors, a spark
offset ignored), the listener's restart on a live change, and the every-field test catching a
field that silently stops working (the catalyst's resistance never applied) and a stated
no-effect field that starts having one. The host's engine-for-reading is stage067h.py (its own
test program).
"""
EDITOR = ["test_editor_every_field_has_an_effect"]
SETTINGS = ["test_api_afr_and_spark_offset"]

BUGS = {
    'afr-not-applied': ("core/src/rb_drive.c",
        "    case RB_PARAM_AFR:          e->afr = v; break;\n",
        "",
        SETTINGS, 300),
    'spark-offset-ignored': ("core/src/engine_computer.c",
        "    return e->spark_offset != 0.0 ? mapped + e->spark_offset : mapped;",
        "    return mapped;",
        SETTINGS, 300),
    'spark-offset-backwards': ("core/src/engine_computer.c",
        "    return e->spark_offset != 0.0 ? mapped + e->spark_offset : mapped;",
        "    return e->spark_offset != 0.0 ? mapped - e->spark_offset : mapped;",
        SETTINGS, 300),
    'listen-not-primed': ("core/src/rb_sim.c",
        "    rb_listener_prime(&sim->listener, q);\n",
        "",
        ["test_api_listen_changes_restart_the_histories"], 300),
    'catalyst-never-resists': ("core/src/engine_build.c",
        "        rb_pipe_resist(x, 1, d->catalyst_laminar);\n",
        "",
        EDITOR, 900),
    'position-moves-the-sound': ("core/src/engine_build.c",
        "        rb_pipe_resist(x, 1, d->catalyst_laminar);\n",
        "        rb_pipe_resist(x, 1, d->catalyst_laminar + 1e5 * d->position[1]);\n",
        EDITOR, 900),
}

# 2026-11-24, stage 138: the exhaust is built from its network (engine.h, exhaust_net.c), so
# these bugs moved with the code they break: the same faults, at their new text.
BUGS['catalyst-never-resists'] = ('core/src/exhaust_net.c',
    '        x->end[1].laminar = d->catalyst_laminar;\n',
    '',
    BUGS['catalyst-never-resists'][3], BUGS['catalyst-never-resists'][4])
BUGS['position-moves-the-sound'] = ('core/src/exhaust_net.c',
    '        x->end[1].laminar = d->catalyst_laminar;\n',
    '        x->end[1].laminar = d->catalyst_laminar + 1e5 * d->position[1];\n',
    BUGS['position-moves-the-sound'][3], BUGS['position-moves-the-sound'][4])
