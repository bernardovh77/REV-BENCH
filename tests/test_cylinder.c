/*
 * test_cylinder.c - slider-crank geometry and piston work: exact geometry,
 * compression against an independent isentrope, second-order convergence,
 * no energy drift over 100 revolutions, and the time symmetry of the
 * volume step.
 */
#include "cylinder.h"
#include "rb_test.h"

#include <math.h>

static const double PI = 3.14159265358979323846;
static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
#define DT (1.0 / 192000.0)   /* the default physics step */

/* A typical 2.0 L four's cylinder: 86 x 86 mm, 143 mm rod, CR 10. */
static rb_cyl_geom engine_86x86(void)
{
    rb_cyl_geom g;
    rb_cyl_geom_init(&g, 0.086, 0.086, 0.143, 10.0);
    return g;
}

/* Temperature after an isentropic volume change V1 -> V2, by RK4 on
 * dT/d(ln V) = -R T / cv(T) with 20000 small steps. Independent of the
 * core's own integrator, so it checks it. */
static double isentrope_T(const rb_gas *g, double T, double V1, double V2)
{
    const int n = 20000;
    const double h = log(V2 / V1) / n;
    for (int i = 0; i < n; i++) {
        double k1 = -g->R * T / rb_gas_cv(g, T);
        double T2 = T + 0.5 * h * k1;
        double k2 = -g->R * T2 / rb_gas_cv(g, T2);
        double T3 = T + 0.5 * h * k2;
        double k3 = -g->R * T3 / rb_gas_cv(g, T3);
        double T4 = T + h * k3;
        double k4 = -g->R * T4 / rb_gas_cv(g, T4);
        T += h * (k1 + 2.0 * k2 + 2.0 * k3 + k4) / 6.0;
    }
    return T;
}

/* Valves closed, from BDC (1 bar, 300 K) up to TDC at `rpm`. */
static void compress(rb_cylinder *c, double rpm)
{
    rb_cyl_geom g = engine_86x86();
    const double dtheta = rpm / 60.0 * 2.0 * PI * DT;   /* crank angle per step */
    const long n = (long)ceil(PI / dtheta);
    rb_cylinder_init(c, &g, PI, 1e5, 300.0, AIR);
    for (long i = 1; i <= n; i++) rb_cylinder_move(c, PI + PI * (double)i / (double)n);
}

void test_cylinder_geometry(void)
{
    rb_cyl_geom g = engine_86x86(), long_rod, bad;
    int wrong = 0;

    CHECK_NEAR(g.v_disp * 1e6, 499.56, 0.01);   /* cm^3: ~500 cc per cylinder */
    CHECK_NEAR(g.v_clear, g.v_disp / 9.0, 1e-15);
    CHECK_NEAR(rb_cyl_volume(&g, 0.0), g.v_clear, 1e-18);
    CHECK_NEAR(rb_cyl_volume(&g, PI), g.v_clear + g.v_disp, 1e-15);
    CHECK_NEAR(rb_cyl_volume(&g, PI) / rb_cyl_volume(&g, 0.0), 10.0, 1e-12);

    /* At 90 degrees the tilted rod has pulled the piston past half stroke. */
    CHECK_NEAR(rb_cyl_piston_x(&g, PI / 2), 0.043 + 0.143 - sqrt(0.143 * 0.143 - 0.043 * 0.043), 1e-12);
    CHECK(rb_cyl_piston_x(&g, PI / 2) > 0.5 * g.stroke);

    for (double th = 0.01; th < 2.0 * PI; th += 0.1) {
        if (fabs(rb_cyl_piston_x(&g, th) - rb_cyl_piston_x(&g, -th)) > 1e-15) wrong++;          /* symmetric */
        if (fabs(rb_cyl_piston_x(&g, th) - rb_cyl_piston_x(&g, th + 2.0 * PI)) > 1e-14) wrong++; /* periodic  */
        double slope = (rb_cyl_volume(&g, th + 1e-6) - rb_cyl_volume(&g, th - 1e-6)) / 2e-6;
        if (fabs(slope - rb_cyl_dvolume(&g, th)) > 1e-9) wrong++;                                 /* dV/dtheta */
    }
    CHECK_EQ_INT(wrong, 0);

    /* With a very long rod the piston moves as a pure sine: x = a (1 - cos theta). */
    CHECK_EQ_INT(rb_cyl_geom_init(&long_rod, 0.086, 0.086, 86.0, 10.0), 0);
    CHECK_NEAR(rb_cyl_piston_x(&long_rod, PI / 2), 0.043, 0.043 * 1e-3);

    /* Impossible engines are rejected. */
    CHECK_EQ_INT(rb_cyl_geom_init(&bad, 0.086, 0.086, 0.080, 10.0), -1);   /* rod shorter than stroke */
    CHECK_EQ_INT(rb_cyl_geom_init(&bad, 0.086, 0.086, 0.143, 1.0), -1);    /* no compression */
    CHECK_EQ_INT(rb_cyl_geom_init(&bad, -0.086, 0.086, 0.143, 10.0), -1);
    CHECK_EQ_INT(rb_cyl_geom_init(&bad, NAN, 0.086, 0.143, 10.0), -1);
}

