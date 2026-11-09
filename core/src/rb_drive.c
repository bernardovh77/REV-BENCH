/*
 * rb_drive.c - driving the engine through the public API (stage 121).
 *
 * What a host (rb_cli sound, the desktop app) needs to run and watch the
 * engine without reaching inside the sim:
 *   - the driver's controls and the settings, as parameters (rb_params.c
 *     keeps their table; rb_sim_apply puts each into the engine);
 *   - the car on the crank in gear: its inertia, and the road's load,
 *     moved here from rb_cli (stage 12c's --sweep);
 *   - rebuilding the engine afresh (rb_sim_build), the one place every
 *     rebuild goes through;
 *   - the engine's telemetry.
 * Nothing here changes what the engine computes: a drive set up through
 * these calls renders bit for bit what rb_cli rendered when it set the
 * engine's fields itself (docs/stage-121.md).
 */
#include "rb_internal.h"

#include "combustion.h"
#include "pipe.h"
#include "fastmath.h"

#include <math.h>
#include <string.h>

_Static_assert((int)RB_LISTENER_TAILPIPE == (int)RB_HEAR_TAILPIPE
               && (int)RB_LISTENER_BEHIND == (int)RB_HEAR_BEHIND
               && (int)RB_LISTENER_SIDE == (int)RB_HEAR_SIDE
               && (int)RB_LISTENER_FRONT == (int)RB_HEAR_FRONT
               && (int)RB_LISTENER_CABIN == (int)RB_HEAR_CABIN
               && (int)RB_LISTENER_COUNT == (int)RB_HEAR_COUNT,
               "the public listener places are audio.h's");
_Static_assert((int)RB_BURN_MODEL_CHARGE == (int)RB_BURN_CHARGE
               && (int)RB_BURN_MODEL_CAPPED == (int)RB_BURN_CAPPED,
               "the public burn models are combustion.h's");

/* ---- the car on the crank ----
 * Stage 12c's car (rb_cli sound --sweep): its weight in the gear the pull is
 * in is the crank's inertia less the engine's own; with the wheels' radius
 * that gives the gearing, and so the road speed and the road's load on the
 * crank: rolling resistance and the air's drag. */
#define CAR_MASS     1300.0   /* kg                                              */
#define CAR_WHEEL    0.31     /* m: the driven wheels' rolling radius            */
#define CAR_ROLLING  0.012    /* rolling resistance coefficient                  */
#define CAR_CDA      0.70     /* m^2: drag coefficient times frontal area        */
#define CAR_AIR      1.2      /* kg/m^3                                          */

/* The road's load on the crank (N m) at crank speed w (rad/s), the car's
 * share of the crank's inertia `j_car` (kg m^2): none without a car. */
static double road_load(double w, double j_car)
{
    if (!(j_car > 0.0)) return 0.0;
    const double ratio = CAR_WHEEL * sqrt(CAR_MASS / j_car);   /* crank turns per wheel turn */
    const double v = w * CAR_WHEEL / ratio;
    return (CAR_ROLLING * CAR_MASS * 9.81 + 0.5 * CAR_AIR * CAR_CDA * v * v) * CAR_WHEEL / ratio;
}

/* 1 while the car is in gear on the crank: the clutch out and a car there. */
static int in_gear(const rb_sim *sim)
{
    return sim->param[RB_PARAM_CLUTCH] == 1.0 && sim->param[RB_PARAM_GEAR_INERTIA] > sim->own_inertia;
}

/* Puts the car on the crank or takes it off, as the clutch and the gear's
 * inertia say. The crank keeps its speed: its kinetic energy is that of
 * what it turns now. */
static void place_car(rb_sim *sim)
{
    rb_engine *e = &sim->engine;
    const double inertia = in_gear(sim) ? sim->param[RB_PARAM_GEAR_INERTIA] : sim->own_inertia;
    if (e->desc.inertia == inertia) return;
    e->desc.inertia = inertia;
    e->kinetic = 0.5 * inertia * e->omega * e->omega;
}

/* The load on the free crank: the driver's, plus the road's in gear (a dyno
 * sets its own). Called when a control changes and before each rendered
 * block, as rb_cli did. */
static void apply_load(rb_sim *sim)
{
    rb_engine *e = &sim->engine;
    if (sim->dyno) return;
    if (!in_gear(sim)) {
        e->load_torque = sim->param[RB_PARAM_LOAD_TORQUE];
        return;
    }
    if (!e->hold_speed)   /* a held crank's speed isn't the road's */
        e->load_torque = sim->param[RB_PARAM_LOAD_TORQUE]
                       + road_load(e->omega, sim->param[RB_PARAM_GEAR_INERTIA] - sim->own_inertia);
}

