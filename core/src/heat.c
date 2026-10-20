/*
 * heat.c - Woschni wall heat transfer (see heat.h).
 */
#include "heat.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

double rb_woschni_h(double bore, double p, double T, double w)
{
    if (!(bore > 0.0 && p > 0.0 && T > 0.0 && w > 0.0)) return 0.0;
    const double p_kpa = p * 1e-3;
    /* B^-0.2 (p w)^0.8 T^-0.55 as one exp of logs: a pow is a log and an exp
     * of its own, and this runs for every cylinder every step. */
    return 3.26 * exp(0.8 * log(p_kpa * w) - 0.55 * log(T) - 0.2 * log(bore));
}

double rb_woschni_w(double piston_speed, int valves_open, int burning_phase,
                    double v_disp, double p, double p_r, double V_r, double T_r,
                    double V)
{
    const double c1 = valves_open ? 6.18 : 2.28;
    double w = c1 * piston_speed;
    if (burning_phase && !valves_open && p_r > 0.0 && V_r > 0.0 && V > 0.0) {
        const double p_motored = p_r * pow(V_r / V, 1.3);
        const double extra = 3.24e-3 * v_disp * T_r / (p_r * V_r) * (p - p_motored);
        if (extra > 0.0) w += extra;
    }
    return w;
}

double rb_wall_area(const rb_cyl_geom *g, double x)
{
    const double clearance_height = g->v_clear / g->area;
    return 2.0 * g->area + PI * g->bore * (clearance_height + x);
}
