/*
 * rb_audit.c - repeatable S1 physics baseline, without changing the engine.
 *
 * stdout is CSV; diagnostics go to stderr. The default run starts all five
 * presets and idles each for 30 s, then independently holds each at 3000 rpm
 * wide open for 10 s. Both sample after 6 s. High quality, seeded variation
 * and stock calibration are retained. Held WOT is a regression point, not a
 * free-crank dyno sweep or rb_cli sound's dyno-held recording.
 *
 * Each cylinder contributes only complete overlap-TDC to overlap-TDC work
 * intervals contained in the warm window. Its population COV is computed
 * separately, so fixed differences between cylinders cannot inflate it.
 */
#include "engine.h"
#include "presets.h"
#include "rb.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RATE 192000.0
#define PIPE_RATE 48000.0

typedef struct stats {
    long n;
    double mean, m2, lo, hi;
} stats;

static rb_engine engine;   /* The engine's internal pointers require a fixed address. */

static void add(stats *s, double x)
{
    const double delta = x - s->mean;
    if (!s->n) s->lo = s->hi = x;
    s->n++;
    s->mean += delta / s->n;
    s->m2 += delta * (x - s->mean);
    s->lo = fmin(s->lo, x);
    s->hi = fmax(s->hi, x);
}

static int number(const char *text, double *value)
{
    char *end;
    errno = 0;
    *value = strtod(text, &end);
    return !errno && end != text && !*end && isfinite(*value);
}

static void usage(void)
{
    fprintf(stderr, "usage: rb_audit [--preset KEY] [--mode idle|held_wot|all]\n"
                    "                [--idle-seconds 30] [--held-seconds 10]\n"
                    "                [--warmup 6] [--variation 1]\n"
                    "Writes CSV to stdout. Seconds include warmup; shorter runs are smoke checks.\n");
}

static int audit(int preset, int idle, double seconds, double warmup, double variation)
{
    rb_engine_desc d;
    rb_engine *e = &engine;
    if (rb_preset_desc(preset, &d) || rb_engine_init_at(e, &d, 1.0 / RATE, PIPE_RATE)) return 1;
    e->variation = variation;
    e->ignition = 1;
    e->hold_speed = !idle;
    e->throttle_pos = idle ? 0.0 : 1.0;
    e->starter = idle;
    e->rpm = idle ? 0.0 : 3000.0;
    const uint64_t initial_cycles = e->rand_cycles, initial_flicker = e->rand_flicker;
    const double vd = rb_engine_displacement(e) / d.cylinders;
    const long steps = lround(seconds * RATE), warm = lround(warmup * RATE);
    stats imep[RB_MAX_CYLINDERS] = { { 0 } }, torque = { 0 }, rpm = { 0 }, smooth = { 0 };
    stats completion[RB_MAX_CYLINDERS] = { { 0 } };
    long partial[RB_MAX_CYLINDERS] = { 0 };
    int warm_cycle[RB_MAX_CYLINDERS] = { 0 };
    int warm_burn[RB_MAX_CYLINDERS] = { 0 };
    long cycle_start = -1, stalled_steps = 0;

    fprintf(stderr, "audit: %s %s, %.3g s, warm window %.3g..%.3g s\n",
            rb_preset_key(preset), idle ? "idle" : "held_wot", seconds, warmup, seconds);
    for (long k = 0; k < steps; k++) {
        double angle[RB_MAX_CYLINDERS];
        int after_spark[RB_MAX_CYLINDERS];
        const long before_cycles = e->cycles;
        if (idle && e->starter && (e->rpm_smooth > 0.75 * d.idle_rpm || k >= 3L * (long)RATE))
            e->starter = 0;
        for (int i = 0; i < d.cylinders; i++) {
            angle[i] = rb_engine_cylinder_angle(e, i);
            after_spark[i] = e->burn[i].after_spark;
        }
        rb_engine_step(e);
        if (!isfinite(e->rpm) || !isfinite(e->rpm_smooth)) goto invalid;
        if (k >= warm) {
            add(&rpm, e->rpm);
            add(&smooth, e->rpm_smooth);
            if (idle && e->rpm_smooth <= d.cranking_rpm) stalled_steps++;
        }
        if (e->cycles != before_cycles) {
            if (!isfinite(e->result.brake_torque)) goto invalid;
            if (cycle_start >= warm) add(&torque, e->result.brake_torque);
            cycle_start = k;
        }
        for (int i = 0; i < d.cylinders; i++) {
            rb_burn *b = &e->burn[i];
            const double next = rb_engine_cylinder_angle(e, i);
            /* The core closes work when a step starts at/passes 360 degrees;
             * a step ending exactly there closes it on the following step. */
            if (angle[i] <= 360.0 && next > 360.0) {
                if (!isfinite(b->last_work)) goto invalid;
                if (warm_cycle[i]) add(&imep[i], b->last_work / vd);
                warm_cycle[i] = k >= warm;
            }
            if (!after_spark[i] && b->after_spark) warm_burn[i] = k >= warm;
            if (after_spark[i] && !b->after_spark) {
                if (!isfinite(b->fraction)) goto invalid;
                if (warm_burn[i]) {
                    add(&completion[i], b->fraction);
                    /* An unfinished modeled Wiebe burn at EVO, not an
                     * inferred ignition misfire or a calibrated event rate. */
                    if (b->fraction < 1.0 - 1e-9) partial[i]++;
                }
                warm_burn[i] = 0;
            }
        }
    }
    if (e->starter || torque.n < 2 || !rpm.n) goto invalid;
    for (int i = 0; i < d.cylinders; i++) {
        const stats *s = &imep[i], *b = &completion[i];
        if (s->n < 2 || !b->n || fabs(s->mean) < 1e-9) goto invalid;
        const double cov = 100.0 * sqrt(fmax(s->m2 / s->n, 0.0)) / fabs(s->mean);
        if (!isfinite(cov)) goto invalid;
        if (printf("%s,%s,%s,%d,%.0f,%.0f,%.9g,core_default_init,%016" PRIx64 ",%016" PRIx64
                   ",%.9g,%.9g,%.9g,%ld,%.9g,%.9g,%ld,%.9g,%.9g,%.9g,%.9g,%.9g,%ld,%ld,%.9g,%ld\n",
                   rb_version(), rb_preset_key(preset), idle ? "idle" : "held_wot", i + 1,
                   RATE, PIPE_RATE, variation, initial_cycles, initial_flicker, seconds, warmup, seconds,
                   s->n, s->mean, cov, torque.n, torque.mean, rpm.lo, rpm.hi, smooth.lo, smooth.hi,
                   stalled_steps, b->n, b->lo, partial[i]) < 0) return 1;
    }
    return fflush(stdout) == EOF ? 1 : 0;

invalid:
    fprintf(stderr, "rb_audit: %s %s has non-finite data, insufficient complete warm cycles, or failed startup\n",
            rb_preset_key(preset), idle ? "idle" : "held_wot");
    return 1;
}

