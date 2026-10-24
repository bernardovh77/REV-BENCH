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

static double flame_speed(double T, double p, double x, double phi, unsigned *flags)
{
    /* Metghalchi/Keck1982 Eq24/Table3A, RMFD303. Floors are the historical
     * numerical extension, explicitly not measured flammability limits. */
    double s0=27.58-78.34*(phi-1.13)*(phi-1.13);
    if(s0<2.0) {s0=2.0;*flags|=RB_FLAME_SPEED_FLOOR;}
    double dilution=1.0-2.1*x;
    if(dilution<.1) {dilution=.1;*flags|=RB_FLAME_DILUTION_FLOOR;}
    if(dilution>1) {dilution=1;*flags|=RB_FLAME_DILUTION_FLOOR;}
    return s0*pow(T/298.0,2.18-.8*(phi-1.0))
             *pow(p/101325.0,-.16+.22*(phi-1.0))*dilution;
}

rb_charge_burn rb_charge_burn_evaluate(double T,double p,double x,double phi,
                                     double rpm,double reference)
{
    rb_charge_burn result={0};
    if(!(phi>=.8 && phi<=1.4)) result.flags|=RB_FLAME_PHI_RANGE;
    if(!(T>=350 && T<=700)) result.flags|=RB_FLAME_T_RANGE;
    if(!(p>=.4*101325 && p<=50*101325)) result.flags|=RB_FLAME_P_RANGE;
    if(!(x>=0 && x<=.2)) result.flags|=RB_FLAME_DILUTION_RANGE;
    if(phi==0) {result.flags|=RB_FLAME_NO_REACTANTS;phi=1;}
    if(!(T>0 && p>0 && x>=0 && x<=1 && phi>0 && rpm>=0 && reference>0)
       || !isfinite(T+p+x+phi+rpm+reference)) {
        result.flags|=RB_FLAME_INVALID;
        result.raw_demand_deg=result.demand_deg=result.capped_deg=
            reference>0 && reference<=180 ? reference:60;
        return result;
    }
    unsigned ref_flags=0;
    const double speed=flame_speed(T,p,x,phi,&result.flags);
    const double s_l=speed/flame_speed(360,1.22e5,.04,1,&ref_flags);
    const double nu=pow(T/360.0,1.7)*(1.22e5/p);
    const double n=(rpm>100?rpm:100)/3000;
    double f=.6+.4*sqrt(n*nu)/s_l;
    if(!isfinite(f) || !(speed>0)) {
        result.flags|=RB_FLAME_INVALID;f=1;
    }
    if(f<.5) f=.5;
    result.speed_m_s=isfinite(speed) && speed>0 ? .01*speed:0;
    result.raw_demand_deg=reference*f;
    result.demand_deg=fmin(result.raw_demand_deg,720);
    if(result.raw_demand_deg>720) result.flags|=RB_FLAME_DEMAND_LIMIT;
    result.capped_deg=fmin(reference*fmin(f,2),180);
    return result;
}

double rb_wiebe_quantile(double fraction,double a,double m)
{
    if(!(fraction>0)) return 0;
    if(fraction>=1) return 1;
    return pow(-log1p(fraction*expm1(-a))/a,1/(m+1));
}

static double normal_cdf(double z) {return .5*erfc(-z/sqrt(2.0));}
static double clamp(double x,double lo,double hi) {return fmin(fmax(x,lo),hi);}

double rb_burn_mean_duration(double base,double sigma,double trim,double ceiling)
{
    const double lower=.25*base, upper=ceiling, B=base*trim;
    if(!(sigma>0 && B>0)) return clamp(B,lower,upper);
    const double lo=clamp(log(lower/B)/sigma,-3,3);
    const double hi=clamp(log(upper/B)/sigma,-3,3);
    const double tail=normal_cdf(-3);
    return tail*(clamp(B*exp(-3*sigma),lower,upper)
                +clamp(B*exp(3*sigma),lower,upper))
        +lower*(normal_cdf(lo)-tail)
        +B*exp(.5*sigma*sigma)*(normal_cdf(hi-sigma)-normal_cdf(lo-sigma))
        +upper*(normal_cdf(3)-normal_cdf(hi));
}

double rb_kernel_positive_mean(void)
{
    return (1-exp(-4.5))/sqrt(2*3.14159265358979323846)+3*normal_cdf(-3);
}
