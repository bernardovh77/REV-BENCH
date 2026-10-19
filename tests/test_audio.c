/*
 * test_audio.c - stage 11: the microphone, the anti-alias filter that takes
 * the physics rate down to the audio rate, the DC blocker and the soft
 * limiter.
 */
#include "audio.h"
#include "engine.h"
#include "rb.h"
#include "rb_internal.h"   /* the sim's sound source and its engine */
#include "rb_test.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846

static rb_decimator dec;   /* about 30 KB: kept off the stack */
static rb_engine twin;     /* about 320 KB */

/* |H(f)| of the decimator's filter, f in cycles per physics step. */
static double response(const rb_decimator *d, double f)
{
    double re = 0.0, im = 0.0;
    for (int n = 0; n < d->taps; n++) {
        re += d->h[n] * cos(2.0 * PI * f * n);
        im -= d->h[n] * sin(2.0 * PI * f * n);
    }
    return sqrt(re * re + im * im);
}

void test_audio_decimator_filter(void)
{
    /* For every oversampling factor: flat to 0.417 of the audio rate (20 kHz
     * at 48 kHz), within 0.001 dB, and 100 dB down from 0.583 of it (28 kHz)
     * to the physics rate's Nyquist. Its length grows with the factor, 157
     * taps at the default 4. */
    static const int factors[] = { 2, 3, 4, 8, 32 };
    for (int k = 0; k < 5; k++) {
        const int m = factors[k];
        CHECK_EQ_INT(rb_decimator_init(&dec, m), 0);
        double pass = 0.0, stop = 0.0;
        for (double f = 0.0; f <= 0.4167 / m; f += 0.002 / m)
            pass = fmax(pass, fabs(response(&dec, f) - 1.0));
        for (double f = 0.5833 / m; f <= 0.5; f += 0.0005 / m)
            stop = fmax(stop, response(&dec, f));
        CHECK(pass < 1.2e-4);                   /* 0.001 dB */
        CHECK(stop < 1.0e-5);                   /* -100 dB  */
        int symmetric = 1;
        for (int n = 0; n < dec.taps; n++) symmetric &= dec.h[n] == dec.h[dec.taps - 1 - n];
        CHECK(symmetric && dec.taps % 2 == 1);  /* linear phase */
    }
    CHECK_EQ_INT(rb_decimator_init(&dec, 4), 0);
    CHECK_EQ_INT(dec.taps, 157);
    CHECK_EQ_INT(rb_decimator_init(&dec, RB_DECIMATOR_MAX_FACTOR), 0);
    CHECK(dec.taps <= RB_DECIMATOR_MAX_TAPS);
    CHECK_EQ_INT(rb_decimator_init(&dec, 0), -1);
    CHECK_EQ_INT(rb_decimator_init(&dec, RB_DECIMATOR_MAX_FACTOR + 1), -1);
    CHECK_EQ_INT(rb_decimator_init(&dec, 1), 0);   /* 1 passes it through */
    rb_decimator_push(&dec, 0.25);
    CHECK(rb_decimator_out(&dec) == 0.25);
}

/* Pushes `frames` audio frames of sin(2 pi f t) at the physics rate (4 steps
 * a frame at 48 kHz) and returns the decimated output's RMS over the second
 * half. */
static double tone_through(double f, long frames)
{
    const double fs = 192000.0;
    double sumsq = 0.0;
    long n = 0;
    rb_decimator_init(&dec, 4);
    for (long i = 0; i < frames; i++) {
        for (int k = 0; k < 4; k++) rb_decimator_push(&dec, sin(2.0 * PI * f * (4 * i + k) / fs));
        const double y = rb_decimator_out(&dec);
        if (i >= frames / 2) {
            sumsq += y * y;
            n++;
        }
    }
    return sqrt(sumsq / n);
}

