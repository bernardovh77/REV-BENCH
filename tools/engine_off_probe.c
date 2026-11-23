/*
 * engine_off_probe.c - rb_engine_off_probe (stage 069): what a stopped engine
 * sounds like right after a build, for every layout and every editable field
 * at either end of its range.
 *
 *   rb_engine_off_probe [--seconds S] [--quality tier|low|...] [--all] [--rest]
 *                       [--preset KEY] [--family inline|v|boxer --cylinders N]
 *
 * Bernardo heard it on 2026-11-22: an inline 3, the engine off, editing; a
 * steady, rising, high-pitched tone for about a second, then nothing. With the
 * key off the engine is still and its gas at rest, so it should be silent. The
 * probe builds the engine as the app does (the preset, the layout, one edit;
 * the hold off, the key off; --rest: RB_PARAM_REST on, as the app's builds),
 * renders S seconds (30 by default) and prints each case whose output's peak
 * after the first 3 s (the build's own settling) is above -80 dB below full
 * scale, with when, or every case with --all. A fresh engine's gas, cold
 * against its warm walls, grows a tone of its own (docs/stage-069.md).
 *
 * Exit code: 0 if every case stayed under -80 dBFS, 1 if one didn't, 2 for a
 * bad command line.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rb.h"

#define RATE 48000.0
#define LOUD_DB (-80.0)
#define WINDOW_S 0.05

#define SETTLE_S 3.0

typedef struct result {
    double peak_db, peak_t;     /* the loudest sample after SETTLE_S, and when */
    double window_db, window_t; /* the loudest 50 ms window's RMS and when */
    double end_db;              /* the last window's RMS                   */
    int    ok;
} result;

static double db(double v)
{
    return v > 1e-12 ? 20.0 * log10(v) : -240.0;
}

/* Builds the engine as the app does and renders `seconds` with the key off. */
static result render_off(int preset, int family, int cylinders, int quality, int field, int index,
                         double value, double seconds, int rest)
{
    result r = { -240.0, 0.0, -240.0, 0.0, -240.0, 0 };
    rb_config cfg;
    rb_config_default(&cfg);
    cfg.sample_rate = RATE;
    if (rb_config_quality(&cfg, quality) != RB_OK) return r;
    rb_sim *sim = rb_create(&cfg);
    if (!sim) return r;
    if (rb_load_preset(sim, preset) != RB_OK) goto done;
    if (family >= 0) {
        const int crank = rb_layout_usual_crank(family, cylinders);
        const double angle = rb_layout_angle(family, cylinders, crank, RB_ANGLE_USUAL);
        if (rb_load_layout(sim, preset, family, cylinders, crank, angle) != RB_OK) goto done;
    }
    if (field >= 0) {
        const int rc = rb_set_engine(sim, field, index, value);
        if (rc != RB_OK && rc != RB_CLAMPED) goto done;
    }
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_HOLD, 0.0);
    rb_set_param(sim, RB_PARAM_REST, rest);

    enum { BLOCK = 480 };   /* 10 ms */
    float buf[BLOCK];
    const int window = (int)(WINDOW_S * RATE);
    double sumsq = 0.0;
    int in_window = 0;
    const long total = (long)(seconds * RATE);
    for (long done_frames = 0; done_frames < total; done_frames += BLOCK) {
        rb_render_block(sim, buf, BLOCK);
        for (int i = 0; i < BLOCK; i++) {
            const double v = fabs((double)buf[i]);
            const double t = (double)(done_frames + i) / RATE;
            if (t > SETTLE_S && db(v) > r.peak_db) {
                r.peak_db = db(v);
                r.peak_t = t;
            }
            sumsq += v * v;
            if (++in_window == window) {
                const double w = db(sqrt(sumsq / window));
                if (w > r.window_db) {
                    r.window_db = w;
                    r.window_t = t;
                }
                r.end_db = w;
                sumsq = 0.0;
                in_window = 0;
            }
        }
    }
    r.ok = 1;
done:
    rb_destroy(sim);
    return r;
}

