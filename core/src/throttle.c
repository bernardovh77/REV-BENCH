/*
 * throttle.c - butterfly throttle open area (see throttle.h).
 */
#include "throttle.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

int rb_throttle_check(const rb_throttle *t)
{
    /* Written so NaN fails every test. */
    if (!t) return -1;
    if (!(t->diameter > 0.0 && t->diameter < 1.0)) return -1;
    if (!(t->shaft >= 0.0 && t->shaft < 0.5 * t->diameter)) return -1;
    if (!(t->closed_angle >= 0.0 && t->closed_angle < 45.0)) return -1;
    if (!(t->cd > 0.0 && t->cd <= 1.0)) return -1;
    if (!(t->leak_area >= 0.0 && t->leak_area < 0.25 * PI * t->diameter * t->diameter)) return -1;
    return 0;
}

double rb_throttle_angle(const rb_throttle *t, double position)
{
    if (!(position > 0.0)) position = 0.0;   /* NaN too */
    if (position > 1.0) position = 1.0;
    return t->closed_angle + position * (90.0 - t->closed_angle);
}

/* Area of the strip |y| < h inside an ellipse with half-width R (along the
 * shaft) and half-height b: 2 R [ (h/b) sqrt(b^2 - h^2) + b asin(h/b) ].
 * A circle is the case b = R. */
static double strip_in_ellipse(double R, double b, double h)
{
    if (h >= b) return PI * R * b;   /* the whole ellipse is inside the strip */
    return 2.0 * R * (h / b * sqrt(b * b - h * h) + b * asin(h / b));
}

double rb_throttle_open_area(const rb_throttle *t, double angle_deg)
{
    const double R = 0.5 * t->diameter, h = 0.5 * t->shaft;
    const double psi = angle_deg * PI / 180.0, psi0 = t->closed_angle * PI / 180.0;
    double b = R * cos(psi) / cos(psi0);   /* plate ellipse half-height */
    if (b > R) b = R;                      /* below the closed angle: shut */
    if (b < 0.0) b = 0.0;

    double area = PI * R * R - PI * R * b - strip_in_ellipse(R, R, h);
    if (b > 0.0) area += strip_in_ellipse(R, b, h);
    return area > 0.0 ? area : 0.0;
}

double rb_throttle_area(const rb_throttle *t, double position)
{
    return t->cd * rb_throttle_open_area(t, rb_throttle_angle(t, position)) + t->leak_area;
}
