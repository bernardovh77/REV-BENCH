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
/* The engine block's middle (stage 12b). */
static const double BAY[3] = { 0.0, 3.3, 0.6 };

/* Where the listener stands: its ears, or the microphone. */
static const double PLACE[RB_HEAR_COUNT][3] = {
    [RB_HEAR_TAILPIPE] = { 0.5, -1.05, 0.5 },   /* 1 m behind tailpipe 0, 15 cm higher */
    [RB_HEAR_BEHIND]   = { 0.6, -3.0, 1.6 },    /* 3 m behind, a little to the right   */
    [RB_HEAR_SIDE]     = { 4.0, 2.2, 1.6 },     /* 4 m to the right of the middle      */
    [RB_HEAR_FRONT]    = { -0.1, 3.4, 1.3 },    /* over the bonnet, near the mouth     */
    [RB_HEAR_CABIN]    = { -0.35, 2.4, 1.1 },   /* the driver's head (left-hand drive) */
};

/* Of the engine bay's sound, what the car's body lets out towards each place:
 * the bonnet shut, most leaves under the car (about -10 dB); over the open
 * bonnet, all of it. In the cabin the body's own filters do it. */
static const double BAY_SHIELD[RB_HEAR_COUNT] = {
    [RB_HEAR_TAILPIPE] = 0.3, [RB_HEAR_BEHIND] = 0.3, [RB_HEAR_SIDE] = 0.4,
    [RB_HEAR_FRONT] = 1.0, [RB_HEAR_CABIN] = 1.0,
};

/* Which sources are in front of the cabin, for its firewall: the flows'
 * mouth (source 2), and the noise's mouth and engine bay (2 and 3). */
#define FLOW_IN_FRONT(j)  ((j) == 2)
#define NOISE_IN_FRONT(j) ((j) >= 2)

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
    l->inside = (where == RB_HEAR_CABIN);
    return 0;
}

int rb_listener_place_noise(rb_listener *l, int where, int tailpipes, double c, double pipe_dt, double dt)
{
    if (where < 0 || where >= RB_HEAR_COUNT || !(dt > 0.0) || !(pipe_dt > 0.0)) return -1;
    const double *at = PLACE[where];
    const double *flows[RB_LISTEN_SOURCES] = { TAILPIPE[0], (tailpipes > 1) ? TAILPIPE[1] : NULL, MOUTH };
    const double *src[RB_LISTEN_NOISES] = { TAILPIPE[0], (tailpipes > 1) ? TAILPIPE[1] : NULL, MOUTH, BAY };

    /* The flows' delays count from their nearest source (rb_listener_place).
     * If one of these is nearer still, everything counts from it. */
    double ref = INFINITY;
    for (int j = 0; j < RB_LISTEN_SOURCES; j++)
        if (flows[j]) ref = fmin(ref, distance(flows[j], at));
    double nearest = ref;
    for (int j = 0; j < RB_LISTEN_NOISES; j++)
        if (src[j]) nearest = fmin(nearest, distance(src[j], at));
    if (nearest < ref) {
        const double later = (ref - nearest) / (c * pipe_dt);
        for (int j = 0; j < RB_LISTEN_SOURCES; j++)
            for (int k = 0; k < 2; k++)
                if (l->gain[j][k] != 0.0) l->delay[j][k] = fmin(l->delay[j][k] + later, RB_LISTEN_HISTORY - 2);
    }

    /* Each one heard straight and off the ground, at (1 m) / r of its
     * pressure 1 m away. */
    for (int j = 0; j < RB_LISTEN_NOISES; j++) {
        for (int k = 0; k < 2; k++) l->ngain[j][k] = l->ndelay[j][k] = 0.0;
        if (!src[j]) continue;
        const double image[3] = { src[j][0], src[j][1], -src[j][2] };
        const double r[2] = { distance(src[j], at), distance(image, at) };
        const double shield = (j == 3) ? BAY_SHIELD[where] : 1.0;
        for (int k = 0; k < 2; k++) {
            const double rr = (r[k] > 0.1) ? r[k] : 0.1;
            l->ngain[j][k] = shield * ((k == 0) ? 1.0 : RB_GROUND) / rr;
            l->ndelay[j][k] = fmin((r[k] - nearest) / (c * dt), RB_LISTEN_RING - 2);
        }
    }
    memset(l->ring, 0, sizeof l->ring);
    l->npos = 0;
    l->mount = 0.0;
    return 0;
}

double rb_listener_distance(const rb_listener *l, int j)
{
    if (j < 0 || j >= RB_LISTEN_SOURCES || !(l->gain[j][0] > 0.0)) return 0.0;
    return 1.0 / l->gain[j][0];
}

/* What arrives now: each path's source as it was `delay` pipe steps ago, read
 * between the two steps it falls between; and the part of it from in front
 * of the cabin. */
static double arriving(const rb_listener *l, double *front)
{
    double sum = 0.0;
    *front = 0.0;
    for (int j = 0; j < RB_LISTEN_SOURCES; j++) {
        const double *q = l->flow[j];
        for (int k = 0; k < 2; k++) {
            if (l->gain[j][k] == 0.0) continue;
            const int whole = (int)l->delay[j][k];
            const double part = l->delay[j][k] - whole;
            const int a = (l->pos - whole + RB_LISTEN_HISTORY) % RB_LISTEN_HISTORY;
            const int b = (a - 1 + RB_LISTEN_HISTORY) % RB_LISTEN_HISTORY;
            const double v = l->gain[j][k] * ((1.0 - part) * q[a] + part * q[b]);
            sum += v;
            if (FLOW_IN_FRONT(j)) *front += v;
        }
    }
    return sum;
}

