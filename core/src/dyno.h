/*
 * dyno.h - an absorbing engine dynamometer (stage 9).
 *
 * The engine runs with its crank free (hold_speed = 0), and the dyno brakes
 * it to hold a speed, the way an eddy-current dyno does: each step it sets the
 * engine's load_torque from a PI controller on the crank speed, smoothed over
 * a few firing pulses. It only absorbs (it can't drive the engine), up to
 * max_torque. Its setpoint can climb at a steady rate: a sweep test.
 *
 * Every engine cycle it takes a reading: the torque it absorbed, and the
 * crank's brake torque, which adds what went into speeding up the crank,
 * flywheel and clutch (the inertia correction). Held at a steady speed the
 * two are the same; in a sweep only the brake torque is the engine's.
 *
 *   rb_dyno y;
 *   rb_dyno_init(&y, &engine, 1500.0);       the engine must be running
 *   rb_dyno_sweep(&y, 6500.0, 100.0);        optional: climb at 100 rpm/s
 *   for (;;) {
 *       if (rb_dyno_step(&y, &engine)) ... y.reading is the last cycle ...
 *       rb_engine_step(&engine);
 *   }
 */
#ifndef RB_DYNO_H
#define RB_DYNO_H

#include "engine.h"

typedef struct rb_dyno_reading {
    double rpm;               /* mean crank speed over the cycle                   */
    double target_rpm;        /* the setpoint when it ended                        */
    double absorbed_torque;   /* N m the dyno took, mean over the cycle's angle    */
    double brake_torque;      /* N m: absorbed, plus what sped up the crank        */
    double brake_power;       /* W                                                 */
} rb_dyno_reading;

typedef struct rb_dyno {
    double target_rpm;        /* the speed it holds; a sweep moves it              */
    double sweep_rate;        /* rpm/s the setpoint climbs at (0: it holds)        */
    double sweep_to;          /* rpm where the climb stops                         */
    double kp, ki;            /* N m per rpm, N m per rpm per second                */
    double max_torque;        /* N m it can absorb                                 */
    double speed;             /* rpm it measures: the crank's, smoothed            */
    double integral;          /* N m: the controller's integral part               */
    rb_dyno_reading reading;  /* the last complete cycle                           */
    long   readings;          /* how many complete cycles it has read              */
    /* where the cycle in progress started (internal) */
    int    primed;
    long   cycles0;
    double theta0, load_work0, kinetic0, time, time0;
} rb_dyno;

/* Puts the dyno on a running engine: frees its crank and holds it at `rpm`
 * from now on. The controller's gains suit the engine's inertia (about 1.5 Hz
 * of bandwidth), and it starts out absorbing what the engine made in its last
 * cycle, so taking over from a crank held at that speed barely disturbs it. */
void rb_dyno_init(rb_dyno *y, rb_engine *e, double rpm);

/* Starts the setpoint climbing at `rate` rpm/s, up to `to_rpm`. */
void rb_dyno_sweep(rb_dyno *y, double to_rpm, double rate);

/* Call before each rb_engine_step: moves the setpoint and sets the engine's
 * load_torque. Returns 1 if the engine finished a cycle since the last call
 * (the reading is then in y->reading), 0 otherwise. The first cycle after
 * rb_dyno_init, which started part-way, gives no reading. */
int  rb_dyno_step(rb_dyno *y, rb_engine *e);

#endif /* RB_DYNO_H */
