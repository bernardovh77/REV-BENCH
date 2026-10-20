/*
 * test_dyno.c - stage 9: the absorbing dyno, and the engine calibrated
 * against real ones.
 *
 * The dyno holds a speed with the crank free and reads the brake torque the
 * engine makes there; a sweep reads the same, the inertia correction adding
 * back what speeding up the crank took; it only absorbs. The presets make
 * their real counterparts' torque and power, compression and cams move the
 * torque the way they do on real engines, the burn lengthens with speed and
 * at light load, and the engine computer puts the spark where the torque is
 * best, holding some back at idle.
 */
#include "dyno.h"
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include "rb_test.h"

#include <math.h>

#define DT_FAST (1.0 / 96000.0)   /* as in test_presets.c: the numbers come out the same */
#define PI 3.14159265358979323846

static rb_engine e, held;   /* about 20 KB each: kept off the stack */

/* Runs an engine held at `rpm` (wide open unless `throttle` says otherwise)
 * until it settles. */
static void run_held(rb_engine *x, const rb_engine_desc *d, double rpm, double throttle)
{
    rb_engine_init(x, d, DT_FAST);
    x->rpm = rpm;
    x->throttle_pos = throttle;
    x->ignition = 1;
    rb_engine_settle(x, 100, 1e-4);
}

/* The spark advance for the best torque for an engine's last cycle: the
 * engine computer puts the middle of the burn 8 degrees after TDC, which on
 * the Wiebe curve comes this long after the spark. */
static double best_spark(const rb_engine *x)
{
    const rb_engine_desc *d = &x->desc;
    return x->result.burn_duration * pow(log(2.0) / d->wiebe_a, 1.0 / (d->wiebe_m + 1.0)) - 8.0;
}

/* ---- the pipes' rate (stage 13's quality setting) ---- */

void test_engine_pipe_rate(void)
{
    /* Stepping the pipes at 24 kHz instead of 48 moves the four's torque and
     * VE by well under 1 %: wide open, and at a fifth of the throttle. */
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i4"), &d);
    static const double THROTTLE[2] = { 1.0, 0.2 };
    for (int k = 0; k < 2; k++) {
        double torque[2], ve[2];
        for (int j = 0; j < 2; j++) {
            CHECK_EQ_INT(rb_engine_init_at(&e, &d, DT_FAST, j ? 24000.0 : 48000.0), 0);
            CHECK_NEAR(e.pipe_dt, j ? 1.0 / 24000.0 : 1.0 / 48000.0, 1e-12);
            e.rpm = 3000.0;
            e.throttle_pos = THROTTLE[k];
            e.ignition = 1;
            rb_engine_settle(&e, 100, 1e-4);
            torque[j] = e.result.torque;
            ve[j] = e.result.ve;
        }
        CHECK_NEAR(torque[1], torque[0], 0.005 * torque[0]);
        CHECK_NEAR(ve[1], ve[0], 0.005 * ve[0]);
    }
    /* Faster than the physics, the pipes step with it; no rate at all is refused. */
    CHECK_EQ_INT(rb_engine_init_at(&e, &d, 1.0 / 20000.0, 48000.0), 0);
    CHECK_EQ_INT(e.pipe_every, 1);
    CHECK_EQ_INT(rb_engine_init_at(&e, &d, DT_FAST, 0.0), -1);
    CHECK_EQ_INT(rb_engine_init_at(&e, &d, DT_FAST, NAN), -1);
}

/* ---- the dyno ---- */

