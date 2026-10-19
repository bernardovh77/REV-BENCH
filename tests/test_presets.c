/*
 * test_presets.c - engine layouts and presets (stage 8): the layout rules,
 * every preset sparking where its layout says and running on its own, and
 * the engine through the public API.
 */
#include "presets.h"
#include "rb_internal.h"
#include "rb_test.h"

#include <math.h>
#include <string.h>

/* Half the default step: the idle, the limiter and the sparks come out the
 * same as at 192 kHz (docs/stage-08.md), in half the time. */
#define DT_FAST (1.0 / 96000.0)
#define PI      3.14159265358979323846

static rb_engine e;   /* kept off the stack */

/* The gaps (degrees) between one firing and the next in the firing order:
 * of all the cylinders (bank -1) or of one bank. Returns how many. */
static int firing_gaps(const rb_engine_desc *d, int bank, double *gap)
{
    double first = -1.0, prev = -1.0;
    int n = 0;
    for (int k = 0; k < d->cylinders; k++) {
        const int i = d->firing_order[k];
        if (bank >= 0 && d->bank[i] != bank) continue;
        if (prev < 0.0) first = d->firing[i];
        else gap[n++] = d->firing[i] - prev;
        prev = d->firing[i];
    }
    gap[n++] = first + 720.0 - prev;
    return n;
}

/* 1 if the gaps are `want` (n of them) in this order, starting anywhere. */
static int gaps_are(const double *gap, int n, const double *want, int n_want)
{
    if (n != n_want) return 0;
    for (int s = 0; s < n; s++) {
        int same = 1;
        for (int k = 0; k < n; k++) same = same && fabs(gap[(s + k) % n] - want[k]) < 1e-9;
        if (same) return 1;
    }
    return 0;
}

static int all_gaps(const rb_engine_desc *d, int bank, double want)
{
    double gap[RB_MAX_CYLINDERS];
    const int n = firing_gaps(d, bank, gap);
    int ok = 1;
    for (int k = 0; k < n; k++) ok = ok && fabs(gap[k] - want) < 1e-9;
    return ok;
}

void test_presets_table(void)
{
    /* Five presets: each a possible engine of the size its name says, firing
     * in the order presets.h gives, found by its key, with its own idle valve
     * and idle gains. */
    static const char *keys[] = { "i4", "i6", "v8-cross", "v8-flat", "boxer4" };
    static const double litres[] = { 2.0, 3.0, 5.0, 4.5, 2.0 };
    static const int cylinders[] = { 4, 6, 8, 8, 4 };
    static const int orders[5][8] = { { 1, 3, 4, 2 }, { 1, 5, 3, 6, 2, 4 },
                                      { 1, 8, 4, 3, 6, 5, 7, 2 }, { 1, 6, 5, 8, 7, 4, 3, 2 },
                                      { 1, 3, 2, 4 } };
    CHECK_EQ_INT(rb_preset_count(), 5);
    for (int p = 0; p < 5; p++) {
        rb_engine_desc d;
        CHECK_EQ_INT(rb_preset_desc(p, &d), 0);
        CHECK_EQ_INT(rb_engine_desc_check(&d), 0);
        CHECK_EQ_INT(d.cylinders, cylinders[p]);
        int out_of_order = 0;
        for (int k = 0; k < d.cylinders; k++) out_of_order += d.firing_order[k] + 1 != orders[p][k];
        CHECK_EQ_INT(out_of_order, 0);
        CHECK(rb_preset_key(p) && strcmp(rb_preset_key(p), keys[p]) == 0);
        CHECK(rb_preset_name(p) && strlen(rb_preset_name(p)) > 5);
        CHECK_EQ_INT(rb_preset_find(keys[p]), p);
        CHECK_EQ_INT(rb_engine_init(&e, &d, DT_FAST), 0);
        CHECK_NEAR(rb_engine_displacement(&e) * 1e3, litres[p], 0.01 * litres[p]);
        CHECK(d.idle_valve_area > 0.0 && d.idle_gain_p > 0.0 && d.idle_gain_i > 0.0);
        /* The cylinders of a bank are at least a bore apart along the crank. */
        int crowded = 0;
        for (int i = 0; i < d.cylinders; i++)
            for (int j = 0; j < i; j++)
                if (d.bank[i] == d.bank[j] && fabs(d.position[i] - d.position[j]) < d.bore) crowded++;
        CHECK_EQ_INT(crowded, 0);
    }
    /* Each preset's idle settings are its own: the eights' valves are bigger,
     * and the cross-plane V8, with 12 L of plenum and runners to fill, opens
     * its valve faster than the four. */
    rb_engine_desc i4, v8;
    rb_preset_desc(0, &i4);
    rb_preset_desc(2, &v8);
    CHECK(v8.idle_valve_area > 1.5 * i4.idle_valve_area);
    CHECK(v8.idle_gain_p > i4.idle_gain_p);

    rb_engine_desc d, before;
    memset(&d, 0x5a, sizeof d);
    before = d;
    CHECK_EQ_INT(rb_preset_desc(-1, &d), -1);
    CHECK_EQ_INT(rb_preset_desc(5, &d), -1);
    CHECK(memcmp(&d, &before, sizeof d) == 0);   /* untouched */
    CHECK_EQ_INT(rb_preset_desc(0, NULL), -1);
    CHECK(rb_preset_key(5) == NULL && rb_preset_name(-1) == NULL);
    CHECK_EQ_INT(rb_preset_find("v12"), RB_ERR_ID);
    CHECK_EQ_INT(rb_preset_find(NULL), RB_ERR_ARG);
}

/* Runs the engine held at 3000 rpm wide open for two cycles after two to
 * warm up. Counts the sparks, the ones out of firing order, and the ones not
 * where they should be: at the spark angle of the cylinder's own cycle plus
 * its firing angle, within the step that passes it. */
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

