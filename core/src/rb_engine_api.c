/*
 * rb_engine_api.c - the engine through the public API (stage 8): loading a
 * preset, the engine's description field by field, and each cylinder's
 * state (see "the engine" in rb.h).
 *
 * The fields are a table, like the parameters in rb_params.c: most are a
 * number stored in rb_engine_desc, a few are worked out from it. To add one:
 *   1. add an id to rb_engine_field in rb.h (just before RB_ENG_COUNT)
 *   2. add its row to FIELDS below (and a case to worked_out() if it's
 *      worked out)
 * The unit tests check that every id has a complete row.
 */
#include "rb_internal.h"

#include "presets.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

enum { STORED_DOUBLE, STORED_INT, WORKED_OUT };

typedef struct field_info {
    const char *name;     /* stable identifier for the UI and the CLI     */
    const char *unit;     /* "" when it has none                          */
    int         flags;    /* RB_EF_* bits                                 */
    double      min, max; /* the range it can be set in (NaN: read-only)  */
    int         kind;     /* STORED_DOUBLE, STORED_INT or WORKED_OUT      */
    size_t      offset;   /* where a stored field is in rb_engine_desc    */
} field_info;

#define DBL(member) STORED_DOUBLE, offsetof(rb_engine_desc, member)
#define INT(member) STORED_INT, offsetof(rb_engine_desc, member)
#define CALC        WORKED_OUT, 0
#define NONE        NAN, NAN

enum {
    RO = RB_EF_READONLY, WHOLE = RB_EF_INTEGER, BANK = RB_EF_PER_BANK,
    CYL = RB_EF_PER_CYLINDER, ORDER = RB_EF_IN_ORDER, LIVE = RB_EF_LIVE
};

/* The ranges are what a control should offer: wide enough for any car or
 * motorcycle engine, narrow enough that nothing silly gets in. Whether the
 * whole engine still works (the rod longer than the stroke, the valves
 * fitting in the bore, ...) is checked on top of them. */
