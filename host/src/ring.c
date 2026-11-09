/*
 * ring.c - the audio ring: see ring.h.
 */
#include "ring.h"

#include <string.h>

#include "audit.h"

int rbh_ring_init(rbh_ring *r, uint32_t capacity_frames, uint32_t channels)
{
    memset(r, 0, sizeof *r);
    if (capacity_frames == 0 || (capacity_frames & (capacity_frames - 1)) != 0 || channels == 0) return -1;
    r->data = (float *)rbh_alloc((size_t)capacity_frames * channels * sizeof(float));
    if (!r->data) return -1;
    r->capacity = capacity_frames;
    r->mask = capacity_frames - 1;
    r->channels = channels;
    atomic_init(&r->write, 0);
    atomic_init(&r->read, 0);
    return 0;
}

void rbh_ring_free(rbh_ring *r)
{
    rbh_free(r->data);
    r->data = NULL;
}

uint32_t rbh_ring_fill(const rbh_ring *r)
{
    /* read first: the fill can only be too small, never negative */
    const uint64_t read = atomic_load_explicit(&r->read, memory_order_acquire);
    const uint64_t write = atomic_load_explicit(&r->write, memory_order_acquire);
    return (uint32_t)(write - read);
}

uint32_t rbh_ring_space(const rbh_ring *r)
{
    return r->capacity - rbh_ring_fill(r);
}

uint32_t rbh_ring_write(rbh_ring *r, const float *src, uint32_t frames)
{
    const uint64_t write = atomic_load_explicit(&r->write, memory_order_relaxed);
    const uint64_t read = atomic_load_explicit(&r->read, memory_order_acquire);
    const uint32_t space = r->capacity - (uint32_t)(write - read);
    const uint32_t n = frames < space ? frames : space;
    const uint32_t ch = r->channels;
    const uint32_t start = (uint32_t)write & r->mask;
    const uint32_t first = (r->capacity - start) < n ? (r->capacity - start) : n;
    memcpy(r->data + (size_t)start * ch, src, (size_t)first * ch * sizeof(float));
    if (n > first) memcpy(r->data, src + (size_t)first * ch, (size_t)(n - first) * ch * sizeof(float));
    atomic_store_explicit(&r->write, write + n, memory_order_release);
    return n;
}

uint32_t rbh_ring_read(rbh_ring *r, float *dst, uint32_t frames)
{
    const uint64_t read = atomic_load_explicit(&r->read, memory_order_relaxed);
    const uint64_t write = atomic_load_explicit(&r->write, memory_order_acquire);
    const uint32_t avail = (uint32_t)(write - read);
    const uint32_t n = frames < avail ? frames : avail;
    const uint32_t ch = r->channels;
    const uint32_t start = (uint32_t)read & r->mask;
    const uint32_t first = (r->capacity - start) < n ? (r->capacity - start) : n;
    memcpy(dst, r->data + (size_t)start * ch, (size_t)first * ch * sizeof(float));
    if (n > first) memcpy(dst + (size_t)first * ch, r->data, (size_t)(n - first) * ch * sizeof(float));
    atomic_store_explicit(&r->read, read + n, memory_order_release);
    return n;
}

uint32_t rbh_ring_flush(rbh_ring *r)
{
    const uint64_t read = atomic_load_explicit(&r->read, memory_order_relaxed);
    const uint64_t write = atomic_load_explicit(&r->write, memory_order_acquire);
    atomic_store_explicit(&r->read, write, memory_order_release);
    return (uint32_t)(write - read);
}

uint64_t rbh_ring_written(const rbh_ring *r)
{
    return atomic_load_explicit(&r->write, memory_order_acquire);
}

uint64_t rbh_ring_consumed(const rbh_ring *r)
{
    return atomic_load_explicit(&r->read, memory_order_acquire);
}
