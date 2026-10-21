/*
 * test_variation.c - stage 12c: no two cycles alike (each burn's own start
 * and length, the cylinders' own mixture and burn, the exhaust valves' jets
 * flickering), the accessories, the rev limiter's soft cut, and a dyno on the
 * sim's crank.
 */
#include "dyno.h"
#include "combustion.h"
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include "rb_internal.h"   /* the sim's engine and dyno */
#include "rb_test.h"
#include "valve.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DT_FAST (1.0 / 96000.0)

static rb_engine e, e2;   /* about 320 KB each: kept off the stack */

static void preset_desc(const char *key, rb_engine_desc *d)
{
    rb_preset_desc(rb_preset_find(key), d);
}

/* The four held wide open at 3000 rpm, fired, with this much variation. */
static void held(rb_engine *x, double variation)
{
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(x, &d, DT_FAST);
    x->rpm = 3000.0;
    x->throttle_pos = 1.0;
    x->ignition = 1;
    x->variation = variation;
}

/* Steps until n more cycles are done, giving up after 2 s of simulated time
 * per cycle (a broken engine fails a test instead of hanging it). */
static int cycles(rb_engine *x, long n)
{
    const long target = x->cycles + n, most = lround(2.0 * n / DT_FAST);
    for (long k = 0; k < most && x->cycles < target; k++) rb_engine_step(x);
    return x->cycles >= target;
}

/* Each cylinder's IMEP (its work over the last cycle per swept volume) over
 * n cycles: its mean and COV (the standard deviation over the mean) in mean[]
 * and cov[]; returns the mean over the cylinders. */
static double imep_stats(rb_engine *x, long n, double *mean, double *cov)
{
    const int c = x->desc.cylinders;
    const double vd = rb_engine_displacement(x) / c;
    double s1[RB_MAX_CYLINDERS] = { 0.0 }, s2[RB_MAX_CYLINDERS] = { 0.0 }, all = 0.0;
    for (long k = 0; k < n; k++) {
        if (!cycles(x, 1)) return NAN;
        for (int i = 0; i < c; i++) {
            const double p = x->burn[i].last_work / vd;
            s1[i] += p;
            s2[i] += p * p;
        }
    }
    for (int i = 0; i < c; i++) {
        mean[i] = s1[i] / n;
        cov[i] = sqrt(fmax(s2[i] / n - mean[i] * mean[i], 0.0)) / fabs(mean[i]);
        all += mean[i] / c;
    }
    return all;
}

void test_variation_off_is_exact(void)
{
    /* With variation 0 (the default) every burn starts at its spark and the
     * exhaust valves open exactly as their cams lift them, so a held engine
     * repeats each cycle: what the tests and the dyno numbers before stage
     * 12c assume. A NaN or a negative variation counts as none, and one far
     * too big as 4. */
    held(&e, 0.0);
    CHECK(e.variation == 0.0);
    CHECK(cycles(&e, 30));
    for (int k = 0; k < 2; k++) {
        e.variation = k ? -1.0 : NAN;
        CHECK(cycles(&e, 2));
        int same = 1;
        for (int i = 0; i < 4; i++) same &= e.burn[i].start_angle == e.burn[i].spark_angle;
        CHECK(same);
    }
    e.variation = 1e9;                  /* and with a long burn, so its start */
    e.desc.burn_duration = 170.0;       /* would move far: it stays within 90 */
    int sane = 1;                       /* degrees of TDC, as the spark does  */
    for (int k = 0; k < 8; k++) {
        CHECK(cycles(&e, 1));
        for (int i = 0; i < 4; i++) {
            const double a = fmod(720.0 - e.burn[i].start_angle + 360.0, 720.0) - 360.0;   /* before TDC */
            sane &= isfinite(e.cyl[i].gas.p) && e.burn[i].duration <= 720.0 && fabs(a) <= 90.0;
        }
    }
    CHECK(sane);
    preset_desc("i4", &e.desc);         /* back to the four's own burn */
    e.variation = 0.0;
    CHECK(cycles(&e, 30));
    int moved = 0, flickered = 0;
    double work[RB_MAX_CYLINDERS];
    for (int i = 0; i < 4; i++) work[i] = e.burn[i].last_work;
    for (long k = 0; k < lround(0.08 / DT_FAST); k++) {
        double area[4];   /* as the step will set them, at the crank's angle now */
        for (int i = 0; i < 4; i++)
            area[i] = rb_valve_area(&e.desc.exhaust, rb_valve_lift(&e.desc.exhaust, rb_engine_cylinder_angle(&e, i)));
        rb_engine_step(&e);
        for (int i = 0; i < 4; i++) {
            moved += e.burn[i].start_angle != e.burn[i].spark_angle;
            flickered += e.exhaust[i].area != area[i];
        }
    }
    CHECK_EQ_INT(moved, 0);
    CHECK_EQ_INT(flickered, 0);
    for (int i = 0; i < 4; i++) CHECK_NEAR(e.burn[i].last_work, work[i], 1e-4 * fabs(work[i]));
}

