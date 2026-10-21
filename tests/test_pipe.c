/*
 * test_pipe.c - stage 10: the 1D pipe. A pressure pulse travels at the speed
 * of sound, comes back from a closed end as it went and from an open end
 * inverted; waves ride a flow at c + u and c - u; a steady flow loses what
 * Darcy and Weisbach say; the walls warm or cool the gas as Colburn says; mass,
 * species and energy are conserved exactly, on one rate or two. Stage 12: a
 * packed pipe damps fast swings and lets a steady flow through.
 */
#include "pipe.h"
#include "rb_test.h"

#include <math.h>

static const double AIR[RB_NSPECIES]    = { [RB_AIR] = 1.0 };
static const double BURNED[RB_NSPECIES] = { [RB_BURNED] = 1.0 };
#define P0 101325.0
#define SLOW_DT (1.0 / 48000.0)   /* the engine's pipe step */
#define FAST_DT (1.0 / 192000.0)  /* the default physics step */

static rb_chamber cells[RB_PIPE_MAX_CELLS];
static rb_face faces[RB_PIPE_MAX_CELLS];
static rb_pipe pipe;
static double trace[2][20000];

/* One step of a pipe on one rate. */
static void step(rb_pipe *p, double dt)
{
    rb_pipe_clear(p, 0);
    rb_pipe_flows(p, 0, dt);
    rb_pipe_integrate(p, 0, dt);
    rb_pipe_accelerate(p, 0, dt);
}

/* Sets cell k's gas to p at the temperature an isentropic squeeze from (P0, T)
 * gives it: a sound wave, with no hot or cold spot riding along. */
static void squeeze(rb_pipe *p, int k, double pressure, double T)
{
    const double g = rb_chamber_gamma(&p->cell[k]);
    rb_chamber_init(&p->cell[k], p->area * p->dx, pressure,
                    T * pow(pressure / p->cell[k].p, (g - 1.0) / g), p->cell[k].y);
}

/* When trace j peaks (sign +1) or dips (-1) between t0 and t1, to a fraction
 * of a step (a parabola through the three samples around the extreme), and
 * how far. */
static double extreme(int j, double dt, double t0, double t1, double sign, double *size)
{
    long best = -1;
    for (long i = lround(t0 / dt); i < lround(t1 / dt) && i < 20000 - 1; i++)
        if (i > 0 && (best < 0 || sign * trace[j][i] > sign * trace[j][best])) best = i;
    if (best < 1) return NAN;
    const double a = trace[j][best - 1], b = trace[j][best], c = trace[j][best + 1];
    if (size) *size = b;
    return (best + 1 + 0.5 * (a - c) / (a - 2.0 * b + c)) * dt;
}

void test_pipe_cells(void)
{
    /* About a cell every so many cm, at least one, at most
     * RB_PIPE_MAX_CELLS, and never so short a wave crosses one in a step. */
    CHECK_EQ_INT(rb_pipe_cells_for(0.33, 0.05, SLOW_DT), 7);
    CHECK_EQ_INT(rb_pipe_cells_for(1.8, 0.075, SLOW_DT), 24);
    CHECK_EQ_INT(rb_pipe_cells_for(0.01, 0.05, SLOW_DT), 1);
    CHECK_EQ_INT(rb_pipe_cells_for(10.0, 0.05, SLOW_DT), RB_PIPE_MAX_CELLS);
    CHECK_EQ_INT(rb_pipe_cells_for(0.33, 0.05, 1.0 / 8000.0), 2);   /* 15.6 cm at 8 kHz */
    CHECK_EQ_INT(rb_pipe_cells_for(NAN, 0.05, SLOW_DT), 1);
    CHECK_EQ_INT(rb_pipe_cells_for(0.33, 0.0, SLOW_DT), 1);
    CHECK(rb_pipe_init(&pipe, cells, faces, 0, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.02) == -1);
    CHECK(rb_pipe_init(&pipe, cells, faces, RB_PIPE_MAX_CELLS + 1, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.02) == -1);
    CHECK(rb_pipe_init(&pipe, cells, faces, 4, -1.0, 0.04, P0, 300.0, AIR, 300.0, 0.02) == -1);
    CHECK(rb_pipe_init(&pipe, cells, faces, 4, 1.0, NAN, P0, 300.0, AIR, 300.0, 0.02) == -1);
    CHECK(rb_pipe_init(&pipe, cells, faces, 4, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.02) == 0);
    CHECK_NEAR(pipe.dx, 0.25, 1e-15);
    CHECK_NEAR(pipe.cell[3].V, 0.25 * 3.14159265358979323846 * 0.04 * 0.04 * 0.25, 1e-18);
}

/* One physics step of a pipe on two rates, in blocks of four (the engine's
 * order, pipe.h): the slow cells' rates when a block begins; the fast cells
 * and faces every step (FAST_DT); the slow cells integrated and their faces'
 * flows updated when it ends; then the fast faces' flows. */
static void step_two_rates(rb_pipe *p, long i)
{
    if (i % 4 == 0) {
        rb_pipe_clear(p, 0);
        rb_pipe_flows(p, 0, 4.0 * FAST_DT);
    }
    rb_pipe_clear(p, 1);
    rb_pipe_flows(p, 1, FAST_DT);
    rb_pipe_integrate(p, 1, FAST_DT);
    if (i % 4 == 3) {
        rb_pipe_integrate(p, 0, 4.0 * FAST_DT);
        rb_pipe_accelerate(p, 0, 4.0 * FAST_DT);
    }
    rb_pipe_accelerate(p, 1, FAST_DT);
}

