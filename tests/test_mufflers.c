/*
 * test_mufflers.c - stage 050: the exhaust's components against plane-wave
 * theory. A transmission-loss bench in the 1D pipe solver: a small pressure
 * pulse runs down a pipe into a component and on down another, both ends
 * soaked up by packed "sponges" (pipes packed harder and harder), so nothing
 * comes back. What arrives past the component, over what arrives past a plain
 * pipe in its place, is its transmission loss. Theory is the four-pole
 * (transfer-matrix) one: a duct of length L and impedance rho c / S is
 *
 *     [ cos kL        j Z sin kL ]
 *     [ j sin kL / Z  cos kL     ]
 *
 * a series resistance R (Pa per m/s of the gas in a pipe of area S) is
 * [ 1  R/S ; 0  1 ], and between pipes of impedance Z,
 * TL = 20 log10 |T11 + T12/Z + Z T21 + T22| / 2.
 *
 * The components are the presets' own (exhaust_models.c, engine_build.c):
 * the four's chambered can (12 L, 0.724 m) with its baffles; its centre can
 * (2.6 L, 0.241 m), held out (the tolerance is set on the main can); the
 * six's packed straight-through; the catalyst (a restriction with a laminar
 * loss). The air is still and at 300 K, the pulse 100 Pa: linear acoustics.
 */
#include "pipe.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
static const double PI = 3.14159265358979323846;
#define P0 101325.0
#define T0 300.0
#define DT (1.0 / 48000.0)   /* the engine's pipe step */
#define STEPS 9600           /* 0.2 s: the cans' rings have died away */
#define MAXP 10
#define WALL 0.0012          /* exhaust_models.c's pipe wall */
#define ENGINE_CELL 0.075    /* engine_build.c's LONG_CELL */
#define FINE_CELL 0.025

static rb_chamber cells[MAXP][RB_PIPE_MAX_CELLS];
static rb_face faces[MAXP][RB_PIPE_MAX_CELLS];
static rb_pipe pipes[MAXP];
static int npipes;
static double passed[STEPS], reference[STEPS];

enum { PLAIN, CAN, PACKED, CATALYST };
typedef struct {
    int kind;
    double d1;        /* the pipes' bore, m                         */
    double d2;        /* a can's bore, m                            */
    double L;         /* the component's length, m                  */
    double area;      /* a restriction's effective area, m^2 (or 0) */
    double laminar;   /* Pa per m/s of the pipe's gas (or 0)        */
    double packing;   /* Pa per m/s per m                           */
    double settle;    /* s                                          */
    double cell;      /* the component's cells, m                   */
} component;

static rb_pipe *add(int n, double length, double d)
{
    rb_pipe *p = &pipes[npipes];
    rb_pipe_init(p, cells[npipes], faces[npipes], n, length, d, P0, T0, AIR, T0, 0.0);
    npipes++;
    return p;
}

static void step(void)
{
    for (int i = 0; i < npipes; i++) rb_pipe_clear(&pipes[i], 0);
    for (int i = 0; i < npipes; i++) rb_pipe_flows(&pipes[i], 0, DT);
    for (int i = 0; i < npipes; i++) rb_pipe_integrate(&pipes[i], 0, DT);
    for (int i = 0; i < npipes; i++) rb_pipe_accelerate(&pipes[i], 0, DT);
}

/* Cell k at p, squeezed isentropically from the still gas: a sound wave. */
static void squeeze(rb_pipe *p, int k, double pressure)
{
    const double g = rb_chamber_gamma(&p->cell[k]);
    rb_chamber_init(&p->cell[k], p->area * p->dx, pressure,
                    T0 * pow(pressure / p->cell[k].p, (g - 1.0) / g), p->cell[k].y);
}

/* The sponges: three 3.2 m pipes packed harder outwards. Packing damps
 * swings faster than its settling time by R / (2 rho c) nepers a metre
 * (pipe.h); R tau stays under the gas's density. Round trip, under -40 dB
 * from 50 Hz up. */
static const double SPONGE[3] = { 50.0, 110.0, 180.0 };
#define SPONGE_SETTLE 5e-3

/* Runs the bench with component c; `out` gets the pressure past it, 1 m down
 * the outlet pipe. */
