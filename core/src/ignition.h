#ifndef RB_IGNITION_H
#define RB_IGNITION_H
#include <stddef.h>
#include <stdint.h>

/* Internal actual-source event tap, not the public snapshot/transport ABI.
 * tick is the engine-owned interval index, fraction in [0,1]. A charge ID is
 * local to a cylinder and advances at IVC. Read/drain only between steps. */

/* The kinds of event, in the order of a cycle: */
enum {
    RB_IGN_SCHEDULED,      /* the spark's angle is fixed (at IVC) */
    RB_IGN_COMMAND,        /* the crank reaches it */
    RB_IGN_DISCHARGE,      /* the plug fires */
    RB_IGN_KERNEL,         /* the flame starts, after its development delay */
    RB_IGN_OFF,            /* no spark: the ignition is off */
    RB_IGN_HARD_CUT,       /* no spark: the rev limiter's hard cut */
    RB_IGN_SOFT_CUT,       /* no spark: its soft cut */
    RB_IGN_NO_REACTANTS,   /* a spark, but nothing to burn */
    RB_IGN_EVO_PENDING,    /* the exhaust opened before the flame started */
    RB_IGN_EVO_PARTIAL,    /* ... or before it finished */
    RB_IGN_KINDS
};
#define RB_IGN_MAX_RECORDS 256

typedef struct rb_ignition_event {
    uint64_t tick, charge;
    int cylinder, kind;
    double fraction, angle_deg, kernel_raw_delay_deg, kernel_delay_deg, fuel_kg;
} rb_ignition_event;

typedef struct rb_ignition_trace {
    rb_ignition_event *records;          /* caller-owned, `capacity` long */
    size_t capacity, count;
    uint64_t dropped, totals[RB_IGN_KINDS];   /* totals count dropped ones too */
} rb_ignition_trace;

int rb_ignition_trace_init(rb_ignition_trace *t, rb_ignition_event *records, size_t capacity);
const char *rb_ignition_kind_name(int kind);
void rb_ignition_emit(rb_ignition_trace *t, const rb_ignition_event *event);
#endif