void test_presets_fire_where_the_layout_says(void)
{
    /* Every preset fires evenly: the fours every 180 degrees, the six every
     * 120, the eights every 90. The cross-plane V8's banks each fire
     * unevenly (90-180-270-180, the burble), the flat-plane's every 180 like
     * two fours, and the boxer's banks 180 then 540 apart. Each cylinder's
     * firing TDC is where its pin points along its bank. And running, the
     * cylinders spark in that order at those angles. */
    static const double cross[4] = { 90.0, 180.0, 270.0, 180.0 };
    static const double boxer[2] = { 180.0, 540.0 };
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        double gap[RB_MAX_CYLINDERS];
        rb_preset_desc(p, &d);
        CHECK(all_gaps(&d, -1, 720.0 / d.cylinders));
        CHECK(d.firing[d.firing_order[0]] == 0.0);
        int wrong = 0;
        for (int i = 0; i < d.cylinders; i++) {
            const double tdc = (d.bank_angle[d.bank[i]] - d.pin[i]) - (d.bank_angle[d.bank[0]] - d.pin[0]);
            if (fabs(remainder(tdc - d.firing[i], 360.0)) > 1e-9) wrong++;
        }
        CHECK_EQ_INT(wrong, 0);
        if (p == 2) {   /* v8-cross */
            for (int b = 0; b < 2; b++) CHECK(gaps_are(gap, firing_gaps(&d, b, gap), cross, 4));
        } else if (p == 3) {   /* v8-flat */
            CHECK(all_gaps(&d, 0, 180.0) && all_gaps(&d, 1, 180.0));
        } else if (p == 4) {   /* boxer4 */
            for (int b = 0; b < 2; b++) CHECK(gaps_are(gap, firing_gaps(&d, b, gap), boxer, 2));
        }

        int sparks, out_of_order, misplaced;
        count_sparks(&d, &sparks, &out_of_order, &misplaced);
        CHECK(sparks >= 2 * d.cylinders - 1 && sparks <= 2 * d.cylinders + 1);
        CHECK_EQ_INT(out_of_order, 0);
        CHECK_EQ_INT(misplaced, 0);
    }
}

/* 1 if rb_engine_layout refuses a description and leaves it as it was. */
static int refused(rb_engine_desc x)
{
    const rb_engine_desc before = x;
    return rb_engine_layout(&x) == -1 && memcmp(&x, &before, sizeof x) == 0;
}

void test_layout_rules(void)
{
    /* rb_engine_layout works out the firing angles from the crank, the banks
     * and the firing order, and refuses layouts that can't work, leaving the
     * description as it was. */
    rb_engine_desc d, bad;
    rb_engine_desc_default(&d);   /* 1-3-4-2 on a flat crank */
    CHECK_EQ_INT(rb_engine_layout(&d), 0);
    CHECK(d.firing[0] == 0.0 && d.firing[2] == 180.0 && d.firing[3] == 360.0 && d.firing[1] == 540.0);

    bad = d; bad.firing_order[1] = 0;                            /* cylinder 1 twice */
    CHECK(refused(bad));
    bad = d; bad.firing_order[0] = 2; bad.firing_order[1] = 0;   /* 3-1-4-2: 1 not first */
    CHECK(refused(bad));
    bad = d; bad.firing_order[1] = 3; bad.firing_order[2] = 1; bad.firing_order[3] = 2;
    CHECK(refused(bad));                   /* 1-4-2-3 needs more than 720 degrees */
    bad = d; bad.firing_order[3] = 7;                            /* no cylinder 8 */
    CHECK(refused(bad));
    bad = d; bad.banks = 0;                CHECK(refused(bad));
    bad = d; bad.banks = 3;                CHECK(refused(bad));
    bad = d; bad.bank[2] = 1;              CHECK(refused(bad));   /* there's one bank */
    bad = d; bad.bank_angle[0] = 200.0;    CHECK(refused(bad));
    bad = d; bad.pin[1] = NAN;             CHECK(refused(bad));
    bad = d; bad.position[3] = NAN;        CHECK(refused(bad));
    bad = d; bad.cylinders = 0;            CHECK(refused(bad));
    bad = d; bad.cylinders = RB_MAX_CYLINDERS + 1; CHECK(refused(bad));
    CHECK_EQ_INT(rb_engine_layout(NULL), -1);

    /* A twin whose two pistons go up and down together, firing 360 degrees
     * apart: 1-2 works. Cylinder 1 twice would fit in the cycle, and so would
     * 2-1, which would put cylinder 2's firing where cylinder 1's is: both
     * refused. */
    rb_engine_desc twin = d;
    CHECK_EQ_INT(rb_engine_layout_inline(&twin, 2), 0);
    CHECK(twin.pin[0] == twin.pin[1] && twin.firing[1] == 360.0);
    bad = twin; bad.firing_order[1] = 0;                       CHECK(refused(bad));
    bad = twin; bad.firing_order[0] = 1; bad.firing_order[1] = 0; CHECK(refused(bad));

    /* The cross-plane V8 with its banks 60 degrees apart instead of 90 still
     * works, firing unevenly. */
    rb_engine_desc v8;
    double gap[RB_MAX_CYLINDERS];
    rb_preset_desc(2, &v8);
    v8.bank_angle[0] = 30.0;
    v8.bank_angle[1] = -30.0;
    CHECK_EQ_INT(rb_engine_layout(&v8), 0);
    CHECK_EQ_INT(rb_engine_desc_check(&v8), 0);
    const int n = firing_gaps(&v8, -1, gap);
    double lo = 720.0, hi = 0.0, sum = 0.0;
    for (int k = 0; k < n; k++) {
        lo = fmin(lo, gap[k]);
        hi = fmax(hi, gap[k]);
        sum += gap[k];
    }
    CHECK_NEAR(lo, 60.0, 1e-9);
    CHECK_NEAR(hi, 120.0, 1e-9);
    CHECK_NEAR(sum, 720.0, 1e-9);

    /* rb_engine_layout_inline: 1 to 16 cylinders in a row, firing evenly in
     * cylinder order. */
    for (int k = 1; k <= RB_MAX_CYLINDERS; k++) {
        rb_engine_desc x;
        rb_engine_desc_default(&x);
        CHECK_EQ_INT(rb_engine_layout_inline(&x, k), 0);
        CHECK_EQ_INT(rb_engine_desc_check(&x), 0);
        CHECK(x.cylinders == k && x.banks == 1 && all_gaps(&x, -1, 720.0 / k));
        for (int i = 0; i < k; i++) CHECK_EQ_INT(x.firing_order[i], i);
    }
    bad = d;
    CHECK_EQ_INT(rb_engine_layout_inline(&bad, 0), -1);
    CHECK_EQ_INT(rb_engine_layout_inline(&bad, RB_MAX_CYLINDERS + 1), -1);
    bad.bore = 0.7;   /* 16 of these in a row: more than 10 m of engine */
    rb_engine_desc before = bad;
    CHECK_EQ_INT(rb_engine_layout_inline(&bad, 16), -1);
    CHECK(memcmp(&bad, &before, sizeof bad) == 0);
}

