#include "injection.h"
#include <string.h>

int rb_injection_trace_init(rb_injection_trace *t, rb_injection_event *records, size_t capacity)
{
    if (!t || capacity > RB_INJ_MAX_RECORDS || (capacity && !records)) return -1;
    memset(t, 0, sizeof *t);
    t->records=records; t->capacity=capacity;
    return 0;
}
const char *rb_injection_kind_name(int kind)
{
    static const char *const names[]={"delivered","ignition-off","afr-disabled",
        "dfco","no-air","retained-sufficient","invalid-state"};
    return kind>=0 && kind<RB_INJ_KINDS ? names[kind] : "unknown";
}
void rb_injection_emit(rb_injection_trace *t, const rb_injection_event *event)
{
    if (!t) return;
    if (event->kind>=0 && event->kind<RB_INJ_KINDS) t->totals[event->kind]++;
    if (t->count==t->capacity) { t->dropped++; return; }
    t->records[t->count++]=*event;
}
