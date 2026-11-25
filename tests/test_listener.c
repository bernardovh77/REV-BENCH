/*
 * test_listener.c - stage 056: the stereo scene's listener against analytic
 * controls (scene.c, stage 024 and S4): fractional delays and their band,
 * Doppler and retarded distance in the pass-by, a history that stays in
 * range, known-position stereo, the cabin's transfer, and all eight scenes.
 */
#include "audio.h"
#include "rb.h"
#include "rb_internal.h"
#include "rb_test.h"
#include "scene.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846
#define FS 48000.0

static rb_scene sc;

#define MOUTH RB_SCENE_OUTLETS   /* the scene's mouth: a point source (scene.h) */

/* Silences every path but ear e's from source j by way k, and its shadow. */
static void only(rb_scene *s, int e, int j, int k)
{
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < RB_SCENE_PLACED; b++)
            for (int c = 0; c < 3; c++) {
                if (a != e || b != j || c != k) s->path[a][b][c].gain = 0.0;
                s->path[a][b][c].shadow = 0.0;
            }
}

/* The ear's amplitude and phase (rad) at f from source j alone, a tone of
 * amplitude 1, measured over whole periods after the scene has filled. */
static void response(rb_scene *s, int e, int j, double f, double *amplitude, double *phase)
{
    const int settle = 6000, count = (int)(lround(floor(0.25 * f)) * FS / f);
    double re = 0.0, im = 0.0;
    for (int i = 0; i < settle + count; i++) {
        double in[RB_SCENE_SOURCES] = { 0.0 }, out[2];
        in[j] = sin(2.0 * PI * f * i / FS);
        rb_scene_step(s, in, out);
        if (i >= settle) {
            re += out[e] * sin(2.0 * PI * f * i / FS);
            im += out[e] * cos(2.0 * PI * f * i / FS);
        }
    }
    *amplitude = 2.0 * sqrt(re * re + im * im) / count;
    *phase = atan2(-im, re);   /* the lag: out = A sin(w t - phase) */
}

void test_listener_fractional_delay_band(void)
{
    /* One path, its delay set to half a sample past a whole one: the worst
     * case for a fractional delay. Its gain is the path's to 0.6 dB up to
     * 10 kHz (cubic Lagrange: 0.54 dB at 10 kHz; a linear read loses 2.0),
     * and its phase is the delay's, 2 pi f d / fs. */
    const double freqs[4] = { 500.0, 4000.0, 8000.0, 10000.0 };
    for (int q = 0; q < 4; q++) {
        rb_scene_init(&sc, RB_SCENE_ASPHALT, FS, 343.0, 1, 0.06);
        only(&sc, 0, MOUTH, 0);   /* the mouth: a point, no aperture */
        rb_scene_path *p = &sc.path[0][MOUTH][0];
        p->delay = floor(p->delay) + 0.5;
        const double gain = p->gain, delay = p->delay;
        double a, ph;
        response(&sc, 0, MOUTH, freqs[q], &a, &ph);
        const double loss = 20.0 * log10(a / gain);
        printf("    %5.0f Hz at half a sample: %+.3f dB\n", freqs[q], loss);
        CHECK(loss > -0.6 && loss < 0.01);
        double lag = 2.0 * PI * freqs[q] * delay / FS;
        lag = fmod(lag, 2.0 * PI);
        double off = fmod(ph - lag + 3.0 * PI, 2.0 * PI) - PI;
        CHECK(fabs(off) < 1e-6);
    }
    /* A whole-sample delay passes everything, to the history's float. */
    rb_scene_init(&sc, RB_SCENE_ASPHALT, FS, 343.0, 1, 0.06);
    only(&sc, 0, MOUTH, 0);
    sc.path[0][MOUTH][0].delay = floor(sc.path[0][MOUTH][0].delay);
    double a, ph;
    response(&sc, 0, MOUTH, 15000.0, &a, &ph);
    CHECK_NEAR(a / sc.path[0][MOUTH][0].gain, 1.0, 1e-6);   /* the history is float */
}

/* The pass-by's retarded flight time to the listener's left ear from the
 * mouth (source 2), or from its image in the ground, at reception time t, by
 * bisection: |p(t - tau) - ear| = c tau, the car at 15 m/s along y, at the
 * listener's line at t = 6 s. The test's own solution of the geometry. */