void test_compression_matches_isentrope(void)
{
    rb_cylinder c;
    compress(&c, 3000.0);
    const double V_bdc = c.geom.v_clear + c.geom.v_disp, V_tdc = c.geom.v_clear;
    const double T_exact = isentrope_T(&c.gas.gas, 300.0, V_bdc, V_tdc);
    const double p_exact = 1e5 * (V_bdc / V_tdc) * (T_exact / 300.0);   /* p V / T is constant */
    const double p_textbook = 1e5 * pow(10.0, 1.4);                     /* constant gamma: 25.1 bar */

    /* The trapezoidal rule is second order: about 1e-6 off at 3000 rpm. */
    CHECK_NEAR(c.gas.T, T_exact, 2e-6 * T_exact);
    CHECK_NEAR(c.gas.p, p_exact, 2e-6 * p_exact);
    /* The textbook p0 CR^gamma with gamma = 1.4 is within a few %, but a bit
     * high: cp rises as the gas heats up, so gamma falls during compression. */
    CHECK_NEAR(c.gas.p, p_textbook, 0.05 * p_textbook);
    CHECK(c.gas.p < p_textbook);
    CHECK_EQ_INT(c.gas.repairs, 0);
}

void test_compression_converges_with_step_size(void)
{
    /* With no heat loss and no leaks only the volume matters, not how fast it
     * changes; what's left is the step-size error of the trapezoidal rule.
     * It's second order: doubling the step (the rpm) quadruples the error. */
    rb_cylinder c;
    double err[3];
    const double rpm[3] = { 3000.0, 6000.0, 12000.0 };
    for (int k = 0; k < 3; k++) {
        compress(&c, rpm[k]);
        double T_exact = isentrope_T(&c.gas.gas, 300.0, c.geom.v_clear + c.geom.v_disp, c.geom.v_clear);
        err[k] = fabs(c.gas.T - T_exact) / T_exact;
    }
    CHECK(err[1] / err[0] > 3.5 && err[1] / err[0] < 4.5);
    CHECK(err[2] / err[1] > 3.5 && err[2] / err[1] < 4.5);
    CHECK(err[2] < 3e-5);   /* even at 12000 rpm (480 steps per stroke): 0.003 % at most */
}

void test_motoring_has_no_energy_drift(void)
{
    rb_cyl_geom g = engine_86x86();
    rb_cylinder c;
    const long steps_per_rev = lround(2.0 * PI / (3000.0 / 60.0 * 2.0 * PI * DT));   /* 3000 rpm */
    double U_min = INFINITY, U_max = -INFINITY, peak_first = 0.0, peak_last = 0.0, worst_net = 0.0;

    rb_cylinder_init(&c, &g, PI, 1e5, 300.0, AIR);   /* BDC */
    const double U0 = c.gas.U;
    for (int rev = 0; rev < 100; rev++) {
        double p_max = 0.0, net_work = 0.0;
        for (long i = 1; i <= steps_per_rev; i++) {
            /* angles from the step count, so every revolution ends exactly at BDC */
            net_work += rb_cylinder_move(&c, PI + 2.0 * PI * (rev + (double)i / (double)steps_per_rev));
            p_max = fmax(p_max, c.gas.p);
            U_min = fmin(U_min, c.gas.U);
            U_max = fmax(U_max, c.gas.U);
        }
        if (rev == 0) peak_first = p_max;
        peak_last = p_max;
        worst_net = fmax(worst_net, fabs(net_work));
    }
    /* Compared with the energy that goes in and out every stroke (~200 J): */
    const double swing = U_max - U_min;
    CHECK(swing > 100.0);
    CHECK_NEAR(c.gas.U, U0, 1e-9 * swing);                   /* back where it started ...  */
    CHECK_NEAR(peak_last, peak_first, 1e-9 * peak_first);    /* ... every revolution ...   */
    CHECK(worst_net < 1e-9 * swing);                          /* ... with no net work       */
    CHECK_NEAR(c.gas.T, 300.0, 1e-6);
}

