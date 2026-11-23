/*
 * test_structure.c - stage 055: the engine's structure and accessories
 * (noise.c's candidate, RB_PARAM_VARIANT 1 and 2). The block's bending modes
 * come from each engine's own geometry and each cylinder strikes them where
 * it is; variant 2 adds the cam chain's and the alternator's tones. The
 * accepted sound (variant 0) is unchanged: tools/cli_parity.py.
 */
#include "engine.h"
#include "noise.h"
#include "presets.h"
#include "rb.h"
#include "rb_internal.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DT      (1.0 / 192000.0)
#define DT_FAST (1.0 / 96000.0)

static rb_engine e;
static rb_noise n, n2;

static void preset_desc(const char *key, rb_engine_desc *d)
{
    rb_preset_desc(rb_preset_find(key), d);
}

/* A mode's frequency from its resonator: the pole r e^(i w dt) gives
 * cos(w dt) = a1 / (2 sqrt(a2)). */
static double mode_hz(const rb_noise_mode *m, double dt)
{
    return acos(m->a1 / (2.0 * sqrt(m->a2))) / (2.0 * PI * dt);
}

/* Blevins's free-free beam, from his table (Formulas for Natural Frequency
 * and Mode Shape, table 8-1): beta L 4.73004 and 7.85320, sigma 0.982502
 * and 1.000777. The test's own copy of the constants. */
static double blevins(int mode, double s)
{
    const double bl = mode ? 7.85320462 : 4.73004074, sigma = mode ? 1.00077731 : 0.98250222;
    const double b = bl * s;
    return cosh(b) + cos(b) - sigma * (sinh(b) + sin(b));
}

/* The block as the candidate sees it, from the description alone. */
static void block_of(const rb_engine_desc *d, double *length, double *depth, double *lo)
{
    double a = d->position[0], b = d->position[0], up = 0.0;
    for (int i = 1; i < d->cylinders; i++) {
        a = fmin(a, d->position[i]);
        b = fmax(b, d->position[i]);
    }
    for (int k = 0; k < d->banks; k++) up = fmax(up, fabs(cos(d->bank_angle[k] * PI / 180.0)));
    *length = b - a + 1.6 * d->bore;
    *depth = fmax((d->stroke + d->rod) * up + 0.5 * d->stroke, d->bore + 0.060);
    *lo = a;
}