void test_presets_run_on_their_own(void)
{
    /* Stage 8's "done when": every preset, with the crank free, is cranked by
     * its starter, starts, settles to its own idle speed and holds it without
     * hunting (each with its own idle gains), revs into its rev limiter
     * floored, comes back to idle with the foot off, and stops when the
     * ignition is switched off. */
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        rb_engine_init(&e, &d, DT_FAST);
        e.hold_speed = 0;
        e.throttle_pos = 0.0;
        e.ignition = 1;
        e.starter = 1;
        long k = 0;
        const long second = lround(1.0 / DT_FAST);
        while (k < second && e.rpm_smooth < 0.75 * d.idle_rpm) {
            rb_engine_step(&e);
            k++;
        }
        CHECK(k < second / 2);   /* runs within half a second */
        e.starter = 0;
        long cycles = e.cycles;
        double lo = INFINITY, hi = 0.0, sum = 0.0;
        int n = 0;
        while (k < lround(6.5 / DT_FAST)) {
            rb_engine_step(&e);
            k++;
            if (e.cycles != cycles && k > lround(4.5 / DT_FAST)) {   /* each cycle's mean */
                lo = fmin(lo, e.result.rpm);
                hi = fmax(hi, e.result.rpm);
                sum += e.result.rpm;
                n++;
            }
            cycles = e.cycles;
        }
        CHECK(n >= 8);
        CHECK_NEAR(sum / n, d.idle_rpm, 10.0);
        CHECK(hi - lo < 15.0);
        CHECK(e.idle_valve > 0.12 && e.idle_valve < 0.18);   /* sized to idle about 15 % open */

        /* A blip, 30 % for a quarter of a second: back at idle without
         * dipping much below it (with the four's gains the six dips 80 rpm). */
        double dip = INFINITY;
        e.throttle_pos = 0.3;
        for (long j = 0; j < lround(0.25 / DT_FAST); j++) rb_engine_step(&e);
        e.throttle_pos = 0.0;
        for (long j = 0; j < lround(3.0 / DT_FAST); j++) {
            rb_engine_step(&e);
            if (e.rpm < dip) dip = e.rpm;
        }
        CHECK(dip > d.idle_rpm - 55.0);
        CHECK_NEAR(e.result.rpm, d.idle_rpm, 30.0);

        double top = 0.0;
        int cut = 0;
        e.throttle_pos = 1.0;
        for (long j = 0; j < lround(1.2 / DT_FAST); j++) {
            rb_engine_step(&e);
            if (e.rpm > top) top = e.rpm;
            cut |= e.spark_cut;
        }
        CHECK(cut);
        CHECK(top > d.rev_limit && top < d.rev_limit + 250.0);

        e.throttle_pos = 0.0;   /* about 3 s down from the limiter, fuel cut */
        double low = INFINITY;
        for (long j = 0; j < lround(4.5 / DT_FAST); j++) {
            rb_engine_step(&e);
            if (e.rpm < low) low = e.rpm;
        }
        CHECK_NEAR(e.result.rpm, d.idle_rpm, 30.0);
        CHECK(low > d.idle_rpm - 100.0);   /* it lands without dipping much */

        e.ignition = 0;
        for (long j = 0; j < lround(1.5 / DT_FAST); j++) rb_engine_step(&e);
        CHECK(e.rpm == 0.0);
    }
}

void test_presets_idle_takes_a_load(void)
{
    /* A 10 N m load thrown on at idle, an air conditioner kicking in, say:
     * the six sags about 300 rpm without stalling and is back within 30 rpm of
     * its idle speed in under 7 s, the idle valve then open further. The
     * engine computer keeps learning the valve's position while the engine is
     * below idle, even before its speed steadies; waiting for it to steady
     * took 10 s (stage 9). */
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i6"), &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.hold_speed = 0;
    e.throttle_pos = 0.0;
    e.ignition = 1;
    e.starter = 1;
    for (long k = 0; k < lround(4.0 / DT_FAST); k++) {
        if (e.starter && e.rpm_smooth > 0.75 * d.idle_rpm) e.starter = 0;
        rb_engine_step(&e);
    }
    CHECK_NEAR(e.result.rpm, d.idle_rpm, 15.0);
    const double valve = e.idle_valve;

    e.load_torque = 10.0;
    double low = INFINITY, back = 0.0;
    long cycles = e.cycles;
    for (long k = 0; k < lround(10.0 / DT_FAST); k++) {
        rb_engine_step(&e);
        low = fmin(low, e.rpm);
        if (e.cycles != cycles) {
            cycles = e.cycles;
            if (fabs(e.result.rpm - d.idle_rpm) > 30.0) back = (k + 1) * DT_FAST;
        }
    }
    CHECK(low > d.idle_rpm - 400.0 && low < d.idle_rpm - 200.0);
    CHECK(back > 1.0 && back < 7.0);
    CHECK(e.idle_valve > valve + 0.04);
}

