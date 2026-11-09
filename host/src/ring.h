/*
 * ring.h - the audio ring (stage 123): a lock-free single-producer,
 * single-consumer ring of float frames, render thread -> device.
 *
 * Two counters only grow: `write` (frames ever written, by the producer)
 * and `read` (frames ever read, by the consumer). The fill is their
 * difference, so full and empty never look alike, and the producer's counter
 * doubles as the host's audio frame. The producer's release store of `write`
 * publishes the samples before it; the consumer's acquire load sees them.
 */
#ifndef RBH_RING_H
#define RBH_RING_H

#include <stdatomic.h>
#include <stdint.h>

typedef struct rbh_ring {
    float                *data;        /* capacity * channels floats          */
    uint32_t              capacity;    /* frames, a power of two              */
    uint32_t              mask;
    uint32_t              channels;
    _Alignas(64) atomic_uint_least64_t write;   /* the producer's, frames ever */
    _Alignas(64) atomic_uint_least64_t read;    /* the consumer's, frames ever */
} rbh_ring;

/* 0, or -1 for a capacity that isn't a power of two (or no memory). */
int      rbh_ring_init(rbh_ring *r, uint32_t capacity_frames, uint32_t channels);
void     rbh_ring_free(rbh_ring *r);
/* Frames waiting to be read / room to write (either thread may ask). */
uint32_t rbh_ring_fill(const rbh_ring *r);
uint32_t rbh_ring_space(const rbh_ring *r);
/* The producer: writes up to `frames` (as many as fit), returns how many. */
uint32_t rbh_ring_write(rbh_ring *r, const float *src, uint32_t frames);
/* The consumer: reads up to `frames`, returns how many. */
uint32_t rbh_ring_read(rbh_ring *r, float *dst, uint32_t frames);
/* The consumer: drops everything waiting, returns how many frames. */
uint32_t rbh_ring_flush(rbh_ring *r);
/* Frames ever written / read. */
uint64_t rbh_ring_written(const rbh_ring *r);
uint64_t rbh_ring_consumed(const rbh_ring *r);

#endif /* RBH_RING_H */
