/*
 * test_walls.c - stage 144: the walls' viscothermal loss (pipe.h,
 * rb_wall_fit), a candidate behind RB_PARAM_VARIANT 6.
 *
 * Without it the solver's walls hardly damp a sound wave: its friction is the
 * quasi-steady turbulent loss of the mean flow, quadratic in the speed, so a
 * pipe's resonance in still gas rings 100-1000 times longer than Kirchhoff's
 * boundary layers allow (experiments/ringdown/pipe_ringdown.c). With it: the
 * fit follows Kirchhoff's sqrt(w) at every pipe rate; a closed pipe's modes
 * die at Kirchhoff's rate; the engine's books still balance; and switching it
 * while the engine runs makes no jump.
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

static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
static const double PI = 3.14159265358979323846;
#define P0 101325.0

static rb_chamber cells[RB_PIPE_MAX_CELLS];
static rb_face faces[RB_PIPE_MAX_CELLS];
static rb_wall_fit fit;
static rb_engine e;

void test_walls_fit_follows_kirchhoff(void)
{
    /* The sections as the steps make them (pipe.c's wall_step), their sum's
     * real part against sqrt(w / 2), from 20 Hz to 10 kHz (or 0.35 of the
     * rate): within -5 % and +15 %, at every pipe rate the qualities use; every
     * weight positive (a passive loss). */
    const double rates[5] = { 19200.0, 24000.0, 48000.0, 96000.0, 192000.0 };
    for (int r = 0; r < 5; r++) {
        rb_wall_fit_init(&fit, 1.0 / rates[r]);
        double lo = INFINITY, hi = -INFINITY;
        int negative = 0;
        for (int k = 0; k < RB_WALL_SECTIONS; k++) negative += !(fit.c[k] > 0.0);
        const double top = fmin(10000.0, 0.35 * rates[r]);
        for (int i = 0; i <= 100; i++) {
            const double f = 20.0 * pow(top / 20.0, i / 100.0), w = 2.0 * PI * f;
            const double zr = cos(w * fit.dt), zi = -sin(w * fit.dt);
            double re = 0.0;
            for (int k = 0; k < RB_WALL_SECTIONS; k++) {
                const double a = fit.a[k], q = 1.0 - a;
                const double dr = 1.0 - q * zr, di = -q * zi, nr = a * zr, ni = a * zi;
                const double fr = (nr * dr + ni * di) / (dr * dr + di * di);
                re += fit.c[k] * q * (1.0 - fr);
            }
            const double ratio = re / sqrt(0.5 * w);
            lo = fmin(lo, ratio), hi = fmax(hi, ratio);
        }
        printf("    pipes at %6.0f Hz: the damping %.3f-%.3f of Kirchhoff's to %.0f Hz\n", rates[r], lo, hi, top);
        CHECK(lo > 0.95 && hi < 1.15);
        CHECK_EQ_INT(negative, 0);
    }
}

/* Kirchhoff's decay rate (1/s) of a plane wave of f Hz in air at T in a pipe
 * of bore d, the gas's gamma as the solver has it. */
static double kirchhoff(double f, double d, double T, double gamma)
{
    const double R = 287.0985, Pr = 0.71;
    const double mu = 1.716e-5 * pow(T / 273.15, 1.5) * (273.15 + 110.4) / (T + 110.4);
    const double nu = mu / (P0 / (R * T)), c = sqrt(gamma * R * T);
    return sqrt(PI * f * nu) / (0.5 * d * c) * (1.0 + (gamma - 1.0) / sqrt(Pr)) * c;
}

/* A closed 1 m pipe's mode `mode` set up at 100 Pa and left to ring: its decay
 * rate (1/s) over its first 40 dB, with the walls' loss on or off. */