void test_dyno_holds_a_speed(void)
{
    /* Taking over from the ideal dyno of stages 5-8 at 3000 rpm wide open,
     * with the crank free, it pulls the speed back to 3000 within about two
     * seconds and holds it to a fraction of an rpm, reading every cycle but
     * the first (which started part-way). Holding, it absorbs all the engine
     * makes: the brake torque the engine made held there, less about 0.3 %
     * (on a free crank the speed ripples with the firings, which costs a
     * little). It never drives the engine. Asked for 100 rpm more, it gets
     * there (90 %) in about 0.15 s, as a 1.5 Hz controller should. */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    run_held(&held, &d, 3000.0, 1.0);
    run_held(&e, &d, 3000.0, 1.0);
    rb_dyno y;
    rb_dyno_init(&y, &e, 3000.0);
    CHECK(e.hold_speed == 0 && y.readings == 0);
    const long cycles0 = e.cycles;

    double lo = INFINITY, hi = 0.0, brake = 0.0, absorbed = 0.0, least = INFINITY, power_error = 0.0;
    int n = 0;
    for (long k = 0; k < lround(2.5 / DT_FAST); k++) {
        if (rb_dyno_step(&y, &e) && k > lround(1.5 / DT_FAST)) {
            const rb_dyno_reading *r = &y.reading;
            lo = fmin(lo, r->rpm);
            hi = fmax(hi, r->rpm);
            brake += r->brake_torque;
            absorbed += r->absorbed_torque;
            power_error = fmax(power_error, fabs(r->brake_power - r->brake_torque * r->rpm * PI / 30.0));
            n++;
        }
        least = fmin(least, e.load_torque);
        rb_engine_step(&e);
    }
    CHECK(n >= 20 && n <= 26);                    /* 25 cycles a second */
    CHECK_EQ_INT(y.readings, e.cycles - cycles0 - 1);
    CHECK(lo > 2999.0 && hi < 3001.0);
    CHECK_NEAR(brake / n, 0.997 * held.result.brake_torque, 0.003 * held.result.brake_torque);
    CHECK_NEAR(absorbed / n, brake / n, 1e-3 * brake / n);
    CHECK(power_error < 1e-6 * held.result.brake_power);
    CHECK(least > 0.9 * held.result.brake_torque);

    y.target_rpm = 3100.0;
    long there = -1;
    double over = 0.0;
    for (long k = 0; k < lround(1.0 / DT_FAST); k++) {
        rb_dyno_step(&y, &e);
        rb_engine_step(&e);
        if (there < 0 && y.speed > 3090.0) there = k;
        over = fmax(over, y.speed - 3100.0);
    }
    CHECK(there > lround(0.1 / DT_FAST) && there < lround(0.2 / DT_FAST));
    CHECK(over < 40.0);
    CHECK_NEAR(y.speed, 3100.0, 5.0);
}

void test_dyno_sweeps(void)
{
    /* Climbing from 2600 to 3400 rpm at 200 rpm/s, the dyno absorbs less than
     * the engine makes: the rest speeds up the crank, J dw/dt. The brake
     * torque adds that back, and over 2900-3100 rpm reads what the dyno reads
     * holding 3000: 0.3 % less than the ideal hold, the free crank's speed
     * ripple (test_dyno_holds_a_speed). The setpoint stops where the sweep
     * ends. */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    run_held(&held, &d, 3000.0, 1.0);
    run_held(&e, &d, 2600.0, 1.0);
    rb_dyno y;
    rb_dyno_init(&y, &e, 2600.0);
    rb_dyno_sweep(&y, 3400.0, 200.0);

    double brake = 0.0, inertia = 0.0, speed = 0.0, highest = 0.0;
    int n = 0;
    for (long k = 0; k < lround(5.0 / DT_FAST); k++) {
        if (rb_dyno_step(&y, &e) && y.reading.rpm > 2900.0 && y.reading.rpm < 3100.0) {
            brake += y.reading.brake_torque;
            inertia += y.reading.brake_torque - y.reading.absorbed_torque;
            speed += y.reading.rpm;
            n++;
        }
        highest = fmax(highest, y.target_rpm);
        rb_engine_step(&e);
    }
    const double j_alpha = d.inertia * 200.0 * PI / 30.0;   /* 3.4 N m */
    CHECK(n >= 20 && n <= 30);                    /* 1 s of the sweep: about 25 cycles */
    CHECK_NEAR(speed / n, 3000.0, 15.0);
    CHECK_NEAR(brake / n, 0.997 * held.result.brake_torque, 0.004 * held.result.brake_torque);
    CHECK_NEAR(inertia / n, j_alpha, 0.1 * j_alpha);
    CHECK(highest == 3400.0 && y.target_rpm == 3400.0);
    CHECK_NEAR(e.rpm, 3400.0, 40.0);
}

