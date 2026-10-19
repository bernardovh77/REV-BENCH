/*
 * engine.h - a whole engine: N cylinders on one crankshaft, fed from an
 * intake plenum behind the throttle, exhausting through a manifold, a
 * catalyst, a muffler and a tailpipe into the open air.
 * The crank is either held at a set speed by a motor or a dyno (stages 5 and
 * 6, and dyno runs), or free: it turns under the torques of the gas,
 * friction, the starter motor and whatever it drives, with an engine computer
 * holding the idle speed and cutting the spark at the rev limit (stage 7).
 * With the ignition off the engine only breathes (stage 5); with it on, fuel
 * is injected and burned (stage 6). The cylinders can stand in a row, in a V
 * or flat (boxer), with any crank and firing order that fit together
 * (stage 8); presets.c has the engines that come ready-made.
 *
 *   ambient air --> throttle --> plenum --> intake valves --> cylinder 1..N
 *                                                                  |
 *   ambient air <-- tailpipe <-- muffler <-- catalyst <-- exhaust  |
 *                                                         manifold <-- exhaust valves
 *
 * The exhaust is two volumes for now (stage 10 replaces it with pipes): the
 * manifold catches the exhaust pulses, and the muffler holds old exhaust gas,
 * so when the pressure dips at light load, the engine pulls back exhaust gas
 * through its tailpipe rather than fresh air.
 * One physics step (dt):
 *   0. how far the crank turns: rpm dt when held; when free, from its speed
 *      and the torques on it now (and the engine computer has its say);
 *   1. events for each cylinder whose crank passes them during the step:
 *      intake valve closing (inject the fuel for the trapped air, then note
 *      the trapped charge), the spark (start burning), exhaust valve opening
 *      (stop burning), overlap TDC (close the books on the cylinder's peak
 *      pressure);
 *   2. set every opening's area: the throttle from the pedal, each valve
 *      from its cam at the current crank angle;
 *   3. everything that changes the gas, computed from the current state and
 *      applied together (explicit Euler, as in stage 3): flow through the
 *      openings, heat to or from the walls, and the burning;
 *   4. turn the crank and move every piston (the trapezoidal step of
 *      stage 4); a free crank's kinetic energy then changes by exactly the
 *      work done on it: the gas's, the crankcase's, the starter's, friction's
 *      and the load's.
 * At the end of every 720-degree cycle the engine adds up what went through
 * it: volumetric efficiency, plenum pressure, work, fuel, heat. The step that
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

/* Everything that defines an engine. presets.c fills one in for each preset,
 * and the public API reads and changes it field by field (rb_engine_api.c). */
