/*
 * test_api.c - the public engine API (stage 121): driving the engine, the car
 * on its crank, the settings, the dyno the sim owns, snapshots and events,
 * all through rb.h alone, as a host (rb_cli, the app) sees it.
 */
#include "rb.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES 480   /* 10 ms blocks at 48 kHz */

static float block[2 * FRAMES];

/* A sim at low quality (physics 96 kHz: quick), playing the engine. */
static rb_sim *engine_sim(const char *preset)
{
    rb_sim *sim = rb_create_with_quality(48000.0, RB_QUALITY_LOW);
    if (!sim) return NULL;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_load_preset(sim, rb_preset_find(preset));
    return sim;
}

/* Renders `seconds` in 10 ms blocks. */
static void run(rb_sim *sim, double seconds)
{
    for (long k = 0; k < lround(seconds * 100.0); k++) rb_render_block(sim, block, FRAMES);
}

/* Starts the engine with the key as a driver does: ignition on, the starter
 * until the engine runs, then off. Returns the seconds it took, or -1. */
static double start(rb_sim *sim)
{
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_STARTER, 1);
    for (int k = 0; k < 300; k++) {
        rb_render_block(sim, block, FRAMES);
        if (rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH) > 0.75 * rb_get_engine(sim, RB_ENG_IDLE_RPM, 0)) {
            rb_set_param(sim, RB_PARAM_STARTER, 0);
            return (k + 1) * 0.01;
        }
    }
    return -1.0;
}

/* ---- the driver's controls ---- */

void test_api_controls_metadata(void)
{
    /* Each control is a parameter with a name, a unit, a range and a
     * default; a value outside the range is clamped, a NaN refused. */
    static const struct { int id; const char *name, *unit; double min, max, def; } C[] = {
        { RB_PARAM_SOURCE,      "source",      "",      0, 1,     0 },
        { RB_PARAM_IGNITION,    "ignition",    "",      0, 1,     0 },
        { RB_PARAM_STARTER,     "starter",     "",      0, 1,     0 },
        { RB_PARAM_THROTTLE,    "throttle",    "",      0, 1,     0 },
        { RB_PARAM_LOAD_TORQUE, "load_torque", "N m", -500, 5000, 0 },
        { RB_PARAM_HOLD,        "hold",        "",      0, 1,     1 },
        { RB_PARAM_HOLD_RPM,    "hold_rpm",    "rpm",   0, 20000, 0 },
    };
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    for (size_t k = 0; k < sizeof C / sizeof C[0]; k++) {
        CHECK(rb_param_name(C[k].id) && !strcmp(rb_param_name(C[k].id), C[k].name));
        CHECK(rb_param_unit(C[k].id) && !strcmp(rb_param_unit(C[k].id), C[k].unit));
        CHECK_EQ_INT(rb_param_find(C[k].name), C[k].id);
        CHECK(rb_param_min(C[k].id) == C[k].min && rb_param_max(C[k].id) == C[k].max);
        CHECK(rb_param_default(C[k].id) == C[k].def && rb_get_param(sim, C[k].id) == C[k].def);
        CHECK(rb_param_flags(C[k].id) & RB_PF_LIVE);
        CHECK_EQ_INT(rb_set_param(sim, C[k].id, C[k].max + 1.0), RB_CLAMPED);
        CHECK(rb_get_param(sim, C[k].id) == C[k].max);
        CHECK_EQ_INT(rb_set_param(sim, C[k].id, NAN), RB_ERR_VALUE);
        CHECK(rb_get_param(sim, C[k].id) == C[k].max);
    }
    /* switches are whole numbers */
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_IGNITION, 0.5), RB_ERR_VALUE);
    rb_destroy(sim);
}

void test_api_key_starts_the_engine(void)
{
    /* The starter turns the free crank; with the ignition on it fires and
     * the engine computer holds its idle. Without the ignition it only
     * cranks, under 400 rpm. */
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    rb_set_param(sim, RB_PARAM_STARTER, 1);
    run(sim, 1.0);
    const double cranking = rb_get_telemetry(sim, RB_TLM_RPM);
    CHECK(cranking > 50.0 && cranking < 400.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_STARTER) == 1.0);
    rb_set_param(sim, RB_PARAM_STARTER, 0);
    run(sim, 1.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_RPM) < 0.5 * cranking);

    rb_reset(sim);   /* at rest again, the key off */
    CHECK(rb_get_param(sim, RB_PARAM_IGNITION) == 0.0 && rb_get_telemetry(sim, RB_TLM_RPM) == 0.0);
    const double took = start(sim);
    CHECK(took > 0.0 && took < 2.0);
    run(sim, 3.0);
    const double idle = rb_get_engine(sim, RB_ENG_IDLE_RPM, 0);
    CHECK_NEAR(rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH), idle, 0.15 * idle);
    CHECK(rb_get_telemetry(sim, RB_TLM_IGNITION) == 1.0 && rb_get_telemetry(sim, RB_TLM_STARTER) == 0.0);
    rb_destroy(sim);
}

void test_api_throttle_and_load(void)
{
    /* Running free: the pedal revs it; a load at the same pedal slows it. */
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK(start(sim) > 0.0);
    run(sim, 1.0);
    const double idle = rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH);
    rb_set_param(sim, RB_PARAM_THROTTLE, 0.12);
    run(sim, 1.5);
    const double revved = rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH);
    CHECK(revved > idle + 500.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_THROTTLE) == 0.12);
    rb_set_param(sim, RB_PARAM_LOAD_TORQUE, 40.0);
    run(sim, 1.5);
    CHECK(rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH) < revved - 300.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_LOAD_TORQUE) == 40.0);
    rb_destroy(sim);
}

void test_api_hold(void)
{
    /* Held (the default), the crank turns at hold_rpm whatever the engine
     * does; let go, it runs free from that speed. */
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK(rb_get_param(sim, RB_PARAM_HOLD) == 1.0);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    run(sim, 0.5);
    CHECK(rb_get_telemetry(sim, RB_TLM_RPM) == 3000.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_MAP) > 80e3);
    CHECK(rb_get_telemetry(sim, RB_TLM_TORQUE) > 100.0);   /* the four makes ~180 N m wide open */
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    rb_render_block(sim, block, 1);
    CHECK_NEAR(rb_get_telemetry(sim, RB_TLM_RPM), 3000.0, 5.0);   /* no jump */
    run(sim, 0.3);
    CHECK(rb_get_telemetry(sim, RB_TLM_RPM) > 3100.0);   /* wide open and free, it climbs */
    rb_destroy(sim);
}

