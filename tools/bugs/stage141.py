"""Stage 141: the 458 as a preset can fail its tests.

python tools/plant_bugs.py 141 WORKDIR all

The network's pipe struct with padding again (an int after its doubles): a description's bytes
then depend on the compiler, and MSVC failed the byte-for-byte comparisons. Its expected outcome is
"BUILD FAILED": engine.h's static assertions ("rb_net_pipe has padding") refuse to compile it.
"""
PRESETS = ["test_presets_table"]

BUGS = {
    'net-pipe-padding': ("core/src/engine.h",
        "    double wall;                /* K: its wall's temperature (0: its role's)  */\n    rb_net_end end[2];",
        "    double wall;                /* K: its wall's temperature (0: its role's)  */\n    int    padded;\n    rb_net_end end[2];",
        PRESETS, 300),
}
