/*
 * evring.c - the event ring: see evring.h.
 */
#include "evring.h"

#include <string.h>

int rbh_evring_init(rbh_evring *e, uint32_t capacity)
{
    memset(e, 0, sizeof *e);
    atomic_init(&e->lost, 0);
    return rbh_queue_init(&e->queue, capacity, sizeof(rb_event));
}

void rbh_evring_free(rbh_evring *e)
{
    rbh_queue_free(&e->queue);
}

int rbh_evring_push(rbh_evring *e, const rb_event *event)
{
    if (rbh_queue_push(&e->queue, event) == 0) return 1;
    atomic_fetch_add_explicit(&e->lost, 1, memory_order_release);
    return 0;
}

void rbh_evring_add_lost(rbh_evring *e, uint64_t count)
{
    if (count) atomic_fetch_add_explicit(&e->lost, count, memory_order_release);
}

int rbh_evring_read(rbh_evring *e, rb_event *out, int max, uint64_t *lost)
{
    int n = 0;
    while (n < max && rbh_queue_pop(&e->queue, &out[n])) n++;
    const uint64_t total = atomic_load_explicit(&e->lost, memory_order_acquire);
    if (lost) *lost = total - e->lost_seen;
    e->lost_seen = total;
    return n;
}

uint64_t rbh_evring_lost(const rbh_evring *e)
{
    return atomic_load_explicit(&e->lost, memory_order_acquire);
}
