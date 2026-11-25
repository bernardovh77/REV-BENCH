/* Geometrical acoustics, aperture radiation and a damped orthogonal FDN.
 * Derivations, approximations and primary sources: docs/stage-S4.md. */
#include "scene.h"
#include "rb.h"
#include "fastmath.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
#define GEOMETRY_BLOCK 64

/* Where the sources are (m): x to the car's right, y forward from its rear
 * bumper, z up from the ground. The legacy two tailpipes, the intake's mouth
 * and the engine bay (a network's outlets are where it says: stage 138). */
static const double TAILPIPES[2][3] = { {0.5, -0.05, 0.35}, {-0.5, -0.05, 0.35} };
static const double MOUTH[3] = { -0.3, 3.7, 0.75 }, BAY[3] = { 0, 3.3, 0.6 };
#define S_MOUTH RB_SCENE_OUTLETS
#define S_BAY   (RB_SCENE_OUTLETS + 1)

/* The time (s) sound takes from a source at r (m, relative to the ear) that
 * moves along y at v (m/s) to reach the ear now: the root of
 * |r - v tau e_y| = c tau. */
double rb_scene_flight_time(const double r[3], double v, double c)
{
    const double r2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
    const double rv = r[1] * v, a = c * c - v * v;
    return (sqrt(rv * rv + a * r2) - rv) / a;
}

/* Paths are evaluated at reception time. Source emission position solves
 * |source(t-tau)-ear|=c tau; moving the listener's delay alone is not exact
 * source Doppler. Static scenes need their geometry only once. */
static void geometry(const rb_scene *s, int ear, int j, int k, double time, double *delay,
                     double *gain, double *spread, double *shadow)
{
    double at[3] = {0.6, -3.0, 1.6};
    if (s->kind == RB_SCENE_CABIN) {
        at[0] = -0.35;
        at[1] = 2.4;
        at[2] = 1.1;
    } else if (s->kind == RB_SCENE_ISO) {
        at[0] = 0.5 + 0.5 / sqrt(2.0);
        at[1] = -0.05 - 0.5 / sqrt(2.0);
        at[2] = 0.35;
    } else if (s->kind == RB_SCENE_PASSBY) {
        at[0] = 7.5;
        at[1] = 0.0;
    }
    at[0] += ear ? 0.09 : -0.09;
    double p[3] = {s->at[j][0], s->at[j][1], s->at[j][2]};
    const double velocity = s->kind == RB_SCENE_PASSBY ? 15.0 : 0.0;
    p[1] += velocity * (time - 6.0);
    if (k == 1)
        p[2] = -p[2]; /* image in ground z=0 */
    if (k == 2)
        p[0] = -6.0 - p[0]; /* image in wall x=-3 */
    double r[3] = {p[0] - at[0], p[1] - at[1], p[2] - at[2]};
    const double tau = rb_scene_flight_time(r, velocity, s->c);
    r[1] -= velocity * tau;
    const double distance = fmax(0.1, s->c * tau);
    double reflection = k == 0                     ? 1.0
                        : k == 1                   ? (s->kind == RB_SCENE_GRASS ? 0.35 : 0.9)
                        : s->kind == RB_SCENE_WALL ? 0.7
                                                   : 0.0;
    if (j < RB_SCENE_OUTLETS && j >= s->tailpipes)
        reflection = 0.0;   /* no such outlet */
    if (j == S_BAY && s->kind != RB_SCENE_CABIN)
        reflection *= 0.3;
    *delay = tau * s->fs;
    *gain = reflection / distance;
    /* Rayleigh disk integral via 8-node Gauss-Chebyshev quadrature. */
    *spread = j < RB_SCENE_OUTLETS ? 0.5 * s->diameter[j] *
                          sqrt(fmax(0.0, 1.0 - r[1] * r[1] / (distance * distance))) / s->c * s->fs
                    : 0.0;
    *shadow = fmax(0.0, fmin(1.0, (ear ? -1.0 : 1.0) * r[0] / distance));
}

/* Every path's delay, gain, spread and shadow now, and how much each
 * changes per sample until the next block of GEOMETRY_BLOCK samples (a
 * moving source's paths are worked out again every block, and followed
 * linearly in between). The paths: each ear, each source, and each way
 * there (direct, off the ground, off the wall). */