void test_audio_decimator_tones(void)
{
    /* Tones pushed at 192 kHz and taken out at 48 kHz: 1 kHz and 18 kHz come
     * through whole; 40 kHz, which would fold to 8 kHz, and 70 kHz (to 22 kHz)
     * don't. Without the filter (every fourth value) 40 kHz would come out at
     * full strength. The 1 kHz tone comes out 78 physics steps late, half the
     * filter. */
    CHECK_NEAR(tone_through(1000.0, 4800), sqrt(0.5), 1e-4 * sqrt(0.5));
    CHECK_NEAR(tone_through(18000.0, 4800), sqrt(0.5), 1e-4 * sqrt(0.5));
    CHECK(tone_through(40000.0, 4800) < 1e-5 * sqrt(0.5));
    CHECK(tone_through(70000.0, 4800) < 1e-5 * sqrt(0.5));

    rb_decimator_init(&dec, 4);
    double worst = 0.0;
    for (long i = 0; i < 960; i++) {
        for (int k = 0; k < 4; k++) rb_decimator_push(&dec, sin(2.0 * PI * 1000.0 * (4 * i + k) / 192000.0));
        if (i > 200)   /* the output at frame i is the input at step 4 i + 3 - 78 */
            worst = fmax(worst, fabs(rb_decimator_out(&dec) - sin(2.0 * PI * 1000.0 * (4 * i + 3 - 78) / 192000.0)));
    }
    CHECK(worst < 2e-4);

    /* A reset forgets the past: silence in, silence out. */
    rb_decimator_reset(&dec);
    rb_decimator_push(&dec, 0.0);
    CHECK(rb_decimator_out(&dec) == 0.0);
}

void test_audio_dc_blocker(void)
{
    /* A 10 Hz corner: a step decays with a time constant of 1 / (2 pi 10)
     * = 16 ms, 1 kHz and 30 Hz pass (30 Hz loses 5 %), and the gain at the
     * corner is 1 / sqrt 2. */
    rb_dc_blocker b;
    rb_dc_blocker_init(&b, RB_DC_CUTOFF, 48000.0);
    double y = 0.0;
    for (int n = 0; n < 48000 / 5; n++) y = rb_dc_blocker_step(&b, 1.0);   /* 0.2 s of a step */
    CHECK(fabs(y) < 1e-5);
    rb_dc_blocker_init(&b, RB_DC_CUTOFF, 48000.0);
    for (int n = 0; n < 960; n++) y = rb_dc_blocker_step(&b, 1.0);         /* 20 ms */
    CHECK_NEAR(y, exp(-0.02 * 2.0 * PI * RB_DC_CUTOFF), 2e-3);

    static const double freqs[3] = { 1000.0, 30.0, RB_DC_CUTOFF };
    const double gains[3] = { 1.0, 30.0 / sqrt(30.0 * 30.0 + 100.0), sqrt(0.5) };
    for (int k = 0; k < 3; k++) {
        rb_dc_blocker_init(&b, RB_DC_CUTOFF, 48000.0);
        double sumsq = 0.0;
        for (int n = 0; n < 4 * 48000; n++) {
            const double v = rb_dc_blocker_step(&b, sin(2.0 * PI * freqs[k] * n / 48000.0));
            if (n >= 2 * 48000) sumsq += v * v;
        }
        CHECK_NEAR(sqrt(sumsq / (2 * 48000)), gains[k] * sqrt(0.5), 2e-3 * sqrt(0.5));
    }
    rb_dc_blocker_reset(&b);
    CHECK(rb_dc_blocker_step(&b, 0.0) == 0.0);
}

void test_audio_soft_limit(void)
{
    /* Untouched up to the knee, then bending smoothly (no corner, no step)
     * towards 1, never past it, the same both ways. */
    int wrong = 0;
    double prev = rb_soft_limit(-50.0);
    for (double x = -50.0 + 1e-3; x <= 50.0; x += 1e-3) {
        const double y = rb_soft_limit(x);
        wrong += !(y >= prev);                         /* never turns back */
        wrong += !(fabs(y) <= 1.0);
        wrong += rb_soft_limit(-x) != -y;
        if (fabs(x) <= RB_LIMIT_KNEE) wrong += y != x;
        prev = y;
    }
    CHECK_EQ_INT(wrong, 0);
    const double h = 1e-7, k = RB_LIMIT_KNEE;
    CHECK_NEAR((rb_soft_limit(k + h) - rb_soft_limit(k)) / h, 1.0, 1e-6);   /* slope 1 at the knee */
    CHECK_NEAR(rb_soft_limit(1.0), k + (1.0 - k) * tanh(1.0), 1e-15);       /* full scale: 0.952 */
    CHECK(rb_soft_limit(2.0) > 0.99 && rb_soft_limit(2.0) < 1.0);
    CHECK(rb_soft_limit(1e300) == 1.0);
    CHECK(isnan(rb_soft_limit(NAN)));   /* left for the guard after it */
}