void test_structure_modes_from_geometry(void)
{
    /* Each engine's block: its bending modes go as depth / length^2, fitted
     * to the four's 950 Hz; the second at (7.8532 / 4.7300)^2 of the first;
     * the local modes by the bore as before. Each cylinder's weight is
     * Blevins's shape at its place, an RMS of 1 over the cylinders, times
     * its bank's projection: vertical modes cos, lateral sin, so a boxer's
     * force bends its block only sideways, its banks opposite. */
    const double ref_l = 3.0 * 0.096 + 1.6 * 0.086, ref_d = 0.086 + 0.143 + 0.043;
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        rb_engine_init(&e, &d, DT);
        rb_noise_init(&n, &e, 1);
        double length, depth, lo;
        block_of(&d, &length, &depth, &lo);
        CHECK_NEAR(n.block_length, length, 1e-12);
        CHECK_NEAR(n.block_depth, depth, 1e-12);
        /* Timoshenko's shear, to first order: (1 + E / (kappa G)) / 12 */
        const double t = (1.0 + 2.66 / (5.0 / 6.0)) / 12.0, b1 = 4.73004, b2 = 7.85320;
        const double r = depth / length, r0 = ref_d / ref_l;
        const double v1 = 950.0 * depth / ref_d * (ref_l / length) * (ref_l / length)
                        * sqrt((1.0 + t * b1 * b1 * r0 * r0) / (1.0 + t * b1 * b1 * r * r));
        CHECK_NEAR(n.cmode_hz[0] / v1, 1.0, 1e-6);
        CHECK_NEAR(n.cmode_hz[2] / n.cmode_hz[0],
                   (b2 / b1) * (b2 / b1) * sqrt((1.0 + t * b1 * b1 * r * r) / (1.0 + t * b2 * b2 * r * r)), 1e-5);
        CHECK_NEAR(n.cmode_hz[3], 1750.0 * sqrt(0.086 / d.bore), 1e-9);
        for (int k = 0; k < RB_NOISE_CMODES; k++)
            CHECK_NEAR(mode_hz(&n.cmode[k], DT) / n.cmode_hz[k], 1.0, 1e-9);
        if (p == 0) CHECK_NEAR(n.cmode_hz[0], 950.0, 1e-9);

        /* the weights */
        double rms[2] = { 0.0, 0.0 };
        for (int i = 0; i < d.cylinders; i++)
            for (int m = 0; m < 2; m++) {
                const double s = blevins(m, (d.position[i] - lo + 0.8 * d.bore) / length);
                rms[m] += s * s;
            }
        for (int m = 0; m < 2; m++) rms[m] = sqrt(rms[m] / d.cylinders);
        double sum2 = 0.0;
        for (int i = 0; i < d.cylinders; i++) {
            const double s = (d.position[i] - lo + 0.8 * d.bore) / length;
            const double a = d.bank_angle[d.bank[i]] * PI / 180.0;
            const double w1 = blevins(0, s) / rms[0], w2 = blevins(1, s) / rms[1];
            CHECK_NEAR(n.cweight[i][0][0], w1 * cos(a), 1e-4);
            CHECK_NEAR(n.cweight[i][1][0], w1 * sin(a), 1e-4);
            CHECK_NEAR(n.cweight[i][2][0], w2 * cos(a), 1e-4);
            CHECK_NEAR(n.cweight[i][0][1], w1 * fabs(sin(a)), 1e-4);
            CHECK_NEAR(n.cweight[i][1][1], w1 * fabs(cos(a)), 1e-4);
            for (int k = 3; k < RB_NOISE_CMODES; k++) CHECK(n.cweight[i][k][0] == 1.0 && n.cweight[i][k][1] == 1.0);
            sum2 += n.cweight[i][0][0] * n.cweight[i][0][0] + n.cweight[i][1][0] * n.cweight[i][1][0];
        }
        /* a cylinder's force, between the vertical and lateral first modes,
         * keeps the shape's energy: cos^2 + sin^2 */
        CHECK_NEAR(sum2 / d.cylinders, 1.0, 1e-9);
        printf("    %-9s block %.3f m long, %.3f deep, %.3f wide: bending %4.0f Hz vertical, %4.0f lateral, %4.0f second\n",
               rb_preset_key(p), n.block_length, n.block_depth, n.block_width,
               n.cmode_hz[0], n.cmode_hz[1], n.cmode_hz[2]);
    }

    /* The four's ends and middle are on either side of the first mode's
     * nodes (0.224 and 0.776 of the length): opposite signs, symmetric; the
     * second mode is antisymmetric. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    rb_noise_init(&n, &e, 1);
    CHECK(n.cweight[0][0][0] > 0.0 && n.cweight[1][0][0] < 0.0);
    CHECK_NEAR(n.cweight[0][0][0], n.cweight[3][0][0], 1e-6);
    CHECK_NEAR(n.cweight[0][2][0], -n.cweight[3][2][0], 1e-6);
    /* The boxer: no vertical force; its banks bend the block opposite ways. */
    preset_desc("boxer4", &d);
    rb_engine_init(&e, &d, DT);
    rb_noise_init(&n, &e, 1);
    for (int i = 0; i < d.cylinders; i++) CHECK(fabs(n.cweight[i][0][0]) < 1e-12);
    for (int i = 0; i < d.cylinders; i++)
        for (int j = 0; j < d.cylinders; j++)
            if (d.bank[i] != d.bank[j]) CHECK(n.cweight[i][1][0] * n.cweight[j][1][0] < 0.0);
}