/* ---- the car on the crank ---- */

void test_api_car_in_gear(void)
{
    /* In gear the crank carries the car: its inertia (gear_inertia) and the
     * road's load, which grows with speed. The clutch going in leaves the
     * engine's own inertia and no road load, at the same speed; coasting,
     * the engine alone then falls far faster than with the car. */
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    const double own = rb_get_engine(sim, RB_ENG_INERTIA, 0);
    CHECK(rb_get_telemetry(sim, RB_TLM_CRANK_INERTIA) == own);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);
    rb_set_param(sim, RB_PARAM_GEAR_INERTIA, 2.5);
    CHECK(rb_get_telemetry(sim, RB_TLM_CRANK_INERTIA) == own);   /* not in gear yet */
    rb_set_param(sim, RB_PARAM_CLUTCH, 1);
    CHECK(rb_get_telemetry(sim, RB_TLM_CRANK_INERTIA) == 2.5);
    CHECK(rb_get_engine(sim, RB_ENG_INERTIA, 0) == own);   /* the description stays the engine's */
    run(sim, 0.3);
    CHECK(rb_get_telemetry(sim, RB_TLM_LOAD_TORQUE) == 0.0);   /* held: no road */
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    rb_render_block(sim, block, 1);
    /* the road at 3000 rpm with 2.2 kg m^2 of car: 0.31 m wheels, so a
     * ratio of 0.31 sqrt(1300 / 2.2...) = 7.5; 15.3 m/s; rolling 153 N and
     * drag 98 N, times 0.31 / 7.5: about 10.4 N m */
    const double road = rb_get_telemetry(sim, RB_TLM_LOAD_TORQUE);
    CHECK(road > 9.0 && road < 12.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 0.0);
    run(sim, 0.5);
    const double geared = rb_get_telemetry(sim, RB_TLM_RPM);
    rb_set_param(sim, RB_PARAM_CLUTCH, 0);
    CHECK(rb_get_telemetry(sim, RB_TLM_RPM) == geared);   /* the same speed */
    CHECK(rb_get_telemetry(sim, RB_TLM_CRANK_INERTIA) == own);
    CHECK(rb_get_telemetry(sim, RB_TLM_LOAD_TORQUE) == 0.0);
    const double fall_car = 3000.0 - geared;   /* in 0.5 s with the car */
    run(sim, 0.5);
    CHECK(geared - rb_get_telemetry(sim, RB_TLM_RPM) > 3.0 * fall_car);
    rb_destroy(sim);
}

/* ---- the settings ---- */

void test_api_settings(void)
{
    /* A setting applies at once, or (the build-time ones, once the engine
     * has run) says RB_PENDING and applies at the next rebuild. Settings
     * stay across rebuilds; the driver's controls go back to rest and the
     * generation goes up. */
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    const double g0 = rb_get_telemetry(sim, RB_TLM_GENERATION);
    CHECK(g0 >= 1.0);
    CHECK(rb_param_flags(RB_PARAM_SEED) & RB_PF_REBUILD);
    CHECK(rb_param_flags(RB_PARAM_VARIATION) & RB_PF_LIVE);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_SEED, 7), RB_OK);   /* not run yet: now */
    CHECK(rb_variation_seed(sim) == 7);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIATION, 1.0), RB_OK);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_LISTENER, RB_LISTENER_CABIN), RB_OK);
    const double cabin = rb_listener_source_distance(sim, 0);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_LISTENER, RB_LISTENER_TAILPIPE), RB_OK);
    CHECK_NEAR(rb_listener_source_distance(sim, 0), 1.0, 0.3);
    CHECK(cabin > 1.5);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_LAYERS, 0), RB_CLAMPED);   /* at least one */
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_LAYERS, RB_LAYER_PIPES), RB_OK);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 2000.0);
    run(sim, 0.05);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_SEED, 9), RB_PENDING);
    CHECK(rb_variation_seed(sim) == 7);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_BURN_MODEL, RB_BURN_MODEL_CAPPED), RB_PENDING);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i6")), RB_OK);   /* a rebuild */
    CHECK(rb_get_telemetry(sim, RB_TLM_GENERATION) == g0 + 1.0);
    CHECK(rb_variation_seed(sim) == 9);
    CHECK(rb_get_param(sim, RB_PARAM_VARIATION) == 1.0 && rb_get_param(sim, RB_PARAM_LAYERS) == RB_LAYER_PIPES);
    CHECK(rb_get_param(sim, RB_PARAM_LISTENER) == RB_LISTENER_TAILPIPE);
    CHECK_NEAR(rb_listener_source_distance(sim, 0), 1.0, 0.3);   /* placed for the new engine */
    CHECK(rb_get_param(sim, RB_PARAM_IGNITION) == 0.0 && rb_get_telemetry(sim, RB_TLM_IGNITION) == 0.0);
    CHECK(rb_get_param(sim, RB_PARAM_HOLD_RPM) == 0.0 && rb_get_telemetry(sim, RB_TLM_RPM) == 0.0);
    /* the quality: a rebuild at other rates, the parameters kept */
    CHECK_EQ_INT(rb_get_quality(sim), RB_QUALITY_LOW);
    CHECK_EQ_INT(rb_set_quality(sim, RB_QUALITY_HIGH), RB_OK);
    CHECK_EQ_INT(rb_get_quality(sim), RB_QUALITY_HIGH);
    CHECK_NEAR(1.0 / rb_physics_dt(sim), 192000.0, 1.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_GENERATION) == g0 + 2.0);
    CHECK(rb_get_param(sim, RB_PARAM_VARIATION) == 1.0 && rb_get_param(sim, RB_PARAM_SOURCE) == RB_SOURCE_ENGINE);
    CHECK_EQ_INT(rb_get_preset(sim), rb_preset_find("i6"));
    CHECK_EQ_INT(rb_set_quality(sim, 9), RB_ERR_ARG);
    rb_destroy(sim);
}

/* ---- the dyno the sim owns ---- */

