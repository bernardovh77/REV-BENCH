/*
 * test_limiter.c - stage 133: the rev limiter's character and the 2-step. Bernardo asked to change
 * how the revs bounce at the limit, slower or faster, and for a 2-step (2026-11-22).
 *
 * Each drive takes the four at the low quality wide open in neutral (held at 1500 rpm, then
 * free), lets it reach its limit, and reads the crank's speed every millisecond for two seconds:
 * the bounce's depth (the highest speed less the lowest) and its rate (the times a second the
 * speed comes back up through the middle of its range).
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "rb.h"
#include "rb_test.h"

#define RATE  48000
#define MS    48           /* frames a millisecond */
#define REACH 2.0          /* s to reach the limit */
#define READ_MS 2000       /* ms read there        */
#define READ  (READ_MS / 1000.0)

typedef struct bounce {
    double lo, hi, mean;   /* rpm */
    double rate;           /* bounces a second */
    double cuts;           /* the limiter's cuts while read */
} bounce;

/* The four at the low quality, the field settings given (field -1 ends the list), the 2-step
 * armed or not, revved wide open in neutral to its limit and read there. */
static bounce rev_to_the_limit(const int *fields, const double *values, int launch)
{
    bounce b = { 1e9, 0.0, 0.0, 0.0, 0.0 };
    rb_sim *sim = rb_create_with_quality(RATE, RB_QUALITY_LOW);
    CHECK(sim != NULL);
    if (!sim) return b;
    rb_load_preset(sim, rb_preset_find("i4"));
    for (int k = 0; fields[k] >= 0; k++) CHECK(rb_set_engine(sim, fields[k], 0, values[k]) == RB_OK);
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 1500.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1);
    rb_set_param(sim, RB_PARAM_LAUNCH, launch);
    static float out[MS];
    for (int i = 0; i < RATE / 10; i += MS) rb_render_block(sim, out, MS);
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    for (int i = 0; i < (int)(REACH * 1000); i++) rb_render_block(sim, out, MS);
    enum { N = READ_MS };
    static double rpm[N];
    const double cuts0 = rb_get_telemetry(sim, RB_TLM_LIMITER_CUTS);
    for (int i = 0; i < N; i++) {
        rb_render_block(sim, out, MS);
        rpm[i] = rb_get_telemetry(sim, RB_TLM_RPM);
        b.lo = fmin(b.lo, rpm[i]);
        b.hi = fmax(b.hi, rpm[i]);
        b.mean += rpm[i] / N;
    }
    b.cuts = rb_get_telemetry(sim, RB_TLM_LIMITER_CUTS) - cuts0;
    const double mid = 0.5 * (b.lo + b.hi);
    int ups = 0;
    for (int i = 1; i < N; i++) ups += rpm[i - 1] < mid && rpm[i] >= mid;
    b.rate = ups / READ;
    rb_destroy(sim);
    return b;
}

static const int NONE[] = { -1 };

void test_limiter_defaults_are_the_old_limiter(void)
{
    /* With nothing set the limiter is stage 12c's, so renders stay as they were (cli_parity):
     * the hard cut 100 rpm deep, a 150 rpm soft band, the sparks cut; the 2-step at 4000, hard. */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_sim *sim = rb_create_with_quality(RATE, RB_QUALITY_LOW);
        CHECK(sim != NULL);
        if (!sim) return;
        rb_load_preset(sim, p);
        CHECK(rb_get_engine(sim, RB_ENG_LIMITER_GAP, 0) == 100.0);
        CHECK(rb_get_engine(sim, RB_ENG_LIMITER_BAND, 0) == 150.0);
        CHECK(rb_get_engine(sim, RB_ENG_LIMITER_CUT, 0) == RB_LIMITER_SPARK);
        CHECK(rb_get_engine(sim, RB_ENG_LAUNCH_RPM, 0) == 4000.0);
        CHECK(rb_get_engine(sim, RB_ENG_LAUNCH_BAND, 0) == 0.0);
        CHECK(rb_get_param(sim, RB_PARAM_LAUNCH) == 0.0);
        /* the gap follows the rev limit, and sets where the spark comes back */
        const double limit = rb_get_engine(sim, RB_ENG_REV_LIMIT, 0);
        CHECK(rb_set_engine(sim, RB_ENG_REV_LIMIT, 0, limit - 500.0) == RB_OK);
        CHECK(rb_get_engine(sim, RB_ENG_LIMITER_GAP, 0) == 100.0);
        CHECK(rb_set_engine(sim, RB_ENG_LIMITER_GAP, 0, 400.0) == RB_OK);
        CHECK(rb_get_engine(sim, RB_ENG_LIMITER_GAP, 0) == 400.0);
        CHECK(rb_set_engine(sim, RB_ENG_LIMITER_GAP, 0, 5000.0) == RB_CLAMPED);   /* range */
        CHECK(rb_get_engine(sim, RB_ENG_LIMITER_GAP, 0) == 1500.0);
        CHECK(rb_set_engine(sim, RB_ENG_LIMITER_CUT, 0, 2.0) == RB_CLAMPED);
        rb_destroy(sim);
    }
}

