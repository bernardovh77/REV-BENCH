/*
 * engine_parts.h - what engine.c (the step and the books), engine_build.c
 * (the description and building) and engine_computer.c (the engine
 * computer) share: internal to the core (stage 130 split them).
 */
#ifndef RB_ENGINE_PARTS_H
#define RB_ENGINE_PARTS_H

#include "engine.h"
#include "rb.h"

static const double PI = 3.14159265358979323846;
static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
static const double FUEL[RB_NSPECIES] = { [RB_FUEL] = 1.0 };

/* Spark advance at start-up, degrees before firing TDC: about the best for
 * torque with the default engine wide open at 3000 rpm (stage 6). */
#define RB_SPARK_DEFAULT 25.0

/* Below this crank speed the gas sets its own pace: the flame burns at least
 * as fast, and Woschni's gas velocity stays at least as high, as at this speed.
 * It matters only while starting and stopping: a crank that stalls with a
 * burn under way still finishes it, and the hot gas still cools. */
#define RB_GAS_PACE_RPM 100.0

/* No two cycles alike (stage 12c). In a real engine each burn starts and
 * runs its own way: the turbulence and the mixture at the plug when it fires
 * set how soon the flame gets going and how fast it then spreads (Heywood,
 * section 9.4; Ozdor, Dulger and Sher, SAE 940987). Its first 10 % varies
 * most, and a slow start mostly means a slow burn. The slower and more
 * diluted the burn (idling, a light load), the more crank degrees it varies
 * by: the cylinders' work varies by 1-3 % of the mean (the COV of IMEP) wide
 * open, and 4-8 % idling, where the spark is held back from its best and the
 * work changes faster with the burn's timing. So each cycle, at intake valve
 * closing, the flame's development delay after the spark is drawn, never
 * negative, with an RMS spread of VARY_START times the expected burn length
 * (an exponential, combustion.c: rb_kernel_delay_unit; stage 115), and the
 * bulk duration varies by VARY_LENGTH. Both scale with `variation`; the two
 * normal numbers they come from are VARY_ALIKE alike. This causal surrogate
 * is not a fitted physical delay law (stages 040, 041 and 115). The engine
 * computer times the spark for the burn it expects, its mean delay
 * included, not this one.
 * The cylinders also differ a little, for good: the injectors and the ports'
 * air give each its own mixture (CYL_FUEL) and swirl, so its own burn length
 * (CYL_BURN); the oxygen sensor keeps their mean on the target, and on
 * average they burn as one. And the exhaust valves' jets are turbulent: the
 * gas through each flickers by FLICKER (RMS) below FLICKER_HZ, a rasp the
 * pipes carry on. Calibrated in docs/stage-12c.md. */
#define VARY_START   0.07      /* of the burn's length, RMS                        */
#define VARY_LENGTH  0.06      /* relative, RMS                                    */
#define VARY_ALIKE   0.5       /* the correlation of the two                       */
#define VARY_MOST    3.0       /* no normal number goes further than this          */
#define VARY_TOP     4.0       /* the most `variation` counts                      */
#define CYL_FUEL     0.02      /* RMS                                              */
#define CYL_BURN     0.04      /* RMS                                              */
#define FLICKER      0.15      /* RMS: as real engines, the sound between 0.5 and
                                  2 kHz then changes more than it repeats from
                                  one cycle to the next (docs/stage-12c.md)        */
#define FLICKER_HZ   2000.0
#define RB_ENGINE_SEED  0x5EED12C0FFEEULL

/* The variation to use: 0 for a NaN or anything negative, at most VARY_TOP. */
static inline double variation(const rb_engine *e)
{
    const double v = e->variation;
    return (v > 0.0) ? ((v < VARY_TOP) ? v : VARY_TOP) : 0.0;
}

/* engine.c: a new cycle's books begin. */
void   rb_engine_start_cycle(rb_engine *e);

/* engine_computer.c */
double rb_computer_spark_advance(double s);
double rb_computer_burn_duration(const rb_engine *e, rb_burn *b, double rpm);
double rb_computer_spark(const rb_engine *e, double duration, double kernel_delay, double rpm);
int    rb_computer_limiter_cut(rb_engine *e);
void   rb_computer_step(rb_engine *e);

#endif