void test_api_dyno(void)
{
    /* The dyno holds a running engine at its speed and reads each cycle; a
     * sweep climbs; clearing forgets the readings; a rebuild takes it off. */
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK(rb_dyno_attached(sim) == 0 && isnan(rb_get_dyno(sim, RB_DYNO_READINGS)));
    CHECK_EQ_INT(rb_dyno_attach(sim, 100.0), RB_ERR_VALUE);   /* below 300 rpm */
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);
    run(sim, 0.5);
    CHECK_EQ_INT(rb_dyno_attach(sim, 3000.0), RB_OK);
    CHECK(rb_dyno_attached(sim) == 1 && rb_get_param(sim, RB_PARAM_HOLD) == 0.0);
    run(sim, 1.0);
    CHECK_NEAR(rb_get_telemetry(sim, RB_TLM_RPM_SMOOTH), 3000.0, 60.0);   /* held by the brake */
    CHECK(rb_get_dyno(sim, RB_DYNO_READINGS) >= 15.0);
    const double torque = rb_get_dyno(sim, RB_DYNO_BRAKE_TORQUE);
    CHECK(torque > 100.0 && torque < 250.0);
    CHECK_NEAR(rb_get_dyno(sim, RB_DYNO_BRAKE_POWER),
               torque * rb_get_dyno(sim, RB_DYNO_RPM) * 3.14159265358979 / 30.0, 1.0);
    CHECK_EQ_INT(rb_dyno_clear(sim), RB_OK);
    CHECK(rb_get_dyno(sim, RB_DYNO_READINGS) == 0.0);
    CHECK_EQ_INT(rb_dyno_set_sweep(sim, 4000.0, 1000.0), RB_OK);
    run(sim, 0.5);
    CHECK(rb_get_dyno(sim, RB_DYNO_TARGET_RPM) > 3400.0);
    CHECK_EQ_INT(rb_dyno_release(sim), RB_OK);
    CHECK(rb_dyno_attached(sim) == 0);
    CHECK_EQ_INT(rb_dyno_attach(sim, 3000.0), RB_OK);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i4")), RB_OK);   /* a rebuild */
    CHECK(rb_dyno_attached(sim) == 0 && isnan(rb_get_dyno(sim, RB_DYNO_RPM)));
    rb_destroy(sim);
}

/* ---- snapshots ---- */

void test_api_snapshot_layout(void)
{
    /* contracts/snapshot-v2.md's offsets: they never move within version 2. */
    CHECK_EQ_INT((int)sizeof(rb_snapshot_header), 112);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, size), 8);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, cylinders), 12);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, instruments), 16);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, cylinder_fields), 20);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, build_id), 32);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, generation), 40);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, sequence), 48);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, physics_tick), 56);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, audio_frame), 64);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, cycle_index), 72);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, physics_hz), 80);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, audio_hz), 88);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, phase_rad), 96);
    CHECK_EQ_INT((int)offsetof(rb_snapshot_header, speed_rad_s), 104);
    CHECK_EQ_INT(RB_SNAPSHOT_INSTRUMENTS, 31);   /* snapshot-v3 (stage 068) */
    CHECK_EQ_INT(RB_SNAPSHOT_CYLINDER_FIELDS, 5);
    /* the instruments are these telemetry ids, as the contract and
     * tools/snapshot_v2.py name them */
    static const char *const NAMES[24] = {
        "rpm", "rpm_smooth", "cycle_rpm", "map", "torque", "power", "lambda", "spark",
        "idle_valve", "throttle", "throttle_area", "ignition", "starter", "held", "fuel_cut",
        "spark_cut", "limiter_cuts", "overrun_time", "load_torque", "crank_inertia", "air_flow",
        "fuel_flow", "exhaust_temperature", "tailpipe_temperature",
    };
    for (int i = 0; i < 24; i++) {
        const char *name = rb_telemetry_name(RB_TLM_RPM + i);
        CHECK(name && !strcmp(name, NAMES[i]));
        CHECK(rb_telemetry_unit(RB_TLM_RPM + i) != NULL);
    }
    CHECK(rb_telemetry_unit(RB_TLM_COUNT) == NULL);
}

static uint64_t snap_buf[1024];   /* 8 KiB, aligned for the header */

void test_api_snapshot(void)
{
    /* A snapshot is the engine at one moment between two steps: its
     * instruments are the telemetry then, bit for bit, its cylinders
     * rb_get_cylinder's; each one is numbered; a rebuild is a new
     * generation and another build id. */
    rb_sim *sim = engine_sim("v8-cross");
    CHECK(sim != NULL);
    if (!sim) return;
    const int size = rb_snapshot_size(sim);
    CHECK_EQ_INT(size, 112 + 8 * RB_SNAPSHOT_INSTRUMENTS + 8 * 5 * 8);
    CHECK_EQ_INT(rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snap_buf, size - 1), RB_ERR_SIZE);
    CHECK_EQ_INT(rb_get_snapshot(sim, 1, snap_buf, size), RB_ERR_ID);
    CHECK_EQ_INT(rb_get_snapshot(NULL, RB_SNAPSHOT_VERSION, snap_buf, size), RB_ERR_ARG);
    CHECK_EQ_INT(rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, NULL, size), RB_ERR_ARG);

    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 0.6);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 4500.0);
    run(sim, 0.3);
    uint64_t sequence = 0, tick = 0, build = 0;
    for (int k = 0; k < 3; k++) {
        CHECK_EQ_INT(rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snap_buf, (int)sizeof snap_buf), size);
        const rb_snapshot_header *h = (const rb_snapshot_header *)snap_buf;
        const double *ins = (const double *)((const char *)snap_buf + sizeof *h);
        const double *cyl = ins + RB_SNAPSHOT_INSTRUMENTS;
        CHECK(h->magic == RB_SNAPSHOT_MAGIC && h->version == RB_SNAPSHOT_VERSION);
        CHECK(h->size == (uint32_t)size && h->cylinders == 8);
        CHECK(h->instruments == RB_SNAPSHOT_INSTRUMENTS && h->cylinder_fields == 5);
        CHECK(h->reserved[0] == 0 && h->reserved[1] == 0);
        CHECK((double)h->generation == rb_get_telemetry(sim, RB_TLM_GENERATION));
        CHECK((double)h->audio_frame == rb_get_telemetry(sim, RB_TLM_AUDIO_FRAMES));
        CHECK(h->physics_hz == 1.0 / rb_physics_dt(sim) && h->audio_hz == 48000.0);
        CHECK(h->phase_rad >= 0.0 && h->phase_rad < 4.0 * 3.14159265358979323846);
        CHECK_NEAR(h->speed_rad_s, 4500.0 * 3.14159265358979323846 / 30.0, 1e-9);
        CHECK(h->cycle_index >= 10);   /* 0.3 s at 4500 rpm: 11 cycles */
        for (int i = 0; i < RB_SNAPSHOT_INSTRUMENTS; i++)
            CHECK(memcmp(&ins[i], &(double){ rb_get_telemetry(sim, RB_TLM_RPM + i) }, 8) == 0);
        for (int c = 0; c < 8; c++) {
            CHECK(cyl[5 * c + 0] == rb_get_cylinder(sim, c, RB_CYL_PRESSURE));
            CHECK(cyl[5 * c + 1] == rb_get_cylinder(sim, c, RB_CYL_TEMPERATURE));
            CHECK(cyl[5 * c + 2] == rb_get_cylinder(sim, c, RB_CYL_BURN));
            CHECK(cyl[5 * c + 3] == rb_get_cylinder(sim, c, RB_CYL_INTAKE_LIFT));
            CHECK(cyl[5 * c + 4] == rb_get_cylinder(sim, c, RB_CYL_EXHAUST_LIFT));
        }
        if (k) {
            CHECK(h->sequence == sequence + 1);
            CHECK(h->physics_tick > tick);
            CHECK(h->build_id == build);
        }
        sequence = h->sequence, tick = h->physics_tick, build = h->build_id;
        run(sim, 0.02);
    }
    /* another engine: a new generation, another build id, its own cylinders */
    const double bore = rb_get_engine(sim, RB_ENG_BORE, 0);
    rb_set_engine(sim, RB_ENG_BORE, 0, 0.09);
    const int size2 = rb_snapshot_size(sim);
    CHECK_EQ_INT(rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snap_buf, (int)sizeof snap_buf), size2);
    const rb_snapshot_header *h = (const rb_snapshot_header *)snap_buf;
    CHECK(h->build_id != build && h->physics_tick == 0 && h->sequence == sequence + 1);
    CHECK((double)h->generation == rb_get_telemetry(sim, RB_TLM_GENERATION));
    rb_set_engine(sim, RB_ENG_BORE, 0, bore);   /* back: the same build id again */
    rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snap_buf, (int)sizeof snap_buf);
    CHECK(h->build_id == build);
    rb_destroy(sim);
}