static void paths(rb_scene *s)
{
    for (int e = 0; e < 2; e++)
        for (int j = 0; j < RB_SCENE_PLACED; j++)
            for (int k = 0; k < 3; k++) {
                rb_scene_path *p = &s->path[e][j][k];
                double d, g, w, h;
                geometry(s, e, j, k, s->time, &p->delay, &p->gain, &p->spread, &p->shadow);
                geometry(s, e, j, k, s->time + GEOMETRY_BLOCK / s->fs, &d, &g, &w, &h);
                p->delay_step = (d - p->delay) / GEOMETRY_BLOCK;
                p->gain_step = (g - p->gain) / GEOMETRY_BLOCK;
                p->spread_step = (w - p->spread) / GEOMETRY_BLOCK;
                p->shadow_step = (h - p->shadow) / GEOMETRY_BLOCK;
            }
}

int rb_scene_init(rb_scene *s, int kind, double fs, double c, int tailpipes, double diameter)
{
    const double each[2] = { diameter, diameter };
    if (tailpipes < 1 || tailpipes > 2)
        return -1;
    return rb_scene_init_at(s, kind, fs, c, &TAILPIPES[0][0], each, tailpipes);
}

int rb_scene_init_at(rb_scene *s, int kind, double fs, double c, const double at[],
                     const double diameter[], int outlets)
{
    if (!s || kind < 0 || kind >= RB_SCENE_COUNT || !(fs >= 8000 && fs <= 96000) ||
        !(c >= 250 && c <= 450) || outlets < 1 || outlets > RB_SCENE_OUTLETS)
        return -1;
    for (int o = 0; o < outlets; o++)
        if (!(diameter[o] > 0 && diameter[o] <= 0.3))
            return -1;
    memset(s, 0, sizeof *s);
    s->fs = fs;
    s->c = c;
    s->kind = kind;
    s->tailpipes = outlets;
    for (int o = 0; o < outlets; o++) {
        memcpy(s->at[o], &at[3 * o], sizeof s->at[o]);
        s->diameter[o] = diameter[o];
    }
    memcpy(s->at[S_MOUTH], MOUTH, sizeof MOUTH);
    memcpy(s->at[S_BAY], BAY, sizeof BAY);
    /* Coprime delays at 48 kHz; orthogonal Householder feedback is lossless
     * before each line's RT60 gain and high-frequency damping. */
    const int lengths[4] = {1493, 1747, 1999, 2381};
    const double rt60 = kind == RB_SCENE_GARAGE ? 1.2 : kind == RB_SCENE_DYNO ? 0.45 : 0.0;
    for (int i = 0; i < 4; i++) {
        s->room_length[i] = (int)lround(lengths[i] * fs / 48000.0);
        s->room_gain[i] = rt60 > 0 ? pow(10.0, -3.0 * s->room_length[i] / (fs * rt60)) : 0;
    }
    s->room_a = 1.0 - exp(-2.0 * PI * 6000.0 / fs);
    s->head_a = 1.0 - exp(-c / (0.09 * fs));
    paths(s);
    return 0;
}

/* Source j as it was `delay` samples ago (fractional), by cubic Lagrange
 * interpolation through the four samples around it (stage 056): flat to
 * 0.54 dB at 10 kHz at the worst fraction, half a sample, where reading
 * between two samples alone lost 2.0 dB (and 1.25 at 8 kHz) and varied as a
 * moving path's fraction did. Within a sample of now, the two samples
 * either side. Out of the finite history is silence, never a wrapped
 * unrelated event. */