void rb_listener_prime(rb_listener *l, const double q[RB_LISTEN_SOURCES])
{
    for (int j = 0; j < RB_LISTEN_SOURCES; j++)
        for (int n = 0; n < RB_LISTEN_HISTORY; n++) l->flow[j][n] = q[j];
    l->sum = l->heard = arriving(l, &l->sum_front);
    l->heard_front = l->sum_front;
}

void rb_listener_push(rb_listener *l, const double q[RB_LISTEN_SOURCES])
{
    l->pos = (l->pos + 1) % RB_LISTEN_HISTORY;
    for (int j = 0; j < RB_LISTEN_SOURCES; j++) l->flow[j][l->pos] = q[j];
    l->sum = arriving(l, &l->sum_front);
}

void rb_listener_push_noise(rb_listener *l, const double p[RB_LISTEN_NOISES], double mount)
{
    l->npos = (l->npos + 1) & (RB_LISTEN_RING - 1);
    for (int j = 0; j < RB_LISTEN_NOISES; j++) l->ring[j][l->npos] = (float)p[j];
    l->mount = mount;
}

/* Stage 12b's sources arriving now: each path's pressure as it was `ndelay`
 * physics steps ago; behind the cabin (the return value) and in front. */
static double noise_arriving(const rb_listener *l, double *front)
{
    double rear = 0.0;
    *front = 0.0;
    for (int j = 0; j < RB_LISTEN_NOISES; j++) {
        const float *p = l->ring[j];
        for (int k = 0; k < 2; k++) {
            if (l->ngain[j][k] == 0.0) continue;
            const int whole = (int)l->ndelay[j][k];
            const double part = l->ndelay[j][k] - whole;
            const int a = (l->npos - whole) & (RB_LISTEN_RING - 1);
            const int b = (a - 1) & (RB_LISTEN_RING - 1);
            const double v = l->ngain[j][k] * ((1.0 - part) * p[a] + part * p[b]);
            if (NOISE_IN_FRONT(j)) *front += v;
            else rear += v;
        }
    }
    return rear;
}

/* The cabin's filters for a physics step of dt: the body's low-passes, and
 * the boom, a peaking filter (Bristow-Johnson's "Audio EQ cookbook"). */
static void cabin_design(rb_cabin *cb, double dt)
{
    cb->dt = dt;
    cb->rear_a = 1.0 - exp(-2.0 * PI * RB_CABIN_REAR_HZ * dt);
    cb->front_a = 1.0 - exp(-2.0 * PI * RB_CABIN_FRONT_HZ * dt);
    const double A = pow(10.0, RB_CABIN_BOOM_DB / 40.0);
    const double w0 = 2.0 * PI * RB_CABIN_BOOM_HZ * dt, cw = cos(w0);
    const double alpha = sin(w0) / (2.0 * RB_CABIN_BOOM_Q);
    const double a0 = 1.0 + alpha / A;
    cb->b0 = (1.0 + alpha * A) / a0;
    cb->b1 = -2.0 * cw / a0;
    cb->b2 = (1.0 - alpha * A) / a0;
    cb->a1 = -2.0 * cw / a0;
    cb->a2 = (1.0 - alpha / A) / a0;
}

/* In the cabin: what's outside behind it and in front of it through the
 * body, the mounts' share, and the boom. */
double rb_cabin_process(rb_cabin *cb, double rear, double front, double mount, double dt)
{
    if (cb->dt != dt) cabin_design(cb, dt);
    cb->rear += cb->rear_a * (RB_CABIN_PASS * rear - cb->rear);
    cb->front += cb->front_a * (RB_CABIN_PASS * front - cb->front);
    const double x = cb->rear + cb->front + mount;
    const double y = cb->b0 * x + cb->b1 * cb->x1 + cb->b2 * cb->x2 - cb->a1 * cb->y1 - cb->a2 * cb->y2;
    cb->x2 = cb->x1;
    cb->x1 = x;
    cb->y2 = cb->y1;
    cb->y1 = y;
    return y;
}

double rb_listener_hear(rb_listener *l, double rho0, double dt)
{
    /* A monopole each: p = rho0 / (4 pi) d/dt (Q / r), summed over the paths.
     * The sum's change over the step is its derivative: where it changes only
     * once a pipe step, each change is an impulse, which the decimator's
     * filter turns back into the smooth derivative. */
    const double change = l->sum - l->heard, change_front = l->sum_front - l->heard_front;
    l->heard = l->sum;
    l->heard_front = l->sum_front;
    const double flows = rho0 / (4.0 * PI) * change / dt;

    /* Stage 12b's sources; and in the cabin, everything through its filters. */
    double noise_front;
    const double noise_rear = noise_arriving(l, &noise_front);
    if (!l->inside) return flows + noise_rear + noise_front;
    const double flows_front = rho0 / (4.0 * PI) * change_front / dt;
    return rb_cabin_process(&l->cabin, flows - flows_front + noise_rear,
                            flows_front + noise_front, l->mount, dt);
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