/* The four posed with its gas at ambient, still; one step of cylinder i's
 * pressure rising by dp, then `steps` more: mode k's peak. */
static double strike_peak(const char *key, int i, int k, double dp)
{
    rb_engine_desc d;
    preset_desc(key, &d);
    rb_engine_init(&e, &d, DT);
    for (int j = 0; j < d.cylinders; j++) e.cyl[j].gas.p = d.p_ambient;
    e.omega = 0.0;
    rb_noise_init(&n, &e, 1);
    n.variant = 1;
    e.cyl[i].gas.p += dp;
    double peak = 0.0;
    for (int s = 0; s < 1920; s++) {
        rb_noise_step(&n, &e, RB_NOISE_ENGINE);
        peak = fmax(peak, fabs(n.cmode[k].y1));
    }
    return peak;
}

void test_structure_paths_per_cylinder(void)
{
    /* Each cylinder's blow reaches each mode as its weight says: the same
     * force step in cylinder 2 (the middle) and 1 (the end) rings the first
     * bending mode as their weights' ratio, and the local modes alike. The
     * accepted sound (variant 0) strikes every mode alike from every
     * cylinder. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    rb_noise_init(&n2, &e, 1);
    const double end = strike_peak("i4", 0, 0, 1e5), middle = strike_peak("i4", 1, 0, 1e5);
    CHECK(end > 0.0);
    CHECK_NEAR(middle / end, fabs(n2.cweight[1][0][0] / n2.cweight[0][0][0]), 1e-9);
    CHECK_NEAR(strike_peak("i4", 1, 4, 1e5) / strike_peak("i4", 0, 4, 1e5), 1.0, 1e-12);
    /* the boxer's lateral mode: opposite banks, opposite rings */
    preset_desc("boxer4", &d);
    const double lateral = strike_peak("boxer4", 0, 1, 1e5);
    CHECK(lateral > 0.0);
    CHECK(strike_peak("boxer4", 0, 0, 1e5) < 1e-12 * lateral);

    /* A slap is across the bore: an inline's pistons slapping at their dead
     * centres (the gas at ambient, so only the slaps strike) ring the
     * block's lateral bending and never its vertical. */
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    for (int i = 0; i < d.cylinders; i++) e.cyl[i].gas.p = d.p_ambient;
    e.omega = 300.0;
    e.theta = fmod(e.desc.firing[0] - 0.05 + 720.0, 720.0);
    rb_noise_init(&n, &e, 1);
    n.variant = 1;
    e.theta = fmod(e.theta + 0.1, 720.0);
    rb_noise_step(&n, &e, RB_NOISE_ENGINE);
    CHECK_EQ_INT(n.impacts[RB_IMPACT_SLAP], 4);
    CHECK(n.cmode[1].y1 != 0.0);
    CHECK(n.cmode[0].y1 == 0.0 && n.cmode[2].y1 == 0.0);
}

/* Runs the engine of `key` at rpm, wide open, fired, settled; then `steps`
 * steps with n at variant `va` and n2 at `vb`, the same seed: their bays'
 * sums of squares, and (if given) the difference of the two into `diff`. */
static void two_variants(const char *key, double rpm, int va, int vb, long steps,
                         double *ea, double *eb, double *diff, uint64_t seed)
{
    rb_engine_desc d;
    preset_desc(key, &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.rpm = rpm;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-4);
    rb_noise_init(&n, &e, seed);
    rb_noise_init(&n2, &e, seed);
    n.variant = va;
    n2.variant = vb;
    *ea = *eb = 0.0;
    for (long s = 0; s < steps; s++) {
        rb_engine_step(&e);
        rb_noise_step(&n, &e, RB_NOISE_ENGINE);
        rb_noise_step(&n2, &e, RB_NOISE_ENGINE);
        *ea += n.out[RB_NOISE_BAY] * n.out[RB_NOISE_BAY];
        *eb += n2.out[RB_NOISE_BAY] * n2.out[RB_NOISE_BAY];
        if (diff) diff[s] = n2.out[RB_NOISE_BAY] - n.out[RB_NOISE_BAY];
    }
}