static void bench(const component *c, double *out)
{
    npipes = 0;
    rb_pipe *s3 = add(64, 3.2, c->d1), *s2 = add(64, 3.2, c->d1), *s1 = add(64, 3.2, c->d1);
    rb_pipe_pack(s3, SPONGE[2], SPONGE_SETTLE);
    rb_pipe_pack(s2, SPONGE[1], SPONGE_SETTLE);
    rb_pipe_pack(s1, SPONGE[0], SPONGE_SETTLE);
    rb_pipe *in = add(64, 3.2, c->d1);
    rb_pipe *o = add(64, 3.2, c->d1);
    rb_pipe *t1 = add(64, 3.2, c->d1), *t2 = add(64, 3.2, c->d1), *t3 = add(64, 3.2, c->d1);
    rb_pipe_pack(t1, SPONGE[0], SPONGE_SETTLE);
    rb_pipe_pack(t2, SPONGE[1], SPONGE_SETTLE);
    rb_pipe_pack(t3, SPONGE[2], SPONGE_SETTLE);
    CHECK(rb_pipe_onward(s3, s2) == 0 && rb_pipe_onward(s2, s1) == 0 && rb_pipe_onward(s1, in) == 0);
    CHECK(rb_pipe_onward(o, t1) == 0 && rb_pipe_onward(t1, t2) == 0 && rb_pipe_onward(t2, t3) == 0);

    const int n = rb_pipe_cells_for(c->L, c->cell, DT);
    rb_pipe *m = add(n, c->L, c->kind == CAN ? c->d2 : c->d1);
    switch (c->kind) {
    case CAN:        /* as engine_build.c joins a chambered muffler */
        rb_pipe_join(in, 1, rb_pipe_end_cell(m, 0), 0.0, 0);
        rb_pipe_join(o, 0, rb_pipe_end_cell(m, 1), 0.0, 0);
        if (c->area > 0.0) rb_pipe_restrict(o, 0, c->area);
        if (c->laminar > 0.0) rb_pipe_resist(o, 0, c->laminar);
        break;
    case CATALYST:   /* as an exhaust pipe's end into the next pipe's cell */
        rb_pipe_join(in, 1, rb_pipe_end_cell(m, 0), 0.0, 0);
        rb_pipe_restrict(in, 1, c->area);
        rb_pipe_resist(in, 1, c->laminar);
        CHECK(rb_pipe_onward(m, o) == 0);
        break;
    case PACKED:
        rb_pipe_pack(m, c->packing, c->settle);
        /* fall through */
    default:
        CHECK(rb_pipe_onward(in, m) == 0 && rb_pipe_onward(m, o) == 0);
        break;
    }
    for (int k = 0; k < 64; k++) {   /* the pulse, 0.8 m along the inlet */
        const double x = (k + 0.5) * in->dx - 0.8;
        squeeze(in, k, P0 + 100.0 * exp(-(x / 0.06) * (x / 0.06)));
    }
    for (int i = 0; i < STEPS; i++) {
        step();
        out[i] = o->cell[20].p - P0;
    }
}

/* |the record's Fourier transform| at f. */
static double spectrum(const double *x, double f)
{
    double re = 0.0, im = 0.0;
    for (int i = 0; i < STEPS; i++) {
        re += x[i] * cos(2.0 * PI * f * i * DT);
        im -= x[i] * sin(2.0 * PI * f * i * DT);
    }
    return sqrt(re * re + im * im);
}

/* ---- four-pole theory (no C99 complex: MSVC lacks it) ---- */
typedef struct { double re, im; } cx;
typedef struct { cx a, b, c, d; } pole4;

static cx cx_add(cx x, cx y) { cx z = { x.re + y.re, x.im + y.im }; return z; }
static cx cx_mul(cx x, cx y) { cx z = { x.re * y.re - x.im * y.im, x.re * y.im + x.im * y.re }; return z; }
static cx cx_scale(cx x, double s) { cx z = { x.re * s, x.im * s }; return z; }

static pole4 pole_times(pole4 x, pole4 y)
{
    pole4 z = { cx_add(cx_mul(x.a, y.a), cx_mul(x.b, y.c)), cx_add(cx_mul(x.a, y.b), cx_mul(x.b, y.d)),
                cx_add(cx_mul(x.c, y.a), cx_mul(x.d, y.c)), cx_add(cx_mul(x.c, y.b), cx_mul(x.d, y.d)) };
    return z;
}

/* A duct of length L, impedance Z (rho c / S), wavenumber k. */
static pole4 pole_duct(double k, double L, double Z)
{
    pole4 z = { { cos(k * L), 0.0 }, { 0.0, Z * sin(k * L) }, { 0.0, sin(k * L) / Z }, { cos(k * L), 0.0 } };
    return z;
}

