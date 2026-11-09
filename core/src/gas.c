/*
 * gas.c - ideal-gas properties: species data, mixtures, u(T) and T(u).
 * The model is described in gas.h; the fits in docs/stage-03.md.
 */
#include "gas.h"
#include "diagnostics.h"
#include "fastmath.h"
#include "gas_inline.h"

#include <math.h>
#include <stddef.h>

/* Molar masses, kg/mol. */
#define M_AIR    28.9603e-3
#define M_FUEL   103.9206e-3
#define M_BURNED 29.0081e-3

typedef struct species_data {
    const char *name;
    double R;            /* J/(kg K): RB_GAS_RU over the molar mass         */
    double lo[5];        /* cp fit 200-1000 K, J/(kg K), powers of T/1000   */
    double hi[5];        /* cp fit 1000-4000 K                              */
    double k_lo, k_hi;   /* the species' own u(T) constants (see rb_gas)    */
} species_data;

/* A species' entry, from its name, molar mass and cp fits. The compiler works
 * out its u(T) constants: with cv = cp - R = c0 + c1 t + ... + c4 t^4
 * (t = T/1000), u(T) = k + T (c0 + c1 t/2 + c2 t^2/3 + c3 t^3/4 + c4 t^4/5),
 * where k_lo makes u(T_REF) = 0 and k_hi keeps u continuous at T_MID. Every
 * property of a mixture is its species' weighted by mass, these constants
 * too, so rb_gas_init never has to evaluate a polynomial. */
#define U_TERM(c0, c1, c2, c3, c4, t) \
    ((c0) + (t) * ((c1) / 2.0 + (t) * ((c2) / 3.0 + (t) * ((c3) / 4.0 + (t) * ((c4) / 5.0)))))
#define K_LO(R, l0, l1, l2, l3, l4) (-RB_T_REF * U_TERM((l0) - (R), l1, l2, l3, l4, RB_T_REF * 1e-3))
#define K_HI(R, l0, l1, l2, l3, l4, h0, h1, h2, h3, h4)                                  \
    (K_LO(R, l0, l1, l2, l3, l4) + RB_T_MID * (U_TERM((l0) - (R), l1, l2, l3, l4, 1.0)   \
                                              - U_TERM((h0) - (R), h1, h2, h3, h4, 1.0)))
#define SPECIES_ENTRY(name, M, l0, l1, l2, l3, l4, h0, h1, h2, h3, h4)                   \
    { name, RB_GAS_RU / (M), { l0, l1, l2, l3, l4 }, { h0, h1, h2, h3, h4 },             \
      K_LO(RB_GAS_RU / (M), l0, l1, l2, l3, l4),                                          \
      K_HI(RB_GAS_RU / (M), l0, l1, l2, l3, l4, h0, h1, h2, h3, h4) }

/* Least-squares fits of the NASA Glenn data (Cantera 3.2, nasa_gas.yaml):
 * 200-1000 K, then 1000-4000 K. Above 1000 K the burned gas uses its
 * *equilibrium* heat capacity at 40 bar (stage 6): hot CO2 and H2O partly
 * dissociate, which soaks up energy that comes back as the gas cools. */
static const species_data SPECIES[RB_NSPECIES] = {
    [RB_AIR] = SPECIES_ENTRY("air", M_AIR,
        1026.1074, -208.00753, 479.58521, -36.262009, -120.8045,
        889.08077, 352.75235, -118.96814, 18.864993, -1.1114364),
    [RB_FUEL] = SPECIES_ENTRY("fuel", M_FUEL,
        212.99399, 3318.505, 5565.8456, -9593.0475, 4076.0479,
        1219.1947, 3380.8607, -1201.0311, 192.83028, -11.509647),
    [RB_BURNED] = SPECIES_ENTRY("burned", M_BURNED,
        988.38783, 245.68805, -144.96926, 408.60824, -237.63136,
        -1104.893625, 5480.652686, -4504.991818, 1556.015261, -173.479527),
};

const char *rb_species_name(int species)
{
    return (species >= 0 && species < RB_NSPECIES) ? SPECIES[species].name : NULL;
}

double rb_species_R(int species)
{
    if (species < 0 || species >= RB_NSPECIES) return NAN;
    return SPECIES[species].R;
}

/* Integrating c_k t^k over T (t = T/1000) gives T c_k t^k / (k + 1). */
/* Where the SSE2 path (below) takes lo[0..4] and hi[0..4] as ten numbers in a
 * row: the species' and the mixture's arrays lie one after another. */
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define RB_GAS_SSE2 1
static const double INTEGRAL10[10] = { 1.0, 1.0 / 2.0, 1.0 / 3.0, 1.0 / 4.0, 1.0 / 5.0,
                                       1.0, 1.0 / 2.0, 1.0 / 3.0, 1.0 / 4.0, 1.0 / 5.0 };
_Static_assert(offsetof(species_data, hi) == offsetof(species_data, lo) + 5 * sizeof(double),
               "a species' lo and hi lie in a row");
_Static_assert(offsetof(rb_gas, hi) == offsetof(rb_gas, lo) + 5 * sizeof(double)
               && offsetof(rb_gas, uhi) == offsetof(rb_gas, ulo) + 5 * sizeof(double),
               "a mixture's lo and hi, and its ulo and uhi, lie in a row");
