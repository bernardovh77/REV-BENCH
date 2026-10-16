/*
 * chamber.c - the 0D control volume (see chamber.h).
 */
#include "chamber.h"

#include <math.h>
#include <string.h>

/* Below this mass (kg) a chamber counts as empty. */
#define RB_MASS_EPS 1e-18

/* The largest volume ratio taken in one trapezoidal step (see below). */
#define RB_MAX_VOLUME_STEP 1.02

/* Copies y into out as proper mass fractions (all-zero or invalid: pure air). */
static void normalise(const double y[RB_NSPECIES], double out[RB_NSPECIES])
{
    double sum = 0.0;
    for (int s = 0; s < RB_NSPECIES; s++) sum += (y[s] > 0.0) ? y[s] : 0.0;
    for (int s = 0; s < RB_NSPECIES; s++) {
        if (sum > 0.0 && sum < INFINITY) out[s] = (y[s] > 0.0) ? y[s] / sum : 0.0;
        else                             out[s] = (s == RB_AIR) ? 1.0 : 0.0;
    }
}

void rb_chamber_init(rb_chamber *ch, double V, double p, double T, const double y[RB_NSPECIES])
{
    double yn[RB_NSPECIES];
    double mass = 0.0;

    memset(ch, 0, sizeof *ch);
    normalise(y, yn);
    ch->V = (V > 0.0) ? V : 1e-9;
    ch->T = (T > 0.0) ? T : RB_T_REF;
    rb_gas_init(&ch->gas, yn);
    if (p > 0.0) mass = p * ch->V / (ch->gas.R * ch->T);   /* ideal gas: m = pV/(RT) */

    for (int s = 0; s < RB_NSPECIES; s++) {
        ch->y[s] = yn[s];
        ch->gas_y[s] = yn[s];
        ch->m[s] = mass * yn[s];
    }
    ch->U = mass * rb_gas_u(&ch->gas, ch->T);
    rb_chamber_update(ch);
}

void rb_chamber_init_reservoir(rb_chamber *ch, double p, double T, const double y[RB_NSPECIES])
{
    rb_chamber_init(ch, 1.0, p, T, y);   /* the volume only scales the stored mass */
    ch->fixed = 1;
}

/* Rebuilds the mixture's properties, but only if the composition changed:
 * rb_gas_init does a fair amount of arithmetic and most steps don't change
 * the mixture (pure air never does). */
static void remix(rb_chamber *ch)
{
    if (memcmp(ch->y, ch->gas_y, sizeof ch->y) == 0) return;
    rb_gas_init(&ch->gas, ch->y);
    memcpy(ch->gas_y, ch->y, sizeof ch->y);
}

/* Everything that follows from the mass, T, u, V and the composition. */
static void derive(rb_chamber *ch)
{
    ch->p     = ch->mass * ch->gas.R * ch->T / ch->V;   /* ideal gas: p = m R T / V */
    ch->rho   = ch->mass / ch->V;
    ch->h     = ch->u + ch->gas.R * ch->T;
    ch->cv    = rb_gas_cv(&ch->gas, ch->T);
    ch->gamma = (ch->cv + ch->gas.R) / ch->cv;
    ch->c     = sqrt(ch->gamma * ch->gas.R * ch->T);
}

void rb_chamber_update(rb_chamber *ch)
{
    double mass = 0.0;
    int repaired = 0;

    /* NaN/inf guard: a corrupted mass is dropped rather than spreading. */
    for (int s = 0; s < RB_NSPECIES; s++) {
        if (!(ch->m[s] >= 0.0 && ch->m[s] < INFINITY)) {
            ch->m[s] = 0.0;
            repaired = 1;
        }
        mass += ch->m[s];
    }
    ch->mass = mass;

    if (mass > RB_MASS_EPS) {
        for (int s = 0; s < RB_NSPECIES; s++) ch->y[s] = ch->m[s] / mass;
        remix(ch);
        if (!isfinite(ch->U)) {                  /* rebuild from the last temperature */
            ch->U = mass * rb_gas_u(&ch->gas, ch->T);
            repaired = 1;
        }
        ch->u = ch->U / mass;
        ch->T = rb_gas_T_from_u(&ch->gas, ch->u, ch->T);
    } else {
        /* Empty: keep the last composition and temperature. */
        remix(ch);
        if (!(ch->T > 0.0)) ch->T = RB_T_REF;
        ch->u = rb_gas_u(&ch->gas, ch->T);
    }
    derive(ch);
    ch->repairs += repaired;
}

