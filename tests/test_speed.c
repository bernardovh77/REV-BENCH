/*
 * test_speed.c - stage 118's speed-ups change nothing the core computes.
 *
 * A faster function is tested the way the stage promises it: against what
 * the slower one gave, bit for bit. Where a result can't be derived by hand
 * (a mixture's twenty coefficients), the expected value is a hash of every
 * bit the function produced for a fixed set of inputs, taken from the code as
 * it was before the speed-up (stage 122's gas.c). The operations are IEEE
 * additions and multiplications in a fixed order, with no fused multiply-add
 * (the build forbids contraction), so the hash is the same on every
 * compiler.
 */
#include "fastmath.h"
#include "gas.h"
#include "pipe.h"
#include "valve.h"
#include "rb_test.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

/* FNV-1a over a block of bytes. */
static uint64_t fnv(uint64_t h, const void *data, size_t n)
{
    const unsigned char *p = data;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 1099511628211ULL;
    return h;
}

/* A deterministic stream of doubles in [0, 1): a small linear congruential
 * generator, so the inputs are the same everywhere. */
static double next01(uint64_t *s)
{
    *s = *s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (double)(*s >> 11) * (1.0 / 9007199254740992.0);
}

/* The mass-fraction vectors the engine meets and the ones its guards are for:
 * random mixtures (some scaled, as a chamber's are not exactly 1), a trace
 * of one species, the pure ones, zeros, negatives, NaN and infinity. */
static void mixture(int k, double y[RB_NSPECIES])
{
    uint64_t s = 0x9e3779b97f4a7c15ULL + 7919ULL * (uint64_t)k;
    for (int j = 0; j < RB_NSPECIES; j++) y[j] = next01(&s);
    switch (k % 16) {
    case 1:  y[RB_FUEL] = 1e-12 * y[RB_FUEL]; break;                    /* a trace of fuel   */
    case 2:  y[RB_FUEL] = 0.0; break;
    case 3:  y[RB_AIR] = 0.0; y[RB_FUEL] = 0.0; break;                  /* only burned gas   */
    case 4:  y[0] = y[1] = y[2] = 0.0; break;                           /* nothing: air      */
    case 5:  y[RB_AIR] = -y[RB_AIR]; break;                             /* a negative        */
    case 6:  y[RB_BURNED] = (double)NAN; break;
    case 7:  y[RB_FUEL] = (double)INFINITY; break;
    case 8:  for (int j = 0; j < RB_NSPECIES; j++) y[j] *= 1e-9; break; /* tiny but valid    */
    case 9:  for (int j = 0; j < RB_NSPECIES; j++) y[j] *= 1e6; break;
    case 10: y[RB_AIR] = 1.0; y[RB_FUEL] = 1e-9; y[RB_BURNED] = 0.0; break;
    default: break;
    }
}

/* Every bit of rb_gas_init's result for 4096 mixtures, hashed. */
static uint64_t gas_init_hash(void)
{
    uint64_t h = 14695981039346656037ULL;
    for (int k = 0; k < 4096; k++) {
        double y[RB_NSPECIES];
        rb_gas g;
        mixture(k, y);
        memset(&g, 0, sizeof g);
        rb_gas_init(&g, y);
        h = fnv(h, &g, sizeof g);
    }
    return h;
}

void test_speed_gas_init_is_bit_exact(void)
{
    /* the hash from stage 122's rb_gas_init, taken on gcc and on MSVC */
    const uint64_t h = gas_init_hash();
    CHECK(h == 0x64e516804a78e53cULL);
}

void test_speed_finite_is_isfinite(void)
{
    const double values[] = {
        0.0, -0.0, 1.0, -1.0, 5e-324, -5e-324, 2.2250738585072014e-308, 1e-300,
        1e300, 1.7976931348623157e308, -1.7976931348623157e308,
        (double)INFINITY, -(double)INFINITY, (double)NAN, -(double)NAN, 40.0, 1e-9,
    };
    for (size_t i = 0; i < sizeof values / sizeof values[0]; i++)
        CHECK_EQ_INT(rb_finite(values[i]) != 0, isfinite(values[i]) != 0);
}

/* rb_min, rb_max and rb_clamp agree in value with fmin, fmax and the clamp
 * written out, on the numbers the engine feeds them (never NaN). */
void test_speed_min_max_clamp_match_the_library(void)
{
    const double v[] = { 0.0, -0.0, 1.0, -1.0, 0.5, 1e-300, -1e-300, 1e300, -1e300, 40.0, 5e-324,
                         (double)INFINITY, -(double)INFINITY };
    const int n = (int)(sizeof v / sizeof v[0]);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            CHECK(rb_min(v[i], v[j]) == fmin(v[i], v[j]));
            CHECK(rb_max(v[i], v[j]) == fmax(v[i], v[j]));
            if (v[i] <= v[j] && isfinite(v[i]) && isfinite(v[j])) {
                CHECK(rb_clamp(0.25, v[i], v[j]) == fmax(v[i], fmin(v[j], 0.25)));
                CHECK(rb_clamp(v[i], v[i], v[j]) == v[i]);
                CHECK(rb_clamp(v[j], v[i], v[j]) == v[j]);
            }
        }
}

