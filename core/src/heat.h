/*
 * heat.h - heat flow between the gas in a cylinder and the walls around it,
 * with Woschni's correlation (Heywood, chapter 12):
 *
 *     h = 3.26 B^-0.2 p^0.8 T^-0.55 w^0.8      [W/(m^2 K)]
 *
 * with the bore B in m, the gas pressure p in kPa and temperature T in K,
 * and w (m/s) a speed that stands for how hard the gas is stirred:
 *
 *     w = C1 Sp + C2 (Vd T_r / (p_r V_r)) (p - p_motored)
 *
 *   Sp      mean piston speed, 2 * stroke * rpm / 60
 *   C1      6.18 while a valve is open (gas exchange), 2.28 otherwise
 *   C2      3.24e-3 m/(s K) from the start of combustion to exhaust valve
 *           opening, 0 otherwise: the burning adds turbulence, measured by
 *           how far the pressure is above what it would be without burning
 *   p_r, V_r, T_r   the state at intake valve closing; the pressure without
 *           burning is p_motored = p_r (V_r / V)^1.3 from there
 *
 * The heat flowing into the gas is h A (T_wall - T), where A is the area the
 * gas touches: the cylinder head and the piston crown (both taken as flat
 * discs of the bore's area) and the liner above the piston.
 */
#ifndef RB_HEAT_H
#define RB_HEAT_H

#include "cylinder.h"

/* Woschni heat transfer coefficient, W/(m^2 K). p in Pa (converted to kPa
 * inside), T in K, w in m/s. 0 for nonsense input. */
double rb_woschni_h(double bore, double p, double T, double w);

/* Woschni's gas speed w (m/s) for the given phase of the cycle. */
double rb_woschni_w(double piston_speed, int valves_open, int burning_phase,
                    double v_disp, double p, double p_r, double V_r, double T_r,
                    double V);

/* Area (m^2) of the walls around the gas with the piston x below TDC. */
double rb_wall_area(const rb_cyl_geom *g, double x);

#endif /* RB_HEAT_H */