int main(int argc, char **argv)
{
    int preset = -1, mode = 2;
    double idle_seconds = 30.0, held_seconds = 10.0, warmup = 6.0, variation = 1.0;
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) { usage(); return 0; }
        if (i + 1 == argc) { usage(); return 2; }
        const char *value = argv[++i];
        if (!strcmp(arg, "--preset")) {
            preset = rb_preset_find(value);
            if (preset < 0) { usage(); return 2; }
        } else if (!strcmp(arg, "--mode")) {
            mode = !strcmp(value, "idle") ? 0 : !strcmp(value, "held_wot") ? 1 : !strcmp(value, "all") ? 2 : -1;
            if (mode < 0) { usage(); return 2; }
        } else {
            double *target = !strcmp(arg, "--idle-seconds") ? &idle_seconds
                           : !strcmp(arg, "--held-seconds") ? &held_seconds
                           : !strcmp(arg, "--warmup") ? &warmup
                           : !strcmp(arg, "--variation") ? &variation : NULL;
            if (!target || !number(value, target)) { usage(); return 2; }
        }
    }
    if (!(idle_seconds > 0.0 && idle_seconds <= 600.0 && held_seconds > 0.0 && held_seconds <= 600.0
          && warmup >= 0.0 && variation >= 0.0 && variation <= 4.0)
        || (mode != 1 && warmup >= idle_seconds) || (mode != 0 && warmup >= held_seconds)) {
        usage();
        return 2;
    }
    puts("version,preset,mode,cylinder,physics_hz,pipe_hz,variation,seed_policy,initial_cycle_state,initial_flicker_state,run_s,window_start_s,window_end_s,imep_cycles,imep_mean_Pa,imep_cov_pct,engine_cycles,brake_torque_mean_Nm,rpm_min,rpm_max,rpm_smooth_min,rpm_smooth_max,warm_stalled_steps,burns_observed,min_completion_at_evo,partial_burns_observed");
    for (int p = 0; p < rb_preset_count(); p++) {
        if (preset >= 0 && p != preset) continue;
        if (mode != 1 && audit(p, 1, idle_seconds, warmup, variation)) return 1;
        if (mode != 0 && audit(p, 0, held_seconds, warmup, variation)) return 1;
    }
    if (fflush(stdout) == EOF) return 1;
    return 0;
}
