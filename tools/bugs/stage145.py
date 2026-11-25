"""Stage 145: the Polo 1.0 TSI preset and its turbine can fail their tests.

python tools/plant_bugs.py 145 WORKDIR all

The turbine's nozzle not narrowed (the turbine a plain pipe); the wastegate shut; the bore a
four-cylinder's (the displacement wrong); the crank's pins for 1-3-2 while it claims 1-2-3.
"""
TURBINE = ["test_net_polo_turbine"]
TABLE = ["test_presets_table", "test_presets_fire_where_the_layout_says"]

BUGS = {
    'turbine-not-narrowed': ("core/src/exhaust_models.c",
        "                                      .area = 3.5e-4 };   /* the turbine's nozzle: 3.5 cm^2 effective (est.) */",
        "                                      .area = 0.0 };",
        TURBINE, 600),
    'wastegate-shut': ("core/src/exhaust_models.c",
        "                                      .area = 2.0e-4 };   /* the wastegate open: 2 cm^2 effective (est.) */",
        "                                      .area = 0.2e-4 };",
        TURBINE, 600),
    'bore-a-fours': ("core/src/presets.c",
        "    d->bore = 74.5 * MM;",
        "    d->bore = 84.5 * MM;",
        TABLE, 300),
    'pins-for-1-3-2': ("core/src/presets.c",
        "    static const double pin[3] = { 0.0, 120.0, 240.0 };   /* 1-2-3: each 240 degrees after the last */",
        "    static const double pin[3] = { 0.0, 240.0, 120.0 };",
        TABLE, 300),
}
