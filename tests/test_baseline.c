/*
 * test_baseline.c - stage 057: the accepted engine (A0) as the baseline any
 * research solver must beat in the real five-preset engine.
 *   - Where today's exhaust waves are weak: each plain exhaust pipe's own
 *     numerical loss, on its own cells, bore and running gas temperature.
 *   - The whole drive on the free crank (start, idle, held load, pull, the
 *     limiter, the overrun's fuel cut, its resume, idle again) with the
 *     engine's mass and energy books closing and its guards quiet.
 *   - A geometry edit downstream returns its load upstream.
 * No research solver reaches the engine yet (docs/stage-057.md): these are the
 * gates one must pass there.
 */
#include "engine.h"
#include "pipe.h"
#include "presets.h"
#include "rb.h"
#include "rb_internal.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846
#define P0 101325.0
#define PIPE_DT (1.0 / 48000.0)   /* the engine's pipe step */
#define STEPS 6000                /* 0.125 s */

static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
static rb_engine e;

/* ---- where the waves are weak ---- */

#define CHAIN 8
static rb_chamber cells[CHAIN][RB_PIPE_MAX_CELLS];
static rb_face faces[CHAIN][RB_PIPE_MAX_CELLS];
static rb_pipe chain[CHAIN];
static double near_rec[STEPS], far_rec[STEPS];

/* A still, frictionless, wall-neutral pipe of n cells of dx at T, bore d. */
static rb_pipe *lay(int k, int n, double dx, double d, double T)
{
    rb_pipe_init(&chain[k], cells[k], faces[k], n, n * dx, d, P0, T, AIR, T, 0.0);
    return &chain[k];
}

/* The numerical loss of a plain pipe of cell dx, bore d, gas at T (dB per
 * metre) at each of `count` frequencies: a one-cell pulse runs along two
 * 48-cell pipes between packed sponges; the spectra at two cells 72 apart
 * give the loss between them. Friction and the walls' heat are off, so all
 * of it is the scheme's own. */
static void loss_per_metre(double dx, double d, double T, const double *f, int count, double *db)
{
    static const double SPONGE[3] = { 50.0, 110.0, 180.0 };
    rb_pipe *s[3], *t[3];
    for (int k = 0; k < 3; k++) s[k] = lay(k, 48, dx, d, T);
    rb_pipe *in = lay(3, 48, dx, d, T), *out = lay(4, 48, dx, d, T);
    for (int k = 0; k < 3; k++) t[k] = lay(5 + k, 48, dx, d, T);
    for (int k = 0; k < 3; k++) {
        rb_pipe_pack(s[k], SPONGE[2 - k], 5e-3);
        rb_pipe_pack(t[k], SPONGE[k], 5e-3);
    }
    CHECK(rb_pipe_onward(s[0], s[1]) == 0 && rb_pipe_onward(s[1], s[2]) == 0
          && rb_pipe_onward(s[2], in) == 0 && rb_pipe_onward(in, out) == 0
          && rb_pipe_onward(out, t[0]) == 0 && rb_pipe_onward(t[0], t[1]) == 0
          && rb_pipe_onward(t[1], t[2]) == 0);
    rb_chamber *c = &in->cell[6];
    const double g = rb_chamber_gamma(c), p1 = P0 + 100.0;
    rb_chamber_init(c, c->V, p1, T * pow(p1 / P0, (g - 1.0) / g), AIR);
    for (int i = 0; i < STEPS; i++) {
        for (int k = 0; k < CHAIN; k++) rb_pipe_clear(&chain[k], 0);
        for (int k = 0; k < CHAIN; k++) rb_pipe_flows(&chain[k], 0, PIPE_DT);
        for (int k = 0; k < CHAIN; k++) rb_pipe_integrate(&chain[k], 0, PIPE_DT);
        for (int k = 0; k < CHAIN; k++) rb_pipe_accelerate(&chain[k], 0, PIPE_DT);
        near_rec[i] = in->cell[12].p - P0;
        far_rec[i] = out->cell[36].p - P0;
    }
    const double apart = (48 - 12 + 36) * dx;
    /* Only the pulse's direct pass at each probe: a window of 48 cells'
     * travel either side of its peak, so the sponges' edges' faint
     * reflections stay out. */
    const double *rec[2] = { near_rec, far_rec };
    int peak[2] = { 0, 0 };
    for (int r = 0; r < 2; r++)
        for (int i = 0; i < STEPS; i++)
            if (fabs(rec[r][i]) > fabs(rec[r][peak[r]])) peak[r] = i;
    const double sound = sqrt(1.4 * 287.0985 * T);
    const int half = (int)(48.0 * dx / sound / PIPE_DT);
    for (int j = 0; j < count; j++) {
        double a[2] = { 0.0, 0.0 };
        for (int r = 0; r < 2; r++) {
            double re = 0.0, im = 0.0;
            for (int i = peak[r] - half; i <= peak[r] + half; i++) {
                if (i < 0 || i >= STEPS) continue;
                re += rec[r][i] * cos(2.0 * PI * f[j] * i * PIPE_DT);
                im += rec[r][i] * sin(2.0 * PI * f[j] * i * PIPE_DT);
            }
            a[r] = sqrt(re * re + im * im);
        }
        db[j] = 20.0 * log10(a[0] / a[1]) / apart;
    }
}

