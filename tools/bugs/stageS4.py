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
