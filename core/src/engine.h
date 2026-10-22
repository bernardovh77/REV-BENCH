/*
 * engine.h - a whole engine: N cylinders on one crankshaft, fed from an
 * intake plenum behind the throttle and the air box, exhausting through
 * pipes, a catalyst and a muffler into the open air.
 * The crank is either held at a set speed by a motor or a dyno (stages 5 and
 * 6, and dyno runs), or free: it turns under the torques of the gas,
 * friction, the starter motor and whatever it drives, with an engine computer
 * holding the idle speed and cutting the spark at the rev limit (stage 7).
 * With the ignition off the engine only breathes (stage 5); with it on, fuel
 * is injected and burned (stage 6). The cylinders can stand in a row, in a V
 * or flat (boxer), with any crank and firing order that fit together
 * (stage 8); presets.c has the engines that come ready-made.
 *
 *   air --> snorkel --> air box --> throttle --> plenum --> runner i --> intake valve
 *                                                                              |
 *   air <-- tailpipe <-- muffler <-- catalyst <-- exhaust pipe <--   cylinder i
 *                                          collector <-- header i <-- exhaust valve
 *
 * The runners, headers, exhaust pipes, mufflers, tailpipes and the snorkel
 * are 1D pipes (stages 10 and 12, pipe.h): pressure waves run along them,
 * which is what tunes an engine's breathing and makes its sound. Each
 * cylinder's header joins one of up to two collectors (on a V or a boxer
 * usually its bank's), each collector has its exhaust pipe, and the exhaust
 * pipes run into one muffler, or with two collectors each into its own (two
 * systems: a twin exhaust). An exhaust pipe opens into its muffler's first
 * cell, and the tailpipe starts from its last. The air box, the plenum and
 * the collectors are volumes (0D chambers).
 *
 * Two rates: the cylinders, the air box, the plenum and the pipes' cells at
 * the valves are stepped every physics step (dt); the rest of the pipes and
 * the collectors every `pipe_every` steps, at about 48 kHz (pipe.h).
 *
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
 *   3. everything that changes the fast gas, computed from the current state
 *      and applied together (explicit Euler, as in stage 3): flow through the
 *      throttle and the valves and along the pipes' ends, heat to or from the
 *      walls, and the burning. When a block of `pipe_every` steps begins, the
 *      slow gas's rates first (the gas along the pipes, into and out of the
 *      collectors);
 *   4. turn the crank and move every piston (the trapezoidal step of
 *      stage 4); a free crank's kinetic energy then changes by exactly the
 *      work done on it: the gas's, the crankcase's, the starter's, friction's
 *      and the load's;
 *   5. when the block ends, the slow gas is integrated over it (what came
 *      in through the snorkel and left through the tailpipes counted first)
 *      and its pipes' faces get their new flows; then, every step, the faces
 *      at the pipes' fast ends.
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
#include "pipe.h"
#include "throttle.h"
#include "valve.h"

#include <stdint.h>

#define RB_MAX_CYLINDERS 16
#define RB_MAX_COLLECTORS 2      /* and as many exhaust systems                */
/* The pipes' cells, all of them: the presets use 96 (the four) to 212 (the
 * cross-plane V8). An engine with more pipe than that gets fewer, longer
 * cells. */
