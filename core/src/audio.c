/*
 * audio.c - the microphone and the chain to the audio rate (see audio.h).
 */
#include "audio.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

/* The anti-alias filter: Kaiser's window, over a transition band a sixth of
 * the audio rate wide (0.417 to 0.583 of it). His formulas are estimates:
 * designed for 101 dB, the filters for factors 2 to 32 reach 100.4-101.4. */
#define AA_ATTENUATION 101.0   /* dB */

/* ---- the microphone ---- */

void rb_mic_init(rb_mic *m, double distance)
{
    m->distance = distance;
    rb_mic_reset(m);
}

void rb_mic_reset(rb_mic *m)
{
    m->q = 0.0;
    m->primed = 0;
}

double rb_mic_hear(rb_mic *m, double q, double rho0, double dt)
{
    /* A monopole: p = rho0 / (4 pi r) dQ/dt. The flow's change over the step
     * is its derivative: where the flow is a staircase (the pipes move their
     * gas every few physics steps), each stair is an impulse, which the
     * decimator's filter turns back into the smooth derivative. */
    const double dq = m->primed ? q - m->q : 0.0;
    m->q = q;
    m->primed = 1;
    return rho0 / (4.0 * PI * m->distance) * dq / dt;
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
