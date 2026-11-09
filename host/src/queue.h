/*
 * queue.h - the fixed-record queue (stage 123): a lock-free single-producer,
 * single-consumer queue of records of one size, used for the commands
 * (control -> render), the acknowledgments and the retired engines
 * (render -> control).
 *
 * Full means the push is refused, never waited for. As in ring.h, two
 * counters only grow and the fill is their difference.
 */
#ifndef RBH_QUEUE_H
#define RBH_QUEUE_H

#include <stdatomic.h>
#include <stdint.h>

typedef struct rbh_queue {
    uint8_t              *data;
    uint32_t              capacity;    /* records, a power of two             */
    uint32_t              mask;
    uint32_t              size;        /* bytes in a record                   */
    _Alignas(64) atomic_uint_least64_t head;    /* the producer's, records ever pushed */
    _Alignas(64) atomic_uint_least64_t tail;    /* the consumer's, records ever popped */
} rbh_queue;

/* 0, or -1 for a capacity that isn't a power of two, no size or no memory. */
int      rbh_queue_init(rbh_queue *q, uint32_t capacity, uint32_t record_size);
void     rbh_queue_free(rbh_queue *q);
/* Records waiting / room for more (either thread may ask). */
uint32_t rbh_queue_count(const rbh_queue *q);
uint32_t rbh_queue_space(const rbh_queue *q);
/* The producer: 0 pushed, -1 full (nothing is written). */
int      rbh_queue_push(rbh_queue *q, const void *record);
/* The consumer: 1 popped into `record`, 0 empty. */
int      rbh_queue_pop(rbh_queue *q, void *record);
/* The consumer: the oldest record without taking it: 1, or 0 if empty. */
int      rbh_queue_peek(const rbh_queue *q, void *record);

#endif /* RBH_QUEUE_H */