void test_variation_cov_of_imep(void)
{
    /* Real engines' cylinders vary from cycle to cycle by 1-3 % of their
     * work (the COV of IMEP) wide open (Heywood, section 9.4). The four held
     * wide open at 3000 rpm with variation 1 reads about 1.2 %; its mean
     * work is less than 1 % under the unvaried engine's (a burn off its best
     * timing either way loses a little); with variation 2 the COV more than
     * doubles. Each cylinder has its own mean: the headers differ, and so do
     * the cylinders' own mixture and burn. */
    double mean0[RB_MAX_CYLINDERS], cov0[RB_MAX_CYLINDERS], mean[RB_MAX_CYLINDERS], cov[RB_MAX_CYLINDERS];
    double mean2[RB_MAX_CYLINDERS], cov2[RB_MAX_CYLINDERS];
    held(&e, 0.0);
    CHECK(cycles(&e, 20));
    const double all0 = imep_stats(&e, 10, mean0, cov0);
    held(&e, 1.0);
    CHECK(cycles(&e, 20));
    const double all = imep_stats(&e, 100, mean, cov);
    held(&e, 2.0);
    CHECK(cycles(&e, 20));
    imep_stats(&e, 60, mean2, cov2);
    double lo = INFINITY, hi = 0.0;
    for (int i = 0; i < 4; i++) {
        CHECK(cov0[i] < 1e-4);
        CHECK(cov[i] > 0.008 && cov[i] < 0.025);
        CHECK(cov2[i] > 2.0 * cov[i]);
        lo = fmin(lo, mean[i]);
        hi = fmax(hi, mean[i]);
    }
    CHECK(all < all0 && all > 0.985 * all0);
    CHECK(hi - lo > 0.003 * all);
}

void test_variation_cov_at_idle(void)
{
    /* Idling, the burns are slower and more diluted, and the engine computer
     * holds the spark back from its best (a torque reserve), where the work
     * changes faster with the burn's timing: real engines' COV of IMEP is
     * 4-8 % there, about four times wide open's. The four, started with the
     * key and idling with variation 1, reads about 4.3 % (its accessories
     * load it: unloaded it read 7 %), and the idle speed still holds. */
    double mean[RB_MAX_CYLINDERS], cov[RB_MAX_CYLINDERS];
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.hold_speed = 0;
    e.throttle_pos = 0.0;
    e.ignition = 1;
    e.starter = 1;
    e.variation = 1.0;
    for (long k = 0; k < lround(6.0 / DT_FAST); k++) {
        if (e.starter && e.rpm_smooth > 0.75 * d.idle_rpm) e.starter = 0;
        rb_engine_step(&e);
    }
    CHECK(!e.starter);
    double rpm_lo = INFINITY, rpm_hi = 0.0, cov_mean = 0.0;
    const double vd = rb_engine_displacement(&e) / 4;
    double s1[4] = { 0.0 }, s2[4] = { 0.0 };
    const long n = 60;
    for (long k = 0; k < n; k++) {
        CHECK(cycles(&e, 1));
        rpm_lo = fmin(rpm_lo, e.result.rpm);
        rpm_hi = fmax(rpm_hi, e.result.rpm);
        for (int i = 0; i < 4; i++) {
            const double p = e.burn[i].last_work / vd;
            s1[i] += p;
            s2[i] += p * p;
        }
    }
    for (int i = 0; i < 4; i++) {
        mean[i] = s1[i] / n;
        cov[i] = sqrt(fmax(s2[i] / n - mean[i] * mean[i], 0.0)) / fabs(mean[i]);
        cov_mean += cov[i] / 4;
    }
    CHECK(cov_mean > 0.03 && cov_mean < 0.08);
    CHECK(rpm_lo > d.idle_rpm - 40.0 && rpm_hi < d.idle_rpm + 40.0);
}