void test_audio_microphone(void)
{
    /* A flow Q0 sin(w t) out of a small opening: at r the microphone hears
     * rho0 w Q0 / (4 pi r) cos(w t), half as much twice as far away. The
     * first step after a reset hears nothing, whatever the flow. */
    const double rho0 = 1.18, q0 = 0.01, w = 2.0 * PI * 100.0, dt = 1.0 / 192000.0;
    for (int k = 1; k <= 2; k++) {
        rb_mic m;
        rb_mic_init(&m, k * 1.0);
        double worst = 0.0;
        CHECK(rb_mic_hear(&m, 0.05, rho0, dt) == 0.0);   /* primes it */
        rb_mic_reset(&m);
        for (int n = 0; n < 1920; n++) {
            const double p = rb_mic_hear(&m, q0 * sin(w * n * dt), rho0, dt);
            const double expected = rho0 * w * q0 / (4.0 * PI * k) * cos(w * (n - 0.5) * dt);
            if (n > 0) worst = fmax(worst, fabs(p - expected));
        }
        CHECK(worst < 1e-6 * rho0 * w * q0 / (4.0 * PI * k));
    }
}

/* A sim (48 kHz, oversample 4) whose sound is the default four held at
 * `rpm`, wide open, heard from `distance` m. */
static rb_sim *engine_sim(double rpm, double distance)
{
    rb_sim *sim = rb_create(NULL);
    if (!sim) return NULL;
    sim->source = RB_SOURCE_ENGINE;
    sim->microphone.distance = distance;
    sim->engine.rpm = rpm;
    sim->engine.throttle_pos = 1.0;
    sim->engine.ignition = 1;
    return sim;
}

/* Renders `frames` frames in blocks of `block`. */
static void render_engine(rb_sim *sim, float *out, int frames, int block)
{
    for (int done = 0; done < frames; done += block)
        rb_render_block(sim, out + done, frames - done < block ? frames - done : block);
}

/* The amplitude of frequency f (Hz) in x[0..n) at 48 kHz (n whole periods). */
static double amplitude(const float *x, int n, double f)
{
    double re = 0.0, im = 0.0;
    for (int i = 0; i < n; i++) {
        re += x[i] * cos(2.0 * PI * f * i / 48000.0);
        im += x[i] * sin(2.0 * PI * f * i / 48000.0);
    }
    return 2.0 * sqrt(re * re + im * im) / n;
}

