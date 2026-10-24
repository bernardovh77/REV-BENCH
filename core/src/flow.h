/*
 * flow.h - gas flowing through a restriction (a valve, the throttle, a hole
 * between two volumes), modelled as an isentropic nozzle.
 *
 * Upstream state (p0, T0) is taken as stagnation (the gas in a chamber is at
 * rest). With pressure ratio r = p / p0 across the restriction:
 *
 *   subsonic, r > r_crit:
 *     mdot = Cd A p0 / sqrt(R T0) * sqrt( 2g/(g-1) * (r^(2/g) - r^((g+1)/g)) )
 *   choked, r <= r_crit:
 *     mdot = Cd A p0 / sqrt(R T0) * sqrt(g) * (2/(g+1))^((g+1)/(2(g-1)))
 *
 * r_crit = (2/(g+1))^(g/(g-1)) (0.528 for air) is where the gas in the
 * narrowest point reaches the speed of sound; from there on the flow no
 * longer depends on the downstream pressure.
 */
#ifndef RB_FLOW_H
#define RB_FLOW_H

#include "chamber.h"

/* Fraction of the "equalising mass" one connection may move per step (see
 * rb_orifice_flow); 0.5 halves the pressure difference per step at most. */
#define RB_FLOW_LIMIT 0.5

/* Pressure ratio p/p0 below which the flow is choked. */
double rb_nozzle_critical_ratio(double gamma);

/* Mass flow (kg/s) from (p0, T0) to p through an effective area cd_area =
 * Cd * A (m^2), for a gas with constant R and ratio gamma. 0 if p >= p0 or
 * an input is invalid. */
double rb_nozzle_mdot(double cd_area, double p0, double T0, double R, double gamma, double p);

/* A restriction between two chambers. Positive flow goes from a to b. */
typedef struct rb_orifice {
    rb_chamber *a;
    rb_chamber *b;
    double area;    /* m^2, geometric                                 */
    double cd;      /* discharge coefficient, 0..1                    */
    double mdot;    /* kg/s, signed result of the last step           */
    int    limited; /* 1 if the equalisation limit was active then    */
    struct rb_diag_slot *diag;
} rb_orifice;

void rb_orifice_init(rb_orifice *o, rb_chamber *a, rb_chamber *b, double area, double cd);

/* Computes the flow from the chambers' current states and adds it to both
 * chambers' rates. Returns the signed mass flow (kg/s). */
double rb_orifice_flow(rb_orifice *o, double dt);

/* One explicit time step of a small network: clear rates, compute every
 * flow from the current states, then integrate every chamber. */
void rb_network_step(rb_chamber *const *chambers, int n_chambers,
                     rb_orifice *orifices, int n_orifices, double dt);

#endif /* RB_FLOW_H */