void test_variation_repeatable(void)
{
    /* The variation comes from a seeded generator: two engines run the same
     * way are the same to the last bit, so renders repeat. And it really
     * varies: one held engine's cycles differ. */
    held(&e, 1.0);
    held(&e2, 1.0);
    for (long k = 0; k < lround(0.2 / DT_FAST); k++) {
        rb_engine_step(&e);
        rb_engine_step(&e2);
    }
    int same = (e.rand_cycles == e2.rand_cycles) && (e.rand_flicker == e2.rand_flicker);
    for (int i = 0; i < 4; i++) {
        same &= e.cyl[i].gas.p == e2.cyl[i].gas.p;
        same &= e.burn[i].duration == e2.burn[i].duration;
        same &= e.burn[i].start_angle == e2.burn[i].start_angle;
    }
    CHECK(same);
    double first = e.burn[0].last_work;
    int differ = 0;
    for (int k = 0; k < 5; k++) {
        CHECK(cycles(&e, 1));
        differ += fabs(e.burn[0].last_work - first) > 1e-3 * fabs(first);
        first = e.burn[0].last_work;
    }
    CHECK(differ >= 4);
}

void test_variation_cylinders_differ(void)
{
    /* Each cylinder has its own mixture and burn length, from two normal
     * numbers drawn at init: 2 % and 4 % (RMS) times the variation, around a
     * mean of none, so the engine as a whole still runs on its target (its
     * oxygen sensor's doing in a real one). Sixteen cylinders in a row make
     * enough of them to see the spread. Right after its intake valve closes,
     * each cylinder's charge has 1 + 0.02 v trim_fuel times the target's fuel. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    CHECK_EQ_INT(rb_engine_layout_inline(&d, 16), 0);
    for (int i = 0; i < 16; i++) d.header_length[i] = 0.45;
    d.collector[0] = 0;
    rb_engine_init(&e, &d, DT_FAST);
    double fuel = 0.0, burn = 0.0, fuel2 = 0.0, burn2 = 0.0;
    for (int i = 0; i < 16; i++) {
        fuel += e.trim_fuel[i];
        burn += e.trim_burn[i];
        fuel2 += e.trim_fuel[i] * e.trim_fuel[i] / 16;
        burn2 += e.trim_burn[i] * e.trim_burn[i] / 16;
    }
    CHECK(fabs(fuel) < 1e-12 && fabs(burn) < 1e-12);
    CHECK(fuel2 > 0.3 && fuel2 < 3.0 && burn2 > 0.3 && burn2 < 3.0);   /* about 1 */

    e.rpm = 3000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    e.variation = 2.0;
    CHECK(cycles(&e, 3));
    const double ivc = rb_valve_closes(&e.desc.intake);
    int seen = 0, wrong = 0;
    double prev[16];
    for (int i = 0; i < 16; i++) prev[i] = rb_engine_cylinder_angle(&e, i);
    for (long k = 0; k < lround(0.05 / DT_FAST) && seen < 16; k++) {
        rb_engine_step(&e);
        for (int i = 0; i < 16; i++) {
            const double a = rb_engine_cylinder_angle(&e, i);
            const double ahead = fmod(a - ivc + 720.0, 720.0);
            if (ahead < fmod(prev[i] - ivc + 720.0, 720.0)) {   /* just passed IVC */
                const rb_chamber *g = &e.cyl[i].gas;
                const double share = g->m[RB_FUEL] * e.afr / g->m[RB_AIR];
                wrong += fabs(share - (1.0 + 2.0 * 0.02 * e.trim_fuel[i])) > 1e-9;
                seen++;
            }
            prev[i] = a;
        }
    }
    CHECK_EQ_INT(seen, 16);
    CHECK_EQ_INT(wrong, 0);

    /* and each burns its own length: over 20 cycles each cylinder's mean
     * burn, against the engine's, follows its own trim (1 + 0.04 v trim) */
    double length[16] = { 0.0 }, all = 0.0;
    for (int k = 0; k < 20; k++) {
        CHECK(cycles(&e, 1));
        for (int i = 0; i < 16; i++) length[i] += e.burn[i].duration / 20;
    }
    for (int i = 0; i < 16; i++) all += length[i] / 16;
    double sxy = 0.0, sxx = 0.0, syy = 0.0;
    for (int i = 0; i < 16; i++) {
        const double x = e.trim_burn[i], y = length[i] / all - 1.0;
        sxy += x * y;
        sxx += x * x;
        syy += y * y;
    }
    CHECK(sxy / sqrt(sxx * syy) > 0.8);              /* they go together */
    CHECK(sxy / sxx > 0.05 && sxy / sxx < 0.11);     /* about 2 x 0.04 */
}

