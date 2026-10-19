/*
 * chamber.h - a 0D control volume: a lump of gas with one pressure and one
 * temperature. Cylinders, plenums, pipe cells and the outside air are all
 * chambers.
 *
 * The integrator advances only the species masses m[i] (kg) and the sensible
 * internal energy U (J). Everything else (T, p, ...) follows from them and
 * the volume V, in rb_chamber_update().
 *
 * During a time step, flows and heat are collected as rates and then applied
 * together by rb_chamber_integrate(), so the order in which connections are
 * processed doesn't matter. The first law for an open system, with mdot > 0
 * meaning into the chamber:
 *
 *     dm_i/dt = sum over flows of  mdot * y_i(upstream)
 *     dU/dt   = sum over flows of  mdot * h(upstream)  +  Qdot  -  p dV/dt
 *
 * The last term, the work of a moving piston, is applied separately by
 * rb_chamber_set_volume() after the flows (see there).
 */
#ifndef RB_CHAMBER_H
#define RB_CHAMBER_H

#include "gas.h"

typedef struct rb_chamber {
    /* ---- state, advanced by the integrator ---- */
    double m[RB_NSPECIES];   /* kg of each species                             */
    double U;                /* J, sensible internal energy                    */
    double V;                /* m^3                                             */
    int    fixed;            /* 1 = reservoir (e.g. the atmosphere): never changes */

    /* ---- derived by rb_chamber_update() ---- */
    double mass;             /* kg                                              */
    double y[RB_NSPECIES];   /* mass fractions                                  */
    rb_gas gas;              /* mixture properties                              */
    double gas_y[RB_NSPECIES]; /* the mass fractions `gas` was last built from   */
    double T;                /* K                                               */
    double p;                /* Pa                                              */
    double rho;              /* kg/m^3                                          */
    double u;                /* J/kg, specific internal energy                  */
    double h;                /* J/kg, specific enthalpy: what an outflow carries */
    double cv;               /* J/(kg K)                                        */
    double gamma;            /* cp/cv                                           */
    double c;                /* m/s, speed of sound                             */
    int    repairs;          /* times a NaN/negative state had to be repaired   */

    /* ---- rates collected during the current step ---- */
    double dm[RB_NSPECIES];  /* kg/s */
    double dU;               /* W    */
} rb_chamber;

/* A volume V (m^3) filled with gas of composition y at pressure p and temperature T. */
void rb_chamber_init(rb_chamber *ch, double V, double p, double T, const double y[RB_NSPECIES]);

/* A reservoir: infinite, so flows never change its p, T or composition. */
void rb_chamber_init_reservoir(rb_chamber *ch, double p, double T, const double y[RB_NSPECIES]);

/* Recomputes the derived values (T, p, ...) from m[], U and V. */
void rb_chamber_update(rb_chamber *ch);

/* Rates: clear at the start of a step, then add flows (mdot > 0 = in, with
 * the composition y and enthalpy h of the gas that moves) and heat (W).
 * Small and called for every chamber and pipe cell every step, so they live
 * here, where every file can inline them. */
static inline void rb_chamber_clear_rates(rb_chamber *ch)
{
    for (int s = 0; s < RB_NSPECIES; s++) ch->dm[s] = 0.0;
    ch->dU = 0.0;
}

static inline void rb_chamber_add_flow(rb_chamber *ch, double mdot, const double y[RB_NSPECIES],
                                       double h)
{
    for (int s = 0; s < RB_NSPECIES; s++) ch->dm[s] += mdot * y[s];
    ch->dU += mdot * h;
}

static inline void rb_chamber_add_heat(rb_chamber *ch, double qdot)
{
    ch->dU += qdot;
}

/* Applies the collected rates over dt (forward Euler) and updates. */
void rb_chamber_integrate(rb_chamber *ch, double dt);

/* Moves dm kg of gas (negative: takes it away) of composition y carrying
 * enthalpy h straight into the chamber's masses and energy, without waiting
 * for the end of the step. Its total mass follows at once; T, p and the rest
 * only at its next rb_chamber_update or rb_chamber_integrate. For a chamber
 * stepped less often than the one it trades gas with (stage 10's pipes): the
 * books stay exact at every step. */
static inline void rb_chamber_add_mass(rb_chamber *ch, double dm, const double y[RB_NSPECIES],
                                       double h)
{
    if (ch->fixed) return;   /* reservoirs never change */
    for (int s = 0; s < RB_NSPECIES; s++) ch->m[s] += dm * y[s];
    ch->U += dm * h;
    ch->mass += dm;
}

/* The pressure (Pa) the chamber has now, to first order, if the rates it
 * has collected have been running for `since` seconds since its last update
 * and rb_chamber_add_mass has moved gas in or out meanwhile: for a chamber
 * stepped less often than its neighbour (stage 10's pipes), the pressure the
 * neighbour should see. Just p when nothing waits (since = 0, no transfers).
 * At the update it held rho V kg with u J/kg; since then dm kg and dU J more,
 * and p V / R = m T changes by T dm + (dU - u dm) / cv. */
static inline double rb_chamber_pressure_now(const rb_chamber *ch, double since)
{
    double dm = ch->mass - ch->rho * ch->V;
    for (int s = 0; s < RB_NSPECIES; s++) dm += since * ch->dm[s];
    const double dU = ch->U - ch->u * ch->rho * ch->V + since * ch->dU;
    return ch->p + ch->gas.R / ch->V * (ch->T * dm + (dU - ch->u * dm) / ch->cv);
}

/* Changes the volume (a piston moving) with no flow or heat during the change,
 * so dU = -p dV. Integrated with the trapezoidal rule, which is symmetric in
 * time: compressing and then expanding back through the same volumes returns
 * exactly to the starting state, so a closed cylinder neither gains nor loses
 * energy over thousands of cycles. Changes of more than 2 % are split into
 * smaller steps. Returns the work done on the gas (J): positive when
 * compressed, negative when the gas pushes the piston. */
double rb_chamber_set_volume(rb_chamber *ch, double V_new);

#endif /* RB_CHAMBER_H */