/* A 2 m pipe of still air, 64 cells, closed at end 0 and either closed or
 * open at end 1. A pressure pulse starts against the closed end (half a bump
 * of 1 kPa and 20 cm: its mirror image in the wall makes it whole) and runs
 * along the pipe; traces 0 and 1 are the pressure 0.5 m and 1.5 m along,
 * every SLOW_DT. On two rates, the end cells are fast. */
static void pulse(int open, int two_rates, double *c)
{
    static rb_chamber air;
    rb_chamber_init_reservoir(&air, P0, 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, 64, 2.0, 0.04, P0, 300.0, AIR, 300.0, 0.0);
    if (open) rb_pipe_join(&pipe, 1, &air, 0.0, 0);
    if (two_rates) {
        rb_pipe_set_fast(&pipe, 0);
        rb_pipe_set_fast(&pipe, 1);
    }
    for (int k = 0; k < pipe.cells; k++) {
        const double x = (k + 0.5) * pipe.dx;
        squeeze(&pipe, k, P0 + 1000.0 * exp(-(x / 0.2) * (x / 0.2)), 300.0);
    }
    *c = rb_chamber_sound_speed(&pipe.cell[40]);
    for (long i = 0; i < lround(0.02 / SLOW_DT); i++) {
        if (two_rates)
            for (long j = 4 * i; j < 4 * i + 4; j++) step_two_rates(&pipe, j);
        else
            step(&pipe, SLOW_DT);
        trace[0][i] = pipe.cell[15].p - P0;   /* (15 + 0.5) / 64 * 2 = 0.484 m */
        trace[1][i] = pipe.cell[47].p - P0;   /* 1.484 m: one metre on        */
    }
}

void test_pipe_pulse_travels_at_the_speed_of_sound(void)
{
    /* The plan's check: the pulse takes L / c to go a metre, comes back from
     * the closed end as it went, and from the open end inverted, as if the
     * pipe were 0.3 diameters longer (the end correction), within a
     * centimetre. It hardly fades: the scheme barely damps a wave. (Six
     * cells across, the pulse runs 0.3 % slow: in this scheme a wave of
     * wavenumber k travels at c cos(k dx / 2).) */
    double c, size, size_back;
    pulse(1, 0, &c);
    const double out = extreme(0, SLOW_DT, 0.0, 3e-3, 1.0, &size);
    const double on = extreme(1, SLOW_DT, 1e-3, 6e-3, 1.0, NULL);
    CHECK_NEAR(on - out, 1.0 / c, 0.005 / c);                    /* 2.88 ms, within 0.5 % */
    CHECK(size > 495.0 && size < 505.0);                         /* half went each way: 500 Pa */
    const double end = 2.0 + 0.3 * 0.04;                         /* where the open end reflects */
    const double back = extreme(1, SLOW_DT, 6e-3, 9e-3, -1.0, &size_back);
    CHECK_NEAR(back - on, 2.0 * (end - 1.484375) / c, 0.01 / c);
    CHECK(size_back < -0.97 * size && size_back > -1.01 * size);   /* inverted */

    pulse(0, 0, &c);   /* both ends closed: it comes back the right way up */
    const double on2 = extreme(1, SLOW_DT, 1e-3, 6e-3, 1.0, NULL);
    const double back2 = extreme(1, SLOW_DT, 6e-3, 9e-3, 1.0, &size_back);
    CHECK_NEAR(back2 - on2, 2.0 * (2.0 - 1.484375) / c, 0.01 / c);
    CHECK(size_back > 0.97 * size && size_back < 1.01 * size);
}

/* Air between two big tanks dp apart: a pipe 2 m long and 40 mm across, with
 * friction f = 0.02, an entry loss of 0.5, a laminar loss (Pa per m/s) at its
 * far end, and both ends narrowed to 1 / narrow of its area (1: not at all),
 * run for a second (the pipe's own ringing dies down eightfold every 0.1 s).
 * Packed (stage 12) with `packing` Pa per m/s per metre, settling in 0.3 ms
 * (a muffler's). */
static double steady(double dp, int n, double laminar, double narrow, double packing)
{
    static rb_chamber in, out;
    rb_chamber_init_reservoir(&in, P0 + dp, 300.0, AIR);
    rb_chamber_init_reservoir(&out, P0, 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, n, 2.0, 0.04, P0, 300.0, AIR, 300.0, 0.02);
    rb_pipe_join(&pipe, 0, &in, 0.5, 0);
    rb_pipe_join(&pipe, 1, &out, 0.0, 0);
    rb_pipe_resist(&pipe, 1, laminar);
    rb_pipe_pack(&pipe, packing, 3e-4);
    if (narrow > 1.0) {
        rb_pipe_restrict(&pipe, 0, pipe.area / narrow);
        rb_pipe_restrict(&pipe, 1, pipe.area / narrow);
    }
    for (long i = 0; i < lround(1.0 / SLOW_DT); i++) step(&pipe, SLOW_DT);
    return pipe.end_face[0].mdot / (0.5 * (in.rho + out.rho) * pipe.area);
}