/* The mean temperature of pipe p's gas now. */
static double mean_T(const rb_pipe *p)
{
    double sum = 0.0;
    for (int k = 0; k < p->cells; k++) sum += p->cell[k].T;
    return sum / p->cells;
}

/* to 2.4 kHz: 7.5 cm cells carry plane waves to about c / (pi dx) (noise.h),
 * and nothing past c / 2 dx, 3.5 kHz at 700 K */
#define FREQS 6
static const double FREQ[FREQS] = { 250.0, 500.0, 1000.0, 1500.0, 2000.0, 2400.0 };

void test_baseline_where_the_waves_are_weak(void)
{
    /* Each preset's plain exhaust pipes (its headers, exhaust pipes, links
     * and tailpipes; not the cans or the packing, which are components),
     * running at 3000 rpm wide open: the scheme's own loss over each pipe's
     * length, on its cells, at its gas's mean temperature, and along each
     * cylinder's whole plain path. Under 1 dB to 500 Hz along every path (the
     * firing orders are sound); by 2.4 kHz it is 3 dB and more (the band
     * where it is loud is the scheme's, not the sound's), and on cells a
     * third as long it is a tenth (it is the grid's). Printed: the path's loss
     * at each frequency. */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        if (d.net.pipes > 0) continue;   /* a network preset's plain pipes have no legacy roles (stage 141) */
        rb_engine_init(&e, &d, 1.0 / 192000.0);
        e.rpm = 3000.0;
        e.throttle_pos = 1.0;
        e.ignition = 1;
        rb_engine_settle(&e, 40, 1e-4);
        double T_sum[64] = { 0.0 };
        /* each plain pipe, and whose it is: a cylinder's (its header), a
         * collector's (k: 100 + c) or a system's (200 + s) */
        const rb_pipe *plain[64];
        int owner[64], n = 0;
        for (int i = 0; i < d.cylinders; i++) { owner[n] = i; plain[n++] = &e.header[i]; }
        for (int c = 0; c < e.collectors; c++) {
            owner[n] = 100 + c; plain[n++] = e.exhaust_pipe[c];
            if (d.center_length > 0.0) { owner[n] = 100 + c; plain[n++] = e.link[c]; }
            if (d.center_length > 0.0 && !(d.center_volume > 0.0) && !(d.center_packing > 0.0)) {
                owner[n] = 100 + c;
                plain[n++] = e.center[c];
            }
        }
        for (int s = 0; s < e.systems; s++) { owner[n] = 200 + s; plain[n++] = e.tailpipe[s]; }
        if (d.muffler_type == RB_MUFFLER_NONE)
            for (int s = 0; s < e.systems; s++) { owner[n] = 200 + s; plain[n++] = e.muffler[s]; }
        const int samples = 8, gap = (int)(120.0 / 3000.0 / 8.0 * 192000.0);
        for (int k = 0; k < samples; k++) {
            for (int j = 0; j < gap; j++) rb_engine_step(&e);
            for (int q = 0; q < n; q++) T_sum[q] += mean_T(plain[q]) / samples;
        }
        double worst[FREQS] = { 0.0 };
        double path[RB_MAX_CYLINDERS][FREQS] = { { 0.0 } };
        for (int q = 0; q < n; q++) {
            double db[FREQS];
            loss_per_metre(plain[q]->dx, plain[q]->diameter, T_sum[q], FREQ, FREQS, db);
            for (int i = 0; i < d.cylinders; i++) {
                const int c = d.collector[i], sys = (e.systems == 2) ? c : 0;
                const int mine = owner[q] == i || owner[q] == 100 + c || owner[q] == 200 + sys;
                if (mine)
                    for (int j = 0; j < FREQS; j++) path[i][j] += db[j] * plain[q]->length;
            }
        }
        for (int i = 0; i < d.cylinders; i++)
            for (int j = 0; j < FREQS; j++) worst[j] = fmax(worst[j], path[i][j]);
        printf("    %-8s along the worst plain path:", rb_preset_key(p));
        for (int j = 0; j < FREQS; j++) printf(" %4.0f Hz %5.2f dB%s", FREQ[j], worst[j], j < FREQS - 1 ? "," : "\n");
        CHECK(worst[1] < 1.0);
        CHECK(worst[2] < 0.5);
        CHECK(worst[5] > 3.0);
    }
    /* It is the grid's: the four's tailpipe on cells a third as long. */
    double coarse[FREQS], fine[FREQS];
    loss_per_metre(0.075, 0.050, 700.0, FREQ, FREQS, coarse);
    loss_per_metre(0.025, 0.050, 700.0, FREQ, FREQS, fine);
    printf("    7.5 cm cells at 700 K: %.3f dB/m at 1 kHz, %.3f at 1.5, %.3f at 2, %.3f at 2.4 kHz; 2.5 cm: %.3f, %.3f, %.3f, %.3f\n",
           coarse[2], coarse[3], coarse[4], coarse[5], fine[2], fine[3], fine[4], fine[5]);
    CHECK(fine[4] < 0.5 * coarse[4]);
}

