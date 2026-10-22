"""High-rev limits, valid bypass descriptors, and real X/network energy behavior."""
BUGS = {
    "old-i6-limit": ("core/src/presets.c", "4.4e-4, 3.2e-4, 8500.0);",
        "4.4e-4, 3.2e-4, 7000.0);", ["test_exhaust_high_rpm_fired_books"], 30),
    "old-cross-limit": ("core/src/presets.c", "5.1e-4, 2.6e-4, 7500.0);",
        "5.1e-4, 2.6e-4, 7000.0);", ["test_exhaust_high_rpm_fired_books"], 30),
    "unvalidated-catalyst-loss": ("core/src/engine.c",
        "if (!(d->catalyst_laminar >= 0.0 && d->catalyst_laminar <= 10000.0)) return -1;",
        "/* BUG: accept negative and non-finite substrate loss */",
        ["test_exhaust_descriptor_guards"], 30),
    "recycled-radiation-work": ("core/src/pipe.c", "rb_chamber_add_heat(cell, -power);",
        "/* BUG: extracted exterior work heats the gas again */",
        ["test_exhaust_high_rpm_fired_books"], 30),
    "reversed-junction-flow": ("core/src/pipe.c",
        "else     face_init(f, chamber, cell, p->area, length, p->friction_factor * 0.5 * p->dx / p->diameter);",
        "else     face_init(f, cell, chamber, p->area, length, p->friction_factor * 0.5 * p->dx / p->diameter);",
        ["test_exhaust_coherent_x_modes"], 30),
}