void test_structure_events_and_energy(void)
{
    /* The candidate changes where the blows go, not the blows: the same
     * impacts, counted and sized, as the accepted sound's; the bay's level
     * within 3 dB of it on every engine (the weights keep the block's
     * energy), and the same seed's noise, so nothing random is added. */
    for (int p = 0; p < rb_preset_count(); p++) {
        double e0, e1;
        two_variants(rb_preset_key(p), 3000.0, 0, 1, 24000, &e0, &e1, NULL, 3);
        for (int k = 0; k < RB_IMPACT_KINDS; k++) {
            CHECK_EQ_INT(n.impacts[k], n2.impacts[k]);
            CHECK(n.impact_energy[k] == n2.impact_energy[k]);
        }
        CHECK(n.state == n2.state);
        const double change = 10.0 * log10(e1 / e0);
        printf("    %-9s the bay at 3000 rpm wide open: %+.2f dB against the accepted sound\n",
               rb_preset_key(p), change);
        CHECK(fabs(change) < 3.0);
    }
}

void test_structure_statistics_over_seeds(void)
{
    /* Repeated seeds: the bay's level over 0.25 s of the V8 at 3000 rpm
     * within 1 dB of the seeds' mean, and the candidate's spread no wider
     * than the accepted sound's twice over. */
    double level[2][6], mean[2] = { 0.0, 0.0 };
    for (int s = 0; s < 6; s++) {
        double e0, e1;
        two_variants("v8-cross", 3000.0, 0, 1, 24000, &e0, &e1, NULL, 11 + 7 * s);
        level[0][s] = 10.0 * log10(e0);
        level[1][s] = 10.0 * log10(e1);
        mean[0] += level[0][s] / 6.0;
        mean[1] += level[1][s] / 6.0;
    }
    double spread[2] = { 0.0, 0.0 };
    for (int v = 0; v < 2; v++)
        for (int s = 0; s < 6; s++) spread[v] = fmax(spread[v], fabs(level[v][s] - mean[v]));
    printf("    seeds' spread: %.3f dB accepted, %.3f dB candidate\n", spread[0], spread[1]);
    CHECK(spread[1] < 1.0);
    CHECK(spread[1] < 2.0 * spread[0] + 0.05);
}

/* x's amplitude at f (Hz) over `count` samples at DT_FAST, the window an
 * integer number of its periods. */
static double tone(const double *x, long count, double f)
{
    const long periods = (long)floor(count * DT_FAST * f);
    const long m = lround(periods / (f * DT_FAST));
    double re = 0.0, im = 0.0;
    for (long k = 0; k < m && k < count; k++) {
        re += x[k] * cos(2.0 * PI * f * k * DT_FAST);
        im += x[k] * sin(2.0 * PI * f * k * DT_FAST);
    }
    return 2.0 * sqrt(re * re + im * im) / (double)m;
}

static double diff[48000];

