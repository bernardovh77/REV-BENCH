/*
 * test_audio.c - stages 11 and 12: the listener (stage 11's microphone, put
 * around the car), the anti-alias filter that takes the physics rate down to
 * the audio rate, the DC blocker and the soft limiter.
 */
#include "audio.h"
#include "engine.h"
#include "presets.h"
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

void test_audio_listener_monopole(void)
{
    /* One source, a flow Q0 sin(w t) out of a small opening, changing once a
     * pipe step (every 4 physics steps): r away, with no ground, each pipe
     * step's physics steps hear rho0 / (4 pi r) (Q_k - Q_k-1) / pipe_dt
     * between them, rho0 w Q0 / (4 pi r) cos(w t) to 1e-4: half as much twice
     * as far away. Primed with a flow, it hears nothing until it changes. */
    const double rho0 = 1.18, q0 = 0.01, w = 2.0 * PI * 100.0, dt = 1.0 / 192000.0, pipe_dt = 4.0 * dt;
    static rb_listener l;
    for (int k = 1; k <= 2; k++) {
        const double src[3] = { 0.0, 0.0, 1.0 }, at[3] = { 0.0, k * 1.0, 1.0 };
        const double *pos[RB_LISTEN_SOURCES] = { src, NULL, NULL };
        rb_listener_set(&l, pos, at, 0.0, 343.0, pipe_dt);
        CHECK_NEAR(rb_listener_distance(&l, 0), k * 1.0, 1e-12);
        CHECK_NEAR(rb_listener_distance(&l, 1), 0.0, 0.0);
        const double q[RB_LISTEN_SOURCES] = { 0.05, 0.0, 0.0 };
        rb_listener_prime(&l, q);
        CHECK(rb_listener_hear(&l, rho0, dt) == 0.0);
        const double zero[RB_LISTEN_SOURCES] = { 0.0 };
        rb_listener_prime(&l, zero);
        double worst = 0.0;
        for (int n = 1; n < 480; n++) {
            const double qn[RB_LISTEN_SOURCES] = { q0 * sin(w * n * pipe_dt), 0.0, 0.0 };
            rb_listener_push(&l, qn);
            double mean = 0.0;
            for (int i = 0; i < 4; i++) mean += 0.25 * rb_listener_hear(&l, rho0, dt);
            const double expected = rho0 * w * q0 / (4.0 * PI * k) * cos(w * (n - 0.5) * pipe_dt);
            worst = fmax(worst, fabs(mean - expected));
        }
        CHECK(worst < 1e-4 * rho0 * w * q0 / (4.0 * PI * k));
    }
}