void test_presets_idle_on_the_spark_alone(void)
{
    /* With runners an idling engine is unstable on its own: the faster it
     * turns, the less exhaust its cylinders push back into their runners
     * during the overlap, so the better they breathe and the more torque
     * they make, before the plenum's pressure can follow (docs/stage-10.md).
     * The engine computer's idle spark has to take back more than that. With
     * its idle valve held still (no gains), the six idles on the spark alone,
     * steady to 5 rpm once settled; on stage 9's spark, 0.1 deg per rpm on
     * the speed now, it swung about 95 rpm every 2.8 s. */
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i6"), &d);
    d.idle_gain_p = d.idle_gain_i = 0.0;   /* the valve stays where the computer starts it */
    rb_engine_init(&e, &d, DT_FAST);
    e.hold_speed = 0;
    e.throttle_pos = 0.0;
    e.ignition = 1;
    e.starter = 1;
    double lo = INFINITY, hi = 0.0;
    long cycles = e.cycles;
    for (long k = 0; k < lround(20.0 / DT_FAST); k++) {
        if (e.starter && e.rpm_smooth > 0.75 * d.idle_rpm) e.starter = 0;
        rb_engine_step(&e);
        if (e.cycles != cycles) {
            cycles = e.cycles;
            if (k > lround(12.0 / DT_FAST)) {
                lo = fmin(lo, e.result.rpm);
                hi = fmax(hi, e.result.rpm);
            }
        }
    }
    CHECK(hi - lo < 5.0);
    CHECK_NEAR(0.5 * (lo + hi), d.idle_rpm, 30.0);
}

void test_api_engine_fields(void)
{
    /* The field table is complete: names unique and found again, units, and
     * only known flags. Every value through the API is the preset's own, and
     * setting a field to the value it has is accepted (read-only ones
     * refused) and changes nothing: the engine isn't even rebuilt. */
    CHECK_EQ_INT(rb_engine_field_count(), RB_ENG_COUNT);
    int bad_range = 0;
    for (int f = 0; f < RB_ENG_COUNT; f++) {
        const char *name = rb_engine_field_name(f);
        CHECK(name && name[0] && rb_engine_field_unit(f));
        if (name) CHECK_EQ_INT(rb_engine_field_find(name), f);
        CHECK((rb_engine_field_flags(f) & ~63) == 0);
        const int per = rb_engine_field_flags(f) & (RB_EF_PER_BANK | RB_EF_PER_CYLINDER | RB_EF_IN_ORDER);
        CHECK(per == 0 || per == RB_EF_PER_BANK || per == RB_EF_PER_CYLINDER || per == RB_EF_IN_ORDER);
        /* A range for everything that can be set, none for the rest. */
        const double lo = rb_engine_field_min(f), hi = rb_engine_field_max(f);
        if (rb_engine_field_flags(f) & RB_EF_READONLY) bad_range += !(isnan(lo) && isnan(hi));
        else bad_range += !(lo < hi);
    }
    CHECK_EQ_INT(bad_range, 0);
    CHECK(isnan(rb_engine_field_min(-1)) && isnan(rb_engine_field_max(RB_ENG_COUNT)));
    CHECK(rb_engine_field_name(-1) == NULL && rb_engine_field_name(RB_ENG_COUNT) == NULL);
    CHECK(rb_engine_field_unit(RB_ENG_COUNT) == NULL);
    CHECK_EQ_INT(rb_engine_field_flags(RB_ENG_COUNT), 0);
    CHECK_EQ_INT(rb_engine_field_find("wheelbase"), RB_ERR_ID);
    CHECK_EQ_INT(rb_engine_field_find(NULL), RB_ERR_ARG);
    CHECK_EQ_INT(rb_engine_field_size(NULL, RB_ENG_BORE), 0);

    rb_sim *s = rb_create(NULL);
    CHECK(s != NULL);
    if (!s) return;
    CHECK_EQ_INT(rb_get_preset(s), 0);
    CHECK_EQ_INT(rb_engine_field_size(s, -1), 0);
    CHECK_EQ_INT(rb_engine_field_size(s, RB_ENG_COUNT), 0);
    for (int p = 0; p < rb_preset_count(); p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        CHECK_EQ_INT(rb_load_preset(s, p), RB_OK);
        CHECK_EQ_INT(rb_get_preset(s), p);
        CHECK_EQ_INT(rb_engine_field_size(s, RB_ENG_BORE), 1);
        CHECK_EQ_INT(rb_engine_field_size(s, RB_ENG_BANK_ANGLE), d.banks);
        CHECK_EQ_INT(rb_engine_field_size(s, RB_ENG_PIN), d.cylinders);
        CHECK_EQ_INT(rb_engine_field_size(s, RB_ENG_FIRING_ORDER), d.cylinders);
        CHECK(rb_get_engine(s, RB_ENG_CYLINDERS, 0) == d.cylinders);
        CHECK(rb_get_engine(s, RB_ENG_BORE, 0) == d.bore && rb_get_engine(s, RB_ENG_ROD, 0) == d.rod);
        CHECK(rb_get_engine(s, RB_ENG_INTAKE_LIFT, 0) == d.intake.max_lift);
        CHECK(rb_get_engine(s, RB_ENG_EXHAUST_CENTER, 0) == d.exhaust.center);
        CHECK(rb_get_engine(s, RB_ENG_EXHAUST_VALVES, 0) == d.exhaust.count);
        CHECK(rb_get_engine(s, RB_ENG_PLENUM_VOLUME, 0) == d.plenum_volume);
        CHECK(rb_get_engine(s, RB_ENG_REV_LIMIT, 0) == d.rev_limit);
        CHECK(rb_get_engine(s, RB_ENG_RUNNER_LENGTH, 0) == d.runner_length);
        CHECK(rb_get_engine(s, RB_ENG_RUNNER_DIAMETER, 0) == d.runner_diameter);
        CHECK_NEAR(rb_get_engine(s, RB_ENG_DISPLACEMENT, 0), rb_engine_displacement(&s->engine), 0.0);
        CHECK(rb_get_engine(s, RB_ENG_INTAKE_OPENS, 0) == rb_valve_opens(&d.intake));
        CHECK(rb_get_engine(s, RB_ENG_EXHAUST_CLOSES, 0) == rb_valve_closes(&d.exhaust));
        int differ = 0;
        for (int i = 0; i < d.cylinders; i++) {
            differ += rb_get_engine(s, RB_ENG_BANK, i) != d.bank[i];
            differ += rb_get_engine(s, RB_ENG_PIN, i) != d.pin[i];
            differ += rb_get_engine(s, RB_ENG_POSITION, i) != d.position[i];
            differ += rb_get_engine(s, RB_ENG_FIRING_ORDER, i) != d.firing_order[i];
            differ += rb_get_engine(s, RB_ENG_FIRING_ANGLE, i) != d.firing[i];
        }
        for (int b = 0; b < d.banks; b++) differ += rb_get_engine(s, RB_ENG_BANK_ANGLE, b) != d.bank_angle[b];
        CHECK_EQ_INT(differ, 0);
        CHECK(isnan(rb_get_engine(s, RB_ENG_PIN, d.cylinders)));
        CHECK(isnan(rb_get_engine(s, RB_ENG_BANK_ANGLE, d.banks)));
        CHECK(isnan(rb_get_engine(s, RB_ENG_BORE, 1)) && isnan(rb_get_engine(s, RB_ENG_BORE, -1)));
        CHECK(isnan(rb_get_engine(s, RB_ENG_COUNT, 0)) && isnan(rb_get_engine(NULL, RB_ENG_BORE, 0)));

        /* Every preset sits inside the fields' ranges (else the loop below
         * gets RB_CLAMPED), and setting each field to the value it has is
         * accepted and changes nothing. */
        s->engine.theta = 123.0;   /* a mark: rebuilding the engine would reset it */
        int refused = 0, wrong_code = 0;
        for (int f = 0; f < RB_ENG_COUNT; f++) {
            for (int i = 0; i < rb_engine_field_size(s, f); i++) {
                const int rc = rb_set_engine(s, f, i, rb_get_engine(s, f, i));
                const int want = (rb_engine_field_flags(f) & RB_EF_READONLY) ? RB_ERR_READONLY : RB_OK;
                if (rc != want) wrong_code++;
                if (rc == RB_ERR_READONLY) refused++;
            }
        }
        CHECK_EQ_INT(wrong_code, 0);
        CHECK(refused >= 6 + 2 * d.cylinders - d.cylinders);   /* cylinders, displacement, banks, ... */
        CHECK(s->engine.theta == 123.0);
        CHECK(memcmp(&s->engine.desc, &d, sizeof d) == 0);
    }
    rb_destroy(s);
}

