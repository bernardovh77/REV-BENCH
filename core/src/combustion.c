/*
 * combustion.c - fuel energy and the Wiebe burn rate (see combustion.h).
 */
#include "combustion.h"

#include "gas.h"
#include "fastmath.h"

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

/* Gasoline's laminar flame speed (cm/s) at T (K), p (Pa), burned-gas mass
 * fraction x and equivalence ratio phi: Metghalchi and Keck's fit,
 * S_L0 (T / 298)^alpha (p / 1 atm)^beta (1 - 2.1 x), with S_L0 peaking at
 * phi 1.13. */
static double flame_speed(double T, double p, double x, double phi, unsigned *flags)
{
    /* Metghalchi/Keck1982 Eq24/Table3A, RMFD303. Floors are the historical
     * numerical extension, explicitly not measured flammability limits. */
    double s0 = 27.58 - 78.34 * (phi - 1.13) * (phi - 1.13);
    if (s0 < 2.0) {
        s0 = 2.0;
        *flags |= RB_FLAME_SPEED_FLOOR;
    }
    double dilution = 1.0 - 2.1 * x;
    if (dilution < .1) {
        dilution = .1;
        *flags |= RB_FLAME_DILUTION_FLOOR;
    }
    if (dilution > 1) {
        dilution = 1;
        *flags |= RB_FLAME_DILUTION_FLOOR;
    }
    return s0 * pow(T / 298.0, 2.18 - .8 * (phi - 1.0)) *
           pow(p / 101325.0, -.16 + .22 * (phi - 1.0)) * dilution;
}

/* The burn's length for a charge, in crank degrees, as a multiple f of the
 * reference length (`reference`: the four's at 3000 rpm wide open). Its
 * charge is the one trapped at intake valve closing (T, p, burned share x,
 * equivalence ratio phi), at `rpm`. 60 % of the reference burn keeps its
 * crank angle; the other 40 % goes as sqrt(rpm nu) / S_L (engine.c,
 * burn_duration, explains why), each relative to the reference charge (360 K,
 * 1.22 bar, 4 % burned gas, phi 1), with the kinematic viscosity
 * nu ~ T^1.7 / p. The result has the length three ways: raw (f at least
 * 0.5), the demand (at most a whole cycle, 720 degrees) and the old capped
 * one (at most twice the reference and 180 degrees). Its flags say which
 * ranges and floors were crossed; a NaN or impossible input returns the
 * reference (or 60 degrees) with RB_FLAME_INVALID. */
rb_charge_burn rb_charge_burn_evaluate(double T, double p, double x, double phi, double rpm,
                                       double reference)
{
    rb_charge_burn result = { 0 };
    if (!(phi >= .8 && phi <= 1.4)) result.flags |= RB_FLAME_PHI_RANGE;
    if (!(T >= 350 && T <= 700)) result.flags |= RB_FLAME_T_RANGE;
    if (!(p >= .4 * 101325 && p <= 50 * 101325)) result.flags |= RB_FLAME_P_RANGE;
    if (!(x >= 0 && x <= .2)) result.flags |= RB_FLAME_DILUTION_RANGE;
    if (phi == 0) {
        result.flags |= RB_FLAME_NO_REACTANTS;
        phi = 1;
    }
    if (!(T > 0 && p > 0 && x >= 0 && x <= 1 && phi > 0 && rpm >= 0 && reference > 0) ||
        !rb_finite(T + p + x + phi + rpm + reference)) {
        result.flags |= RB_FLAME_INVALID;
        result.raw_demand_deg = result.demand_deg = result.capped_deg =
            reference > 0 && reference <= 180 ? reference : 60;
        return result;
    }
    unsigned ref_flags = 0;
    const double speed = flame_speed(T, p, x, phi, &result.flags);
    const double s_l = speed / flame_speed(360, 1.22e5, .04, 1, &ref_flags);
    const double nu = pow(T / 360.0, 1.7) * (1.22e5 / p);
    const double n = (rpm > 100 ? rpm : 100) / 3000;
    double f = .6 + .4 * sqrt(n * nu) / s_l;
    if (!rb_finite(f) || !(speed > 0)) {
        result.flags |= RB_FLAME_INVALID;
        f = 1;
    }
    if (f < .5) f = .5;
    result.speed_m_s = rb_finite(speed) && speed > 0 ? .01 * speed : 0;
    result.raw_demand_deg = reference * f;
    result.demand_deg = fmin(result.raw_demand_deg, 720);
    if (result.raw_demand_deg > 720) result.flags |= RB_FLAME_DEMAND_LIMIT;
    result.capped_deg = fmin(reference * fmin(f, 2), 180);
    return result;
}

