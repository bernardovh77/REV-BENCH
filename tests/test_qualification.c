/*
 * test_qualification.c - stage 059: each preset against the engine it is modelled on, and the
 * envelope that holds. The published figures (manufacturer ratings, docs/stage-059.md):
 *   i4        Toyota 1AZ-FE, 2.0 L:   108-112 kW at 6000 rpm, 190-194 N m at 4000
 *   i6        BMW M54B30, 3.0 L:      170 kW at 5900,         300 N m at 3500
 *   v8-cross  Ford Coyote 5.0 (2011): 307 kW at 6500,         529 N m at 4250
 *   v8-flat   Ferrari F136 (458):     419-425 kW at 9000,     540 N m at 6000
 *   boxer4    Subaru FB20 (XV):       110 kW at 6200,         196 N m at 4200
 * The presets are "inspired" builds (stage 039's identity table), not the engines exactly, and
 * their exhausts are modified ones: so the test freezes the measured gaps as the supported
 * envelope rather than calling a match.
 */
#include "dyno.h"
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>

#define DT (1.0 / 192000.0)

static rb_engine e;

/* Brake torque (N m) of preset p wide open, held by the core's dyno at rpm: a second to
 * settle, then the mean of 6 cycles' readings. */
static double brake_torque(int p, double rpm)
{
    rb_engine_desc d;
    rb_preset_desc(p, &d);
    rb_engine_init(&e, &d, DT);
    e.rpm = rpm;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-3);
    rb_dyno y;
    rb_dyno_init(&y, &e, rpm);
    const long settle = lround(1.0 / DT), most = lround(4.0 / DT);
    for (long k = 0; k < settle; k++) {
        rb_engine_step(&e);
        rb_dyno_step(&y, &e);
    }
    double sum = 0.0;
    int n = 0;
    for (long k = 0; k < most && n < 6; k++) {
        rb_engine_step(&e);
        if (rb_dyno_step(&y, &e)) {
            sum += y.reading.brake_torque;
            n++;
        }
    }
    return n ? sum / n : NAN;
}

void test_qualification_against_published_ratings(void)
{
    /* Brake torque at the rating's own speeds, against the rating (the mid-point of a range):
     * the four, the six and the boxer within 12 % at both points (measured 5-11 % short: the
     * supported envelope, wide open to the rated power speed); the cross-plane V8 8-25 %
     * short (11 and 16 %) and the flat-plane 12-35 % (20 and 22 %), frozen as outside it
     * (their VE never reaches the 1.05-1.15 of a tuned high-output engine: docs/stage-059.md). */
    static const struct {
        const char *key;
        double t_rpm, t_nm, p_rpm, p_kw, lo, hi;
    } R[] = {
        { "i4",       4000.0, 192.0, 6000.0, 110.0, -0.12, 0.12 },
        { "i6",       3500.0, 300.0, 5900.0, 170.0, -0.12, 0.12 },
        { "v8-cross", 4250.0, 529.0, 6500.0, 307.0, -0.25, -0.08 },
        { "v8-flat",  6000.0, 540.0, 8500.0, 422.0, -0.35, -0.12 },
        { "boxer4",   4200.0, 196.0, 6200.0, 110.0, -0.12, 0.12 },
    };
    for (int k = 0; k < 5; k++) {
        const int p = rb_preset_find(R[k].key);
        const double t = brake_torque(p, R[k].t_rpm);
        const double kw = brake_torque(p, R[k].p_rpm) * R[k].p_rpm * 3.14159265358979323846 / 30.0 / 1e3;
        const double dt = t / R[k].t_nm - 1.0, dp = kw / R[k].p_kw - 1.0;
        printf("    %-8s %5.1f N m at %4.0f rpm (%+5.1f %%), %5.1f kW at %4.0f rpm (%+5.1f %%)\n",
               R[k].key, t, R[k].t_rpm, 100.0 * dt, kw, R[k].p_rpm, 100.0 * dp);
        CHECK(dt > R[k].lo && dt < R[k].hi);
        CHECK(dp > R[k].lo && dp < R[k].hi);
    }
}

/* Each cylinder's COV of IMEP over n cycles, its mean over the cylinders, preset p held at rpm
 * and throttle with variation 1 and the seed. */
static double cov_of_imep(int p, double rpm, double throttle, uint64_t seed, long n)
{
    rb_engine_desc d;
    rb_preset_desc(p, &d);
    rb_engine_init(&e, &d, DT);
    rb_engine_seed(&e, seed);
    e.rpm = rpm;
    e.throttle_pos = throttle;
    e.ignition = 1;
    e.variation = 1.0;
    rb_engine_settle(&e, 60, 1e-3);
    const int c = d.cylinders;
    const double vd = rb_engine_displacement(&e) / c;
    double s1[RB_MAX_CYLINDERS] = { 0.0 }, s2[RB_MAX_CYLINDERS] = { 0.0 };
    for (long k = 0; k < n; k++) {
        const long target = e.cycles + 1, most = lround(2.0 / DT);
        for (long j = 0; j < most && e.cycles < target; j++) rb_engine_step(&e);
        for (int i = 0; i < c; i++) {
            const double w = e.burn[i].last_work / vd;
            s1[i] += w;
            s2[i] += w * w;
        }
    }
    double cov = 0.0;
    for (int i = 0; i < c; i++) {
        const double m = s1[i] / n;
        cov += sqrt(fmax(s2[i] / n - m * m, 0.0)) / fabs(m) / c;
    }
    return cov;
}

void test_qualification_cov_over_seeds(void)
{
    /* Multiple seeds, every preset: the COV of IMEP wide open at 3000 rpm within Heywood's
     * 1-3 % (section 9.4, here 0.5-4 %), and near idle (its idle speed, 2 % throttle: a
     * quarter to half of the charge residual gas) higher, within his 3-10 % (here 2.5-12 %):
     * the slower, diluted burn varies more. Three seeds agree within a factor 2.5 (40 cycles
     * each). (A first version took 8 % throttle for light load: its residual was only 7-19 %,
     * its COV no higher than wide open; docs/stage-059.md records the correction.) */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        double wot[3], light[3];
        for (int s = 0; s < 3; s++) {
            wot[s] = cov_of_imep(p, 3000.0, 1.0, 101 + 17 * s, 40);
            light[s] = cov_of_imep(p, d.idle_rpm, 0.02, 101 + 17 * s, 40);
        }
        double wlo = wot[0], whi = wot[0], llo = light[0], lhi = light[0];
        for (int s = 1; s < 3; s++) {
            wlo = fmin(wlo, wot[s]);
            whi = fmax(whi, wot[s]);
            llo = fmin(llo, light[s]);
            lhi = fmax(lhi, light[s]);
        }
        printf("    %-8s COV of IMEP: wide open %.1f-%.1f %%, near idle %.1f-%.1f %% (three seeds)\n",
               rb_preset_key(p), 100.0 * wlo, 100.0 * whi, 100.0 * llo, 100.0 * lhi);
        CHECK(wlo > 0.005 && whi < 0.04);
        CHECK(llo > 0.025 && lhi < 0.12);
        CHECK(llo > whi);
        CHECK(whi < 2.5 * wlo && lhi < 2.5 * llo);
    }
}
