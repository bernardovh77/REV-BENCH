"""Stage 131: the host's builds of a layout can fail their tests.

python tools/plant_bugs.py 131h WORKDIR all
A build's layout ignored (the preset's own engine built), a refused layout leaking its engine,
and the trace's layout line not taken.
"""
TARGET = "rb_host_tests"

BUILD = ["test_host_engine_build_takes_a_layout"]

BUGS = {
    'layout-ignored': ("host/src/sim_build.c",
        "    if (spec->layout_family >= 0) {\n        rc = rb_load_layout(",
        "    if (spec->layout_family >= 99) {\n        rc = rb_load_layout(",
        BUILD, 300),
    'refused-layout-leaks': ("host/src/sim_build.c",
        "            if (why && why_size) snprintf(why, why_size, \"the layout: %s\", reason ? reason : \"its engine was refused\");\n            rbh_sim_destroy(sim);\n",
        "            if (why && why_size) snprintf(why, why_size, \"the layout: %s\", reason ? reason : \"its engine was refused\");\n",
        BUILD, 300),
    'trace-layout-refusal-skipped': ("host/src/trace.c",
        "            || rb_layout_check((int)f, (int)c, (int)k, v) != RB_OK) {",
        "            ) {",
        ["test_trace_errors_name_the_line"], 300),
}