typedef struct rb_engine_desc {
    int    cylinders;                     /* 1..RB_MAX_CYLINDERS                      */
    double bore, stroke, rod, cr;         /* m, m, m, compression ratio (cylinder.h)  */
    /* layout (stage 8): where each cylinder sits and when it fires. Angles are
     * degrees, measured in the direction the crank turns. A cylinder's piston
     * is at TDC when its crank pin points along its bank: at crank angle
     * bank_angle[bank[i]] - pin[i] (mod 360). */
    int    banks;                         /* 1 (inline), 2 (V or boxer)               */
    double bank_angle[2];                 /* each bank's cylinder axis from vertical  */
    int    bank[RB_MAX_CYLINDERS];        /* the bank each cylinder is in             */
    double pin[RB_MAX_CYLINDERS];         /* each cylinder's crank pin, its angle on
                                           * the crank                                */
    double position[RB_MAX_CYLINDERS];    /* m along the crank from the front (for
                                           * drawing it)                              */
    int    firing_order[RB_MAX_CYLINDERS]; /* cylinders in firing order, 0 = cylinder 1,
                                           * which comes first                        */
    double firing[RB_MAX_CYLINDERS];      /* degrees after cylinder 1 that each cylinder
                                           * reaches its firing TDC (firing[0] = 0):
                                           * worked out by rb_engine_layout            */
    rb_valve    intake, exhaust;          /* each cylinder's valves (valve.h)         */
    rb_throttle throttle;                 /* throttle.h                               */
    double plenum_volume;                 /* m^3                                      */
    double runner_length;                 /* m: each cylinder's intake runner, plenum
                                           * to valve (0: the valves draw straight from
                                           * the plenum, as in stages 5-8)            */
    double runner_diameter;               /* m                                        */
    double exhaust_volume;                /* m^3: headers and collector               */
    double catalyst_area;                 /* m^2: effective area (Cd included) of the
                                           * downpipe and catalyst                    */
    double muffler_volume;                /* m^3: mufflers and the pipes between them */
    double tailpipe_area;                 /* m^2: effective area of the way out       */
    double p_ambient, T_ambient;          /* Pa, K: the air around the engine         */
    /* combustion (combustion.h) */
    double burn_duration;                 /* crank degrees from the spark to the end,
                                           * at 3000 rpm with a wide-open charge; it
                                           * changes with speed and the charge (stage 9,
                                           * see burn_duration() in engine.c)          */
    double wiebe_a, wiebe_m;              /* shape of the burn                        */
    double mixture_spread;                /* spread of the local fuel-air ratio, as a
                                           * standard deviation of phi (combustion.h) */
    double combustion_efficiency;         /* share of the burnable fuel that burns    */
    /* walls (heat.h) */
    int    heat_transfer;                 /* 1 = Woschni heat flow, 0 = adiabatic     */
    double wall_temperature;              /* K                                        */
    /* the crank (stage 7) */
    double inertia;                       /* kg m^2: crankshaft, flywheel and clutch  */
    double fmep_constant;                 /* Pa: friction (Chen and Flynn's form):
                                           * bearings, seals, pumps                   */
    double fmep_pressure;                 /* Pa per Pa of peak cylinder pressure      */
    double fmep_speed;                    /* Pa per m/s of mean piston speed          */
    double fmep_speed2;                   /* Pa per (m/s)^2                           */
    double starter_torque;                /* N m at the crank, starter stalled        */
    double starter_free_rpm;              /* crank rpm where the starter stops pushing */
    /* the engine computer (stage 7; it only runs with the crank free) */
    double idle_rpm;                      /* idle speed it holds (0 = no idle control
                                           * and no fuel cut on overrun)             */
    double idle_valve_area;               /* m^2: effective area of the idle air
                                           * valve beside the throttle, fully open    */
    double idle_gain_p;                   /* idle valve opening per rpm below idle    */
    double idle_gain_i;                   /* opening it learns per rpm per second of
                                           * error, while idling                      */
    double rev_limit;                     /* rpm: the spark is cut above this ...     */
    double rev_limit_resume;              /* ... until the crank is back below this   */
    double cranking_rpm;                  /* below this the engine counts as cranking ... */
    double cranking_spark;                /* ... and sparks this late (deg before TDC) */
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
    double imep_gross;                /* Pa: work of the compression and power strokes /
                                       * displacement                                         */
    double imep_net;                  /* Pa: net_work / displacement = imep_gross - pmep      */
    double torque;                    /* N m: indicated, net_work / (4 pi)                    */
    double power;                     /* W: indicated                                         */
    double fuel_flow;                 /* kg/s injected                                        */
    double afr;                       /* air drawn in / fuel injected (0 with no fuel)        */
    double efficiency;                /* indicated: net_work / (fuel * RB_FUEL_LHV)           */
    double heat_released;             /* J from burning, all cylinders                        */
    double wall_loss;                 /* J of heat lost to the walls (< 0: walls warmed the gas) */
    double t_exhaust;                 /* K: mass-averaged temperature of the gas leaving
                                       * through the exhaust valves                          */
    double p_exhaust;                 /* Pa: mean exhaust manifold pressure                   */
    double p_max;                     /* Pa: peak cylinder pressure, mean over the cylinders  */
    double angle_p_max;               /* degrees after firing TDC where it happens (mean)     */
    double residual;                  /* burned-gas share of the trapped charge (mean)        */
    double spark;                     /* spark advance, degrees before TDC (mean)             */
    double burn_duration;             /* crank degrees from spark to end of burn (mean)       */
    double rpm;                       /* mean crank speed over the cycle                      */
    double fmep;                      /* Pa: friction mean effective pressure                 */
    double bmep;                      /* Pa: brake mean effective pressure, imep_net - fmep   */
    double brake_torque;              /* N m: what the crank delivers, torque minus friction  */
    double brake_power;               /* W                                                    */
} rb_engine_cycle;

