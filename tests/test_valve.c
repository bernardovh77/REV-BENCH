/*
 * test_valve.c - cam lift profile and valve flow area.
 */
#include "rb_test.h"
#include "valve.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

/* Two 33 mm intake valves, 9.5 mm lift, 250 degrees, centerline 110 degrees
 * after overlap TDC (cycle angle 470). */
static rb_valve intake(void)
{
    rb_valve v = { 2, 0.033, 0.85 * 0.033, 0.0095, 250.0, 470.0, 0.7 };
    return v;
}

void test_valve_lift_profile(void)
{
    rb_valve v = intake(), wrap = intake();
    int wrong = 0;

    CHECK_EQ_INT(rb_valve_check(&v), 0);
    CHECK_NEAR(rb_valve_lift(&v, 470.0), 0.0095, 1e-15);   /* full lift at the centerline */
    CHECK_NEAR(rb_valve_opens(&v), 345.0, 1e-12);           /* 15 degrees before overlap TDC */
    CHECK_NEAR(rb_valve_closes(&v), 595.0, 1e-12);          /* 55 degrees after BDC */
    CHECK(rb_valve_lift(&v, 345.0) == 0.0 && rb_valve_lift(&v, 595.0) == 0.0);
    CHECK(rb_valve_lift(&v, 345.5) > 0.0 && rb_valve_lift(&v, 594.5) > 0.0);
    CHECK(rb_valve_lift(&v, 0.0) == 0.0 && rb_valve_lift(&v, 180.0) == 0.0);   /* shut while firing */
    CHECK(rb_valve_lift(&v, NAN) == 0.0);

    for (double d = 0.0; d < 130.0; d += 0.7) {
        const double L = rb_valve_lift(&v, 470.0 + d);
        if (fabs(L - rb_valve_lift(&v, 470.0 - d)) > 1e-15) wrong++;            /* symmetric */
        if (fabs(L - rb_valve_lift(&v, 470.0 + d + 720.0)) > 1e-15) wrong++;    /* periodic  */
        if (fabs(L - rb_valve_lift(&v, 470.0 + d - 1440.0)) > 1e-15) wrong++;
        if (d > 0.0 && L > rb_valve_lift(&v, 470.0 + d - 0.7)) wrong++;         /* falls after the peak */
    }
    CHECK_EQ_INT(wrong, 0);

    /* It leaves the seat gently: lift grows with the square of the angle. */
    CHECK_NEAR(rb_valve_lift(&v, 347.0) / rb_valve_lift(&v, 346.0), 4.0, 0.05);

    /* At 1.27 mm (0.050 in) of lift it's open for about 80 % of its duration,
     * like a real street cam. */
    double open050 = 0.0;
    for (double th = 300.0; th < 640.0; th += 0.01)
        if (rb_valve_lift(&v, th) >= 0.00127) open050 += 0.01;
    CHECK(open050 / v.duration > 0.76 && open050 / v.duration < 0.84);

    /* A lobe across the end of the cycle: centred at 10 degrees, 60 long. */
    wrap.center = 10.0;
    wrap.duration = 60.0;
    CHECK_NEAR(rb_valve_opens(&wrap), 700.0, 1e-12);
    CHECK_NEAR(rb_valve_closes(&wrap), 40.0, 1e-12);
    CHECK_NEAR(rb_valve_lift(&wrap, 710.0), rb_valve_lift(&wrap, 30.0), 1e-15);
    CHECK(rb_valve_lift(&wrap, 705.0) > 0.0 && rb_valve_lift(&wrap, 695.0) == 0.0);
}

void test_valve_flow_area(void)
{
    rb_valve v = intake(), bad;
    const double d = v.diameter;
    const double curtain_cd = 0.7 * 2.0 * PI * d;   /* effective curtain area per m of lift */
    const double ports = 2.0 * RB_PORT_CD * 0.25 * PI * v.port_diameter * v.port_diameter;
    int wrong = 0;

    CHECK(rb_valve_area(&v, 0.0) == 0.0 && rb_valve_area(&v, -1e-3) == 0.0
          && rb_valve_area(&v, NAN) == 0.0);
    CHECK_NEAR(rb_valve_area(&v, 1e-6), curtain_cd * 1e-6, curtain_cd * 1e-6 * 1e-6);   /* curtain */
    CHECK_NEAR(rb_valve_area(&v, 1.0), ports, ports * 1e-3);                            /* ports   */

    /* Measured as a fraction of the geometric curtain area pi d L, the
     * discharge coefficient falls from 0.68 at L = d/20 to 0.48 at L = d/4,
     * like the intake valve data in Heywood, chapter 6. */
    CHECK_NEAR(rb_valve_area(&v, d / 20.0) / (2.0 * PI * d * d / 20.0), 0.68, 0.01);
    CHECK_NEAR(rb_valve_area(&v, d / 4.0) / (2.0 * PI * d * d / 4.0), 0.48, 0.01);

    for (double L = 1e-4; L < 0.02; L += 1e-4)
        if (!(rb_valve_area(&v, L) > rb_valve_area(&v, L - 1e-4))) wrong++;   /* grows with lift */
    CHECK_EQ_INT(wrong, 0);

    bad = v; bad.count = 0;                   CHECK_EQ_INT(rb_valve_check(&bad), -1);
    bad = v; bad.port_diameter = 1.1 * d;     CHECK_EQ_INT(rb_valve_check(&bad), -1);
    bad = v; bad.max_lift = 0.0;              CHECK_EQ_INT(rb_valve_check(&bad), -1);
    bad = v; bad.duration = 400.0;            CHECK_EQ_INT(rb_valve_check(&bad), -1);
    bad = v; bad.center = NAN;                CHECK_EQ_INT(rb_valve_check(&bad), -1);
    bad = v; bad.center = 720.0;              CHECK_EQ_INT(rb_valve_check(&bad), -1);
    bad = v; bad.cd = 0.0;                    CHECK_EQ_INT(rb_valve_check(&bad), -1);
    CHECK_EQ_INT(rb_valve_check(NULL), -1);
}