/* ---- events ---- */

static rb_event events[RB_EVENT_CAPACITY];

void test_api_events(void)
{
    /* With RB_PARAM_EVENTS on, a running engine's sparks, injections and
     * valve openings arrive in the order they happen, numbered without a
     * gap, each in its cylinder's own order (fuel at IVC, the spark's
     * command, its discharge, the flame, the exhaust opening). The rev
     * limiter's cuts and the overrun fuel cut arrive too. Off, nothing. */
    static const char *const NAMES[RB_EVENT_KIND_COUNT] = {
        "spark-command", "spark-discharge", "flame-start", "fuel-injection",
        "intake-open", "exhaust-open", "fuel-cut", "limiter-cut"
    };
    for (int k = 0; k < RB_EVENT_KIND_COUNT; k++)
        CHECK(rb_event_kind_name(k) && !strcmp(rb_event_kind_name(k), NAMES[k]));
    CHECK(rb_event_kind_name(RB_EVENT_KIND_COUNT) == NULL);
    rb_sim *sim = engine_sim("i4");
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK(start(sim) > 0.0);
    run(sim, 0.5);
    CHECK_EQ_INT(rb_read_events(sim, events, RB_EVENT_CAPACITY), 0);   /* off by default */

    rb_set_param(sim, RB_PARAM_EVENTS, 1);
    long seen[RB_EVENT_KIND_COUNT] = { 0 };
    int last_kind[4] = { -1, -1, -1, -1 };
    long out_of_order = 0;
    uint64_t next = 0, tick = 0;
    double sub = 0.0;
    const double gen = rb_get_telemetry(sim, RB_TLM_GENERATION);
    for (int b = 0; b < 300; b++) {   /* 3 s: idle, then floored, then lifted */
        rb_set_param(sim, RB_PARAM_THROTTLE, (b >= 50 && b < 200) ? 1.0 : 0.0);
        rb_render_block(sim, block, FRAMES);
        const int n = rb_read_events(sim, events, RB_EVENT_CAPACITY);
        for (int i = 0; i < n; i++) {
            const rb_event *v = &events[i];
            CHECK(v->sequence == next++);
            CHECK(v->generation == (uint64_t)gen);
            CHECK(v->physics_tick > tick || (v->physics_tick == tick && v->subtick >= sub));
            tick = v->physics_tick, sub = v->subtick;
            CHECK(v->subtick >= 0.0 && v->subtick < 1.0);
            CHECK(v->phase_rad >= 0.0 && v->phase_rad < 4.0 * 3.14159265358979323846);
            CHECK(v->cylinder >= 0 && v->cylinder < 4);
            CHECK(v->kind >= 0 && v->kind < RB_EVENT_KIND_COUNT);
            if (v->kind < 0 || v->kind >= RB_EVENT_KIND_COUNT || v->cylinder < 0 || v->cylinder >= 4) continue;
            seen[v->kind]++;
            /* each cylinder's own order, round its cycle */
            const int prev = last_kind[v->cylinder];
            if (v->kind == RB_EVENT_SPARK_DISCHARGE && prev != RB_EVENT_SPARK_COMMAND) out_of_order++;
            if (v->kind == RB_EVENT_FLAME_START && prev != RB_EVENT_SPARK_DISCHARGE) out_of_order++;
            last_kind[v->cylinder] = v->kind;
        }
    }
    CHECK(rb_events_lost(sim) == 0);
    CHECK_EQ_INT((int)out_of_order, 0);
    for (int k = 0; k < RB_EVENT_KIND_COUNT; k++) CHECK(seen[k] > 0);
    /* every cylinder opens its valves once a cycle: about 3 s of cycles */
    CHECK(labs(seen[RB_EVENT_INTAKE_OPEN] - seen[RB_EVENT_EXHAUST_OPEN]) <= 4);
    CHECK(seen[RB_EVENT_SPARK_DISCHARGE] <= seen[RB_EVENT_SPARK_COMMAND]);
    CHECK(seen[RB_EVENT_LIMITER_CUT] > 0 && seen[RB_EVENT_FUEL_CUT] > 0);

    /* unread, the ring keeps the oldest and counts what it lost */
    run(sim, 1.0);
    CHECK(rb_events_lost(sim) > 0);
    CHECK_EQ_INT(rb_read_events(sim, events, RB_EVENT_CAPACITY), RB_EVENT_CAPACITY);
    CHECK(events[0].sequence == next);
    CHECK_EQ_INT(rb_read_events(sim, events, 0), 0);
    CHECK_EQ_INT(rb_read_events(sim, NULL, 4), RB_ERR_ARG);
    /* a rebuild clears them */
    run(sim, 0.1);
    rb_reset(sim);
    CHECK(rb_events_lost(sim) == 0);
    CHECK_EQ_INT(rb_read_events(sim, events, RB_EVENT_CAPACITY), 0);
    rb_destroy(sim);
}