/* One cylinder's combustion bookkeeping. */
typedef struct rb_burn {
    double p_ivc, V_ivc, T_ivc;   /* trapped charge at intake valve closing, fuel and all
                                     (Woschni's reference)                               */
    double residual;              /* its burned-gas mass fraction                        */
    double spark_angle;           /* cycle angle of this cycle's spark (set at IVC)      */
    double duration;              /* this cycle's burn: crank degrees from the spark to
                                     the end (set at IVC)                                */
    int    spark_due;             /* 1 from intake valve closing until the spark         */
    double burnable;              /* kg of fuel set to burn at the spark                 */
    double fraction;              /* burned fraction so far, 0..1                        */
    double burn_angle;            /* the burn's clock: crank degrees since the spark     */
    int    burning;               /* 1 from the spark until the burn ends                */
    int    after_spark;           /* 1 from the spark to exhaust valve opening           */
    double heat, wall_heat;       /* J released by burning / added by the walls, last step */
    double p_max, angle_p_max;    /* highest pressure since overlap TDC, and where       */
    double last_p_max, last_angle_p_max;   /* the same for the last complete cycle       */
} rb_burn;

/* The orifices point at chambers inside the same struct, so an rb_engine must
 * stay where rb_engine_init put it: don't copy it (init another one instead). */
typedef struct rb_engine {
    rb_engine_desc desc;
    double dt;              /* s, physics step                                         */
    int    hold_speed;      /* 1: a motor or dyno holds the crank at rpm (the default);
                             * 0: the crank is free (stage 7)                          */
    double rpm;             /* crank speed: set it while held; free, it's the result   */
    int    starter;         /* 1 = starter motor engaged, key held at START (free)     */
    double load_torque;     /* N m taken by what the crank drives (free; < 0 drives it) */
    double throttle_pos;    /* pedal, 0..1 (set it any time)                           */
    int    ignition;        /* 1 = injectors and spark on, 0 = motored (any time)      */
    double afr;             /* air-fuel ratio (by mass) the injectors aim for          */
    int    spark_map;       /* 1: the engine computer times the spark, aiming the
                             * middle of the burn just after TDC (the default);
                             * 0: the spark comes at `spark`                          */
    double spark;           /* spark advance, crank degrees before firing TDC, used
                             * when spark_map is 0                                     */
    double theta;           /* degrees, cylinder 1's crank angle in the cycle          */
    long   cycles;          /* cycles completed since rb_engine_init                    */
    rb_engine_cycle result; /* the last completed cycle                                */

    /* the gas */
    rb_cylinder cyl[RB_MAX_CYLINDERS];
    rb_burn     burn[RB_MAX_CYLINDERS];
    rb_chamber  intake_air, plenum, manifold, muffler, exhaust_air;   /* the air reservoirs are fixed */
    rb_orifice  throttle;                          /* intake_air -> plenum             */
    rb_chamber  port[RB_MAX_CYLINDERS];            /* the gas in intake runner i (stage 9) */
    rb_duct     runner[RB_MAX_CYLINDERS];          /* plenum -> port i: its moving column */
    rb_orifice  intake[RB_MAX_CYLINDERS];          /* port i (the plenum with no runners)
                                                    * -> cylinder i                     */
    rb_orifice  exhaust[RB_MAX_CYLINDERS];         /* cylinder i -> manifold           */
    rb_orifice  catalyst;                          /* manifold -> muffler              */
    rb_orifice  tailpipe;                          /* muffler -> exhaust_air           */
    rb_orifice  idle_air;                          /* intake_air -> plenum, beside the
                                                    * throttle                         */
    rb_gas      fuel_gas;                          /* pure fuel vapour, for injection  */

    /* the crank */
    double omega;               /* rad/s                                               */
    double kinetic;             /* J: 1/2 inertia omega^2                              */
    double gas_torque;          /* N m from the gas (net of the crankcase), last step  */
    double friction_torque;     /* N m, last step                                      */
    double starter_torque_now;  /* N m, last step                                      */
    double load_torque_now;     /* N m the load took, last step                        */

    /* the engine computer (runs only with the crank free) */
    double rpm_smooth;      /* crank speed smoothed over a few firing pulses ...       */
    double rpm_trend;       /* ... and over half a second                              */
    double idle_valve;      /* idle air valve opening, 0..1                            */
    double idle_trim;       /* the part of it learned while idling                     */
    int    spark_cut;       /* 1 while the rev limiter cuts the spark                  */
    int    fuel_cut;        /* 1 while the injectors are off, coasting (overrun)       */

    /* totals since rb_engine_init, for checking conservation */
    double mass_in, mass_out;       /* kg taken from the intake air / given to the outside air */
    double energy_in, energy_out;   /* J of enthalpy carried along with that mass              */
    double species_in[RB_NSPECIES], species_out[RB_NSPECIES];   /* the same mass, by species */
    double work_on_gas;             /* J the pistons did on the gas                            */
    double fuel_in;                 /* kg of fuel injected                                     */
    double fuel_energy_in;          /* J of sensible energy the injected fuel brought          */
    double fuel_burned;             /* kg of fuel burned                                       */
    double heat_released;           /* J released by burning                                   */
    double wall_heat;               /* J the walls gave the gas (negative: the gas lost heat)  */
    double crank_gas_work;          /* J the gas did on the crank through the pistons          */
    double crank_case_work;         /* J the crank did on the crankcase air (at ambient)       */
    double starter_work, friction_work, load_work;   /* J into / out of the crank          */
    double hold_work;               /* J the motor or dyno put in to hold the speed            */
    double stop_energy;             /* J added when a free crank stops part-way through a step */

    /* the cycle in progress (internal) */
    double acc_air[RB_MAX_CYLINDERS], acc_throttle, acc_p, acc_rho, acc_pmin, acc_pmax;
    double acc_pump, acc_work, acc_time, acc_fuel, acc_heat, acc_wall, acc_exh_m, acc_exh_mT;
    double acc_pexh, acc_friction;
    double throttle_area_pos;       /* pedal position the throttle's area was computed for */
} rb_engine;

