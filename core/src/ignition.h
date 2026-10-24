#ifndef RB_IGNITION_H
#define RB_IGNITION_H
#include <stddef.h>
#include <stdint.h>
/* Internal actual-source event tap, not the public snapshot/transport ABI.
 * tick is the engine-owned interval index, fraction in [0,1]. A charge ID is
 * local to a cylinder and advances at IVC. Read/drain only between steps. */
enum { RB_IGN_SCHEDULED, RB_IGN_COMMAND, RB_IGN_DISCHARGE, RB_IGN_KERNEL,
       RB_IGN_OFF, RB_IGN_HARD_CUT, RB_IGN_SOFT_CUT, RB_IGN_NO_REACTANTS,
       RB_IGN_EVO_PENDING, RB_IGN_EVO_PARTIAL, RB_IGN_KINDS };
#define RB_IGN_MAX_RECORDS 256
typedef struct rb_ignition_event {
    uint64_t tick, charge;
    int cylinder, kind;
    double fraction, angle_deg, kernel_raw_delay_deg, kernel_delay_deg, fuel_kg;
} rb_ignition_event;
typedef struct rb_ignition_trace {
    rb_ignition_event *records;
    size_t capacity, count;
    uint64_t dropped, totals[RB_IGN_KINDS];
} rb_ignition_trace;
int rb_ignition_trace_init(rb_ignition_trace *t,rb_ignition_event *records,size_t capacity);
const char *rb_ignition_kind_name(int kind);
void rb_ignition_emit(rb_ignition_trace *t,const rb_ignition_event *event);
#endif
