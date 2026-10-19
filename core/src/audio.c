/*
 * audio.c - the listener and the chain to the audio rate (see audio.h).
 */
#include "audio.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

/* The anti-alias filter: Kaiser's window, over a transition band a sixth of
 * the audio rate wide (0.417 to 0.583 of it). His formulas are estimates:
 * designed for 101 dB, the filters for factors 2 to 32 reach 100.4-101.4. */
#define AA_ATTENUATION 101.0   /* dB */

/* ---- the listener ---- */

/* The car, in metres: x to the right, y forwards from the rear bumper, z up
 * from the ground. A front-engined car about 4.5 m long. Its tailpipes stick
 * out 5 cm under the bumper, 35 cm up: one on the right, or one each side.
 * The intake's mouth (its snorkel) is behind the grille. */
static const double TAILPIPE[2][3] = { { 0.5, -0.05, 0.35 }, { -0.5, -0.05, 0.35 } };
static const double MOUTH[3] = { -0.3, 3.7, 0.75 };

/* Where the listener stands: its ears, or the microphone. */
static const double PLACE[RB_HEAR_COUNT][3] = {
    [RB_HEAR_TAILPIPE] = { 0.5, -1.05, 0.5 },   /* 1 m behind tailpipe 0, 15 cm higher */
    [RB_HEAR_BEHIND]   = { 0.6, -3.0, 1.6 },    /* 3 m behind, a little to the right   */
    [RB_HEAR_SIDE]     = { 4.0, 2.2, 1.6 },     /* 4 m to the right of the middle      */
    [RB_HEAR_FRONT]    = { -0.1, 3.4, 1.3 },    /* over the bonnet, near the mouth     */
};

static double distance(const double a[3], const double b[3])
{
    const double dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrt(dx * dx + dy * dy + dz * dz);
}

void rb_listener_set(rb_listener *l, const double *const pos[RB_LISTEN_SOURCES],
                     const double at[3], double ground, double c, double pipe_dt)
{
    /* Each path's length: straight, and off the ground (from the source's
     * image, as far below the ground as the source is above it). */
    double r[RB_LISTEN_SOURCES][2], nearest = INFINITY;
    for (int j = 0; j < RB_LISTEN_SOURCES; j++) {
        r[j][0] = r[j][1] = 0.0;
        if (!pos[j]) continue;
        const double image[3] = { pos[j][0], pos[j][1], -pos[j][2] };
        r[j][0] = distance(pos[j], at);
        r[j][1] = distance(image, at);
        if (r[j][0] < nearest) nearest = r[j][0];
    }
    memset(l, 0, sizeof *l);
    for (int j = 0; j < RB_LISTEN_SOURCES; j++) {
        if (!pos[j]) continue;
        for (int k = 0; k < 2; k++) {
            const double g = (k == 0) ? 1.0 : ground;
            /* A source closer than 10 cm is heard as if from 10 cm: 1 / r
             * holds only away from the pipe's end. */
            const double rr = (r[j][k] > 0.1) ? r[j][k] : 0.1;
            double delay = (r[j][k] - nearest) / (c * pipe_dt);
            if (delay > RB_LISTEN_HISTORY - 2) delay = RB_LISTEN_HISTORY - 2;   /* 14 m on */
            l->gain[j][k] = g / rr;
            l->delay[j][k] = delay;
        }
    }
}

int rb_listener_place(rb_listener *l, int where, int tailpipes, double c, double pipe_dt)
{
    if (where < 0 || where >= RB_HEAR_COUNT) return -1;
    const double *pos[RB_LISTEN_SOURCES] = { TAILPIPE[0], (tailpipes > 1) ? TAILPIPE[1] : NULL, MOUTH };
    rb_listener_set(l, pos, PLACE[where], RB_GROUND, c, pipe_dt);
    return 0;
}

double rb_listener_distance(const rb_listener *l, int j)
{
    if (j < 0 || j >= RB_LISTEN_SOURCES || !(l->gain[j][0] > 0.0)) return 0.0;
    return 1.0 / l->gain[j][0];
}

/* What arrives now: each path's source as it was `delay` pipe steps ago, read
 * between the two steps it falls between. */
static double arriving(const rb_listener *l)
{
    double sum = 0.0;
    for (int j = 0; j < RB_LISTEN_SOURCES; j++) {
        const double *q = l->flow[j];
        for (int k = 0; k < 2; k++) {
            if (l->gain[j][k] == 0.0) continue;
            const int whole = (int)l->delay[j][k];
            const double part = l->delay[j][k] - whole;
            const int a = (l->pos - whole + RB_LISTEN_HISTORY) % RB_LISTEN_HISTORY;
            const int b = (a - 1 + RB_LISTEN_HISTORY) % RB_LISTEN_HISTORY;
            sum += l->gain[j][k] * ((1.0 - part) * q[a] + part * q[b]);
        }
    }
    return sum;
}