void rb_sim_before_block(rb_sim *sim)
{
    if (sim->source == RB_SOURCE_ENGINE && in_gear(sim)) apply_load(sim);
}

/* ---- the parameters' effects ---- */

/* Whether the engine has run since it was built: settings that only a
 * build can take wait for the next one then. */
static int has_run(const rb_sim *sim)
{
    return sim->engine.physics_tick != 0;
}

/* The build-time settings: the burn model, the variation's seed, the pipes'
 * momentum flux. Only at physics tick 0. */
static void apply_build_settings(rb_sim *sim)
{
    rb_engine *e = &sim->engine;
    rb_engine_combustion_model(e, (int)sim->param[RB_PARAM_BURN_MODEL]);
    if (sim->param[RB_PARAM_SEED] > 0.0) rb_engine_seed(e, (uint64_t)sim->param[RB_PARAM_SEED]);
    const int order = (int)sim->param[RB_PARAM_MOMENTUM_ORDER];
    for (int k = 0; k < rb_engine_pipe_count(e); k++)
        ((rb_pipe *)rb_engine_pipe(e, k))->momentum_order = order;
}

int rb_sim_apply(rb_sim *sim, int id)
{
    rb_engine *e = &sim->engine;
    const double v = sim->param[id];
    switch (id) {
    case RB_PARAM_SOURCE:     sim->source = (int)v; break;
    case RB_PARAM_IGNITION:   e->ignition = (int)v; break;
    case RB_PARAM_STARTER:    e->starter = (int)v; break;
    case RB_PARAM_THROTTLE:   e->throttle_pos = v; break;
    case RB_PARAM_LOAD_TORQUE: apply_load(sim); break;
    case RB_PARAM_HOLD:
        if (v == 1.0 && sim->dyno) sim->dyno = NULL;   /* held by the motor instead */
        e->hold_speed = (int)v;
        if (e->hold_speed) e->rpm = sim->param[RB_PARAM_HOLD_RPM];
        apply_load(sim);
        break;
    case RB_PARAM_HOLD_RPM:
        if (e->hold_speed) e->rpm = v;
        break;
    case RB_PARAM_GEAR_INERTIA:
    case RB_PARAM_CLUTCH:
        place_car(sim);
        apply_load(sim);
        break;
    case RB_PARAM_VARIATION:  e->variation = v; break;
    case RB_PARAM_OVERRUN_TRANSITION: e->overrun_transition = v; break;
    case RB_PARAM_LISTENER:
        sim->hear = (int)v;
        rb_sim_listen(sim);
        break;
    case RB_PARAM_LAYERS:
        sim->layers = (int)v;
        rb_sim_listen(sim);
        break;
    case RB_PARAM_BURN_MODEL:
    case RB_PARAM_SEED:
    case RB_PARAM_MOMENTUM_ORDER:
        if (has_run(sim)) return RB_PENDING;
        apply_build_settings(sim);
        break;
    case RB_PARAM_EVENTS:
        rb_sim_events_setup(sim);
        break;
    default: break;   /* the gain and the test tone: read where they're used */
    }
    return RB_OK;
}

/* ---- rebuilding ---- */

rb_engine_desc rb_sim_desc(const rb_sim *sim)
{
    rb_engine_desc d = sim->engine.desc;
    if (sim->generation) d.inertia = sim->own_inertia;   /* without the car */
    return d;
}

int rb_sim_build(rb_sim *sim, const rb_engine_desc *d)
{
    if (rb_engine_init_at(&sim->engine, d, sim->dt, sim->cfg.pipe_rate) != 0) return -1;
    sim->own_inertia = d->inertia;
    sim->dyno = NULL;
    sim->generation++;
    /* the driver's controls start from rest (contracts/runtime-v1.md) */
    for (int id = 0; id < RB_PARAM_COUNT; id++)
        if (rb_param_flags(id) & RB_PF_CONTROL) sim->param[id] = rb_param_default(id);
    apply_build_settings(sim);
    for (int id = 0; id < RB_PARAM_COUNT; id++)
        if (id != RB_PARAM_SOURCE && id != RB_PARAM_LISTENER && id != RB_PARAM_LAYERS
            && id != RB_PARAM_EVENTS
            && !(rb_param_flags(id) & RB_PF_REBUILD))
            rb_sim_apply(sim, id);
    sim->hear = (int)sim->param[RB_PARAM_LISTENER];
    sim->layers = (int)sim->param[RB_PARAM_LAYERS];
    rb_sim_events_setup(sim);
    rb_sim_listen(sim);
    return 0;
}

