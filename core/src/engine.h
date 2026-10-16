/*
 * engine.h - a whole engine breathing: N cylinders on one crankshaft, fed
 * from an intake plenum behind the throttle, exhausting straight into the
 * open air. There is no fuel yet (stage 6) and the crank is turned at a set
 * speed by a motor (stage 7 lets it run itself), so this measures how well
 * the engine breathes.
 *
 *   ambient air --> throttle --> plenum --> intake valves --> cylinder 1..N
 *                                                                  |
 *   ambient air <--------------------------------- exhaust valves <-+
 *
 * One physics step (dt):
 *   1. set every opening's area: the throttle from the pedal, each valve
 *      from its cam at the current crank angle;
 *   2. move gas through all the openings at once, computed from the current
 *      state (explicit Euler, as in stage 3);
 *   3. turn the crank by 6 rpm dt degrees and move every piston (the
 *      trapezoidal step of stage 4).
 * At the end of every 720-degree cycle the engine adds up what went through
 * it: volumetric efficiency, plenum pressure, pumping loss. The step that
 * crosses the end of a cycle is shared between the two cycles in proportion
 * to its angle on either side, so the totals don't jitter when a cycle isn't
 * a whole number of steps.
 *
 * Crank angles are degrees. theta is cylinder 1's angle in the 720-degree
 * cycle (0 = its firing TDC); cylinder i is at theta - firing[i].
 */
#ifndef RB_ENGINE_H
#define RB_ENGINE_H

#include "chamber.h"
#include "cylinder.h"
#include "flow.h"
#include "throttle.h"
#include "valve.h"

#define RB_MAX_CYLINDERS 16

/* Everything that defines an engine (stage 8 turns these into presets). */
typedef struct rb_engine_desc {
    int    cylinders;                     /* 1..RB_MAX_CYLINDERS                      */
    double bore, stroke, rod, cr;         /* m, m, m, compression ratio (cylinder.h)  */
    double firing[RB_MAX_CYLINDERS];      /* degrees after cylinder 1 that each cylinder
                                           * reaches its firing TDC (firing[0] = 0)   */
    rb_valve    intake, exhaust;          /* each cylinder's valves (valve.h)         */
    rb_throttle throttle;                 /* throttle.h                               */
    double plenum_volume;                 /* m^3                                      */
    double p_ambient, T_ambient;          /* Pa, K: the air around the engine         */
} rb_engine_desc;

/* What went through the engine during one 720-degree cycle. */
typedef struct rb_engine_cycle {
    double ve;                        /* air drawn in / (ambient air density * displacement) */
    double ve_plenum;                 /* the same against the plenum's mean density          */
    double ve_cyl[RB_MAX_CYLINDERS];  /* each cylinder's, against ambient density            */
    double map, map_min, map_max;     /* Pa: plenum ("manifold absolute") pressure            */
    double air_flow;                  /* kg/s through the throttle, averaged over the cycle   */
    double pumping_work;              /* J: work the pistons did on the gas from BDC to BDC
                                       * through the exhaust and intake strokes (> 0 = lost) */
    double pmep;                      /* Pa: pumping mean effective pressure = pumping_work /
                                       * displacement                                         */
    double net_work;                  /* J: work done BY the gas on the pistons, whole cycle  */
} rb_engine_cycle;

typedef struct rb_engine {
    rb_engine_desc desc;
    double dt;              /* s, physics step                                    */
    double rpm;             /* crank speed, held by the motor (set it any time)   */
    double throttle_pos;    /* pedal, 0..1 (set it any time)                      */
    double theta;           /* degrees, cylinder 1's crank angle in the cycle     */
    long   cycles;          /* cycles completed since rb_engine_init               */
    rb_engine_cycle result; /* the last completed cycle                           */

    /* the gas */
    rb_cylinder cyl[RB_MAX_CYLINDERS];
    rb_chamber  intake_air, plenum, exhaust_air;   /* the two air reservoirs are fixed */
    rb_orifice  throttle;                          /* intake_air -> plenum             */
    rb_orifice  intake[RB_MAX_CYLINDERS];          /* plenum -> cylinder i             */
    rb_orifice  exhaust[RB_MAX_CYLINDERS];         /* cylinder i -> exhaust_air        */

    /* totals since rb_engine_init, for checking conservation */
    double mass_in, mass_out;       /* kg taken from the intake air / given to the exhaust air */
    double energy_in, energy_out;   /* J of enthalpy carried along with that mass              */
    double work_on_gas;             /* J the pistons did on the gas                            */

    /* the cycle in progress (internal) */
    double acc_air[RB_MAX_CYLINDERS], acc_throttle, acc_p, acc_rho, acc_pmin, acc_pmax;
    double acc_pump, acc_work, acc_time;
    double throttle_area_pos;       /* pedal position the throttle's area was computed for */
} rb_engine;

/* A 2.0 L inline four: 86 x 86 mm, firing order 1-3-4-2, two intake and two
 * exhaust valves per cylinder, a 60 mm throttle and a 2.5 L plenum. */
void   rb_engine_desc_default(rb_engine_desc *d);

/* 0 if the description makes a possible engine, -1 otherwise. */
int    rb_engine_desc_check(const rb_engine_desc *d);

/* Sets up the engine at rest with the crank at 0 and all the gas at ambient
 * conditions. Returns 0, or -1 (and leaves e unusable) for a bad description
 * or time step. */
int    rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt);

/* One physics step at the current rpm and throttle. */
void   rb_engine_step(rb_engine *e);

/* Runs whole cycles until the volumetric efficiency and plenum pressure
 * change by less than tol (relative) for 3 cycles in a row. Returns the
 * number of cycles run, or -1 if it didn't settle in max_cycles (or the crank
 * isn't turning). */
int    rb_engine_settle(rb_engine *e, int max_cycles, double tol);

/* Total swept volume of all cylinders (m^3). */
double rb_engine_displacement(const rb_engine *e);

/* Cylinder i's crank angle (degrees, 0..720 in its own cycle). */
double rb_engine_cylinder_angle(const rb_engine *e, int i);

#endif /* RB_ENGINE_H */