void rb_listener_prime(rb_listener *l, const double q[RB_LISTEN_SOURCES])
{
    for (int j = 0; j < RB_LISTEN_SOURCES; j++)
        for (int n = 0; n < RB_LISTEN_HISTORY; n++) l->flow[j][n] = q[j];
    l->sum = l->heard = arriving(l);
}

void rb_listener_push(rb_listener *l, const double q[RB_LISTEN_SOURCES])
{
    l->pos = (l->pos + 1) % RB_LISTEN_HISTORY;
    for (int j = 0; j < RB_LISTEN_SOURCES; j++) l->flow[j][l->pos] = q[j];
    l->sum = arriving(l);
}

double rb_listener_hear(rb_listener *l, double rho0, double dt)
{
    /* A monopole each: p = rho0 / (4 pi) d/dt (Q / r), summed over the paths.
     * The sum's change over the step is its derivative: where it changes only
     * once a pipe step, each change is an impulse, which the decimator's
     * filter turns back into the smooth derivative. */
    const double change = l->sum - l->heard;
    l->heard = l->sum;
    return rho0 / (4.0 * PI) * change / dt;
}

/* ---- the decimator ---- */

/* The modified Bessel function I0, from its power series: the terms are
 * ((x/2)^k / k!)^2. */
static double bessel_i0(double x)
{
    const double q = 0.25 * x * x;
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 200 && term > 1e-17 * sum; k++) {
        term *= q / ((double)k * k);
        sum += term;
    }
    return sum;
}

int rb_decimator_init(rb_decimator *d, int factor)
{
    if (factor < 1 || factor > RB_DECIMATOR_MAX_FACTOR) return -1;
    memset(d, 0, sizeof *d);
    d->factor = factor;
    if (factor == 1) {   /* the physics runs at the audio rate: nothing to filter */
        d->taps = 1;
        d->h[0] = 1.0;
        return 0;
    }

    /* Kaiser's design (Oppenheim and Schafer, chapter 7): for an attenuation
     * of A dB over a transition band dw (rad per physics step) the filter
     * needs (A - 8) / (2.285 dw) + 1 taps, with beta = 0.1102 (A - 8.7). The
     * ideal low-pass cuts at the audio rate's Nyquist, half the frame rate:
     * h[k] = sin(2 pi fc k) / (pi k), fc = 1 / (2 factor) per physics step. */
    const double dw = 2.0 * PI / (6.0 * factor);
    const int half = (int)ceil((AA_ATTENUATION - 8.0) / (2.285 * dw) / 2.0);
    if (2 * half + 1 > RB_DECIMATOR_MAX_TAPS) return -1;
    d->taps = 2 * half + 1;
    const double beta = 0.1102 * (AA_ATTENUATION - 8.7);
    const double fc = 0.5 / factor;
    const double norm = bessel_i0(beta);
    double sum = 0.0;
    for (int n = 0; n < d->taps; n++) {
        const double k = n - half, r = k / half;
        const double ideal = (n == half) ? 2.0 * fc : sin(2.0 * PI * fc * k) / (PI * k);
        d->h[n] = ideal * bessel_i0(beta * sqrt(1.0 - r * r)) / norm;
        sum += d->h[n];
    }
    for (int n = 0; n < d->taps; n++) d->h[n] /= sum;   /* exactly 1 at DC */
    return 0;
}

void rb_decimator_reset(rb_decimator *d)
{
    memset(d->hist, 0, sizeof d->hist);
    d->pos = 0;
}

void rb_decimator_push(rb_decimator *d, double x)
{
    d->hist[d->pos] = d->hist[d->pos + d->taps] = x;
    if (++d->pos == d->taps) d->pos = 0;
}

double rb_decimator_out(const rb_decimator *d)
{
    /* hist[pos .. pos + taps - 1] are the last `taps` inputs, oldest first;
     * the filter is symmetric, so it can run either way along them. */
    const double *x = d->hist + d->pos;
    double y = 0.0;
    for (int n = 0; n < d->taps; n++) y += d->h[n] * x[n];
    return y;
}

/* ---- the output stage ---- */

void rb_dc_blocker_init(rb_dc_blocker *b, double cutoff, double sample_rate)
{
    b->r = exp(-2.0 * PI * cutoff / sample_rate);
    rb_dc_blocker_reset(b);
}

void rb_dc_blocker_reset(rb_dc_blocker *b)
{
    b->x1 = b->y1 = 0.0;
}

double rb_dc_blocker_step(rb_dc_blocker *b, double x)
{
    const double y = x - b->x1 + b->r * b->y1;
    b->x1 = x;
    b->y1 = y;
    return y;
}

double rb_soft_limit(double x)
{
    const double a = fabs(x);
    if (!(a > RB_LIMIT_KNEE)) return x;   /* NaN passes too, for the guard after */
    const double room = 1.0 - RB_LIMIT_KNEE;
    const double y = RB_LIMIT_KNEE + room * tanh((a - RB_LIMIT_KNEE) / room);
    return (x < 0.0) ? -y : y;
}