#define RB_ENGINE_CELLS  512

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
    /* the way in (stage 12): a snorkel from the open air to the air box, whose
     * filter the throttle draws through */
    double airbox_volume;                 /* m^3                                      */
    double snorkel_length;                /* m                                        */
    double snorkel_diameter;              /* m                                        */
    double runner_length;                 /* m: each cylinder's intake runner, plenum
                                           * to valve (0: the valves draw straight from
                                           * the plenum, as in stages 5-8)            */
    double runner_diameter;               /* m                                        */
    /* the exhaust (stage 10) */
    double header_length[RB_MAX_CYLINDERS]; /* m: each cylinder's header (primary pipe),
                                           * exhaust valve to collector               */
    double header_diameter;               /* m                                        */
    int    collector[RB_MAX_CYLINDERS];   /* the collector each header joins: 0, or 1
                                           * for a second group (a V's other bank, an
                                           * inline six's rear three)                 */
    double exhaust_pipe_length;           /* m: each collector's pipe to its catalyst
                                           * and muffler                              */
    double exhaust_pipe_diameter;         /* m                                        */
    int    exhaust_systems;               /* 1: every exhaust pipe into one muffler and
                                           * tailpipe; 2: each collector its own (needs
                                           * two collectors)                          */
    double catalyst_area;                 /* m^2: effective area (Cd included) of each
                                           * exhaust pipe's catalyst, where it opens
                                           * into the muffler                         */
    int    muffler_type;                  /* RB_MUFFLER_* (rb.h): what each muffler is
                                           * (stage 12)                               */
    double muffler_volume;                /* m^3, each: a chambered one's             */
    double muffler_length;                /* m, each                                  */
    double muffler_area;                  /* m^2: effective area of the way through a
                                           * chambered muffler's baffles, into its
                                           * tailpipe                                 */
    double tailpipe_length;               /* m, each                                  */
    double tailpipe_diameter;             /* m                                        */
    /* Optional researched intermediate section, before the rear silencers.
     * Zero length preserves the original editable single-silencer layout.
     * The section follows each collector; its link then enters either its
     * rear silencer or a shared X/Y junction (junction_volume > 0). */
    double center_length, center_diameter, center_volume; /* m, m, m^3; 0 volume: inline bore */
    double center_packing;               /* Pa s/m^2, acoustic-flow resistance per metre */
    double link_length;                  /* m, same bore as exhaust_pipe_diameter */
    double junction_volume;              /* m^3; finite common X/Y junction, 0: separate */
    int junction_before_center;           /* 1: X before intermediate pipes; 0: rear Y */
    int outlet_radiation;                 /* 1: causal unflanged radiation load */
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
    int    heat_transfer;                 /* 1 = Woschni heat flow, 0 = adiabatic
                                           * cylinders (the pipes' walls always trade
                                           * heat: motored with the throttle shut, the
                                           * pumping work would otherwise heat the
                                           * little gas that flows to nearly 5000 K)  */
    double wall_temperature;              /* K                                        */
    /* the crank (stage 7) */
    double inertia;                       /* kg m^2: crankshaft, flywheel and clutch  */
    double fmep_constant;                 /* Pa: friction (Chen and Flynn's form):
                                           * bearings, seals, pumps                   */
    double fmep_pressure;                 /* Pa per Pa of peak cylinder pressure      */
    double fmep_speed;                    /* Pa per m/s of mean piston speed          */
    double fmep_speed2;                   /* Pa per (m/s)^2                           */
    double accessory_power;               /* W the accessories take once the engine
                                           * runs: the alternator for the car's
                                           * electrics, the pumps (stage 12c; part
                                           * of the friction, as Heywood counts it)   */
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
    double catalyst_laminar;              /* Pa per m/s: substrate's linear loss;
                                           * 0 with catalyst bypass pipes (13b-r)    */
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
    double p_exhaust;                 /* Pa: mean pressure in the exhaust collectors          */
    double t_tailpipe;                /* K: mass-averaged temperature of the gas leaving the
                                       * tailpipes                                            */
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
    double phi_ivc;               /* trapped fuel/air equivalence ratio, after injection;
                                     0 without both reactants or for an invalid ratio   */
    double spark_angle;           /* cycle angle of this cycle's spark (set at IVC)      */
    double start_angle;           /* ... and of its burn's start: the spark, give or take
                                     this cycle's flame (stage 12c; set at IVC)          */
    double demand_duration;       /* uncapped charge model, bounded to one cycle (S1b)   */
    double duration;              /* this cycle's burn: crank degrees from its start to
                                     the end (set at IVC)                                */
    int    spark_due;             /* 1 from intake valve closing until the burn starts   */
    double burnable;              /* kg of fuel set to burn at the spark                 */
    double fraction;              /* burned fraction so far, 0..1                        */
    double burn_angle;            /* the burn's clock: crank degrees since the spark     */
    int    burning;               /* 1 from the spark until the burn ends                */
    int    after_spark;           /* 1 from the spark to exhaust valve opening           */
    double heat, wall_heat;       /* J released by burning / added by the walls, last step */
    double p_max, angle_p_max;    /* highest pressure since overlap TDC, and where       */
    double last_p_max, last_angle_p_max;   /* the same for the last complete cycle       */
    double work, last_work;       /* J the gas did on the piston since overlap TDC, and
                                     over the last complete cycle (stage 12c: per
                                     cylinder, its IMEP times its swept volume)          */
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
    double variation;       /* stage 12c: how much no two cycles are alike (any time):
                             * 0 every cycle the same (the default, as the tests and
                             * the dyno numbers want), 1 a real engine's (rb_cli sound
                             * sets it): each burn's start and length vary, the
                             * cylinders differ a little, the exhaust valves' jets
                             * flicker                                                 */

    /* the gas */
    rb_cylinder cyl[RB_MAX_CYLINDERS];
    rb_burn     burn[RB_MAX_CYLINDERS];
    rb_chamber  intake_air, plenum, exhaust_air;   /* the air reservoirs are fixed */
    rb_pipe     snorkel;                           /* intake_air -> the air box, through
                                                    * the filter at its end           */
    rb_chamber  airbox;
    rb_orifice  throttle;                          /* air box -> plenum                */
    rb_orifice  idle_air;                          /* air box -> plenum, beside the
                                                    * throttle                         */
    rb_orifice  intake[RB_MAX_CYLINDERS];          /* runner i's last cell (the plenum
                                                    * with no runners) -> cylinder i   */
    rb_orifice  exhaust[RB_MAX_CYLINDERS];         /* cylinder i -> header i's first cell */
    rb_pipe     runner[RB_MAX_CYLINDERS];          /* plenum -> intake valve i         */
    rb_pipe     header[RB_MAX_CYLINDERS];          /* exhaust valve i -> its collector  */
    rb_chamber  collector[RB_MAX_COLLECTORS];      /* where a group of headers meets   */
    rb_pipe     exhaust_pipe[RB_MAX_COLLECTORS];   /* collector c -> its system's
                                                    * muffler, through the catalyst at
                                                    * its end (rb_pipe_restrict)       */
    rb_pipe     muffler[RB_MAX_COLLECTORS];        /* the exhaust pipes open into its
                                                    * first cell ...                   */
    rb_pipe     tailpipe[RB_MAX_COLLECTORS];       /* ... and this starts from its last
                                                    * (through a chambered one's
                                                    * baffles) -> the open air         */
    rb_pipe     center[RB_MAX_COLLECTORS], link[RB_MAX_COLLECTORS];
    rb_chamber  junction;                         /* common finite X/Y junction, optional */
    int         collectors, systems;               /* how many of each                 */
    int         pipe_every;                        /* the slow gas steps once every this
                                                    * many physics steps ...           */
    int         pipe_phase;                        /* ... and this many have passed     */
    double      pipe_dt;                           /* s: pipe_every dt                  */
    double      tail_flow[RB_MAX_COLLECTORS];      /* m^3/s of gas each system's
                                                    * tailpipe let out into the air
                                                    * over the last slow step (< 0:
                                                    * it drew air in) ...             */
    double      mouth_flow;                        /* ... and the intake's mouth (< 0:
                                                    * drawing air in): the sound's
                                                    * sources (audio.h)               */
    double      acoustic_energy_out;               /* J: exterior acoustic work already booked */
    long        pipe_steps;                        /* slow steps since rb_engine_init  */
    rb_chamber  cell_pool[RB_ENGINE_CELLS];        /* the pipes' cells ...             */
    rb_face     face_pool[RB_ENGINE_CELLS];        /* ... and the faces between them   */
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
    double rpm_recent;      /* ... and over 0.15 s, for how fast it's changing         */
    double idle_valve;      /* idle air valve opening, 0..1                            */
    double idle_trim;       /* the part of it learned while idling                     */
    int    spark_cut;       /* 1 while the rev limiter cuts every spark (the hard cut) */
    long   limiter_cuts;    /* sparks the rev limiter cut, soft or hard (stage 12c)    */
    int    fuel_cut;        /* 1 while the injectors are off, coasting (overrun)       */
    double overrun_transition; /* s from qualifying lift to fuel cut; 0: immediate   */
    double overrun_time;       /* s in this qualifying lift, saturated at transition */
    double overrun_retard;     /* deg removed progressively before fuel cut         */
    int overrun_armed;         /* pedal used since last return to idle               */

    /* no two cycles alike (stage 12c; `variation` above) */
    uint64_t rand_cycles;           /* the generator's state for the cycles (rand.h) ...  */
    uint64_t rand_flicker;          /* ... and for the valves' jets                       */
    double   trim_fuel[RB_MAX_CYLINDERS];   /* each cylinder's own fuel and burn length:   */
    double   trim_burn[RB_MAX_CYLINDERS];   /* fixed normal numbers, drawn at init         */
    double   flicker[RB_MAX_CYLINDERS];     /* each exhaust valve's jet now, filtered      */
    double   flicker_a, flicker_norm;       /* its filter, and what makes its RMS 1        */

    /* totals since rb_engine_init, for checking conservation */
    double mass_in, mass_out;       /* kg taken from the intake air / given to the outside air */
    double energy_in, energy_out;   /* J of enthalpy carried along with that mass              */
    double species_in[RB_NSPECIES], species_out[RB_NSPECIES];   /* the same mass, by species */
    double work_on_gas;             /* J the pistons did on the gas                            */
    double fuel_in;                 /* kg of fuel injected                                     */
    double fuel_energy_in;          /* J of sensible energy the injected fuel brought          */
    double fuel_burned;             /* kg of fuel burned                                       */
    double heat_released;           /* J released by burning                                   */
    double wall_heat;               /* J the cylinders' walls gave the gas (negative: the gas
                                     * lost heat); the pipes' walls: rb_engine_pipe_heat()   */
    double crank_gas_work;          /* J the gas did on the crank through the pistons          */
    double crank_case_work;         /* J the crank did on the crankcase air (at ambient)       */
    double starter_work, friction_work, load_work;   /* J into / out of the crank          */
    double hold_work;               /* J the motor or dyno put in to hold the speed            */
    double stop_energy;             /* J added when a free crank stops part-way through a step */

    /* the cycle in progress (internal) */
    double acc_air[RB_MAX_CYLINDERS], acc_throttle, acc_p, acc_rho, acc_pmin, acc_pmax;
    double acc_pump, acc_work, acc_time, acc_fuel, acc_heat, acc_wall, acc_exh_m, acc_exh_mT;
    double acc_pexh, acc_friction, acc_tail_m, acc_tail_mT;
    double throttle_area_pos;       /* pedal position the throttle's area was computed for */
} rb_engine;

