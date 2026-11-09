/*
 * queue.c - the fixed-record queue: see queue.h.
 */
#include "queue.h"

#include <string.h>

#include "audit.h"

int rbh_queue_init(rbh_queue *q, uint32_t capacity, uint32_t record_size)
{
    memset(q, 0, sizeof *q);
    if (capacity == 0 || (capacity & (capacity - 1)) != 0 || record_size == 0) return -1;
    q->data = (uint8_t *)rbh_alloc((size_t)capacity * record_size);
    if (!q->data) return -1;
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->size = record_size;
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
    return 0;
}

void rbh_queue_free(rbh_queue *q)
{
    rbh_free(q->data);
    q->data = NULL;
}

uint32_t rbh_queue_count(const rbh_queue *q)
{
    const uint64_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    const uint64_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    return (uint32_t)(head - tail);
}

uint32_t rbh_queue_space(const rbh_queue *q)
{
    return q->capacity - rbh_queue_count(q);
}

int rbh_queue_push(rbh_queue *q, const void *record)
{
    const uint64_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
    const uint64_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    if (head - tail >= q->capacity) return -1;
    memcpy(q->data + (size_t)(head & q->mask) * q->size, record, q->size);
    atomic_store_explicit(&q->head, head + 1, memory_order_release);
    return 0;
}

int rbh_queue_pop(rbh_queue *q, void *record)
{
    const uint64_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    const uint64_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (head == tail) return 0;
    memcpy(record, q->data + (size_t)(tail & q->mask) * q->size, q->size);
    atomic_store_explicit(&q->tail, tail + 1, memory_order_release);
    return 1;
}

int rbh_queue_peek(const rbh_queue *q, void *record)
{
    const uint64_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    const uint64_t head = atomic_load_explicit(&q->head, memory_order_acquire);
    if (head == tail) return 0;
    memcpy(record, q->data + (size_t)(tail & q->mask) * q->size, q->size);
    return 1;
}
