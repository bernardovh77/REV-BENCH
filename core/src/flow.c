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

    if (r <= rb_nozzle_critical_ratio(g))   /* choked */
        return scale * sqrt(g) * pow(2.0 / (g + 1.0), (g + 1.0) / (2.0 * (g - 1.0)));

    /* subsonic: r^(2/g) - r^((g+1)/g), using one pow: a = r^(1/g) */
    const double a = pow(r, 1.0 / g);
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
        k += up->c * up->c / up->V;
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
        mdot = rb_nozzle_mdot(o->cd * o->area, up->p, up->T, up->gas.R, up->gamma, down->p);

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

void rb_duct_init(rb_duct *d, rb_chamber *a, rb_chamber *b, double area, double length,
                  double loss)
{
    d->a = a;
    d->b = b;
    d->area = area;
    d->length = length;
    d->loss = loss;
    d->mdot = 0.0;
}

double rb_duct_flow(rb_duct *d, double dt)
{
    if (!(d->mdot == d->mdot)) d->mdot = 0.0;   /* NaN: start again from rest */
    const int forward = d->mdot >= 0.0;
    rb_chamber *up = forward ? d->a : d->b, *down = forward ? d->b : d->a;
    double m = fabs(d->mdot);
    if (!(up->mass > 0.0)) m = 0.0;
    else if (dt > 0.0 && m * dt > 0.5 * up->mass) m = 0.5 * up->mass / dt;
    rb_chamber_add_flow(up, -m, up->y, up->h);
    rb_chamber_add_flow(down, m, up->y, up->h);
    return forward ? m : -m;
}

void rb_duct_accelerate(rb_duct *d, double dt)
{
    const rb_chamber *up = (d->mdot >= 0.0) ? d->a : d->b;
    const double rho = (up->rho > 1e-6) ? up->rho : 1e-6;
    const double u = d->mdot / (rho * d->area);
    const double dp = d->a->p - d->b->p - d->loss * 0.5 * rho * u * fabs(u);
    const double mdot = d->mdot + dt * d->area / d->length * dp;
    d->mdot = (mdot == mdot) ? mdot : 0.0;
}
