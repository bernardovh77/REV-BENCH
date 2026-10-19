/*
 * gas.c - ideal-gas properties: species data, mixtures, u(T) and T(u).
 * The model is described in gas.h; the fits in docs/stage-03.md.
 */
#include "gas.h"

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
} species_data;

/* Least-squares fits of the NASA Glenn data (Cantera 3.2, nasa_gas.yaml).
 * Above 1000 K the burned gas uses its *equilibrium* heat capacity at 40 bar
 * (stage 6): hot CO2 and H2O partly dissociate, which soaks up energy that
 * comes back as the gas cools. */
static const species_data SPECIES[RB_NSPECIES] = {
    [RB_AIR] = {
        "air", RB_GAS_RU / M_AIR,
        { 1026.1074, -208.00753, 479.58521, -36.262009, -120.8045 },
        { 889.08077, 352.75235, -118.96814, 18.864993, -1.1114364 },
    },
    [RB_FUEL] = {
        "fuel", RB_GAS_RU / M_FUEL,
        { 212.99399, 3318.505, 5565.8456, -9593.0475, 4076.0479 },
        { 1219.1947, 3380.8607, -1201.0311, 192.83028, -11.509647 },
    },
    [RB_BURNED] = {
        "burned", RB_GAS_RU / M_BURNED,
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
    return SPECIES[species].R;
}

/* c0 + c1 t + c2 t^2 + c3 t^3 + c4 t^4 (Horner form). */
static double poly(const double c[5], double t)
{
    return c[0] + t * (c[1] + t * (c[2] + t * (c[3] + t * c[4])));
}

/* Integrating c_k t^k over T (t = T/1000) gives T c_k t^k / (k + 1). */
static const double INTEGRAL[5] = { 1.0, 1.0 / 2.0, 1.0 / 3.0, 1.0 / 4.0, 1.0 / 5.0 };

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

    /* A mixture's R and cp are its species' weighted by mass. Then cv = cp - R,
     * and u(T) is its integral from T_REF, continuous at T_MID. */
    const species_data *a = &SPECIES[RB_AIR], *f = &SPECIES[RB_FUEL], *b = &SPECIES[RB_BURNED];
    const double R = w[RB_AIR] * a->R + w[RB_FUEL] * f->R + w[RB_BURNED] * b->R;
    double lo[5], hi[5];
    for (int k = 0; k < 5; k++) {
        lo[k] = w[RB_AIR] * a->lo[k] + w[RB_FUEL] * f->lo[k] + w[RB_BURNED] * b->lo[k];
        hi[k] = w[RB_AIR] * a->hi[k] + w[RB_FUEL] * f->hi[k] + w[RB_BURNED] * b->hi[k];
    }
    lo[0] -= R;
    hi[0] -= R;
    g->R = R;
    for (int k = 0; k < 5; k++) {
        g->lo[k] = lo[k];
        g->hi[k] = hi[k];
        g->ulo[k] = lo[k] * INTEGRAL[k];
        g->uhi[k] = hi[k] * INTEGRAL[k];
    }
    g->k_lo = -RB_T_REF * poly(g->ulo, RB_T_REF * 1e-3);
    g->k_hi = g->k_lo + RB_T_MID * (poly(g->ulo, 1.0) - poly(g->uhi, 1.0));
}

/* u(T) and cv(T) together (they share the powers of T). Outside the fitted
 * range cv is held constant, so u continues as a straight line. */
static double u_and_cv(const rb_gas *g, double T, double *cv)
{
    const double Tc = (T < RB_T_LO) ? RB_T_LO : (T > RB_T_HI) ? RB_T_HI : T;
    const double t = Tc * 1e-3;
    double u;
    if (Tc <= RB_T_MID) {
        *cv = poly(g->lo, t);
        u = g->k_lo + Tc * poly(g->ulo, t);
    } else {
        *cv = poly(g->hi, t);
        u = g->k_hi + Tc * poly(g->uhi, t);
    }
    return u + *cv * (T - Tc);
}

double rb_gas_cv(const rb_gas *g, double T)
{
    const double Tc = (T < RB_T_LO) ? RB_T_LO : (T > RB_T_HI) ? RB_T_HI : T;
    return poly(Tc <= RB_T_MID ? g->lo : g->hi, Tc * 1e-3);
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
    return u_and_cv(g, T, &cv);
}

double rb_gas_h(const rb_gas *g, double T)
{
    return rb_gas_u(g, T) + g->R * T;
}

double rb_gas_T_from_u(const rb_gas *g, double u, double T_guess)
{
    /* Newton's method on f(T) = u(T) - u, whose slope is cv(T) > 0. It
     * converges quadratically: a correction dT leaves an error of about
     * (dcv/dT) / (2 cv) dT^2, under 1e-4 dT^2 per kelvin. So once a
     * correction is below a millionth of T, what's left is below 1e-10 K at
     * engine temperatures. From the previous temperature that's usually one
     * or two steps. */
    double T = (T_guess > 0.0 && T_guess < INFINITY) ? T_guess : RB_T_REF;
    if (!isfinite(u)) return NAN;
    for (int i = 0; i < 50; i++) {
        double cv;
        const double dT = (u_and_cv(g, T, &cv) - u) / cv;
        T -= dT;
        if (T < 1.0) T = 1.0;                   /* stay physical while iterating */
        if (fabs(dT) <= 1e-6 * T) break;
    }
    return T;
}