void test_pipe_friction(void)
{
    /* A steady flow loses its entry, its dynamic head and the friction along
     * the pipe (Darcy-Weisbach): dp = (1 + K_in + f L / D) rho v^2 / 2, with
     * the same flow through every face, however many cells; and what a
     * laminar loss or narrowed ends add. */
    const double rho = (P0 + 250.0) / (287.0985 * 300.0);   /* the mean */
    const double want = sqrt(2.0 * 500.0 / (rho * (1.0 + 0.5 + 0.02 * 2.0 / 0.04)));
    CHECK_NEAR(steady(500.0, 40, 0.0, 1.0, 0.0), want, 0.003 * want);  /* 18.4 m/s, Mach 0.05 */
    int uneven = 0;
    for (int k = 0; k < pipe.cells - 1; k++)
        uneven += fabs(pipe.face[k].mdot - pipe.end_face[0].mdot) > 1e-9 * pipe.end_face[0].mdot;
    CHECK_EQ_INT(uneven, 0);
    CHECK_NEAR(steady(500.0, 6, 0.0, 1.0, 0.0), want, 0.003 * want);
    /* A laminar loss of 10 Pa per m/s on top: a v^2 + 10 v = dp. */
    const double a = 0.5 * rho * (1.0 + 0.5 + 0.02 * 2.0 / 0.04);
    const double slow = (-10.0 + sqrt(100.0 + 4.0 * a * 500.0)) / (2.0 * a);
    CHECK_NEAR(steady(500.0, 40, 10.0, 1.0, 0.0), slow, 0.003 * slow);  /* 11.5 m/s */
    /* Both ends narrowed to 80 % of the pipe's area, a catalyst's or a muffler's
     * way through: in, the gas speeds up through the narrowing and spreads out
     * again, (A/A_e - 1)^2 more velocity heads; out, its jet is faster,
     * (A/A_e)^2 - 1 more. */
    const double r = 1.25, heads = 1.0 + 0.5 + (r - 1.0) * (r - 1.0) + 0.02 * 2.0 / 0.04 + r * r - 1.0;
    const double narrowed = sqrt(2.0 * 500.0 / (rho * heads));
    CHECK_NEAR(steady(500.0, 40, 0.0, r, 0.0), narrowed, 0.003 * narrowed);   /* 16.4 m/s */
    /* At Mach 0.17 the gas thins along the pipe and speeds up: a little less
     * flow than the incompressible formula says. */
    const double fast = sqrt(2.0 * 5000.0 / ((P0 + 2500.0) / (287.0985 * 300.0) * (1.0 + 0.5 + 0.02 * 2.0 / 0.04)));
    const double v = steady(5000.0, 40, 0.0, 1.0, 0.0);
    CHECK(v < fast && v > 0.97 * fast);
}

void test_pipe_waves_ride_the_flow(void)
{
    /* A small pulse in a pipe with air flowing at about 70 m/s goes
     * downstream at c + u and upstream at c - u. (On these 6 cm cells, a
     * 25 cm pulse would run 1.8 % slow either way, and ride the flow at
     * 0.95 u: 50 cm, 0.5 % and 0.98 u.) */
    static rb_chamber in, out;
    rb_chamber_init_reservoir(&in, P0 + 3000.0, 300.0, AIR);
    rb_chamber_init_reservoir(&out, P0, 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, 64, 4.0, 0.04, P0, 300.0, AIR, 300.0, 0.0);
    rb_pipe_join(&pipe, 0, &in, 0.0, 0);
    rb_pipe_join(&pipe, 1, &out, 0.0, 0);
    for (long i = 0; i < lround(1.0 / SLOW_DT); i++) step(&pipe, SLOW_DT);
    const double u = rb_pipe_cell_velocity(&pipe, 32), c = rb_chamber_sound_speed(&pipe.cell[32]);
    for (int k = 0; k < pipe.cells; k++) {
        const double x = (k + 0.5) * pipe.dx - 2.0;
        squeeze(&pipe, k, pipe.cell[k].p + 200.0 * exp(-(x / 0.5) * (x / 0.5)), pipe.cell[k].T);
    }
    const double down0 = pipe.cell[47].p, up0 = pipe.cell[16].p;   /* 15.5 cells either side */
    for (long i = 0; i < lround(0.006 / SLOW_DT); i++) {
        step(&pipe, SLOW_DT);
        trace[0][i] = pipe.cell[47].p - down0;
        trace[1][i] = pipe.cell[16].p - up0;
    }
    const double down = 15.5 * pipe.dx / extreme(0, SLOW_DT, 0.0, 0.006, 1.0, NULL);
    const double up = 15.5 * pipe.dx / extreme(1, SLOW_DT, 0.0, 0.006, 1.0, NULL);
    CHECK(u > 65.0 && u < 80.0);
    CHECK_NEAR(down, c + u, 0.01 * (c + u));
    CHECK_NEAR(up, c - u, 0.01 * (c - u));
}

/* Acoustic packet energy in a lossless pipe: the sum of compression and
 * velocity perturbation energies. Compare both flow directions and a
 * donor-cell control against the same physical initial condition. */
