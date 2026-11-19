/*
 * test_layouts.c - stage 131's evidence: engines built from a family, a cylinder count, a crank
 * and a bank angle (core/src/layouts.c).
 *
 * - What's offered is exactly the stated set (inline 1-6, V 2-16 even at 30-120 degrees,
 *   boxer 2/4/6, each crank only where it belongs), with a reason for everything else.
 * - Each preset is its own layout: its layout gives it back whole, and the generator makes its
 *   crank (pins, banks, firing order) from any other preset too.
 * - Every combination fires as stated: the even ones every 720/n degrees (and a V's banks, a
 *   boxer six's, every 1440/n), the known uneven ones as the real engines do (a 90-degree
 *   common-pin V6 90/150, a 45-degree V-twin 315/405, a 90-degree V10 54/90, the twins), each
 *   cylinder's firing TDC where its pin points along its bank, and running, the sparks in that
 *   order at those angles.
 * - What a layout keeps from its preset and what it works out.
 * - The public API: building, refusing whole, reading back, 16 cylinders' state.
 * - Every combination runs: starts, idles 30 s without hunting, takes a blip and a load, revs
 *   into its limiter, comes back and stops.
 */
#include "layouts.h"
#include "presets.h"
#include "rb_internal.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DT_FAST (1.0 / 96000.0)   /* as test_presets.c: the idle comes out as at 192 kHz */
#define PI      3.14159265358979323846

static rb_engine e;   /* kept off the stack */

/* The stated set, written out again here rather than asked of the core. */
static int offered(int family, int n, int crank, double angle)
{
    if (family == RB_FAMILY_INLINE)
        return n >= 1 && n <= 6 && angle == 0.0
            && (crank == RB_CRANK_EVEN || (n == 2 && (crank == RB_CRANK_TWIN_180 || crank == RB_CRANK_TWIN_270)));
    if (family == RB_FAMILY_V)
        return n >= 2 && n <= 16 && n % 2 == 0 && angle >= 30.0 && angle <= 120.0
            && (crank == RB_CRANK_EVEN || crank == RB_CRANK_COMMON_PIN || (crank == RB_CRANK_CROSS_PLANE && n == 8));
    if (family == RB_FAMILY_BOXER)
        return (n == 2 || n == 4 || n == 6) && angle == 0.0 && crank == RB_CRANK_EVEN;
    return 0;
}

/* Every offered combination at its usual angle: family, count, crank. */
typedef struct combo { int family, n, crank; } combo;

static int all_combos(combo *out)
{
    int k = 0;
    for (int f = 0; f < RB_FAMILY_COUNT; f++)
        for (int n = 1; n <= RB_MAX_CYLINDERS; n++)
            for (int c = 0; c < RB_CRANK_COUNT; c++) {
                const double angle = rb_layout_angle(f, n, c, RB_ANGLE_USUAL);
                if (!isnan(angle) && rb_layout_check(f, n, c, angle) == RB_OK) out[k++] = (combo){ f, n, c };
            }
    return k;
}

/* The gaps (deg) from one firing to the next in the firing order, of all the cylinders (bank
 * -1), of one bank, or of one collector (by_collector). How many. */
static int gaps_of(const rb_engine_desc *d, int bank, int by_collector, double *gap)
{
    double first = -1.0, prev = -1.0;
    int n = 0;
    for (int k = 0; k < d->cylinders; k++) {
        const int i = d->firing_order[k];
        const int group = by_collector ? d->collector[i] : d->bank[i];
        if (bank >= 0 && group != bank) continue;
        if (prev < 0.0) first = d->firing[i];
        else gap[n++] = d->firing[i] - prev;
        prev = d->firing[i];
    }
    gap[n++] = first + 720.0 - prev;
    return n;
}

static int all_gaps(const rb_engine_desc *d, int bank, int by_collector, double want)
{
    double gap[RB_MAX_CYLINDERS];
    const int n = gaps_of(d, bank, by_collector, gap);
    int ok = 1;
    for (int k = 0; k < n; k++) ok = ok && fabs(gap[k] - want) < 1e-6;
    return ok;
}

/* 1 if the gaps are `want` repeated, starting anywhere. */
static int gaps_repeat(const rb_engine_desc *d, const double *want, int n_want)
{
    double gap[RB_MAX_CYLINDERS];
    const int n = gaps_of(d, -1, 0, gap);
    if (n % n_want) return 0;
    for (int s = 0; s < n_want; s++) {
        int same = 1;
        for (int k = 0; k < n; k++) same = same && fabs(gap[k] - want[(s + k) % n_want]) < 1e-6;
        if (same) return 1;
    }
    return 0;
}

