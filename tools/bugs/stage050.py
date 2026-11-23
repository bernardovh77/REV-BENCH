"""Stage 050: the exhaust's components against plane-wave theory can fail their tests.

python tools/plant_bugs.py 050 WORKDIR all
The joined ends' end correction tripled (it moves the cans' nulls and domes), a laminar loss at
a joined end dropped (the catalyst and the baffles), the packing's loss halved, its settling
ignored for slow swings (all of the loss at every frequency), and a pipe joined into a can
seeing the can's area instead of its own.
"""
CAN = ["test_muffler_can_is_an_expansion_chamber", "test_muffler_centre_can_held_out"]
PACK = ["test_muffler_packing_is_its_stated_loss"]
CAT = ["test_muffler_catalyst_is_a_series_resistance"]
GRID = ["test_muffler_engine_grid_limits"]

BUGS = {
    'end-correction-tripled': ("core/src/pipe.c",
        "#define END_CORRECTION 0.3   /* diameters */",
        "#define END_CORRECTION 0.9   /* diameters */",
        CAN, 300),
    'laminar-dropped': ("core/src/pipe.c",
        "    p->end_face[end ? 1 : 0].laminar = (laminar > 0.0) ? laminar : 0.0;",
        "    p->end_face[end ? 1 : 0].laminar = 0.0;",
        CAT + GRID, 300),
    'packing-halved': ("core/src/pipe.c",
        "        f->packing = on ? resistance * f->length : 0.0;",
        "        f->packing = on ? 0.5 * resistance * f->length : 0.0;",
        PACK + GRID, 300),
    'packing-never-settles': ("core/src/pipe.c",
        "    f->settled += lag * (f->mdot - f->settled);",
        "    f->settled += 0.0 * lag * (f->mdot - f->settled);",
        PACK, 300),
    'join-at-the-cans-area': ("core/src/pipe.c",
        "    if (end) face_init(f, cell, chamber, p->area, length, p->friction_factor * 0.5 * p->dx / p->diameter);",
        "    if (end) face_init(f, cell, chamber, 4.0 * p->area, length, p->friction_factor * 0.5 * p->dx / p->diameter);",
        CAN + CAT, 300),
}