/* ---- the whole drive ---- */

static float block[2 * 1024];

/* Renders `seconds` of the sim (mono), returning the largest |sample| and
 * leaving its rpm in *rpm. */
static double run(rb_sim *sim, double seconds, double *rpm)
{
    double most = 0.0;
    const int blocks = (int)(seconds * 48000.0 / 1024.0);
    for (int b = 0; b < blocks; b++) {
        rb_render_block(sim, block, 1024);
        for (int i = 0; i < 1024; i++) {
            if (!(fabs(block[i]) <= 1.0)) most = 2.0;   /* a NaN or a clip */
            most = fmax(most, fabs(block[i]));
        }
    }
    *rpm = rb_get_telemetry(sim, RB_TLM_RPM);
    return most;
}

void test_baseline_whole_drive_books(void)
{
    /* Every preset at its real-time quality on its own crank: the key
     * (starter), idle, a held load, a pull to the limiter and a second on
     * it, the lift into the overrun's fuel cut, the coast to its resume, and
     * idle again. Each step does what it says (the rpm, the cuts; the part
     * throttle with 40 N m, a smaller engine's share of it, only has to run); the sound
     * stays a number inside full scale; the guards stay under their warning;
     * and the engine's books close over the whole drive: its mass to 1e-9 of
     * what passed through, its species as the burn moved them, its energy to
     * 1e-6 of what crossed it (enthalpy in and out, the pistons' work, the
     * walls' heat, the fuel's energy and the heat released). The soft cut
     * holds each at its limit, within 30 rpm over it. */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_sim *sim = rb_create_with_quality(48000.0, rb_preset_quality(p));
        CHECK(sim != NULL);
        if (!sim) return;
        CHECK(rb_load_preset(sim, p) == RB_OK);
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_set_param(sim, RB_PARAM_HOLD, 0);
        rb_engine *x = &sim->engine;
        const double idle = x->desc.idle_rpm, limit = x->desc.rev_limit;
        double m0, u0, y0[RB_NSPECIES];
        rb_engine_contents(x, &m0, &u0, y0);
        double rpm, loudest = 0.0;
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        rb_set_param(sim, RB_PARAM_STARTER, 1);
        loudest = fmax(loudest, run(sim, 1.0, &rpm));
        rb_set_param(sim, RB_PARAM_STARTER, 0);
        loudest = fmax(loudest, run(sim, 2.0, &rpm));
        const double idled = rpm;
        rb_set_param(sim, RB_PARAM_THROTTLE, 0.4);
        /* 40 N m for 2 L and over; a smaller engine its share (stage 145: the
         * Polo's 1.0 L, without its boost, makes 88 N m flat out) */
        const double litres = rb_engine_displacement(x) * 1e3;
        rb_set_param(sim, RB_PARAM_LOAD_TORQUE, 40.0 * fmin(1.0, litres / 2.0));
        loudest = fmax(loudest, run(sim, 1.0, &rpm));
        const double held = rpm;
        rb_set_param(sim, RB_PARAM_LOAD_TORQUE, 0.0);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
        double top = 0.0;
        for (int k = 0; k < 15; k++) {
            loudest = fmax(loudest, run(sim, 0.1, &rpm));
            top = fmax(top, rpm);
        }
        const double cuts = rb_get_telemetry(sim, RB_TLM_LIMITER_CUTS), pulled = rpm;
        rb_set_param(sim, RB_PARAM_THROTTLE, 0.0);
        int fuel_cut = 0;
        for (int k = 0; k < 20; k++) {
            loudest = fmax(loudest, run(sim, 0.1, &rpm));
            fuel_cut |= rb_get_telemetry(sim, RB_TLM_FUEL_CUT) > 0.5;
        }
        loudest = fmax(loudest, run(sim, 3.0, &rpm));
        const int resumed = rb_get_telemetry(sim, RB_TLM_FUEL_CUT) < 0.5;
        const double guards = (double)rb_engine_guards(x);
        printf("    %-8s idle %4.0f rpm, held %4.0f, pulled to %4.0f (top %4.0f, limit %4.0f, %3.0f cuts), fuel cut %d, resumed %d at %4.0f rpm; loudest %.2f; %0.f guards\n",
               rb_preset_key(p), idled, held, pulled, top, limit, cuts, fuel_cut, resumed, rpm, loudest, guards);
        CHECK(fabs(idled - idle) < 0.2 * idle);
        CHECK(held > 0.0);
        CHECK(cuts > 0.0 && pulled > limit - 600.0);
        CHECK(top < limit + 30.0);   /* the soft cut holds it at the limit */
        CHECK(fuel_cut && resumed);
        CHECK(fabs(rpm - idle) < 0.25 * idle);
        CHECK(loudest < 1.0);
        CHECK(rb_get_telemetry(sim, RB_TLM_GUARD_RATE) < RB_GUARD_RATE_WARN);

        double m1, u1, y1[RB_NSPECIES];
        rb_engine_contents(x, &m1, &u1, y1);
        const double through = x->mass_in + x->mass_out;
        CHECK_NEAR(m1 - m0, x->mass_in - x->mass_out + x->fuel_in, 1e-9 * through);
        const double crossed = fabs(x->energy_in) + fabs(x->energy_out) + fabs(x->heat_released);
        const double booked = x->energy_in - x->energy_out + x->work_on_gas + x->wall_heat
                            + rb_engine_pipe_heat(x) + x->fuel_energy_in + x->heat_released;
        printf("             mass book %.1e of the flow; energy book %.1e of what crossed\n",
               fabs(m1 - m0 - (x->mass_in - x->mass_out + x->fuel_in)) / through,
               fabs(u1 - u0 - booked) / crossed);
        CHECK_NEAR(u1 - u0, booked, 1e-6 * crossed);
        rb_destroy(sim);
    }
}