/* Each cylinder's firing TDC is where its crank pin points along its bank: found by turning
 * the crank in quarter degrees and looking for where the pin's direction meets the bank's
 * (unit vectors, not the core's angle sums). The number of cylinders where it isn't. */
static int pins_off_their_tdc(const rb_engine_desc *d)
{
    int wrong = 0;
    for (int i = 0; i < d->cylinders; i++) {
        const double axis = d->bank_angle[d->bank[i]] * PI / 180.0;
        const double axis0 = d->bank_angle[d->bank[0]] * PI / 180.0;
        double best = -2.0, at = -1.0;
        for (int q = 0; q < 4 * 360; q++) {
            /* crank angle measured from cylinder 1's TDC: there pin 0 points along bank 0 */
            const double crank = q * 0.25 * PI / 180.0;
            const double pin = axis0 - d->pin[0] * PI / 180.0 + crank + d->pin[i] * PI / 180.0;
            const double meet = cos(pin) * cos(axis) + sin(pin) * sin(axis);
            if (meet > best) {
                best = meet;
                at = q * 0.25;
            }
        }
        if (fabs(remainder(at - d->firing[i], 360.0)) > 0.26) wrong++;
    }
    return wrong;
}

/* Runs the engine held at 3000 rpm for two cycles and counts its sparks: those out of the
 * firing order, and those away from their cylinder's firing angle less its advance. */
static void count_sparks(const rb_engine_desc *d, int *sparks, int *out_of_order, int *misplaced)
{
    rb_engine_init(&e, d, DT_FAST);
    e.rpm = 3000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    const double step_deg = 6.0 * e.rpm * DT_FAST;
    const long cycle = lround(720.0 / step_deg);
    for (long k = 0; k < 2 * cycle; k++) rb_engine_step(&e);
    int was[RB_MAX_CYLINDERS], next = -1;
    *sparks = *out_of_order = *misplaced = 0;
    for (int i = 0; i < d->cylinders; i++) was[i] = e.burn[i].after_spark;
    for (long k = 0; k < 2 * cycle; k++) {
        rb_engine_step(&e);
        for (int i = 0; i < d->cylinders; i++) {
            if (e.burn[i].after_spark && !was[i]) {
                int place = 0;
                while (d->firing_order[place] != i) place++;
                if (next >= 0 && place != next) (*out_of_order)++;
                next = (place + 1) % d->cylinders;
                double late = fmod(e.theta - d->firing[i] - e.burn[i].spark_angle + 1440.0, 720.0);
                if (late > 360.0) late -= 720.0;
                if (!(late >= 0.0 && late <= step_deg + 1e-9)) (*misplaced)++;
                (*sparks)++;
            }
            was[i] = e.burn[i].after_spark;
        }
    }
}

static double litres(const rb_engine_desc *d)
{
    return d->cylinders * 0.25 * PI * d->bore * d->bore * d->stroke * 1000.0;
}