void test_api_events_share_a_step(void)
{
    /* Events that fall in the same physics step must still come out in the
     * order they happened in it. The intake cam is set so that each
     * cylinder's intake opens half a degree after the exhaust of the
     * cylinder 180 deg before it opens: with coarse steps (16 kHz physics,
     * 1-2.6 deg a step from 2500 to 7000 rpm) the two share a step every
     * time, two cylinders' events, which the sim finds cylinder by
     * cylinder, not in the order they happen. */
    rb_sim *sim = rb_create_with(16000.0, 1);
    CHECK(sim != NULL);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_load_preset(sim, rb_preset_find("i4"));
    const double evo = rb_get_engine(sim, RB_ENG_EXHAUST_OPENS, 0);
    double move = fmod(evo + 0.5 - rb_get_engine(sim, RB_ENG_INTAKE_OPENS, 0), 180.0);
    if (move > 90.0) move -= 180.0;
    if (move <= -90.0) move += 180.0;
    CHECK_EQ_INT(rb_set_engine(sim, RB_ENG_INTAKE_CENTER, 0,
                               rb_get_engine(sim, RB_ENG_INTAKE_CENTER, 0) + move), RB_OK);
    CHECK_NEAR(fmod(rb_get_engine(sim, RB_ENG_INTAKE_OPENS, 0) - evo + 720.0, 180.0), 0.5, 1e-9);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_EVENTS, 1);
    uint64_t tick = 0, shared = 0, total = 0;
    double sub = -1.0;
    int cylinder = -1;
    long backwards = 0;
    for (int b = 0; b < 300; b++) {   /* 3 s */
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 2500.0 + 15.0 * b);
        rb_render_block(sim, block, 160);
        const int n = rb_read_events(sim, events, RB_EVENT_CAPACITY);
        for (int i = 0; i < n; i++) {
            const rb_event *v = &events[i];
            if (v->physics_tick == tick && total) {
                if (v->cylinder != cylinder) shared++;
                if (v->subtick < sub) backwards++;
            } else if (v->physics_tick < tick) {
                backwards++;
            }
            tick = v->physics_tick, sub = v->subtick, cylinder = v->cylinder;
            total++;
        }
    }
    CHECK(total > 1000);
    CHECK(shared > 100);   /* two cylinders' events in a step: the test means something */
    CHECK_EQ_INT((int)backwards, 0);
    rb_destroy(sim);
}

void test_api_variant(void)
{
    /* The A/B switch (stage 127): a live integer setting. With no candidate
     * reading a number, it plays the accepted sound, bit for bit, and it
     * switches while the engine runs without a rebuild. */
    enum { HEARD = 4800 };
    static float a[HEARD], b[HEARD];
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK(rb_get_param(sim, RB_PARAM_VARIANT) == 0.0);
    CHECK(strcmp(rb_param_name(RB_PARAM_VARIANT), "variant") == 0);
    CHECK(rb_param_flags(RB_PARAM_VARIANT) & RB_PF_LIVE);
    CHECK(rb_param_flags(RB_PARAM_VARIANT) & RB_PF_INTEGER);
    CHECK(!(rb_param_flags(RB_PARAM_VARIANT) & (RB_PF_REBUILD | RB_PF_CONTROL)));
    CHECK(rb_param_max(RB_PARAM_VARIANT) == RB_VARIANT_MAX);

    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIANT, 1.5), RB_ERR_VALUE);
    CHECK(rb_get_param(sim, RB_PARAM_VARIANT) == 0.0);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIANT, 99), RB_CLAMPED);
    CHECK(rb_get_param(sim, RB_PARAM_VARIANT) == RB_VARIANT_MAX);

    /* It is a setting, not a control: a rebuild keeps it, without a new
     * generation of its own. */
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIANT, 3), RB_OK);
    const double generation = rb_get_telemetry(sim, RB_TLM_GENERATION);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIANT, 2), RB_OK);
    CHECK(rb_get_telemetry(sim, RB_TLM_GENERATION) == generation);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i4")), RB_OK);
    CHECK(rb_get_param(sim, RB_PARAM_VARIANT) == 2.0);

    /* A number no candidate uses is today's sound exactly. */
    for (int pass = 0; pass < 2; pass++) {
        float *out = pass ? b : a;
        rb_destroy(sim);   /* a fresh sim each: the output's filters start alike */
        sim = rb_create(NULL);
        CHECK(sim != NULL);
        if (!sim) return;
        CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i4")), RB_OK);
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_set_param(sim, RB_PARAM_VARIANT, pass ? RB_VARIANT_MAX : 0);
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 2500);
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1);
        rb_render_block(sim, out, HEARD);
    }
    CHECK(memcmp(a, b, sizeof a) == 0);
    double power = 0.0;
    for (int n = 0; n < HEARD; n++) power += (double)a[n] * a[n];
    CHECK(power > 0.0);   /* and it is a sound */
    rb_destroy(sim);
}

/* Runs an engine for `seconds` of sound (blocks of 0.1 s at the default 48 kHz). */
static void run_seconds(rb_sim *sim, double seconds)
{
    static float out[4800];
    for (int i = 0; i < (int)(seconds * 10.0 + 0.5); i++) rb_render_block(sim, out, 4800);
}