static double flight_from(double t, const double src[3], double *y_then)
{
    const double ear[3] = { 7.5 - 0.09, 0.0, 1.6 };
    double lo = 0.0, hi = 1.0;
    for (int it = 0; it < 64; it++) {
        const double tau = 0.5 * (lo + hi);
        const double y = src[1] + 15.0 * (t - tau - 6.0);
        const double dx = src[0] - ear[0], dy = y - ear[1], dz = src[2] - ear[2];
        if (sqrt(dx * dx + dy * dy + dz * dz) > 343.0 * tau) lo = tau;
        else hi = tau;
    }
    const double tau = 0.5 * (lo + hi);
    if (y_then) *y_then = src[1] + 15.0 * (t - tau - 6.0) - ear[1];
    return tau;
}

static double flight(double t, int ground)
{
    const double ear[3] = { 7.5 - 0.09, 0.0, 1.6 };
    const double src[3] = { -0.3, 3.7, ground ? -0.75 : 0.75 };
    double lo = 0.0, hi = 1.0;
    for (int it = 0; it < 64; it++) {
        const double tau = 0.5 * (lo + hi);
        const double y = src[1] + 15.0 * (t - tau - 6.0);
        const double dx = src[0] - ear[0], dy = y - ear[1], dz = src[2] - ear[2];
        if (sqrt(dx * dx + dy * dy + dz * dz) > 343.0 * tau) lo = tau;
        else hi = tau;
    }
    return 0.5 * (lo + hi);
}

void test_listener_doppler_analytic(void)
{
    /* The pass-by: a 600 Hz tone from the mouth, 10 s, the car passing at
     * 6 s. The near ear hears, sample by sample, the tone as it left the car
     * a retarded flight time ago, at 1 / (c tau), directly and off the
     * asphalt (0.9): sum over k of g_k sin(2 pi f (t - tau_k)), tau solved
     * here independently. Within 0.1 % of the loudest sample, every sample
     * (the scene works its paths out every 64 samples and follows them
     * linearly in between). Its Doppler factor 1 - dtau/dt, printed. The
     * history is read only in range all the while. */
    rb_scene_init(&sc, RB_SCENE_PASSBY, FS, 343.0, 1, 0.06);
    double most = 0.0, worst = 0.0, loudest = 0.0;
    for (int i = 0; i < 480000; i++) {
        double in[RB_SCENE_SOURCES] = { 0.0 }, o[2];
        in[MOUTH] = sin(2.0 * PI * 600.0 * i / FS);
        rb_scene_step(&sc, in, o);
        most = fmax(most, sc.path[0][MOUTH][0].delay);
        /* The tailpipe's opening, seen from where the car was when it
         * sent the sound: its spread is 0.5 D sin(angle) / c, the angle the
         * retarded position's (checked as each block's paths are worked out). */
        if ((i & 63) == 0 && i % 48000 == 0) {
            const double pipe[3] = { 0.5, -0.05, 0.35 };
            double y;
            const double tau = flight_from(i / FS, pipe, &y);
            const double d = 343.0 * tau;
            const double spread = 0.5 * 0.06 * sqrt(1.0 - y * y / (d * d)) / 343.0 * FS;
            CHECK_NEAR(sc.path[0][0][0].spread / spread, 1.0, 1e-4);   /* one step on */
        }
        if (i < 24000) continue;   /* the history filling */
        const double t = i / FS;
        double expected = 0.0;
        for (int k = 0; k < 2; k++) {
            const double tau = flight(t, k);
            expected += (k ? 0.9 : 1.0) / (343.0 * tau) * sin(2.0 * PI * 600.0 * (t - tau));
        }
        worst = fmax(worst, fabs(o[0] - expected));
        loudest = fmax(loudest, fabs(expected));
    }
    printf("    worst sample off by %.2e of the loudest\n", worst / loudest);
    CHECK(worst < 1e-3 * loudest);
    CHECK(most < RB_SCENE_RING - 3);
    const double at[5] = { 2.0, 4.0, 5.5, 6.5, 8.0 };
    for (int w = 0; w < 5; w++) {
        const double h = 1e-3;
        printf("    at %.1f s: Doppler factor %.4f\n", at[w],
               1.0 - (flight(at[w] + h, 0) - flight(at[w] - h, 0)) / (2.0 * h));
    }
}

