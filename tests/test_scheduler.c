/*
 * test_scheduler.c - stage 052: the mixed-rate scheduler and its transitions.
 *
 * The engine steps its cylinders, valves and the pipes' end cells at the
 * physics rate (192 kHz by default) and the rest of the pipes at the pipe
 * rate (48 kHz), every pipe_every steps (engine.c, pipe.h's two rates). The
 * pipes' cells are sized for their rate up to 48 kHz and no larger, so pipes
 * at 24 and 48 kHz share one grid: comparing them changes only the
 * scheduling, nothing the grid resolves. The common-clock control (pipes on
 * the physics clock) would change the grid as well (cells a quarter as long):
 * that comparison is the sound's, stage S2c's and 062's.
 *
 * And the sim around the engine: whatever the host's block size, the same
 * sound, sample for sample, through a throttle step, a live setting and a
 * rebuild.
 */
#include "engine.h"
#include "presets.h"
#include "rb.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const double PI = 3.14159265358979323846;
static rb_engine e;

#define DT_FAST (1.0 / 192000.0)
#define WINDOW 38400   /* steps: 0.2 s, ten cycles at 3000 rpm */

static double flow[WINDOW], angle[WINDOW];

/* The four held at 3000 rpm wide open with pipes at `pipe_rate`: settled,
 * then ten cycles of its tailpipe's volume flow against the crank's angle.
 * Returns pipe_every. */
static int held_four(double pipe_rate, double result[3])
{
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i4"), &d);
    CHECK_EQ_INT(rb_engine_init_at(&e, &d, DT_FAST, pipe_rate), 0);
    e.rpm = 3000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-5);
    result[0] = e.result.torque;
    result[1] = e.result.ve;
    result[2] = e.result.imep_net;
    double turned = 0.0;
    for (int i = 0; i < WINDOW; i++) {
        const double before = e.theta;
        rb_engine_step(&e);
        double step = e.theta - before;
        if (step < 0.0) step += 720.0;
        turned += step;
        flow[i] = e.tail_flow[0];
        angle[i] = turned;
    }
    return e.pipe_every;
}

/* The flow's order `order` of the cycle over its whole cycles, as a phasor. */
static void order_of(int order, double *re, double *im)
{
    const double whole = floor(angle[WINDOW - 1] / 720.0) * 720.0;
    *re = *im = 0.0;
    for (int i = 0; i < WINDOW && angle[i] <= whole; i++) {
        const double a = 2.0 * PI * order * angle[i] / 720.0;
        *re += flow[i] * cos(a);
        *im -= flow[i] * sin(a);
    }
}

void test_scheduler_pipe_rates_agree_on_one_grid(void)
{
    /* Pipes at 48 kHz (every 4 physics steps) and at 24 kHz (every 8), on the
     * same cells: the work and the breathing within 0.15 % (measured 0.07 %),
     * and the tailpipe's strong orders (2, 4, 8, 16: 50 to 400 Hz) within
     * 0.5 dB and 0.03 ms of each other (measured 0.39 dB and 0.021 ms). The
     * time is what matters for the delay: a slow step's flows are its
     * average, centred half a step back, so 24 kHz runs about half a 48 kHz
     * step later at most. One more pipe step anywhere in the loop (a value
     * held a step too long: the "inserted feedback sample") puts the 24 kHz
     * run 0.021 ms further behind, past the tolerance. */
    static double fast[WINDOW][2];
    double r48[3], r24[3];
    CHECK_EQ_INT(held_four(48000.0, r48), 4);
    for (int i = 0; i < WINDOW; i++) {
        fast[i][0] = flow[i];
        fast[i][1] = angle[i];
    }
    CHECK_EQ_INT(held_four(24000.0, r24), 8);
    for (int k = 0; k < 3; k++) CHECK_NEAR(r24[k], r48[k], 1.5e-3 * fabs(r48[k]));

    static const int ORDERS[4] = { 2, 4, 8, 16 };
    double worst_db = 0.0, worst_ms = 0.0;
    for (int j = 0; j < 4; j++) {
        double re24, im24, re48, im48;
        order_of(ORDERS[j], &re24, &im24);
        for (int i = 0; i < WINDOW; i++) {   /* the 48 kHz run's, from its copy */
            const double f = flow[i], a = angle[i];
            flow[i] = fast[i][0];
            angle[i] = fast[i][1];
            fast[i][0] = f;
            fast[i][1] = a;
        }
        order_of(ORDERS[j], &re48, &im48);
        for (int i = 0; i < WINDOW; i++) {   /* swapped back */
            const double f = flow[i], a = angle[i];
            flow[i] = fast[i][0];
            angle[i] = fast[i][1];
            fast[i][0] = f;
            fast[i][1] = a;
        }
        const double db = 20.0 * log10(hypot(re24, im24) / hypot(re48, im48));
        double phase = atan2(im24, re24) - atan2(im48, re48);
        while (phase > PI) phase -= 2.0 * PI;
        while (phase < -PI) phase += 2.0 * PI;
        const double ms = -phase / (2.0 * PI) / (ORDERS[j] * 3000.0 / 120.0) * 1000.0;
        if (fabs(db) > worst_db) worst_db = fabs(db);
        if (fabs(ms) > worst_ms) worst_ms = fabs(ms);
    }
    printf("    pipes at 24 and 48 kHz: work within %.3f %%, orders within %.2f dB and %.4f ms\n",
           100.0 * fabs(r24[0] / r48[0] - 1.0), worst_db, worst_ms);
    CHECK(worst_db <= 0.5);
    CHECK(worst_ms <= 0.03);
}