/* The share of the burn's length at which `fraction` of the charge has
 * burned: rb_wiebe solved for its angle, with the curve's normalisation
 * 1 - exp(-a) kept (log1p and expm1 keep it exact for small values). */
double rb_wiebe_quantile(double fraction, double a, double m)
{
    if (!(fraction > 0)) return 0;
    if (fraction >= 1) return 1;
    return pow(-log1p(fraction * expm1(-a)) / a, 1 / (m + 1));
}

/* The standard normal's cumulative distribution, Phi(z). */
static double normal_cdf(double z)
{
    return .5 * erfc(-z / sqrt(2.0));
}
/* x kept within lo..hi. */
static double clamp(double x, double lo, double hi)
{
    return fmin(fmax(x, lo), hi);
}

/* The mean of a cycle's burn length, base trim exp(sigma z) for a standard
 * normal z kept within +-3, the length then kept between a quarter of base
 * and the ceiling (engine.c, close_intake). Over z the clamped draw is: the
 * value at -3 or +3 beyond them (each with probability Phi(-3)); the lower
 * bound below lo and the upper above hi (where the draw reaches them); and
 * the lognormal between, whose share of the mean is
 * B exp(sigma^2 / 2) (Phi(hi - sigma) - Phi(lo - sigma)). The engine
 * computer sets its spark for this mean. */
double rb_burn_mean_duration(double base, double sigma, double trim, double ceiling)
{
    const double lower = .25 * base, upper = ceiling, B = base * trim;
    if (!(sigma > 0 && B > 0)) return clamp(B, lower, upper);
    const double lo = clamp(log(lower / B) / sigma, -3, 3);
    const double hi = clamp(log(upper / B) / sigma, -3, 3);
    const double tail = normal_cdf(-3);
    return tail * (clamp(B * exp(-3 * sigma), lower, upper) +
                   clamp(B * exp(3 * sigma), lower, upper)) +
           lower * (normal_cdf(lo) - tail) +
           B * exp(.5 * sigma * sigma) * (normal_cdf(hi - sigma) - normal_cdf(lo - sigma)) +
           upper * (normal_cdf(3) - normal_cdf(hi));
}

/* The flame kernel's development delay, in units of its spread, from a
 * standard normal draw z kept within +-3 (stage 115): -ln(Phi(-z)), a unit
 * exponential. It is never negative (the flame can't start before the
 * spark), every cycle has its own, the slow starts are the long tail (as
 * real engines' slow cycles are), and its RMS spread is 0.991: the spread
 * of stage 12c's symmetric start. A larger z gives a longer delay, so the
 * burn's length, drawn alike with z, stays longer after a slow start.
 * (Stage 040 used max(z, 0) instead: half the cycles then started at once,
 * and the spread fell to 0.582, under the real engines' 1-3 % COV of IMEP
 * wide open.) */
double rb_kernel_delay_unit(double z)
{
    return -log(normal_cdf(-clamp(z, -3, 3)));
}

/* Its mean over the kept normal: with q = Phi(-3), the share beyond each
 * bound, 1 - 2 q - ln(1 - q) = 0.998651. */
double rb_kernel_delay_mean(void)
{
    const double q = normal_cdf(-3);
    return 1 - 2 * q - log1p(-q);
}
