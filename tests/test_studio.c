/*
 * test_studio.c - stage 148: the studio quality (RB_QUALITY_STUDIO), the
 * pipes at 192 kHz and their cells a quarter of the high quality's, for
 * offline renders.
 *
 * Every preset fits it without its cells being squeezed (the limits raised:
 * RB_ENGINE_CELLS, RB_PIPE_MAX_CELLS, RB_PIPE_RATE_MAX), its engines play,
 * and the finer grid carries the gas's own sound far higher than the high
 * quality's: 144's grid study, here as a check that the grid is refined at all.
 */
#include "rb.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const double PI = 3.14159265358979323846;
#define RATE 48000

void test_studio_cells_a_quarter(void)
{
    /* Each preset at high and at studio: every pipe of 3 cells or more at
     * high has about four times as many at studio (the rounding of each
     * pipe's length to whole cells: within 4), none at the most a pipe can
     * have; so no pipe's cells were squeezed to fit the pool. */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_sim *high = rb_create_with_quality(RATE, RB_QUALITY_HIGH);
        rb_sim *studio = rb_create_with_quality(RATE, RB_QUALITY_STUDIO);
        CHECK(high != NULL && studio != NULL);
        if (!high || !studio) {
            rb_destroy(high);
            rb_destroy(studio);
            return;
        }
        CHECK_EQ_INT(rb_load_preset(high, p), RB_OK);
        CHECK_EQ_INT(rb_load_preset(studio, p), RB_OK);
        CHECK_NEAR(rb_pipe_rate(studio), 192000.0, 1e-6);
        CHECK_EQ_INT(rb_pipe_count(high), rb_pipe_count(studio));
        int total = 0, bad = 0;
        for (int i = 0; i < rb_pipe_count(high); i++) {
            const int a = (int)rb_get_pipe(high, i, RB_PIPE_CELLS);
            const int b = (int)rb_get_pipe(studio, i, RB_PIPE_CELLS);
            total += b;
            if (b >= 256) bad++;   /* RB_PIPE_MAX_CELLS: capped */
            if (a >= 3 && abs(b - 4 * a) > 4) {
                printf("        %s pipe %d: %d cells at high, %d at studio\n", rb_preset_key(p), i, a, b);
                bad++;
            }
        }
        CHECK_EQ_INT(bad, 0);
        CHECK(total <= 2048);   /* RB_ENGINE_CELLS */
        rb_destroy(high);
        rb_destroy(studio);
    }

    /* A pipe longer than any preset's (the four's exhaust pipe at 3 m, the
     * editor's): its 40 cells at high are 160 at studio, more than the 64 a
     * pipe could have before. */
    int found = 0;
    for (int q = 0; q < 2; q++) {
        rb_sim *sim = rb_create_with_quality(RATE, q ? RB_QUALITY_STUDIO : RB_QUALITY_HIGH);
        CHECK(sim != NULL);
        if (!sim) return;
        rb_load_preset(sim, rb_preset_find("i4"));
        CHECK(rb_set_engine(sim, RB_ENG_EXHAUST_PIPE_LENGTH, 0, 3.0) >= RB_OK);
        for (int i = 0; i < rb_pipe_count(sim); i++)
            if (fabs(rb_get_pipe(sim, i, RB_PIPE_LENGTH) - 3.0) < 1e-9) {
                CHECK_EQ_INT((int)rb_get_pipe(sim, i, RB_PIPE_CELLS), q ? 160 : 40);
                found++;
            }
        rb_destroy(sim);
    }
    CHECK_EQ_INT(found, 2);
}

void test_studio_plays_the_largest_engines(void)
{
    /* The presets with the most pipe (the cross-plane V8, the 458) start and
     * run on their own at studio: a finite, audible sound. */
    static const char *const KEYS[2] = { "v8-cross", "ferrari-458" };
    static float out[RATE / 10];
    for (int k = 0; k < 2; k++) {
        rb_sim *sim = rb_create_with_quality(RATE, RB_QUALITY_STUDIO);
        CHECK(sim != NULL);
        if (!sim) return;
        CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find(KEYS[k])), RB_OK);
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        rb_set_param(sim, RB_PARAM_HOLD, 1);
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
        double sum = 0.0;
        int finite = 1;
        for (int block = 0; block < 3; block++) {
            rb_render_block(sim, out, RATE / 10);
            for (int i = 0; i < RATE / 10; i++) {
                if (!isfinite(out[i])) finite = 0;
                if (block == 2) sum += (double)out[i] * out[i];
            }
        }
        CHECK(finite);
        CHECK(sqrt(sum / (RATE / 10)) > 1e-4);
        rb_destroy(sim);
    }
}

