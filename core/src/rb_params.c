/*
 * rb_params.c - the parameter and telemetry tables.
 *
 * To add a parameter:
 *   1. add an id to rb_param in rb.h (just before RB_PARAM_COUNT)
 *   2. add its row to PARAMS below
 *   3. read sim->param[id] in the model, or apply it in rb_sim_apply
 *      (rb_drive.c) when it's set
 * The unit tests check that every id has a complete, sane row.
 */
#include "rb_internal.h"

#include <math.h>
#include <string.h>

typedef struct param_info {
    const char *name;   /* stable identifier used by the CLI and the UI */
    const char *unit;   /* display unit, "" when dimensionless          */
    double      min, max, def;
    int         flags;  /* RB_PF_* bits                                 */
} param_info;

enum {   /* shorthands for the flags */
    LIVE = RB_PF_LIVE, REBUILD = RB_PF_REBUILD, INT = RB_PF_INTEGER, CONTROL = RB_PF_CONTROL
};

static const param_info PARAMS[RB_PARAM_COUNT] = {
    /*                             name               unit  min    max      default flags      */
    [RB_PARAM_MASTER_GAIN]     = { "master_gain",     "",   0.0,   4.0,     1.0,    LIVE },
    [RB_PARAM_TEST_TONE_HZ]    = { "test_tone_hz",    "Hz", 20.0,  20000.0, 220.0,  LIVE },
    [RB_PARAM_TEST_TONE_LEVEL] = { "test_tone_level", "",   0.0,   1.0,     0.5,    LIVE },
    /* stage 121 */
    [RB_PARAM_SOURCE]          = { "source",          "",   0.0,   1.0,     0.0,    LIVE | INT },
    [RB_PARAM_IGNITION]        = { "ignition",        "",   0.0,   1.0,     0.0,    LIVE | INT | CONTROL },
    [RB_PARAM_STARTER]         = { "starter",         "",   0.0,   1.0,     0.0,    LIVE | INT | CONTROL },
    [RB_PARAM_THROTTLE]        = { "throttle",        "",   0.0,   1.0,     0.0,    LIVE | CONTROL },
    [RB_PARAM_LOAD_TORQUE]     = { "load_torque",     "N m", -500.0, 5000.0, 0.0,   LIVE | CONTROL },
    [RB_PARAM_HOLD]            = { "hold",            "",   0.0,   1.0,     1.0,    LIVE | INT | CONTROL },
    [RB_PARAM_HOLD_RPM]        = { "hold_rpm",        "rpm", 0.0,  20000.0, 0.0,    LIVE | CONTROL },
    [RB_PARAM_GEAR_INERTIA]    = { "gear_inertia",    "kg m^2", 0.0, 99.0,  0.0,    LIVE | CONTROL },
    [RB_PARAM_CLUTCH]          = { "clutch",          "",   0.0,   1.0,     0.0,    LIVE | INT | CONTROL },
    [RB_PARAM_VARIATION]       = { "variation",       "",   0.0,   4.0,     0.0,    LIVE },
    [RB_PARAM_LISTENER]        = { "listener",        "",   0.0,   RB_LISTENER_COUNT - 1, RB_LISTENER_BEHIND, LIVE | INT },
    [RB_PARAM_LAYERS]          = { "layers",          "",   1.0,   RB_LAYER_ALL, RB_LAYER_ALL, LIVE | INT },
    [RB_PARAM_OVERRUN_TRANSITION] = { "overrun_transition", "s", 0.0, 2.0,  0.15,   LIVE },
    [RB_PARAM_BURN_MODEL]      = { "burn_model",      "",   0.0,   1.0,     RB_BURN_MODEL_CHARGE, REBUILD | INT },
    [RB_PARAM_SEED]            = { "seed",            "",   0.0,   4294967295.0, 0.0, REBUILD | INT },
    [RB_PARAM_MOMENTUM_ORDER]  = { "momentum_order",  "",   1.0,   2.0,     2.0,    REBUILD | INT },
    [RB_PARAM_EVENTS]          = { "events",          "",   0.0,   1.0,     0.0,    LIVE | INT },
};

typedef struct telemetry_info {
    const char *name, *unit;
} telemetry_info;

