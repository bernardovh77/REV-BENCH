/*
 * gas.h - ideal-gas properties of the gases inside the engine.
 *
 * Three pseudo-species are tracked everywhere (cylinders, manifolds, pipes):
 *
 *   RB_AIR     dry air: O2 20.95 %, N2 78.08 %, Ar 0.93 % (by mole)
 *   RB_FUEL    gasoline vapour, modelled as a "toluene reference fuel":
 *              47.85 % isooctane, 15 % n-heptane, 37.15 % toluene (by mole),
 *              which has real gasoline's hydrogen/carbon ratio of 1.87
 *   RB_BURNED  what air and fuel become when they burn in the exact
 *              (stoichiometric) ratio: CO2, H2O, N2 and Ar
 *
 * A lean mixture burns into BURNED plus leftover AIR, a rich one into BURNED
 * plus leftover FUEL, so these three are enough to follow the air-fuel ratio.
 *
 * Heat capacity depends strongly on temperature (air: cp = 1005 J/(kg K) at
 * 300 K, 1141 at 1000 K, 1296 at 3000 K), so each species has a cp(T) fit in
 * two ranges, 200-1000 K and 1000-4000 K, made from the NASA Glenn
 * thermodynamic database; it matches the data to better than 0.001 %.
 * Outside 200-4000 K, cp is held constant.
 *
 * Energies are "sensible": zero at T_REF = 298.15 K for every species. The
 * chemical energy released by burning is added separately (stage 6).
 */
#ifndef RB_GAS_H
#define RB_GAS_H

enum { RB_AIR = 0, RB_FUEL, RB_BURNED, RB_NSPECIES };

#define RB_GAS_RU  8.314462618   /* J/(mol K), universal gas constant      */
#define RB_T_REF   298.15        /* K, where sensible energies are zero    */
#define RB_T_LO    200.0         /* K, the fits are valid from here ...    */
#define RB_T_MID   1000.0        /* K, ... switch from low to high fit ... */
#define RB_T_HI    4000.0        /* K, ... up to here                      */

/* Stoichiometric air/fuel mass ratio of the fuel above. */
#define RB_AFR_STOICH 14.596

/* Thermodynamic description of one gas mixture, built by rb_gas_init(). */
typedef struct rb_gas {
    double R;       /* specific gas constant, J/(kg K)                        */
    double lo[5];   /* cp(T), T <= 1000 K: lo[0] + lo[1] t + ... + lo[4] t^4, t = T/1000 */
    double hi[5];   /* cp(T), T >  1000 K, same form                          */
    double k_lo;    /* integration constants: make u(T_REF) = 0 ...           */
    double k_hi;    /* ... and u continuous at T_MID                           */
} rb_gas;

const char *rb_species_name(int species);   /* NULL if unknown */
double      rb_species_R(int species);      /* J/(kg K), NaN if unknown */

/* Mixture of the species with mass fractions y (normalised here; all-zero
 * or invalid input means pure air). Mixing is a mass-weighted average. */
void   rb_gas_init(rb_gas *g, const double y[RB_NSPECIES]);

double rb_gas_cp(const rb_gas *g, double T);      /* J/(kg K) */
double rb_gas_cv(const rb_gas *g, double T);      /* J/(kg K), cp - R */
double rb_gas_gamma(const rb_gas *g, double T);   /* cp / cv */
double rb_gas_u(const rb_gas *g, double T);       /* J/kg, sensible internal energy */
double rb_gas_h(const rb_gas *g, double T);       /* J/kg, enthalpy = u + R T */

/* Temperature (K) whose internal energy is u, by Newton's method starting
 * from T_guess (the previous temperature is a good guess). */
double rb_gas_T_from_u(const rb_gas *g, double u, double T_guess);

#endif /* RB_GAS_H */
