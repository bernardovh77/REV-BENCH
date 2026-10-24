/*
 * combustion.h - burning fuel in a cylinder: how much energy it releases and
 * how fast (the Wiebe function).
 *
 * Energy. Burning 1 kg of the gasoline surrogate (RB_FUEL, gas.h) takes
 * RB_AFR_STOICH = 14.596 kg of air, makes 15.596 kg of burned gas and
 * releases RB_FUEL_LHV = 43.12 MJ. That is the lower heating value (the
 * water leaves as steam) of the *liquid* fuel, computed from the same NASA
 * data as the gas properties (docs/stage-06.md): the 350 kJ/kg it takes to
 * evaporate the fuel is already taken off, because the model adds the fuel
 * as vapour without cooling the charge.
 *
 * The model's energies are sensible (zero at T_REF = 25 C), so burning shows
 * up as heat added to the gas. At constant volume it is slightly more than
 * the heating value: the reaction makes more molecules than it uses, so the
 * part of the enthalpy change that would have pushed the surroundings back
 * stays in the gas as energy. Per kg of fuel burned:
 *
 *     Q = LHV + T_REF ((1 + AFR) R_burned - R_fuel - AFR R_air)    (+60 kJ/kg)
 *
 * Burn rate. The spark starts a flame that grows slowly, then sweeps the
 * chamber, then slows near the walls. The burned fraction of the charge,
 * phi crank degrees after the start of combustion, follows the Wiebe
 * function, scaled here to reach exactly 1 at the end of the burn:
 *
 *     x(phi) = (1 - exp(-a (phi / duration)^(m+1))) / (1 - exp(-a))
 *
 *      1 |                   ___------
 *        |                ,-'
 *   x    |              /
 *        |            /
 *      0 |______..--'
 *        +--------|--------------------|----> phi
 *               spark              + duration
 *
 * With a = 5 and m = 2, 10 to 90 % of the charge burns in the middle half of
 * the duration, as in real spark-ignition engines (Heywood, chapter 9).
 *
 * How much burns. Fuel and air are never perfectly mixed: some pockets are
 * richer than average, some leaner. Each pocket burns its fuel or its air,
 * whichever runs out first. With the local equivalence ratio (fuel/air
 * against stoichiometric) spread normally around the mean phi with standard
 * deviation s, the fuel that can burn, per kg of air, is
 *
 *     (1 / AFR) [ phi - E[(phi_local - 1)+] ]
 *     E[(phi_local - 1)+] = (phi - 1) (1 - N(z)) + s n(z),   z = (1 - phi) / s
 *
 * (N and n are the normal distribution's cumulative and density). On top of
 * that a little fuel always escapes the flame in crevices and near the walls:
 * the combustion efficiency. Together they give the shape of real engines'
 * combustion efficiency against mixture strength (Heywood, chapter 4): about
 * 98 % lean, 95 % stoichiometric, 82 % at phi = 1.2. And a slightly rich
 * mixture burns more of the air, which is why engines make their best power
 * there.
 */
#ifndef RB_COMBUSTION_H
#define RB_COMBUSTION_H

/* J/kg: lower heating value of the liquid fuel at 25 C. */
#define RB_FUEL_LHV 43.116e6

/* J added to the gas's sensible energy per kg of fuel burned (see above). */
double rb_fuel_heat(void);

/* Burned fraction (0..1) phi crank degrees after the start of combustion, for
 * a burn of `duration` degrees with Wiebe parameters a and m. 0 before the
 * start, 1 after the end. */
double rb_wiebe(double phi, double duration, double a, double m);

/* kg of fuel that burns from `fuel` and `air` (kg) mixed with a spread s of
 * the local equivalence ratio and the given combustion efficiency. */
double rb_burnable_fuel(double fuel, double air, double spread, double efficiency);

/* Internal research policies, independent of stochastic amplitude. CHARGE is
 * the production default; CAPPED is an explicit historical-duration control.
 * Neither changes fuel/species or supplies a spark-extinction criterion. */
enum { RB_BURN_CHARGE, RB_BURN_CAPPED };
enum {
    RB_FLAME_PHI_RANGE = 1u << 0,
    RB_FLAME_T_RANGE = 1u << 1,
    RB_FLAME_P_RANGE = 1u << 2,
    RB_FLAME_DILUTION_RANGE = 1u << 3,
    RB_FLAME_SPEED_FLOOR = 1u << 4,
    RB_FLAME_DILUTION_FLOOR = 1u << 5,
    RB_FLAME_DEMAND_LIMIT = 1u << 6,
    RB_FLAME_INVALID = 1u << 7,
    RB_FLAME_NO_REACTANTS = 1u << 8,
    RB_FLAME_CAPPED_MODEL = 1u << 9,
    RB_FLAME_SPARK_LIMIT = 1u << 10
};
typedef struct rb_charge_burn {
    double speed_m_s;      /* bounded indolene/dilution surrogate at supplied state */
    double raw_demand_deg, demand_deg, capped_deg;
    unsigned flags;       /* range/extrapolation/fallback, never a misfire verdict */
} rb_charge_burn;

/* Existing entrainment/burn-up duration evaluated at an IVC-state proxy.
 * Numeric range screen: phi .8..1.4, T350..700K, p .4..50atm, burned mass
 * fraction0..0.2. Dilution data are stoichiometric isooctane, not a validation
 * of this surrogate/IVC proxy. Missing phi denotes no reactants. All floors,
 * demand ceilings and invalid-input fallbacks are reported. */
rb_charge_burn rb_charge_burn_evaluate(double T, double p, double residual,
                                     double phi, double rpm, double reference_deg);

/* Exact normalized Wiebe quantile, including the finite-duration normalization. */
double rb_wiebe_quantile(double fraction, double a, double m);

/* E[clamp(base*trim*exp(sigma*clamp(Z,-3,3)), .25*base, ceiling)],
 * Z standard normal. This is the actual duration draw's conditional mean,
 * including winsorized tails and duration bounds, not one random cycle. */
double rb_burn_mean_duration(double base, double sigma, double trim, double ceiling);
/* Mean positive arm of clamp(Z,-3,3), used by the causal kernel surrogate. */
double rb_kernel_positive_mean(void);

#endif /* RB_COMBUSTION_H */