void test_layout_offers_the_stated_set(void)
{
    static const double angles[] = { 0.0, 29.9, 30.0, 45.0, 60.0, 90.0, 120.0, 120.1, 180.0 };
    int checked = 0, wrong = 0;
    for (int f = 0; f < RB_FAMILY_COUNT; f++)
        for (int n = 0; n <= RB_MAX_CYLINDERS + 1; n++)
            for (int c = 0; c < RB_CRANK_COUNT; c++)
                for (size_t a = 0; a < sizeof angles / sizeof angles[0]; a++) {
                    const int want = offered(f, n, c, angles[a]);
                    const int got = rb_layout_check(f, n, c, angles[a]);
                    const char *why = rb_layout_why(f, n, c, angles[a]);
                    /* the cross-plane crank fires once a cycle only near 90 degrees */
                    if (want && c == RB_CRANK_CROSS_PLANE && got != RB_OK) continue;
                    /* on common pins, two cylinders fire at once where a bank's own firings
                     * fall on the other's: where angle x n/2 is a whole number of half turns */
                    if (want && c == RB_CRANK_COMMON_PIN && got != RB_OK && why
                        && fabs(remainder(angles[a] * (n / 2), 180.0)) < 1e-9 && strstr(why, "at once"))
                        continue;
                    if ((got == RB_OK) != want || (why == NULL) != want || (!want && got != RB_ERR_VALUE)) {
                        fprintf(stderr, "layout %d %d %d %.1f: offered %d, check %d\n", f, n, c, angles[a], want, got);
                        wrong++;
                    }
                    checked++;
                }
    CHECK_EQ_INT(wrong, 0);
    CHECK(checked > 1000);
    CHECK_EQ_INT(rb_layout_check(RB_FAMILY_COUNT, 4, RB_CRANK_EVEN, 0.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_layout_check(RB_FAMILY_V, 8, RB_CRANK_COUNT, 90.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_layout_check(RB_FAMILY_V, 8, RB_CRANK_EVEN, NAN), RB_ERR_VALUE);
    CHECK(strstr(rb_layout_why(RB_FAMILY_INLINE, 8, RB_CRANK_EVEN, 0.0), "planned") != NULL);
    CHECK(strstr(rb_layout_why(RB_FAMILY_BOXER, 8, RB_CRANK_EVEN, 0.0), "planned") != NULL);
    CHECK(rb_layout_check(RB_FAMILY_V, 8, RB_CRANK_CROSS_PLANE, 90.0) == RB_OK);

    /* names, the usual crank and angles, and a tier for each */
    for (int f = 0; f < RB_FAMILY_COUNT; f++) CHECK(rb_family_name(f) != NULL);
    for (int c = 0; c < RB_CRANK_COUNT; c++) CHECK(rb_crank_name(c) != NULL);
    CHECK(rb_family_name(-1) == NULL && rb_crank_name(RB_CRANK_COUNT) == NULL);
    CHECK_EQ_INT(rb_layout_usual_crank(RB_FAMILY_V, 2), RB_CRANK_COMMON_PIN);
    CHECK_EQ_INT(rb_layout_usual_crank(RB_FAMILY_V, 8), RB_CRANK_CROSS_PLANE);
    CHECK_EQ_INT(rb_layout_usual_crank(RB_FAMILY_V, 12), RB_CRANK_EVEN);
    CHECK_EQ_INT(rb_layout_usual_crank(RB_FAMILY_INLINE, 7), RB_ERR_VALUE);
    CHECK(rb_layout_angle(RB_FAMILY_V, 12, RB_CRANK_EVEN, RB_ANGLE_USUAL) == 60.0);
    CHECK(rb_layout_angle(RB_FAMILY_V, 10, RB_CRANK_EVEN, RB_ANGLE_USUAL) == 72.0);
    CHECK(rb_layout_angle(RB_FAMILY_V, 6, RB_CRANK_EVEN, RB_ANGLE_MIN) == 30.0);
    CHECK(rb_layout_angle(RB_FAMILY_V, 6, RB_CRANK_EVEN, RB_ANGLE_MAX) == 120.0);
    CHECK(rb_layout_angle(RB_FAMILY_BOXER, 6, RB_CRANK_EVEN, RB_ANGLE_USUAL) == 0.0);
    CHECK(isnan(rb_layout_angle(RB_FAMILY_BOXER, 8, RB_CRANK_EVEN, RB_ANGLE_USUAL)));
    combo list[64];
    const int k = all_combos(list);
    CHECK_EQ_INT(k, 6 + 2 + 8 * 2 + 1 + 3);
    for (int i = 0; i < k; i++) {
        const int q = rb_layout_quality(list[i].family, list[i].n, list[i].crank);
        CHECK(q == RB_QUALITY_BALANCED || q == RB_QUALITY_LOW || q == RB_QUALITY_LEAN);
    }
}

void test_layout_presets_are_their_own_layouts(void)
{
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_layout own;
        rb_engine_desc want, got, other;
        CHECK(rb_preset_layout_of(p, &own) == 0);
        rb_preset_desc(p, &want);
        CHECK_EQ_INT(own.cylinders, want.cylinders);
        CHECK(rb_layout_desc(p, &own, &got) == 0);
        CHECK(memcmp(&got, &want, sizeof got) == 0);   /* the preset itself, whole */

        /* its crank made from another preset: the same banks, pins and firing */
        rb_preset_desc((p + 1) % rb_preset_count(), &other);
        CHECK(rb_layout_crank_into(&own, &other) == 0);
        CHECK_EQ_INT(other.cylinders, want.cylinders);
        CHECK_EQ_INT(other.banks, want.banks);
        int differ = 0;
        for (int b = 0; b < want.banks; b++) differ += fabs(other.bank_angle[b] - want.bank_angle[b]) > 1e-9;
        for (int i = 0; i < want.cylinders; i++) {
            differ += other.bank[i] != want.bank[i];
            differ += fabs(remainder(other.pin[i] - want.pin[i], 360.0)) > 1e-9;
            differ += other.firing_order[i] != want.firing_order[i];
            differ += fabs(other.firing[i] - want.firing[i]) > 1e-9;
        }
        if (differ) fprintf(stderr, "preset %s: its layout's crank differs in %d places\n", rb_preset_key(p), differ);
        CHECK_EQ_INT(differ, 0);
        CHECK_EQ_INT((int)rb_preset_layout(p, RB_LAYOUT_CYLINDERS), want.cylinders);
        CHECK_EQ_INT((int)rb_preset_layout(p, RB_LAYOUT_GENERATED), 0);
    }
    CHECK(isnan(rb_preset_layout(-1, RB_LAYOUT_FAMILY)));
    CHECK(isnan(rb_preset_layout(0, RB_LAYOUT_COUNT)));
}

void test_layout_fires_as_stated(void)
{
    combo list[64];
    const int k = all_combos(list);
    int checked = 0;
    for (int i = 0; i < k; i++) {
        const combo c = list[i];
        const double angles[3] = {
            rb_layout_angle(c.family, c.n, c.crank, RB_ANGLE_USUAL),
            rb_layout_angle(c.family, c.n, c.crank, RB_ANGLE_MIN),
            rb_layout_angle(c.family, c.n, c.crank, RB_ANGLE_MAX),
        };
        for (int a = 0; a < 3; a++) {
            const rb_layout l = { c.family, c.n, c.crank, angles[a] };
            rb_engine_desc d;
            if (rb_layout_check(c.family, c.n, c.crank, angles[a]) != RB_OK) continue;   /* cross-plane far from 90 */
            CHECK(rb_layout_desc(0, &l, &d) == 0);
            CHECK_EQ_INT(d.cylinders, c.n);
            CHECK(d.firing_order[0] == 0 && d.firing[0] == 0.0);
            CHECK_EQ_INT(pins_off_their_tdc(&d), 0);
            double gap[RB_MAX_CYLINDERS];
            const int n_gaps = gaps_of(&d, -1, 0, gap);
            double sum = 0.0, smallest = 720.0;
            for (int g = 0; g < n_gaps; g++) {
                sum += gap[g];
                smallest = fmin(smallest, gap[g]);
            }
            CHECK_NEAR(sum, 720.0, 1e-9);   /* each cylinder once a cycle */
            CHECK(smallest > 1.0);
            if (c.crank == RB_CRANK_EVEN) {
                CHECK(all_gaps(&d, -1, 0, 720.0 / c.n));
                /* each bank evenly too: a V's and an odd-rowed boxer's */
                if (c.family == RB_FAMILY_V || (c.family == RB_FAMILY_BOXER && (c.n / 2) % 2 == 1))
                    CHECK(all_gaps(&d, 0, 0, 1440.0 / c.n) && all_gaps(&d, 1, 0, 1440.0 / c.n));
            }
            /* each exhaust collector fed evenly where there are two (a V's banks, the six's halves) */
            if (c.crank == RB_CRANK_EVEN && d.exhaust_systems + 0 >= 1 && d.collector[d.cylinders - 1] == 1
                && c.family != RB_FAMILY_BOXER)
                CHECK(all_gaps(&d, 0, 1, 1440.0 / c.n) && all_gaps(&d, 1, 1, 1440.0 / c.n));
            checked++;
        }
    }
    CHECK(checked >= k + 2 * 17);

    /* the known uneven ones, at their angles */
    rb_engine_desc d;
    static const double v6_odd[2] = { 150.0, 90.0 }, twin45[2] = { 315.0, 405.0 }, v10_odd[2] = { 90.0, 54.0 };
    static const double twin180[2] = { 180.0, 540.0 }, twin270[2] = { 270.0, 450.0 };
    rb_layout l = { RB_FAMILY_V, 6, RB_CRANK_COMMON_PIN, 90.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && gaps_repeat(&d, v6_odd, 2));
    l = (rb_layout){ RB_FAMILY_V, 2, RB_CRANK_COMMON_PIN, 45.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && gaps_repeat(&d, twin45, 2));
    l = (rb_layout){ RB_FAMILY_V, 10, RB_CRANK_COMMON_PIN, 90.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && gaps_repeat(&d, v10_odd, 2));
    l = (rb_layout){ RB_FAMILY_V, 12, RB_CRANK_COMMON_PIN, 60.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && all_gaps(&d, -1, 0, 60.0));
    l = (rb_layout){ RB_FAMILY_INLINE, 2, RB_CRANK_TWIN_180, 0.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && gaps_repeat(&d, twin180, 2));
    l = (rb_layout){ RB_FAMILY_INLINE, 2, RB_CRANK_TWIN_270, 0.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && gaps_repeat(&d, twin270, 2));
    /* even firing at 60 degrees splits a V6's pins by 60; a V12 at 60 shares them */
    l = (rb_layout){ RB_FAMILY_V, 6, RB_CRANK_EVEN, 60.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0 && fabs(remainder(d.pin[1] - d.pin[0], 360.0)) == 60.0);
    l = (rb_layout){ RB_FAMILY_V, 12, RB_CRANK_EVEN, 60.0 };
    CHECK(rb_layout_desc(0, &l, &d) == 0);
    int split = 0;
    for (int r = 0; r < 6; r++) split += fabs(remainder(d.pin[2 * r + 1] - d.pin[2 * r], 360.0)) > 1e-9;
    CHECK_EQ_INT(split, 0);

    /* running, a few of each family spark in that order at those angles */
    static const rb_layout run[] = {
        { RB_FAMILY_INLINE, 1, RB_CRANK_EVEN, 0.0 }, { RB_FAMILY_INLINE, 3, RB_CRANK_EVEN, 0.0 },
        { RB_FAMILY_INLINE, 5, RB_CRANK_EVEN, 0.0 }, { RB_FAMILY_INLINE, 2, RB_CRANK_TWIN_270, 0.0 },
        { RB_FAMILY_V, 2, RB_CRANK_COMMON_PIN, 45.0 }, { RB_FAMILY_V, 6, RB_CRANK_COMMON_PIN, 90.0 },
        { RB_FAMILY_V, 6, RB_CRANK_EVEN, 60.0 }, { RB_FAMILY_V, 10, RB_CRANK_EVEN, 72.0 },
        { RB_FAMILY_V, 16, RB_CRANK_EVEN, 45.0 }, { RB_FAMILY_BOXER, 2, RB_CRANK_EVEN, 0.0 },
        { RB_FAMILY_BOXER, 6, RB_CRANK_EVEN, 0.0 },
    };
    for (size_t r = 0; r < sizeof run / sizeof run[0]; r++) {
        int sparks, out_of_order, misplaced;
        CHECK(rb_layout_desc(0, &run[r], &d) == 0);
        count_sparks(&d, &sparks, &out_of_order, &misplaced);
        CHECK(sparks >= 2 * d.cylinders - 1 && sparks <= 2 * d.cylinders + 1);
        CHECK_EQ_INT(out_of_order, 0);
        CHECK_EQ_INT(misplaced, 0);
    }
}

void test_layout_keeps_the_presets_part(void)
{
    /* a V10 from the flat-plane V8: its cylinder, cams and limits, two exhausts, the running
     * set worked out from its size */
    const int p = rb_preset_find("v8-flat");
    rb_engine_desc b, d;
    rb_preset_desc(p, &b);
    const rb_layout v10 = { RB_FAMILY_V, 10, RB_CRANK_EVEN, 72.0 };
    CHECK(rb_layout_desc(p, &v10, &d) == 0);
    CHECK(d.bore == b.bore && d.stroke == b.stroke && d.rod == b.rod && d.cr == b.cr);
    CHECK(memcmp(&d.intake, &b.intake, sizeof d.intake) == 0 && memcmp(&d.exhaust, &b.exhaust, sizeof d.exhaust) == 0);
    CHECK(d.rev_limit == b.rev_limit && d.burn_duration == b.burn_duration && d.muffler_type == b.muffler_type);
    CHECK(d.fmep_constant == b.fmep_constant && d.header_diameter == b.header_diameter);
    CHECK_NEAR(litres(&d), litres(&b) * 10.0 / 8.0, 1e-9);
    CHECK_EQ_INT(d.exhaust_systems, 2);
    int wrong = 0;
    for (int i = 0; i < 10; i++) wrong += d.collector[i] != d.bank[i] || !(d.header_length[i] >= 0.28);
    CHECK_EQ_INT(wrong, 0);
    /* the idle's loop as quick as the four's: gain x valve / inertia */
    rb_engine_desc four;
    rb_preset_desc(0, &four);
    CHECK_NEAR(d.idle_gain_p * d.idle_valve_area / d.inertia, four.idle_gain_p * four.idle_valve_area / four.inertia, 1e-15);
    CHECK_NEAR(d.inertia, 0.08 * litres(&d), 1e-12);
    CHECK(d.throttle.diameter > b.throttle.diameter && d.plenum_volume > b.plenum_volume);

    /* an inline six from the four: two collectors, the front and rear three, each fed evenly */
    const rb_layout i6 = { RB_FAMILY_INLINE, 6, RB_CRANK_EVEN, 0.0 };
    CHECK(rb_layout_desc(0, &i6, &d) == 0);
    CHECK(d.collector[0] == 0 && d.collector[2] == 0 && d.collector[3] == 1 && d.collector[5] == 1);
    CHECK_EQ_INT(d.exhaust_systems, 1);
    CHECK(all_gaps(&d, 0, 1, 240.0) && all_gaps(&d, 1, 1, 240.0));

    /* a boxer four from the six: one collector, the far bank's headers crossing under */
    const rb_layout b4 = { RB_FAMILY_BOXER, 4, RB_CRANK_EVEN, 0.0 };
    CHECK(rb_layout_desc(rb_preset_find("i6"), &b4, &d) == 0);
    CHECK_EQ_INT(d.exhaust_systems, 1);
    CHECK(d.header_length[1] > d.header_length[0] + 0.25 && d.header_length[3] > d.header_length[2] + 0.25);

    /* the cross-plane V8's X junction stays only with two systems */
    const rb_layout v4 = { RB_FAMILY_V, 4, RB_CRANK_EVEN, 90.0 };
    CHECK(rb_layout_desc(rb_preset_find("v8-cross"), &v4, &d) == 0);
    CHECK(d.exhaust_systems == 1 && d.junction_volume == 0.0 && d.junction_before_center == 0);
    const rb_layout v12 = { RB_FAMILY_V, 12, RB_CRANK_EVEN, 60.0 };
    CHECK(rb_layout_desc(rb_preset_find("v8-cross"), &v12, &d) == 0);
    CHECK(d.exhaust_systems == 2 && d.junction_volume > 0.0);
    CHECK(rb_layout_desc(99, &v12, &d) == -1);
}

void test_layout_api(void)
{
    rb_sim *sim = rb_create_with_quality(48000, RB_QUALITY_LOW);
    CHECK(sim != NULL);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    CHECK_EQ_INT(rb_load_layout(sim, 0, RB_FAMILY_V, 16, RB_CRANK_EVEN, 45.0), RB_OK);
    CHECK_EQ_INT((int)rb_get_engine(sim, RB_ENG_CYLINDERS, 0), 16);
    CHECK_EQ_INT((int)rb_get_engine(sim, RB_ENG_BANKS, 0), 2);
    CHECK_EQ_INT(rb_get_preset(sim), 0);
    CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_FAMILY), RB_FAMILY_V);
    CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_CYLINDERS), 16);
    CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_CRANK), RB_CRANK_EVEN);
    CHECK(rb_get_layout(sim, RB_LAYOUT_BANK_ANGLE) == 45.0);
    CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_GENERATED), 1);
    CHECK(isnan(rb_get_layout(sim, RB_LAYOUT_COUNT)) && isnan(rb_get_layout(NULL, RB_LAYOUT_FAMILY)));

    /* 16 cylinders' state: every cylinder, a snapshot, and sound */
    CHECK(!isnan(rb_get_cylinder(sim, 15, RB_CYL_PRESSURE)) && isnan(rb_get_cylinder(sim, 16, RB_CYL_PRESSURE)));
    static float out[4800];
    CHECK_EQ_INT(rb_render_block(sim, out, 4800), 4800);
    static unsigned char snap[1 << 16];
    const int size = rb_snapshot_size(sim);
    CHECK(size > 0 && size <= (int)sizeof snap);
    CHECK_EQ_INT(rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, snap, sizeof snap), size);
    /* a quality change keeps it */
    CHECK_EQ_INT(rb_set_quality(sim, RB_QUALITY_LEAN), RB_OK);
    CHECK_EQ_INT((int)rb_get_engine(sim, RB_ENG_CYLINDERS, 0), 16);
    CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_CYLINDERS), 16);

    /* refused whole: the engine as it was */
    const double bore = rb_get_engine(sim, RB_ENG_BORE, 0);
    CHECK_EQ_INT(rb_load_layout(sim, 0, RB_FAMILY_INLINE, 8, RB_CRANK_EVEN, 0.0), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_load_layout(sim, 0, RB_FAMILY_V, 9, RB_CRANK_EVEN, 90.0), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_load_layout(sim, 0, RB_FAMILY_V, 8, RB_CRANK_EVEN, 140.0), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_load_layout(sim, 0, 7, 8, RB_CRANK_EVEN, 90.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_load_layout(sim, 42, RB_FAMILY_V, 8, RB_CRANK_EVEN, 90.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_load_layout(NULL, 0, RB_FAMILY_V, 8, RB_CRANK_EVEN, 90.0), RB_ERR_ARG);
    CHECK_EQ_INT((int)rb_get_engine(sim, RB_ENG_CYLINDERS, 0), 16);
    CHECK(rb_get_engine(sim, RB_ENG_BORE, 0) == bore);
    CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_CYLINDERS), 16);

    /* a preset's own layout is the preset: not generated, and it renders the same */
    rb_sim *plain = rb_create_with_quality(48000, RB_QUALITY_LOW);
    CHECK(plain != NULL);
    if (plain) {
        rb_set_param(plain, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_load_preset(plain, 1);
        CHECK_EQ_INT(rb_set_quality(sim, RB_QUALITY_LOW), RB_OK);
        CHECK_EQ_INT(rb_load_layout(sim, 1, RB_FAMILY_INLINE, 6, RB_CRANK_EVEN, 0.0), RB_OK);
        CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_GENERATED), 0);
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000.0);
        rb_set_param(plain, RB_PARAM_HOLD_RPM, 3000.0);
        static float a[4800], b[4800];
        rb_render_block(sim, a, 4800);
        rb_render_block(plain, b, 4800);
        CHECK(memcmp(a, b, sizeof a) == 0);
        rb_load_preset(sim, 2);
        CHECK_EQ_INT((int)rb_get_layout(sim, RB_LAYOUT_CRANK), RB_CRANK_CROSS_PLANE);
        rb_destroy(plain);
    }
    rb_destroy(sim);
}