void test_api_engine_changes(void)
{
    rb_sim *s = rb_create(NULL);   /* the 2.0 L four */
    CHECK(s != NULL);
    if (!s) return;

    /* Most changes rebuild the engine, at rest. */
    s->engine.theta = 123.0;
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BORE, 0, 0.09), RB_OK);
    CHECK(s->engine.theta == 0.0 && s->engine.desc.bore == 0.09);
    CHECK_NEAR(rb_get_engine(s, RB_ENG_DISPLACEMENT, 0), 4 * PI / 4 * 0.09 * 0.09 * 0.086, 1e-15);

    /* The live ones change the running engine. The rev limiter keeps its
     * 100 rpm between cutting the spark and bringing it back. */
    s->engine.theta = 123.0;
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_REV_LIMIT, 0, 6000.0), RB_OK);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_IDLE_RPM, 0, 900.0), RB_OK);
    CHECK(s->engine.theta == 123.0);
    CHECK(s->engine.desc.rev_limit == 6000.0 && s->engine.desc.rev_limit_resume == 5900.0);
    CHECK(s->engine.desc.idle_rpm == 900.0);

    /* Refusals leave the engine exactly as it was. */
    const rb_engine_desc before = s->engine.desc;
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_ROD, 0, 0.03), RB_ERR_VALUE);   /* even clamped, shorter than the stroke */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BORE, 0, NAN), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BORE, 0, INFINITY), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_INTAKE_VALVES, 0, 2.5), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_INTAKE_DIAMETER, 0, 0.07), RB_ERR_VALUE);   /* 60 mm: won't fit */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_INTAKE_DIAMETER, 0, 0.05), RB_ERR_VALUE);   /* won't fit */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BANK, 2, 1.0), RB_ERR_VALUE);        /* there's one bank */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BANK, 2, 0.5), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_FIRING_ORDER, 1, 4.0), RB_ERR_VALUE);   /* no cylinder 5 */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_FIRING_ORDER, 1, -1.0), RB_ERR_VALUE);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_FIRING_ORDER, 0, 1.0), RB_ERR_VALUE);   /* 1 must be first */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_FIRING_ORDER, 1, 3.0), RB_ERR_VALUE);   /* 1-4-2-3 won't fit */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_CYLINDERS, 0, 6.0), RB_ERR_READONLY);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_FIRING_ANGLE, 1, 0.0), RB_ERR_READONLY);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_INTAKE_OPENS, 0, 0.0), RB_ERR_READONLY);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_PIN, 4, 0.0), RB_ERR_ARG);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BORE, 1, 0.08), RB_ERR_ARG);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_COUNT, 0, 1.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_set_engine(s, -1, 0, 1.0), RB_ERR_ID);
    CHECK_EQ_INT(rb_set_engine(NULL, RB_ENG_BORE, 0, 0.08), RB_ERR_ARG);
    CHECK(memcmp(&s->engine.desc, &before, sizeof before) == 0);
    CHECK(s->engine.theta == 123.0);

    /* Out of range: clamped, if the engine still works. Angles around the
     * crank wrap instead. */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_REV_LIMIT, 0, 50.0), RB_CLAMPED);
    CHECK(s->engine.desc.rev_limit == 2000.0 && s->engine.desc.rev_limit_resume == 1900.0);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_EXHAUST_VALVES, 0, 9.0), RB_CLAMPED);
    CHECK_EQ_INT(rb_get_engine(s, RB_ENG_EXHAUST_VALVES, 0), 4);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_EXHAUST_VALVES, 0, 2.0), RB_OK);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_PIN, 1, -180.0), RB_OK);        /* = 180: as it was */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_PIN, 2, 540.0), RB_OK);         /* = 180 */
    CHECK(s->engine.desc.pin[1] == 180.0 && s->engine.desc.pin[2] == 180.0);
    s->engine.theta = 55.0;
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_PIN, 0, -0.0), RB_OK);          /* = 0: nothing changes */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_PIN, 0, -1e-17), RB_OK);
    CHECK(s->engine.theta == 55.0 && !signbit(s->engine.desc.pin[0]));
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BANK_ANGLE, 0, 330.0), RB_OK);  /* = -30: a slanted four */
    CHECK(s->engine.desc.bank_angle[0] == -30.0 && s->engine.desc.firing[1] == 540.0);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BANK_ANGLE, 0, 0.0), RB_OK);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_REV_LIMIT, 0, 6000.0), RB_OK);

    /* The intake runners (stage 9, pipes since stage 10): through the API, at
     * least 5 cm of them. */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_RUNNER_LENGTH, 0, 0.0), RB_CLAMPED);
    CHECK(s->engine.runner[0].length == 0.05);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_RUNNER_DIAMETER, 0, 0.045), RB_OK);
    const rb_pipe *runner = &s->engine.runner[3];
    CHECK_NEAR(runner->area, PI / 4 * 0.045 * 0.045, 1e-18);
    CHECK_NEAR(runner->cells * runner->cell[0].V, runner->area * 0.05, 1e-18);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_RUNNER_LENGTH, 0, 0.33), RB_OK);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_RUNNER_DIAMETER, 0, 0.04), RB_OK);

    /* Putting a cylinder somewhere in the firing order swaps it with the one
     * that was there, and the firing angles follow. */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_FIRING_ORDER, 1, 1.0), RB_OK);   /* 1-3-4-2 -> 1-2-4-3 */
    int order[4], firing[4];
    for (int k = 0; k < 4; k++) {
        order[k] = (int)rb_get_engine(s, RB_ENG_FIRING_ORDER, k);
        firing[k] = (int)rb_get_engine(s, RB_ENG_FIRING_ANGLE, k);
    }
    CHECK(order[0] == 0 && order[1] == 1 && order[2] == 3 && order[3] == 2);
    CHECK(firing[0] == 0 && firing[1] == 180 && firing[2] == 540 && firing[3] == 360);

    /* The cross-plane V8 with a 60 degree V. */
    CHECK_EQ_INT(rb_load_preset(s, 2), RB_OK);
    CHECK(s->engine.desc.bore == 0.092);   /* the edits went with the old engine */
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BANK_ANGLE, 0, 30.0), RB_OK);
    CHECK_EQ_INT(rb_set_engine(s, RB_ENG_BANK_ANGLE, 1, -30.0), RB_OK);
    CHECK(rb_get_engine(s, RB_ENG_FIRING_ANGLE, 7) == 120.0);   /* cylinder 8 fires 120 after 1 */

    /* rb_reset starts the engine again at rest, as edited; a bad preset
     * changes nothing. */
    s->engine.theta = 77.0;
    rb_reset(s);
    CHECK(s->engine.theta == 0.0 && s->engine.desc.bank_angle[0] == 30.0);
    CHECK_EQ_INT(rb_get_preset(s), 2);
    CHECK_EQ_INT(rb_load_preset(s, 5), RB_ERR_ID);
    CHECK_EQ_INT(rb_load_preset(s, -1), RB_ERR_ID);
    CHECK_EQ_INT(rb_load_preset(NULL, 0), RB_ERR_ARG);
    CHECK_EQ_INT(rb_get_preset(NULL), RB_ERR_ARG);
    CHECK_EQ_INT(rb_get_preset(s), 2);
    CHECK(s->engine.desc.bank_angle[0] == 30.0);
    rb_destroy(s);
}