void test_limiter_gap_sets_the_bounce(void)
{
    /* The hard cut alone (no soft band): a wider gap bounces deeper and slower, a narrow one
     * fast and shallow, and the limit holds in every one. */
    const int fields[] = { RB_ENG_LIMITER_BAND, RB_ENG_LIMITER_GAP, -1 };
    const double gaps[] = { 50.0, 200.0, 600.0 };
    bounce b[3];
    for (int k = 0; k < 3; k++) {
        const double values[] = { 0.0, gaps[k] };
        b[k] = rev_to_the_limit(fields, values, 0);
        printf("        gap %4.0f rpm: %4.0f-%4.0f rpm, %.1f bounces a second\n", gaps[k], b[k].lo, b[k].hi, b[k].rate);
        CHECK(b[k].hi < 6800.0 + 300.0);   /* the limit holds */
        CHECK(b[k].cuts > 0.0);
    }
    for (int k = 0; k < 2; k++) {
        CHECK(b[k + 1].hi - b[k + 1].lo > b[k].hi - b[k].lo);   /* deeper */
        CHECK(b[k + 1].rate < b[k].rate);                       /* slower */
    }
    CHECK(b[2].hi - b[2].lo > 500.0);   /* about the gap deep */
}

void test_limiter_band_softens_the_cut(void)
{
    /* A soft band starts cutting below the limit: the engine runs lower on average, and with
     * the widest band it never touches the hard cut's limit. */
    const int fields[] = { RB_ENG_LIMITER_BAND, -1 };
    const double hard[] = { 0.0 }, soft[] = { 400.0 };
    const bounce h = rev_to_the_limit(fields, hard, 0), s = rev_to_the_limit(fields, soft, 0);
    printf("        hard: mean %.0f rpm, top %.0f; band 400: mean %.0f rpm, top %.0f\n", h.mean, h.hi, s.mean, s.hi);
    CHECK(s.mean < h.mean - 50.0);
    CHECK(s.hi < h.hi);
}

void test_limiter_fuel_cut_holds_the_limit(void)
{
    /* Cutting the fuel instead of the sparks still holds the limit, a cycle later (a cylinder
     * already fuelled fires), and counts its cuts. It sounds different from the spark cut. */
    const int fields[] = { RB_ENG_LIMITER_CUT, -1 };
    const double fuel[] = { RB_LIMITER_FUEL };
    const bounce f = rev_to_the_limit(fields, fuel, 0), s = rev_to_the_limit(NONE, NULL, 0);
    printf("        spark cut: %.0f-%.0f rpm, %.0f cuts; fuel cut: %.0f-%.0f rpm, %.0f cuts\n", s.lo, s.hi, s.cuts, f.lo, f.hi, f.cuts);
    CHECK(f.cuts > 0.0);
    CHECK(f.hi < 6800.0 + 500.0);
    CHECK(f.lo > 6800.0 - 1000.0);
    CHECK(fabs(f.mean - s.mean) > 1.0 || fabs(f.hi - s.hi) > 1.0);
}

void test_limiter_launch_holds_its_own_limit(void)
{
    /* The 2-step armed holds the engine at its limit (4000 rpm by default) with its own gap;
     * a 2-step above the rev limit leaves the rev limiter in charge. */
    const bounce l = rev_to_the_limit(NONE, NULL, 1);
    printf("        2-step at 4000: %.0f-%.0f rpm, %.1f bounces a second\n", l.lo, l.hi, l.rate);
    CHECK(l.hi < 4000.0 + 300.0);
    CHECK(l.lo > 4000.0 - 600.0);
    CHECK(l.cuts > 0.0);
    const int fields[] = { RB_ENG_LAUNCH_RPM, RB_ENG_LAUNCH_GAP, -1 };
    const double values[] = { 3000.0, 400.0 };
    const bounce m = rev_to_the_limit(fields, values, 1);
    CHECK(m.hi < 3000.0 + 300.0);
    CHECK(m.hi - m.lo > l.hi - l.lo);   /* its own gap: deeper */
    const double above[] = { 9000.0, 100.0 };
    const bounce a = rev_to_the_limit(fields, above, 1);
    CHECK(a.hi > 6500.0 && a.hi < 6800.0 + 300.0);   /* the rev limiter's limit */
}

void test_limiter_launch_lets_go(void)
{
    /* Let go of the 2-step and the engine revs on to the rev limiter. */
    rb_sim *sim = rb_create_with_quality(RATE, RB_QUALITY_LOW);
    CHECK(sim != NULL);
    if (!sim) return;
    rb_load_preset(sim, rb_preset_find("i4"));
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 1500.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1);
    rb_set_param(sim, RB_PARAM_LAUNCH, 1);
    static float out[480];
    for (int i = 0; i < 10; i++) rb_render_block(sim, out, 480);
    rb_set_param(sim, RB_PARAM_HOLD, 0);
    double top = 0.0;
    for (int i = 0; i < 200; i++) {
        rb_render_block(sim, out, 480);
        top = fmax(top, rb_get_telemetry(sim, RB_TLM_RPM));
    }
    CHECK(top < 4300.0);
    rb_set_param(sim, RB_PARAM_LAUNCH, 0);
    for (int i = 0; i < 200; i++) {
        rb_render_block(sim, out, 480);
        top = fmax(top, rb_get_telemetry(sim, RB_TLM_RPM));
    }
    CHECK(top > 6000.0);
    rb_destroy(sim);
}