static double packet_energy_ratio(int order, int direction)
{
    static rb_chamber in, out;
    double base_p[64], base_u[64], rho[64], sound[64];
    const double pressure = direction == 0 ? 0.0 : 3000.0;
    rb_chamber_init_reservoir(&in, P0 + (direction >= 0 ? pressure : 0.0), 300.0, AIR);
    rb_chamber_init_reservoir(&out, P0 + (direction < 0 ? pressure : 0.0), 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, 64, 4.0, 0.04, P0, 300.0, AIR, 300.0, 0.0);
    if (order) pipe.momentum_order = order;
    rb_pipe_join(&pipe, 0, &in, 0.0, 0);
    rb_pipe_join(&pipe, 1, &out, 0.0, 0);
    for (long i = 0; i < lround(1.0 / SLOW_DT); i++) step(&pipe, SLOW_DT);
    const double c = rb_chamber_sound_speed(&pipe.cell[32]);
    const double u = rb_pipe_cell_velocity(&pipe, 32);
    const double sign = direction < 0 ? -1.0 : 1.0;
    const double wave_number = 2.0 * acos(-1.0) * 800.0 / (c + fabs(u));
    for (int k = 0; k < 64; k++) {
        base_p[k] = pipe.cell[k].p;
        base_u[k] = rb_pipe_cell_velocity(&pipe, k);
        rho[k] = pipe.cell[k].rho;
        sound[k] = rb_chamber_sound_speed(&pipe.cell[k]);
        const double x = (k + 0.5) * pipe.dx - 2.0;
        const double dp = 100.0 * exp(-x * x / (0.3 * 0.3)) * cos(wave_number * x);
        squeeze(&pipe, k, base_p[k] + dp, pipe.cell[k].T);
    }
    for (int k = 0; k < 63; k++) {
        const double x = (k + 1.0) * pipe.dx - 2.0;
        const double dp = 100.0 * exp(-x * x / (0.3 * 0.3)) * cos(wave_number * x);
        pipe.face[k].mdot += sign * pipe.area * dp / c;
    }
    double energy[2] = {0.0, 0.0};
    for (int pass = 0; pass < 2; pass++) {
        for (int k = 0; k < 64; k++) {
            const double v = rb_pipe_cell_velocity(&pipe, k);
            const double dp = pipe.cell[k].p - base_p[k], dv = v - base_u[k];
            energy[pass] += (dp * dp / (rho[k] * sound[k] * sound[k]) + rho[k] * dv * dv)
                            * pipe.area * pipe.dx;
            if (!pass) pipe.kinetic[k] = 0.5 * pipe.cell[k].mass * v * v;
        }
        if (!pass)
            for (long i = 0; i < lround(0.0025 / SLOW_DT); i++) step(&pipe, SLOW_DT);
    }
    return energy[1] / energy[0];
}

void test_pipe_acoustic_transport(void)
{
    for (int direction = -1; direction <= 1; direction++) {
        const double donor = packet_energy_ratio(1, direction);
        const double limited = packet_energy_ratio(0, direction); /* shipped default */
        CHECK(isfinite(limited) && limited > 0.0 && limited < 1.01);
        if (direction) {
            /* Ideal lossless transport retains 100%. On this coarse grid
             * donor-cell retains ~39%, limited transport ~60%. Require a
             * meaningful reduction in loss without permitting amplification. */
            CHECK(limited > 0.55 && donor < 0.45);
            CHECK(limited > 1.35 * donor);
        } else {
            CHECK_NEAR(limited, 1.0, 0.01);
            CHECK_NEAR(limited, donor, 0.005);
        }
    }
}

void test_pipe_walls(void)
{
    /* Burned gas at 1000 K flowing steadily through a pipe with its walls at
     * 500 K cools on the way, exponentially: by Colburn's analogy,
     * T_out - T_wall = (T_in - T_wall) exp(-St 4 L / D), St = (f / 8) Pr^(-2/3). */
    static rb_chamber in, out;
    rb_chamber_init_reservoir(&in, P0 + 3000.0, 1000.0, BURNED);
    rb_chamber_init_reservoir(&out, P0, 1000.0, BURNED);
    rb_pipe_init(&pipe, cells, faces, 40, 2.0, 0.04, P0, 1000.0, BURNED, 500.0, 0.02);
    rb_pipe_join(&pipe, 0, &in, 0.0, 0);
    rb_pipe_join(&pipe, 1, &out, 0.0, 0);
    for (long i = 0; i < lround(0.5 / SLOW_DT); i++) step(&pipe, SLOW_DT);
    const double st = 0.02 / 8.0 * 1.27;
    const double hot = pipe.cell[0].T, cold = pipe.cell[39].T;
    const double want = 500.0 + (hot - 500.0) * exp(-st * 4.0 * 39.0 * pipe.dx / 0.04);   /* 762 K */
    CHECK(hot < 1000.0 && hot > 980.0);   /* a bit cooler as it speeds up into the pipe */
    CHECK_NEAR(cold, want, 0.015 * (hot - want));
    CHECK(pipe.heat < 0.0);
}

/* What's in the test's chambers and pipe: mass, energy, each species. */
typedef struct totals {
    double mass, energy, species[RB_NSPECIES];
} totals;

static totals count(const rb_chamber *a, const rb_chamber *b, const rb_pipe *p)
{
    totals t;
    rb_pipe_contents(p, &t.mass, &t.energy, t.species);
    t.mass += a->mass + b->mass;
    t.energy += a->U + b->U;
    for (int s = 0; s < RB_NSPECIES; s++) t.species[s] += a->m[s] + b->m[s];
    return t;
}