/* A series resistance of Zs (Pa s/m^3). */
static pole4 pole_series(double Zs)
{
    pole4 z = { { 1.0, 0.0 }, { Zs, 0.0 }, { 0.0, 0.0 }, { 1.0, 0.0 } };
    return z;
}

/* Transmission loss, dB, between pipes of impedance Z either side. */
static double pole_tl(pole4 T, double Z)
{
    const cx s = cx_add(cx_add(T.a, cx_scale(T.b, 1.0 / Z)), cx_add(cx_scale(T.c, Z), T.d));
    return 20.0 * log10(0.5 * sqrt(s.re * s.re + s.im * s.im));
}

static double rho0(void) { return P0 / (287.0985 * T0); }
static double c0(void) { return sqrt(1.4 * 287.0985 * T0); }   /* air at 300 K: gamma 1.40 */

/* The theory's TL of component c at f. */
static double theory(const component *c, double f)
{
    const double k = 2.0 * PI * f / c0(), S1 = 0.25 * PI * c->d1 * c->d1, Z1 = rho0() * c0() / S1;
    pole4 T = pole_series(0.0);
    if (c->kind == CAN) {
        const double S2 = 0.25 * PI * c->d2 * c->d2;
        T = pole_duct(k, c->L, rho0() * c0() / S2);
        if (c->laminar > 0.0) T = pole_times(T, pole_series(c->laminar / S1));
    } else if (c->kind == CATALYST) {
        T = pole_series(c->laminar / S1);
    } else if (c->kind == PACKED) {
        /* a packed pipe's own attenuation (pipe.h): R / (2 rho c) nepers a
         * metre for swings faster than tau, (w tau)^2 / (1 + (w tau)^2) of it
         * below */
        const double wt = 2.0 * PI * f * c->settle;
        return 20.0 / log(10.0) * c->packing / (2.0 * rho0() * c0()) * wt * wt / (1.0 + wt * wt) * c->L;
    }
    return pole_tl(T, Z1);
}

/* The bench's TL of c against theory, every 50 Hz from 50 Hz to `top`: the
 * worst difference (dB), and the lowest TL measured (passivity). */
static double worst_gap(const component *c, double top, double *lowest)
{
    component plain = *c;
    plain.kind = PLAIN;
    bench(&plain, reference);
    bench(c, passed);
    double worst = 0.0, low = INFINITY;
    for (double f = 50.0; f <= top + 1e-9; f += 50.0) {
        const double tl = 20.0 * log10(spectrum(reference, f) / spectrum(passed, f));
        const double gap = fabs(tl - theory(c, f));
        if (gap > worst) worst = gap;
        if (tl < low) low = tl;
    }
    if (lowest) *lowest = low;
    return worst;
}

/* The four's chambered muffler (exhaust_models.c, preset 0) and its baffles
 * (engine_build.c's BAFFLE_LAMINAR, the tailpipe's restriction). */
static component four_can(double cell)
{
    const double d1 = 0.0508 - 2.0 * WALL;
    component c = { CAN, d1, sqrt(4.0 * 0.012 / (PI * 0.7239)), 0.7239, 0.0019, 30.0, 0.0, 0.0, cell };
    return c;
}

/* The four's centre silencer, as a can (center_volume over center_length). */
static component four_centre(double cell)
{
    const double d1 = 0.0508 - 2.0 * WALL;
    component c = { CAN, d1, sqrt(4.0 * 0.0026 / (PI * 0.2413)), 0.2413, 0.0, 0.0, 0.0, 0.0, cell };
    return c;
}

/* The six's straight-through (PACKING, PACKING_SETTLE in engine_build.c). */
static component six_packed(double cell)
{
    component c = { PACKED, 0.054 - 2.0 * WALL, 0.0, 0.52, 0.0, 0.0, 1500.0, 2.5e-4, cell };
    return c;
}

/* A catalyst as engine_build.c makes one: the four's default (21 cm^2,
 * CATALYST_LAMINAR 25 Pa per m/s), 0.3 m of pipe after it. */
static component catalyst(double cell)
{
    component c = { CATALYST, 0.0508 - 2.0 * WALL, 0.0, 0.3, 21e-4, 25.0, 0.0, 0.0, cell };
    return c;
}