void test_variation_flicker(void)
{
    /* The exhaust valves' jets are turbulent: with variation 1 each one's
     * area flickers about its cam's by 15 % (RMS), low-passed at 2 kHz (a
     * one-pole filter: its correlation falls to 1/e in 1/(2 pi 2000) s). */
    held(&e, 1.0);
    CHECK(cycles(&e, 3));
    const long lag = lround(1.0 / (2.0 * PI * 2000.0) / DT_FAST);
    static double r[96000];
    long n = 0;
    double sum = 0.0, sum2 = 0.0;
    for (long k = 0; k < lround(0.5 / DT_FAST); k++) {
        const double lift = rb_valve_lift(&e.desc.exhaust, rb_engine_cylinder_angle(&e, 0));
        const double area = rb_valve_area(&e.desc.exhaust, lift);   /* at the angle the step sets it */
        rb_engine_step(&e);
        r[k] = (area > 0.0) ? e.exhaust[0].area / area - 1.0 : NAN;
        if (area > 0.0) {
            sum += r[k];
            sum2 += r[k] * r[k];
            n++;
        }
    }
    CHECK(n > 10000);
    const double m = sum / n, rms = sqrt(sum2 / n - m * m);
    CHECK(fabs(m) < 0.02);
    CHECK(rms > 0.12 && rms < 0.18);
    double c = 0.0, c0 = 0.0;
    for (long k = 0; k + lag < lround(0.5 / DT_FAST); k++)
        if (isfinite(r[k]) && isfinite(r[k + lag])) {
            c += r[k] * r[k + lag];
            c0 += r[k] * r[k];
        }
    CHECK(c / c0 > 0.25 && c / c0 < 0.5);   /* about exp(-1) */
}

