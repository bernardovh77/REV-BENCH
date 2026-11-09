"""Stage 121: the public engine API's tests can fail.

python tools/plant_bugs.py 121 WORKDIR all
Each bug breaks one thing a host relies on: a control, the car, a setting,
the rebuild recipe, the dyno, a snapshot or an event.
"""
KEY = ["test_api_key_starts_the_engine"]
DRIVE = ["test_api_throttle_and_load"]
HOLD = ["test_api_hold"]
CAR = ["test_api_car_in_gear"]
SETTINGS = ["test_api_settings"]
DYNO = ["test_api_dyno"]
SNAP = ["test_api_snapshot", "test_api_snapshot_layout"]
EVENTS = ["test_api_events"]
META = ["test_api_controls_metadata"]

BUGS = {
    # the controls
    'ignition-ignored': ("core/src/rb_drive.c",
        "    case RB_PARAM_IGNITION:   e->ignition = (int)v; break;",
        "    case RB_PARAM_IGNITION:   break;",
        KEY, 300),
    'throttle-ignored': ("core/src/rb_drive.c",
        "    case RB_PARAM_THROTTLE:   e->throttle_pos = v; break;",
        "    case RB_PARAM_THROTTLE:   break;",
        DRIVE + HOLD, 300),
    'load-ignored': ("core/src/rb_drive.c",
        "        e->load_torque = sim->param[RB_PARAM_LOAD_TORQUE];\n        return;",
        "        e->load_torque = 0.0;\n        return;",
        DRIVE, 300),
    'held-at-the-old-speed': ("core/src/rb_drive.c",
        "        if (e->hold_speed) e->rpm = v;",
        "        (void)v;",
        HOLD, 300),
    'switches-take-halves': ("core/src/rb_params.c",
        "    if ((PARAMS[id].flags & RB_PF_INTEGER) && value != floor(value)) return RB_ERR_VALUE;\n",
        "",
        META, 120),
    # the car
    'no-car-on-the-crank': ("core/src/rb_drive.c",
        "    if (e->desc.inertia == inertia) return;",
        "    if (1) return;",
        CAR, 300),
    'clutch-loses-the-speed': ("core/src/rb_drive.c",
        "    e->kinetic = 0.5 * inertia * e->omega * e->omega;\n}",
        "}",
        CAR, 300),
    'no-road': ("core/src/rb_drive.c",
        "    if (!(j_car > 0.0)) return 0.0;",
        "    if (1) return 0.0;",
        CAR, 300),
    # the rebuild recipe and the settings
    'controls-survive-a-rebuild': ("core/src/rb_drive.c",
        "        if (rb_param_flags(id) & RB_PF_CONTROL) sim->param[id] = rb_param_default(id);",
        "        (void)id;",
        SETTINGS, 300),
    'settings-lost-in-a-rebuild': ("core/src/rb_drive.c",
        "    sim->dyno = NULL;\n    sim->generation++;\n",
        "    sim->dyno = NULL;\n    sim->generation++;\n    sim->param[RB_PARAM_SEED] = 0;\n",
        SETTINGS, 300),
    'seed-changed-mid-run': ("core/src/rb_drive.c",
        "        if (has_run(sim)) return RB_PENDING;",
        "        if (0) return RB_PENDING;",
        SETTINGS, 300),
    'generation-not-counted': ("core/src/rb_drive.c",
        "    sim->generation++;\n",
        "",
        SETTINGS + SNAP, 300),
    'quality-forgets-the-engine': ("core/src/rb_sim.c",
        "    sim->preset = preset;\n    sim->generation = generation;",
        "    sim->preset = 0;\n    sim->generation = generation;",
        SETTINGS, 300),
    # the dyno
    # both ways a rebuild takes it off: directly, and when the reset hold
    # control holds the crank by the motor again
    'dyno-outlives-its-engine': ("core/src/rb_drive.c",
        ("    sim->own_inertia = d->inertia;\n    sim->dyno = NULL;",
         "        if (v == 1.0 && sim->dyno) sim->dyno = NULL;"),
        ("    sim->own_inertia = d->inertia;",
         "        if (0) sim->dyno = NULL;"),
        DYNO, 300),
    'dyno-never-clears': ("core/src/rb_drive.c",
        "    sim->dyno->readings = 0;\n",
        "",
        DYNO, 300),
    # snapshots
    'snapshot-cylinders-swapped': ("core/src/rb_drive.c",
        "        RB_CYL_PRESSURE, RB_CYL_TEMPERATURE, RB_CYL_BURN,",
        "        RB_CYL_TEMPERATURE, RB_CYL_PRESSURE, RB_CYL_BURN,",
        SNAP, 300),
    'snapshot-overruns-the-buffer': ("core/src/rb_drive.c",
        "    if (size < need) return RB_ERR_SIZE;",
        "    if (size < 0) return RB_ERR_SIZE;",
        SNAP, 300),
    'snapshot-not-numbered': ("core/src/rb_drive.c",
        "        .sequence = ++sim->snapshot_sequence,",
        "        .sequence = sim->snapshot_sequence,",
        SNAP, 300),
    'build-id-of-the-bore-alone': ("core/src/rb_drive.c",
        "    for (int f = 0; f < RB_ENG_COUNT; f++)\n        for (int i = 0; i < rb_engine_field_size(sim, f); i++) {",
        "    for (int f = 0; f < 1; f++)\n        for (int i = 0; i < rb_engine_field_size(sim, f); i++) {",
        SNAP, 300),
    'snapshot-instruments-shifted': ("core/src/rb_drive.c",
        "        const double v = rb_sim_engine_telemetry(sim, RB_TLM_RPM + k);",
        "        const double v = rb_sim_engine_telemetry(sim, RB_TLM_RPM + (k + 1) % RB_SNAPSHOT_INSTRUMENTS);",
        SNAP, 300),
    # events
    'events-out-of-order': ("core/src/rb_events.c",
        "    for (int a = 1; a < n; a++) {\n        const pending p = got[a];",
        "    for (int a = n; a < n; a++) {\n        const pending p = got[a];",
        EVENTS + ["test_api_events_share_a_step"], 300),
    'losses-not-counted': ("core/src/rb_events.c",
        "        sim->events_lost++;\n",
        "",
        EVENTS, 300),
    'ring-overwrites-the-oldest': ("core/src/rb_events.c",
        "    if (sim->event_count == RB_EVENT_CAPACITY) {   /* full: this one is lost */",
        "    if (sim->event_count == RB_EVENT_CAPACITY) {   /* full: drop the oldest */\n"
        "        sim->event_head = (sim->event_head + 1) % RB_EVENT_CAPACITY;\n"
        "        sim->event_count--;\n"
        "    }\n"
        "    if (0) {",
        EVENTS, 300),
    'events-survive-a-rebuild': ("core/src/rb_events.c",
        "    sim->event_head = sim->event_count = 0;\n",
        "",
        EVENTS, 300),
    'no-valve-events': ("core/src/rb_events.c",
        "            if (at_ivo >= 0.0) got[n++]",
        "            if (0) got[n++]",
        EVENTS, 300),
    'limiter-cuts-unreported': ("core/src/rb_events.c",
        "        case RB_IGN_SOFT_CUT:  kind = RB_EVENT_LIMITER_CUT; break;",
        "        case RB_IGN_SOFT_CUT:  break;",
        EVENTS, 300),
}
