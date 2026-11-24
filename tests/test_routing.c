/*
 * test_routing.c - stage 054: the exhaust's excitation routed through one
 * geometry. Same-state captures: two engines run identically, one gets a
 * small pulse in a cylinder's header, and their difference at each outlet is
 * the resolved layer's response from that port, through the running gas.
 * Against it, the noise layer's routing (noise.c), its fixed paths at
 * 580 m/s. Stage 054's candidate (RB_PARAM_VARIANT 3: live path times and
 * the reversed jets heard) lost Bernardo's A/B on 2026-11-24 and was deleted
 * with its tests; its number is retired.
 */
#include "engine.h"
#include "noise.h"
#include "presets.h"
#include "rb.h"
#include "rb_internal.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DT (1.0 / 192000.0)
#define CAPTURE 9600   /* 50 ms */

static rb_engine a, b, c2, d2;
static rb_noise n;
static double response[3][RB_MAX_COLLECTORS][CAPTURE];

typedef void (*edit_fn)(rb_engine_desc *d);

/* Engines x (and y, z if given) built alike from preset `key` (edited), run
 * to the same state at rpm and throttle, then on until cylinder `cyl` is at
 * 600 degrees of its cycle (its exhaust valve shut, its intake's closing). */
static void same_state(const char *key, edit_fn edit, double rpm, double throttle, int cyl,
                       rb_engine *x, rb_engine *y, rb_engine *z, rb_engine *w)
{
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find(key), &d);
    if (edit) edit(&d);
    rb_engine *all[4] = { x, y, z, w };
    for (int j = 0; j < 4; j++) {
        if (!all[j]) continue;
        rb_engine_init(all[j], &d, DT);
        all[j]->rpm = rpm;
        all[j]->throttle_pos = throttle;
        all[j]->ignition = 1;
        rb_engine_settle(all[j], 60, 1e-4);
    }
    for (int s = 0; s < 400000 && fabs(rb_engine_cylinder_angle(x, cyl) - 600.0) > 2.0; s++)
        for (int j = 0; j < 4; j++)
            if (all[j]) rb_engine_step(all[j]);
}

/* A 300 Pa isentropic squeeze in the first cell of cylinder i's header. */
static void pulse(rb_engine *e, int i)
{
    rb_chamber *c = &e->header[i].cell[0];
    const double g = rb_chamber_gamma(c), p1 = c->p + 300.0;
    double y[RB_NSPECIES];
    memcpy(y, c->y, sizeof y);
    rb_chamber_init(c, c->V, p1, c->T * pow(p1 / c->p, (g - 1.0) / g), y);
}

/* Steps the reference x and the pulsed ys together for CAPTURE steps: each
 * y's tailpipe flows less x's into response[k]. */
static void capture(rb_engine *x, rb_engine **ys, int count)
{
    for (int s = 0; s < CAPTURE; s++) {
        rb_engine_step(x);
        for (int k = 0; k < count; k++) {
            rb_engine_step(ys[k]);
            for (int q = 0; q < x->systems; q++) response[k][q][s] = ys[k]->tail_flow[q] - x->tail_flow[q];
        }
    }
}

/* The noise layer attached to x now. */
static void noise_on(rb_engine *x)
{
    rb_noise_init(&n, x, 1);
    rb_noise_step(&n, x, RB_NOISE_FLOW);
}

void test_routing_source_sum(void)
{
    /* Two sources at once are the sum of each alone: pulses in cylinders 1
     * and 4 together against each alone, at the tailpipe, within 1 % of the
     * peak (the pulses are small: linear). */
    same_state("i4", NULL, 3000.0, 1.0, 0, &a, &b, &c2, &d2);
    pulse(&b, 0);
    pulse(&c2, 3);
    pulse(&d2, 0);
    pulse(&d2, 3);
    rb_engine *ys[3] = { &b, &c2, &d2 };
    capture(&a, ys, 3);
    double worst = 0.0, most = 0.0;
    for (int s = 0; s < CAPTURE; s++) {
        most = fmax(most, fabs(response[2][0][s]));
        worst = fmax(worst, fabs(response[2][0][s] - response[0][0][s] - response[1][0][s]));
    }
    printf("    the two together against the sum of each: %.2e of the peak\n", worst / most);
    CHECK(worst < 0.01 * most);
}