void test_variation_accessories(void)
{
    /* The accessories take a steady torque below 1500 rpm (an alternator
     * that can't give its full output that slowly, a power steering pump),
     * accessory_power above it, and nothing while cranking: they come in
     * between 300 and 500 rpm. They count as friction. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    CHECK(d.accessory_power > 0.0);
    rb_engine_init(&e, &d, DT_FAST);
    d.accessory_power = 0.0;
    rb_engine_init(&e2, &d, DT_FAST);
    const double P = e.desc.accessory_power, per = 4.0 * PI / rb_engine_displacement(&e);
    static const double rpm[6] = { 0.0, 250.0, 400.0, 1000.0, 1500.0, 3000.0 };
    const double full = P / (1500.0 * PI / 30.0);
    const double expect[6] = { 0.0, 0.0, 0.5 * full, full, full, P / (3000.0 * PI / 30.0) };
    for (int k = 0; k < 6; k++) {
        const double w = rpm[k] * PI / 30.0;
        CHECK_NEAR((rb_engine_fmep(&e, w) - rb_engine_fmep(&e2, w)) / per, expect[k], 1e-9 * P);
    }
    CHECK_EQ_INT(rb_engine_desc_check(&e.desc), 0);
    d.accessory_power = -1.0;
    CHECK_EQ_INT(rb_engine_desc_check(&d), -1);
}

void test_variation_soft_cut(void)
{
    /* The rev limiter: within 150 rpm below the limit it skips single sparks
     * at random, more the nearer the limit; above it, every spark until the
     * crank is back below the limit less 100 rpm. Floored in neutral the four
     * stutters at its limit, firing about a quarter of its sparks, and
     * bounces off the hard cut several times a second, never quiet for 0.1 s.
     * In gear, with a car's inertia on the crank, the soft cut holds it in
     * its band, firing on some of its sparks, and it never stops firing for
     * 0.2 s: before stage 12c it was cut 0.6 s at a time there. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    const double limit = d.rev_limit;

    /* held by a dyno just under the limit, the engine computer is off: no cuts */
    rb_engine_init(&e, &d, DT_FAST);
    e.rpm = limit - 50.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    CHECK(cycles(&e, 10));
    CHECK(e.limiter_cuts == 0);

    for (int gear = 0; gear < 2; gear++) {
        if (gear) d.inertia = 3.0;
        rb_engine_init(&e, &d, DT_FAST);
        e.rpm = 4000.0;
        e.throttle_pos = 1.0;
        e.ignition = 1;
        CHECK(cycles(&e, 5));
        e.hold_speed = 0;
        e.load_torque = gear ? 20.0 : 0.0;
        long k = 0;
        while (e.rpm < limit - 160.0 && k < lround(8.0 / DT_FAST)) {
            rb_engine_step(&e);
            k++;
        }
        CHECK(e.rpm >= limit - 160.0);
        /* one second at the limiter: the hard cut's bounces, the sparks fired
         * and cut, and the longest gap between fired ones */
        long bounces = 0, cut0 = e.limiter_cuts, fired = 0, gap = 0, longest = 0;
        int was = e.spark_cut, burning[4];
        double lo = INFINITY, hi = 0.0;
        for (int i = 0; i < 4; i++) burning[i] = e.burn[i].burning;
        for (k = 0; k < lround(1.0 / DT_FAST); k++) {
            rb_engine_step(&e);
            bounces += e.spark_cut && !was;
            was = e.spark_cut;
            lo = fmin(lo, e.rpm);
            hi = fmax(hi, e.rpm);
            int lit = 0;
            for (int i = 0; i < 4; i++) {
                lit |= e.burn[i].burning && !burning[i];
                burning[i] = e.burn[i].burning;
            }
            fired += lit;
            gap = lit ? 0 : gap + 1;
            if (gap > longest) longest = gap;
        }
        const long cut = e.limiter_cuts - cut0;
        CHECK(fired > 0 && cut > 0);
        CHECK(hi < limit + 100.0 && lo > limit - 300.0);
        if (!gear) {
            CHECK(bounces >= 4 && bounces <= 30);
            CHECK(fired > 0.1 * (cut + fired) && fired < 0.6 * (cut + fired));
            CHECK(longest * DT_FAST < 0.1);
        } else {
            CHECK(longest * DT_FAST < 0.2);
            CHECK(cut > 0.3 * (cut + fired) && cut < 0.97 * (cut + fired));
        }
    }
}

void test_variation_conserves(void)
{
    /* However much each cycle varies, the books still balance: the mass and
     * energy that came in and left, the burning, the walls and the pistons,
     * and with the crank free its kinetic energy, the work done on it, here
     * floored into the rev limiter's soft and hard cuts. */
    held(&e, 2.0);
    double m0, u0, y0[RB_NSPECIES];
    rb_engine_contents(&e, &m0, &u0, y0);
    for (long k = 0; k < lround(0.08 / DT_FAST); k++) rb_engine_step(&e);   /* four cycles */
    double m1, u1, y1[RB_NSPECIES];
    rb_engine_contents(&e, &m1, &u1, y1);
    const double books = e.energy_in - e.energy_out + e.work_on_gas + e.wall_heat + rb_engine_pipe_heat(&e)
                       + e.heat_released + e.fuel_energy_in;
    CHECK(e.heat_released > 0.0);
    CHECK_NEAR(m1 - m0, e.mass_in - e.mass_out + e.fuel_in, 1e-12 * m0);
    CHECK_NEAR(u1 - u0, books, 1e-9 * e.heat_released);

    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.hold_speed = 0;
    e.throttle_pos = 0.0;
    e.ignition = 1;
    e.starter = 1;
    e.variation = 1.0;
    for (long k = 0; k < lround(3.0 / DT_FAST); k++) {
        if (e.starter && e.rpm_smooth > 0.75 * d.idle_rpm) e.starter = 0;
        e.throttle_pos = (k * DT_FAST > 1.0) ? 1.0 : 0.0;
        rb_engine_step(&e);
    }
    CHECK(e.limiter_cuts > 50);
    const double crank = e.crank_gas_work - e.crank_case_work + e.starter_work - e.friction_work
                       - e.load_work + e.stop_energy + e.hold_work;
    CHECK_NEAR(e.kinetic, crank, 1e-9 * fabs(e.crank_gas_work));
}

