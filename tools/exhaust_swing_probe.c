/*
 * exhaust_swing_probe.c - rb_exhaust_swing_probe (stage 129, its premise): how strong the
 * exhaust's waves are, pipe by pipe and cell by cell, so that the place where they become weak
 * enough to carry linearly (on delay lines instead of gas cells) can be found.
 *
 *   rb_exhaust_swing_probe [--quality high|...] [--preset KEY] [--cells]
 *
 * For each preset, two drives held by the dyno: wide open at 5000 rpm (the full load), and the
 * throttle shut at 4000 rpm (the overrun). After 1.5 s to settle, 0.5 s is read, every cell's
 * state 12 000 times a second: its pressure's swing ((max - min) / 2 over its mean) and its
 * highest Mach number (|u| / c, c from the cell's temperature). Printed per exhaust pipe, in the
 * exhaust's order, its loudest cell and its quietest; with --cells, every cell. The card's line
 * is a swing under 5 % of the mean.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rb.h"

#define RATE        48000
#define READ_EVERY  4          /* frames between reads: 12 kHz */
#define SETTLE_S    1.5
#define READ_S      0.5
#define MAX_PIPES   64
#define MAX_CELLS   64
#define GAMMA       1.35
#define R_GAS       287.0

typedef struct cell_stat {
    double pmin, pmax, psum, mach;
    long   n;
} cell_stat;

static const char *kind_name(int kind)
{
    static const char *names[] = { "runner", "header", "exhaust", "tailpipe", "muffler", "snorkel", "center", "link" };
    return (kind >= 0 && kind < 8) ? names[kind] : "?";
}

/* The exhaust's pipes in the gas's order: headers, exhaust pipes, centre, links, mufflers, tailpipes. */
static int exhaust_rank(int kind)
{
    switch (kind) {
    case RB_PIPE_HEADER:   return 0;
    case RB_PIPE_EXHAUST:  return 1;
    case RB_PIPE_CENTER:   return 2;
    case RB_PIPE_LINK:     return 3;
    case RB_PIPE_MUFFLER:  return 4;
    case RB_PIPE_TAILPIPE: return 5;
    default:               return -1;
    }
}

static void probe(int preset, int quality, double rpm, double throttle, int cells_too)
{
    rb_sim *sim = rb_create_with_quality(RATE, quality);
    if (!sim) return;
    rb_load_preset(sim, preset);
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, rpm);
    rb_set_param(sim, RB_PARAM_THROTTLE, throttle);
    static float out[RATE];
    for (int i = 0; i < (int)(SETTLE_S * 10); i++) rb_render_block(sim, out, RATE / 10);

    const int pipes = rb_pipe_count(sim);
    static cell_stat st[MAX_PIPES][MAX_CELLS];
    for (int p = 0; p < pipes && p < MAX_PIPES; p++)
        for (int c = 0; c < MAX_CELLS; c++) st[p][c] = (cell_stat){ 1e30, -1e30, 0.0, 0.0, 0 };
    for (int f = 0; f < (int)(READ_S * RATE); f += READ_EVERY) {
        rb_render_block(sim, out, READ_EVERY);
        for (int p = 0; p < pipes && p < MAX_PIPES; p++) {
            if (exhaust_rank((int)rb_get_pipe(sim, p, RB_PIPE_KIND)) < 0) continue;
            const int n = (int)rb_get_pipe(sim, p, RB_PIPE_CELLS);
            for (int c = 0; c < n && c < MAX_CELLS; c++) {
                const double pr = rb_get_pipe_cell(sim, p, c, RB_CELL_PRESSURE);
                const double T = rb_get_pipe_cell(sim, p, c, RB_CELL_TEMPERATURE);
                const double u = rb_get_pipe_cell(sim, p, c, RB_CELL_VELOCITY);
                cell_stat *s = &st[p][c];
                s->pmin = fmin(s->pmin, pr);
                s->pmax = fmax(s->pmax, pr);
                s->psum += pr;
                s->n++;
                s->mach = fmax(s->mach, fabs(u) / sqrt(GAMMA * R_GAS * T));
            }
        }
    }
    printf("%s, %s at %.0f rpm:\n", rb_preset_name(preset), throttle > 0.5 ? "wide open" : "throttle shut", rpm);
    for (int rank = 0; rank <= 5; rank++)
        for (int p = 0; p < pipes && p < MAX_PIPES; p++) {
            const int kind = (int)rb_get_pipe(sim, p, RB_PIPE_KIND);
            if (exhaust_rank(kind) != rank) continue;
            const int n = (int)rb_get_pipe(sim, p, RB_PIPE_CELLS);
            double most = 0.0, least = 1e30, mach = 0.0;
            for (int c = 0; c < n && c < MAX_CELLS; c++) {
                const cell_stat *s = &st[p][c];
                const double swing = 0.5 * (s->pmax - s->pmin) / (s->psum / s->n);
                most = fmax(most, swing);
                least = fmin(least, swing);
                mach = fmax(mach, s->mach);
            }
            printf("  %-8s %d  %5.2f m %2d cells: swing %5.1f-%5.1f %% of the mean, Mach up to %.3f%s\n", kind_name(kind),
                   (int)rb_get_pipe(sim, p, RB_PIPE_NUMBER), rb_get_pipe(sim, p, RB_PIPE_LENGTH), n, 100.0 * least, 100.0 * most, mach,
                   most < 0.05 ? "  (all under 5 %)" : "");
            if (cells_too)
                for (int c = 0; c < n && c < MAX_CELLS; c++) {
                    const cell_stat *s = &st[p][c];
                    printf("      cell %2d: swing %5.1f %%, Mach %.3f\n", c, 50.0 * (s->pmax - s->pmin) / (s->psum / s->n), s->mach);
                }
        }
    rb_destroy(sim);
}

int main(int argc, char **argv)
{
    int quality = RB_QUALITY_HIGH, only = -1, cells = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--cells")) cells = 1;
        else if (!strcmp(argv[i], "--preset") && i + 1 < argc) {
            only = rb_preset_find(argv[++i]);
            if (only < 0) return 2;
        } else if (!strcmp(argv[i], "--quality") && i + 1 < argc) {
            const char *q = argv[++i];
            quality = !strcmp(q, "low") ? RB_QUALITY_LOW : !strcmp(q, "medium") ? RB_QUALITY_MEDIUM
                    : !strcmp(q, "high") ? RB_QUALITY_HIGH : !strcmp(q, "balanced") ? RB_QUALITY_BALANCED
                    : !strcmp(q, "lean") ? RB_QUALITY_LEAN : -1;
            if (quality < 0) return 2;
        } else {
            fprintf(stderr, "usage: rb_exhaust_swing_probe [--quality high|...] [--preset KEY] [--cells]\n");
            return 2;
        }
    }
    for (int p = 0; p < rb_preset_count(); p++) {
        if (only >= 0 && p != only) continue;
        probe(p, quality, 5000.0, 1.0, cells);
        probe(p, quality, 4000.0, 0.0, cells);
    }
    return 0;
}