static int report(const char *what, result r, int all)
{
    const int loud = r.ok && r.peak_db > LOUD_DB;
    if (!r.ok) {
        if (all) printf("  %-48s refused\n", what);
        return 0;
    }
    if (loud || all)
        printf("  %-48s after 3 s: peak %7.1f dBFS at %5.2f s, loudest 50 ms %7.1f at %5.2f s, last %7.1f%s\n", what,
               r.peak_db, r.peak_t, r.window_db, r.window_t, r.end_db, loud ? "  LOUD" : "");
    return loud;
}

int main(int argc, char **argv)
{
    double seconds = 30.0;
    int rest = 0;
    int quality = -1, all = 0, only_preset = -1, family = RB_FAMILY_INLINE, cylinders = 3;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = atof(argv[++i]);
        else if (!strcmp(argv[i], "--all")) all = 1;
        else if (!strcmp(argv[i], "--rest")) rest = 1;
        else if (!strcmp(argv[i], "--preset") && i + 1 < argc) {
            only_preset = rb_preset_find(argv[++i]);
            if (only_preset < 0) return 2;
        } else if (!strcmp(argv[i], "--family") && i + 1 < argc) {
            const char *f = argv[++i];
            family = !strcmp(f, "inline") ? RB_FAMILY_INLINE : !strcmp(f, "v") ? RB_FAMILY_V
                   : !strcmp(f, "boxer") ? RB_FAMILY_BOXER : !strcmp(f, "preset") ? -1 : -2;
            if (family == -2) return 2;
        } else if (!strcmp(argv[i], "--cylinders") && i + 1 < argc) cylinders = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--quality") && i + 1 < argc) {
            const char *q = argv[++i];
            static const char *names[] = { "low", "medium", "high", "ultra", "balanced", "lean" };
            static const int ids[] = { RB_QUALITY_LOW, RB_QUALITY_MEDIUM, RB_QUALITY_HIGH, RB_QUALITY_ULTRA,
                                       RB_QUALITY_BALANCED, RB_QUALITY_LEAN };
            quality = -2;
            for (int k = 0; k < 6; k++)
                if (!strcmp(q, names[k])) quality = ids[k];
            if (!strcmp(q, "tier")) quality = -1;
            if (quality == -2) return 2;
        } else {
            fprintf(stderr, "usage: rb_engine_off_probe [--seconds S] [--quality tier|low|medium|high|ultra|balanced|lean] "
                            "[--all] [--rest] [--preset KEY] [--family inline|v|boxer|preset --cylinders N]\n");
            return 2;
        }
    }

    int loud = 0;
    for (int p = 0; p < rb_preset_count(); p++) {
        if (only_preset >= 0 && p != only_preset) continue;
        const int q = quality >= 0 ? quality
                    : family >= 0 ? rb_layout_quality(family, cylinders, rb_layout_usual_crank(family, cylinders))
                                  : rb_preset_quality(p);
        printf("%s as %s %d (quality %d), the key off, %.1f s:\n", rb_preset_name(p),
               family >= 0 ? rb_family_name(family) : "itself", family >= 0 ? cylinders : 0, q, seconds);
        loud += report("as built", render_off(p, family, cylinders, q, -1, 0, 0.0, seconds, rest), all);
        for (int f = 0; f < rb_engine_field_count(); f++) {
            if (rb_engine_field_flags(f) & RB_EF_READONLY) continue;
            const double lo = rb_engine_field_min(f), hi = rb_engine_field_max(f);
            if (!(lo == lo) || !(hi == hi)) continue;
            const double ends[2] = { lo, hi };
            for (int k = 0; k < 2; k++) {
                char what[96];
                snprintf(what, sizeof what, "%s = %g", rb_engine_field_name(f), ends[k]);
                loud += report(what, render_off(p, family, cylinders, q, f, 0, ends[k], seconds, rest), all);
            }
        }
    }
    printf("%d loud case%s\n", loud, loud == 1 ? "" : "s");
    return loud ? 1 : 0;
}