int rb_set_quality(rb_sim *sim, int quality)
{
    if (!sim) return RB_ERR_ARG;
    rb_config c = sim->cfg;
    if (rb_config_quality(&c, quality) != RB_OK) return RB_ERR_ARG;
    return rb_sim_reconfigure(sim, &c);
}

int rb_get_quality(const rb_sim *sim)
{
    if (!sim) return -1;
    for (int q = 0; q < RB_QUALITY_COUNT; q++) {
        rb_config c = sim->cfg;
        if (rb_config_quality(&c, q) == RB_OK && c.oversample == sim->cfg.oversample
            && c.pipe_rate == sim->cfg.pipe_rate)
            return q;
    }
    return -1;
}

uint64_t rb_variation_seed(const rb_sim *sim)
{
    return sim ? sim->engine.variation_seed : 0;
}

double rb_listener_source_distance(const rb_sim *sim, int source)
{
    if (!sim || source < 0 || source >= RB_LISTEN_SOURCES) return NAN;
    return rb_listener_distance(&sim->listener, source);
}

int rb_set_scene_time(rb_sim *sim, double seconds)
{
    if (!sim || !sim->scene_active || !rb_finite(seconds)) return RB_ERR_ARG;
    sim->scene.time = seconds;
    return RB_OK;
}

/* ---- the engine's telemetry ---- */

double rb_sim_engine_telemetry(const rb_sim *sim, int id)
{
    const rb_engine *e = &sim->engine;
    const rb_engine_cycle *r = &e->result;
    switch (id) {
    case RB_TLM_RPM:           return e->rpm;
    case RB_TLM_RPM_SMOOTH:    return e->rpm_smooth;
    case RB_TLM_CYCLE_RPM:     return r->rpm;
    case RB_TLM_MAP:           return e->plenum.p;
    case RB_TLM_TORQUE:        return r->brake_torque;
    case RB_TLM_POWER:         return r->brake_power;
    case RB_TLM_LAMBDA:        return r->afr / RB_AFR_STOICH;
    case RB_TLM_SPARK:         return r->spark;
    case RB_TLM_IDLE_VALVE:    return e->idle_valve;
    case RB_TLM_THROTTLE:      return e->throttle_pos;
    case RB_TLM_THROTTLE_AREA: return e->throttle.area;
    case RB_TLM_IGNITION:      return e->ignition;
    case RB_TLM_STARTER:       return e->starter;
    case RB_TLM_HELD:          return e->hold_speed;
    case RB_TLM_FUEL_CUT:      return e->fuel_cut;
    case RB_TLM_SPARK_CUT:     return e->spark_cut;
    case RB_TLM_LIMITER_CUTS:  return (double)e->limiter_cuts;
    case RB_TLM_OVERRUN_TIME:  return e->overrun_time;
    case RB_TLM_LOAD_TORQUE:   return e->load_torque;
    case RB_TLM_CRANK_INERTIA: return e->desc.inertia;
    case RB_TLM_AIR_FLOW:      return r->air_flow;
    case RB_TLM_FUEL_FLOW:     return r->fuel_flow;
    case RB_TLM_EXHAUST_T:     return r->t_exhaust;
    case RB_TLM_TAILPIPE_T:    return r->t_tailpipe;
    default:                   return NAN;
    }
}

/* ---- the dyno the sim owns ---- */

int rb_dyno_attach(rb_sim *sim, double rpm)
{
    if (!sim) return RB_ERR_ARG;
    if (!(rpm >= 300.0 && rpm <= 20000.0)) return RB_ERR_VALUE;
    rb_dyno_init(&sim->own_dyno, &sim->engine, rpm);   /* frees the crank */
    sim->dyno = &sim->own_dyno;
    sim->param[RB_PARAM_HOLD] = 0.0;
    return RB_OK;
}

int rb_dyno_release(rb_sim *sim)
{
    if (!sim) return RB_ERR_ARG;
    sim->dyno = NULL;
    apply_load(sim);   /* the driver's load (and the road's) again */
    return RB_OK;
}

int rb_dyno_attached(const rb_sim *sim)
{
    return sim && sim->dyno != NULL;
}

int rb_dyno_set_sweep(rb_sim *sim, double to_rpm, double rate)
{
    if (!sim || !sim->dyno) return RB_ERR_ARG;
    if (!(to_rpm >= 300.0 && to_rpm <= 20000.0) || !(rate >= 0.0 && rate <= 1e5)) return RB_ERR_VALUE;
    rb_dyno_sweep(sim->dyno, to_rpm, rate);
    return RB_OK;
}

