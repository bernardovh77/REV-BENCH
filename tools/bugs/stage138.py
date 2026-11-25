"""Stage 138: the exhaust as a network (engine.h, exhaust_net.c) can fail its tests.

python tools/plant_bugs.py 138 WORKDIR all

A side branch joined at the tapped pipe's first cell, not where it says; a network with two pipes
into one outlet let through; the ways through side branches ignored (an H-pipe's banks never
meeting); the description's network ignored (the legacy fields' built instead); a network's
noise routed without its parts' passes; a catalyst's face heard as a baffle jet; the faces'
restrictions left out; the listener hearing only a network's first outlet; the stereo scene
hearing only one; the pool shared out before the pipes are counted (a crash once there are
more cells wanted than it holds); the API's source 2 a third tailpipe, not the intake's mouth.
"""
NET = ["test_net_new_parts_run_and_keep_the_books"]
CHECK = ["test_net_check_refuses_bad_networks"]
NOISE = ["test_net_noise_routes_as_the_legacy_exhaust"]
LISTEN = ["test_net_listener_hears_every_outlet"]
DESCRIPTION = ["test_engine_description"]
API = ["test_api_settings"]

BUGS = {
    'tap-first-cell': ("core/src/engine_build.c",
        "        rb_pipe_join(p, end, &m->cell[(int)lround(x->at * (m->cells - 1))], x->entry_loss, 0);",
        "        rb_pipe_join(p, end, &m->cell[0 * (int)lround(x->at * (m->cells - 1))], x->entry_loss, 0);",
        NET, 300),
    'two-pipes-one-outlet': ("core/src/exhaust_net.c",
        "        if (users != 1) return -1;",
        "        if (users < 1) return -1;",
        CHECK, 300),
    'paths-skip-taps': ("core/src/exhaust_net.c",
        "            if (x->kind == RB_NET_TAP) {",
        "            if (x->kind == RB_NET_TAP) { continue;",
        NET, 300),
    'description-net-ignored': ("core/src/engine_build.c",
        "    if (d->net.pipes > 0) e->net = d->net;",
        "    if (0 * d->net.pipes > 0) e->net = d->net;",
        NET, 300),
    'route-without-pass': ("core/src/noise.c",
        "            n->blow_share[i][s] = sqrt(pass / reached);",
        "            n->blow_share[i][s] = sqrt((0.0 * pass + 1.0) / reached);",
        NOISE, 300),
    'catalyst-heard-as-baffle': ("core/src/noise.c",
        "            if (!(x->area > 0.0) || (end == 1 && net->pipe[k].catalyst > 0.0) || n->holes >= RB_NOISE_HOLES)",
        "            if (!(x->area > 0.0) || n->holes >= RB_NOISE_HOLES)",
        NOISE, 300),
    'restrictions-left-out': ("core/src/engine_build.c",
        "            rb_pipe_restrict(p, end, x->area);",
        "            rb_pipe_restrict(p, end, 0.0 * x->area);",
        ["test_net_as_description_builds_the_same_engine"], 300),
    'listener-first-outlet-only': ("core/src/audio.c",
        "    for (int o = 0; o < outlets; o++) pos[o] = &at[3 * o];",
        "    for (int o = 0; o < 1; o++) pos[o] = &at[3 * o];",
        LISTEN, 300),
    'scene-one-outlet': ("core/src/scene.c",
        "    if (j < RB_SCENE_OUTLETS && j >= s->tailpipes)",
        "    if (j < RB_SCENE_OUTLETS && j >= 1)",
        LISTEN, 300),
    'pool-before-pipes-counted': ("core/src/engine_build.c",
        "    const int pipes = 1 + d->cylinders * (1 + runners) + net->pipes;",
        "    const int pipes = rb_engine_pipe_count(e);",
        DESCRIPTION, 300),
    'source-two-a-tailpipe': ("core/src/rb_drive.c",
        "    return rb_listener_distance(&sim->listener, LISTENER_SOURCE[source]);",
        "    return rb_listener_distance(&sim->listener, source);",
        API, 300),
}