static const field_info FIELDS[RB_ENG_COUNT] = {
    /*                               name                 unit      flags          min      max      stored as */
    [RB_ENG_CYLINDERS]         = { "cylinders",         "",       RO | WHOLE,    NONE,             INT(cylinders) },
    [RB_ENG_BORE]              = { "bore",              "m",      0,             0.04,    0.15,    DBL(bore) },
    [RB_ENG_STROKE]            = { "stroke",            "m",      0,             0.04,    0.15,    DBL(stroke) },
    [RB_ENG_ROD]               = { "rod",               "m",      0,             0.08,    0.30,    DBL(rod) },
    [RB_ENG_COMPRESSION_RATIO] = { "compression_ratio", "",       0,             6.0,     16.0,    DBL(cr) },
    [RB_ENG_DISPLACEMENT]      = { "displacement",      "m^3",    RO,            NONE,             CALC },
    [RB_ENG_BANKS]             = { "banks",             "",       RO | WHOLE,    NONE,             INT(banks) },
    [RB_ENG_BANK_ANGLE]        = { "bank_angle",        "deg",    BANK,          -180.0,  180.0,   DBL(bank_angle) },
    [RB_ENG_BANK]              = { "bank",              "",       CYL | WHOLE,   0.0,     1.0,     INT(bank) },
    [RB_ENG_PIN]               = { "pin",               "deg",    CYL,           0.0,     360.0,   DBL(pin) },
    [RB_ENG_POSITION]          = { "position",          "m",      CYL,           -1.0,    3.0,     DBL(position) },
    [RB_ENG_FIRING_ORDER]      = { "firing_order",      "",       ORDER | WHOLE, 0.0,     RB_MAX_CYLINDERS - 1.0, INT(firing_order) },
    [RB_ENG_FIRING_ANGLE]      = { "firing_angle",      "deg",    CYL | RO,      NONE,             DBL(firing) },
    [RB_ENG_INTAKE_VALVES]     = { "intake_valves",     "",       WHOLE,         1.0,     4.0,     INT(intake.count) },
    [RB_ENG_INTAKE_DIAMETER]   = { "intake_diameter",   "m",      0,             0.015,   0.06,    DBL(intake.diameter) },
    [RB_ENG_INTAKE_PORT]       = { "intake_port",       "m",      0,             0.01,    0.06,    DBL(intake.port_diameter) },
    [RB_ENG_INTAKE_LIFT]       = { "intake_lift",       "m",      0,             0.003,   0.02,    DBL(intake.max_lift) },
    [RB_ENG_INTAKE_DURATION]   = { "intake_duration",   "deg",    0,             160.0,   360.0,   DBL(intake.duration) },
    [RB_ENG_INTAKE_CENTER]     = { "intake_center",     "deg",    0,             400.0,   540.0,   DBL(intake.center) },
    [RB_ENG_INTAKE_OPENS]      = { "intake_opens",      "deg",    RO,            NONE,             CALC },
    [RB_ENG_INTAKE_CLOSES]     = { "intake_closes",     "deg",    RO,            NONE,             CALC },
    [RB_ENG_EXHAUST_VALVES]    = { "exhaust_valves",    "",       WHOLE,         1.0,     4.0,     INT(exhaust.count) },
    [RB_ENG_EXHAUST_DIAMETER]  = { "exhaust_diameter",  "m",      0,             0.015,   0.06,    DBL(exhaust.diameter) },
    [RB_ENG_EXHAUST_PORT]      = { "exhaust_port",      "m",      0,             0.01,    0.06,    DBL(exhaust.port_diameter) },
    [RB_ENG_EXHAUST_LIFT]      = { "exhaust_lift",      "m",      0,             0.003,   0.02,    DBL(exhaust.max_lift) },
    [RB_ENG_EXHAUST_DURATION]  = { "exhaust_duration",  "deg",    0,             160.0,   360.0,   DBL(exhaust.duration) },
    [RB_ENG_EXHAUST_CENTER]    = { "exhaust_center",    "deg",    0,             180.0,   320.0,   DBL(exhaust.center) },
    [RB_ENG_EXHAUST_OPENS]     = { "exhaust_opens",     "deg",    RO,            NONE,             CALC },
    [RB_ENG_EXHAUST_CLOSES]    = { "exhaust_closes",    "deg",    RO,            NONE,             CALC },
    [RB_ENG_THROTTLE_DIAMETER] = { "throttle_diameter", "m",      0,             0.03,    0.12,    DBL(throttle.diameter) },
    [RB_ENG_PLENUM_VOLUME]     = { "plenum_volume",     "m^3",    0,             0.0005,  0.02,    DBL(plenum_volume) },
    [RB_ENG_INERTIA]           = { "inertia",           "kg m^2", 0,             0.03,    1.0,     DBL(inertia) },
    [RB_ENG_IDLE_RPM]          = { "idle_rpm",          "rpm",    LIVE,          0.0,     2000.0,  DBL(idle_rpm) },
    [RB_ENG_REV_LIMIT]         = { "rev_limit",         "rpm",    LIVE,          2000.0,  12000.0, DBL(rev_limit) },
    [RB_ENG_RUNNER_LENGTH]     = { "runner_length",     "m",      0,             0.05,    0.8,     DBL(runner_length) },
    [RB_ENG_RUNNER_DIAMETER]   = { "runner_diameter",   "m",      0,             0.02,    0.08,    DBL(runner_diameter) },
};

typedef struct cylinder_field_info {
    const char *name;
    const char *unit;
} cylinder_field_info;