void test_audio_engine_sound(void)
{
    /* The four held at 3000 rpm, wide open, through the sim's own chain (as
     * the browser will play it from stage 14). What comes out is what the
     * microphone hears, rho0 / (4 pi r) dQ/dt over full scale: its RMS is the
     * one worked out from a twin engine's tailpipe flow, to 1 % (the DC
     * blocker takes 0.4 % off the lowest orders). Its loudest
     * line is the firing (order 2, 100 Hz); it has no DC, nothing limited or
     * replaced; it's the same whatever the block size; and twice as far away
     * it's half as loud. */
    enum { SETTLE = 24000, N = 24000 };   /* 0.5 s each: 50 and 25 cycles */
    float *a = malloc(sizeof(float) * (SETTLE + N)), *b = malloc(sizeof(float) * (SETTLE + N));
    float *c = malloc(sizeof(float) * (SETTLE + N));
    rb_sim *one = engine_sim(3000.0, 1.0), *odd = engine_sim(3000.0, 1.0), *far = engine_sim(3000.0, 2.0);
    CHECK(a && b && c && one && odd && far);
    if (a && b && c && one && odd && far) {
        render_engine(one, a, SETTLE + N, 128);
        render_engine(odd, b, SETTLE + N, 7);
        render_engine(far, c, SETTLE + N, 128);
        CHECK(memcmp(a, b, sizeof(float) * (SETTLE + N)) == 0);

        double sumsq = 0.0, mean = 0.0, worst = 0.0;
        for (int i = SETTLE; i < SETTLE + N; i++) {
            sumsq += (double)a[i] * a[i];
            mean += a[i];
            worst = fmax(worst, fabs(0.5 * a[i] - c[i]));
        }
        const double rms = sqrt(sumsq / N);
        CHECK(fabs(mean / N) < 1e-3 * rms);
        CHECK(worst < 1e-6);
        CHECK_NEAR(rb_get_telemetry(one, RB_TLM_LIMITED), 0.0, 0.0);
        CHECK_NEAR(rb_get_telemetry(one, RB_TLM_NONFINITE), 0.0, 0.0);

        /* The microphone by hand: the same engine, stepped on its own. */
        rb_engine_init(&twin, &one->engine.desc, rb_physics_dt(one));
        twin.rpm = 3000.0;
        twin.throttle_pos = 1.0;
        twin.ignition = 1;
        double q = 0.0, p2 = 0.0;
        long n = 0;
        for (long k = 0; k < 4L * (SETTLE + N); k++) {
            rb_engine_step(&twin);
            if (twin.pipe_phase != 0) continue;   /* the flow changes once a pipe step */
            const double p = twin.exhaust_air.rho / (4.0 * PI) * (twin.tail_flow - q) / twin.pipe_dt;
            q = twin.tail_flow;
            if (k >= 4L * SETTLE) {
                p2 += p * p;
                n++;
            }
        }
        CHECK_NEAR(rms * RB_FULL_SCALE_PA, sqrt(p2 / n), 0.01 * sqrt(p2 / n));

        /* What the microphone hears is the volume flow: over whole cycles,
         * wide open (where it never turns round), the air and fuel the engine
         * takes in, at the density of the gas leaving, (air + fuel) R T / p,
         * to 0.1 % (it comes out at 0.03 %). */
        double volume = 0.0;
        long m = 0;
        const long c0 = twin.cycles;
        while (twin.cycles < c0 + 4) {
            rb_engine_step(&twin);
            if (twin.cycles > c0) {
                volume += twin.tail_flow;
                m++;
            }
        }
        const rb_pipe *tail = &twin.tailpipe[0];
        const double expected = (twin.result.air_flow + twin.result.fuel_flow) * tail->cell[tail->cells - 1].gas.R
                              * twin.result.t_tailpipe / twin.desc.p_ambient;
        CHECK_NEAR(volume / m, expected, 1e-3 * expected);

        double loudest = 0.0, at = 0.0;
        for (int k = 1; k <= 6; k++) {
            const double v = amplitude(a + SETTLE, N, k * 50.0);
            if (v > loudest) {
                loudest = v;
                at = k;
            }
        }
        CHECK_NEAR(at, 2.0, 0.0);
    }
    rb_destroy(one);
    rb_destroy(odd);
    rb_destroy(far);
    free(a);
    free(b);
    free(c);
}

void test_audio_engine_rebuilt_quietly(void)
{
    /* Loading another engine while the sound plays starts it again at rest:
     * the sound stops, but the microphone forgets the old flow, so the jump
     * to none makes no click (without that the first step would hear
     * 4500 Pa). The test tone is what a new sim plays (the browser's, until
     * stage 14). */
    enum { N = 9600 };
    float *x = malloc(sizeof(float) * N);
    rb_sim *sim = engine_sim(3000.0, 1.0);
    rb_sim *fresh = rb_create(NULL);
    CHECK(x && sim && fresh);
    if (x && sim && fresh) {
        render_engine(sim, x, N, 128);
        double before = 0.0, after = 0.0;
        for (int i = N / 2; i < N; i++) before = fmax(before, fabs(x[i]));
        CHECK(before > 0.05);
        CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("v8-cross")), RB_OK);
        render_engine(sim, x, N, 128);
        for (int i = 0; i < N; i++) after = fmax(after, fabs(x[i]));
        CHECK(after < before);
        CHECK_NEAR(rb_get_telemetry(sim, RB_TLM_LIMITED), 0.0, 0.0);
        CHECK_EQ_INT(fresh->source, RB_SOURCE_TONE);
    }
    rb_destroy(sim);
    rb_destroy(fresh);
    free(x);
}

