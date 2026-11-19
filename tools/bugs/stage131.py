"""Stage 131: the layouts' tests can fail.

python tools/plant_bugs.py 131 WORKDIR all      (a few at a time, under `timeout`)
The card's bugs: a wrong firing interval, a pin off, a bank swapped, the idle gains not scaled,
a count silently clamped; and a count not refused, the cross-plane crank fired as another, an
exhaust split on a V that shouldn't be. The drive of every combination (test_layout_every_
combination_runs) needs its share named: the idle valve not scaled is checked with
RB_LAYOUT_SHARD=23/28 in the environment (the even V16 alone; without it the test skips and the
bug is not caught). The host's layout build is stage131h.py.
"""
FIRES = ["test_layout_fires_as_stated"]
OFFERED = ["test_layout_offers_the_stated_set"]
PRESETS = ["test_layout_presets_are_their_own_layouts"]
KEEPS = ["test_layout_keeps_the_presets_part"]
RUNS = ["test_layout_every_combination_runs"]

BUGS = {
    'firing-interval-wrong': ("core/src/layouts.c",
        "            target[2 * r + 1] = turn720(j * 2.0 * step - step);",
        "            target[2 * r + 1] = turn720(j * 2.0 * step - 0.9 * step);",
        FIRES, 300),
    'pin-off': ("core/src/layouts.c",
        "        d->pin[i] = turn360(d->bank_angle[d->bank[i]] - base - target[i]);",
        "        d->pin[i] = turn360(d->bank_angle[d->bank[i]] - base - target[i] + (i == 2 ? 1.0 : 0.0));",
        FIRES + PRESETS, 300),
    'bank-swapped': ("core/src/layouts.c",
        "    for (int i = 0; i < n; i++) x.bank[i] = (x.banks == 2) ? i % 2 : 0;",
        "    for (int i = 0; i < n; i++) x.bank[i] = (x.banks == 2) ? (i + 1) % 2 : 0;",
        PRESETS, 300),
    'idle-gains-not-scaled': ("core/src/layouts.c",
        "    d->idle_gain_p = loop * d->inertia / d->idle_valve_area;",
        "    d->idle_gain_p = 2.4e-4;",
        KEEPS, 300),
    'count-clamped': ("core/src/layouts.c",
        "int rb_layout_check(int family, int cylinders, int crank, double bank_angle)\n{\n    const rb_layout l = { family, cylinders, crank, bank_angle };",
        "int rb_layout_check(int family, int cylinders, int crank, double bank_angle)\n{\n    const rb_layout l = { family, cylinders > 16 ? 16 : cylinders, crank, bank_angle };",
        OFFERED, 300),
    'inline-eight-not-refused': ("core/src/layouts.c",
        "        if (n == 7 || n == 8) return \"an inline engine of 7 or 8 cylinders isn't offered yet (an 8 is planned)\";\n",
        "",
        OFFERED, 300),
    'cross-plane-as-flat': ("core/src/layouts.c",
        "static const double CROSS_PIN[8] = { 0.0, 0.0, 90.0, 90.0, 270.0, 270.0, 180.0, 180.0 };",
        "static const double CROSS_PIN[8] = { 0.0, 0.0, 180.0, 180.0, 180.0, 180.0, 0.0, 0.0 };",
        PRESETS, 300),
    'v-one-exhaust': ("core/src/layouts.c",
        "    if (l->family == RB_FAMILY_V && n >= 4) collectors = 2;",
        "    if (l->family == RB_FAMILY_V && n >= 40) collectors = 2;",
        KEEPS, 300),
    'exhaust-ratio-unchecked': ("core/src/engine.c",
        "                           <= 6.0 * joined[c] * d->header_diameter * d->header_diameter)) return -1;",
        "                           <= 60.0 * joined[c] * d->header_diameter * d->header_diameter)) return -1;",
        ["test_layout_refuses_an_exhaust_far_wider_than_its_headers"], 300),
    'idle-valve-not-scaled': ("core/src/layouts.c",
        "    d->idle_valve_area = 47e-6 * pow(litres / 2.0, 0.8);",
        "    d->idle_valve_area = 47e-6;",
        RUNS, 1800),
}