/* A 2.0 L inline four: 86 x 86 mm, firing order 1-3-4-2, two intake and two
 * exhaust valves per cylinder, a 60 mm throttle, a 2.5 L plenum, a 2 L
 * exhaust manifold and a 10 L muffler. */
void   rb_engine_desc_default(rb_engine_desc *d);

/* 0 if the description makes a possible engine, -1 otherwise. That includes
 * firing[] agreeing with the layout (see rb_engine_layout). */
int    rb_engine_desc_check(const rb_engine_desc *d);

/* Works out firing[] from the layout. Each cylinder in the firing order fires
 * at its first TDC after the one before it; cylinder 1 fires at 0. Returns 0,
 * or -1 if the layout can't work: a bad count, bank or angle, a cylinder
 * missing from the firing order or in it twice, cylinder 1 not first, or an
 * order that doesn't fit in one 720-degree cycle (d is then unchanged). */
int    rb_engine_layout(rb_engine_desc *d);

/* Gives d an inline layout of n cylinders (1..RB_MAX_CYLINDERS) firing evenly
 * in the order 1, 2, ..., n, with a crank to match, the cylinders a bore and
 * 10 mm apart. Returns 0, or -1 (d unchanged) for a bad n or a row of
 * cylinders longer than 10 m. */
int    rb_engine_layout_inline(rb_engine_desc *d, int n);

/* Sets up the engine at rest with the crank at 0, all the gas at ambient
 * conditions, the ignition off, a stoichiometric mixture and the default
 * spark advance. The crank starts held (at rpm 0 until you set it); set
 * hold_speed to 0 to free it. Returns 0, or -1 (and leaves e unusable) for a
 * bad description or time step. */
int    rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt);

/* One physics step with the current settings: rpm (held), starter and load
 * (free), throttle, ignition, afr and spark. */
void   rb_engine_step(rb_engine *e);

/* Friction mean effective pressure (Pa) at crank speed omega (rad/s), with
 * the last cycle's peak cylinder pressure: Chen and Flynn's
 * fmep = c0 + c1 p_max + c2 S_p + c3 S_p^2, S_p the mean piston speed. */
double rb_engine_fmep(const rb_engine *e, double omega);

/* Runs whole cycles until the volumetric efficiency, plenum pressure and net
 * work change by less than tol (relative) for 3 cycles in a row. Returns the
 * number of cycles run, or -1 if it didn't settle in max_cycles (or the crank
 * turns slower than 10 rpm, where a single cycle takes millions of steps). */
int    rb_engine_settle(rb_engine *e, int max_cycles, double tol);

/* Total swept volume of all cylinders (m^3). */
double rb_engine_displacement(const rb_engine *e);

/* Cylinder i's crank angle (degrees, 0..720 in its own cycle). */
double rb_engine_cylinder_angle(const rb_engine *e, int i);

#endif /* RB_ENGINE_H */
