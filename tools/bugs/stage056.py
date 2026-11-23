"""Stage 056: the stereo scene's listener can fail its analytic controls.

python tools/plant_bugs.py 056 WORKDIR all
Reading between two samples again (the upper-band loss); a history that wraps; the source's place
not retarded; the Doppler from the listener's delay alone; the head's shadow too strong; the
cabin's boom twice as sharp; grass as hard as asphalt; the ears swapped.
"""
BAND = ["test_listener_fractional_delay_band"]
DOPPLER = ["test_listener_doppler_analytic"]
HISTORY = ["test_listener_bounded_history"]
STEREO = ["test_listener_stereo_known_position"]
CABIN = ["test_listener_cabin_transfer"]
SCENES = ["test_listener_eight_scenes"]

BUGS = {
    'linear-read': ("core/src/scene.c",
        "    if (whole < 1)\n        return (1 - p) * s->source[j][a] + p * s->source[j][b];",
        "    if (whole < 1000000)\n        return (1 - p) * s->source[j][a] + p * s->source[j][b];",
        BAND, 300),
    'history-wraps': ("core/src/scene.c",
        "    if (!(delay >= 0 && delay < RB_SCENE_RING - 3))",
        "    if (!(delay >= 0))",
        HISTORY, 300),
    'source-not-retarded': ("core/src/scene.c",
        "    r[1] -= velocity * tau;",
        "    r[1] -= 0.0 * velocity * tau;",
        DOPPLER, 300),
    'doppler-from-the-listener': ("core/src/scene.c",
        "    const double tau = rb_scene_flight_time(r, velocity, s->c);",
        "    const double tau = rb_scene_flight_time(r, 0.0 * velocity, s->c);",
        DOPPLER, 300),
    'shadow-too-strong': ("core/src/scene.c",
        "                v = p->gain * ((1.0 - 0.7 * p->shadow) * v + 0.7 * p->shadow * p->low);",
        "                v = p->gain * ((1.0 - 0.9 * p->shadow) * v + 0.9 * p->shadow * p->low);",
        STEREO, 300),
    'cabin-boom-sharper': ("core/src/audio.c",
        "    const double alpha = sin(w0) / (2.0 * RB_CABIN_BOOM_Q);",
        "    const double alpha = sin(w0) / (4.0 * RB_CABIN_BOOM_Q);",
        CABIN, 300),
    'grass-as-hard': ("core/src/scene.c",
        "(s->kind == RB_SCENE_GRASS ? 0.35 : 0.9)",
        "(s->kind == RB_SCENE_GRASS ? 0.9 : 0.9)",
        SCENES, 300),
    'ears-swapped': ("core/src/scene.c",
        "    at[0] += ear ? 0.09 : -0.09;",
        "    at[0] += ear ? -0.09 : 0.09;",
        STEREO, 300),
}
