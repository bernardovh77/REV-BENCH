/*
 * test_throttle.c - butterfly throttle geometry.
 */
#include "rb_test.h"
#include "throttle.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

/* A 60 mm throttle body with an 8 mm shaft, closing at 7 degrees. */
static rb_throttle body(void)
{
    rb_throttle t = { 0.060, 0.008, 7.0, 0.8, 2e-6 };
    return t;
}

/* The open area integrated numerically, slice by slice across the bore: at
 * height y the bore is 2 sqrt(R^2 - y^2) wide, the plate covers
 * 2 R sqrt(1 - y^2/b^2) of it, and the shaft covers everything at |y| < h. */
static double sliced_area(const rb_throttle *t, double angle_deg)
{
    const double R = 0.5 * t->diameter, h = 0.5 * t->shaft;
    const double b = R * cos(angle_deg * PI / 180.0) / cos(t->closed_angle * PI / 180.0);
    const int n = 400000;
    const double dy = 2.0 * R / n;
    double area = 0.0;
    for (int i = 0; i < n; i++) {
        const double y = -R + (i + 0.5) * dy;
        if (fabs(y) < h) continue;
        double open = 2.0 * sqrt(R * R - y * y);
        if (fabs(y) < b) open -= 2.0 * R * sqrt(1.0 - y * y / (b * b));
        area += open * dy;
    }
    return area;
}

void test_throttle_geometry(void)
{
    rb_throttle t = body(), bare = body();
    const double R = 0.03, h = 0.004;
    const double wide_open = PI * R * R - 2.0 * (h * sqrt(R * R - h * h) + R * R * asin(h / R));
    int wrong = 0;

    CHECK(rb_throttle_open_area(&t, 7.0) < 1e-15);          /* closed: shut */
    CHECK(rb_throttle_open_area(&t, 3.0) == 0.0);            /* can't close further */
    CHECK_NEAR(rb_throttle_open_area(&t, 90.0), wide_open, 1e-15);   /* only the shaft is left */

    /* Matches the slice-by-slice integration from nearly shut to wide open. */
    const double angles[] = { 8.0, 10.0, 20.0, 35.0, 50.0, 70.0, 85.0, 89.0 };
    for (int i = 0; i < 8; i++) {
        double exact = sliced_area(&t, angles[i]);
        if (fabs(rb_throttle_open_area(&t, angles[i]) - exact) > 1e-4 * exact) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);

    /* Without a shaft it's just the bore minus the plate's shadow. */
    bare.shaft = 0.0;
    CHECK_NEAR(rb_throttle_open_area(&bare, 40.0),
               PI * R * R * (1.0 - cos(40.0 * PI / 180.0) / cos(7.0 * PI / 180.0)), 1e-15);

    /* It opens steadily until the plate, seen edge-on, hides behind the shaft
     * (cos(angle) < h cos(7) / R, at 82.4 degrees); after that nothing changes. */
    for (double a = 7.1; a <= 90.0; a += 0.1) {
        const double now = rb_throttle_open_area(&t, a), before = rb_throttle_open_area(&t, a - 0.1);
        if (a < 82.3 && !(now > before)) wrong++;
        if (a > 82.6 && !(now == before)) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);
}

void test_throttle_pedal(void)
{
    rb_throttle t = body(), bad;
    int wrong = 0;

    CHECK_EQ_INT(rb_throttle_check(&t), 0);
    CHECK_NEAR(rb_throttle_angle(&t, 0.0), 7.0, 1e-12);
    CHECK_NEAR(rb_throttle_angle(&t, 0.5), 48.5, 1e-12);
    CHECK_NEAR(rb_throttle_angle(&t, 1.0), 90.0, 1e-12);
    CHECK_NEAR(rb_throttle_angle(&t, NAN), 7.0, 1e-12);     /* NaN: closed */
    CHECK_NEAR(rb_throttle_angle(&t, -1.0), 7.0, 1e-12);
    CHECK_NEAR(rb_throttle_angle(&t, 2.0), 90.0, 1e-12);

    CHECK_NEAR(rb_throttle_area(&t, 0.0), 2e-6, 1e-18);     /* only the leak */
    CHECK_NEAR(rb_throttle_area(&t, 1.0), 0.8 * rb_throttle_open_area(&t, 90.0) + 2e-6, 1e-15);
    for (double x = 0.01; x <= 1.0; x += 0.01)   /* the last 9 % of travel changes nothing */
        if (!(rb_throttle_area(&t, x) >= rb_throttle_area(&t, x - 0.01))) wrong++;
    CHECK_EQ_INT(wrong, 0);

    bad = t; bad.shaft = 0.031;         CHECK_EQ_INT(rb_throttle_check(&bad), -1);
    bad = t; bad.closed_angle = 50.0;   CHECK_EQ_INT(rb_throttle_check(&bad), -1);
    bad = t; bad.cd = 0.0;              CHECK_EQ_INT(rb_throttle_check(&bad), -1);
    bad = t; bad.leak_area = -1e-6;     CHECK_EQ_INT(rb_throttle_check(&bad), -1);
    bad = t; bad.diameter = NAN;        CHECK_EQ_INT(rb_throttle_check(&bad), -1);
    CHECK_EQ_INT(rb_throttle_check(NULL), -1);
}
