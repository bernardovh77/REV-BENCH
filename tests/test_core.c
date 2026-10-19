/*
 * test_core.c - tests for the simulation core's plumbing (stage 1).
 */
#include "rb.h"
#include "rb_test.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Renders `frames` samples into out[] in blocks of `block`. */
static void render(rb_sim *s, float *out, int frames, int block)
{
    for (int done = 0; done < frames; ) {
        int n = frames - done < block ? frames - done : block;
        rb_render_block(s, out + done, n);
        done += n;
    }
}

/* A default sim (48 kHz, oversample 4) playing a test tone. */
static rb_sim *make_tone(double hz, double level)
{
    rb_sim *s = rb_create(NULL);
    if (!s) return NULL;
    rb_set_param(s, RB_PARAM_TEST_TONE_HZ, hz);
    rb_set_param(s, RB_PARAM_TEST_TONE_LEVEL, level);
    rb_set_param(s, RB_PARAM_MASTER_GAIN, 1.0);
    return s;
}

void test_core_config(void)
{
    rb_config c, bad;
    rb_sim *s;

    rb_config_default(&c);
    CHECK_NEAR(c.sample_rate, 48000.0, 0.0);
    CHECK_EQ_INT(c.oversample, 4);

    s = rb_create(&c);
    CHECK(s != NULL);
    if (s) {
        CHECK_NEAR(rb_physics_dt(s), 1.0 / 192000.0, 1e-18);
        CHECK_NEAR(rb_sample_rate(s), 48000.0, 0.0);
        CHECK_EQ_INT(rb_oversample(s), 4);
        CHECK_NEAR(rb_get_telemetry(s, RB_TLM_TIME_S), 0.0, 0.0);
        rb_destroy(s);
    }

    s = rb_create(NULL);                  /* NULL config means defaults */
    CHECK(s != NULL);
    rb_destroy(s);
    rb_destroy(NULL);                     /* must be harmless */

    bad = c; bad.sample_rate = 0.0;  CHECK(rb_create(&bad) == NULL);
    bad = c; bad.sample_rate = NAN;  CHECK(rb_create(&bad) == NULL);
    bad = c; bad.sample_rate = 1e7;  CHECK(rb_create(&bad) == NULL);
    bad = c; bad.oversample  = 0;    CHECK(rb_create(&bad) == NULL);
    bad = c; bad.oversample  = 1000; CHECK(rb_create(&bad) == NULL);

    CHECK(rb_sizeof() > 0);
    CHECK_EQ_INT(rb_init(NULL, &c), RB_ERR_ARG);
    CHECK(rb_version() != NULL && rb_version()[0] != '\0');
}

void test_core_param_table(void)
{
    CHECK_EQ_INT(rb_param_count(), RB_PARAM_COUNT);
    for (int i = 0; i < rb_param_count(); i++) {
        const char *name = rb_param_name(i);
        CHECK(name != NULL && name[0] != '\0');
        CHECK(rb_param_unit(i) != NULL);
        CHECK(rb_param_min(i) <= rb_param_default(i));
        CHECK(rb_param_default(i) <= rb_param_max(i));
        CHECK(rb_param_flags(i) != 0);
        if (name)
            CHECK_EQ_INT(rb_param_find(name), i);   /* also proves names are unique */
    }
    CHECK(rb_param_name(-1) == NULL);
    CHECK(rb_param_name(RB_PARAM_COUNT) == NULL);
    CHECK(isnan(rb_param_min(RB_PARAM_COUNT)));
    CHECK_EQ_INT(rb_param_find("no_such_param"), RB_ERR_ID);
    CHECK_EQ_INT(rb_param_find(NULL), RB_ERR_ARG);

    CHECK_EQ_INT(rb_telemetry_count(), RB_TLM_COUNT);
    for (int i = 0; i < rb_telemetry_count(); i++) {
        const char *name = rb_telemetry_name(i);
        CHECK(name != NULL && name[0] != '\0');
    }
    CHECK(rb_telemetry_name(RB_TLM_COUNT) == NULL);
}