int rb_dyno_clear(rb_sim *sim)
{
    if (!sim || !sim->dyno) return RB_ERR_ARG;
    sim->dyno->readings = 0;
    memset(&sim->dyno->reading, 0, sizeof sim->dyno->reading);
    return RB_OK;
}

double rb_get_dyno(const rb_sim *sim, int field)
{
    if (!sim || !sim->dyno) return NAN;
    const rb_dyno *y = sim->dyno;
    switch (field) {
    case RB_DYNO_READINGS:        return (double)y->readings;
    case RB_DYNO_TARGET_RPM:      return y->target_rpm;
    case RB_DYNO_RPM:             return y->reading.rpm;
    case RB_DYNO_ABSORBED_TORQUE: return y->reading.absorbed_torque;
    case RB_DYNO_BRAKE_TORQUE:    return y->reading.brake_torque;
    case RB_DYNO_BRAKE_POWER:     return y->reading.brake_power;
    default:                      return NAN;
    }
}

/* ---- snapshots (contracts/snapshot-v2.md) ---- */

_Static_assert(sizeof(rb_snapshot_header) == 112, "snapshot-v2's header is 112 bytes");
_Static_assert(RB_SNAPSHOT_INSTRUMENTS == 24, "snapshot-v2 has 24 instruments");

/* The engine description's identity: a 64-bit FNV-1a hash of every field's
 * value, as rb_get_engine gives it (the bits of each double), so the same
 * engine always has the same id, on every platform. */
static uint64_t build_id(const rb_sim *sim)
{
    uint64_t h = 0xcbf29ce484222325ULL;
    for (int f = 0; f < RB_ENG_COUNT; f++)
        for (int i = 0; i < rb_engine_field_size(sim, f); i++) {
            const double v = rb_get_engine(sim, f, i);
            uint64_t bits;
            memcpy(&bits, &v, sizeof bits);
            for (int b = 0; b < 8; b++) {
                h ^= (bits >> (8 * b)) & 0xff;
                h *= 0x100000001b3ULL;
            }
        }
    return h;
}

int rb_snapshot_size(const rb_sim *sim)
{
    if (!sim) return 0;
    return (int)(sizeof(rb_snapshot_header) + sizeof(double) * RB_SNAPSHOT_INSTRUMENTS
                 + sizeof(double) * RB_SNAPSHOT_CYLINDER_FIELDS * sim->engine.desc.cylinders);
}

int rb_get_snapshot(rb_sim *sim, int version, void *buffer, int size)
{
    if (!sim || !buffer) return RB_ERR_ARG;
    if (version != RB_SNAPSHOT_VERSION) return RB_ERR_ID;
    const int need = rb_snapshot_size(sim);
    if (size < need) return RB_ERR_SIZE;
    const rb_engine *e = &sim->engine;
    rb_snapshot_header h = {
        .magic = RB_SNAPSHOT_MAGIC,
        .version = RB_SNAPSHOT_VERSION,
        .size = (uint32_t)need,
        .cylinders = (uint32_t)e->desc.cylinders,
        .instruments = RB_SNAPSHOT_INSTRUMENTS,
        .cylinder_fields = RB_SNAPSHOT_CYLINDER_FIELDS,
        .build_id = build_id(sim),
        .generation = sim->generation,
        .sequence = ++sim->snapshot_sequence,
        .physics_tick = e->physics_tick,
        .audio_frame = sim->frames,
        .cycle_index = e->cycles > 0 ? (uint64_t)e->cycles : 0,
        .physics_hz = 1.0 / sim->dt,
        .audio_hz = sim->cfg.sample_rate,
        .phase_rad = e->theta * (RB_PI / 180.0),
        .speed_rad_s = e->omega > 0.0 ? e->omega : 0.0,
    };
    char *out = buffer;
    memcpy(out, &h, sizeof h);
    out += sizeof h;
    for (int k = 0; k < RB_SNAPSHOT_INSTRUMENTS; k++) {   /* the telemetry, in order */
        const double v = rb_sim_engine_telemetry(sim, RB_TLM_RPM + k);
        memcpy(out, &v, sizeof v);
        out += sizeof v;
    }
    static const int FIELDS[RB_SNAPSHOT_CYLINDER_FIELDS] = {
        RB_CYL_PRESSURE, RB_CYL_TEMPERATURE, RB_CYL_BURN, RB_CYL_INTAKE_LIFT, RB_CYL_EXHAUST_LIFT
    };
    for (int c = 0; c < e->desc.cylinders; c++)
        for (int k = 0; k < RB_SNAPSHOT_CYLINDER_FIELDS; k++) {
            const double v = rb_get_cylinder(sim, c, FIELDS[k]);
            memcpy(out, &v, sizeof v);
            out += sizeof v;
        }
    return need;
}
