/*
 * fourpole.h - plane-wave (four-pole, transfer-matrix) acoustics for the tests
 * (test_mufflers.c, test_parts.c). No C99 complex: MSVC lacks it.
 *
 * A two-port relates (pressure, volume velocity) on its inlet to those on
 * its outlet: [p1; u1] = T [p2; u2]. A duct of length L, impedance
 * Z = rho c / S, wavenumber k, is [cos kL, j Z sin kL; j sin kL / Z, cos kL];
 * a series resistance R (Pa s/m^3) is [1, R; 0, 1]; a side branch of input
 * impedance Zb, where the pressure is shared and the flow splits, is
 * [1, 0; 1 / Zb, 1]. Between pipes of impedance Z either side, the
 * transmission loss is TL = 20 log10 |T11 + T12 / Z + Z T21 + T22| / 2.
 */
#ifndef RB_FOURPOLE_H
#define RB_FOURPOLE_H

#include <math.h>

typedef struct { double re, im; } cx;
typedef struct { cx a, b, c, d; } pole4;

static inline cx cx_add(cx x, cx y) { cx z = { x.re + y.re, x.im + y.im }; return z; }
static inline cx cx_mul(cx x, cx y) { cx z = { x.re * y.re - x.im * y.im, x.re * y.im + x.im * y.re }; return z; }
static inline cx cx_scale(cx x, double s) { cx z = { x.re * s, x.im * s }; return z; }
static inline cx cx_div(cx x, cx y)
{
    const double n = y.re * y.re + y.im * y.im;
    cx z = { (x.re * y.re + x.im * y.im) / n, (x.im * y.re - x.re * y.im) / n };
    return z;
}

static inline pole4 pole_times(pole4 x, pole4 y)
{
    pole4 z = { cx_add(cx_mul(x.a, y.a), cx_mul(x.b, y.c)), cx_add(cx_mul(x.a, y.b), cx_mul(x.b, y.d)),
                cx_add(cx_mul(x.c, y.a), cx_mul(x.d, y.c)), cx_add(cx_mul(x.c, y.b), cx_mul(x.d, y.d)) };
    return z;
}

/* A duct of length L, impedance Z (rho c / S), wavenumber k. */
static inline pole4 pole_duct(double k, double L, double Z)
{
    pole4 z = { { cos(k * L), 0.0 }, { 0.0, Z * sin(k * L) }, { 0.0, sin(k * L) / Z }, { cos(k * L), 0.0 } };
    return z;
}

/* A series resistance of Zs (Pa s/m^3). */
static inline pole4 pole_series(double Zs)
{
    pole4 z = { { 1.0, 0.0 }, { Zs, 0.0 }, { 0.0, 0.0 }, { 1.0, 0.0 } };
    return z;
}

/* A side branch of input impedance Zb. */
static inline pole4 pole_branch(cx Zb)
{
    const cx one = { 1.0, 0.0 };
    pole4 z = { { 1.0, 0.0 }, { 0.0, 0.0 }, cx_div(one, Zb), { 1.0, 0.0 } };
    return z;
}

/* The input impedance of two-port T ended by load Zl: (T11 Zl + T12) / (T21 Zl + T22). */
static inline cx pole_input(pole4 T, cx Zl)
{
    return cx_div(cx_add(cx_mul(T.a, Zl), T.b), cx_add(cx_mul(T.c, Zl), T.d));
}

/* Transmission loss, dB, between pipes of impedance Z either side. */
static inline double pole_tl(pole4 T, double Z)
{
    const cx s = cx_add(cx_add(T.a, cx_scale(T.b, 1.0 / Z)), cx_add(cx_scale(T.c, Z), T.d));
    return 20.0 * log10(0.5 * sqrt(s.re * s.re + s.im * s.im));
}

#endif