/* ---- the sim: block sizes and transitions ---- */

enum { STRIDE = 896,            /* 128 x 7: a boundary for both block sizes */
       CHANGE1 = 30 * STRIDE,   /* the throttle shuts and the spark moves   */
       CHANGE2 = 50 * STRIDE,   /* a rebuild: a longer tailpipe             */
       FRAMES = 80 * STRIDE };  /* 1.5 s */

/* The four on the sim, held at 2500 rpm wide open, rendered in blocks of
 * `block`, with the changes at their frames. */
static void render_with_changes(float *out, int block)
{
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i4")), RB_OK);
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 2500);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1);
    const int tailpipe = rb_engine_field_find("tailpipe_length");
    CHECK(tailpipe >= 0);
    for (int done = 0; done < FRAMES;) {
        if (done == CHANGE1) {
            rb_set_param(sim, RB_PARAM_THROTTLE, 0.2);
            rb_set_param(sim, RB_PARAM_SPARK_OFFSET, -5.0);
        }
        if (done == CHANGE2) CHECK(rb_set_engine(sim, tailpipe, 0, 0.9) >= 0);
        int n = block;
        if (done < CHANGE1 && done + n > CHANGE1) n = CHANGE1 - done;
        if (done < CHANGE2 && done + n > CHANGE2) n = CHANGE2 - done;
        if (done + n > FRAMES) n = FRAMES - done;
        CHECK_EQ_INT(rb_render_block(sim, out + done, n), n);
        done += n;
    }
    rb_destroy(sim);
}

void test_scheduler_blocks_and_transitions_are_seamless(void)
{
    /* The host's block size never changes the sound: in blocks of 128 and of
     * 7 frames, through a throttle step and a spark change (live) and a
     * rebuild (a longer tailpipe), sample for sample the same. The rebuild
     * starts a new engine (no state is carried across: the host fades
     * between them, runtime-v1), so there is no physical state transfer for
     * a fade to hide. */
    float *a = malloc(sizeof(float) * FRAMES), *b = malloc(sizeof(float) * FRAMES);
    CHECK(a && b);
    if (a && b) {
        render_with_changes(a, 128);
        render_with_changes(b, 7);
        CHECK(memcmp(a, b, sizeof(float) * FRAMES) == 0);
        double before = 0.0, after = 0.0;
        for (int i = CHANGE1 - 4800; i < CHANGE1; i++) before += (double)a[i] * a[i];
        for (int i = CHANGE2 - 4800; i < CHANGE2; i++) after += (double)a[i] * a[i];
        CHECK(before > 0.0 && after > 0.0 && after < before);   /* the changes took */
    }
    free(a);
    free(b);
}