static double ring(double T, int mode, int walls, double dt)
{
    rb_pipe p;
    const double L = 1.0, d = 0.0484;
    const int n = 40;   /* 2.5 cm: 10 cells a wavelength or more for the modes read */
    rb_pipe_init(&p, cells, faces, n, L, d, P0, T, AIR, T, 0.025);
    for (int k = 0; k < n; k++) {
        const double pk = P0 + 100.0 * cos(mode * PI * (k + 0.5) / n);
        const double g = rb_chamber_gamma(&p.cell[k]);
        rb_chamber_init(&p.cell[k], p.area * p.dx, pk, T * pow(pk / P0, (g - 1.0) / g), AIR);
    }
    rb_wall_fit_init(&fit, dt);
    if (walls) rb_pipe_walls(&p, &fit);
    const double f = mode * sqrt(rb_chamber_gamma(&p.cell[0]) * 287.0985 * T) / (2.0 * L);
    const long per = lround(1.0 / f / dt), steps = lround(1.0 / dt);
    double first = 0.0, last = 0.0, t_first = 0.0, t_last = 0.0, peak = 0.0;
    for (long s = 0; s < steps; s++) {
        rb_pipe_clear(&p, 0);
        rb_pipe_flows(&p, 0, dt);
        rb_pipe_integrate(&p, 0, dt);
        rb_pipe_accelerate(&p, 0, dt);
        const double x = (mode % 2) ? 0.5 * (p.cell[0].p - p.cell[n - 1].p) : p.cell[0].p - P0;
        peak = fmax(peak, fabs(x));
        if ((s + 1) % per == 0) {
            const double t = (s + 1) * dt;
            if (t > 0.02 && first == 0.0) first = peak, t_first = t;
            if (t > 0.02 && peak > 0.01 * first) last = peak, t_last = t;
            peak = 0.0;
        }
    }
    return log(first / last) / (t_last - t_first);
}

void test_walls_pipe_rings_down_as_kirchhoff_says(void)
{
    /* A closed pipe loses its sound only through its walls. Its first and
     * fourth modes, in air at 300 and 800 K, on cells fine enough to carry
     * them: with the walls' loss they die at Kirchhoff's rate within 10 %;
     * without, at under a fiftieth of it (the engine's quasi-steady friction
     * alone). */
    const double temps[2] = { 300.0, 800.0 };
    const int modes[2] = { 1, 4 };
    for (int a = 0; a < 2; a++)
        for (int m = 0; m < 2; m++) {
            rb_chamber air;
            rb_chamber_init(&air, 1.0, P0, temps[a], AIR);
            const double g = rb_chamber_gamma(&air);
            const double f = modes[m] * sqrt(g * 287.0985 * temps[a]) / 2.0;
            const double want = kirchhoff(f, 0.0484, temps[a], g);
            const double on = ring(temps[a], modes[m], 1, 1.0 / 48000.0);
            const double off = ring(temps[a], modes[m], 0, 1.0 / 48000.0);
            printf("    %3.0f K, mode %d (%4.0f Hz): decay %.2f /s with the walls, %.3f without; "
                   "Kirchhoff %.2f\n", temps[a], modes[m], f, on, off, want);
            CHECK_NEAR(on, want, 0.10 * want);
            CHECK(off < want / 50.0);
        }
}

/* The engine's books over a run, motored wide open at 3000 rpm, the
 * cylinders' walls adiabatic (as test_net_new_parts_run_and_keep_the_books). */
