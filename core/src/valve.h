/*
 * valve.h - poppet valves opened by a cam: how far the valves are open at
 * each crank angle (the lift), and how much flow area that gives the gas.
 *
 * Lift. The cam lobe lifts the valve over `duration` crank degrees, centred
 * on the lobe centerline (the crank angle of maximum lift):
 *
 *     L = max_lift (1 - u^2)^2,   u = 2 (theta - center) / duration,   |u| < 1
 *
 *     lift
 *      ^              .-""-.
 *      |            /        \
 *      |          /            \
 *      |  ______/                \______
 *      +--------|--------+--------|---------> crank angle
 *             opens    center    closes
 *               |<--- duration --->|
 *
 * The valve leaves and returns to its seat with zero speed, like a real cam's
 * ramps, and the curve is about as "full" as a real street cam's: at 1.27 mm
 * (0.050 in) of lift it is open for about 80 % of its full duration.
 *
 * Flow area. A barely open valve lets gas through its "curtain", the short
 * cylinder of height L between the valve head and its seat (area pi d L).
 * At high lift the port behind the valve becomes the bottleneck instead.
 * The two are in series, so their effective areas combine like resistances
 * (pressure drop ~ (flow / area)^2 for each):
 *
 *     1 / A^2 = 1 / (cd pi d L)^2 + 1 / (RB_PORT_CD pi d_port^2 / 4)^2
 *
 * With cd = 0.7 and a port 0.85 times the valve diameter, this follows the
 * measured discharge coefficients of real intake valves (Heywood, chapter 6):
 * about 0.68 of the curtain area at low lift, falling to about 0.48 at
 * L = d/4, where the port takes over.
 *
 * Angles are crank degrees in the 720-degree cycle (0 = firing TDC, 360 =
 * overlap TDC; see cylinder.h).
 */
#ifndef RB_VALVE_H
#define RB_VALVE_H

/* Discharge coefficient of the port throat behind a valve. */
#define RB_PORT_CD 0.9

/* The intake (or exhaust) valves of one cylinder and the cam lobe that
 * opens them. */
typedef struct rb_valve {
    int    count;           /* valves of this kind per cylinder, 1..4            */
    double diameter;        /* m, valve head diameter (where it seals)           */
    double port_diameter;   /* m, port throat diameter behind the valve          */
    double max_lift;        /* m                                                 */
    double duration;        /* crank degrees from opening to closing (0..360)    */
    double center;          /* crank degrees of maximum lift, in the 720 cycle   */
    double cd;              /* discharge coefficient of the curtain area         */
} rb_valve;

/* 0 if every field is sensible, -1 otherwise. */
int    rb_valve_check(const rb_valve *v);

/* Lift (m) at crank angle theta (degrees, any value: wrapped to the cycle). */
double rb_valve_lift(const rb_valve *v, double theta_deg);

/* Effective flow area (m^2, discharge coefficients included) of all `count`
 * valves at the given lift. */
double rb_valve_area(const rb_valve *v, double lift);

/* Crank angles (degrees, 0..720) where the valves leave and reach their seats. */
double rb_valve_opens(const rb_valve *v);
double rb_valve_closes(const rb_valve *v);

/* The cycle angle wrapped to -360..360: how far theta is after `center`. */
double rb_cycle_offset(double theta_deg, double center_deg);

#endif /* RB_VALVE_H */