#else
#define RB_GAS_SSE2 0
static const double INTEGRAL[5] = { 1.0, 1.0 / 2.0, 1.0 / 3.0, 1.0 / 4.0, 1.0 / 5.0 };
#endif

void rb_gas_init(rb_gas *g, const double y[RB_NSPECIES])
{
    double sum = 0.0, w[RB_NSPECIES];
    for (int s = 0; s < RB_NSPECIES; s++)
        if (y[s] > 0.0) sum += y[s];             /* NaN and negatives count as 0 */
    const int valid = sum > 0.0 && sum < INFINITY;
    const double scale = valid ? 1.0 / sum : 0.0;
    for (int s = 0; s < RB_NSPECIES; s++) {
        if (valid) w[s] = (y[s] > 0.0) ? y[s] * scale : 0.0;
        else       w[s] = (s == RB_AIR) ? 1.0 : 0.0;
    }

    /* A mixture's R, cp and u(T) constants are its species' weighted by mass.
     * Then cv = cp - R, and u(T)'s polynomial is cv's integrated. */
    const species_data *a = &SPECIES[RB_AIR], *f = &SPECIES[RB_FUEL], *b = &SPECIES[RB_BURNED];
    const double wa = w[RB_AIR], wf = w[RB_FUEL], wb = w[RB_BURNED];
    const double R = wa * a->R + wf * f->R + wb * b->R;
    g->R = R;
    /* The ten coefficients of cv (lo[0..4], then hi[0..4]) and each one's
     * integral for u(T): every one is its species' weighted by mass, less R
     * for the first of each range, then times 1/(k + 1). Each coefficient is
     * independent of the others, so doing two at a time in one SSE2 register
     * (stage 118) gives the same bits as one by one: the same additions and
     * multiplications, in the same order, never fused. */
#if RB_GAS_SSE2
    {
        const __m128d vwa = _mm_set1_pd(wa), vwf = _mm_set1_pd(wf), vwb = _mm_set1_pd(wb);
        const __m128d take_R[5] = { _mm_set_pd(0.0, R), _mm_setzero_pd(), _mm_set_pd(R, 0.0),
                                    _mm_setzero_pd(), _mm_setzero_pd() };
        for (int i = 0; i < 5; i++) {
            const __m128d x = _mm_sub_pd(
                _mm_add_pd(_mm_add_pd(_mm_mul_pd(vwa, _mm_loadu_pd(a->lo + 2 * i)),
                                      _mm_mul_pd(vwf, _mm_loadu_pd(f->lo + 2 * i))),
                           _mm_mul_pd(vwb, _mm_loadu_pd(b->lo + 2 * i))),
                take_R[i]);
            _mm_storeu_pd(g->lo + 2 * i, x);
            _mm_storeu_pd(g->ulo + 2 * i, _mm_mul_pd(x, _mm_loadu_pd(INTEGRAL10 + 2 * i)));
        }
    }
#else
    /* Keep each coefficient and its integral together in this loop. Each
     * coefficient is independent, so SIMD does not reorder species sums. */
#if defined(__clang__)
#pragma clang loop vectorize(enable) interleave(disable) unroll(disable)
#endif
    for (int k = 0; k < 5; k++) {
        const double subtract_R = k == 0 ? R : 0.0;
        g->lo[k] = (wa * a->lo[k] + wf * f->lo[k] + wb * b->lo[k]) - subtract_R;
        g->hi[k] = (wa * a->hi[k] + wf * f->hi[k] + wb * b->hi[k]) - subtract_R;
        g->ulo[k] = g->lo[k] * INTEGRAL[k];
        g->uhi[k] = g->hi[k] * INTEGRAL[k];
    }
#endif
    g->k_lo = wa * a->k_lo + wf * f->k_lo + wb * b->k_lo;
    g->k_hi = wa * a->k_hi + wf * f->k_hi + wb * b->k_hi;
}

double rb_gas_cv(const rb_gas *g, double T)
{
    return rb_gas_cv_inline(g, T);
}

double rb_gas_cp(const rb_gas *g, double T)
{
    return rb_gas_cv(g, T) + g->R;
}

double rb_gas_gamma(const rb_gas *g, double T)
{
    const double cv = rb_gas_cv(g, T);
    return (cv + g->R) / cv;
}

double rb_gas_u(const rb_gas *g, double T)
{
    double cv;
    return rb_gas_u_and_cv(g, T, &cv);
}

double rb_gas_u_cv(const rb_gas *g, double T, double *cv)
{
    return rb_gas_u_and_cv(g, T, cv);
}

double rb_gas_h(const rb_gas *g, double T)
{
    return rb_gas_u(g, T) + g->R * T;
}

double rb_gas_T_from_u(const rb_gas *g, double u, double T_guess)
{
    double cv;
    return rb_gas_T_cv_from_u(g, u, T_guess, &cv);
}

double rb_gas_T_cv_from_u_diag(const rb_gas *g, double u, double T_guess, double *cv, rb_diag_slot *diag)
{
    return rb_gas_newton_T(g, u, T_guess, cv, diag);
}

double rb_gas_T_cv_from_u(const rb_gas *g, double u, double guess, double *cv)
{
    return rb_gas_T_cv_from_u_diag(g, u, guess, cv, NULL);
}