/* The response's energy from 800 Hz to 3 kHz (the valves' noise band). */
static double band(int k)
{
    double sum = 0.0;
    for (double f = 800.0; f <= 3000.0; f += 50.0) {
        double re = 0.0, im = 0.0;
        for (int s = 0; s < CAPTURE; s++) {
            re += response[k][0][s] * cos(2.0 * PI * f * s * DT);
            im += response[k][0][s] * sin(2.0 * PI * f * s * DT);
        }
        sum += re * re + im * im;
    }
    return sum;
}

static void no_catalyst(rb_engine_desc *d) { d->catalyst_laminar = 0.0; }
static void no_muffler(rb_engine_desc *d) { d->muffler_type = RB_MUFFLER_NONE; }
static void straight(rb_engine_desc *d) { d->muffler_type = RB_MUFFLER_STRAIGHT; }

void test_routing_component_controls(void)
{
    /* Bypass controls, the components in the noise's band (800 Hz-3 kHz),
     * each layer against itself: the pulses' energy through the four's
     * exhaust with each component as built and bypassed, and the noise's
     * pass (noise.c's PASS_*). Both layers lose with each component; their
     * amounts differ (stated in docs/stage-054.md: the pipes' cans and
     * packing are grid-limited above 450 Hz-1.1 kHz, stage 050). */
    edit_fn edits[4] = { NULL, no_catalyst, no_muffler, straight };
    double pulses[4], pass[4];
    for (int j = 0; j < 4; j++) {
        same_state("i4", edits[j], 3000.0, 1.0, 0, &a, &b, NULL, NULL);
        noise_on(&a);
        pass[j] = n.blow_pass;
        pulse(&b, 0);
        rb_engine *ys[1] = { &b };
        capture(&a, ys, 1);
        pulses[j] = band(0);
    }
    const char *names[3] = { "the catalyst", "the chambered can", "a straight-through" };
    const int with[3] = { 0, 0, 3 }, without[3] = { 1, 2, 2 };
    for (int k = 0; k < 3; k++) {
        const double resolved = 10.0 * log10(pulses[without[k]] / pulses[with[k]]);
        const double noise = 10.0 * log10(pass[without[k]] / pass[with[k]]);
        printf("    %-19s takes %5.1f dB of the pulses, %5.1f of the noise\n", names[k], resolved, noise);
        CHECK(resolved > 0.3 && noise > 0.3);
    }
}

void test_routing_no_duplicated_pulse(void)
{
    /* The valves' noise is the jets' turbulence, not their pulses: at each
     * tailpipe it is uncorrelated with the resolved flow's swings (|r| under
     * 0.05 over 0.25 s). */
    same_state("i4", NULL, 3000.0, 1.0, 0, &a, NULL, NULL, NULL);
    {
        rb_noise_init(&n, &a, 9);
        double sxy = 0.0, sxx = 0.0, syy = 0.0, last = a.tail_flow[0];
        for (int s = 0; s < 48000; s++) {
            rb_engine_step(&a);
            rb_noise_step(&n, &a, RB_NOISE_FLOW);
            const double x = n.out[RB_NOISE_TAIL0], y = a.tail_flow[0] - last;
            last = a.tail_flow[0];
            sxy += x * y;
            sxx += x * x;
            syy += y * y;
        }
        const double r = sxy / sqrt(sxx * syy);
        printf("    the noise against the flow's swings, r = %+.4f\n", r);
        CHECK(fabs(r) < 0.05);
    }
}