void test_core_param_set_get(void)
{
    rb_sim *s = rb_create(NULL);
    const int hz = RB_PARAM_TEST_TONE_HZ;

    CHECK(s != NULL);
    if (!s) return;

    CHECK_NEAR(rb_get_param(s, hz), rb_param_default(hz), 0.0);
    CHECK_EQ_INT(rb_set_param(s, hz, 440.0), RB_OK);
    CHECK_NEAR(rb_get_param(s, hz), 440.0, 0.0);

    CHECK_EQ_INT(rb_set_param(s, hz, 1e9), RB_CLAMPED);
    CHECK_NEAR(rb_get_param(s, hz), rb_param_max(hz), 0.0);
    CHECK_EQ_INT(rb_set_param(s, hz, -5.0), RB_CLAMPED);
    CHECK_NEAR(rb_get_param(s, hz), rb_param_min(hz), 0.0);

    /* NaN/inf are rejected and leave the old value untouched. */
    CHECK_EQ_INT(rb_set_param(s, hz, 300.0), RB_OK);
    CHECK_EQ_INT(rb_set_param(s, hz, NAN), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_set_param(s, hz, INFINITY), RB_ERR_VALUE);
    CHECK_NEAR(rb_get_param(s, hz), 300.0, 0.0);

    CHECK_EQ_INT(rb_set_param(s, -1, 1.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_set_param(s, RB_PARAM_COUNT, 1.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_set_param(NULL, hz, 1.0), RB_ERR_ARG);
    CHECK(isnan(rb_get_param(s, 999)));
    CHECK(isnan(rb_get_param(NULL, hz)));

    rb_destroy(s);
}

void test_core_tone_frequency_and_level(void)
{
    /* A 440 Hz tone at 0.7, below the soft limiter's knee: it comes through
     * the anti-alias filter and the DC blocker at its level (the blocker
     * takes 0.03 % at 440 Hz). At full level the limiter bends its peaks
     * (stage 11): they come out at 0.952. */
    enum { FS = 48000 };
    rb_sim *s = make_tone(440.0, 0.7);
    float *x = malloc(FS * sizeof *x);
    int crossings = 0;
    double peak = 0.0, sumsq = 0.0;

    CHECK(s != NULL && x != NULL);
    if (!s || !x) {
        rb_destroy(s);
        free(x);
        return;
    }

    render(s, x, FS / 4, 128);    /* 0.25 s: let the fade-in settle */
    render(s, x, FS, 128);        /* then exactly one second        */

    for (int i = 1; i < FS; i++)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f) crossings++;
    for (int i = 0; i < FS; i++) {
        if (fabs(x[i]) > peak) peak = fabs(x[i]);
        sumsq += (double)x[i] * x[i];
    }

    CHECK(crossings >= 439 && crossings <= 441);       /* 440 Hz */
    CHECK_NEAR(peak, 0.7, 2e-3);
    CHECK_NEAR(sqrt(sumsq / FS), 0.7 / sqrt(2.0), 2e-3);
    CHECK_NEAR(rb_get_telemetry(s, RB_TLM_TIME_S), 1.25, 1e-9);
    CHECK_NEAR(rb_get_telemetry(s, RB_TLM_STEPS), 1.25 * FS * 4, 0.5);
    CHECK_NEAR(rb_get_telemetry(s, RB_TLM_NONFINITE), 0.0, 0.0);
    CHECK_NEAR(rb_get_telemetry(s, RB_TLM_LIMITED), 0.0, 0.0);

    rb_set_param(s, RB_PARAM_TEST_TONE_LEVEL, 1.0);
    render(s, x, FS / 4, 128);
    render(s, x, FS / 10, 128);
    peak = 0.0;
    for (int i = 0; i < FS / 10; i++)
        if (fabs(x[i]) > peak) peak = fabs(x[i]);
    CHECK_NEAR(peak, 0.8 + 0.2 * tanh(1.0), 2e-3);
    CHECK(rb_get_telemetry(s, RB_TLM_LIMITED) > 0.0);

    rb_destroy(s);
    free(x);
}

void test_core_no_clicks_on_frequency_change(void)
{
    enum { N = 48000, CHUNK = 37 };
    rb_sim *s = make_tone(100.0, 1.0);
    float *x = malloc(N * sizeof *x);
    double max_step = 0.0;
    /* A unit sine at f Hz changes by at most 2*pi*f/fs between samples.
     * A phase jump would show up as a much bigger step (up to 2). The
     * anti-alias filter (stage 11) rounds the corners where the frequency
     * switches, overshooting the slope by 5 %. */
    const double limit = 2.0 * 3.14159265358979 * 2000.0 / 48000.0;

    CHECK(s != NULL && x != NULL);
    if (!s || !x) {
        rb_destroy(s);
        free(x);
        return;
    }

    render(s, x, N / 4, 128);     /* settle */
    for (int done = 0, k = 0; done < N; done += CHUNK, k++) {
        int n = N - done < CHUNK ? N - done : CHUNK;
        rb_set_param(s, RB_PARAM_TEST_TONE_HZ, (k & 1) ? 2000.0 : 100.0);
        rb_render_block(s, x + done, n);
    }
    for (int i = 1; i < N; i++) {
        double d = fabs((double)x[i] - (double)x[i - 1]);
        if (d > max_step) max_step = d;
    }
    CHECK(max_step <= limit * 1.08);

    rb_destroy(s);
    free(x);
}

void test_core_level_smoothing(void)
{
    /* A sudden jump in the tone's level, to 0.7, fades in over 10 ms. */
    rb_sim *s = make_tone(1000.0, 0.0);
    float x[480];
    double peak;

    CHECK(s != NULL);
    if (!s) return;

    render(s, x, 480, 128);                                   /* silence */
    CHECK_NEAR(rb_get_telemetry(s, RB_TLM_OUT_PEAK), 0.0, 0.0);

    rb_set_param(s, RB_PARAM_TEST_TONE_LEVEL, 0.7);           /* sudden jump */
    render(s, x, 48, 48);                                     /* first 1 ms  */
    peak = rb_get_telemetry(s, RB_TLM_OUT_PEAK);
    /* tau = 10 ms, so after 1 ms the level is only ~1 - e^-0.1 = 9.5 % of
     * the way there; the anti-alias filter delays it 0.4 ms besides. */
    CHECK(peak > 0.01 && peak < 0.11 * 0.7);

    for (int i = 0; i < 20; i++) render(s, x, 480, 480);      /* +200 ms */
    CHECK_NEAR(rb_get_telemetry(s, RB_TLM_OUT_PEAK), 0.7, 5e-3);

    rb_destroy(s);
}

void test_core_block_size_independent_and_reset(void)
{
    enum { N = 10000 };
    rb_sim *a = make_tone(330.0, 0.7);
    rb_sim *b = make_tone(330.0, 0.7);
    float *xa = malloc(N * sizeof *xa);
    float *xb = malloc(N * sizeof *xb);
    float *xc = malloc(N * sizeof *xc);

    CHECK(a && b && xa && xb && xc);
    if (a && b && xa && xb && xc) {
        /* The block size the host uses must not change the sound. */
        render(a, xa, N, 128);
        render(b, xb, N, 100);
        CHECK(memcmp(xa, xb, N * sizeof *xa) == 0);

        /* Reset restarts the model but keeps the parameters. */
        rb_reset(a);
        CHECK_NEAR(rb_get_telemetry(a, RB_TLM_TIME_S), 0.0, 0.0);
        CHECK_NEAR(rb_get_param(a, RB_PARAM_TEST_TONE_HZ), 330.0, 0.0);
        render(a, xc, N, 7);
        CHECK(memcmp(xa, xc, N * sizeof *xa) == 0);
    }
    rb_destroy(a);
    rb_destroy(b);
    free(xa);
    free(xb);
    free(xc);
}

/* Deterministic pseudo-random numbers in [0, 1) (xorshift64*). */
static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static double rnd01(void)
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return (double)((rng_state * 0x2545F4914F6CDD1Dull) >> 11) * (1.0 / 9007199254740992.0);
}