void test_walls_keep_the_books(void)
{
    /* The loss takes the swings' speed, and the cells' energy (which carries
     * the gas's kinetic energy) gets it back as heat: mass, energy and every
     * species balance as tightly as without it, on two presets, one with its
     * own network. */
    const char *keys[2] = { "i4", "ferrari-458" };
    for (int p = 0; p < 2; p++) {
        rb_engine_desc d;
        rb_preset_desc(rb_preset_find(keys[p]), &d);
        d.heat_transfer = 0;
        CHECK(rb_engine_init(&e, &d, 1.0 / 192000.0) == 0);
        rb_engine_walls(&e, 1);
        e.rpm = 3000.0;
        e.throttle_pos = 1.0;
        for (long s = 0; s < 9600; s++) rb_engine_step(&e);
        double m0, u0, y0[3], m1, u1, y1[3];
        const double in0 = e.mass_in, out0 = e.mass_out, ein0 = e.energy_in, eout0 = e.energy_out,
                     w0 = e.work_on_gas, q0 = rb_engine_pipe_heat(&e);
        double sin0[3], sout0[3];
        for (int j = 0; j < 3; j++) sin0[j] = e.species_in[j], sout0[j] = e.species_out[j];
        rb_engine_contents(&e, &m0, &u0, y0);
        for (long s = 0; s < 3 * 7680; s++) rb_engine_step(&e);
        rb_engine_contents(&e, &m1, &u1, y1);
        const double dm = (e.mass_in - in0) - (e.mass_out - out0);
        const double du = (e.energy_in - ein0) - (e.energy_out - eout0) + (e.work_on_gas - w0)
                        + (rb_engine_pipe_heat(&e) - q0);
        printf("    %-12s with the walls' loss: mass %+.1e, energy %+.1e of the flow\n", keys[p],
               (m1 - m0 - dm) / m0, (u1 - u0 - du) / fabs(e.energy_out - eout0));
        CHECK_NEAR(m1 - m0, dm, m0 * 1e-11);
        CHECK_NEAR(u1 - u0, du, fabs(e.energy_out - eout0) * 1e-9);
        for (int j = 0; j < 3; j++)
            CHECK_NEAR(y1[j] - y0[j], (e.species_in[j] - sin0[j]) - (e.species_out[j] - sout0[j]), m0 * 1e-11);
        CHECK(rb_engine_guards(&e) == 0);
        CHECK(isfinite(e.omega) && e.omega > 0.0);
    }
}

void test_walls_switch_live_without_a_jump(void)
{
    /* RB_PARAM_VARIANT 6 turns the loss on in every pipe while the engine
     * runs, and anything else turns it off. Turned on, a face's sections
     * start at its flow: the first step's loss is only what the flow changed
     * in that step, so the flows move no further from where they'd have gone
     * than the step itself moves them (a jump would be the whole flow). */
    rb_sim *sim = rb_create_with_quality(48000.0, RB_QUALITY_LOW);
    CHECK(sim != NULL);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("v8-cross")), RB_OK);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 4000.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1.0);
    rb_set_param(sim, RB_PARAM_IGNITION, 1.0);
    static float block[2 * 4800];
    rb_render_block(sim, block, 4800);
    CHECK(sim->engine.walls == 0);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIANT, 6), RB_OK);
    rb_render_block(sim, block, 1);
    CHECK(sim->engine.walls == 1);
    const rb_pipe *x = &sim->engine.xpipe[0];
    CHECK(x->wall == &sim->engine.wall_fit);
    /* the sections follow the flow: within the last step's change of it */
    double worst = 0.0, scale = 0.0;
    for (int k = 0; k + 1 < x->cells; k++) {
        scale = fmax(scale, fabs(x->face[k].mdot));
        for (int j = 0; j < RB_WALL_SECTIONS; j++)
            worst = fmax(worst, fabs(x->face[k].wall[j] - x->face[k].mdot));
    }
    printf("    switched on: the sections within %.2e kg/s of the flows (largest %.3f kg/s)\n", worst, scale);
    CHECK(worst < 0.05 * scale);
    rb_render_block(sim, block, 4800);
    float loudest = 0.0f;
    for (int i = 0; i < 2 * 4800; i++) loudest = fmaxf(loudest, fabsf(block[i]));
    CHECK(isfinite(loudest) && loudest > 0.0f);
    CHECK_EQ_INT(rb_set_param(sim, RB_PARAM_VARIANT, 0), RB_OK);
    rb_render_block(sim, block, 1);
    CHECK(sim->engine.walls == 0 && x->wall == NULL);
    rb_destroy(sim);
}
