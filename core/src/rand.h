/*
 * rand.h - the core's random numbers: xorshift64* (Vigna 2016), seeded, so a
 * render repeats exactly. The noise (noise.c, stage 12b) and the engine's
 * cycle-to-cycle variation (engine.c, stage 12c) each keep their own state.
 * A state must never be 0 (xorshift would stay there): seed with anything
 * else.
 */
#ifndef RB_RAND_H
#define RB_RAND_H

#include <math.h>
#include <stdint.h>

/* The next 64 random bits. */
static inline uint64_t rb_rand_next(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* Uniform in [0, 1). */
static inline double rb_rand_uniform(uint64_t *state)
{
    return (double)(rb_rand_next(state) >> 11) / 9007199254740992.0;
}

/* Uniform in [-1, 1), times sqrt(3) for an RMS of 1: white noise. */
static inline double rb_rand_white(uint64_t *state)
{
    const uint64_t r = rb_rand_next(state);
    return ((double)(r >> 11) * (2.0 / 9007199254740992.0) - 1.0) * 1.7320508075688772;
}

/* A standard normal number (mean 0, RMS 1), by Box and Muller's way: one of
 * the pair, from two uniform numbers. */
static inline double rb_rand_normal(uint64_t *state)
{
    const double u1 = ((double)(rb_rand_next(state) >> 11) + 1.0) / 9007199254740992.0;   /* (0, 1] */
    const double u2 = (double)(rb_rand_next(state) >> 11) / 9007199254740992.0;          /* [0, 1) */
    return sqrt(-2.0 * log(u1)) * cos(6.283185307179586 * u2);
}

#endif /* RB_RAND_H */