void test_variation_dyno_in_the_sim(void)
{
    /* rb_cli sound --hold puts stage 9's dyno on the sim's crank (stage
     * 12c): the sim steps it before the engine every physics step, the crank
     * is free, and the dyno holds its speed, each firing moving it a little.
     * A rebuilt engine drops the dyno. */
    rb_sim *s = rb_create(NULL);
    CHECK(s != NULL);
    if (!s) return;
    CHECK_EQ_INT(rb_load_preset(s, rb_preset_find("i4")), RB_OK);
    rb_engine *x = &s->engine;
    s->source = RB_SOURCE_ENGINE;
    x->ignition = 1;
    x->rpm = 3000.0;
    x->throttle_pos = 1.0;
    float buf[256];
    for (int b = 0; b < 300; b++) rb_render_block(s, buf, 256);   /* 1.6 s held, to settle */
    static rb_dyno dyno;
    rb_dyno_init(&dyno, x, 3000.0);
    s->dyno = &dyno;
    double lo = INFINITY, hi = 0.0, sum = 0.0;
    int n = 0;
    for (int b = 0; b < 375; b++) {   /* 2 s */
        rb_render_block(s, buf, 256);
        if (b >= 188) {
            lo = fmin(lo, x->rpm);
            hi = fmax(hi, x->rpm);
            sum += x->rpm;
            n++;
        }
    }
    CHECK(x->hold_speed == 0);
    CHECK(dyno.readings > 10);
    CHECK_NEAR(sum / n, 3000.0, 30.0);
    CHECK(hi - lo > 3.0 && hi - lo < 150.0);
    CHECK_EQ_INT(rb_load_preset(s, rb_preset_find("v8-cross")), RB_OK);
    CHECK(s->dyno == NULL);
    rb_destroy(s);
}

/* A lean charge's flame cannot finish before the exhaust opens. The old
 * duration ceiling made it complete anyway. No extra misfire probability:
 * the charge model sets the pace and EVO leaves fuel/air in the gas. */
void test_variation_partial_burns(void)
{
    for (int life = 0; life < 2; life++) {
        held(&e, (double)life);
        e.afr = 26.0;
        CHECK(cycles(&e, 25));
        const double evo = rb_valve_opens(&e.desc.exhaust);
        int seen = 0;
        for (long k = 0; k < lround(0.4 / DT_FAST) && seen < 6; k++) {
            const double previous = rb_engine_cylinder_angle(&e, 0);
            rb_engine_step(&e);
            const double angle = rb_engine_cylinder_angle(&e, 0);
            if (!(previous < evo && angle >= evo)) continue;
            const rb_burn *b = &e.burn[0];
            CHECK(!b->burning && !b->after_spark);
            CHECK(b->demand_duration > 200.0 && b->demand_duration <= 720.0);
            if (life) {
                CHECK(b->duration > 180.0 && b->duration <= 720.0);
                CHECK(b->fraction > 0.1 && b->fraction < 0.85);
                CHECK(e.cyl[0].gas.m[RB_FUEL] > 0.1 * b->burnable);
                const double available = fmod(evo - b->start_angle + 720.0, 720.0);
                CHECK_NEAR(b->fraction, rb_wiebe(available, b->duration,
                           e.desc.wiebe_a, e.desc.wiebe_m), 0.002);
            } else {
                CHECK(b->duration <= 180.0);
                CHECK(b->fraction == 1.0);
            }
            seen++;
        }
        CHECK_EQ_INT(seen, 6);
    }
}