static const cylinder_field_info CYLINDER_FIELDS[RB_CYL_COUNT] = {
    [RB_CYL_ANGLE]        = { "angle",        "deg" },
    [RB_CYL_PISTON]       = { "piston",       "m" },
    [RB_CYL_PRESSURE]     = { "pressure",     "Pa" },
    [RB_CYL_TEMPERATURE]  = { "temperature",  "K" },
    [RB_CYL_BURN]         = { "burn",         "" },
    [RB_CYL_BURNED_GAS]   = { "burned_gas",   "" },
    [RB_CYL_INTAKE_LIFT]  = { "intake_lift",  "m" },
    [RB_CYL_EXHAUST_LIFT] = { "exhaust_lift", "m" },
};

/* ---- presets ---- */

int rb_load_preset(rb_sim *sim, int preset)
{
    rb_engine_desc d;
    if (!sim) return RB_ERR_ARG;
    if (rb_preset_desc(preset, &d) != 0) return RB_ERR_ID;
    rb_engine_init(&sim->engine, &d, sim->dt);   /* presets are possible engines */
    sim->preset = preset;
    return RB_OK;
}

int rb_get_preset(const rb_sim *sim)
{
    return sim ? sim->preset : RB_ERR_ARG;
}

/* ---- the description ---- */

static int field_ok(int field)
{
    return field >= 0 && field < RB_ENG_COUNT;
}

int         rb_engine_field_count(void)      { return RB_ENG_COUNT; }
const char *rb_engine_field_name(int field)  { return field_ok(field) ? FIELDS[field].name  : NULL; }
const char *rb_engine_field_unit(int field)  { return field_ok(field) ? FIELDS[field].unit  : NULL; }
int         rb_engine_field_flags(int field) { return field_ok(field) ? FIELDS[field].flags : 0; }
double      rb_engine_field_min(int field)   { return field_ok(field) ? FIELDS[field].min   : NAN; }
double      rb_engine_field_max(int field)   { return field_ok(field) ? FIELDS[field].max   : NAN; }

int rb_engine_field_find(const char *name)
{
    if (!name) return RB_ERR_ARG;
    for (int i = 0; i < RB_ENG_COUNT; i++)
        if (FIELDS[i].name && strcmp(FIELDS[i].name, name) == 0)
            return i;
    return RB_ERR_ID;
}

int rb_engine_field_size(const rb_sim *sim, int field)
{
    if (!sim || !field_ok(field)) return 0;
    const int flags = FIELDS[field].flags;
    if (flags & RB_EF_PER_BANK) return sim->engine.desc.banks;
    if (flags & (RB_EF_PER_CYLINDER | RB_EF_IN_ORDER)) return sim->engine.desc.cylinders;
    return 1;
}

static double worked_out(const rb_engine *e, int field)
{
    switch (field) {
    case RB_ENG_DISPLACEMENT:   return rb_engine_displacement(e);
    case RB_ENG_INTAKE_OPENS:   return rb_valve_opens(&e->desc.intake);
    case RB_ENG_INTAKE_CLOSES:  return rb_valve_closes(&e->desc.intake);
    case RB_ENG_EXHAUST_OPENS:  return rb_valve_opens(&e->desc.exhaust);
    case RB_ENG_EXHAUST_CLOSES: return rb_valve_closes(&e->desc.exhaust);
    default:                    return NAN;
    }
}

double rb_get_engine(const rb_sim *sim, int field, int index)
{
    /* rb_engine_field_size is 0 for a NULL sim or an unknown field. */
    if (index < 0 || index >= rb_engine_field_size(sim, field)) return NAN;
    const field_info *f = &FIELDS[field];
    const char *at = (const char *)&sim->engine.desc + f->offset;
    switch (f->kind) {
    case STORED_DOUBLE: return ((const double *)at)[index];
    case STORED_INT:    return ((const int *)at)[index];
    default:            return worked_out(&sim->engine, field);
    }
}

