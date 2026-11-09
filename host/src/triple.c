/*
 * triple.c - the snapshot triple buffer: see triple.h.
 */
#include "triple.h"

#include <string.h>

#include "audit.h"

#define RBH_TRIPLE_DIRTY 4u    /* "published, not yet read", beside the slot index in bits 0-1 */
#define RBH_TRIPLE_SLOT  3u

int rbh_triple_init(rbh_triple *t, size_t slot_size)
{
    memset(t, 0, sizeof *t);
    if (slot_size == 0) return -1;
    t->data = (uint8_t *)rbh_alloc(3 * slot_size);
    if (!t->data) return -1;
    t->slot_size = slot_size;
    t->back = 0;
    t->front = 2;
    atomic_init(&t->middle, 1);          /* not dirty: nothing published yet */
    atomic_init(&t->published, 0);
    atomic_init(&t->dropped, 0);
    return 0;
}

void rbh_triple_free(rbh_triple *t)
{
    rbh_free(t->data);
    t->data = NULL;
}

void *rbh_triple_write_begin(rbh_triple *t)
{
    return t->data + (size_t)t->back * t->slot_size;
}

void rbh_triple_write_end(rbh_triple *t)
{
    /* hand the filled slot over, take the middle one to fill next */
    const uint32_t old = atomic_exchange_explicit(&t->middle, t->back | RBH_TRIPLE_DIRTY, memory_order_acq_rel);
    if (old & RBH_TRIPLE_DIRTY) atomic_fetch_add_explicit(&t->dropped, 1, memory_order_relaxed);
    t->back = old & RBH_TRIPLE_SLOT;
    atomic_fetch_add_explicit(&t->published, 1, memory_order_relaxed);
}

const void *rbh_triple_read(rbh_triple *t, int *fresh)
{
    int is_new = 0;
    if (atomic_load_explicit(&t->middle, memory_order_acquire) & RBH_TRIPLE_DIRTY) {
        const uint32_t old = atomic_exchange_explicit(&t->middle, t->front, memory_order_acq_rel);
        t->front = old & RBH_TRIPLE_SLOT;
        is_new = 1;
    }
    if (fresh) *fresh = is_new;
    return t->data + (size_t)t->front * t->slot_size;
}
