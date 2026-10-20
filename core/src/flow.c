/*
 * flow.c - isentropic nozzle flow and orifices between chambers (see flow.h).
 */
#include "flow.h"

#include <math.h>

double rb_nozzle_critical_ratio(double gamma)
{
    return pow(2.0 / (gamma + 1.0), gamma / (gamma - 1.0));
}

double rb_nozzle_mdot(double cd_area, double p0, double T0, double R, double gamma, double p)
{
    /* Written so that NaN inputs also land here and give no flow. */
    if (!(cd_area > 0.0) || !(p0 > 0.0) || !(T0 > 0.0) || !(R > 0.0) || !(gamma > 1.0)
        || !(p < p0))
        return 0.0;

    const double g     = gamma;
    const double r     = (p > 0.0) ? p / p0 : 0.0;
    const double scale = cd_area * p0 / sqrt(R * T0);

    /* The flow goes as sqrt(r^(2/g) - r^((g+1)/g)), which with a = r^(1/g) is
     * sqrt(a^2 - r a). It peaks at the critical ratio, where the gas reaches
     * the speed of sound; below that the flow stays at the peak (choked). Its
     * slope, (a / g)(2 a / r - (g + 1)), is positive below the peak, so the
     * nozzle is choked exactly when 2 a >= (g + 1) r: no second pow for the
     * critical ratio itself. */
    const double a = pow(r, 1.0 / g);
    if (2.0 * a >= (g + 1.0) * r)   /* choked */
        return scale * sqrt(g) * pow(2.0 / (g + 1.0), (g + 1.0) / (2.0 * (g - 1.0)));

    /* subsonic */
    double term = a * a - r * a;
    if (term < 0.0) term = 0.0;             /* rounding near r = 1 */
    return scale * sqrt(2.0 * g / (g - 1.0) * term);
}

void rb_orifice_init(rb_orifice *o, rb_chamber *a, rb_chamber *b, double area, double cd)
{
    o->a = a;
    o->b = b;
    o->area = area;
    o->cd = cd;
    o->mdot = 0.0;
    o->limited = 0;
}

/*
 * The mass (kg) that, moved from `up` to `down`, would make their pressures
 * equal, to first order. Taking dm out of a closed volume lowers its pressure
 * by c^2 dm / V (an isentropic expansion, c = speed of sound); putting dm in
 * with enthalpy h raises the receiver's by R (T + (h - u)/cv) dm / V.
 * Reservoirs don't change, so they add nothing.
 */
static double equalising_mass(const rb_chamber *up, const rb_chamber *down)
{
    double k = 0.0;   /* change of (p_up - p_down) per kg moved, Pa/kg */
    if (!up->fixed)
        k += rb_chamber_gamma(up) * up->gas.R * up->T / up->V;   /* c^2 / V */
    if (!down->fixed) {
        double rise = down->gas.R * (down->T + (up->h - down->u) / down->cv) / down->V;
        if (rise > 0.0) k += rise;
    }
    if (!(k > 0.0)) return INFINITY;   /* between two reservoirs nothing equalises */
    return (up->p - down->p) / k;
}

double rb_orifice_flow(rb_orifice *o, double dt)
{
    rb_chamber *up = o->a, *down = o->b;
    double sign = 1.0, mdot = 0.0;

    if (!(o->cd * o->area > 0.0)) {   /* shut, like most valves most of the time */
        o->mdot = 0.0;
        o->limited = 0;
        return 0.0;
    }

    if (o->b->p > o->a->p) {
        up = o->b;
        down = o->a;
        sign = -1.0;
    }
    o->limited = 0;

    if (up->mass > 0.0 && up->p > down->p) {
        mdot = rb_nozzle_mdot(o->cd * o->area, up->p, up->T, up->gas.R, rb_chamber_gamma(up), down->p);

        /* Explicit time steps overshoot when a big opening joins small volumes:
         * the pressures would cross and oscillate. Never move more than a
         * fraction of the mass that would equalise them. (When this limit is
         * active, the real gas equalises faster than one time step anyway.) */
        if (dt > 0.0) {
            double cap = RB_FLOW_LIMIT * equalising_mass(up, down) / dt;
            if (mdot > cap) {
                mdot = cap;
                o->limited = 1;
            }
        }
    }

    /* The moving gas has the upstream composition and enthalpy. */
    rb_chamber_add_flow(up, -mdot, up->y, up->h);
    rb_chamber_add_flow(down, mdot, up->y, up->h);
    o->mdot = sign * mdot;
    return o->mdot;
}

void rb_network_step(rb_chamber *const *chambers, int n_chambers,
                     rb_orifice *orifices, int n_orifices, double dt)
{
    for (int i = 0; i < n_chambers; i++) rb_chamber_clear_rates(chambers[i]);
    for (int i = 0; i < n_orifices; i++) rb_orifice_flow(&orifices[i], dt);
    for (int i = 0; i < n_chambers; i++) rb_chamber_integrate(chambers[i], dt);
}