void test_layout_every_combination_runs(void)
{
    /* As test_presets_run_on_their_own asks of the presets, for every combination at its usual
     * angle, started from the four: cranked by its starter it runs within a second, idles
     * 30 s and holds its idle speed through the last 20 s without hunting, comes back from a
     * blip, revs into its limiter floored and comes back to idle, carries a load scaled to its
     * size (5 N m per litre: the four's 10) and stops with the ignition off.
     *
     * About a minute of engine each, 4 to 10 minutes of computer: the `layouts` suite
     * (tools/run_tests.py) runs them in parallel processes, each taking its share
     * (RB_LAYOUT_SHARD=k/N: the combinations c with c % N == k). Without it this test says
     * so and runs none: the native suite would take hours. */
    combo list[64];
    const int k = all_combos(list);
    int shard = 0, shards = 0;
    const char *env = getenv("RB_LAYOUT_SHARD");
    if (!env || sscanf(env, "%d/%d", &shard, &shards) != 2 || shards < 1 || shard < 0 || shard >= shards) {
        fprintf(stderr, "    skipped: every combination's drive runs in the layouts suite (RB_LAYOUT_SHARD=k/N)\n");
        return;
    }
    for (int c = shard; c < k; c += shards) {
        const rb_layout l = { list[c].family, list[c].n, list[c].crank,
                              rb_layout_angle(list[c].family, list[c].n, list[c].crank, RB_ANGLE_USUAL) };
        rb_engine_desc d;
        CHECK(rb_layout_desc(0, &l, &d) == 0);
        char name[64];
        snprintf(name, sizeof name, "%s %d %s", rb_family_name(l.family), l.cylinders, rb_crank_name(l.crank));
        rb_engine_init(&e, &d, DT_FAST);
        e.hold_speed = 0;
        e.throttle_pos = 0.0;
        e.ignition = 1;
        e.starter = 1;
        long t = 0;
        const long second = lround(1.0 / DT_FAST);
        while (t < second && e.rpm_smooth < 0.75 * d.idle_rpm) {
            rb_engine_step(&e);
            t++;
        }
        /* a second: a single pushes through one compression a turn and a half (0.57 s) */
        const int started = t < second;
        e.starter = 0;
        const double start_s = t * DT_FAST;
        long cycles = e.cycles;
        double lo = INFINITY, hi = 0.0, sum = 0.0, swing = INFINITY;
        int n = 0;
        while (t < lround(30.0 / DT_FAST)) {
            rb_engine_step(&e);
            t++;
            /* the lowest instant of the settled idle: a single's speed swings tens of rpm
             * inside each cycle, so the dips below are measured from there */
            if (t > lround(10.0 / DT_FAST)) swing = fmin(swing, e.rpm);
            if (e.cycles != cycles && t > lround(10.0 / DT_FAST)) {
                lo = fmin(lo, e.result.rpm);
                hi = fmax(hi, e.result.rpm);
                sum += e.result.rpm;
                n++;
            }
            cycles = e.cycles;
        }
        const double mean = n ? sum / n : 0.0, valve = e.idle_valve;

        double dip = INFINITY;
        e.throttle_pos = 0.3;
        for (long j = 0; j < lround(0.25 / DT_FAST); j++) rb_engine_step(&e);
        e.throttle_pos = 0.0;
        for (long j = 0; j < lround(4.0 / DT_FAST); j++) {   /* 4 s: at 3 the V12 is still 30 rpm low */
            rb_engine_step(&e);
            dip = fmin(dip, e.rpm);
        }
        const double after_blip = e.result.rpm;

        double top = 0.0;
        int cut = 0;
        const long cuts0 = e.limiter_cuts;
        e.throttle_pos = 1.0;   /* 4 s: the V12, heaviest for its power, needs 3 to reach the cut */
        for (long j = 0; j < lround(4.0 / DT_FAST); j++) {
            rb_engine_step(&e);
            top = fmax(top, e.rpm);
            cut |= e.spark_cut;
        }
        e.throttle_pos = 0.0;
        double low = INFINITY;
        for (long j = 0; j < lround(5.0 / DT_FAST); j++) {
            rb_engine_step(&e);
            low = fmin(low, e.rpm);
        }
        const double back = e.result.rpm;

        /* the load last, from a settled idle: thrown on before the rev, the idle valve the
         * computer learned under it was still open as the engine came back from the limiter */
        e.load_torque = 5.0 * litres(&d);
        double sag = INFINITY;
        for (long j = 0; j < lround(8.0 / DT_FAST); j++) {
            rb_engine_step(&e);
            sag = fmin(sag, e.rpm);
        }
        const double loaded = e.result.rpm;
        e.load_torque = 0.0;
        e.ignition = 0;
        for (long j = 0; j < lround(2.0 / DT_FAST); j++) rb_engine_step(&e);

        const int ok = started && n >= 8 && fabs(mean - d.idle_rpm) < 10.0 && hi - lo < 15.0
                    && valve > 0.08 && valve < 0.25 && dip > swing - 80.0
                    && fabs(after_blip - d.idle_rpm) < 30.0 && sag > 0.4 * d.idle_rpm
                    && fabs(loaded - d.idle_rpm) < 40.0 && (cut || e.limiter_cuts > cuts0)
                    && top > d.rev_limit - 50.0 && top < d.rev_limit + 250.0
                    && fabs(back - d.idle_rpm) < 30.0 && low > swing - 120.0 && e.rpm == 0.0;
        fprintf(stderr, "    %-28s %s start %.2f s idle %.0f (%.0f..%.0f, lowest %.0f) valve %.2f dip %.0f then %.0f load %.0f (sag %.0f) top %.0f%s back %.0f low %.0f\n",
                name, ok ? "ok  " : "FAIL", start_s, mean, lo, hi, swing, valve, dip, after_blip, loaded, sag, top,
                (cut || e.limiter_cuts > cuts0) ? " cut" : " no cut", back, low);
        CHECK(ok);
    }
}