void test_listener_bounded_history(void)
{
    /* A path longer than the history is silence, never a wrapped, unrelated
     * event; one just inside it is the source as it was. */
    rb_scene_init(&sc, RB_SCENE_ASPHALT, FS, 343.0, 1, 0.06);
    only(&sc, 0, MOUTH, 0);
    double o[2];
    for (int i = 0; i < RB_SCENE_RING + 10; i++) {
        double in[RB_SCENE_SOURCES] = { 0.0 };
        in[MOUTH] = 1.0;   /* a full history */
        rb_scene_step(&sc, in, o);
    }
    sc.path[0][MOUTH][0].delay = RB_SCENE_RING + 5.25;
    double in[RB_SCENE_SOURCES] = { 0.0 };
    in[MOUTH] = 1.0;
    rb_scene_step(&sc, in, o);
    CHECK(o[0] == 0.0);
    sc.path[0][MOUTH][0].delay = RB_SCENE_RING - 4.5;
    rb_scene_step(&sc, in, o);
    CHECK_NEAR(o[0], sc.path[0][MOUTH][0].gain, 1e-9);
    sc.path[0][MOUTH][0].delay = -1.0;
    rb_scene_step(&sc, in, o);
    CHECK(o[0] == 0.0);
}

void test_listener_stereo_known_position(void)
{
    /* A known source, both ears: the interaural delay is the geometry's,
     * and the far ear's head shadow is its stated filter, (1 - 0.7 h) +
     * 0.7 h H(f), H a one-pole low-pass at c / (2 pi 0.09 m) (607 Hz),
     * measured in magnitude at 4 kHz within 0.05 dB. */
    for (int e = 0; e < 2; e++) {
        rb_scene_init(&sc, RB_SCENE_ASPHALT, FS, 343.0, 1, 0.06);
        for (int a = 0; a < 2; a++)
            for (int b = 0; b < RB_SCENE_PLACED; b++)
                for (int c = 0; c < 3; c++)
                    if (a != e || b != MOUTH || c != 0) sc.path[a][b][c].gain = 0.0;
        rb_scene_path *p = &sc.path[e][MOUTH][0];
        p->delay = floor(p->delay);
        const double h = 0.8, f = 4000.0;
        p->shadow = h;
        double a, ph;
        response(&sc, e, MOUTH, f, &a, &ph);
        const double k = sc.head_a, w = 2.0 * PI * f / FS;
        /* the one-pole y += k (x - y): H = k / (1 - (1 - k) e^-iw) */
        const double dr = 1.0 - (1.0 - k) * cos(w), di = (1.0 - k) * sin(w);
        const double hr = k * dr / (dr * dr + di * di), hi = -k * di / (dr * dr + di * di);
        const double gr = (1.0 - 0.7 * h) + 0.7 * h * hr, gi = 0.7 * h * hi;
        CHECK_NEAR(20.0 * log10(a / p->gain), 20.0 * log10(sqrt(gr * gr + gi * gi)), 0.05);
    }
    /* The ears' delays: the mouth from each, as the documented geometry
     * puts them (listener at (0.6, -3, 1.6), ears 0.09 m either side). */
    rb_scene_init(&sc, RB_SCENE_ASPHALT, FS, 343.0, 1, 0.06);
    for (int e = 0; e < 2; e++) {
        const double dx = -0.3 - (0.6 + (e ? 0.09 : -0.09)), dy = 3.7 + 3.0, dz = 0.75 - 1.6;
        CHECK_NEAR(sc.path[e][MOUTH][0].delay, sqrt(dx * dx + dy * dy + dz * dz) / 343.0 * FS, 1e-8);
    }
}

