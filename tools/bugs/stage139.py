"""Stage 139: the network's parts against four-pole theory, and the noise layer's passes from
their geometry, can fail their tests.

python tools/plant_bugs.py 139 WORKDIR all

A joined end without its end correction (the Helmholtz neck too short: it rings high); the end
correction doubled; an expansion chamber's pass not averaged over kL; a packed core passing all; Piening's law with the area for the perimeter.
"""
PARTS = ["test_parts_helmholtz_side_branch", "test_parts_quarter_wave_tube"]
PASSES = ["test_net_passes_from_geometry"]

BUGS = {
    'no-end-correction': ("core/src/pipe.c",
        "    const double length = 0.5 * p->dx + END_CORRECTION * p->diameter;",
        "    const double length = 0.5 * p->dx + 0.0 * END_CORRECTION * p->diameter;",
        PARTS, 600),
    'end-correction-doubled': ("core/src/pipe.c",
        "#define END_CORRECTION 0.3   /* diameters */",
        "#define END_CORRECTION 0.6   /* diameters */",
        PARTS, 600),
    'expansion-not-averaged': ("core/src/noise.c",
        "    return pass / sqrt(1.0 + 0.25 * a * a);",
        "    return pass / (1.0 + 0.25 * a * a);",
        PASSES, 300),
    'packed-passes-all': ("core/src/noise.c",
        "        return pass * pow(10.0, -0.1 * 1.05 * pow(PACKED_ABSORPTION, 1.4) * 4.0 / p->diameter * p->length);",
        "        return pass;",
        PASSES, 300),
    'piening-area-for-perimeter': ("core/src/noise.c",
        "        return pass * pow(10.0, -0.1 * 1.05 * pow(PACKED_ABSORPTION, 1.4) * 4.0 / p->diameter * p->length);",
        "        return pass * pow(10.0, -0.1 * 1.05 * pow(PACKED_ABSORPTION, 1.4) * 4.0 / (p->diameter * p->diameter) * p->length);",
        PASSES, 300),
}
