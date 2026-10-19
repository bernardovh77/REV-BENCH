/*
 * rb_params.c - the parameter and telemetry tables.
 *
 * To add a parameter:
 *   1. add an id to rb_param in rb.h (just before RB_PARAM_COUNT)
 *   2. add its row to PARAMS below
 *   3. read sim->param[id] in the model
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

static const param_info PARAMS[RB_PARAM_COUNT] = {
    /*                             name               unit  min    max      default flags      */
    [RB_PARAM_MASTER_GAIN]     = { "master_gain",     "",   0.0,   4.0,     1.0,    RB_PF_LIVE },
    [RB_PARAM_TEST_TONE_HZ]    = { "test_tone_hz",    "Hz", 20.0,  20000.0, 220.0,  RB_PF_LIVE },
    [RB_PARAM_TEST_TONE_LEVEL] = { "test_tone_level", "",   0.0,   1.0,     0.5,    RB_PF_LIVE },
};

static const char *const TELEMETRY_NAMES[RB_TLM_COUNT] = {
    [RB_TLM_TIME_S]    = "time_s",
    [RB_TLM_STEPS]     = "steps",
    [RB_TLM_OUT_PEAK]  = "out_peak",
    [RB_TLM_OUT_RMS]   = "out_rms",
    [RB_TLM_NONFINITE] = "nonfinite",
    [RB_TLM_LIMITED]   = "limited",
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

    if (value < PARAMS[id].min) { value = PARAMS[id].min; rc = RB_CLAMPED; }
    if (value > PARAMS[id].max) { value = PARAMS[id].max; rc = RB_CLAMPED; }

    sim->param[id] = value;
    return rc;
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
    return (id >= 0 && id < RB_TLM_COUNT) ? TELEMETRY_NAMES[id] : NULL;
}