void test_listener_cabin_transfer(void)
{
    /* The body: behind through a one-pole at 100 Hz, in front at 200, both
     * at 0.3, then the cabin's boom (an RBJ peak, 8 dB at 55 Hz, Q 2.5).
     * Measured against those filters' own responses, within 0.02 dB, from
     * 20 Hz to 2 kHz. */
    const double freqs[6] = { 20.0, 55.0, 100.0, 200.0, 500.0, 2000.0 };
    for (int side = 0; side < 2; side++)
        for (int q = 0; q < 6; q++) {
            rb_cabin cb;
            memset(&cb, 0, sizeof cb);
            const double f = freqs[q], dt = 1.0 / FS, w = 2.0 * PI * f * dt;
            const int settle = 48000, count = (int)(lround(floor(0.5 * f)) / f * FS);
            double re = 0.0, im = 0.0;
            for (int i = 0; i < settle + count; i++) {
                const double x = sin(2.0 * PI * f * i * dt);
                const double y = rb_cabin_process(&cb, side ? 0.0 : x, side ? x : 0.0, 0.0, dt);
                if (i >= settle) {
                    re += y * sin(2.0 * PI * f * i * dt);
                    im += y * cos(2.0 * PI * f * i * dt);
                }
            }
            const double measured = 20.0 * log10(2.0 * sqrt(re * re + im * im) / count);
            const double corner = side ? RB_CABIN_FRONT_HZ : RB_CABIN_REAR_HZ;
            const double k = 1.0 - exp(-2.0 * PI * corner * dt);
            const double dr = 1.0 - (1.0 - k) * cos(w), di = (1.0 - k) * sin(w);
            const double pole = k / sqrt(dr * dr + di * di);
            /* the peak's response at w from its coefficients */
            const double A = pow(10.0, RB_CABIN_BOOM_DB / 40.0), w0 = 2.0 * PI * RB_CABIN_BOOM_HZ * dt;
            const double alpha = sin(w0) / (2.0 * RB_CABIN_BOOM_Q), cw = cos(w0);
            const double b0 = 1.0 + alpha * A, b1 = -2.0 * cw, b2 = 1.0 - alpha * A;
            const double a0 = 1.0 + alpha / A, a1 = -2.0 * cw, a2 = 1.0 - alpha / A;
            const double nr = b0 + b1 * cos(w) + b2 * cos(2 * w), ni = -b1 * sin(w) - b2 * sin(2 * w);
            const double mr = a0 + a1 * cos(w) + a2 * cos(2 * w), mi = -a1 * sin(w) - a2 * sin(2 * w);
            const double peak = sqrt((nr * nr + ni * ni) / (mr * mr + mi * mi));
            const double expected = 20.0 * log10(RB_CABIN_PASS * pole * peak);
            CHECK_NEAR(measured, expected, 0.02);
        }
}

void test_listener_eight_scenes(void)
{
    /* All eight scenes: each starts, each answers an impulse from every
     * source with finite sound that falls silent (but the rooms', which
     * decay), the ground's reflection in each outdoor one (grass softer), the
     * wall's in the wall scene alone, the room in the garage and on the dyno
     * alone. */
    CHECK_EQ_INT(RB_SCENE_COUNT, 8);
    for (int kind = 0; kind < RB_SCENE_COUNT; kind++) {
        CHECK(rb_scene_init(&sc, kind, FS, 343.0, 2, 0.06) == 0);
        const int roomy = kind == RB_SCENE_GARAGE || kind == RB_SCENE_DYNO;
        CHECK((sc.room_gain[0] > 0.0) == roomy);
        CHECK((sc.path[0][0][2].gain != 0.0) == (kind == RB_SCENE_WALL));
        CHECK(sc.path[0][0][1].gain > 0.0);
        if (kind == RB_SCENE_GRASS) {
            static rb_scene asphalt;
            rb_scene_init(&asphalt, RB_SCENE_ASPHALT, FS, 343.0, 2, 0.06);
            CHECK_NEAR(sc.path[0][0][1].gain / asphalt.path[0][0][1].gain, 0.35 / 0.9, 1e-12);
            rb_scene_init(&sc, kind, FS, 343.0, 2, 0.06);
        }
        double early = 0.0, late = 0.0;
        int finite = 1;
        for (int i = 0; i < 96000; i++) {
            double in[RB_SCENE_SOURCES] = { 0.0 }, o[2];
            if (i == 0) in[0] = in[1] = in[MOUTH] = in[3] = 1.0;
            rb_scene_step(&sc, in, o);
            finite &= isfinite(o[0]) && isfinite(o[1]);
            if (i < 24000) early += o[0] * o[0] + o[1] * o[1];   /* the pass-by's car starts 90 m off */
            else if (i > 72000) late += o[0] * o[0] + o[1] * o[1];
        }
        if (!(finite && early > 0.0)) printf("    scene %d: finite %d, early %g\n", kind, finite, early);
        CHECK(finite && early > 0.0);
        if (roomy) CHECK(late > 0.0 && late < 1e-3 * early);
        else if (kind != RB_SCENE_CABIN) CHECK(late < 1e-12 * early);
    }
}

