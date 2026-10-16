/*
 * test_engine.c - the breathing engine: conservation, steady state, the
 * throttle, choked flow, pumping loss, and no step-to-step oscillation.
 */
#include "engine.h"
#include "rb_test.h"

#include <math.h>

#define DT (1.0 / 192000.0)   /* the default physics step */

static rb_engine e;           /* about 10 KB: kept off the stack */

/* The default 2.0 L four, from rest, at the given speed and throttle. */
static void start(double rpm, double throttle)
{
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    rb_engine_init(&e, &d, DT);
    e.rpm = rpm;
    e.throttle_pos = throttle;
}

static double ambient_density(void)
{
    return e.desc.p_ambient / (e.intake_air.gas.R * e.desc.T_ambient);
}

void test_engine_description(void)
{
    rb_engine_desc d, bad;
    rb_engine_desc_default(&d);

    CHECK_EQ_INT(rb_engine_desc_check(&d), 0);
    CHECK_EQ_INT(rb_engine_init(&e, &d, DT), 0);
    CHECK_NEAR(rb_engine_displacement(&e), 4 * 499.557e-6, 1e-9);   /* a 2.0 L four */
    CHECK_NEAR(rb_engine_cylinder_angle(&e, 1), 180.0, 1e-12);       /* fires last: 540 after cylinder 1 */
    CHECK_NEAR(e.plenum.p, d.p_ambient, 1e-9 * d.p_ambient);         /* everything starts at ambient */
    CHECK_NEAR(e.cyl[3].gas.T, d.T_ambient, 1e-9);

    bad = d; bad.cylinders = 0;                CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.cylinders = RB_MAX_CYLINDERS + 1; CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.firing[2] = 720.0;            CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.firing[3] = NAN;              CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.rod = 0.05;                   CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.intake.diameter = 0.05;       /* four valves no longer fit in the bore */
    bad.intake.port_diameter = 0.04;           CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.exhaust.duration = 0.0;       CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.throttle.cd = 2.0;            CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.plenum_volume = 0.0;          CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.p_ambient = NAN;              CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    CHECK_EQ_INT(rb_engine_desc_check(NULL), -1);
    CHECK_EQ_INT(rb_engine_init(&e, &bad, DT), -1);
    CHECK_EQ_INT(rb_engine_init(&e, &d, 0.0), -1);
    CHECK_EQ_INT(rb_engine_init(&e, &d, NAN), -1);
}

