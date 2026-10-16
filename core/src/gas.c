/*
 * gas.c - ideal-gas properties: species data, mixtures, u(T) and T(u).
 * The model is described in gas.h; the fits in docs/stage-03.md.
 */
#include "gas.h"

#include <math.h>
#include <stddef.h>

typedef struct species_data {
    const char *name;
    double molar_mass;   /* kg/mol                                          */
    double lo[5];        /* cp fit 200-1000 K, J/(kg K), powers of T/1000   */
    double hi[5];        /* cp fit 1000-4000 K                              */
} species_data;

/* Least-squares fits of the NASA Glenn data (Cantera 3.2, nasa_gas.yaml).
 * Above 1000 K the burned gas uses its *equilibrium* heat capacity at 40 bar
 * (stage 6): hot CO2 and H2O partly dissociate, which soaks up energy that
 * comes back as the gas cools. */
static const species_data SPECIES[RB_NSPECIES] = {
    [RB_AIR] = {
        "air", 28.9603e-3,
        { 1026.1074, -208.00753, 479.58521, -36.262009, -120.8045 },
        { 889.08077, 352.75235, -118.96814, 18.864993, -1.1114364 },
    },
    [RB_FUEL] = {
        "fuel", 103.9206e-3,
        { 212.99399, 3318.505, 5565.8456, -9593.0475, 4076.0479 },
        { 1219.1947, 3380.8607, -1201.0311, 192.83028, -11.509647 },
    },
    [RB_BURNED] = {
        "burned", 29.0081e-3,
        { 988.38783, 245.68805, -144.96926, 408.60824, -237.63136 },
        { -1104.893625, 5480.652686, -4504.991818, 1556.015261, -173.479527 },
    },
};

const char *rb_species_name(int species)
{
    return (species >= 0 && species < RB_NSPECIES) ? SPECIES[species].name : NULL;
}

double rb_species_R(int species)
{
    if (species < 0 || species >= RB_NSPECIES) return NAN;
    return RB_GAS_RU / SPECIES[species].molar_mass;
}

/* cp(T) = c0 + c1 t + c2 t^2 + c3 t^3 + c4 t^4 with t = T/1000 (Horner form). */
static double poly(const double c[5], double T)
{
    double t = T * 1e-3;
    return c[0] + t * (c[1] + t * (c[2] + t * (c[3] + t * c[4])));
}

/* Antiderivative of that polynomial with respect to T (dT = 1000 dt). */
static double poly_integral(const double c[5], double T)
{
    double t = T * 1e-3;
    return 1000.0 * t
         * (c[0] + t * (c[1] / 2.0 + t * (c[2] / 3.0 + t * (c[3] / 4.0 + t * c[4] / 5.0))));
}

void rb_gas_init(rb_gas *g, const double y[RB_NSPECIES])
{
    double sum = 0.0;
    for (int s = 0; s < RB_NSPECIES; s++)
        if (y[s] > 0.0) sum += y[s];             /* NaN and negatives count as 0 */

    g->R = 0.0;
    for (int k = 0; k < 5; k++) g->lo[k] = g->hi[k] = 0.0;

    for (int s = 0; s < RB_NSPECIES; s++) {
        double w;
        if (sum > 0.0 && sum < INFINITY) w = (y[s] > 0.0) ? y[s] / sum : 0.0;
        else                             w = (s == RB_AIR) ? 1.0 : 0.0;
        g->R += w * RB_GAS_RU / SPECIES[s].molar_mass;
        for (int k = 0; k < 5; k++) {
            g->lo[k] += w * SPECIES[s].lo[k];
            g->hi[k] += w * SPECIES[s].hi[k];
        }
    }

    /* u(T) = integral of cv = cp - R, starting at T_REF. */
    g->k_lo = -poly_integral(g->lo, RB_T_REF) + g->R * RB_T_REF;
    g->k_hi = g->k_lo + poly_integral(g->lo, RB_T_MID) - poly_integral(g->hi, RB_T_MID);
}

/* u(T) on the fitted range T_LO..T_HI. */
static double u_fit(const rb_gas *g, double T)
{
    if (T <= RB_T_MID) return poly_integral(g->lo, T) - g->R * T + g->k_lo;
    return poly_integral(g->hi, T) - g->R * T + g->k_hi;
}

double rb_gas_cp(const rb_gas *g, double T)
{
    if (T < RB_T_LO) T = RB_T_LO;
    if (T > RB_T_HI) T = RB_T_HI;
    return (T <= RB_T_MID) ? poly(g->lo, T) : poly(g->hi, T);
}

double rb_gas_cv(const rb_gas *g, double T)
{
    return rb_gas_cp(g, T) - g->R;
}

double rb_gas_gamma(const rb_gas *g, double T)
{
    double cp = rb_gas_cp(g, T);
    return cp / (cp - g->R);
}

double rb_gas_u(const rb_gas *g, double T)
{
    /* Outside the fitted range cp is constant, so u continues as a line. */
    if (T < RB_T_LO) return u_fit(g, RB_T_LO) + rb_gas_cv(g, RB_T_LO) * (T - RB_T_LO);
    if (T > RB_T_HI) return u_fit(g, RB_T_HI) + rb_gas_cv(g, RB_T_HI) * (T - RB_T_HI);
    return u_fit(g, T);
}

double rb_gas_h(const rb_gas *g, double T)
{
    return rb_gas_u(g, T) + g->R * T;
}

double rb_gas_T_from_u(const rb_gas *g, double u, double T_guess)
{
    /* Newton's method on f(T) = u(T) - u, whose slope is cv(T) > 0. From the
     * previous temperature it usually converges in one or two steps. */
    double T = (T_guess > 0.0 && T_guess < INFINITY) ? T_guess : RB_T_REF;
    if (!isfinite(u)) return NAN;
    for (int i = 0; i < 50; i++) {
        double dT = (rb_gas_u(g, T) - u) / rb_gas_cv(g, T);
        T -= dT;
        if (T < 1.0) T = 1.0;                   /* stay physical while iterating */
        if (fabs(dT) <= 1e-10 * T) break;
    }
    return T;
}