void test_dyno_only_absorbs(void)
{
    /* Asked for more speed than the engine makes at 8 % throttle, the dyno
     * lets go: it can't drive the engine, and the engine runs up to where its
     * own friction holds it. Its controller doesn't wind up meanwhile, so
     * asked for 2000 rpm again it brakes at once. */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    run_held(&e, &d, 2000.0, 0.08);
    rb_dyno y;
    rb_dyno_init(&y, &e, 2000.0);
    const double integral0 = y.integral;          /* the load it took over */
    y.target_rpm = 6000.0;
    double least = INFINITY, most_after = 0.0;
    for (long k = 0; k < lround(2.0 / DT_FAST); k++) {
        rb_dyno_step(&y, &e);
        least = fmin(least, e.load_torque);
        if (k > lround(0.5 / DT_FAST)) most_after = fmax(most_after, e.load_torque);
        rb_engine_step(&e);
    }
    CHECK(least == 0.0 && most_after == 0.0);
    CHECK(e.rpm > 3000.0 && e.rpm < 5000.0);
    CHECK(integral0 > 30.0 && y.integral == integral0);   /* remembered, not wound down */

    y.target_rpm = 2000.0;
    long back = -1;
    for (long k = 0; k < lround(1.5 / DT_FAST); k++) {
        rb_dyno_step(&y, &e);
        rb_engine_step(&e);
        if (back < 0 && e.rpm < 2020.0) back = k;
    }
    CHECK(back >= 0 && back < lround(0.25 / DT_FAST));
    CHECK_NEAR(e.rpm, 2000.0, 50.0);              /* still settling from the stop: the
                                                     plenum and runners (4 L) refill */
    CHECK(e.load_torque > 20.0);
}

/* ---- the calibration ---- */

/* The real engines the presets stand for, from their makers' figures: peak
 * brake torque and power, and where they come. */
typedef struct real_engine {
    const char *preset, *name;
    double torque, torque_rpm, power, power_rpm;
} real_engine;

static const real_engine REAL[] = {
    { "i4",     "Toyota 1AZ-FE (2.0 L, Camry)",       190.0, 4000.0, 108e3, 6000.0 },
    { "i6",     "BMW M54B30 (3.0 L)",                 300.0, 3500.0, 170e3, 5900.0 },
    { "boxer4", "Subaru FB20 (2.0 L, Impreza 2012)",  196.0, 4200.0, 110e3, 6200.0 },
};

void test_calibration_matches_real_engines(void)
{
    /* Stage 9's "done when": wide open, with the spark at best torque, each
     * preset's peak brake torque and power are within 15 % of its real
     * engine's (the model's come out within 9 %, a few % low since stage 12's
     * air filter and mufflers), the torque peaks in the mid range and the
     * power near the top, and the curves pass within 15 % of the real peaks
     * at the real peaks' speeds. */
    for (int r = 0; r < (int)(sizeof REAL / sizeof REAL[0]); r++) {
        const real_engine *x = &REAL[r];
        rb_engine_desc d;
        CHECK_EQ_INT(rb_preset_desc(rb_preset_find(x->preset), &d), RB_OK);
        double torque = 0.0, torque_at = 0.0, power = 0.0, power_at = 0.0;
        for (double rpm = 2000.0; rpm <= d.rev_limit - 500.0; rpm += 500.0) {
            run_held(&e, &d, rpm, 1.0);
            if (e.result.brake_torque > torque) {
                torque = e.result.brake_torque;
                torque_at = rpm;
            }
            if (e.result.brake_power > power) {
                power = e.result.brake_power;
                power_at = rpm;
            }
        }
        CHECK_NEAR(torque, x->torque, 0.15 * x->torque);
        CHECK_NEAR(power, x->power, 0.15 * x->power);
        CHECK(torque_at >= 3000.0 && torque_at <= 5000.0);
        CHECK(power_at >= 5500.0 && power_at <= d.rev_limit);
        run_held(&e, &d, x->torque_rpm, 1.0);
        CHECK_NEAR(e.result.brake_torque, x->torque, 0.15 * x->torque);
        run_held(&e, &d, x->power_rpm, 1.0);
        CHECK_NEAR(e.result.brake_power, x->power, 0.15 * x->power);
    }
}

