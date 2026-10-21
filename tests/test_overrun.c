/* ECU overrun timing: deterministic state transitions, not an audio envelope. */
#include "engine.h"
#include "rb_test.h"
#include <math.h>

#define PI 3.14159265358979323846
static rb_engine e;

static void running(void)
{
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    rb_engine_init(&e, &d, 1.0 / 96000.0);
    e.hold_speed = 0;
    e.ignition = 1;
    e.throttle_pos = 0.0;
    e.overrun_armed = 1; /* a preceding open pedal, before this driven lift */
}

/* A driven crank lets us test the ECU at specified speeds independently
 * of a decelerating engine's instantaneous pumping torque. */
static void at_rpm(double rpm)
{
    e.omega = rpm * PI / 30.0;
    e.kinetic = 0.5 * e.desc.inertia * e.omega * e.omega;
    e.rpm_smooth = e.rpm_recent = e.rpm_trend = rpm;
    rb_engine_step(&e);
}

void test_overrun_transition(void)
{
    running();
    e.overrun_transition = 0.15;
    for (int k = 0; k < 14000; k++)
        at_rpm(3000.0);
    CHECK(!e.fuel_cut);
    CHECK(e.overrun_time > 0.14 && e.overrun_time < 0.15);
    /* Entry continues in the hysteresis band, without chattering. */
    for (int k = 0; k < 500; k++)
        at_rpm(e.desc.idle_rpm + 500.0);
    CHECK(e.fuel_cut);
    const double fuel = e.fuel_in;
    for (int k = 0; k < 10000; k++)
        at_rpm(3000.0);
    CHECK_NEAR(e.fuel_in, fuel, 1e-14);
    CHECK_NEAR(e.overrun_time, 0.15, 1e-12);
    at_rpm(e.desc.idle_rpm + 300.0);
    CHECK(!e.fuel_cut && e.overrun_time == 0.0);
}

void test_overrun_reset_and_legacy(void)
{
    running();
    for (int k = 0; k < 6000; k++)
        at_rpm(3000.0);
    e.throttle_pos = 0.2;
    at_rpm(3000.0);
    CHECK(!e.fuel_cut && e.overrun_time == 0.0);
    e.throttle_pos = 0.0;
    for (int k = 0; k < 6000; k++)
        at_rpm(3000.0);
    CHECK(!e.fuel_cut); /* interrupted entry must earn its delay again */
    e.ignition = 0;
    at_rpm(3000.0);
    CHECK(e.overrun_time == 0.0);
    e.ignition = 1;
    e.overrun_transition = 0.0;
    at_rpm(3000.0);
    CHECK(e.fuel_cut); /* explicit immediate-cut control */
    e.hold_speed = 1;
    e.rpm = 3000.0;
    rb_engine_step(&e);
    CHECK(!e.fuel_cut && e.overrun_time == 0.0);
}

void test_overrun_retards_combustion(void)
{
    double advance[2];
    for (int control = 0; control < 2; control++) {
        running();
        e.overrun_retard = control ? 0.0 : 12.0;
        for (int k = 0; k < 10000; k++)
            at_rpm(3000.0);
        advance[control] = fmod(720.0 - e.burn[0].spark_angle + 360.0, 720.0) - 360.0;
    }
    /* A later scheduled spark, measured at IVC, while both runs still fuel.
     * This catches a decorative timer which never changes combustion. */
    CHECK(advance[0] < advance[1] - 2.0);
    CHECK(!e.fuel_cut);
    running();
    e.overrun_armed = 0; /* key-start overspeed is not a preceding pedal lift */
    at_rpm(3000.0);
    CHECK(e.fuel_cut && e.overrun_time == 0.0);
}