static const telemetry_info TELEMETRY[RB_TLM_COUNT] = {
    [RB_TLM_TIME_S]        = { "time_s",        "s" },
    [RB_TLM_STEPS]         = { "steps",         "" },
    [RB_TLM_OUT_PEAK]      = { "out_peak",      "" },
    [RB_TLM_OUT_RMS]       = { "out_rms",       "" },
    [RB_TLM_NONFINITE]     = { "nonfinite",     "" },
    [RB_TLM_LIMITED]       = { "limited",       "" },
    [RB_TLM_GENERATION]    = { "generation",    "" },
    [RB_TLM_AUDIO_FRAMES]  = { "audio_frames",  "" },
    [RB_TLM_RPM]           = { "rpm",           "rpm" },
    [RB_TLM_RPM_SMOOTH]    = { "rpm_smooth",    "rpm" },
    [RB_TLM_CYCLE_RPM]     = { "cycle_rpm",     "rpm" },
    [RB_TLM_MAP]           = { "map",           "Pa" },
    [RB_TLM_TORQUE]        = { "torque",        "N m" },
    [RB_TLM_POWER]         = { "power",         "W" },
    [RB_TLM_LAMBDA]        = { "lambda",        "" },
    [RB_TLM_SPARK]         = { "spark",         "deg" },
    [RB_TLM_IDLE_VALVE]    = { "idle_valve",    "" },
    [RB_TLM_THROTTLE]      = { "throttle",      "" },
    [RB_TLM_THROTTLE_AREA] = { "throttle_area", "m^2" },
    [RB_TLM_IGNITION]      = { "ignition",      "" },
    [RB_TLM_STARTER]       = { "starter",       "" },
    [RB_TLM_HELD]          = { "held",          "" },
    [RB_TLM_FUEL_CUT]      = { "fuel_cut",      "" },
    [RB_TLM_SPARK_CUT]     = { "spark_cut",     "" },
    [RB_TLM_LIMITER_CUTS]  = { "limiter_cuts",  "" },
    [RB_TLM_OVERRUN_TIME]  = { "overrun_time",  "s" },
    [RB_TLM_LOAD_TORQUE]   = { "load_torque",   "N m" },
    [RB_TLM_CRANK_INERTIA] = { "crank_inertia", "kg m^2" },
    [RB_TLM_AIR_FLOW]      = { "air_flow",      "kg/s" },
    [RB_TLM_FUEL_FLOW]     = { "fuel_flow",     "kg/s" },
    [RB_TLM_EXHAUST_T]     = { "exhaust_temperature",  "K" },
    [RB_TLM_TAILPIPE_T]    = { "tailpipe_temperature", "K" },
};

static int param_ok(int id)
{
    return id >= 0 && id < RB_PARAM_COUNT;
}

int         rb_param_count(void)     { return RB_PARAM_COUNT; }
const char *rb_param_name(int id)    { return param_ok(id) ? PARAMS[id].name  : NULL; }
const char *rb_param_unit(int id)    { return param_ok(id) ? PARAMS[id].unit  : NULL; }
double      rb_param_min(int id)     { return param_ok(id) ? PARAMS[id].min   : NAN; }
double      rb_param_max(int id)     { return param_ok(id) ? PARAMS[id].max   : NAN; }
double      rb_param_default(int id) { return param_ok(id) ? PARAMS[id].def   : NAN; }
int         rb_param_flags(int id)   { return param_ok(id) ? PARAMS[id].flags : 0; }

int rb_param_find(const char *name)
{
    if (!name) return RB_ERR_ARG;
    for (int i = 0; i < RB_PARAM_COUNT; i++)
        if (PARAMS[i].name && strcmp(PARAMS[i].name, name) == 0)
            return i;
    return RB_ERR_ID;
}

int rb_set_param(rb_sim *sim, int id, double value)
{
    int rc = RB_OK;

    if (!sim)             return RB_ERR_ARG;
    if (!param_ok(id))    return RB_ERR_ID;
    if (!isfinite(value)) return RB_ERR_VALUE;   /* reject, keep the old value */
    if ((PARAMS[id].flags & RB_PF_INTEGER) && value != floor(value)) return RB_ERR_VALUE;

    if (value < PARAMS[id].min) { value = PARAMS[id].min; rc = RB_CLAMPED; }
    if (value > PARAMS[id].max) { value = PARAMS[id].max; rc = RB_CLAMPED; }

    sim->param[id] = value;
    const int applied = rb_sim_apply(sim, id);   /* into the engine and the sound */
    return applied == RB_PENDING ? RB_PENDING : rc;
}

double rb_get_param(const rb_sim *sim, int id)
{
    if (!sim || !param_ok(id)) return NAN;
    return sim->param[id];
}

int rb_telemetry_count(void)
{
    return RB_TLM_COUNT;
}

const char *rb_telemetry_name(int id)
{
    return (id >= 0 && id < RB_TLM_COUNT) ? TELEMETRY[id].name : NULL;
}

const char *rb_telemetry_unit(int id)
{
    return (id >= 0 && id < RB_TLM_COUNT) ? TELEMETRY[id].unit : NULL;
}
