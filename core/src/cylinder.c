/*
 * cylinder.c - slider-crank kinematics and the cylinder (see cylinder.h).
 *
 * Piston position below TDC for crank radius a, rod length l (Heywood):
 *     x(theta) = a (1 - cos theta) + l - sqrt(l^2 - a^2 sin^2 theta)
 * The first term is what an infinitely long rod would give; the second is the
 * extra drop because the rod tilts, which is why the piston is already past
 * half stroke at 90 degrees.
 */
#include "cylinder.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

int rb_cyl_geom_init(rb_cyl_geom *g, double bore, double stroke, double rod, double cr)
{
    /* Written so NaN fails every test. */
    if (!(bore > 0.0 && bore < 10.0) || !(stroke > 0.0 && stroke < 10.0)
        || !(rod > stroke && rod < 100.0) || !(cr > 1.0 && cr < 100.0))
        return -1;

    g->bore    = bore;
    g->stroke  = stroke;
    g->rod     = rod;
    g->cr      = cr;
    g->area    = 0.25 * PI * bore * bore;
    g->crank_r = 0.5 * stroke;
    g->v_disp  = g->area * stroke;
    g->v_clear = g->v_disp / (cr - 1.0);   /* cr = (v_clear + v_disp) / v_clear */
    return 0;
}

double rb_cyl_piston_x(const rb_cyl_geom *g, double theta)
{
    const double a = g->crank_r, l = g->rod;
    const double s = sin(theta), c = cos(theta);
    /* Both terms are small differences near TDC, so they are rewritten to
     * avoid cancellation: 1 - cos = sin^2 / (1 + cos), and
     * l - sqrt(l^2 - q) = q / (l + sqrt(l^2 - q)). */
    const double one_minus_cos = (c > 0.0) ? s * s / (1.0 + c) : 1.0 - c;
    const double q = a * a * s * s;
    return a * one_minus_cos + q / (l + sqrt(l * l - q));
}

double rb_cyl_piston_dx(const rb_cyl_geom *g, double theta)
{
    const double a = g->crank_r, l = g->rod;
    const double s = sin(theta), c = cos(theta);
    return a * s * (1.0 + a * c / sqrt(l * l - a * a * s * s));
}

double rb_cyl_volume(const rb_cyl_geom *g, double theta)
{
    return g->v_clear + g->area * rb_cyl_piston_x(g, theta);
}

double rb_cyl_dvolume(const rb_cyl_geom *g, double theta)
{
    return g->area * rb_cyl_piston_dx(g, theta);
}

int rb_cylinder_init(rb_cylinder *c, const rb_cyl_geom *geom, double theta,
                     double p, double T, const double y[RB_NSPECIES])
{
    rb_cyl_geom g;
    if (!geom || rb_cyl_geom_init(&g, geom->bore, geom->stroke, geom->rod, geom->cr) != 0)
        return -1;
    c->geom  = g;
    c->theta = isfinite(theta) ? theta : 0.0;
    c->work  = 0.0;
    rb_chamber_init(&c->gas, rb_cyl_volume(&g, c->theta), p, T, y);
    return 0;
}

double rb_cylinder_move(rb_cylinder *c, double theta)
{
    if (!isfinite(theta)) {
        c->work = 0.0;
        return 0.0;
    }
    c->work  = rb_chamber_set_volume(&c->gas, rb_cyl_volume(&c->geom, theta));
    c->theta = theta;
    return c->work;
}
