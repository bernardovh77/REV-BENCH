/*
 * triple.h - the snapshot triple buffer (stage 123): the render thread
 * publishes whole snapshots, the control thread reads the newest, and
 * neither waits for the other.
 *
 * Three slots: the writer owns one (`back`), the reader owns one (`front`)
 * and the third (`middle`) is where they hand over, in one atomic word
 * (slot index, and a flag meaning "published and not yet read"). Publishing
 * swaps the writer's slot with the middle one; reading swaps the reader's
 * with the middle one if the flag is set. A slot is only ever written by the
 * thread that owns it, so a reader sees a snapshot whole or not at all. If
 * the reader is slow the writer overwrites the unread one and counts it
 * dropped.
 */
#ifndef RBH_TRIPLE_H
#define RBH_TRIPLE_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

typedef struct rbh_triple {
    uint8_t              *data;        /* 3 slots of slot_size bytes          */
    size_t                slot_size;
    atomic_uint_least32_t middle;      /* slot index | RBH_TRIPLE_DIRTY       */
    uint32_t              back;        /* the writer's slot (writer only)     */
    uint32_t              front;       /* the reader's slot (reader only)     */
    atomic_uint_least64_t published;
    atomic_uint_least64_t dropped;     /* published and overwritten unread    */
} rbh_triple;

/* 0, or -1 (no size, no memory). */
int         rbh_triple_init(rbh_triple *t, size_t slot_size);
void        rbh_triple_free(rbh_triple *t);
/* The writer: the slot to fill, then publish it. */
void       *rbh_triple_write_begin(rbh_triple *t);
void        rbh_triple_write_end(rbh_triple *t);
/* The reader: the newest published slot, or the one it read last. *fresh
 * says whether it's new since the last call. Valid until the next call. */
const void *rbh_triple_read(rbh_triple *t, int *fresh);

#endif /* RBH_TRIPLE_H */