void test_structure_accessories(void)
{
    /* Variant 2 is variant 1 and two tones, nothing else: the chain's at
     * 21 times the crank, 0.02 Pa at 3000 rpm and as the rpm; the
     * alternator's at 97.2 times, 0.006 Pa for 800 W of accessories. */
    const double rpms[2] = { 3000.0, 1500.0 };
    for (int r = 0; r < 2; r++) {
        double e1, e2;
        two_variants("i4", rpms[r], 1, 2, 48000, &e1, &e2, diff, 5);
        const double crank = e.rpm / 60.0;
        CHECK_NEAR(tone(diff, 48000, 21.0 * crank) / (0.02 * e.rpm / 3000.0), 1.0, 0.01);
        CHECK_NEAR(tone(diff, 48000, 97.2 * crank) / 0.006, 1.0, 0.01);
        /* nowhere else: the difference's energy is the two tones' */
        double all = 0.0;
        for (long k = 0; k < 48000; k++) all += diff[k] * diff[k];
        const double tones = 0.5 * (pow(0.02 * e.rpm / 3000.0, 2) + 0.006 * 0.006) * 48000;
        CHECK_NEAR(all / tones, 1.0, 0.02);
    }
    /* A still engine: no tones. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT_FAST);
    rb_noise_init(&n, &e, 1);
    n.variant = 2;
    double most = 0.0;
    for (int s = 0; s < 9600; s++) {
        rb_noise_step(&n, &e, RB_NOISE_ENGINE);
        most = fmax(most, fabs(n.out[RB_NOISE_BAY]));
    }
    CHECK(most == 0.0);
}

void test_structure_variant_reaches_the_noise(void)
{
    /* RB_PARAM_VARIANT 1, 2 (and 3, stage 054's) reach the noise; any other
     * number leaves the accepted sound. */
    rb_sim *sim = rb_create(NULL);
    CHECK(sim != NULL);
    if (!sim) return;
    float out[256];
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    const int want[5][2] = { { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 5, 0 } };   /* 3: stage 054's */
    for (int k = 0; k < 5; k++) {
        rb_set_param(sim, RB_PARAM_VARIANT, want[k][0]);
        rb_render_block(sim, out, 128);
        CHECK_EQ_INT(sim->noise.variant, want[k][1]);
    }
    rb_destroy(sim);
}

/* Each cylinder's share of the bay's sound over its own firing interval
 * (from its firing TDC on, 720 / N degrees), as a spread: the loudest
 * cylinder's against the quietest's (dB), for variant 0 [0] and 1 [1]. */
static void cylinder_spread(const char *key, double spread[2])
{
    rb_engine_desc d;
    preset_desc(key, &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.rpm = 3000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-4);
    rb_noise_init(&n, &e, 21);
    rb_noise_init(&n2, &e, 21);
    n2.variant = 1;
    double sum[2][RB_MAX_CYLINDERS] = { { 0.0 } };
    const double interval = 720.0 / d.cylinders;
    for (long s = 0; s < 96000; s++) {   /* 50 cycles */
        rb_engine_step(&e);
        rb_noise_step(&n, &e, RB_NOISE_ENGINE);
        rb_noise_step(&n2, &e, RB_NOISE_ENGINE);
        for (int i = 0; i < d.cylinders; i++)
            if (rb_engine_cylinder_angle(&e, i) < interval) {
                sum[0][i] += n.out[RB_NOISE_BAY] * n.out[RB_NOISE_BAY];
                sum[1][i] += n2.out[RB_NOISE_BAY] * n2.out[RB_NOISE_BAY];
            }
    }
    for (int v = 0; v < 2; v++) {
        double lo = sum[v][0], hi = sum[v][0];
        for (int i = 1; i < d.cylinders; i++) {
            lo = fmin(lo, sum[v][i]);
            hi = fmax(hi, sum[v][i]);
        }
        spread[v] = 10.0 * log10(hi / lo);
    }
}

void test_structure_order_statistics(void)
{
    /* The cylinders' blows now differ by where they land: each cylinder's
     * firing interval in the bay is as loud as before give or take its place
     * on the block. The accepted sound's cylinders differ only by their gas
     * and the random spread of their impacts; the candidate's more, and
     * within 3 dB. */
    const char *keys[3] = { "i4", "i6", "v8-cross" };
    for (int k = 0; k < 3; k++) {
        double spread[2];
        cylinder_spread(keys[k], spread);
        printf("    %-9s cylinder to cylinder in the bay: %.2f dB accepted, %.2f dB candidate\n",
               keys[k], spread[0], spread[1]);
        CHECK(spread[1] > spread[0]);
        CHECK(spread[1] < 3.0);
    }
}