void test_audio_listener_paths(void)
{
    /* Two sources 1 m and about 2.5 m away, 0.35 m up, the listener 0.5 m
     * up, the ground reflecting 0.9: each source is heard straight and off
     * the ground, the nearest path at once, the others (r - 1) / c later (in
     * pipe steps, read between steps), each at its 1 / r. A flow switched on
     * in both arrives path by path, as those say, and adds up to every path's
     * 1 / r. */
    static rb_listener l;
    const double c = 343.0, pipe_dt = 1.0 / 48000.0, at[3] = { 0.0, 1.0, 0.5 };
    const double s0[3] = { 0.0, 1.0 - sqrt(1.0 - 0.15 * 0.15), 0.35 };   /* 1 m away exactly */
    const double s1[3] = { 0.0, 1.0 + 2.5, 0.35 };
    const double *pos[RB_LISTEN_SOURCES] = { s0, s1, NULL };
    rb_listener_set(&l, pos, at, 0.9, c, pipe_dt);
    const double r0 = 1.0, r1 = sqrt(2.5 * 2.5 + 0.15 * 0.15);
    const double g0 = sqrt(1.0 - 0.15 * 0.15), g1 = 2.5;   /* horizontal distances */
    const double i0 = sqrt(g0 * g0 + 0.85 * 0.85), i1 = sqrt(g1 * g1 + 0.85 * 0.85);   /* off the ground */
    CHECK_NEAR(rb_listener_distance(&l, 0), r0, 1e-12);
    CHECK_NEAR(1.0 / l.gain[1][0], r1, 1e-12);
    CHECK_NEAR(l.gain[0][1], 0.9 / i0, 1e-12);
    CHECK_NEAR(l.gain[1][1], 0.9 / i1, 1e-12);
    CHECK_NEAR(l.delay[0][0], 0.0, 0.0);
    CHECK_NEAR(l.delay[1][0], (r1 - r0) / (c * pipe_dt), 1e-9);
    CHECK_NEAR(l.delay[0][1], (i0 - r0) / (c * pipe_dt), 1e-9);
    CHECK_NEAR(l.delay[1][1], (i1 - r0) / (c * pipe_dt), 1e-9);

    const double off[RB_LISTEN_SOURCES] = { 0.0 }, on[RB_LISTEN_SOURCES] = { 1.0, 1.0, 1.0 };
    rb_listener_prime(&l, off);
    int wrong = 0;
    for (int n = 0; n < 1200; n++) {
        rb_listener_push(&l, on);
        double expected = 0.0;   /* each path: how much of the step has arrived after n steps */
        for (int j = 0; j < 2; j++)
            for (int k = 0; k < 2; k++) {
                const double d = l.delay[j][k];
                const double arrived = (n >= d) ? 1.0 : (n + 1 > d) ? n + 1 - d : 0.0;
                expected += l.gain[j][k] * arrived;
            }
        wrong += fabs(l.sum - expected) > 1e-12;
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK_NEAR(l.sum, 1.0 / r0 + 1.0 / r1 + 0.9 / i0 + 0.9 / i1, 1e-12);
}

void test_audio_listener_places(void)
{
    /* Around the car: 1 m behind tailpipe 0 (stage 11's microphone, 15 cm
     * higher), the other tailpipe of a twin exhaust a metre to the side; from
     * beside the car both tailpipes, the second further away; over the bonnet,
     * the intake's mouth nearest. One tailpipe: no second source. */
    static rb_listener l;
    const double c = 346.0, pipe_dt = 1.0 / 48000.0;
    CHECK_EQ_INT(rb_listener_place(&l, RB_HEAR_TAILPIPE, 2, c, pipe_dt), 0);
    CHECK_NEAR(rb_listener_distance(&l, 0), sqrt(1.0 + 0.15 * 0.15), 1e-12);
    CHECK_NEAR(rb_listener_distance(&l, 1), sqrt(1.0 + 1.0 + 0.15 * 0.15), 1e-12);
    CHECK(rb_listener_distance(&l, 2) > 4.0);
    CHECK_EQ_INT(rb_listener_place(&l, RB_HEAR_TAILPIPE, 1, c, pipe_dt), 0);
    CHECK_NEAR(rb_listener_distance(&l, 1), 0.0, 0.0);
    CHECK_EQ_INT(rb_listener_place(&l, RB_HEAR_SIDE, 2, c, pipe_dt), 0);
    CHECK(rb_listener_distance(&l, 0) > 4.0 && rb_listener_distance(&l, 1) > rb_listener_distance(&l, 0) + 0.5);
    CHECK_EQ_INT(rb_listener_place(&l, RB_HEAR_FRONT, 1, c, pipe_dt), 0);
    CHECK(rb_listener_distance(&l, 2) < 1.0 && rb_listener_distance(&l, 0) > 3.0);
    CHECK_NEAR(l.delay[2][0], 0.0, 0.0);   /* the mouth's the nearest */
    CHECK_EQ_INT(rb_listener_place(&l, RB_HEAR_BEHIND, 1, c, pipe_dt), 0);
    CHECK(rb_listener_distance(&l, 0) > 3.0 && rb_listener_distance(&l, 0) < 3.5);
    CHECK_EQ_INT(rb_listener_place(&l, -1, 1, c, pipe_dt), -1);
    CHECK_EQ_INT(rb_listener_place(&l, RB_HEAR_COUNT, 1, c, pipe_dt), -1);
}

/* A sim (48 kHz, oversample 4) whose sound is the default four held at
 * `rpm`, wide open, heard r m from its tailpipe, with nothing else and no
 * ground. */
static rb_sim *engine_sim(double rpm, double r)
{
    rb_sim *sim = rb_create(NULL);
    if (!sim) return NULL;
    sim->source = RB_SOURCE_ENGINE;
    const double src[3] = { 0.0, 0.0, 1.0 }, at[3] = { 0.0, r, 1.0 };
    const double *pos[RB_LISTEN_SOURCES] = { src, NULL, NULL };
    const double zero[RB_LISTEN_SOURCES] = { 0.0 };
    rb_listener_set(&sim->listener, pos, at, 0.0, 346.0, sim->engine.pipe_dt);
    rb_listener_prime(&sim->listener, zero);
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
     * listener hears, rho0 / (4 pi r) dQ/dt over full scale: its RMS is the
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

        /* The listener by hand: the same engine, stepped on its own. */
        rb_engine_init(&twin, &one->engine.desc, rb_physics_dt(one));
        twin.rpm = 3000.0;
        twin.throttle_pos = 1.0;
        twin.ignition = 1;
        double q = 0.0, p2 = 0.0;
        long n = 0;
        for (long k = 0; k < 4L * (SETTLE + N); k++) {
            rb_engine_step(&twin);
            if (twin.pipe_phase != 0) continue;   /* the flow changes once a pipe step */
            const double p = twin.exhaust_air.rho / (4.0 * PI) * (twin.tail_flow[0] - q) / twin.pipe_dt;
            q = twin.tail_flow[0];
            if (k >= 4L * SETTLE) {
                p2 += p * p;
                n++;
            }
        }
        CHECK_NEAR(rms * RB_FULL_SCALE_PA, sqrt(p2 / n), 0.01 * sqrt(p2 / n));

        /* What the listener hears is the volume flow: over whole cycles,
         * wide open (where it never turns round), the air and fuel the engine
         * takes in, at the density of the gas leaving, (air + fuel) R T / p,
         * to 0.1 % (it comes out at 0.03 %). */
        double volume = 0.0;
        long m = 0;
        const long c0 = twin.cycles;
        while (twin.cycles < c0 + 4) {
            rb_engine_step(&twin);
            if (twin.cycles > c0) {
                volume += twin.tail_flow[0];
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
     * the sound stops, but the listener forgets the old flow, so the jump
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

/* ---- stage 12: the mufflers, the intake's mouth, the listener and the
 * headers, through the tailpipes' and the mouth's flows ---- */

/* The amplitude of engine order `order` (per turn of the crank) in x[0..n),
 * sampled every pipe step of an engine at `rpm` (n whole cycles' worth). */
static double order_amplitude(const double *x, long n, double order, double rpm, double pipe_dt)
{
    const double w = 2.0 * PI * order * rpm / 60.0 * pipe_dt;   /* rad per sample */
    double re = 0.0, im = 0.0;
    for (long i = 0; i < n; i++) {
        re += x[i] * cos(w * i);
        im += x[i] * sin(w * i);
    }
    return 2.0 * sqrt(re * re + im * im) / n;
}

/* The power of x[0..n) in a band of orders [lo, hi), in steps of half an
 * order (every line a 720-degree cycle can have). */
static double band_power(const double *x, long n, double lo, double hi, double rpm, double pipe_dt)
{
    double p = 0.0;
    for (double k = lo; k < hi; k += 0.5) {
        const double a = order_amplitude(x, n, k, rpm, pipe_dt);
        p += 0.5 * a * a;
    }
    return p;
}

static double flows[2][4 * 1920];   /* 4 cycles at 3000 rpm, every pipe step */

/* Runs `twin` (set up by the caller) held wide open at 3000 rpm for `settle`
 * seconds, then keeps each tailpipe's sound source, the change of its volume
 * flow over each pipe step, for 4 cycles. Returns the samples kept. */
static long record_tailpipes(double settle)
{
    twin.rpm = 3000.0;
    twin.throttle_pos = 1.0;
    twin.ignition = 1;
    for (long k = 0; k < lround(settle / twin.dt); k++) rb_engine_step(&twin);
    double prev[2] = { twin.tail_flow[0], twin.tail_flow[1] };
    long n = 0;
    while (n < 4 * 1920) {
        rb_engine_step(&twin);
        if (twin.pipe_phase != 0) continue;   /* a pipe step has just ended */
        for (int s = 0; s < 2; s++) {
            flows[s][n] = twin.tail_flow[s] - prev[s];
            prev[s] = twin.tail_flow[s];
        }
        n++;
    }
    return n;
}

void test_audio_mufflers(void)
{
    /* The four held wide open at 3000 rpm (orders 2, 4, 6, ... at 100, 200,
     * 300 Hz) with each muffler, the tailpipe's sound source compared. A
     * chambered one, the can and its baffles, takes everything above the
     * exhaust's lowest ring (about 63 Hz) down: 14 dB in all, at least 8. A
     * straight-through one passes the firing (order 2 within 1 dB of the
     * straight pipe's) but its packing takes 6 dB off orders 16 to 32
     * (800-1600 Hz; at least 4), and 3 dB off orders 8 to 16. */
    double all[3], firing[3], mid[3];
    for (int type = RB_MUFFLER_NONE; type <= RB_MUFFLER_CHAMBERED; type++) {
        rb_engine_desc d;
        rb_engine_desc_default(&d);
        d.muffler_type = type;
        rb_engine_init(&twin, &d, 1.0 / 192000.0);
        const long n = record_tailpipes(0.5);
        all[type] = band_power(flows[0], n, 0.5, 40.0, 3000.0, twin.pipe_dt);
        firing[type] = order_amplitude(flows[0], n, 2.0, 3000.0, twin.pipe_dt);
        mid[type] = band_power(flows[0], n, 16.0, 32.0, 3000.0, twin.pipe_dt);
    }
    const double db = 10.0 / log(10.0);
    CHECK(db * log(all[RB_MUFFLER_NONE] / all[RB_MUFFLER_CHAMBERED]) > 8.0);
    CHECK(fabs(20.0 / log(10.0) * log(firing[RB_MUFFLER_STRAIGHT] / firing[RB_MUFFLER_NONE])) < 1.0);
    CHECK(db * log(mid[RB_MUFFLER_NONE] / mid[RB_MUFFLER_STRAIGHT]) > 4.0);
}

void test_audio_intake_mouth(void)
{
    /* The intake's mouth (stage 12) is a source like the tailpipes: its
     * volume flow, at the density of the air at the snorkel's end. Wide open
     * at 4000 rpm, over whole cycles it's the air the engine takes in, at the
     * outside air's density (to 1 %: the air speeds up into the mouth, and
     * thins a little). Nearly shut, the throttle is choked: the plenum's swings
     * can't get through it, and the mouth's flow holds still (its swings 60 dB
     * below wide open's). */
    double swing[2];
    for (int k = 0; k < 2; k++) {
        rb_engine_desc d;
        rb_engine_desc_default(&d);
        rb_engine_init(&twin, &d, 1.0 / 192000.0);
        twin.rpm = 4000.0;
        twin.throttle_pos = k ? 0.04 : 1.0;
        twin.ignition = 1;
        if (k == 0) CHECK(rb_engine_settle(&twin, 100, 1e-4) > 0);   /* ends as a cycle does */
        else
            while (twin.cycles < 10) rb_engine_step(&twin);   /* choked within a few cycles */
        double sum = 0.0, sq = 0.0;
        long n = 0;
        const long c0 = twin.cycles;
        while (twin.cycles < c0 + 4) {
            rb_engine_step(&twin);
            if (twin.pipe_phase != 0) continue;
            sum += twin.mouth_flow;
            sq += twin.mouth_flow * twin.mouth_flow;
            n++;
        }
        const double mean = sum / n;
        swing[k] = sqrt(sq / n - mean * mean);
        if (k == 0) {
            const double rho = d.p_ambient / (twin.intake_air.gas.R * d.T_ambient);
            CHECK(mean < 0.0);   /* drawing in */
            CHECK_NEAR(-mean, twin.result.air_flow / rho, 0.01 * twin.result.air_flow / rho);
        }
    }
    CHECK(swing[1] < 1e-3 * swing[0]);
}

void test_audio_headers_rumble(void)
{
    /* The plan's check for stage 12: equal- and unequal-length headers sound
     * different. The boxer's headers are 45 and 75 cm, so its four pulses a
     * cycle reach the collector unevenly, and its exhaust carries every half
     * order: order 1.5 within 25 dB of the firing (order 2; 17 dB at the
     * listener). With equal headers its pulses come evenly, every 180
     * degrees, and only the firing and its multiples are left: order 1.5 at
     * least 60 dB down. */
    double rel[2];
    for (int equal = 0; equal < 2; equal++) {
        rb_engine_desc d;
        rb_preset_desc(rb_preset_find("boxer4"), &d);
        if (equal)
            for (int i = 0; i < d.cylinders; i++) d.header_length[i] = 0.6;
        rb_engine_init(&twin, &d, 1.0 / 192000.0);
        const long n = record_tailpipes(0.4);
        rel[equal] = order_amplitude(flows[0], n, 1.5, 3000.0, twin.pipe_dt)
                   / order_amplitude(flows[0], n, 2.0, 3000.0, twin.pipe_dt);
    }
    CHECK(rel[0] > 0.056);   /* -25 dB */
    CHECK(rel[1] < 1e-3);    /* -60 dB */
}

void test_audio_cylinders_sound_apart(void)
{
    /* Stage 12c: like a production manifold, the four's and the six's
     * headers aren't all alike, so each cylinder's pulse reaches the
     * tailpipe its own way and the orders between the firing ones are there
     * even with every cycle the same (variation 0): the strongest of them
     * within 35 dB of the firing order. With every header alike they were
     * gone (the four's 60 dB down): a pure note. */
    static const char *const keys[2] = { "i4", "i6" };
    for (int p = 0; p < 2; p++) {
        double rel[2];
        for (int equal = 0; equal < 2; equal++) {
            rb_engine_desc d;
            rb_preset_desc(rb_preset_find(keys[p]), &d);
            if (equal)
                for (int i = 0; i < d.cylinders; i++) d.header_length[i] = 0.45;
            rb_engine_init(&twin, &d, 1.0 / 192000.0);
            const long n = record_tailpipes(0.4);
            const double fire = d.cylinders / 2.0;
            double most = 0.0;
            for (double k = 0.5; k < fire; k += 0.5)
                most = fmax(most, order_amplitude(flows[0], n, k, 3000.0, twin.pipe_dt));
            rel[equal] = most / order_amplitude(flows[0], n, fire, 3000.0, twin.pipe_dt);
        }
        CHECK(rel[0] > 0.018);   /* -35 dB */
        CHECK(rel[1] < 1e-3);    /* -60 dB */
    }
}

void test_audio_listener_hears_each_tailpipe(void)
{
    /* The cross-plane V8's banks fire the same uneven pattern, one turn of
     * the crank apart. With every header alike (since stage 12c the preset's
     * differ, as a production manifold's do), heard from the middle of its
     * twin exhaust, the half orders of one tailpipe cancel the other's, as in
     * stage 11 (60 dB below the firing, order 4). Heard from 1 m behind one
     * tailpipe (the listener's RB_HEAR_TAILPIPE), the other's arrives later
     * and weaker, and they don't: the strongest of orders 0.5 to 3.5 is within
     * 25 dB of the firing. */
    static double heard[4 * 1920];
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("v8-cross"), &d);
    for (int i = 0; i < d.cylinders; i++) d.header_length[i] = 0.40;
    rb_engine_init(&twin, &d, 1.0 / 192000.0);
    const long n = record_tailpipes(0.3);
    static rb_listener l;
    const double c = 346.0;
    double half[2], fire[2];
    for (int where = 0; where < 2; where++) {
        if (where == 0) {   /* the middle: both tailpipes 1.1 m away */
            const double t0[3] = { 0.5, 0.0, 0.35 }, t1[3] = { -0.5, 0.0, 0.35 }, at[3] = { 0.0, -1.0, 0.5 };
            const double *pos[RB_LISTEN_SOURCES] = { t0, t1, NULL };
            rb_listener_set(&l, pos, at, RB_GROUND, c, twin.pipe_dt);
        } else {
            rb_listener_place(&l, RB_HEAR_TAILPIPE, 2, c, twin.pipe_dt);
            l.gain[2][0] = l.gain[2][1] = 0.0;   /* the tailpipes alone */
        }
        const double zero[RB_LISTEN_SOURCES] = { 0.0 };
        rb_listener_prime(&l, zero);
        double q[RB_LISTEN_SOURCES] = { 0.0 };
        for (long i = 0; i < n; i++) {
            q[0] += flows[0][i];   /* the flows themselves, back from their changes */
            q[1] += flows[1][i];
            rb_listener_push(&l, q);
            heard[i] = rb_listener_hear(&l, 1.2, twin.pipe_dt);
        }
        /* the last 3 cycles: the history's longest delay has filled */
        const long from = n - 3 * 1920;
        half[where] = 0.0;
        for (double k = 0.5; k < 4.0; k += 1.0)
            half[where] = fmax(half[where], order_amplitude(heard + from, 3 * 1920, k, 3000.0, twin.pipe_dt));
        fire[where] = order_amplitude(heard + from, 3 * 1920, 4.0, 3000.0, twin.pipe_dt);
    }
    CHECK(half[0] < 1e-3 * fire[0]);
    CHECK(half[1] > 0.056 * fire[1]);
}

void test_audio_sim_hears_every_source(void)
{
    /* The sim's listener (stage 12) stands behind the car until told
     * otherwise, and hears every one of the engine's sources: each pipe step
     * it gets both tailpipes' flows and the intake mouth's, as they are. A
     * rebuilt engine with one tailpipe has no second source. */
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK_EQ_INT(sim->hear, RB_HEAR_BEHIND);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("v8-cross")), RB_OK);
    sim->source = RB_SOURCE_ENGINE;
    sim->engine.rpm = 3000.0;
    sim->engine.throttle_pos = 1.0;
    sim->engine.ignition = 1;
    static float x[4800];
    rb_render_block(sim, x, 4800);
    const rb_listener *l = &sim->listener;
    CHECK(sim->engine.pipe_steps > 1000);
    CHECK(l->flow[0][l->pos] == sim->engine.tail_flow[0] && l->flow[0][l->pos] != 0.0);
    CHECK(l->flow[1][l->pos] == sim->engine.tail_flow[1] && l->flow[1][l->pos] != 0.0);
    CHECK(l->flow[2][l->pos] == sim->engine.mouth_flow && l->flow[2][l->pos] < 0.0);
    CHECK(rb_listener_distance(l, 0) > 3.0 && rb_listener_distance(l, 1) > 3.0 && rb_listener_distance(l, 2) > 6.0);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i4")), RB_OK);
    CHECK(rb_listener_distance(&sim->listener, 0) > 3.0 && rb_listener_distance(&sim->listener, 1) == 0.0);
    rb_destroy(sim);
}
