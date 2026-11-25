"""Stage 057: the accepted engine's baseline gates (tests/test_baseline.c) can fail.

python tools/plant_bugs.py 057 WORKDIR all
The fuel's mass and energy not booked; the engine's cells twice as long (more numerical loss);
the baffles not narrowing the way out; the rev limiter's soft cut off; the overrun's fuel cut
never resuming.
"""
WEAK = ["test_baseline_where_the_waves_are_weak"]
DRIVE = ["test_baseline_whole_drive_books"]
UPSTREAM = ["test_baseline_geometry_returns_upstream"]

BUGS = {
    'fuel-not-booked': ("core/src/engine.c",
        "        e->fuel_in += fuel;\n        e->fuel_energy_in += fuel * u_fuel;",
        "        e->fuel_in += 0.999 * fuel;\n        e->fuel_energy_in += fuel * u_fuel;",
        DRIVE, 400),
    'fuel-energy-not-booked': ("core/src/engine.c",
        "        e->fuel_energy_in += fuel * u_fuel;",
        "        e->fuel_energy_in += 0.9 * fuel * u_fuel;",
        DRIVE, 400),
    # Not 'first-order pipes': the momentum flux's reconstruction acts on rho u^2, which a small
    # wave in still gas never feels: the bench measured the same loss to 0.01 dB (not a fault).
    'cells-twice-as-long': ("core/src/engine_build.c",
        "#define LONG_CELL        0.075",
        "#define LONG_CELL        0.15",
        WEAK, 400),
    'baffles-open': ("core/src/engine_build.c",
        "                rb_pipe_restrict(t, 0, d->muffler_area);",
        "                rb_pipe_restrict(t, 0, 0.01 + 0.0 * d->muffler_area);",
        UPSTREAM, 400),
    # Not 'no hard cut': above the limit the soft cut already cuts every spark, so the engine
    # holds its limit the same without it (measured; not a fault). Without the soft cut:
    'no-soft-cut': ("core/src/engine_computer.c",
        "    if (!(band > 0.0)) return 0;",
        "    if (!(band > 1e9)) return 0;",
        DRIVE, 400),
    'fuel-cut-sticks': ("core/src/engine_computer.c",
        "    if (!foot_off || e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) {\n        e->fuel_cut = 0;",
        "    if (!foot_off || e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) {\n        e->fuel_cut = e->fuel_cut && foot_off;",
        DRIVE, 400),
}

# 2026-11-24, stage 138: the exhaust is built from its network (engine.h, exhaust_net.c), so
# these bugs moved with the code they break: the same faults, at their new text.
BUGS['baffles-open'] = ('core/src/exhaust_net.c',
    '            t->end[0].area = d->muffler_area;',
    '            t->end[0].area = 0.01 + 0.0 * d->muffler_area;',
    BUGS['baffles-open'][3], BUGS['baffles-open'][4])