void test_layout_refuses_an_exhaust_far_wider_than_its_headers(void)
{
    /* A twin's two 36 mm headers into a 131 mm pipe (6.6 times their area): the fuzz found its
     * pipes losing their state at the slow pipe rates. The description check refuses a pipe
     * over 6 times the headers joining it; the six's 149 mm (5.7 times its three) stays. */
    rb_engine_desc d;
    const rb_layout twin = { RB_FAMILY_INLINE, 2, RB_CRANK_TWIN_270, 0.0 };
    CHECK(rb_layout_desc(rb_preset_find("i6"), &twin, &d) == 0);
    d.exhaust_pipe_diameter = 0.131;
    CHECK_EQ_INT(rb_engine_desc_check(&d), -1);
    d.exhaust_pipe_diameter = 0.088;   /* 5.98 times */
    CHECK_EQ_INT(rb_engine_desc_check(&d), 0);
    rb_preset_desc(rb_preset_find("i6"), &d);
    d.exhaust_pipe_diameter = 0.149;
    CHECK_EQ_INT(rb_engine_desc_check(&d), 0);
    /* every preset at its field's largest pipe still builds (the editor's range unchanged) */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_preset_desc(p, &d);
        d.exhaust_pipe_diameter = rb_engine_field_max(RB_ENG_EXHAUST_PIPE_DIAMETER);
        if (rb_engine_desc_check(&d) != 0) fprintf(stderr, "preset %s refuses its largest pipe\n", rb_preset_key(p));
        CHECK_EQ_INT(rb_engine_desc_check(&d), 0);
    }
}