void test_variation_partial_burn_books(void)
{
    held(&e, 1.0);
    e.afr = 26.0;
    double m0, u0, y0[RB_NSPECIES], m1, u1, y1[RB_NSPECIES];
    rb_engine_contents(&e, &m0, &u0, y0);
    CHECK(cycles(&e, 30));
    rb_engine_contents(&e, &m1, &u1, y1);
    const double books = e.energy_in - e.energy_out + e.work_on_gas + e.wall_heat
                       + rb_engine_pipe_heat(&e) + e.heat_released + e.fuel_energy_in;
    CHECK(e.heat_released > 0.0 && e.species_out[RB_FUEL] > 0.0);
    CHECK_NEAR(m1 - m0, e.mass_in - e.mass_out + e.fuel_in, 1e-10 * m0);
    CHECK_NEAR(u1 - u0, books, 1e-9 * e.heat_released);
    CHECK_NEAR(y1[RB_FUEL] - y0[RB_FUEL], e.species_in[RB_FUEL] - e.species_out[RB_FUEL]
               + e.fuel_in - e.fuel_burned, 1e-10 * e.fuel_in);
    CHECK_NEAR(y1[RB_AIR] - y0[RB_AIR], e.species_in[RB_AIR] - e.species_out[RB_AIR]
               - RB_AFR_STOICH * e.fuel_burned, 1e-10 * e.fuel_in);
    CHECK_NEAR(y1[RB_BURNED] - y0[RB_BURNED], e.species_in[RB_BURNED] - e.species_out[RB_BURNED]
               + (1.0 + RB_AFR_STOICH) * e.fuel_burned, 1e-10 * e.fuel_in);
}

void test_variation_partial_burn_residual(void)
{
    /* Isolate dilution from the temperature, pressure, speed and mixture
     * changes that accompany it in a running engine. At cylinder 1's next
     * IVC install either charge at the same p/T and stoichiometric AFR;
     * the existing flame-speed law must leave less of the dilute charge
     * burned at EVO. These are test charges, not misfire cutoffs. */
    const double residual[] = { 0.04, 0.44 };
    double demand[2], duration[2], fraction[2], start[2];
    for (int case_no = 0; case_no < 2; case_no++) {
        held(&e, 1.0);
        e.afr = RB_AFR_STOICH;
        e.trim_fuel[0] = 0.0;   /* same actual AFR; no extra fuel at IVC */
        e.spark_map = 0;
        e.spark = 25.0;          /* same available crank angle in both cases */
        const double ivc = rb_valve_closes(&e.desc.intake);
        const double step = 6.0 * e.rpm * DT_FAST;
        long k = 0;
        while (!(e.theta <= ivc && e.theta + step > ivc) && k < 5000) {
            rb_engine_step(&e);
            k++;
        }
        CHECK(k < 5000);
        if (k >= 5000) return;
        const double fresh = 1.0 - residual[case_no];
        const double y[RB_NSPECIES] = {
            fresh * e.afr / (e.afr + 1.0), fresh / (e.afr + 1.0), residual[case_no]
        };
        rb_chamber_init(&e.cyl[0].gas, e.cyl[0].gas.V, 1.22e5, 360.0, y);
        rb_engine_step(&e);      /* IVC captures the controlled charge first */
        CHECK_NEAR(e.burn[0].T_ivc, 360.0, 1e-8);
        CHECK_NEAR(e.burn[0].p_ivc, 1.22e5, 1e-6);
        CHECK_NEAR(e.burn[0].residual, residual[case_no], 1e-12);
        demand[case_no] = e.burn[0].demand_duration;
        duration[case_no] = e.burn[0].duration;
        /* Start jitter scales with the expected burn length. Pin the
         * already-generated start to the fixed spark to isolate residence
         * time as well as charge p/T/AFR and crank speed. */
        e.burn[0].start_angle = e.burn[0].spark_angle;
        start[case_no] = e.burn[0].start_angle;
        const double evo = rb_valve_opens(&e.desc.exhaust);
        int seen = 0;
        for (k = 0; k < 5000; k++) {
            const double previous = rb_engine_cylinder_angle(&e, 0);
            rb_engine_step(&e);
            const double angle = rb_engine_cylinder_angle(&e, 0);
            if (!(previous < evo && angle >= evo)) continue;
            CHECK(!e.burn[0].burning && !e.burn[0].after_spark);
            fraction[case_no] = e.burn[0].fraction;
            seen = 1;
            break;
        }
        CHECK(seen);
        if (!seen) return;
    }
    CHECK_NEAR(start[0], start[1], 1e-12);
    CHECK(demand[1] > 3.0 * demand[0]);
    CHECK(duration[1] > 3.0 * duration[0]);
    CHECK(fraction[0] == 1.0);
    CHECK(fraction[1] > 0.1 && fraction[1] < 0.85);
}