void test_pipe_conserves_everything(void)
{
    /* A litre of burned gas at 3 bar and 1100 K blowing down through a pipe
     * with friction and warm walls into 5 L of air: everything that left the
     * small tank is in the pipe or the big one, every species, and every
     * joule bar what the walls took. Once on one rate, once on two: the
     * pipe's end cells stepped every step with their tanks, the rest every
     * fourth, the books exact at every step in between. */
    for (int rates = 1; rates <= 2; rates++) {
        static rb_chamber hot, cold;
        rb_chamber_init(&hot, 1e-3, 3e5, 1100.0, BURNED);
        rb_chamber_init(&cold, 5e-3, P0, 300.0, AIR);
        rb_pipe_init(&pipe, cells, faces, 12, 0.6, 0.035, P0, 300.0, AIR, 600.0, 0.025);
        rb_pipe_join(&pipe, 0, &hot, 0.4, rates == 2);
        rb_pipe_join(&pipe, 1, &cold, 0.4, rates == 2);
        if (rates == 2) {
            rb_pipe_set_fast(&pipe, 0);
            rb_pipe_set_fast(&pipe, 1);
        }
        const totals t0 = count(&hot, &cold, &pipe);
        double heat = 0.0, worst_mass = 0.0, worst_energy = 0.0, worst_species = 0.0;
        int repairs = 0;
        for (long i = 0; i < lround(0.05 / FAST_DT); i++) {
            /* the fast pass (on one rate, the only one): the tanks with it;
             * on two, the slow pass around it in blocks of four steps */
            const int fast = (rates == 2);
            if (fast && i % 4 == 0) {
                rb_pipe_clear(&pipe, 0);
                rb_pipe_flows(&pipe, 0, 4.0 * FAST_DT);
            }
            rb_chamber_clear_rates(&hot);
            rb_chamber_clear_rates(&cold);
            rb_pipe_clear(&pipe, fast);
            rb_pipe_flows(&pipe, fast, FAST_DT);
            rb_pipe_integrate(&pipe, fast, FAST_DT);
            rb_chamber_integrate(&hot, FAST_DT);
            rb_chamber_integrate(&cold, FAST_DT);
            if (fast && i % 4 == 3) {
                rb_pipe_integrate(&pipe, 0, 4.0 * FAST_DT);
                rb_pipe_accelerate(&pipe, 0, 4.0 * FAST_DT);
            }
            rb_pipe_accelerate(&pipe, fast, FAST_DT);
            const totals t = count(&hot, &cold, &pipe);
            heat = pipe.heat;
            worst_mass = fmax(worst_mass, fabs(t.mass - t0.mass));
            worst_energy = fmax(worst_energy, fabs(t.energy - t0.energy - heat));
            for (int s = 0; s < RB_NSPECIES; s++)
                worst_species = fmax(worst_species, fabs(t.species[s] - t0.species[s]));
        }
        for (int k = 0; k < pipe.cells; k++) repairs += pipe.cell[k].repairs;
        CHECK(worst_mass < 1e-14 * t0.mass);
        CHECK(worst_species < 1e-14 * t0.mass);
        CHECK(worst_energy < 1e-11 * fabs(t0.energy));
        CHECK_EQ_INT(repairs + hot.repairs + cold.repairs, 0);
        CHECK(cold.y[RB_BURNED] > 0.03 && hot.p < 1.6e5);   /* it did blow down */
        CHECK(heat != 0.0);
    }
}

void test_pipe_two_rates_agree(void)
{
    /* The pulse of test_pipe_pulse_travels_at_the_speed_of_sound with its two
     * end cells fast (every step at 192 kHz) and the rest slow (every fourth
     * step): it arrives when it does on one rate, as big. */
    double c, size1, size2;
    pulse(1, 0, &c);
    const double t1 = extreme(1, SLOW_DT, 1e-3, 6e-3, 1.0, &size1);
    pulse(1, 1, &c);
    const double t2 = extreme(1, SLOW_DT, 1e-3, 6e-3, 1.0, &size2);
    CHECK_NEAR(t2, t1, 0.002 * t1);
    CHECK_NEAR(size2, size1, 0.01 * size1);
}

/* Rings a pipe 0.5 m long and 40 mm across, open to the air at end 0, with a
 * chamber of `volume` (m^3) on end 1 (none: a closed end), for 0.1 s on 5 cm
 * cells. It starts in the shape of its slowest ring, p = 1 bar + 1 kPa
 * sin(k x) (x from the end correction beyond the open end), so the faster
 * ones hardly show. Returns the period of the pressure swings at end 1 (s,
 * from its upward crossings of 1 bar). */
static double ring(double volume, double k)
{
    static rb_chamber air, cavity;
    rb_chamber_init_reservoir(&air, 1e5, 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, 10, 0.5, 0.04, 1e5, 300.0, AIR, 300.0, 0.0);
    rb_pipe_join(&pipe, 0, &air, 0.0, 0);
    for (int j = 0; j < pipe.cells; j++) {
        rb_chamber_init(&pipe.cell[j], pipe.area * pipe.dx, 1e5, 300.0, AIR);
        squeeze(&pipe, j, 1e5 + 1000.0 * sin(k * ((j + 0.5) * pipe.dx + 0.3 * 0.04)), 300.0);
    }
    if (volume > 0.0) {
        rb_chamber_init(&cavity, volume, 1e5 + 1000.0 * sin(k * (0.5 + 0.3 * 0.04)), 300.0, AIR);
        rb_pipe_join(&pipe, 1, &cavity, 0.0, 0);
    }
    const rb_chamber *watch = (volume > 0.0) ? &cavity : &pipe.cell[9];
    double t_first = -1.0, t_last = -1.0, prev = watch->p - 1e5;
    int crossings = 0;
    for (long n = 1; n <= lround(0.1 / SLOW_DT); n++) {
        rb_pipe_clear(&pipe, 0);
        if (volume > 0.0) rb_chamber_clear_rates(&cavity);
        rb_pipe_flows(&pipe, 0, SLOW_DT);
        rb_pipe_integrate(&pipe, 0, SLOW_DT);
        if (volume > 0.0) rb_chamber_integrate(&cavity, SLOW_DT);
        rb_pipe_accelerate(&pipe, 0, SLOW_DT);
        const double swing = watch->p - 1e5;
        if (prev < 0.0 && swing >= 0.0) {   /* between steps n - 1 and n */
            const double t = (n - 1 + prev / (prev - swing)) * SLOW_DT;
            if (t_first < 0.0) t_first = t;
            else crossings++;
            t_last = t;
        }
        prev = swing;
    }
    return crossings > 0 ? (t_last - t_first) / crossings : 0.0;
}

