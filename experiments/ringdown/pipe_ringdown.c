/*
 * pipe_ringdown.c - stage 144, area 6: how fast a pipe's own resonance dies
 * in the solver, against the walls' viscothermal loss (Kirchhoff).
 *
 * A pipe closed at both ends loses its sound only through its walls: no
 * radiation, no junction. Its first mode is set up exactly (p = p0 + A
 * cos(pi x / L) in every cell, isentropic) and left to ring; the mode's
 * amplitude, read from the end cells' difference each period, decays as
 * exp(-sigma t). Kirchhoff's wide-tube loss for the same gas gives
 *
 *   alpha = sqrt(omega nu / 2) / (r c) * (1 + (gamma - 1) / sqrt(Pr))   (Np/m)
 *   sigma = alpha c                                                      (1/s)
 *
 * and the solver's sigma is printed beside it, for the engine's Darcy
 * friction (0.025) and none, at 300 and 800 K, the exhaust's 2 in bore and its
 * 7.5 cm and 2.5 cm cells. The solver has no viscothermal term: what decay it
 * shows is its friction (quadratic in the flow: small for small waves) and
 * its scheme's own damping.
 *
 * Build: cc -O2 -std=c11 -Icore/include -Icore/src experiments/ringdown/pipe_ringdown.c \
 *        core/src/pipe.c core/src/chamber.c core/src/gas.c core/src/flow.c core/src/heat.c \
 *        core/src/radiation.c core/src/diagnostics.c -lm
 */
#include "pipe.h"
#include "rb.h"

#include <math.h>
#include <stdio.h>

static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
static const double PI = 3.14159265358979323846;
#define P0 101325.0
#define DT (1.0 / 48000.0)

static rb_chamber cells[RB_PIPE_MAX_CELLS];
static rb_face faces[RB_PIPE_MAX_CELLS];

/* Kirchhoff's decay rate (1/s) of a plane wave of f Hz in a pipe of bore d
 * at T (air: the viscosity and conductivity at T by Sutherland's law). */
static double kirchhoff(double f, double d, double T, double gamma)
{
    const double R = 287.0985, Pr = 0.71;
    const double mu = 1.716e-5 * pow(T / 273.15, 1.5) * (273.15 + 110.4) / (T + 110.4);
    const double nu = mu / (P0 / (R * T));
    const double c = sqrt(gamma * R * T);
    const double alpha = sqrt(PI * f * nu) / (0.5 * d * c) * (1.0 + (gamma - 1.0) / sqrt(Pr));
    return alpha * c;
}

/* The solver's decay rate of the first mode, and its frequency. */
static rb_wall_fit fit;

static double ring(double L, double d, double cell, double T, double friction, double amp, double *freq,
                   int walls, int mode)
{
    rb_pipe p;
    const int n = (int)lround(L / cell);
    rb_pipe_init(&p, cells, faces, n, L, d, P0, T, AIR, T, friction);
    for (int k = 0; k < n; k++) {
        const double x = (k + 0.5) / n, pk = P0 + amp * cos(mode * PI * x);
        const double g = rb_chamber_gamma(&p.cell[k]);
        rb_chamber_init(&p.cell[k], p.area * p.dx, pk, T * pow(pk / P0, (g - 1.0) / g), AIR);
    }
    if (walls) rb_pipe_walls(&p, &fit);
    const double c = sqrt(1.4 * 287.0985 * T);
    const double f = mode * c / (2.0 * L);
    const long steps = lround(1.5 / DT);
    /* the mode's amplitude each period: half the end cells' pressure
     * difference's peak over that period */
    const long per = lround(1.0 / f / DT);
    double first = 0.0, last = 0.0, t_first = 0.0, t_last = 0.0, peak = 0.0;
    long crossings = 0, prev_sign = 0;
    double t_cross0 = 0.0, t_cross1 = 0.0;
    for (long s = 0; s < steps; s++) {
        rb_pipe_clear(&p, 0);
        rb_pipe_flows(&p, 0, DT);
        rb_pipe_integrate(&p, 0, DT);
        rb_pipe_accelerate(&p, 0, DT);
        const double diff = (mode % 2) ? 0.5 * (p.cell[0].p - p.cell[n - 1].p) : p.cell[0].p - P0;
        if (fabs(diff) > peak) peak = fabs(diff);
        const long sign = diff >= 0.0 ? 1 : -1;
        if (prev_sign && sign != prev_sign) {
            if (!crossings) t_cross0 = s * DT;
            t_cross1 = s * DT;
            crossings++;
        }
        prev_sign = sign;
        if ((s + 1) % per == 0) {
            const double t = (s + 1) * DT;
            /* from 20 ms (the start's settling) until it has fallen 40 dB:
             * past that the residue of other modes and roundoff, not the
             * mode's decay, would be read */
            if (t > 0.02 && first == 0.0) { first = peak; t_first = t; }
            if (t > 0.02 && peak > 0.01 * first) { last = peak; t_last = t; }
            peak = 0.0;
        }
    }
    *freq = (crossings > 1) ? 0.5 * (crossings - 1) / (t_cross1 - t_cross0) : 0.0;
    (void)mode;
    return log(first / last) / (t_last - t_first);
}

int main(void)
{
    const double L = 1.0, d = 0.0484;
    rb_wall_fit_init(&fit, DT);
    printf("A 1 m pipe of 48.4 mm bore closed at both ends, a mode at 100 Pa, the engine's friction:\n");
    printf("%6s %6s %5s | %9s %9s | %9s %9s %9s | %8s\n", "T K", "cell", "mode", "f solver", "f theory",
           "walls off", "walls on", "Kirchhoff", "on/Kh");
    const double temps[2] = { 300.0, 800.0 }, cellz[2] = { 0.075, 0.025 };
    const int modes[3] = { 1, 4, 8 };
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < 2; b++)
            for (int m = 0; m < 3; m++) {
                const double T = temps[a];
                if (cellz[b] < (sqrt(1.4 * 287.0985 * T) + 5.0) * DT) continue;   /* past the CFL floor */
                double fs, fs2;
                const double off = ring(L, d, cellz[b], T, 0.025, 100.0, &fs, 0, modes[m]);
                const double on = ring(L, d, cellz[b], T, 0.025, 100.0, &fs2, 1, modes[m]);
                rb_chamber air;
                rb_chamber_init(&air, 1.0, P0, T, AIR);
                const double g = rb_chamber_gamma(&air);   /* the solver's gas at T */
                const double ft = modes[m] * sqrt(g * 287.0985 * T) / (2.0 * L);
                const double kh = kirchhoff(ft, d, T, g);
                printf("%6.0f %6.3f %5d | %9.1f %9.1f | %9.3f %9.3f %9.3f | %8.3f\n", T, cellz[b], modes[m], fs2, ft,
                       off, on, kh, on / kh);
            }
    return 0;
}