void test_api_afr_and_spark_offset(void)
{
    /* Stage 067's two engine-computer settings: live, kept by a rebuild, and
     * each with its physical effect. At their defaults nothing changes. */
    CHECK(strcmp(rb_param_name(RB_PARAM_AFR), "afr") == 0);
    CHECK(strcmp(rb_param_name(RB_PARAM_SPARK_OFFSET), "spark_offset") == 0);
    CHECK(strcmp(rb_param_unit(RB_PARAM_SPARK_OFFSET), "deg") == 0);
    for (int id = RB_PARAM_AFR; id <= RB_PARAM_SPARK_OFFSET; id++) {
        CHECK(rb_param_flags(id) & RB_PF_LIVE);
        CHECK(!(rb_param_flags(id) & (RB_PF_REBUILD | RB_PF_CONTROL | RB_PF_INTEGER)));
    }
    CHECK_NEAR(rb_param_default(RB_PARAM_AFR), 14.596, 1e-12);
    CHECK(rb_param_default(RB_PARAM_SPARK_OFFSET) == 0.0);

    /* the effects, at 3000 rpm wide open, held */
    double lambda[3], spark[3], torque[3];
    static const double AFR[3] = { 14.596, 12.0, 14.596 };
    static const double OFFSET[3] = { 0.0, 0.0, -10.0 };
    for (int k = 0; k < 3; k++) {
        rb_sim *sim = rb_create(NULL);
        CHECK(sim != NULL);
        if (!sim) return;
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);   /* not the test tone */
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1);
        CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_AFR, AFR[k]), RB_OK);
        CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_SPARK_OFFSET, OFFSET[k]), RB_OK);
        run_seconds(sim, 1.0);              /* about 25 cycles at 3000 rpm */
        lambda[k] = rb_get_telemetry(sim, RB_TLM_LAMBDA);
        spark[k] = rb_get_telemetry(sim, RB_TLM_SPARK);
        torque[k] = rb_get_telemetry(sim, RB_TLM_TORQUE);
        rb_destroy(sim);
    }
    CHECK_NEAR(lambda[0], 1.0, 0.02);
    CHECK_NEAR(lambda[1], 12.0 / 14.596, 0.02);                  /* richer, as asked */
    CHECK_NEAR(spark[2], spark[0] - 10.0, 0.5);                  /* 10 degrees later */
    CHECK(torque[2] < torque[0] - 2.0);                          /* off the best torque: less */
    CHECK(fabs(spark[1] - spark[0]) < 3.0);

    /* settings, not controls: a rebuild keeps them */
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_AFR, 13.0), RB_OK);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_SPARK_OFFSET, 4.0), RB_OK);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("v8-cross")), RB_OK);
    CHECK(rb_get_param(sim, RB_PARAM_AFR) == 13.0);
    CHECK(rb_get_param(sim, RB_PARAM_SPARK_OFFSET) == 4.0);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_SPARK_OFFSET, 40.0), RB_CLAMPED);
    CHECK(rb_get_param(sim, RB_PARAM_SPARK_OFFSET) == 15.0);
    rb_destroy(sim);
}

/* The largest step between two samples in out[from, to). */
static double max_step(const float *out, int from, int to)
{
    double m = 0.0;
    for (int i = from + 1; i < to; i++) {
        const double d = fabs((double)out[i] - (double)out[i - 1]);
        if (d > m) m = d;
    }
    return m;
}

void test_api_listen_changes_restart_the_histories(void)
{
    /* Stage 067's editor auditions the sound's layers and the listener's place live: each change
     * starts the listener's histories afresh from the flows now (rb_sim_listen primes them), so
     * nothing jumps. A scene change is a rebuild, a new engine behind the host's fade. */
    enum { STEADY = 9600, AFTER = 960 };
    static float before[STEADY], after[AFTER];
    for (int change = 0; change < 2; change++) {
        rb_sim *sim = rb_create(NULL);
        CHECK(sim != NULL);
        if (!sim) return;
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1);
        for (int i = 0; i < 5; i++) rb_render_block(sim, before, STEADY);   /* 1 s to settle */
        const double steady = max_step(before, 0, STEADY);
        if (change == 0) CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_LAYERS, RB_LAYER_FLOW | RB_LAYER_ENGINE), RB_OK);
        else CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_LISTENER, RB_LISTENER_TAILPIPE), RB_OK);
        rb_render_block(sim, after, AFTER);
        /* the first samples after the change step no further than the engine's own sound did;
         * the tailpipe's place is louder, so it gets the ratio of the places' distances */
        const double allowed = change == 0 ? 1.5 * steady : 6.0 * steady;
        const double step = fmax(fabs((double)after[0] - (double)before[STEADY - 1]), max_step(after, 0, 48));
        if (!(step <= allowed)) printf("        change %d: a step of %g after it, %g before\n", change, step, steady);
        CHECK(step <= allowed);
        rb_destroy(sim);
    }
}

/* The busiest second of RB_TLM_GUARD_RATE in a drive: started, idled, floored and lifted, a second
 * each, at the lean tier. */
static double busiest_second(int preset, int field, double value, int phase_of_max[1])
{
    rb_sim *sim = rb_create_with_quality(48000, RB_QUALITY_LEAN);
    if (!sim) return NAN;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_load_preset(sim, preset);
    if (field >= 0) rb_set_engine(sim, field, 0, value);
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    static float out[480];
    double worst = 0.0;
    for (int phase = 0; phase < 4; phase++) {
        rb_set_param(sim, RB_PARAM_STARTER, phase == 0);
        rb_set_param(sim, RB_PARAM_THROTTLE, phase == 2 ? 1.0 : 0.0);
        double sum = 0.0;
        for (int k = 0; k < 100; k++) {
            rb_render_block(sim, out, 480);
            sum += rb_get_telemetry(sim, RB_TLM_GUARD_RATE);
        }
        if (sum / 100.0 > worst) {
            worst = sum / 100.0;
            phase_of_max[0] = phase;
        }
    }
    rb_destroy(sim);
    return worst;
}

void test_api_guard_rate_warns_only_outside_the_model(void)
{
    /* Stage 068: the model's own warning. The presets' guards stay far under RB_GUARD_RATE_WARN
     * in every phase of a drive; the narrowest headers the editor allows choke wide open and go
     * over it. The telemetry is named and in the snapshot. */
    for (int p = 0; p < rb_preset_count(); p++) {
        int phase = -1;
        const double worst = busiest_second(p, -1, 0.0, &phase);
        if (!(worst < 0.25 * RB_GUARD_RATE_WARN)) fprintf(stderr, "    %s: %.0f per second (phase %d)\n", rb_preset_key(p), worst, phase);
        CHECK(worst < 0.25 * RB_GUARD_RATE_WARN);
    }
    int phase = -1;
    const double choked = busiest_second(0, RB_ENG_HEADER_DIAMETER, rb_engine_field_min(RB_ENG_HEADER_DIAMETER), &phase);
    CHECK(choked > 2.0 * RB_GUARD_RATE_WARN);
    CHECK_EQ_INT(phase, 2);   /* wide open */
    CHECK(strcmp(rb_telemetry_name(RB_TLM_GUARD_RATE), "guard_rate") == 0);
    CHECK(RB_TLM_GUARD_RATE >= RB_TLM_RPM && RB_TLM_GUARD_RATE - RB_TLM_RPM < RB_SNAPSHOT_INSTRUMENTS);
}

