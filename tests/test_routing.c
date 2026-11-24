/*
 * test_routing.c - stage 054: the exhaust's excitation routed through one
 * geometry. Same-state captures: two engines run identically, one gets a
 * small pulse in a cylinder's header, and their difference at each outlet is
 * the resolved layer's response from that port, through the running gas.
 * Against it, the noise layer's routing (noise.c): today's fixed paths at
 * 580 m/s, and stage 054's candidate (RB_PARAM_VARIANT 3), whose path times
 * follow the cells' temperature and flow to each outlet the cylinder reaches.
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

/* When response k at outlet q arrives (s): when it first reaches a tenth
 * of its largest swing, the wave's front. Not much lower: the solver's
 * numerical diffusion sends a faint precursor ahead of the wave (at a
 * thousandth the four's front comes 0.5 ms early). Not its peak: the cans
 * smear the pulse's low frequencies over a millisecond. Not its noise band
 * alone: there the cans ring with their grid-limited modes (stage 050)
 * louder than the direct wave. Its largest swing into *peak. */
static double arrival(int k, int q, double *peak)
{
    double most = 0.0;
    for (int s = 0; s < CAPTURE; s++) most = fmax(most, fabs(response[k][q][s]));
    *peak = most;
    for (int s = 0; s < CAPTURE; s++)
        if (fabs(response[k][q][s]) > 0.1 * most) return s * DT;
    return -1.0;
}

/* The noise layer attached to x now, its routing worked out (variant 3). */
static void noise_on(rb_engine *x, int variant)
{
    rb_noise_init(&n, x, 1);
    n.variant = variant;
    rb_noise_step(&n, x, RB_NOISE_FLOW);
}

