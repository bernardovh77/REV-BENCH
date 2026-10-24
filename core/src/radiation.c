#include "radiation.h"
#include "diagnostics.h"
#include <math.h>
#include <string.h>

void rb_radiation_init(rb_radiation *r, double radius, double rho, double c)
{
    memset(r, 0, sizeof *r);
    if (!(radius > 0.0 && radius <= 1.0 && rho > 0.0 && rho <= 1e4
          && c > 0.0 && c <= 1e5)) return;
    r->enabled = 1;
    r->tau = radius / c;
    r->impedance = rho * c;
}

void rb_radiation_linear(rb_radiation *r, double dt, double *direct, double *history)
{
    *direct = *history = 0.0;
    if (!r->enabled || !(dt >= 1e-12 && dt <= 1.0)) return;
    if (dt != r->dt) {
        /* R(s)=-(1+.167 x)/(1+1.393 x+.457 x²), x=s a/c.
         * Z=(1+R)/(1-R)=(1.226 x+.457 x²)/(2+1.560 x+.457 x²).
         * This is a physical pressure/velocity load, not output coloration. */
        const double k = 2.0 * r->tau / dt, square = 0.457 * k * k;
        const double den = 2.0 + 1.560 * k + square;
        r->b0 = (1.226 * k + square) / den;
        r->b1 = -2.0 * square / den;
        r->b2 = (-1.226 * k + square) / den;
        r->a1 = (4.0 - 2.0 * square) / den;
        r->a2 = (2.0 - 1.560 * k + square) / den;
        r->dt = dt;
    }
    *direct = r->impedance * r->b0;
    *history = r->impedance * (r->b1*r->v1 + r->b2*r->v2 - r->a1*r->z1 - r->a2*r->z2);
}

double rb_radiation_step(rb_radiation *r, double velocity)
{
    if (!r->enabled || !(r->dt > 0.0)) return 0.0;
    if (!isfinite(velocity) || fabs(velocity) > 1e6) {
        if(r->diag) rb_diag_note(r->diag,RB_D_BIT(RB_D_RADIATION_RESET));
        velocity = 0.0;
    }
    const double z = r->b0*velocity + r->b1*r->v1 + r->b2*r->v2 - r->a1*r->z1 - r->a2*r->z2;
    r->v2 = r->v1; r->v1 = velocity;
    r->z2 = r->z1; r->z1 = z;
    r->pressure = r->impedance * z;
    return r->pressure;
}
