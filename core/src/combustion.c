/*
 * combustion.c - fuel energy and the Wiebe burn rate (see combustion.h).
 */
#include "combustion.h"

#include "gas.h"

#include <math.h>

double rb_fuel_heat(void)
{
    const double afr = RB_AFR_STOICH;
    const double dnRT = RB_T_REF * ((1.0 + afr) * rb_species_R(RB_BURNED)
                                    - rb_species_R(RB_FUEL) - afr * rb_species_R(RB_AIR));
    return RB_FUEL_LHV + dnRT;
}

double rb_burnable_fuel(double fuel, double air, double spread, double efficiency)
{
    if (!(fuel > 0.0 && air > 0.0) || !(efficiency > 0.0)) return 0.0;
    const double per_air = 1.0 / RB_AFR_STOICH;     /* fuel per kg of air at phi = 1 */
    const double phi = fuel / (air * per_air);
    double burnt_phi;                               /* phi - E[(phi_local - 1)+] */
    if (spread > 0.0) {
        const double z = (1.0 - phi) / spread;
        const double cdf = 0.5 * (1.0 + erf(z / sqrt(2.0)));
        const double pdf = exp(-0.5 * z * z) / sqrt(2.0 * 3.14159265358979323846);
        burnt_phi = phi - ((phi - 1.0) * (1.0 - cdf) + spread * pdf);
    } else {
        burnt_phi = phi < 1.0 ? phi : 1.0;          /* perfectly mixed */
    }
    double burnable = efficiency * air * per_air * burnt_phi;
    if (burnable > fuel) burnable = fuel;           /* rounding at the extremes */
    return burnable > 0.0 ? burnable : 0.0;
}

double rb_wiebe(double phi, double duration, double a, double m)
{
    if (!(phi > 0.0)) return 0.0;              /* before the start (and NaN) */
    if (!(phi < duration)) return 1.0;         /* after the end */
    const double s = phi / duration;
    return (1.0 - exp(-a * pow(s, m + 1.0))) / (1.0 - exp(-a));
}
