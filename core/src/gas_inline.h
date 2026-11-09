/*
 * gas_inline.h - the gas's u(T) and Newton step for T, where the chambers
 * can inline them (stage 118).
 *
 * rb_gas_u_cv and rb_gas_T_cv_from_u_diag live in gas.c, which the chamber's
 * code calls tens of millions of times a second; MSVC's link-time
 * optimisation doesn't inline them across the two files, so each call cost a
 * function call and a store of cv through a pointer. The same code as static
 * inline functions, included by gas.c (whose functions now call these) and
 * chamber.c, gives the same bits: the arithmetic and its order are not
 * touched.
 */
#ifndef RB_GAS_INLINE_H
#define RB_GAS_INLINE_H

#include "diagnostics.h"
#include "fastmath.h"
#include "gas.h"

#include <math.h>

/* c0 + c1 t + c2 t^2 + c3 t^3 + c4 t^4 (Horner form). */
RB_INLINE double rb_gas_poly(const double c[5], double t)
{
    return c[0] + t * (c[1] + t * (c[2] + t * (c[3] + t * c[4])));
}

/* u(T) and cv(T) together (they share the powers of T). Outside the fitted
 * range cv is held constant, so u continues as a straight line. */
RB_INLINE double rb_gas_u_and_cv(const rb_gas *g, double T, double *cv)
{
    const double Tc = (T < RB_T_LO) ? RB_T_LO : (T > RB_T_HI) ? RB_T_HI : T;
    const double t = Tc * 1e-3;
    double u, c;
    if (Tc <= RB_T_MID) {
        c = rb_gas_poly(g->lo, t);
        u = g->k_lo + Tc * rb_gas_poly(g->ulo, t);
    } else {
        c = rb_gas_poly(g->hi, t);
        u = g->k_hi + Tc * rb_gas_poly(g->uhi, t);
    }
    *cv = c;
    return u + c * (T - Tc);
}

/* cv(T) alone. */
RB_INLINE double rb_gas_cv_inline(const rb_gas *g, double T)
{
    const double Tc = (T < RB_T_LO) ? RB_T_LO : (T > RB_T_HI) ? RB_T_HI : T;
    return rb_gas_poly(Tc <= RB_T_MID ? g->lo : g->hi, Tc * 1e-3);
}

/* Newton's method on f(T) = u(T) - u, whose slope is cv(T) > 0. It converges
 * quadratically: a correction dT leaves an error of about (dcv/dT) / (2 cv)
 * dT^2, under 1e-4 dT^2 per kelvin. So once a correction is below a
 * millionth of T, what's left is below 1e-10 K at engine temperatures. From
 * a good guess (a chamber's update makes one, chamber.c) that's usually a
 * single step. Returns T; *cv is cv(T) as of the last step. */
RB_INLINE double rb_gas_newton_T(const rb_gas *g, double u, double T_guess, double *cv_out,
                                 rb_diag_slot *diag)
{
    double T = (T_guess > 0.0 && T_guess < INFINITY) ? T_guess : RB_T_REF;
    double cv;
    if (diag && T != T_guess) rb_diag_note(diag, RB_D_BIT(RB_D_TEMP_FLOOR));
    if (!rb_finite(u)) {
        *cv_out = rb_gas_cv_inline(g, T);
        return NAN;
    }
    cv = 0.0;
    for (int i = 0; i < 50; i++) {
        const double dT = (rb_gas_u_and_cv(g, T, &cv) - u) / cv;
        T -= dT;
        if (T < 1.0) {
            if (diag) rb_diag_note(diag, RB_D_BIT(RB_D_TEMP_FLOOR));
            T = 1.0;
        }
        if (fabs(dT) <= 1e-6 * T) break;
        if (diag && i == 49) rb_diag_note(diag, RB_D_BIT(RB_D_NEWTON_LIMIT));
    }
    *cv_out = cv;
    return T;
}

#endif /* RB_GAS_INLINE_H */