void test_api_cylinder_state(void)
{
    /* Each cylinder's state through the API is the engine's own. The sim's
     * engine only runs from stage 14, so the test turns it here: the
     * cross-plane V8 held at 3000 rpm, wide open, for three cycles. The burn
     * rises from 0 to all of the fuel between the spark and the exhaust
     * valve opening, and is 0 the rest of the cycle. */
    CHECK_EQ_INT(rb_cylinder_field_count(), RB_CYL_COUNT);
    for (int f = 0; f < RB_CYL_COUNT; f++)
        CHECK(rb_cylinder_field_name(f) && rb_cylinder_field_name(f)[0] && rb_cylinder_field_unit(f));
    CHECK(rb_cylinder_field_name(RB_CYL_COUNT) == NULL && rb_cylinder_field_unit(-1) == NULL);

    rb_sim *s = rb_create(NULL);
    CHECK(s != NULL);
    if (!s) return;
    rb_load_preset(s, 2);
    rb_engine *en = &s->engine;
    const rb_engine_desc *d = &en->desc;
    en->rpm = 3000.0;
    en->throttle_pos = 1.0;
    en->ignition = 1;
    const long cycle = lround(720.0 / (6.0 * en->rpm * rb_physics_dt(s)));
    for (long k = 0; k < 2 * cycle; k++) rb_engine_step(en);

    const double evo = rb_valve_opens(&d->exhaust);
    int differ = 0, burn_outside = 0, burn_fell = 0, full = 0;
    double last[RB_MAX_CYLINDERS] = { 0.0 };
    for (long k = 0; k < cycle; k++) {
        rb_engine_step(en);
        for (int c = 0; c < d->cylinders; c++) {
            const rb_cylinder *cy = &en->cyl[c];
            const double angle = rb_engine_cylinder_angle(en, c);
            differ += rb_get_cylinder(s, c, RB_CYL_ANGLE) != angle;
            differ += fabs(rb_get_cylinder(s, c, RB_CYL_PISTON)
                           - rb_cyl_piston_x(&cy->geom, angle * PI / 180.0)) > 1e-12;
            differ += rb_get_cylinder(s, c, RB_CYL_PRESSURE) != cy->gas.p;
            differ += rb_get_cylinder(s, c, RB_CYL_TEMPERATURE) != cy->gas.T;
            differ += rb_get_cylinder(s, c, RB_CYL_BURNED_GAS) != cy->gas.y[RB_BURNED];
            differ += rb_get_cylinder(s, c, RB_CYL_INTAKE_LIFT) != rb_valve_lift(&d->intake, angle);
            differ += rb_get_cylinder(s, c, RB_CYL_EXHAUST_LIFT) != rb_valve_lift(&d->exhaust, angle);
            const double burn = rb_get_cylinder(s, c, RB_CYL_BURN);
            const double spark = en->burn[c].spark_angle;
            const int between = (spark > evo) ? (angle >= spark || angle < evo)
                                              : (angle >= spark && angle < evo);
            if (!between && burn != 0.0 && fabs(angle - evo) > 1.0 && fabs(angle - spark) > 1.0)
                burn_outside++;
            if (between && burn < last[c]) burn_fell++;
            if (burn > 0.999) full |= 1 << c;
            last[c] = burn;
        }
    }
    CHECK_EQ_INT(differ, 0);
    CHECK_EQ_INT(burn_outside, 0);
    CHECK_EQ_INT(burn_fell, 0);
    CHECK_EQ_INT(full, (1 << d->cylinders) - 1);   /* every cylinder burned all of it */
    CHECK(rb_get_cylinder(s, 0, RB_CYL_PRESSURE) > 0.5e5);

    CHECK(isnan(rb_get_cylinder(s, 8, RB_CYL_ANGLE)) && isnan(rb_get_cylinder(s, -1, RB_CYL_ANGLE)));
    CHECK(isnan(rb_get_cylinder(s, 0, RB_CYL_COUNT)) && isnan(rb_get_cylinder(s, 0, -1)));
    CHECK(isnan(rb_get_cylinder(NULL, 0, RB_CYL_ANGLE)));
    rb_destroy(s);
}