void rb_chamber_clear_rates(rb_chamber *ch)
{
    for (int s = 0; s < RB_NSPECIES; s++) ch->dm[s] = 0.0;
    ch->dU = 0.0;
}

void rb_chamber_add_flow(rb_chamber *ch, double mdot, const double y[RB_NSPECIES], double h)
{
    for (int s = 0; s < RB_NSPECIES; s++) ch->dm[s] += mdot * y[s];
    ch->dU += mdot * h;
}

void rb_chamber_add_heat(rb_chamber *ch, double qdot)
{
    ch->dU += qdot;
}

void rb_chamber_integrate(rb_chamber *ch, double dt)
{
    if (ch->fixed) return;   /* reservoirs never change */
    for (int s = 0; s < RB_NSPECIES; s++) {
        ch->m[s] += ch->dm[s] * dt;
        if (ch->m[s] < 0.0) {   /* more went out than was there: should never happen */
            ch->m[s] = 0.0;
            ch->repairs++;
        }
    }
    ch->U += ch->dU * dt;
    rb_chamber_update(ch);
}

/* One trapezoidal volume step for a chamber with gas in it. */
static void volume_step(rb_chamber *ch, double V_new)
{
    /* Trapezoidal rule for dU = -p dV, using the average of the pressures
     * before and after the step:
     *     U1 = U0 - (p0 + p1)/2 * dV,  with  U1 = m u(T1)  and  p1 = m R T1 / V1.
     * Solved for T1 with Newton's method: f(T) = m (u(T) + k T) - rhs, where
     * k = R dV / (2 V1), and f'(T) = m (cv(T) + k). Small steps keep f' > 0. */
    const double m = ch->mass;
    const double dV = V_new - ch->V;
    const double rhs = ch->U - 0.5 * ch->p * dV;
    const double k = 0.5 * ch->gas.R * dV / V_new;
    double T = ch->T;
    for (int i = 0; i < 50; i++) {
        double f = m * (rb_gas_u(&ch->gas, T) + k * T) - rhs;
        double dT = f / (m * (rb_gas_cv(&ch->gas, T) + k));
        T -= dT;
        if (T < 1.0) T = 1.0;
        if (fabs(dT) <= 1e-12 * T) break;
    }
    if (!(T < INFINITY)) {   /* NaN or inf: only possible from a corrupted state */
        T = ch->T;
        ch->repairs++;
    }

    /* The composition has not changed, so there is no need to remix the gas
     * or to solve for T again as rb_chamber_update() would. */
    ch->T = T;
    ch->u = rb_gas_u(&ch->gas, T);
    ch->U = m * ch->u;
    ch->V = V_new;
    derive(ch);
}

double rb_chamber_set_volume(rb_chamber *ch, double V_new)
{
    if (!(V_new > 0.0 && V_new < INFINITY) || ch->fixed) return 0.0;
    if (ch->mass <= RB_MASS_EPS) {   /* nothing to compress */
        ch->V = V_new;
        rb_chamber_update(ch);
        return 0.0;
    }

    const double U0 = ch->U, V0 = ch->V;
    /* The trapezoidal rule needs small steps: its error grows with the square
     * of the step, and squeezed sixfold in one step it has no solution at all.
     * A piston changes the volume by at most 0.3 % per physics step at 3000 rpm
     * and 1.6 % at 18000 rpm. Bigger jumps are split into steps of at most 2 %,
     * at volumes evenly spaced in log V, so going back retraces them. */
    if (V_new > RB_MAX_VOLUME_STEP * V0 || RB_MAX_VOLUME_STEP * V_new < V0) {
        const double lr = log(V_new / V0);
        const int n = (int)ceil(fabs(lr) / log(RB_MAX_VOLUME_STEP));
        for (int i = 1; i < n; i++) volume_step(ch, V0 * exp(lr * i / n));
    }
    volume_step(ch, V_new);
    return ch->U - U0;
}