void test_core_fuzz_output_bounded(void)
{
    static const double rates[] = { 22050.0, 44100.0, 48000.0, 96000.0 };
    float x[512];
    int bad = 0;

    for (int it = 0; it < 200; it++) {
        rb_config c;
        c.sample_rate = rates[(int)(rnd01() * 4.0) & 3];
        c.oversample  = 1 + (int)(rnd01() * 8.0);
        rb_sim *s = rb_create(&c);
        if (!s) {
            bad++;
            continue;
        }
        /* Random values, including some outside the allowed range. */
        for (int p = 0; p < RB_PARAM_COUNT; p++) {
            double lo = rb_param_min(p), hi = rb_param_max(p), span = hi - lo;
            rb_set_param(s, p, lo - 0.1 * span + rnd01() * 1.2 * span);
        }
        for (int done = 0; done < 512; ) {
            int n = 1 + (int)(rnd01() * 200.0);
            if (n > 512 - done) n = 512 - done;
            rb_render_block(s, x + done, n);
            done += n;
        }
        for (int i = 0; i < 512; i++)
            if (!isfinite(x[i]) || fabs(x[i]) > 1.0) bad++;
        if (rb_get_telemetry(s, RB_TLM_NONFINITE) != 0.0) bad++;
        rb_destroy(s);
    }
    CHECK_EQ_INT(bad, 0);
}