void test_pipe_rings(void)
{
    /* A pipe closed at one end and open at the other rings at c / 4L, as an
     * organ pipe does, L counting the end correction at the open end (d,
     * 0.3 D): 169.5 Hz here. With a 0.1 L chamber on its end instead (an
     * intake runner on its port and cylinder) it rings where
     * tan(k (L + d)) + k d = A / (k V), k = 2 pi f / c: the moving air beyond
     * each end adds weight but no room. That's 146 Hz, for a pipe holding
     * 0.63 L of air itself. The stage 9 runner, one column of air with no
     * room of its own, rang at the Helmholtz frequency c sqrt(A / (L V)) / 2 pi:
     * 277 Hz. */
    const double pi = 3.14159265358979323846, d = 0.3 * 0.04;
    const double c = sqrt(1.4 * 287.0985 * 300.0), A = 0.25 * pi * 0.04 * 0.04, L = 0.5 + d;
    const double organ = 4.0 * L / c;
    CHECK_NEAR(ring(0.0, pi / (2.0 * L)), organ, 0.005 * organ);
    double lo = 0.0, hi = pi / (2.0 * L);   /* k, by bisection */
    for (int i = 0; i < 60; i++) {
        const double k = 0.5 * (lo + hi);
        if (tan(k * L) + k * d < A / (k * 1e-4)) lo = k;
        else hi = k;
    }
    const double runner = 2.0 * pi / (c * lo);
    CHECK_NEAR(ring(1e-4, lo), runner, 0.01 * runner);
    CHECK(runner > 1.8 / 277.0);
}

/* Rings a pipe `length` m long and 40 mm across, closed at both ends, packed
 * with `packing` Pa per m/s per metre for swings faster than `settle` (s),
 * for 0.1 s from the shape of its slowest ring, p = 1 bar + 20 Pa
 * cos(pi x / L) (c / 2L; small, so it doesn't steepen). Returns how fast the
 * swings at end 0 fade (1/s), from the first and last of their peaks.
 * *energy: how much the pipe's energy changed, over all of it. */
static double packed_ring(double length, double packing, double settle, double *energy)
{
    rb_pipe_init(&pipe, cells, faces, 20, length, 0.04, 1e5, 300.0, AIR, 300.0, 0.0);
    rb_pipe_pack(&pipe, packing, settle);
    for (int j = 0; j < pipe.cells; j++) {
        rb_chamber_init(&pipe.cell[j], pipe.area * pipe.dx, 1e5, 300.0, AIR);
        squeeze(&pipe, j, 1e5 + 20.0 * cos(3.14159265358979323846 * (j + 0.5) / pipe.cells), 300.0);
    }
    double e0, e1;
    rb_pipe_contents(&pipe, NULL, &e0, NULL);
    const long steps = lround(0.1 / SLOW_DT);
    for (long n = 0; n < steps; n++) {
        step(&pipe, SLOW_DT);
        trace[0][n] = pipe.cell[0].p - 1e5;
    }
    rb_pipe_contents(&pipe, NULL, &e1, NULL);
    *energy = fabs(e1 - e0) / e0;
    long first = -1, last = -1;   /* the first and last peaks of the swing */
    for (long n = 1; n + 1 < steps; n++)
        if (trace[0][n] > trace[0][n - 1] && trace[0][n] >= trace[0][n + 1] && trace[0][n] > 4.0) {
            if (first < 0) first = n;
            last = n;
        }
    if (first < 0 || last <= first) return NAN;
    return log(trace[0][first] / trace[0][last]) / ((last - first) * SLOW_DT);
}

/* How fast a packing (R Pa per m/s per metre, settling in tau) makes a ring
 * of gas of density rho at omega fade (1/s): its loss, R (omega tau)^2 /
 * (1 + (omega tau)^2), over twice the gas's weight and the packing's, which
 * is R tau / (1 + (omega tau)^2) (pipe.h), at the slower omega that weight
 * rings it at. */
static double packing_fade(double R, double tau, double rho, double omega)
{
    const double x0 = omega * tau;
    const double w = omega * sqrt(rho / (rho + R * tau / (1.0 + x0 * x0))), x = w * tau;
    return R * x * x / (1.0 + x * x) / (2.0 * (rho + R * tau / (1.0 + x * x)));
}

