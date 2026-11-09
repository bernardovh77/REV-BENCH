/*
 * fastmath.h - small arithmetic helpers that give the same result as the C
 * library's, but without its function call (stage 118).
 *
 * MSVC compiles isfinite to a call to _finite, fmin and fmax to calls into
 * the C runtime, and copysign to a call to copysignl, on every use: in the
 * engine's per-step code they cost about 5 % of the time. Each helper here
 * is exact for the arguments it is documented for, so a render is bit for bit
 * the same as before (tools/bench_render.py checks every build's hash).
 */
#ifndef RB_FASTMATH_H
#define RB_FASTMATH_H

#include <math.h>

/* A function that is always inlined where it is called: a helper in a header
 * that the compilers' own heuristics (and MSVC's link-time code generation
 * across files) would leave as a call. */
#if defined(_MSC_VER)
#define RB_INLINE static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define RB_INLINE static inline __attribute__((always_inline))
#else
#define RB_INLINE static inline
#endif
/* isfinite(x): 1 for every number, 0 for infinities and NaNs. */
static inline int rb_finite(double x)
{
    return fabs(x) < (double)INFINITY;   /* false for NaN and for infinity */
}

/* min(a, b) for numbers that aren't NaN. Equal values (and signed zeros)
 * return a: the callers never tell them apart. */
static inline double rb_min(double a, double b)
{
    return (b < a) ? b : a;
}

/* max(a, b) for numbers that aren't NaN. */
static inline double rb_max(double a, double b)
{
    return (b > a) ? b : a;
}

/* a clamped to [lo, hi], for finite a and lo <= hi. */
static inline double rb_clamp(double a, double lo, double hi)
{
    return (a > hi) ? hi : (a < lo) ? lo : a;
}

#endif /* RB_FASTMATH_H */