void test_core_render_bad_args(void)
{
    rb_sim *s = rb_create(NULL);
    float x[4];

    CHECK(s != NULL);
    if (!s) return;
    CHECK_EQ_INT(rb_render_block(NULL, x, 4), RB_ERR_ARG);
    CHECK_EQ_INT(rb_render_block(s, NULL, 4), RB_ERR_ARG);
    CHECK_EQ_INT(rb_render_block(s, x, -1), RB_ERR_ARG);
    CHECK_EQ_INT(rb_render_block(s, x, 0), 0);
    CHECK(isnan(rb_get_telemetry(NULL, RB_TLM_TIME_S)));
    CHECK(isnan(rb_get_telemetry(s, RB_TLM_COUNT)));
    rb_destroy(s);
}

void test_core_host_helpers(void)
{
    rb_sim *s = rb_create_with(44100.0, 2);
    float *buf;

    CHECK(s != NULL);
    if (s) {
        CHECK_NEAR(rb_sample_rate(s), 44100.0, 0.0);
        CHECK_EQ_INT(rb_oversample(s), 2);
        rb_destroy(s);
    }
    CHECK(rb_create_with(0.0, 4) == NULL);
    CHECK(rb_create_with(48000.0, 0) == NULL);
    CHECK(rb_create_with(NAN, 4) == NULL);

    buf = rb_alloc_floats(128);
    CHECK(buf != NULL);
    if (buf) {
        CHECK(buf[0] == 0.0f && buf[127] == 0.0f);   /* zero-initialised */
        s = rb_create(NULL);
        if (s) {
            CHECK_EQ_INT(rb_render_block(s, buf, 128), 128);
            rb_destroy(s);
        }
    }
    rb_free_floats(buf);
    CHECK(rb_alloc_floats(0) == NULL);
    CHECK(rb_alloc_floats(-5) == NULL);
    rb_free_floats(NULL);
}