void test_pipe_packing(void)
{
    /* A muffler's packing (stage 12). A ring much faster than the settling
     * time (173 Hz, settling in 10 ms) dies away as a laminar loss R makes it,
     * its swings falling as exp(-R t / (2 rho)), and its energy stays in the
     * gas, warming it. One much slower (43 Hz, settling in 0.4 ms) gets only
     * (omega tau)^2 / (1 + (omega tau)^2) of the loss, 1.1 %: with five times
     * the packing it keeps 95 % over 0.1 s, where the same laminar loss would
     * leave 0.01 %. Both to 1 %, counting the weight the packing adds to slow
     * swings (3 % here). The unpacked pipe keeps its ring. A steady flow goes
     * through as if the pipe weren't packed. */
    const double rho = 1e5 / (287.0985 * 300.0), c = sqrt(1.4 * 287.0985 * 300.0), pi = 3.14159265358979323846;
    double energy;
    const double bare = packed_ring(1.0, 0.0, 0.0, &energy), bare4 = packed_ring(4.0, 0.0, 0.0, &energy);
    CHECK(fabs(bare) < 0.03 && fabs(bare4) < 0.03);   /* 1/s: under 0.3 % in 0.1 s */
    const double fast = packed_ring(1.0, 20.0, 0.01, &energy) - bare;
    const double want_fast = packing_fade(20.0, 0.01, rho, pi * c);   /* 8.5/s */
    CHECK_NEAR(fast, want_fast, 0.01 * want_fast);
    CHECK(energy < 1e-12);
    const double slow = packed_ring(4.0, 100.0, 4e-4, &energy) - bare4;
    const double want_slow = packing_fade(100.0, 4e-4, rho, pi * c / 4.0);   /* 0.47/s */
    CHECK_NEAR(slow, want_slow, 0.01 * want_slow);
    CHECK(exp(-slow * 0.1) > 0.95);

    const double open = steady(500.0, 40, 0.0, 1.0, 0.0), packed = steady(500.0, 40, 0.0, 1.0, 1500.0);
    CHECK_NEAR(packed, open, 1e-6 * open);
}

static rb_chamber cells2[RB_PIPE_MAX_CELLS];
static rb_face faces2[RB_PIPE_MAX_CELLS];
static rb_pipe pipe2;

/* One slow step of two pipes, a going on into b (or joined to it: the clear
 * of both before either's flows). */
static void step2(rb_pipe *a, rb_pipe *b, double dt)
{
    rb_pipe_clear(a, 0);
    rb_pipe_clear(b, 0);
    rb_pipe_flows(a, 0, dt);
    rb_pipe_flows(b, 0, dt);
    rb_pipe_integrate(a, 0, dt);
    rb_pipe_integrate(b, 0, dt);
    rb_pipe_accelerate(a, 0, dt);
    rb_pipe_accelerate(b, 0, dt);
}

/* A 2 m pipe, 40 mm across, of still air at 300 K, closed at both ends: as
 * two 1 m pipes of 32 cells, a going on into b (how = 1) or joined to b's
 * first cell as to a volume (how = 0), or as one pipe of 64 cells (how = 2).
 * A 200 Pa pulse starts 0.5 m along, as in pulse(); trace 0 is the pressure
 * 0.266 m along (where an echo off the joint would pass), trace 1 1.516 m. */
static void pulse_across(int how)
{
    if (how == 2) {
        rb_pipe_init(&pipe, cells, faces, 64, 2.0, 0.04, P0, 300.0, AIR, 300.0, 0.0);
    } else {
        rb_pipe_init(&pipe, cells, faces, 32, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.0);
        rb_pipe_init(&pipe2, cells2, faces2, 32, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.0);
        if (how == 1) rb_pipe_onward(&pipe, &pipe2);
        else rb_pipe_join(&pipe, 1, rb_pipe_end_cell(&pipe2, 0), 0.0, 0);
    }
    for (int k = 0; k < 32; k++) {
        const double x = (k + 0.5) * pipe.dx - 0.5;
        squeeze(&pipe, k, P0 + 200.0 * exp(-(x / 0.1) * (x / 0.1)), 300.0);
    }
    for (long i = 0; i < lround(0.006 / SLOW_DT); i++) {
        if (how == 2) step(&pipe, SLOW_DT);
        else step2(&pipe, &pipe2, SLOW_DT);
        trace[0][i] = pipe.cell[8].p - P0;                                   /* 0.266 m */
        trace[1][i] = (how == 2 ? pipe.cell[48].p : pipe2.cell[16].p) - P0;   /* 1.516 m */
    }
}

void test_pipe_most_sonic(void)
{
    /* However hard a column is pushed, a face never lets through more than
     * RB_PIPE_MOST_SONIC times the sonic flow of the gas it comes from,
     * A sqrt(1.4 rho p). A pipe from 10 bar into a near vacuum (1 kPa) drives
     * its faces there for 20 ms: they reach the guard, and never pass it. */
    static rb_chamber hi, lo;
    rb_chamber_init_reservoir(&hi, 10.0 * P0, 300.0, AIR);
    rb_chamber_init_reservoir(&lo, 1000.0, 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, 20, 1.0, 0.04, 1000.0, 300.0, AIR, 300.0, 0.02);
    rb_pipe_join(&pipe, 0, &hi, 0.5, 0);
    rb_pipe_join(&pipe, 1, &lo, 0.0, 0);
    double worst = 0.0;
    for (long i = 0; i < lround(0.02 / SLOW_DT); i++) {
        step(&pipe, SLOW_DT);
        for (int k = -1; k < pipe.cells; k++) {   /* the end faces too */
            const rb_face *f = (k < 0) ? &pipe.end_face[0] : (k == pipe.cells - 1) ? &pipe.end_face[1]
                                                                                    : &pipe.face[k];
            const rb_chamber *from = (f->mdot >= 0.0) ? f->a : f->b;
            worst = fmax(worst, fabs(f->mdot) / (f->area * sqrt(1.4 * from->rho * from->p)));
        }
    }
    CHECK(worst > 1.9 && worst <= RB_PIPE_MOST_SONIC * (1.0 + 1e-12));
}

