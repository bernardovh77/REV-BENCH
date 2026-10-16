/*
 * valve.c - cam lift profile and valve flow area (see valve.h).
 */
#include "valve.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

int rb_valve_check(const rb_valve *v)
{
    /* Written so NaN fails every test. */
    if (!v || v->count < 1 || v->count > 4) return -1;
    if (!(v->diameter > 0.0 && v->diameter < 1.0)) return -1;
    if (!(v->port_diameter > 0.0 && v->port_diameter <= v->diameter)) return -1;
    if (!(v->max_lift > 0.0 && v->max_lift < v->diameter)) return -1;
    if (!(v->duration > 0.0 && v->duration <= 360.0)) return -1;
    if (!(v->center >= 0.0 && v->center < 720.0)) return -1;
    if (!(v->cd > 0.0 && v->cd <= 1.0)) return -1;
    return 0;
}

double rb_cycle_offset(double theta_deg, double center_deg)
{
    double d = fmod(theta_deg - center_deg, 720.0);   /* -720 < d < 720 */
    if (d < -360.0) d += 720.0;
    if (d >= 360.0) d -= 720.0;
    return d;
}

double rb_valve_lift(const rb_valve *v, double theta_deg)
{
    if (!isfinite(theta_deg)) return 0.0;
    const double u = 2.0 * rb_cycle_offset(theta_deg, v->center) / v->duration;
    if (!(fabs(u) < 1.0)) return 0.0;
    const double w = 1.0 - u * u;
    return v->max_lift * w * w;
}

double rb_valve_area(const rb_valve *v, double lift)
{
    if (!(lift > 0.0)) return 0.0;
    const double curtain = v->cd * PI * v->diameter * lift;
    const double port = RB_PORT_CD * 0.25 * PI * v->port_diameter * v->port_diameter;
    /* 1/A^2 = 1/curtain^2 + 1/port^2, written without overflow */
    return v->count * curtain * port / sqrt(curtain * curtain + port * port);
}

double rb_valve_opens(const rb_valve *v)
{
    return fmod(v->center - 0.5 * v->duration + 720.0, 720.0);
}

double rb_valve_closes(const rb_valve *v)
{
    return fmod(v->center + 0.5 * v->duration, 720.0);
}
