/* ignition.c - the spark's events, traced (stage 040).
 *
 * The engine emits one event per spark decision: scheduled, commanded,
 * discharged, the kernel starting, or why it didn't (ignition off, a limiter
 * cut, no reactants, the exhaust valve opening first). A trace keeps them in
 * a caller-owned array and counts every kind, kept or not. */
#include "ignition.h"
#include <string.h>

int rb_ignition_trace_init(rb_ignition_trace *t, rb_ignition_event *records, size_t capacity)
{
    if (!t || capacity > RB_IGN_MAX_RECORDS || (capacity && !records)) return -1;
    memset(t, 0, sizeof *t);
    t->records = records;
    t->capacity = capacity;
    return 0;
}

/* The name of an event kind, as the probes and their CSVs print it. */
const char *rb_ignition_kind_name(int k)
{
    static const char *const names[] = {"scheduled", "command", "discharge", "kernel",
        "ignition-off", "hard-cut", "soft-cut", "no-reactants", "pending-at-evo", "partial-at-evo"};
    return k >= 0 && k < RB_IGN_KINDS ? names[k] : "unknown";
}

/* Counts the event, and keeps it while the array has room (else counts it
 * as dropped). A NULL trace takes nothing. */
void rb_ignition_emit(rb_ignition_trace *t, const rb_ignition_event *event)
{
    if (!t) return;
    if (event->kind >= 0 && event->kind < RB_IGN_KINDS) t->totals[event->kind]++;
    if (t->count == t->capacity) {
        t->dropped++;
        return;
    }
    t->records[t->count++] = *event;
}