void test_volume_step_is_time_symmetric(void)
{
    /* Squeeze and release a chamber along an irregular path, then retrace the
     * path backwards: the trapezoidal rule must land exactly on the start. */
    static const double PATH[8] = { 1.0, 0.7, 0.45, 0.3, 0.33, 0.2, 0.12, 0.18 };
    static const double BURNED[RB_NSPECIES] = { [RB_BURNED] = 1.0 };
    rb_chamber ch, empty, atmosphere;
    double total = 0.0;

    rb_chamber_init(&ch, 1e-3, 3e5, 1800.0, BURNED);
    const double U0 = ch.U, T0 = ch.T;
    for (int i = 1; i < 8; i++) total += rb_chamber_set_volume(&ch, 1e-3 * PATH[i]);
    CHECK(ch.T > T0);                                 /* compressed: hotter */
    for (int i = 6; i >= 0; i--) total += rb_chamber_set_volume(&ch, 1e-3 * PATH[i]);
    CHECK_NEAR(ch.U, U0, 1e-12 * fabs(U0));
    CHECK_NEAR(ch.T, T0, 1e-9);
    CHECK(fabs(total) < 1e-9);

    /* Nothing to compress, or a nonsense volume: no work and no NaN. */
    rb_chamber_init(&empty, 1e-3, 0.0, 300.0, AIR);
    CHECK(rb_chamber_set_volume(&empty, 5e-4) == 0.0 && empty.V == 5e-4 && empty.p == 0.0);
    rb_chamber_init_reservoir(&atmosphere, 1e5, 300.0, AIR);
    CHECK(rb_chamber_set_volume(&atmosphere, 0.5) == 0.0 && atmosphere.p == 1e5);
    CHECK(rb_chamber_set_volume(&ch, 0.0) == 0.0 && rb_chamber_set_volume(&ch, NAN) == 0.0);
    CHECK(isfinite(ch.p) && ch.V > 0.0);
}

void test_big_volume_jump_is_split(void)
{
    /* Squeezed tenfold in one call. The trapezoidal rule alone has no
     * solution for that (the gas would need a negative temperature), so the
     * jump is split into 2 % steps and still follows the isentrope closely. */
    rb_chamber ch;
    rb_chamber_init(&ch, 1e-3, 1e5, 300.0, AIR);
    const double U0 = ch.U;
    const double T_exact = isentrope_T(&ch.gas, 300.0, 1e-3, 1e-4);

    double w = rb_chamber_set_volume(&ch, 1e-4);
    CHECK_NEAR(ch.T, T_exact, 2e-4 * T_exact);
    CHECK_NEAR(w, ch.U - U0, 1e-12);
    w += rb_chamber_set_volume(&ch, 1e-3);     /* and back, also in one call */
    CHECK_NEAR(ch.T, 300.0, 1e-9);
    CHECK(fabs(w) < 1e-9);
    CHECK_EQ_INT(ch.repairs, 0);
}

void test_cylinder_bad_input(void)
{
    rb_cyl_geom bad = engine_86x86(), g = engine_86x86();
    rb_cylinder c;

    bad.rod = 0.01;                                   /* shorter than the stroke */
    CHECK_EQ_INT(rb_cylinder_init(&c, &bad, 0.0, 1e5, 300.0, AIR), -1);
    CHECK_EQ_INT(rb_cylinder_init(&c, NULL, 0.0, 1e5, 300.0, AIR), -1);

    CHECK_EQ_INT(rb_cylinder_init(&c, &g, PI, 1e5, 300.0, AIR), 0);
    const double U = c.gas.U;
    CHECK(rb_cylinder_move(&c, NAN) == 0.0);          /* ignored */
    CHECK_NEAR(c.gas.U, U, 0.0);
    CHECK_NEAR(c.theta, PI, 0.0);
}
