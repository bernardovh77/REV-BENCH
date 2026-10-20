/*
 * rb_bench.c - how fast the core runs (stage 13).
 *
 *   rb_bench [PRESET...] [--rate HZ] [--pipe-rate HZ] [--seconds S]
 *
 * Each preset (all of them by default) is held wide open at 3000 rpm, fired,
 * at a physics rate of HZ (default 192000), its pipes at --pipe-rate (default
 * RB_PIPE_RATE, 48000). After 0.3 s to settle, S seconds
 * of simulated time (default 0.5) are timed, best of three: the cost of one
 * simulated second, and how many times faster than real time that is. Then
 * the pieces the profile points at: the nozzle's flow (two pow calls), the
 * Newton step for T, and building a gas mixture.
 *
 * The same file builds natively (make bench) and as WebAssembly for Node's
 * WASI (make bench-wasm, run by tools/bench_wasm.mjs), so the two can be
 * compared on the same machine.
 */
#define _POSIX_C_SOURCE 200809L   /* clock_gettime */

#include "engine.h"
#include "flow.h"
#include "gas.h"
#include "presets.h"
#include "rb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static rb_engine engine;   /* 300+ KB: kept out of the stack */

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}

/* Seconds of computing per simulated second, best of three. */
static double engine_cost(int preset, double rate, double pipe_rate, double seconds, int *cells)
{
    rb_engine_desc d;
    if (rb_preset_desc(preset, &d) != 0) return -1.0;
    rb_engine *e = &engine;
    if (rb_engine_init_at(e, &d, 1.0 / rate, pipe_rate) != 0) return -1.0;
    e->rpm = 3000.0;
    e->throttle_pos = 1.0;
    e->ignition = 1;
    const long warm = (long)(0.3 * rate), n = (long)(seconds * rate);
    for (long k = 0; k < warm; k++) rb_engine_step(e);
    double best = 1e30;
    for (int r = 0; r < 3; r++) {
        const double t0 = now();
        for (long k = 0; k < n; k++) rb_engine_step(e);
        const double cost = (now() - t0) / seconds;
        if (cost < best) best = cost;
    }
    *cells = 0;
    for (int k = 0; k < rb_engine_pipe_count(e); k++) *cells += rb_engine_pipe(e, k)->cells;
    return best;
}

/* A sink for results, so the compiler can't drop the work. */
static volatile double sink;

/* ns per rb_nozzle_mdot: pressure ratios from 0.3 (choked) to 1, gamma 1.30-1.40. */
static double nozzle_ns(void)
{
    const int n = 2000000;
    double acc = 0.0;
    const double t0 = now();
    for (int i = 0; i < n; i++) {
        const double r = 0.3 + 0.7 * (double)(i % 1000) / 1000.0;
        const double g = 1.30 + 0.1 * (double)(i % 97) / 97.0;
        acc += rb_nozzle_mdot(1e-4, 2e5, 900.0, 287.0, g, r * 2e5);
    }
    sink = acc;
    return (now() - t0) / n * 1e9;
}

/* ns per rb_gas_T_from_u: burned gas around 900 K, each call a few kelvin from
 * the last, as from one step to the next. */
static double newton_ns(void)
{
    rb_gas g;
    const double y[RB_NSPECIES] = { 0.05, 0.0, 0.95 };
    rb_gas_init(&g, y);
    const int n = 2000000;
    double T = 900.0, acc = 0.0;
    const double t0 = now();
    for (int i = 0; i < n; i++) {
        const double target = 900.0 + 50.0 * (double)((i * 7) % 100) / 100.0;
        T = rb_gas_T_from_u(&g, rb_gas_u(&g, target) + 0.0, T);
        acc += T;
    }
    sink = acc;
    /* rb_gas_u is timed with it: subtract it */
    const double t1 = now();
    for (int i = 0; i < n; i++) acc += rb_gas_u(&g, 900.0 + 50.0 * (double)((i * 7) % 100) / 100.0);
    sink = acc;
    const double t2 = now();
    return ((t1 - t0) - (t2 - t1)) / n * 1e9;
}

/* ns per rb_gas_init. */
static double mixture_ns(void)
{
    rb_gas g;
    const int n = 2000000;
    double acc = 0.0;
    const double t0 = now();
    for (int i = 0; i < n; i++) {
        const double b = (double)(i % 1000) / 1000.0;
        const double y[RB_NSPECIES] = { 1.0 - b, 0.01, b };
        rb_gas_init(&g, y);
        acc += g.k_hi;
    }
    sink = acc;
    return (now() - t0) / n * 1e9;
}

int main(int argc, char **argv)
{
    double rate = 192000.0, pipe_rate = RB_PIPE_RATE, seconds = 0.5;
    int chosen[64], n_chosen = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rate") && i + 1 < argc) rate = atof(argv[++i]);
        else if (!strcmp(argv[i], "--pipe-rate") && i + 1 < argc) pipe_rate = atof(argv[++i]);
        else if (!strcmp(argv[i], "--seconds") && i + 1 < argc) seconds = atof(argv[++i]);
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("usage: rb_bench [PRESET...] [--rate HZ] [--pipe-rate HZ] [--seconds S]\n");
            return 0;
        } else {
            const int p = rb_preset_find(argv[i]);
            if (p < 0 || n_chosen == 64) {
                fprintf(stderr, "rb_bench: unknown preset %s\n", argv[i]);
                return 2;
            }
            chosen[n_chosen++] = p;
        }
    }
    if (!(rate >= 8000.0 && rate <= 2e6) || !(seconds > 0.0 && seconds <= 60.0)) {
        fprintf(stderr, "rb_bench: --rate must be 8000..2e6 and --seconds 0..60\n");
        return 2;
    }
    if (!n_chosen)
        for (int p = 0; p < rb_preset_count() && n_chosen < 64; p++) chosen[n_chosen++] = p;

    printf("REV-BENCH %s, physics at %.0f Hz, pipes at %.0f Hz: each preset held wide open at "
           "3000 rpm\n", rb_version(), rate, pipe_rate);
    printf("%-10s %6s %9s %12s %11s\n", "preset", "cells", "us/step", "s per s", "x real time");
    for (int i = 0; i < n_chosen; i++) {
        int cells = 0;
        const double cost = engine_cost(chosen[i], rate, pipe_rate, seconds, &cells);
        if (cost < 0.0) {
            printf("%-10s (can't build it)\n", rb_preset_key(chosen[i]));
            continue;
        }
        printf("%-10s %6d %9.2f %12.3f %11.2f\n", rb_preset_key(chosen[i]), cells,
               cost / rate * 1e6, cost, 1.0 / cost);
        fflush(stdout);
    }
    printf("pieces, ns per call: nozzle flow %.1f, T from u %.1f, gas mixture %.1f\n",
           nozzle_ns(), newton_ns(), mixture_ns());
    return 0;
}
