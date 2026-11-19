"""Stage 068: the host's output meter and the trace's observer can fail their tests.

python tools/plant_bugs.py 068h WORKDIR all
The meter never starting again (silence still reads the tone), its RMS of the magnitudes'
sum, the read's info without the meter, and the observer never called.
"""
TARGET = "rb_host_tests"

METER = ["test_host_snapshots_carry_the_output_meter"]

BUGS = {
    'meter-never-reset': ("host/src/runner.c",
        "    r->meter_peak = 0.0f;\n    r->meter_sumsq = 0.0;\n    r->meter_samples = 0;\n",
        "",
        METER, 300),
    'meter-rms-of-magnitudes': ("host/src/runner.c",
        "        r->meter_sumsq += (double)v * v;",
        "        r->meter_sumsq += (double)a;",
        METER, 300),
    'meter-not-read': ("host/src/host.c",
        "        info->out_peak = s->out_peak;\n",
        "",
        METER, 300),
    'observer-never-called': ("host/src/trace.c",
        "        if (observer) observer(user, h->run.sim, frame + n);\n",
        "",
        ["test_trace_observer_sees_the_engine_after_every_block"], 300),
}