static rb_listener mono;

void test_listener_mono_aperture(void)
{
    /* Stage 051 (accepted 2026-11-24): the mono listener hears each tailpipe as its
     * opening, as the stereo scene does. From the side place (4 m right of the middle, 2.2 m
     * ahead of the bumper) tailpipe 0's direct path is 59 degrees off the pipe's axis: its half
     * spread is D sin(theta) / 2c, computed here from the documented coordinates. A tone
     * through that path alone comes out at the Rayleigh disk's gain, 2 J1(x) / x with
     * x = w D sin(theta) / 2c (the 8-node quadrature within 0.04, the scene's tolerance); the
     * reads only lag, never lead. Against it, the same path read at a point (no spread). */
    const double D = 0.06, c = 343.0, pipe_dt = 1.0 / 48000.0, dt = 1.0 / 192000.0;
    rb_listener_place(&mono, RB_HEAR_SIDE, 2, c, pipe_dt);
    rb_listener_aperture(&mono, RB_HEAR_SIDE, 2, D, c, pipe_dt, dt);
    const double at[3] = { 4.0, 2.2, 1.6 }, src[3] = { 0.5, -0.05, 0.35 };
    const double dx = at[0] - src[0], dy = at[1] - src[1], dz = at[2] - src[2];
    const double r = sqrt(dx * dx + dy * dy + dz * dz), sine = sqrt(1.0 - dy * dy / (r * r));
    CHECK_NEAR(mono.spread[0][0], 0.5 * D * sine / c / pipe_dt, 1e-12);
    CHECK_NEAR(mono.nspread[0][0], 0.5 * D * sine / c / dt, 1e-12);
    CHECK(mono.lag >= mono.spread[0][0] && mono.lag >= mono.spread[1][1]);
    CHECK(mono.spread[0][0] > 1.0);   /* the side is well off the axis */

    /* one path alone: tailpipe 0, direct */
    for (int j = 0; j < RB_LISTEN_SOURCES; j++)
        for (int k = 0; k < 2; k++)
            if (j || k) mono.gain[j][k] = 0.0;
    const double f = 3000.0, w = 2.0 * PI * f;
    double amp[2];
    for (int on = 0; on < 2; on++) {
        /* off: the same listener with no opening (a point; no lag either,
         * which moves only the tone's phase) */
        rb_listener_aperture(&mono, RB_HEAR_SIDE, 2, on ? D : 0.0, c, pipe_dt, dt);
        double re = 0.0, im = 0.0;
        const int n = 48000;
        for (int i = 0; i < n; i++) {
            const double q[RB_LISTEN_SOURCES] = { sin(w * i * pipe_dt), 0.0, 0.0 };
            rb_listener_push(&mono, q);
            if (i >= 4800) {
                re += mono.sum * sin(w * i * pipe_dt);
                im += mono.sum * cos(w * i * pipe_dt);
            }
        }
        amp[on] = 2.0 * sqrt(re * re + im * im) / (n - 4800);
    }
    const double x = w * 0.5 * D * sine / c;
    double term = 1.0, airy = 1.0;
    for (int k = 1; k < 20; k++) {
        term *= (-x * x / 4.0) / (k * (k + 1));
        airy += term;
    }
    printf("    from the side, 3 kHz: the opening's gain %.3f, the disk's %.3f\n", amp[1] / amp[0], airy);
    CHECK(fabs(amp[1] / amp[0] - fabs(airy)) < 0.04);
    CHECK(amp[1] < 0.97 * amp[0]);

    /* the sim places the openings with its listener, in every variant, at every place */
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    float out[256];
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    for (int v = 0; v < 3; v++) {
        rb_set_param(sim, RB_PARAM_VARIANT, v);
        rb_set_param(sim, RB_PARAM_LISTENER, v ? RB_LISTENER_SIDE : RB_LISTENER_BEHIND);
        rb_render_block(sim, out, 128);
        CHECK(sim->listener.lag >= 1.0 && sim->listener.nlag >= 1.0);
        if (v) CHECK(sim->listener.spread[0][0] > 0.1);
    }
    rb_destroy(sim);
}