void test_api_pipes(void)
{
    /* The pipes through the API (stage 10), for the 3D view: what each pipe
     * is and where its ends open, and each cell's state, the engine's own.
     * The cross-plane V8 has eight runners, eight headers into a collector
     * per bank, and a twin exhaust: two exhaust pipes, tailpipes and (stage
     * 12) mufflers; then the snorkel. Run as in test_api_cylinder_state. */
    CHECK_EQ_INT(rb_pipe_field_count(), RB_PIPE_FIELD_COUNT);
    CHECK_EQ_INT(rb_pipe_cell_field_count(), RB_CELL_COUNT);
    for (int f = 0; f < RB_PIPE_FIELD_COUNT; f++)
        CHECK(rb_pipe_field_name(f) && rb_pipe_field_name(f)[0] && rb_pipe_field_unit(f));
    for (int f = 0; f < RB_CELL_COUNT; f++)
        CHECK(rb_pipe_cell_field_name(f) && rb_pipe_cell_field_name(f)[0] && rb_pipe_cell_field_unit(f));
    CHECK(rb_pipe_field_name(RB_PIPE_FIELD_COUNT) == NULL && rb_pipe_field_unit(-1) == NULL);
    CHECK(rb_pipe_cell_field_name(RB_CELL_COUNT) == NULL && rb_pipe_cell_field_unit(-1) == NULL);
    CHECK_EQ_INT(rb_pipe_count(NULL), 0);
    CHECK(isnan(rb_get_pipe(NULL, 0, RB_PIPE_KIND)) && isnan(rb_get_pipe_cell(NULL, 0, 0, RB_CELL_PRESSURE)));

    rb_sim *s = rb_create(NULL);
    CHECK(s != NULL);
    if (!s) return;
    rb_load_preset(s, rb_preset_find("v8-cross"));
    rb_engine *en = &s->engine;
    const rb_engine_desc *d = &en->desc;
    CHECK_EQ_INT(rb_pipe_count(s), 23);
    int wrong = 0;
    for (int k = 0; k < 23; k++) {
        const rb_pipe *p = rb_engine_pipe(en, k);
        const int kind = (int)rb_get_pipe(s, k, RB_PIPE_KIND), number = (int)rb_get_pipe(s, k, RB_PIPE_NUMBER);
        const int from = (int)rb_get_pipe(s, k, RB_PIPE_FROM), from_number = (int)rb_get_pipe(s, k, RB_PIPE_FROM_NUMBER);
        const int to = (int)rb_get_pipe(s, k, RB_PIPE_TO), to_number = (int)rb_get_pipe(s, k, RB_PIPE_TO_NUMBER);
        wrong += rb_get_pipe(s, k, RB_PIPE_LENGTH) != p->length;
        wrong += rb_get_pipe(s, k, RB_PIPE_DIAMETER) != p->diameter;
        wrong += rb_get_pipe(s, k, RB_PIPE_CELLS) != p->cells;
        if (k < 8)        /* runner k: plenum -> intake valve k */
            wrong += !(kind == RB_PIPE_RUNNER && number == k && p == &en->runner[k] && from == RB_PLACE_PLENUM
                       && to == RB_PLACE_INTAKE_VALVE && to_number == k);
        else if (k < 16)  /* header c: exhaust valve c -> its bank's collector, */
            wrong += !(kind == RB_PIPE_HEADER && number == k - 8 && p == &en->header[k - 8]
                       && from == RB_PLACE_EXHAUST_VALVE && from_number == k - 8
                       && to == RB_PLACE_COLLECTOR && to_number == d->bank[k - 8]
                       && p->end[1].chamber == &en->collector[d->bank[k - 8]]);   /* really */
        else if (k < 18)  /* each bank's exhaust pipe into its own muffler's inlet, */
            wrong += !(kind == RB_PIPE_EXHAUST && number == k - 16 && from == RB_PLACE_COLLECTOR
                       && from_number == k - 16 && to == RB_PLACE_MUFFLER && to_number == k - 16
                       && p->end[1].chamber == rb_pipe_end_cell(&en->muffler[k - 16], 0));
        else if (k < 20)  /* its tailpipe from the muffler's outlet out into the air, */
            wrong += !(kind == RB_PIPE_TAILPIPE && number == k - 18 && from == RB_PLACE_MUFFLER_OUTLET
                       && from_number == k - 18 && to == RB_PLACE_AIR
                       && p->end[0].chamber == rb_pipe_end_cell(&en->muffler[k - 18], 1));
        else if (k < 22)  /* the muffler between them, */
            wrong += !(kind == RB_PIPE_MUFFLER && number == k - 20 && p == &en->muffler[k - 20]
                       && from == RB_PLACE_MUFFLER && from_number == k - 20
                       && to == RB_PLACE_MUFFLER_OUTLET && to_number == k - 20);
        else              /* and the snorkel from the air into the air box */
            wrong += !(kind == RB_PIPE_SNORKEL && number == 0 && p == &en->snorkel && from == RB_PLACE_AIR
                       && to == RB_PLACE_AIRBOX && p->end[1].chamber == &en->airbox);
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK(isnan(rb_get_pipe(s, 23, RB_PIPE_KIND)) && isnan(rb_get_pipe(s, -1, RB_PIPE_KIND)));
    /* The V8's straight-throughs go on into their pipes; the four's chambered
     * can takes its exhaust pipe at its first cell and gives its tailpipe its
     * last (stage 12). */
    CHECK(en->exhaust_pipe[0].end[1].kind == RB_PIPE_ONWARD && en->tailpipe[0].end[0].kind == RB_PIPE_ONWARD_FROM);
    static rb_engine four;
    rb_engine_desc d4;
    rb_engine_desc_default(&d4);
    CHECK_EQ_INT(rb_engine_init(&four, &d4, rb_physics_dt(s)), 0);
    CHECK(four.exhaust_pipe[0].end[1].kind == RB_PIPE_JOINED
          && four.exhaust_pipe[0].end[1].chamber == rb_pipe_end_cell(&four.muffler[0], 0));
    CHECK(four.tailpipe[0].end[0].kind == RB_PIPE_JOINED
          && four.tailpipe[0].end[0].chamber == rb_pipe_end_cell(&four.muffler[0], 1));
    CHECK(isnan(rb_get_pipe(s, 0, RB_PIPE_FIELD_COUNT)) && isnan(rb_get_pipe(s, 0, -1)));

    en->rpm = 3000.0;
    en->throttle_pos = 1.0;
    en->ignition = 1;
    const long cycle = lround(720.0 / (6.0 * en->rpm * rb_physics_dt(s)));
    for (long k = 0; k < 2 * cycle + 123; k++) rb_engine_step(en);
    int differ = 0, burned = 0, waves = 0;
    for (int k = 0; k < 23; k++) {
        const rb_pipe *p = rb_engine_pipe(en, k);
        for (int c = 0; c < p->cells; c++) {
            differ += rb_get_pipe_cell(s, k, c, RB_CELL_PRESSURE) != p->cell[c].p;
            differ += rb_get_pipe_cell(s, k, c, RB_CELL_TEMPERATURE) != p->cell[c].T;
            differ += rb_get_pipe_cell(s, k, c, RB_CELL_VELOCITY) != rb_pipe_cell_velocity(p, c);
            differ += rb_get_pipe_cell(s, k, c, RB_CELL_BURNED_GAS) != p->cell[c].y[RB_BURNED];
            burned += (k >= 8 && k < 22) && rb_get_pipe_cell(s, k, c, RB_CELL_BURNED_GAS) > 0.9;
            waves += fabs(rb_get_pipe_cell(s, k, c, RB_CELL_PRESSURE) - d->p_ambient) > 2000.0;
        }
        CHECK(isnan(rb_get_pipe_cell(s, k, p->cells, RB_CELL_PRESSURE)));
        CHECK(isnan(rb_get_pipe_cell(s, k, -1, RB_CELL_PRESSURE)));
    }
    CHECK_EQ_INT(differ, 0);
    CHECK(burned > 50);   /* the exhaust side is full of burned gas ...    */
    CHECK(waves > 20);    /* ... and the pipes are anything but still      */
    CHECK(en->collector[0].y[RB_BURNED] > 0.9 && en->collector[1].y[RB_BURNED] > 0.9);   /* both banks' */
    CHECK(isnan(rb_get_pipe_cell(s, 0, 0, RB_CELL_COUNT)) && isnan(rb_get_pipe_cell(s, 23, 0, RB_CELL_PRESSURE)));
    rb_destroy(s);
}