void test_audio_chain_blocks_dc(void)
{
    /* The DC blocker is in the sim's chain: a 20 Hz tone, twice its corner,
     * comes out at 20 / sqrt(20^2 + 10^2) = 0.894 of its level. */
    enum { N = 48000 };
    float *x = malloc(sizeof(float) * N);
    rb_sim *sim = rb_create(NULL);
    CHECK(x && sim);
    if (x && sim) {
        rb_set_param(sim, RB_PARAM_TEST_TONE_HZ, 20.0);
        rb_set_param(sim, RB_PARAM_TEST_TONE_LEVEL, 0.5);
        render_engine(sim, x, N, 128);   /* 1 s to settle */
        render_engine(sim, x, N, 128);
        CHECK_NEAR(amplitude(x, N, 20.0), 0.5 * 20.0 / sqrt(500.0), 2e-3);
    }
    rb_destroy(sim);
    free(x);
}

void test_audio_chain_survives_nan(void)
{
    /* A NaN from the source (here the tone's level, broken from the inside)
     * becomes silence and is counted at every physics step, before it can
     * reach the filters' memories: once the source is well again, so is the
     * sound, without a reset. */
    enum { N = 4800 };
    float *x = malloc(sizeof(float) * N);
    rb_sim *sim = rb_create(NULL);
    CHECK(x && sim);
    if (x && sim) {
        rb_set_param(sim, RB_PARAM_TEST_TONE_HZ, 440.0);
        render_engine(sim, x, N, 128);
        sim->tone_level_s = NAN;
        render_engine(sim, x, N, 128);
        double tail = 0.0;   /* what's left is the DC blocker's memory, dying away */
        for (int i = N / 2; i < N; i++) tail = fmax(tail, fabs(x[i]));
        CHECK(tail < 0.002);
        CHECK_NEAR(rb_get_telemetry(sim, RB_TLM_NONFINITE), 4.0 * N, 0.0);
        sim->tone_level_s = 0.0;   /* well again: it fades back in */
        render_engine(sim, x, N, 128);
        double peak = 0.0;
        for (int i = N / 2; i < N; i++) peak = fmax(peak, fabs(x[i]));
        CHECK(peak > 0.4 && peak < 0.51);
        CHECK_NEAR(rb_get_telemetry(sim, RB_TLM_NONFINITE), 4.0 * N, 0.0);
    }
    rb_destroy(sim);
    free(x);
}

void test_audio_engine_reversal_quiet(void)
{
    /* At 2000 rpm, wide open, the four's tailpipe flow turns round every
     * firing. Its volume flow is taken at the density of the gas at the pipe's
     * end either way, so it bends smoothly through the turn: held, the sound
     * repeats every cycle, and its harmonics above 5 kHz carry -67 dB of its
     * power. (Taking the outside air's density for the flow drawn in kinks it
     * at every turn: -37 dB, a click a firing.) */
    enum { SETTLE = 24000, N = 9 * 2880 };   /* 9 cycles of 60 ms */
    float *x = malloc(sizeof(float) * (SETTLE + N));
    rb_sim *sim = engine_sim(2000.0, 1.0);
    CHECK(x && sim);
    if (x && sim) {
        render_engine(sim, x, SETTLE + N, 128);
        const float *y = x + SETTLE;
        double all = 0.0, high = 0.0;
        for (int i = 0; i < N; i++) all += (double)y[i] * y[i] / N;   /* every harmonic's power */
        for (int k = 301; k <= 1440; k++) {    /* the cycle's harmonics, 5 to 24 kHz */
            const double w = 2.0 * PI * k / 2880.0;   /* rad per frame */
            const double c = cos(w), sn = sin(w);
            double re = 0.0, im = 0.0, pr = 1.0, pi = 0.0;
            for (int i = 0; i < N; i++) {      /* a phasor turned step by step */
                re += y[i] * pr;
                im += y[i] * pi;
                const double t = pr * c - pi * sn;
                pi = pr * sn + pi * c;
                pr = t;
            }
            high += 2.0 * (re * re + im * im) / ((double)N * N);
        }
        CHECK(high < 3.2e-6 * all);   /* -55 dB */
    }
    rb_destroy(sim);
    free(x);
}
