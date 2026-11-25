"""End-to-end acoustic tests must catch decorative or unstable scene effects."""
BUGS = {
    "no-moving-source": ("core/src/scene.c",
        "s->kind == RB_SCENE_PASSBY ? 15.0 : 0.0", "0.0",
        ["test_scene_doppler_tone"], 30),
    "coincident-ears": ("core/src/scene.c",
        "at[0] += ear ? 0.09 : -0.09;", "at[0] += 0.0;",
        ["test_scene_api_repeatability", "test_scene_ear_geometry"], 30),
    "amplifying-room": ("core/src/scene.c",
        "-3.0 * s->room_length[i]", "3.0 * s->room_length[i]",
        ["test_scene_propagation_and_room"], 30),
    "point-aperture": ("core/src/scene.c",
        "*spread = j < 2 ?", "*spread = 0 ?",
        ["test_scene_aperture_radiation"], 30),
}

# 2026-11-24, stage 138: the exhaust is built from its network (engine.h, exhaust_net.c), so
# these bugs moved with the code they break: the same faults, at their new text.
BUGS['point-aperture'] = ('core/src/scene.c',
    '*spread = j < RB_SCENE_OUTLETS ?',
    '*spread = 0 ?',
    BUGS['point-aperture'][3], BUGS['point-aperture'][4])
