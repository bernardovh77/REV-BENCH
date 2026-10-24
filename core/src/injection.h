#ifndef RB_INJECTION_H
#define RB_INJECTION_H
#include <stddef.h>
#include <stdint.h>

/* Internal instantaneous charge-fueling surrogate, not a resolved PFI/GDI
 * needle schedule or the public event ABI. One attempt at IVC; only positive
 * actual delivery can excite its collapsed injector source. Tick names the
 * engine interval [tick,tick+1); fraction is in [0,1). All quantities SI. */
enum { RB_INJ_DELIVERED, RB_INJ_OFF, RB_INJ_AFR_DISABLED, RB_INJ_DFCO,
       RB_INJ_NO_AIR, RB_INJ_RETAINED, RB_INJ_INVALID, RB_INJ_KINDS };
typedef struct rb_injection_event {
    uint64_t tick, charge;
    int cylinder, kind;
    double fraction, angle_deg;
    double retained_fuel_kg, target_fuel_kg, fuel_kg, sensible_energy_J;
} rb_injection_event;

#define RB_INJ_MAX_RECORDS 256
typedef struct rb_injection_trace {
    rb_injection_event *records;
    size_t capacity, count;
    uint64_t dropped, totals[RB_INJ_KINDS];
} rb_injection_trace;
int rb_injection_trace_init(rb_injection_trace *t, rb_injection_event *records, size_t capacity);
const char *rb_injection_kind_name(int kind);
void rb_injection_emit(rb_injection_trace *t, const rb_injection_event *event);
#endif