void test_calibration_compression(void)
{
    /* More compression, more torque: one point more (10 to 11:1) gives about
     * 2.5 % at 3000 rpm wide open, as the Otto cycle's efficiency
     * (1 - r^-0.3: 2.8 %) and real engines say, and the efficiency goes up. */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    run_held(&held, &d, 3000.0, 1.0);
    d.cr = 11.0;
    run_held(&e, &d, 3000.0, 1.0);
    CHECK(e.result.brake_torque > 1.015 * held.result.brake_torque);
    CHECK(e.result.brake_torque < 1.04 * held.result.brake_torque);
    CHECK(e.result.efficiency > held.result.efficiency + 0.005);
}

void test_calibration_cams(void)
{
    /* Bigger cams, 280 degrees and 10.5 mm of lift instead of 250 and 9.5:
     * the torque moves up the rev range. At 2000 rpm the late intake closing
     * pushes charge back out and the torque falls more than 10 %; at 6000 it
     * holds up, and the peak comes at a higher speed. */
    static const double rpm[] = { 2000.0, 4000.0, 4500.0, 5000.0, 5500.0, 6000.0 };
    enum { N = sizeof rpm / sizeof rpm[0] };
    double torque[2][N], peak_at[2] = { 0.0, 0.0 };
    for (int c = 0; c < 2; c++) {
        rb_engine_desc d;
        rb_engine_desc_default(&d);
        if (c == 1) {
            d.intake.duration = d.exhaust.duration = 280.0;
            d.intake.max_lift = 10.5e-3;
            d.exhaust.max_lift = 10.0e-3;
        }
        double best = 0.0;
        for (int k = 0; k < N; k++) {
            run_held(&e, &d, rpm[k], 1.0);
            torque[c][k] = e.result.brake_torque;
            if (k > 0 && torque[c][k] > best) {
                best = torque[c][k];
                peak_at[c] = rpm[k];
            }
        }
    }
    CHECK(torque[1][0] < 0.9 * torque[0][0]);
    CHECK(torque[1][N - 1] > 0.9 * torque[0][N - 1]);
    CHECK(torque[1][N - 1] / torque[1][0] > 1.08 * torque[0][N - 1] / torque[0][0]);
    CHECK(peak_at[1] > peak_at[0]);
}

