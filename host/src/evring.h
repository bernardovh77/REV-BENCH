/*
 * evring.h - the event ring (stage 123): the engine's events, render thread
 * -> control thread, with loss counting.
 *
 * A queue (queue.h) of rb_event records and a counter of the events that
 * couldn't be pushed because it was full or that the core's own ring lost
 * first. The consumer takes the events and, with them, how many were lost
 * since its last read: a trace is complete only when that is 0
 * (contracts/runtime-v1.md). Nothing ever waits.
 */
#ifndef RBH_EVRING_H
#define RBH_EVRING_H

#include <stdatomic.h>
#include <stdint.h>

#include "queue.h"
#include "rb.h"

typedef struct rbh_evring {
    rbh_queue             queue;
    atomic_uint_least64_t lost;        /* events lost, ever (the producer's) */
    uint64_t              lost_seen;   /* what the consumer has been told    */
} rbh_evring;

int      rbh_evring_init(rbh_evring *e, uint32_t capacity);
void     rbh_evring_free(rbh_evring *e);
/* The producer: 1 pushed, 0 lost (counted). */
int      rbh_evring_push(rbh_evring *e, const rb_event *event);
/* The producer: events lost somewhere before the ring. */
void     rbh_evring_add_lost(rbh_evring *e, uint64_t count);
/* The consumer: up to `max` events; *lost the events lost since its last
 * call (may be NULL). Returns how many events. */
int      rbh_evring_read(rbh_evring *e, rb_event *out, int max, uint64_t *lost);
uint64_t rbh_evring_lost(const rbh_evring *e);

#endif /* RBH_EVRING_H */
