/* Geometrical acoustics, aperture radiation and a damped orthogonal FDN.
 * Derivations, approximations and primary sources: docs/stage-S4.md. */
#include "scene.h"
#include "rb.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
#define GEOMETRY_BLOCK 64

static const double SOURCE[4][3] = {
    {0.5, -0.05, 0.35}, {-0.5, -0.05, 0.35}, {-0.3, 3.7, 0.75}, {0, 3.3, 0.6}};

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
    double p[3] = {SOURCE[j][0], SOURCE[j][1], SOURCE[j][2]};
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
    if (j == 1 && s->tailpipes < 2)
        reflection = 0.0;
    if (j == 3 && s->kind != RB_SCENE_CABIN)
        reflection *= 0.3;
    *delay = tau * s->fs;
    *gain = reflection / distance;
    /* Rayleigh disk integral via 8-node Gauss-Chebyshev quadrature. */
    *spread = j < 2 ? 0.5 * s->diameter *
                          sqrt(fmax(0.0, 1.0 - r[1] * r[1] / (distance * distance))) / s->c * s->fs
                    : 0.0;
    *shadow = fmax(0.0, fmin(1.0, (ear ? -1.0 : 1.0) * r[0] / distance));
}

static void paths(rb_scene *s)
{
    for (int e = 0; e < 2; e++)
        for (int j = 0; j < 4; j++)
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
    if (!s || kind < 0 || kind >= RB_SCENE_COUNT || !(fs >= 8000 && fs <= 96000) ||
        !(c >= 250 && c <= 450) || tailpipes < 1 || tailpipes > 2 ||
        !(diameter > 0 && diameter <= 0.3))
        return -1;
    memset(s, 0, sizeof *s);
    s->fs = fs;
    s->c = c;
    s->kind = kind;
    s->tailpipes = tailpipes;
    s->diameter = diameter;
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

static double read_source(const rb_scene *s, int j, double delay)
{
    /* Out of the finite history is silence, never a wrapped unrelated event. */
    if (!(delay >= 0 && delay < RB_SCENE_RING - 2))
        return 0;
    int whole = (int)delay;
    double part = delay - whole;
    int a = (s->pos - whole) & (RB_SCENE_RING - 1), b = (a - 1) & (RB_SCENE_RING - 1);
    return (1 - part) * s->source[j][a] + part * s->source[j][b];
}

void rb_scene_step(rb_scene *s, const double source[RB_SCENE_SOURCES], double out[2])
{
    static const double node[8] = {.9396926208,  .7660444431, .5,           .1736481777,
                                   -.1736481777, -.5,         -.7660444431, -.9396926208};
    static const double weight[8] = {.02599506188, .09181686915, .16666666667, .21552140231,
                                     .21552140231, .16666666667, .09181686915, .02599506188};
    if (s->kind == RB_SCENE_PASSBY && s->geometry_tick == 0)
        paths(s);
    s->pos = (s->pos + 1) & (RB_SCENE_RING - 1);
    for (int j = 0; j < 4; j++)
        s->source[j][s->pos] = (float)(isfinite(source[j]) ? source[j] : 0.0);
    double wet_in = 0;
    for (int e = 0; e < 2; e++) {
        double rear = 0, front = 0;
        for (int j = 0; j < 4; j++)
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
                if (j < 2)
                    rear += v;
                else
                    front += v;
                p->delay += p->delay_step;
                p->gain += p->gain_step;
                p->spread += p->spread_step;
                p->shadow += p->shadow_step;
            }
        out[e] = s->kind == RB_SCENE_CABIN
                     ? rb_cabin_process(&s->cabin[e], rear, front, source[4], 1.0 / s->fs)
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