void test_muffler_can_is_an_expansion_chamber(void)
{
    /* On fine cells the chambered can is the four-pole expansion chamber
     * (with the baffles' resistance in series) within 2 dB from 50 Hz to
     * 1.3 kHz: its domes (15 dB at kL = pi/2, 3pi/2...) and its nulls
     * (kL = n pi, 240 Hz apart) where theory puts them. 1.3 kHz is short of
     * its first cross mode (1.84 c / (pi D): 1.4 kHz here, 2 kHz in hot gas),
     * above which no 1D model holds. */
    double low;
    const component can = four_can(FINE_CELL);
    const double gap = worst_gap(&can, 1300.0, &low);
    printf("    the four's can, %.1f cm cells: within %.2f dB of theory to 1.3 kHz, lowest TL %.2f dB\n",
           100.0 * FINE_CELL, gap, low);
    CHECK(gap <= 2.0);
    CHECK(low >= -0.3);   /* passive: never louder past it */
    /* the theory itself: the bare can's dome is 10 log10(1 + (m - 1/m)^2 / 4) */
    component bare = can;
    bare.laminar = 0.0;
    const double m = (can.d2 / can.d1) * (can.d2 / can.d1);
    CHECK_NEAR(theory(&bare, 0.25 * c0() / can.L), 10.0 * log10(1.0 + 0.25 * (m - 1.0 / m) * (m - 1.0 / m)), 1e-9);
}

void test_muffler_centre_can_held_out(void)
{
    /* The four's centre silencer, held out: the main can's tolerance, never
     * fitted to it, from 50 Hz to 1.6 kHz. */
    double low;
    const component can = four_centre(FINE_CELL);
    const double gap = worst_gap(&can, 1600.0, &low);
    printf("    the four's centre can (held out): within %.2f dB to 1.6 kHz, lowest TL %.2f dB\n", gap, low);
    CHECK(gap <= 2.0);
    CHECK(low >= -0.3);
}

void test_muffler_packing_is_its_stated_loss(void)
{
    /* The packed straight-through takes what pipe.h says it does: R / (2 rho
     * c) nepers a metre above 1 / tau, (w tau)^2 / (1 + (w tau)^2) of it
     * below; within 1 dB to 1.6 kHz on fine cells. A steady flow sees none
     * (test_pipe_packing). */
    double low;
    const component packed = six_packed(FINE_CELL);
    const double gap = worst_gap(&packed, 1600.0, &low);
    printf("    the six's packing: within %.2f dB of its stated loss to 1.6 kHz\n", gap);
    CHECK(gap <= 1.0);
    CHECK(low >= -0.3);
}

void test_muffler_catalyst_is_a_series_resistance(void)
{
    /* A catalyst (a restriction with a laminar loss) is, for sound, a series
     * resistance of its laminar loss: 0.26 dB for the four's, flat, within
     * 0.15 dB to 1.6 kHz (the restriction's velocity heads are second order
     * and vanish for small swings). */
    double low;
    const component cat = catalyst(FINE_CELL);
    const double gap = worst_gap(&cat, 1600.0, &low);
    printf("    the catalyst: within %.3f dB of a series resistance (%.2f dB)\n", gap, theory(&cat, 500.0));
    CHECK(gap <= 0.15);
    CHECK(low >= 0.0);
}

void test_muffler_engine_grid_limits(void)
{
    /* What the engine's own grid (7.5 cm exhaust cells) keeps, stated in
     * docs/stage-050.md: within 2 dB of the same theory to 450 Hz in the
     * four's main can (10 cells), 750 Hz in its centre can (3 cells),
     * 1.1 kHz in the six's packing and 1.4 kHz across a catalyst, at 300 K
     * (in hot exhaust gas 1.5 times higher). Above, the cells are too few
     * for the waves in the cans. Every component stays passive. */
    double low;
    component c = four_can(ENGINE_CELL);
    CHECK(worst_gap(&c, 450.0, &low) <= 2.0);
    CHECK(worst_gap(&c, 1600.0, &low) > 2.0);   /* the limit is real */
    c = four_centre(ENGINE_CELL);
    CHECK(worst_gap(&c, 750.0, &low) <= 2.0);
    CHECK(low >= -0.3);
    c = six_packed(ENGINE_CELL);
    CHECK(worst_gap(&c, 1100.0, &low) <= 2.0);
    CHECK(low >= -0.3);
    c = catalyst(ENGINE_CELL);
    CHECK(worst_gap(&c, 1400.0, &low) <= 2.0);
    CHECK(low >= -0.3);
}