void test_pipe_onward(void)
{
    /* Two pipes going on into each other (stage 12) are one pipe: a pulse
     * crosses where they meet as it crosses any face, in the time one pipe
     * takes and as big (to 0.01 %), and what comes back past 0.266 m is one
     * pipe's own 0.2 % (joined as through a volume, the joint echoes 1.9 %).
     * A steady flow between two tanks loses what it would in one pipe (to
     * 0.1 %), and mass and energy are conserved exactly. */
    const double c = sqrt(1.4 * 287.0985 * 300.0);
    double t[3], size[3], echo[3];
    for (int how = 0; how < 3; how++) {
        pulse_across(how);
        /* the half going right passes 1.516 m 1.016 m / c after it set off; an
         * echo off the joint (at 1 m) would pass 0.266 m 1.234 m / c after */
        t[how] = extreme(1, SLOW_DT, 2.0e-3, 4.5e-3, 1.0, &size[how]);
        echo[how] = 0.0;
        for (long i = lround((1.234 - 0.15) / c / SLOW_DT); i < lround((1.234 + 0.15) / c / SLOW_DT); i++)
            echo[how] = fmax(echo[how], fabs(trace[0][i]) / 100.0);
    }
    CHECK_NEAR(t[1], t[2], 1e-4 * t[2]);
    CHECK_NEAR(size[1], size[2], 1e-4 * size[2]);
    CHECK(echo[1] < 1.1 * echo[2] && echo[2] < 0.003);
    CHECK(echo[0] > 5.0 * echo[2]);
    /* steady, between two tanks 500 Pa apart: as test_pipe_friction's 2 m pipe */
    static rb_chamber in, out;
    rb_chamber_init_reservoir(&in, P0 + 500.0, 300.0, AIR);
    rb_chamber_init_reservoir(&out, P0, 300.0, AIR);
    rb_pipe_init(&pipe, cells, faces, 20, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.02);
    rb_pipe_init(&pipe2, cells2, faces2, 20, 1.0, 0.04, P0, 300.0, AIR, 300.0, 0.02);
    rb_pipe_join(&pipe, 0, &in, 0.5, 0);
    CHECK_EQ_INT(rb_pipe_onward(&pipe, &pipe2), 0);
    rb_pipe_join(&pipe2, 1, &out, 0.0, 0);
    double m0, e0, m1, e1;
    for (long i = 0; i < lround(1.0 / SLOW_DT); i++) step2(&pipe, &pipe2, SLOW_DT);
    const double two = pipe.end_face[0].mdot;
    const double u_a = rb_pipe_cell_velocity(&pipe, pipe.cells - 1), u_b = rb_pipe_cell_velocity(&pipe2, 0);
    CHECK_NEAR(u_b, u_a, 0.01 * u_a);   /* the gas goes on at the speed it had */
    const double one = steady(500.0, 40, 0.0, 1.0, 0.0);
    CHECK_NEAR(two / (0.5 * (in.rho + out.rho) * pipe.area), one, 0.001 * one);

    /* closed at both outer ends, with a pressure step: the books are exact */
    rb_pipe_init(&pipe, cells, faces, 12, 0.6, 0.035, 2e5, 900.0, BURNED, 600.0, 0.025);
    rb_pipe_init(&pipe2, cells2, faces2, 9, 0.6, 0.035, P0, 300.0, AIR, 400.0, 0.025);
    rb_pipe_onward(&pipe, &pipe2);
    rb_pipe_contents(&pipe, &m0, &e0, NULL);
    rb_pipe_contents(&pipe2, &m1, &e1, NULL);
    const double mass = m0 + m1, energy = e0 + e1;
    for (long i = 0; i < 2000; i++) step2(&pipe, &pipe2, SLOW_DT);
    rb_pipe_contents(&pipe, &m0, &e0, NULL);
    rb_pipe_contents(&pipe2, &m1, &e1, NULL);
    CHECK_NEAR(m0 + m1, mass, 1e-14 * mass);
    CHECK_NEAR(e0 + e1 - pipe.heat - pipe2.heat, energy, 1e-11 * energy);
    CHECK(pipe2.cell[0].y[RB_BURNED] > 0.1);   /* the burned gas got across */
    CHECK_EQ_INT(rb_pipe_onward(&pipe, &pipe), -1);

    /* Bores more than RB_PIPE_ONWARD_MAX_RATIO apart in area don't go straight
     * on (stage 13: their momentum fluxes pump the flow from the narrow one into
     * the wide one, and the exhaust's pulses ran away with it), either way. */
    rb_pipe_init(&pipe, cells, faces, 12, 0.6, 0.055, P0, 300.0, AIR, 300.0, 0.02);
    rb_pipe_init(&pipe2, cells2, faces2, 9, 0.6, 0.075, P0, 300.0, AIR, 300.0, 0.02);   /* x1.86 */
    CHECK_EQ_INT(rb_pipe_onward(&pipe, &pipe2), -1);
    CHECK_EQ_INT(rb_pipe_onward(&pipe2, &pipe), -1);
    rb_pipe_init(&pipe2, cells2, faces2, 9, 0.6, 0.065, P0, 300.0, AIR, 300.0, 0.02);   /* x1.40 */
    CHECK_EQ_INT(rb_pipe_onward(&pipe, &pipe2), 0);
}