static double read_source(const rb_scene *s, int j, double delay)
{
    if (!(delay >= 0 && delay < RB_SCENE_RING - 3))
        return 0;
    const int whole = (int)delay;
    const double p = delay - whole;
    const int a = (s->pos - whole) & (RB_SCENE_RING - 1), b = (a - 1) & (RB_SCENE_RING - 1);
    if (whole < 1)
        return (1 - p) * s->source[j][a] + p * s->source[j][b];
    const int n = (a + 1) & (RB_SCENE_RING - 1), c = (b - 1) & (RB_SCENE_RING - 1);
    /* the samples at delays whole - 1, whole, whole + 1, whole + 2 */
    const double hn = -p * (p - 1.0) * (p - 2.0) / 6.0;
    const double ha = (p + 1.0) * (p - 1.0) * (p - 2.0) / 2.0;
    const double hb = -(p + 1.0) * p * (p - 2.0) / 2.0;
    const double hc = (p + 1.0) * p * (p - 1.0) / 6.0;
    return hn * s->source[j][n] + ha * s->source[j][a] + hb * s->source[j][b] + hc * s->source[j][c];
}

/* One sample: the sources' pressures 1 m away (Pa) in, each ear's out.
 * Each path reads its source's history at its own fractional delay; a
 * tailpipe's opening, seen from the side, spreads it over a range of delays
 * (8 Gauss-Chebyshev nodes across the Rayleigh disk). The far ear hears it
 * through the head's shadow, a one-pole low-pass. In the cabin the car's
 * body filters what's behind and in front (rb_cabin); in the garage and on
 * the dyno a four-line feedback delay network adds the room. */
void rb_scene_step(rb_scene *s, const double source[RB_SCENE_SOURCES], double out[2])
{
    static const double node[8] = {.9396926208,  .7660444431, .5,           .1736481777,
                                   -.1736481777, -.5,         -.7660444431, -.9396926208};
    static const double weight[8] = {.02599506188, .09181686915, .16666666667, .21552140231,
                                     .21552140231, .16666666667, .09181686915, .02599506188};
    if (s->kind == RB_SCENE_PASSBY && s->geometry_tick == 0)
        paths(s);
    s->pos = (s->pos + 1) & (RB_SCENE_RING - 1);
    for (int j = 0; j < RB_SCENE_PLACED; j++)
        s->source[j][s->pos] = (float)(rb_finite(source[j]) ? source[j] : 0.0);
    double wet_in = 0;
    for (int e = 0; e < 2; e++) {
        double rear = 0, front = 0;
        for (int j = 0; j < RB_SCENE_PLACED; j++)
            for (int k = 0; k < 3; k++) {
                rb_scene_path *p = &s->path[e][j][k];
                if (p->gain == 0)
                    continue;
                double v = 0;
                if (p->spread < 0.05)
                    v = read_source(s, j, p->delay);
                else
                    for (int q = 0; q < 8; q++)
                        v += weight[q] * read_source(s, j, p->delay + p->spread * node[q]);
                p->low += s->head_a * (v - p->low);
                v = p->gain * ((1.0 - 0.7 * p->shadow) * v + 0.7 * p->shadow * p->low);
                if (j < RB_SCENE_OUTLETS)
                    rear += v;
                else
                    front += v;
                p->delay += p->delay_step;
                p->gain += p->gain_step;
                p->spread += p->spread_step;
                p->shadow += p->shadow_step;
            }
        out[e] = s->kind == RB_SCENE_CABIN
                     ? rb_cabin_process(&s->cabin[e], rear, front, source[RB_SCENE_MOUNTS], 1.0 / s->fs)
                     : rear + front;
        wet_in += 0.5 * out[e];
    }
    if (s->kind == RB_SCENE_GARAGE || s->kind == RB_SCENE_DYNO) {
        double v[4], sum = 0;
        for (int i = 0; i < 4; i++) {
            v[i] = s->room[i][s->room_pos[i]];
            sum += v[i];
        }
        out[0] += 0.20 * (v[0] + v[1] - v[2] - v[3]);
        out[1] += 0.20 * (v[0] - v[1] + v[2] - v[3]);
        for (int i = 0; i < 4; i++) {
            s->room_low[i] += s->room_a * (v[i] - 0.5 * sum - s->room_low[i]);
            s->room[i][s->room_pos[i]] = (float)(0.5 * wet_in + s->room_gain[i] * s->room_low[i]);
            if (++s->room_pos[i] == s->room_length[i])
                s->room_pos[i] = 0;
        }
    }
    s->time += 1.0 / s->fs;
    s->geometry_tick = (s->geometry_tick + 1) % GEOMETRY_BLOCK;
}
