/*
 * dyno.c - the absorbing dynamometer (see dyno.h).
 */
#include "dyno.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DYNO_BANDWIDTH 1.5    /* Hz: how quickly it pulls the speed back         */
#define DYNO_SMOOTHING 0.02   /* s: its speed measurement, over a few firings    */
#define DYNO_MAX_TORQUE 1e4   /* N m                                             */

/* Note where a cycle starts. */
static void mark(rb_dyno *y, const rb_engine *e)
{
    y->cycles0 = e->cycles;
    y->theta0 = e->theta;
    y->load_work0 = e->load_work;
    y->kinetic0 = e->kinetic;
    y->time0 = y->time;
}

void rb_dyno_init(rb_dyno *y, rb_engine *e, double rpm)
{
    /* PI on a crank of inertia J: kp = J wc puts the crossover at wc; the
     * integral's zero sits a quarter of the way there. */
    const double wc = 2.0 * PI * DYNO_BANDWIDTH;
    memset(y, 0, sizeof *y);
    y->target_rpm = rpm;
    y->sweep_to = rpm;
    y->kp = e->desc.inertia * wc * PI / 30.0;
    y->ki = 0.25 * wc * y->kp;
    y->max_torque = DYNO_MAX_TORQUE;
    y->speed = e->rpm;
    y->integral = (e->result.brake_torque > 0.0) ? e->result.brake_torque : 0.0;
    e->hold_speed = 0;
    mark(y, e);
}

void rb_dyno_sweep(rb_dyno *y, double to_rpm, double rate)
{
    y->sweep_to = to_rpm;
    y->sweep_rate = (rate > 0.0) ? rate : 0.0;   /* NaN: no sweep */
}

int rb_dyno_step(rb_dyno *y, rb_engine *e)
{
    int done = 0;

    /* 1. A reading of the cycle the engine just finished: over the angle the
     * crank turned since the last one, the work the dyno absorbed, and that
     * plus the crank's gain in kinetic energy (its brake work). */
    if (e->cycles != y->cycles0) {
        const double angle = ((e->cycles - y->cycles0) * 720.0 + e->theta - y->theta0) * PI / 180.0;
        const double time = y->time - y->time0;
        if (y->primed && angle > 0.0 && time > 0.0) {
            const double absorbed = e->load_work - y->load_work0;
            const double brake = absorbed + (e->kinetic - y->kinetic0);
            y->reading.rpm = angle / time * 30.0 / PI;
            y->reading.target_rpm = y->target_rpm;
            y->reading.absorbed_torque = absorbed / angle;
            y->reading.brake_torque = brake / angle;
            y->reading.brake_power = brake / time;
            y->readings++;
            done = 1;
        }
        y->primed = 1;
        mark(y, e);
    }

    /* 2. The setpoint climbs during a sweep. */
    if (y->target_rpm < y->sweep_to) {
        y->target_rpm += y->sweep_rate * e->dt;
        if (y->target_rpm > y->sweep_to) y->target_rpm = y->sweep_to;
    }

    /* 3. Brake harder the faster the crank turns than it should. The integral
     * stops growing while the brake is pinned at either end, so it comes off
     * the stop as soon as the speed asks for it. */
    const double rpm = (e->rpm == e->rpm) ? e->rpm : 0.0;   /* NaN: treat as stopped */
    y->speed += (rpm - y->speed) * (e->dt / DYNO_SMOOTHING);
    const double error = y->speed - y->target_rpm;
    const double load = y->integral + y->kp * error;
    if (!(load > y->max_torque && error > 0.0) && !(load < 0.0 && error < 0.0)) {
        y->integral += y->ki * error * e->dt;
        if (y->integral < 0.0) y->integral = 0.0;
        if (y->integral > y->max_torque) y->integral = y->max_torque;
    }
    e->load_torque = (load > 0.0) ? ((load < y->max_torque) ? load : y->max_torque) : 0.0;
    y->time += e->dt;
    return done;
}
