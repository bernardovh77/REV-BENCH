/*
 * rb_events.c - the engine's actual events, for a host (stage 121).
 *
 * Recorded only while RB_PARAM_EVENTS is 1, so that a host that doesn't
 * want them pays nothing. Then the engine's own traces (ignition.h,
 * injection.h) collect each step's sparks and injections, and the sim adds
 * the valves' openings it sees the crank pass. After each step they go,
 * in the order they happened in it, into a ring of RB_EVENT_CAPACITY; a
 * host takes them out with rb_read_events. When the ring is full, a new
 * event is dropped and counted (rb_events_lost): its sequence number is
 * spent all the same, so the gap shows (contracts/runtime-v1.md).
 */
#include "rb_internal.h"

#include <math.h>
#include <string.h>

static const char *const KIND_NAMES[RB_EVENT_KIND_COUNT] = {
    [RB_EVENT_SPARK_COMMAND]   = "spark-command",
    [RB_EVENT_SPARK_DISCHARGE] = "spark-discharge",
    [RB_EVENT_FLAME_START]     = "flame-start",
    [RB_EVENT_FUEL_INJECTION]  = "fuel-injection",
    [RB_EVENT_INTAKE_OPEN]     = "intake-open",
    [RB_EVENT_EXHAUST_OPEN]    = "exhaust-open",
    [RB_EVENT_FUEL_CUT]        = "fuel-cut",
    [RB_EVENT_LIMITER_CUT]     = "limiter-cut",
};

const char *rb_event_kind_name(int kind)
{
    return (kind >= 0 && kind < RB_EVENT_KIND_COUNT) ? KIND_NAMES[kind] : NULL;
}

void rb_sim_events_setup(rb_sim *sim)
{
    rb_engine *e = &sim->engine;
    sim->event_head = sim->event_count = 0;
    sim->events_lost = sim->event_sequence = 0;
    if (sim->param[RB_PARAM_EVENTS] != 1.0) {
        e->ignition_trace = NULL;
        e->injection_trace = NULL;
        return;
    }
    rb_ignition_trace_init(&sim->ignition_trace, sim->ignition_records, RB_TRACE_RECORDS);
    rb_injection_trace_init(&sim->injection_trace, sim->injection_records, RB_TRACE_RECORDS);
    e->ignition_trace = &sim->ignition_trace;
    e->injection_trace = &sim->injection_trace;
    sim->event_theta = e->theta;
    sim->event_cycles = e->cycles;
    for (int i = 0; i < e->desc.cylinders; i++) sim->event_angle[i] = rb_engine_cylinder_angle(e, i);
}

/* One step's events before they're ordered: the event, less its sequence. */
typedef struct pending {
    double subtick;
    uint64_t tick, cycle_id;
    int kind, cylinder;
} pending;

/* At most: each cylinder's spark (command, discharge, flame or a cut), its
 * injection and its two valves in one step, with room to spare. */
#define MOST_PER_STEP (8 * RB_MAX_CYLINDERS)

/* Where in [0, 1) through the step an angle `at` lies, the cylinder having
 * turned from `from` by `step` degrees; -1 if it didn't pass it. */
static double passed(double from, double step, double at)
{
    double ahead = at - from;
    if (ahead < 0.0) ahead += 720.0;
    return (step > 0.0 && ahead < step) ? ahead / step : -1.0;
}

static void push(rb_sim *sim, const pending *p)
{
    const rb_engine *e = &sim->engine;
    /* the crank's angle then: between where it was and where it is */
    double turned = e->theta - sim->event_theta;
    if (e->cycles != sim->event_cycles) turned += 720.0;
    double angle = sim->event_theta + p->subtick * turned;
    uint64_t cycle = sim->event_cycles > 0 ? (uint64_t)sim->event_cycles : 0;
    if (angle >= 720.0) {
        angle -= 720.0;
        cycle++;
    }
    const uint64_t sequence = sim->event_sequence++;
    if (sim->event_count == RB_EVENT_CAPACITY) {   /* full: this one is lost */
        sim->events_lost++;
        return;
    }
    rb_event *v = &sim->event[(sim->event_head + sim->event_count) % RB_EVENT_CAPACITY];
    sim->event_count++;
    *v = (rb_event){
        .generation = sim->generation,
        .sequence = sequence,
        .physics_tick = p->tick,
        .subtick = p->subtick,
        .cycle_index = cycle,
        .phase_rad = angle * (RB_PI / 180.0),
        .cycle_id = p->cycle_id,
        .kind = p->kind,
        .cylinder = p->cylinder,
    };
}