void test_calibration_burn_and_spark(void)
{
    /* The burn takes a little more crank angle at higher speed and more at
     * light load, where the charge is thinner and has more burned gas in it
     * (Heywood, chapter 9). The engine computer's spark puts the middle of
     * the burn 8 degrees after TDC, so it advances with them, and that's
     * the best torque: 5 degrees either side gives less. */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    run_held(&held, &d, 3000.0, 1.0);
    const double burn = held.result.burn_duration, spark = held.result.spark;
    CHECK_NEAR(burn, d.burn_duration, 0.03 * d.burn_duration);   /* the reference charge */
    run_held(&e, &d, 1500.0, 1.0);
    CHECK(e.result.burn_duration < 0.95 * burn && e.result.spark < spark - 1.0);
    run_held(&e, &d, 6000.0, 1.0);
    CHECK(e.result.burn_duration > 1.04 * burn && e.result.burn_duration < 1.2 * burn);
    CHECK(e.result.spark > spark + 1.0);
    run_held(&e, &d, 3000.0, 0.1);   /* light load: about 0.35 bar in the plenum */
    CHECK(e.result.burn_duration > 1.1 * burn && e.result.spark > spark + 3.0);

    /* Where the middle of the burn comes: cylinder 1's burn passing half. */
    const rb_burn *b = &held.burn[0];
    long k = 0;
    while (k < 100000 && !(b->burning && b->fraction < 0.5)) {
        rb_engine_step(&held);
        k++;
    }
    while (k < 100000 && b->burning && b->fraction < 0.5) {
        rb_engine_step(&held);
        k++;
    }
    double ca50 = rb_engine_cylinder_angle(&held, 0);
    if (ca50 > 360.0) ca50 -= 720.0;
    CHECK_NEAR(ca50, 8.0, 0.3);

    for (int side = -1; side <= 1; side += 2) {
        rb_engine_init(&e, &d, DT_FAST);
        e.rpm = 3000.0;
        e.throttle_pos = 1.0;
        e.ignition = 1;
        e.spark_map = 0;
        e.spark = spark + 5.0 * side;
        rb_engine_settle(&e, 100, 1e-4);
        CHECK(e.result.brake_torque < 0.997 * held.result.brake_torque);
    }

    /* With a wild cam (320 degrees) and the throttle shut, more than half the
     * charge is burned gas and the flame would crawl (a real engine would
     * misfire): the burn is held at twice the reference. */
    d.intake.duration = d.exhaust.duration = 320.0;
    run_held(&e, &d, 3000.0, 0.0);
    CHECK(e.result.residual > 0.5);
    CHECK_NEAR(e.result.burn_duration, 2.0 * d.burn_duration, 1e-9);
}

void test_calibration_idle_spark(void)
{
    /* Idling, the engine computer holds the spark back from the best torque,
     * a reserve, and advances it as soon as the speed sags: a 10 N m load
     * thrown on gets more spark within a few cycles, long before the idle
     * valve's air can arrive, up to the best torque's and no further. With
     * the foot on the pedal, even at idle speeds, it's the best torque's. */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    rb_engine_init(&e, &d, DT_FAST);
    e.hold_speed = 0;
    e.throttle_pos = 0.0;
    e.ignition = 1;
    e.starter = 1;
    for (long k = 0; k < lround(3.0 / DT_FAST); k++) {
        if (e.starter && e.rpm_smooth > 0.75 * d.idle_rpm) e.starter = 0;
        rb_engine_step(&e);
    }
    const double idle_spark = e.result.spark;
    CHECK_NEAR(e.result.rpm, d.idle_rpm, 15.0);
    CHECK_NEAR(idle_spark, 15.0, 2.0);
    CHECK(idle_spark < best_spark(&e) - 5.0);

    e.load_torque = 10.0;
    double most = 0.0, above = -INFINITY;   /* the most spark, and the most past the best */
    for (long k = 0; k < lround(0.8 / DT_FAST); k++) {
        rb_engine_step(&e);
        most = fmax(most, e.result.spark);
        above = fmax(above, e.result.spark - best_spark(&e));
    }
    CHECK(most > idle_spark + 5.0);
    CHECK_NEAR(above, 0.0, 1e-9);   /* it got to the best torque's spark, and no further */

    /* The foot on, the load still there: it speeds up through the idle
     * speeds, with the best torque's spark all the way. */
    e.throttle_pos = 0.05;
    long cycles = e.cycles;
    int in_band = 0, off_best = 0;
    for (long k = 0; k < lround(0.6 / DT_FAST); k++) {
        rb_engine_step(&e);
        if (e.cycles != cycles) {
            cycles = e.cycles;
            if (e.rpm_smooth > d.idle_rpm && e.rpm_smooth < d.idle_rpm + 400.0) {
                in_band++;
                off_best += !(fabs(e.result.spark - best_spark(&e)) < 1e-9);
            }
        }
    }
    CHECK(in_band >= 2);
    CHECK_EQ_INT(off_best, 0);
}
