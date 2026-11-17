"""Stage 067: the host's engine built for reading (rb_host_engine_build) can fail its test.

python tools/plant_bugs.py 067h WORKDIR all
"""
TARGET = "rb_host_tests"

BUILD = ["test_host_engine_build"]

BUGS = {
    'engine-build-any-rate': ("host/src/sim_build.c",
        "    if (rate != 44100 && rate != 48000) {",
        "    if (rate < 0) {",
        BUILD, 300),
    'engine-build-leaks': ("host/src/sim_build.c",
        "void rb_host_engine_free(rb_sim *sim)\n{\n    rbh_sim_destroy(sim);\n}",
        "void rb_host_engine_free(rb_sim *sim)\n{\n    rb_destroy(sim);\n}",
        BUILD, 300),
    'engine-build-skips-edits': ("host/src/sim_build.c",
        "    for (int i = 0; i < spec->n_edits; i++) {",
        "    for (int i = 0; i < 0; i++) {",
        BUILD + ["test_swap"], 300),
}
