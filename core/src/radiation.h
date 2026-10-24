#ifndef RB_RADIATION_H
#define RB_RADIATION_H

/* Passive, causal unflanged plane-mode radiation load, Silva et al. (2009),
 * rational approximation in their table 1. The external medium is fixed;
 * strong mean jets and flange/body scattering are outside this approximation.
 * v is the signed mean velocity at the aperture (m/s), output pressure Pa.
 * The caller must book pressure * volume flow as work to the surroundings. */
typedef struct rb_radiation {
    struct rb_diag_slot *diag;
    int enabled;
    double tau, impedance, dt;
    double b0, b1, b2, a1, a2;
    double v1, v2, z1, z2, pressure;
} rb_radiation;

void rb_radiation_init(rb_radiation *r, double radius, double rho, double c);
/* pressure = direct * v_new + history. Coefficients use a bilinear transform;
 * the caller can include direct in its implicit momentum denominator. */
void rb_radiation_linear(rb_radiation *r, double dt, double *direct, double *history);
double rb_radiation_step(rb_radiation *r, double velocity);
#endif