/* A trace's fraction as a subtick: [0, 1), a NaN at the start. */
static double subtick_of(double fraction)
{
    if (!(fraction >= 0.0)) return 0.0;
    return fraction < 1.0 ? fraction : nextafter(1.0, 0.0);
}

void rb_sim_events_step(rb_sim *sim)
{
    rb_engine *e = &sim->engine;
    pending got[MOST_PER_STEP];
    int n = 0;
    const uint64_t tick = e->physics_tick - 1;   /* the step just taken */

    for (size_t k = 0; k < sim->ignition_trace.count && n < MOST_PER_STEP; k++) {
        const rb_ignition_event *r = &sim->ignition_records[k];
        int kind = -1;
        switch (r->kind) {
        case RB_IGN_COMMAND:   kind = RB_EVENT_SPARK_COMMAND; break;
        case RB_IGN_DISCHARGE: kind = RB_EVENT_SPARK_DISCHARGE; break;
        case RB_IGN_KERNEL:    kind = RB_EVENT_FLAME_START; break;
        case RB_IGN_HARD_CUT:
        case RB_IGN_SOFT_CUT:  kind = RB_EVENT_LIMITER_CUT; break;
        default: break;
        }
        if (kind >= 0)
            got[n++] = (pending){ subtick_of(r->fraction), r->tick, r->charge, kind, r->cylinder };
    }
    for (size_t k = 0; k < sim->injection_trace.count && n < MOST_PER_STEP; k++) {
        const rb_injection_event *r = &sim->injection_records[k];
        const int kind = r->kind == RB_INJ_DELIVERED ? RB_EVENT_FUEL_INJECTION
                       : r->kind == RB_INJ_DFCO ? RB_EVENT_FUEL_CUT : -1;
        if (kind >= 0)
            got[n++] = (pending){ subtick_of(r->fraction), r->tick, r->charge, kind, r->cylinder };
    }
    sim->ignition_trace.count = 0;
    sim->injection_trace.count = 0;

    /* the valves the crank passed: an intake opening fills the charge the
     * next IVC traps; an exhaust opening empties the one burning now */
    const double ivo = rb_valve_opens(&e->desc.intake), evo = rb_valve_opens(&e->desc.exhaust);
    for (int i = 0; i < e->desc.cylinders && n + 2 <= MOST_PER_STEP; i++) {
        const double now = rb_engine_cylinder_angle(e, i);
        double step = now - sim->event_angle[i];
        if (step < 0.0) step += 720.0;
        if (step < 90.0) {   /* a jump (a reset) isn't turning */
            const double at_ivo = passed(sim->event_angle[i], step, ivo);
            const double at_evo = passed(sim->event_angle[i], step, evo);
            const uint64_t charge = e->burn[i].charge_sequence;
            if (at_ivo >= 0.0) got[n++] = (pending){ at_ivo, tick, charge + 1, RB_EVENT_INTAKE_OPEN, i };
            if (at_evo >= 0.0) got[n++] = (pending){ at_evo, tick, charge, RB_EVENT_EXHAUST_OPEN, i };
        }
        sim->event_angle[i] = now;
    }

    /* in the order they happened: by tick, then subtick (insertion sort:
     * a handful at most) */
    for (int a = 1; a < n; a++) {
        const pending p = got[a];
        int b = a - 1;
        while (b >= 0 && (got[b].tick > p.tick || (got[b].tick == p.tick && got[b].subtick > p.subtick))) {
            got[b + 1] = got[b];
            b--;
        }
        got[b + 1] = p;
    }
    for (int a = 0; a < n; a++) push(sim, &got[a]);
    sim->event_theta = e->theta;
    sim->event_cycles = e->cycles;
}

int rb_read_events(rb_sim *sim, rb_event *out, int max)
{
    if (!sim || !out || max < 0) return RB_ERR_ARG;
    int n = 0;
    while (n < max && sim->event_count > 0) {
        out[n++] = sim->event[sim->event_head];
        sim->event_head = (sim->event_head + 1) % RB_EVENT_CAPACITY;
        sim->event_count--;
    }
    return n;
}

uint64_t rb_events_lost(const rb_sim *sim)
{
    return sim ? sim->events_lost : 0;
}