void test_engine_pistons_follow_the_crank(void)
{
    /* After any number of steps, every piston is exactly where its crank
     * angle puts it: the gas volume is V(theta - firing). */
    int wrong = 0;
    start(4321.0, 0.5);
    for (int k = 0; k < 12345; k++) rb_engine_step(&e);
    for (int i = 0; i < 4; i++) {
        const double theta = rb_engine_cylinder_angle(&e, i) * 3.14159265358979323846 / 180.0;
        const double V = rb_cyl_volume(&e.cyl[i].geom, theta);
        if (fabs(e.cyl[i].gas.V - V) > 1e-12 * V) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK(e.theta >= 0.0 && e.theta < 720.0);
}

void test_engine_conserves_mass_and_energy(void)
{
    /* Part throttle from rest: the plenum is still pumping down, so a lot is
     * changing. Every kg and every J must be accounted for. */
    double m0 = 0.0, u0 = 0.0, m1 = 0.0, u1 = 0.0;
    int repairs = 0;

    start(3000.0, 0.1);
    m0 = e.plenum.mass;
    u0 = e.plenum.U;
    for (int i = 0; i < 4; i++) {
        m0 += e.cyl[i].gas.mass;
        u0 += e.cyl[i].gas.U;
    }
    for (int k = 0; k < 3 * 7680; k++) rb_engine_step(&e);   /* three cycles */
    m1 = e.plenum.mass;
    u1 = e.plenum.U;
    repairs = e.plenum.repairs;
    for (int i = 0; i < 4; i++) {
        m1 += e.cyl[i].gas.mass;
        u1 += e.cyl[i].gas.U;
        repairs += e.cyl[i].gas.repairs;
    }
    CHECK(m1 < 0.8 * m0);   /* a good part of the air has been pumped out */
    CHECK_NEAR(m1 - m0, e.mass_in - e.mass_out, 1e-12 * m0);
    CHECK_NEAR(u1 - u0, e.energy_in - e.energy_out + e.work_on_gas, 1e-9 * fabs(e.energy_out));
    CHECK_EQ_INT(repairs, 0);
    CHECK_EQ_INT((int)e.cycles, 3);
}

void test_engine_settles_and_balances(void)
{
    /* Wide open at 3000 rpm: it settles within a few cycles, the air through
     * the throttle equals the air into the cylinders, and the volumetric
     * efficiency is in the range of real naturally aspirated engines. */
    start(3000.0, 1.0);
    const int cycles = rb_engine_settle(&e, 60, 1e-6);
    const rb_engine_cycle *r = &e.result;
    const double expected_flow = r->ve * ambient_density() * rb_engine_displacement(&e) * 3000.0 / 120.0;

    CHECK(cycles > 0 && cycles < 30);
    CHECK(r->ve > 0.85 && r->ve < 0.95);
    CHECK_NEAR(r->air_flow, expected_flow, 1e-6 * expected_flow);
    CHECK(r->map < e.desc.p_ambient && r->map > 0.98 * e.desc.p_ambient);
    CHECK(r->map_min < r->map && r->map < r->map_max);
    CHECK(r->pmep > 0.0);           /* turning it over costs work ... */
    CHECK(r->net_work < 0.0);       /* ... which the motor supplies */
}

void test_engine_cycle_totals_dont_jitter(void)
{
    /* At 7000 rpm a cycle is 3291.4 steps, not a whole number. The step that
     * crosses the end of a cycle is shared between the two cycles, so the
     * totals still settle to a steady value (without sharing, they wobble
     * by about one step in 3291, 3e-4). */
    start(7000.0, 1.0);
    CHECK(rb_engine_settle(&e, 60, 1e-6) > 0);
}

void test_engine_cylinders_breathe_alike(void)
{
    /* An even-firing four: every cylinder sees the same plenum pressure
     * history, 180 degrees apart, so once settled they breathe the same,
     * even at part throttle where the plenum pulses hard. */
    double worst = 0.0;
    start(2000.0, 0.15);
    CHECK(rb_engine_settle(&e, 80, 1e-6) > 0);
    for (int i = 0; i < 4; i++)
        worst = fmax(worst, fabs(e.result.ve_cyl[i] - e.result.ve));
    CHECK(worst < 1e-5 * e.result.ve);
    CHECK(e.result.map_max - e.result.map_min > 0.03 * e.result.map);   /* it does pulse */
}

void test_engine_throttle_response(void)
{
    /* Closing the throttle at 2000 rpm: the plenum pressure and the air drawn
     * in fall, and the pumping loss grows. */
    static const double pos[4] = { 1.0, 0.15, 0.04, 0.0 };
    double map[4], ve[4], pmep[4];
    int wrong = 0;

    for (int k = 0; k < 4; k++) {
        start(2000.0, pos[k]);
        if (rb_engine_settle(&e, 100, 1e-5) < 0) wrong++;
        map[k] = e.result.map;
        ve[k] = e.result.ve;
        pmep[k] = e.result.pmep;
        if (k > 0 && !(map[k] < map[k - 1] && ve[k] < ve[k - 1] && pmep[k] > pmep[k - 1])) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK(map[0] > 0.99 * e.desc.p_ambient);   /* wide open: hardly any vacuum */
    CHECK(map[3] < 0.20 * e.desc.p_ambient);   /* shut: deep vacuum */
    CHECK(ve[3] < 0.05);
}

void test_engine_choked_throttle(void)
{
    /* Nearly shut, the plenum stays below 0.528 bar the whole time, so the
     * throttle is choked: the air flow is fixed by the throttle alone,
     * Cd A p0 sqrt(g / (R T0)) (2/(g+1))^((g+1)/(2(g-1))), whatever the speed. */
    const double g = 1.4, R = 287.1;   /* air near 300 K; stage 3 */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    const double area = rb_throttle_area(&d.throttle, 0.03);
    const double choked = area * d.p_ambient * sqrt(g / (R * d.T_ambient))
                        * pow(2.0 / (g + 1.0), (g + 1.0) / (2.0 * (g - 1.0)));
    const double rpms[2] = { 1500.0, 4000.0 };

    for (int k = 0; k < 2; k++) {
        start(rpms[k], 0.03);
        CHECK(rb_engine_settle(&e, 100, 1e-5) > 0);
        CHECK(e.result.map_max < 0.528 * d.p_ambient);
        CHECK_NEAR(e.result.air_flow, choked, 2e-3 * choked);   /* gamma of real air: 1.4 +- 0.1 % */
    }
}

void test_engine_pumping_loss(void)
{
    /* At low speed the pumping loss is simply the pressure difference the
     * pistons work against: they push out against the atmosphere and pull in
     * against the plenum, so PMEP = p_ambient - MAP (Heywood, chapter 5). */
    start(1000.0, 0.04);
    CHECK(rb_engine_settle(&e, 100, 1e-5) > 0);
    const double expected = e.desc.p_ambient - e.result.map;
    CHECK(expected > 0.5e5);
    CHECK_NEAR(e.result.pmep, expected, 0.02 * expected);
}

void test_engine_pressures_dont_zigzag(void)
{
    /* Explicit flow steps can make a small chamber's pressure flip up and
     * down every step when several openings feed it. Watch every chamber for
     * three changes of direction in a row. */
    static const double cases[3][2] = { { 1200.0, 0.02 }, { 3000.0, 1.0 }, { 6500.0, 1.0 } };
    int zigzags = 0;

    for (int k = 0; k < 3; k++) {
        start(cases[k][0], cases[k][1]);
        rb_engine_settle(&e, 100, 1e-4);
        double prev[5], dprev[5] = { 0 };
        int flips[5] = { 0 };
        for (int j = 0; j < 5; j++) prev[j] = (j < 4) ? e.cyl[j].gas.p : e.plenum.p;
        const long c = e.cycles;
        while (e.cycles == c) {
            rb_engine_step(&e);
            for (int j = 0; j < 5; j++) {
                const double p = (j < 4) ? e.cyl[j].gas.p : e.plenum.p;
                const double dp = p - prev[j];
                flips[j] = (dp * dprev[j] < 0.0) ? flips[j] + 1 : 0;
                if (flips[j] >= 3) zigzags++;
                prev[j] = p;
                dprev[j] = dp;
            }
        }
    }
    CHECK_EQ_INT(zigzags, 0);
}