/* The power of x (n samples at RATE) in [lo, hi) Hz, a Hann window, a DFT
 * bin by bin. */
static double band_power(const float *x, int n, double lo, double hi)
{
    const double df = (double)RATE / n;
    double sum = 0.0;
    for (int k = (int)ceil(lo / df); k * df < hi; k++) {
        double re = 0.0, im = 0.0;
        for (int i = 0; i < n; i++) {
            const double w = (0.5 - 0.5 * cos(2.0 * PI * i / n)) * x[i];
            re += w * cos(2.0 * PI * k * i / n);
            im -= w * sin(2.0 * PI * k * i / n);
        }
        sum += re * re + im * im;
    }
    return sum;
}

/* The four held at 5000 rpm wide open, the pipes' sound alone a metre behind
 * its tailpipe, no variation: 2.5-4 kHz against 100-500 Hz (dB). */
static double highs(int quality)
{
    enum { N = RATE / 8 };
    static float out[N];
    rb_sim *sim = rb_create_with_quality(RATE, quality);
    if (!sim) return NAN;
    rb_load_preset(sim, rb_preset_find("i4"));
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 5000.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_VARIATION, 0.0);
    rb_set_param(sim, RB_PARAM_LAYERS, RB_LAYER_PIPES);
    rb_set_param(sim, RB_PARAM_LISTENER, RB_LISTENER_TAILPIPE);
    for (int block = 0; block < 5; block++) rb_render_block(sim, out, N);   /* 0.625 s to settle */
    rb_render_block(sim, out, N);
    rb_destroy(sim);
    return 10.0 * log10(band_power(out, N, 2500.0, 4000.0) / band_power(out, N, 100.0, 500.0));
}

void test_studio_carries_the_gas_higher(void)
{
    /* 144's grid study: at 2.5-4 kHz the high quality's grid takes away
     * 20-48 dB of what studio's carries (docs/stage-148.md). Here: at least
     * 10 dB more of it, against the same low band. */
    const double high = highs(RB_QUALITY_HIGH), studio = highs(RB_QUALITY_STUDIO);
    printf("        2.5-4 kHz against 100-500 Hz: high %.1f dB, studio %.1f dB\n", high, studio);
    CHECK(studio > high + 10.0);
}

/* The four held at 3000 rpm wide open at a quality and variant, the flow's
 * noise alone a metre behind its tailpipe: N samples after 0.375 s. */
enum { NOISE_N = RATE / 8 };
static void flow_noise(int quality, int variant, float *out)
{
    rb_sim *sim = rb_create_with_quality(RATE, quality);
    if (!sim) return;
    rb_load_preset(sim, rb_preset_find("i4"));
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_VARIATION, 0.0);
    rb_set_param(sim, RB_PARAM_LAYERS, RB_LAYER_FLOW);
    rb_set_param(sim, RB_PARAM_LISTENER, RB_LISTENER_TAILPIPE);
    rb_set_param(sim, RB_PARAM_VARIANT, variant);
    for (int block = 0; block < 3; block++) rb_render_block(sim, out, NOISE_N);
    rb_render_block(sim, out, NOISE_N);
    rb_destroy(sim);
}

void test_studio_rebalance_only_where_the_gas_carries(void)
{
    /* Candidate 7: on studio's grid (its edge 3.2 kHz) the valves' noise is
     * taken away below the edge (1-1.6 kHz at least 10 dB down: two
     * first-order high-passes, an octave and more below) and kept above it
     * (10-14 kHz, three times the edge and more, within 1.5 dB: the
     * hand-over as gradual as the grid's own loss); on high's grid it is the
     * accepted sound, bit for bit. */
    static float a[NOISE_N], b[NOISE_N];
    flow_noise(RB_QUALITY_STUDIO, 0, a);
    flow_noise(RB_QUALITY_STUDIO, 7, b);
    const double low = 10.0 * log10(band_power(b, NOISE_N, 1000.0, 1600.0) / band_power(a, NOISE_N, 1000.0, 1600.0));
    const double high = 10.0 * log10(band_power(b, NOISE_N, 10000.0, 14000.0) / band_power(a, NOISE_N, 10000.0, 14000.0));
    printf("        studio, candidate 7 against 0: 1-1.6 kHz %.1f dB, 10-14 kHz %.1f dB\n", low, high);
    CHECK(low < -10.0);
    CHECK(fabs(high) < 1.5);
    flow_noise(RB_QUALITY_HIGH, 0, a);
    flow_noise(RB_QUALITY_HIGH, 7, b);
    int same = 1;
    for (int i = 0; i < NOISE_N; i++)
        if (a[i] != b[i]) same = 0;
    CHECK(same);
}