/* The valves' cycle offset is what the library's fmod gives: wrapped into
 * -360..360 degrees, over angles inside a cycle (the engine's), exactly at
 * its edges and beyond them. */
void test_speed_cycle_offset_matches_fmod(void)
{
    double angle[] = { 0.0, -0.0, 1.5, 359.999, 360.0, 361.0, 540.25, 719.999999, 720.0, 720.5, 1439.0,
                       1440.0, 2160.75, -1.0, -359.0, -360.0, -720.0, -721.5, -1500.0, 1e9, -1e9 };
    const double center[] = { 0.0, 100.0, 360.0, 475.5, 719.0 };
    for (size_t i = 0; i < sizeof angle / sizeof angle[0]; i++)
        for (size_t j = 0; j < sizeof center / sizeof center[0]; j++) {
            double want = fmod(angle[i] - center[j], 720.0);
            if (want < -360.0) want += 720.0;
            if (want >= 360.0) want -= 720.0;
            const double got = rb_cycle_offset(angle[i], center[j]);
            CHECK(got == want);
            CHECK(signbit(got) == signbit(want));
        }
    CHECK(rb_cycle_offset((double)NAN, 100.0) != rb_cycle_offset((double)NAN, 100.0));
    CHECK(rb_cycle_offset((double)INFINITY, 100.0) != rb_cycle_offset((double)INFINITY, 100.0));
}

/* Newton's method for T never leaves the floor: an energy far below any the
 * gas can have gives 1 K, not a negative temperature or NaN. */
void test_speed_newton_floors_the_temperature(void)
{
    rb_gas g;
    rb_gas_init(&g, (const double[RB_NSPECIES]){ [RB_AIR] = 1.0 });
    double cv = 0.0;
    const double T = rb_gas_T_cv_from_u(&g, -1e12, 300.0, &cv);
    CHECK(T == 1.0);
    CHECK(cv > 0.0);
    /* and a normal energy still finds its temperature */
    CHECK_NEAR(rb_gas_T_cv_from_u(&g, rb_gas_u(&g, 800.0), 700.0, &cv), 800.0, 1e-6);
}

/* The pipe's second-order momentum flux limits its slope to the smaller of the
 * two sides' (minmod), in the direction of the flow: a face's new flow is its
 * old one plus dt / length times the fluxes through the two cells it joins,
 * worked out here from the rule, on a pipe with no friction, no pressure
 * differences and closed ends. */
void test_speed_momentum_flux_is_minmod(void)
{
    static rb_chamber c[RB_PIPE_MAX_CELLS];
    static rb_face f[RB_PIPE_MAX_CELLS];
    static rb_pipe p;
    const double air[RB_NSPECIES] = { [RB_AIR] = 1.0 };
    CHECK(rb_pipe_init(&p, c, f, 6, 0.6, 0.04, 101325.0, 300.0, air, 300.0, 0.0) == 0);
    const double dt = 1.0 / 48000.0;
    /* flows through the five interior faces: for cell 2 the sides' differences
     * are 0.001 behind and 0.003 ahead, for cell 3 0.003 and 0.0005 */
    const double flow[5] = { 0.001, 0.002, 0.005, 0.0055, 0.0045 };
    for (int k = 0; k < 5; k++) p.face[k].mdot = flow[k];
    rb_pipe_accelerate(&p, 0, dt);
    /* the flux through cell k (1..4), by the rule */
    double flux[6] = { 0 };
    for (int k = 1; k <= 4; k++) {
        const double in = flow[k - 1], out = flow[k], mdot = 0.5 * (in + out);
        double transported = (mdot >= 0.0) ? in : out;
        const double left = in - (k > 1 ? flow[k - 2] : 0.0), right = out - in;
        if (mdot >= 0.0 && k > 0 && left > 0.0 && right > 0.0)
            transported += 0.5 * (fabs(left) < fabs(right) ? fabs(left) : fabs(right));
        flux[k] = mdot * transported / (p.cell[k].rho * p.area);
    }
    for (int k = 1; k <= 3; k++) {   /* faces between cells k and k + 1, both interior */
        const double want = flow[k] + dt * (flux[k] - flux[k + 1]) / p.face[k].length;
        CHECK_NEAR(p.face[k].mdot, want, 1e-12 * fabs(want) + 1e-15);
    }
}