/* ---- a geometry edit returns its load upstream ---- */

/* The four held at 4000 rpm wide open, its exhaust edited by `edit`: the
 * collectors' mean pressure and the pumping loss. */
static void exhaust_load(int edit, double *p_exhaust, double *pmep, double *ve)
{
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i4"), &d);
    if (edit == 1) d.muffler_type = RB_MUFFLER_NONE;
    if (edit == 2) d.muffler_area *= 0.5;
    if (edit == 3) d.tailpipe_length += 0.6;
    rb_engine_init(&e, &d, 1.0 / 192000.0);
    e.rpm = 4000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 80, 1e-5);
    *p_exhaust = e.result.p_exhaust;
    *pmep = e.result.pmep;
    *ve = e.result.ve;
}

void test_baseline_geometry_returns_upstream(void)
{
    /* Edits behind the catalyst reach the cylinders: no muffler lowers the
     * collectors' back pressure and the pumping loss; baffles half as open
     * raise both; a longer tailpipe moves the back pressure and the VE. */
    double p[4], w[4], v[4];
    for (int k = 0; k < 4; k++) exhaust_load(k, &p[k], &w[k], &v[k]);
    printf("    back pressure %.0f Pa, pumping %.0f Pa as built; no muffler %+.0f, %+.0f; baffles half open %+.0f, %+.0f; 0.6 m more tailpipe %+.0f Pa, VE %+.3f %%\n",
           p[0] - P0, w[0], p[1] - p[0], w[1] - w[0], p[2] - p[0], w[2] - w[0], p[3] - p[0], 100.0 * (v[3] / v[0] - 1.0));
    CHECK(p[1] < p[0] && w[1] < w[0]);
    CHECK(p[2] > p[0] && w[2] > w[0]);
    CHECK(fabs(p[3] - p[0]) > 20.0 && fabs(v[3] / v[0] - 1.0) > 1e-4);
}