void test_api_dyno_in_the_telemetry(void)
{
    /* Stage 068: the core's dyno as telemetry (and so in every snapshot): attached, its target,
     * its readings, the last one's speed, torque and power, what rb_get_dyno says. */
    rb_sim *sim = rb_create_with_quality(48000, RB_QUALITY_LEAN);
    CHECK(sim != NULL);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_load_preset(sim, 0);
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_ATTACHED) == 0.0 && rb_get_telemetry(sim, RB_TLM_DYNO_READINGS) == 0.0);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);   /* held running at 3000, then the dyno's */
    static float out[480];
    for (int k = 0; k < 50; k++) rb_render_block(sim, out, 480);
    CHECK_EQ_INT(rb_dyno_attach(sim, 3000.0), RB_OK);
    for (int k = 0; k < 100; k++) rb_render_block(sim, out, 480);
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_ATTACHED) == 1.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_TARGET_RPM) == rb_get_dyno(sim, RB_DYNO_TARGET_RPM));
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_READINGS) == rb_get_dyno(sim, RB_DYNO_READINGS));
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_READINGS) > 10.0);
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_RPM) == rb_get_dyno(sim, RB_DYNO_RPM));
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_TORQUE) == rb_get_dyno(sim, RB_DYNO_BRAKE_TORQUE));
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_POWER) == rb_get_dyno(sim, RB_DYNO_BRAKE_POWER));
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_TORQUE) > 100.0);
    /* in the snapshot, at its place among the instruments */
    static unsigned char snap[1 << 15];
    CHECK(rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snap, sizeof snap) > 0);
    double torque;
    memcpy(&torque, snap + sizeof(rb_snapshot_header) + 8 * (RB_TLM_DYNO_TORQUE - RB_TLM_RPM), sizeof torque);
    const double told = rb_get_telemetry(sim, RB_TLM_DYNO_TORQUE);
    if (torque != told) {   /* CI's MSVC 19.52 (2026-11-23) disagreed: say how */
        fprintf(stderr, "    snapshot %a, telemetry %a, rb_get_dyno %a; header %u bytes, instruments %d, index %d\n",
                torque, told, rb_get_dyno(sim, RB_DYNO_BRAKE_TORQUE), (unsigned)sizeof(rb_snapshot_header),
                RB_SNAPSHOT_INSTRUMENTS, RB_TLM_DYNO_TORQUE - RB_TLM_RPM);
        for (int k = 0; k < RB_SNAPSHOT_INSTRUMENTS; k++) {
            double v;
            memcpy(&v, snap + sizeof(rb_snapshot_header) + 8 * k, sizeof v);
            fprintf(stderr, "      [%d] snapshot %a telemetry %a\n", k, v, rb_get_telemetry(sim, RB_TLM_RPM + k));
        }
    }
    CHECK(torque == told);
    rb_dyno_release(sim);
    CHECK(rb_get_telemetry(sim, RB_TLM_DYNO_ATTACHED) == 0.0);
    rb_destroy(sim);
}

/* ---- stage 069: a stopped engine asleep (RB_PARAM_REST) ---- */

/* A lean preset 0, the engine the source, the hold off, RB_PARAM_REST as given. */
static rb_sim *still_engine(double rest)
{
    rb_sim *sim = rb_create_with_quality(48000, RB_QUALITY_LEAN);
    if (!sim) return NULL;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_load_preset(sim, 0);
    rb_set_param(sim, RB_PARAM_HOLD, 0.0);
    rb_set_param(sim, RB_PARAM_REST, rest);
    return sim;
}

/* Renders `blocks` blocks of 480 frames into out (which holds them all). */
static void render_blocks(rb_sim *sim, float *out, int blocks)
{
    for (int k = 0; k < blocks; k++) rb_render_block(sim, out + 480 * k, 480);
}

void test_api_rest_sleeps_a_still_engine_only(void)
{
    /* A stopped engine left alone falls asleep after RB_REST_HOLD_S of quiet, and writes silence
     * from then; until then it renders exactly what a sim without the rest renders, and with
     * RB_PARAM_REST 0 (the default) it never sleeps. */
    CHECK(strcmp(rb_param_name(RB_PARAM_REST), "rest") == 0);
    CHECK(rb_param_default(RB_PARAM_REST) == 0.0);
    rb_sim *a = still_engine(0.0), *b = still_engine(1.0);
    CHECK(a && b);
    if (!a || !b) return;
    enum { BLOCKS = 600 };   /* 6 s */
    static float xa[480 * BLOCKS], xb[480 * BLOCKS];
    int slept_at = -1;
    for (int k = 0; k < BLOCKS; k++) {
        rb_render_block(a, xa + 480 * k, 480);
        rb_render_block(b, xb + 480 * k, 480);
        CHECK(!rb_resting(a));
        if (slept_at < 0 && rb_resting(b)) slept_at = k;
    }
    CHECK(slept_at >= 0);
    if (slept_at < 0) slept_at = BLOCKS - 1;
    /* not before a second of quiet blocks: the last block over RB_REST_LEVEL (the build's own
     * settling) is RB_REST_HOLD_S or more before it slept */
    int last_loud = -1;
    for (int k = 0; k <= slept_at; k++) {
        double peak = 0.0;
        for (int i = 0; i < 480; i++) peak = fmax(peak, fabs((double)xa[480 * k + i]));
        if (!(peak < RB_REST_LEVEL)) last_loud = k;
    }
    CHECK(last_loud >= 0);   /* the fresh build settles audibly first */
    CHECK(slept_at - last_loud >= (int)(RB_REST_HOLD_S * 100));
    CHECK(memcmp(xa, xb, sizeof(float) * 480 * (size_t)(slept_at + 1)) == 0);
    double after = 0.0, b_after = 0.0;
    for (int i = 480 * (slept_at + 1); i < 480 * BLOCKS; i++) {
        after = fmax(after, fabs((double)xa[i]));
        b_after = fmax(b_after, fabs((double)xb[i]));
    }
    CHECK(b_after == 0.0);
    CHECK(after < RB_REST_LEVEL);   /* what sleep left out was silent anyway */
    CHECK(rb_get_telemetry(b, RB_TLM_RPM) == 0.0);
    rb_destroy(a);
    rb_destroy(b);
}