void test_routing_path_times_follow_the_gas(void)
{
    /* All sources, all outlets. Each cylinder's pulse arrives at each
     * outlet when the candidate's path time says, within 0.3 ms (0.12 on
     * the four and the boxer; the V8's X puts the noise at its two centre
     * pipes' mean) and within half today's error, wide open and idling, on
     * the four, the cross-plane V8 and the boxer; and it reaches exactly the
     * outlets the noise is sent to. Today's fixed 580 m/s is 0.6-0.7 ms off
     * either way (late wide open, early idling): printed. */
    static const struct { const char *key; double rpm, throttle; int cylinders; } CASES[] = {
        { "i4", 3000.0, 1.0, 4 }, { "i4", 850.0, 0.04, 4 },
        { "v8-cross", 3000.0, 1.0, 4 }, { "boxer4", 3000.0, 1.0, 2 },
    };
    for (int k = 0; k < 4; k++) {
        double worst = 0.0, fixed = 0.0;
        for (int i = 0; i < CASES[k].cylinders; i++) {
            same_state(CASES[k].key, NULL, CASES[k].rpm, CASES[k].throttle, i, &a, &b, NULL, NULL);
            noise_on(&a, 3);
            pulse(&b, i);
            rb_engine *ys[1] = { &b };
            capture(&a, ys, 1);
            double loudest = 0.0, peak[RB_MAX_COLLECTORS];
            for (int q = 0; q < a.systems; q++) {
                arrival(0, q, &peak[q]);
                loudest = fmax(loudest, peak[q]);
            }
            for (int q = 0; q < a.systems; q++) {
                const int reached = peak[q] > 0.01 * loudest;
                CHECK_EQ_INT(n.blow_share[i][q] > 0.0, reached);
                if (!reached) continue;
                double p;
                const double t = arrival(0, q, &p);
                worst = fmax(worst, fabs(n.route_delay[i][q] * DT - t));
                fixed = fmax(fixed, fabs(n.blow_delay[i] * DT - t));
            }
        }
        printf("    %-8s %4.0f rpm: the candidate's path times within %.3f ms of the pulses; today's fixed, %.3f ms\n",
               CASES[k].key, CASES[k].rpm, worst * 1e3, fixed * 1e3);
        CHECK(worst < 0.3e-3);
        CHECK(worst < 0.5 * fixed);
    }
    /* The paths follow the gas as it changes: attached idling, then 0.3 s
     * wide open, the candidate's path times are a freshly worked-out set's
     * within 3 steps (the gas moves on for at most 256 between them). */
    same_state("i4", NULL, 850.0, 0.04, 0, &a, NULL, NULL, NULL);
    static rb_noise fresh;
    noise_on(&a, 3);
    a.rpm = 3000.0;
    a.throttle_pos = 1.0;
    for (int s = 0; s < 57600; s++) {
        rb_engine_step(&a);
        rb_noise_step(&n, &a, RB_NOISE_FLOW);
    }
    rb_noise_init(&fresh, &a, 1);
    fresh.variant = 3;
    rb_noise_step(&fresh, &a, RB_NOISE_FLOW);
    int drift = 0;
    for (int i = 0; i < a.desc.cylinders; i++)
        drift = (int)fmax(drift, abs(n.route_delay[i][0] - fresh.route_delay[i][0]));
    printf("    idle to wide open: the paths %d steps from a fresh set's\n", drift);
    CHECK(drift <= 3);
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

static void longer_tailpipe(rb_engine_desc *d) { d->tailpipe_length += 0.5; }

void test_routing_geometry_edit(void)
{
    /* Half a metre more tailpipe moves the pulse and the candidate's noise
     * alike, within a tenth of the move (today's fixed path, 12 %). */
    double moved[2][2];
    for (int j = 0; j < 2; j++) {
        same_state("i4", j ? longer_tailpipe : NULL, 3000.0, 1.0, 0, &a, &b, NULL, NULL);
        noise_on(&a, 3);
        pulse(&b, 0);
        rb_engine *ys[1] = { &b };
        capture(&a, ys, 1);
        double p;
        moved[j][0] = arrival(0, 0, &p);
        moved[j][1] = n.route_delay[0][0] * DT;
    }
    const double pulse_shift = moved[1][0] - moved[0][0], noise_shift = moved[1][1] - moved[0][1];
    printf("    0.5 m more tailpipe: the pulse %.3f ms later, the candidate's noise %.3f ms, today's %.3f\n",
           pulse_shift * 1e3, noise_shift * 1e3, 0.5 / 580.0 * 1e3);
    CHECK(pulse_shift > 0.5e-3);
    CHECK(fabs(noise_shift - pulse_shift) < 0.1 * pulse_shift);
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
        noise_on(&a, 0);
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

void test_routing_reverse_jets_heard(void)
{
    /* The candidate hears the reversed jets: idling, the cylinders' gas
     * blowing back into the runners (made in a pipe, heard at the mouth with
     * the intake's roar, at the exhaust valves' efficiency: 3 dB and more
     * louder there) and the headers' back into the cylinders (made in the
     * cylinder, heard back through the gap as the intake valves' forward jets
     * are: a few per cent of the exhaust's noise, the tailpipe within 1.5 dB,
     * its own draws a new realisation of the noise). The accepted sound
     * hears neither; the same gas either way. */
    same_state("i4", NULL, 850.0, 0.04, 0, &a, NULL, NULL, NULL);
    static rb_noise m;
    rb_noise_init(&n, &a, 5);
    rb_noise_init(&m, &a, 5);
    n.variant = 3;
    double reverse = 0.0, forward = 0.0, mouth[2] = { 0.0, 0.0 }, tail[2] = { 0.0, 0.0 };
    const int cycle = (int)(120.0 / 850.0 / DT);
    for (int s = 0; s < cycle; s++) {
        rb_engine_step(&a);
        rb_noise_step(&n, &a, RB_NOISE_FLOW);
        rb_noise_step(&m, &a, RB_NOISE_FLOW);
        CHECK(m.reverse_power == 0.0);
        reverse += n.reverse_power;
        forward += n.intake_power;
        CHECK(n.intake_power == m.intake_power);
        mouth[0] += m.out[RB_NOISE_MOUTH] * m.out[RB_NOISE_MOUTH];
        mouth[1] += n.out[RB_NOISE_MOUTH] * n.out[RB_NOISE_MOUTH];
        tail[0] += m.out[RB_NOISE_TAIL0] * m.out[RB_NOISE_TAIL0];
        tail[1] += n.out[RB_NOISE_TAIL0] * n.out[RB_NOISE_TAIL0];
    }
    printf("    idling: the reversed intake jets %.3g times the forward; the mouth %+.1f dB, the tailpipe %+.1f dB\n",
           reverse / forward, 10.0 * log10(mouth[1] / mouth[0]), 10.0 * log10(tail[1] / tail[0]));
    CHECK(reverse > 1e3 * forward);
    CHECK(mouth[1] > 2.0 * mouth[0]);
    CHECK(fabs(10.0 * log10(tail[1] / tail[0])) < 1.5);
    /* ... and they were heard: each reversed exhaust jet draws its own noise, so the two
     * generators part (the reversed intake jets join the roar's draws and add none) */
    CHECK(n.state != m.state);
}

void test_routing_no_duplicated_pulse(void)
{
    /* The valves' noise is the jets' turbulence, not their pulses: at each
     * tailpipe it is uncorrelated with the resolved flow's swings (|r| under
     * 0.05 over 0.25 s), in the accepted sound and the candidate. */
    same_state("i4", NULL, 3000.0, 1.0, 0, &a, NULL, NULL, NULL);
    for (int v = 0; v < 2; v++) {
        rb_noise_init(&n, &a, 9);
        n.variant = v ? 3 : 0;
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
        printf("    variant %d: the noise against the flow's swings, r = %+.4f\n", n.variant, r);
        CHECK(fabs(r) < 0.05);
    }
}