/* A 2.0 L inline four: 86 x 86 mm, firing order 1-3-4-2, two intake and two
 * exhaust valves per cylinder, an 8 L air box, a 60 mm throttle, a 2.5 L
 * plenum, 4-into-1 headers, one exhaust pipe and a chambered 10 L muffler. */
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
 * bad description or time step. The pipes step at about RB_PIPE_RATE. */
int    rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt);

/* The pipes' rate (Hz): how often their slow cells are stepped, a whole
 * number of physics steps (stage 13's quality setting). */
#define RB_PIPE_RATE     48000.0
#define RB_PIPE_RATE_MIN 12000.0
#define RB_PIPE_RATE_MAX 96000.0

/* rb_engine_init with the pipes at about `pipe_rate` (Hz): a whole number of
 * physics steps, and every step when the physics is slower. The public API
 * keeps it within RB_PIPE_RATE_MIN..RB_PIPE_RATE_MAX (rb_config). */
int    rb_engine_init_at(rb_engine *e, const rb_engine_desc *d, double dt, double pipe_rate);

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

/* All the gas inside the engine (cylinders, air box, plenum, pipes,
 * collectors): kg, J (internal, and the pipes' gas's kinetic energy) and kg
 * of each species. NULL skips one. */
void   rb_engine_contents(const rb_engine *e, double *mass, double *energy,
                          double species[RB_NSPECIES]);

/* J the pipes' walls have given the gas since rb_engine_init (< 0: lost). */
double rb_engine_pipe_heat(const rb_engine *e);

/* How many pipes the engine has, and pipe k: the runners first (one per
 * cylinder, if it has them), then the headers, the exhaust pipes (one per
 * collector), the tailpipes (one per system), the mufflers (one per system)
 * and the snorkel. NULL if k is out of range. */
int    rb_engine_pipe_count(const rb_engine *e);
const rb_pipe *rb_engine_pipe(const rb_engine *e, int k);

#endif /* RB_ENGINE_H */