void test_api_rest_wakes_on_anything_set(void)
{
    /* Asleep, the engine wakes for a parameter that changes, a field, the scene, the dyno and a
     * rebuild; setting a parameter to the value it has doesn't wake it. Woken by the key and the
     * starter, it starts and runs as it would have. */
    static float out[480 * 400];
    rb_sim *sim = still_engine(1.0);
    CHECK(sim != NULL);
    if (!sim) return;
    render_blocks(sim, out, 400);
    CHECK(rb_resting(sim));
    rb_set_param(sim, RB_PARAM_THROTTLE, 0.0);   /* the same value: nothing set */
    render_blocks(sim, out, 1);
    CHECK(rb_resting(sim));

    rb_set_param(sim, RB_PARAM_THROTTLE, 0.3);
    render_blocks(sim, out, 1);
    CHECK(!rb_resting(sim));
    render_blocks(sim, out, 200);
    CHECK(rb_resting(sim));   /* asleep again, the throttle open: nothing moves */

    CHECK(rb_set_engine(sim, RB_ENG_IDLE_RPM, 0, 900.0) == RB_OK);
    render_blocks(sim, out, 1);
    CHECK(!rb_resting(sim));
    render_blocks(sim, out, 200);
    CHECK(rb_resting(sim));

    rb_reset(sim);   /* a rebuild, nothing set */
    render_blocks(sim, out, 1);
    CHECK(!rb_resting(sim));
    render_blocks(sim, out, 300);
    CHECK(rb_resting(sim));

    CHECK(rb_dyno_attach(sim, 3000.0) == RB_OK);   /* on a dyno: never asleep, even still */
    render_blocks(sim, out, 300);
    CHECK(!rb_resting(sim));
    rb_destroy(sim);

    /* the key and the starter wake it, and the engine runs */
    sim = still_engine(1.0);
    CHECK(sim != NULL);
    if (!sim) return;
    render_blocks(sim, out, 400);
    CHECK(rb_resting(sim));
    rb_set_param(sim, RB_PARAM_IGNITION, 1.0);
    rb_set_param(sim, RB_PARAM_STARTER, 1.0);
    render_blocks(sim, out, 150);
    rb_set_param(sim, RB_PARAM_STARTER, 0.0);
    render_blocks(sim, out, 300);
    CHECK(!rb_resting(sim));
    CHECK(rb_get_telemetry(sim, RB_TLM_RPM) > 500.0);
    double peak = 0.0;
    for (int i = 0; i < 480 * 300; i++) peak = fmax(peak, fabs((double)out[i]));
    CHECK(peak > 0.01);   /* it sounds */
    rb_destroy(sim);
}

void test_api_rest_never_touches_a_running_engine(void)
{
    /* Running (idling, the throttle blipped), a sim with the rest on renders exactly what one
     * without it renders, and never sleeps. */
    static float xa[480 * 500], xb[480 * 500];
    rb_sim *a = still_engine(0.0), *b = still_engine(1.0);
    CHECK(a && b);
    if (!a || !b) return;
    rb_sim *both[2] = { a, b };
    for (int s = 0; s < 2; s++) {
        rb_set_param(both[s], RB_PARAM_IGNITION, 1.0);
        rb_set_param(both[s], RB_PARAM_STARTER, 1.0);
    }
    for (int part = 0; part < 4; part++) {
        for (int s = 0; s < 2; s++) {
            if (part == 1) rb_set_param(both[s], RB_PARAM_STARTER, 0.0);
            if (part == 2) rb_set_param(both[s], RB_PARAM_THROTTLE, 0.6);
            if (part == 3) rb_set_param(both[s], RB_PARAM_THROTTLE, 0.0);
        }
        render_blocks(a, xa, 125);
        render_blocks(b, xb, 125);
        CHECK(memcmp(xa, xb, sizeof(float) * 480 * 125) == 0);
        CHECK(!rb_resting(b));
    }
    /* turned all the way down, the running engine is silent but still turning: never asleep */
    rb_set_param(b, RB_PARAM_MASTER_GAIN, 0.0);
    render_blocks(b, xb, 300);
    CHECK(!rb_resting(b));
    CHECK(rb_get_telemetry(b, RB_TLM_RPM) > 500.0);
    rb_destroy(a);
    rb_destroy(b);
}

/* The loudest sample (dB below full scale) from `from_s` to `to_s` seconds of a freshly built,
 * still engine: `preset` as itself (family < 0) or as a layout, lean, the rest as given. */
static double still_engine_level(int preset, int family, int cylinders, double rest, double from_s, double to_s)
{
    rb_sim *sim = rb_create_with_quality(48000, RB_QUALITY_LEAN);
    if (!sim) return 0.0;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_load_preset(sim, preset);
    if (family >= 0) {
        const int crank = rb_layout_usual_crank(family, cylinders);
        rb_load_layout(sim, preset, family, cylinders, crank, rb_layout_angle(family, cylinders, crank, RB_ANGLE_USUAL));
    }
    rb_set_param(sim, RB_PARAM_HOLD, 0.0);
    rb_set_param(sim, RB_PARAM_REST, rest);
    static float out[4800];
    double peak = 0.0;
    for (double t = 0.1; t <= to_s + 1e-9; t += 0.1) {
        rb_render_block(sim, out, 4800);
        if (t > from_s)
            for (int i = 0; i < 4800; i++) peak = fmax(peak, fabs((double)out[i]));
    }
    rb_destroy(sim);
    return peak > 0.0 ? 20.0 * log10(peak) : -999.0;
}

void test_api_rest_keeps_a_fresh_engine_from_singing(void)
{
    /* Bernardo, 2026-11-22: an inline 3, the key off, editing; a rising high-pitched tone for
     * about a second, then nothing. A freshly built engine's gas starts at 298 K against walls
     * held at a warm engine's (the exhaust's 550 K); the pipes' heat transfer follows the flow
     * (St cp |mdot|), so a still engine's tiny oscillations pump heat in and grow a tone of
     * about 800 Hz (the four's peaks about 24 s after the build, -51 dBFS before the app's
     * level). It's the core's (docs/stage-069.md, left for phase B). Asleep (RB_PARAM_REST) the
     * engine stops before it starts: no preset, nor the inline 3, gets back above the rest's
     * level after the first 3 s. */
    CHECK(still_engine_level(0, -1, 0, 0.0, 15.0, 30.0) > -70.0);   /* without the rest it sings */
    for (int p = 0; p < rb_preset_count(); p++) {
        const double level = still_engine_level(p, -1, 0, 1.0, 3.0, 30.0);
        if (!(level < 20.0 * log10(RB_REST_LEVEL))) fprintf(stderr, "    %s: %.1f dBFS\n", rb_preset_key(p), level);
        CHECK(level < 20.0 * log10(RB_REST_LEVEL));
    }
    CHECK(still_engine_level(0, RB_FAMILY_INLINE, 3, 1.0, 3.0, 30.0) < 20.0 * log10(RB_REST_LEVEL));
}