int rb_set_engine(rb_sim *sim, int field, int index, double value)
{
    if (!sim) return RB_ERR_ARG;
    if (!field_ok(field)) return RB_ERR_ID;
    if (index < 0 || index >= rb_engine_field_size(sim, field)) return RB_ERR_ARG;
    const field_info *f = &FIELDS[field];
    if (f->flags & RB_EF_READONLY) return RB_ERR_READONLY;
    if (!isfinite(value)) return RB_ERR_VALUE;
    if ((f->flags & RB_EF_INTEGER) && value != floor(value)) return RB_ERR_VALUE;

    /* Angles around the crank wrap; everything else is clamped into its range. */
    int rc = RB_OK;
    if (field == RB_ENG_PIN) {                  /* into [0, 360) */
        value = fmod(value, 360.0);
        if (value < 0.0) value += 360.0;
        if (value >= 360.0) value -= 360.0;     /* -1e-17 + 360 rounds to 360 */
        value += 0.0;                           /* -0 becomes 0 */
    } else if (field == RB_ENG_BANK_ANGLE) {    /* into [-180, 180] */
        value = fmod(value, 360.0);
        if (value > 180.0) value -= 360.0;
        else if (value < -180.0) value += 360.0;
        value += 0.0;
    } else if (value < f->min || value > f->max) {
        value = (value < f->min) ? f->min : f->max;
        rc = RB_CLAMPED;
    }

    /* Change a copy, then keep it only if it makes a possible engine. */
    rb_engine_desc d = sim->engine.desc;
    char *at = (char *)&d + f->offset;
    if (f->kind == STORED_INT) {
        int *slot = (int *)at;
        const int v = (int)value;
        if (field == RB_ENG_FIRING_ORDER) {   /* swap places with cylinder v */
            if (v < 0 || v >= d.cylinders) return RB_ERR_VALUE;
            for (int k = 0; k < d.cylinders; k++)
                if (slot[k] == v) slot[k] = slot[index];
        }
        slot[index] = v;
    } else {
        ((double *)at)[index] = value;
        if (field == RB_ENG_REV_LIMIT) d.rev_limit_resume += value - sim->engine.desc.rev_limit;
    }
    if (rb_engine_layout(&d) != 0 || rb_engine_desc_check(&d) != 0) return RB_ERR_VALUE;

    if (memcmp(&d, &sim->engine.desc, sizeof d) == 0) return rc;   /* the same engine */
    if (f->flags & RB_EF_LIVE) sim->engine.desc = d;
    else rb_engine_init(&sim->engine, &d, sim->dt);
    return rc;
}

/* ---- each cylinder's state ---- */

int rb_cylinder_field_count(void)
{
    return RB_CYL_COUNT;
}

const char *rb_cylinder_field_name(int field)
{
    return (field >= 0 && field < RB_CYL_COUNT) ? CYLINDER_FIELDS[field].name : NULL;
}

const char *rb_cylinder_field_unit(int field)
{
    return (field >= 0 && field < RB_CYL_COUNT) ? CYLINDER_FIELDS[field].unit : NULL;
}

double rb_get_cylinder(const rb_sim *sim, int cylinder, int field)
{
    if (!sim || cylinder < 0 || cylinder >= sim->engine.desc.cylinders) return NAN;
    const rb_engine *e = &sim->engine;
    const rb_cylinder *c = &e->cyl[cylinder];
    const rb_burn *b = &e->burn[cylinder];
    const double angle = rb_engine_cylinder_angle(e, cylinder);
    switch (field) {
    case RB_CYL_ANGLE:        return angle;
    case RB_CYL_PISTON:       return rb_cyl_piston_x(&c->geom, c->theta);
    case RB_CYL_PRESSURE:     return c->gas.p;
    case RB_CYL_TEMPERATURE:  return c->gas.T;
    case RB_CYL_BURN:         return b->after_spark ? b->fraction : 0.0;
    case RB_CYL_BURNED_GAS:   return c->gas.y[RB_BURNED];
    case RB_CYL_INTAKE_LIFT:  return rb_valve_lift(&e->desc.intake, angle);
    case RB_CYL_EXHAUST_LIFT: return rb_valve_lift(&e->desc.exhaust, angle);
    default:                  return NAN;
    }
}
