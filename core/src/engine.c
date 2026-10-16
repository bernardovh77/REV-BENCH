/*
 * engine.c - cylinders, valves, throttle and plenum stepped together (see
 * engine.h).
 */
#include "engine.h"

#include <math.h>
#include <string.h>

static const double PI = 3.14159265358979323846;
static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };

void rb_engine_desc_default(rb_engine_desc *d)
{
    memset(d, 0, sizeof *d);
    d->cylinders = 4;
    d->bore = 0.086;
    d->stroke = 0.086;
    d->rod = 0.143;
    d->cr = 10.0;
    /* Firing order 1-3-4-2: cylinder 3 fires 180 degrees after cylinder 1,
     * then 4, then 2. */
    d->firing[0] = 0.0;
    d->firing[1] = 540.0;
    d->firing[2] = 180.0;
    d->firing[3] = 360.0;

    /* Typical modern street cams: 250 degrees (at zero lift), lobe
     * centerlines 110 degrees either side of overlap TDC. The intake opens
     * 15 degrees before overlap TDC and closes 55 after BDC; the exhaust opens
     * 55 before BDC and closes 15 after overlap TDC: 30 degrees of overlap. */
    d->intake = (rb_valve){ 2, 0.033, 0.85 * 0.033, 0.0095, 250.0, 360.0 + 110.0, 0.7 };
    d->exhaust = (rb_valve){ 2, 0.028, 0.85 * 0.028, 0.0090, 250.0, 360.0 - 110.0, 0.7 };

    d->throttle = (rb_throttle){ 0.060, 0.008, 7.0, 0.8, 2e-6 };
    d->plenum_volume = 2.5e-3;
    d->p_ambient = 101325.0;   /* standard sea-level air, 25 C */
    d->T_ambient = 298.15;
}

int rb_engine_desc_check(const rb_engine_desc *d)
{
    rb_cyl_geom g;
    if (!d || d->cylinders < 1 || d->cylinders > RB_MAX_CYLINDERS) return -1;
    if (rb_cyl_geom_init(&g, d->bore, d->stroke, d->rod, d->cr) != 0) return -1;
    for (int i = 0; i < d->cylinders; i++)
        if (!(d->firing[i] >= 0.0 && d->firing[i] < 720.0)) return -1;
    if (rb_valve_check(&d->intake) != 0 || rb_valve_check(&d->exhaust) != 0) return -1;
    /* The valve heads must fit in the cylinder head: at most 80 % of the bore's area. */
    const double heads = d->intake.count * d->intake.diameter * d->intake.diameter
                       + d->exhaust.count * d->exhaust.diameter * d->exhaust.diameter;
    if (!(heads <= 0.8 * d->bore * d->bore)) return -1;
    if (rb_throttle_check(&d->throttle) != 0) return -1;
    if (!(d->plenum_volume > 0.0 && d->plenum_volume < 1.0)) return -1;
    if (!(d->p_ambient > 1e3 && d->p_ambient < 1e7)) return -1;
    if (!(d->T_ambient > 100.0 && d->T_ambient < 1000.0)) return -1;
    return 0;
}

double rb_engine_displacement(const rb_engine *e)
{
    return e->desc.cylinders * e->cyl[0].geom.v_disp;
}

double rb_engine_cylinder_angle(const rb_engine *e, int i)
{
    double a = e->theta - e->desc.firing[i];   /* -720 < a < 720 */
    return a < 0.0 ? a + 720.0 : a;
}

static void start_cycle(rb_engine *e)
{
    for (int i = 0; i < RB_MAX_CYLINDERS; i++) e->acc_air[i] = 0.0;
    e->acc_throttle = e->acc_p = e->acc_rho = 0.0;
    e->acc_pmin = INFINITY;
    e->acc_pmax = -INFINITY;
    e->acc_pump = e->acc_work = e->acc_time = 0.0;
}

int rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt)
{
    if (rb_engine_desc_check(d) != 0 || !(dt > 0.0 && dt < 1e-3)) return -1;

    memset(e, 0, sizeof *e);
    e->desc = *d;
    e->dt = dt;
    e->throttle_pos = 1.0;
    e->throttle_area_pos = NAN;   /* not computed yet */

    rb_cyl_geom g;
    rb_cyl_geom_init(&g, d->bore, d->stroke, d->rod, d->cr);
    rb_chamber_init_reservoir(&e->intake_air, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init_reservoir(&e->exhaust_air, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init(&e->plenum, d->plenum_volume, d->p_ambient, d->T_ambient, AIR);
    rb_orifice_init(&e->throttle, &e->intake_air, &e->plenum, 0.0, 1.0);

    for (int i = 0; i < d->cylinders; i++) {
        const double theta = rb_engine_cylinder_angle(e, i) * PI / 180.0;
        rb_cylinder_init(&e->cyl[i], &g, theta, d->p_ambient, d->T_ambient, AIR);
        /* The areas below are effective areas (Cd included), so cd = 1. */
        rb_orifice_init(&e->intake[i], &e->plenum, &e->cyl[i].gas, 0.0, 1.0);
        rb_orifice_init(&e->exhaust[i], &e->cyl[i].gas, &e->exhaust_air, 0.0, 1.0);
    }
    start_cycle(e);
    return 0;
}

/* The upstream enthalpy (J/kg) of an orifice's last flow: from a if the flow
 * went a -> b, otherwise from b. Valid until the chambers are integrated. */
static double upstream_h(const rb_orifice *o)
{
    return (o->mdot >= 0.0) ? o->a->h : o->b->h;
}

static void finish_cycle(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    rb_engine_cycle *r = &e->result;
    const double v_cyl = e->cyl[0].geom.v_disp;
    const double rho_ambient = d->p_ambient / (e->intake_air.gas.R * d->T_ambient);
    const double rho_plenum = e->acc_rho / e->acc_time;
    double air = 0.0;

    for (int i = 0; i < d->cylinders; i++) {
        r->ve_cyl[i] = e->acc_air[i] / (rho_ambient * v_cyl);
        air += e->acc_air[i];
    }
    r->ve = air / (rho_ambient * v_cyl * d->cylinders);
    r->ve_plenum = air / (rho_plenum * v_cyl * d->cylinders);
    r->map = e->acc_p / e->acc_time;
    r->map_min = e->acc_pmin;
    r->map_max = e->acc_pmax;
    r->air_flow = e->acc_throttle / e->acc_time;
    r->pumping_work = e->acc_pump;
    r->pmep = e->acc_pump / (v_cyl * d->cylinders);
    r->net_work = -e->acc_work;

    e->cycles++;
    start_cycle(e);
}

/* What one step adds to the cycle's totals. */
typedef struct step_share {
    double air[RB_MAX_CYLINDERS], throttle, p, rho, pump, work, dt;
} step_share;

static void add_share(rb_engine *e, const step_share *s, double f, int n)
{
    for (int i = 0; i < n; i++) e->acc_air[i] += f * s->air[i];
    e->acc_throttle += f * s->throttle;
    e->acc_p += f * s->p * s->dt;
    e->acc_rho += f * s->rho * s->dt;
    e->acc_pump += f * s->pump;
    e->acc_work += f * s->work;
    e->acc_time += f * s->dt;
    if (s->p < e->acc_pmin) e->acc_pmin = s->p;
    if (s->p > e->acc_pmax) e->acc_pmax = s->p;
}

void rb_engine_step(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const int n = d->cylinders;
    const double dt = e->dt;
    step_share share = { .p = e->plenum.p, .rho = e->plenum.rho, .dt = dt };

    /* 1. Opening areas. The throttle's only changes with the pedal. */
    if (!(e->throttle_pos == e->throttle_area_pos)) {
        e->throttle.area = rb_throttle_area(&d->throttle, e->throttle_pos);
        e->throttle_area_pos = e->throttle_pos;
    }
    for (int i = 0; i < n; i++) {
        const double angle = rb_engine_cylinder_angle(e, i);
        e->intake[i].area = rb_valve_area(&d->intake, rb_valve_lift(&d->intake, angle));
        e->exhaust[i].area = rb_valve_area(&d->exhaust, rb_valve_lift(&d->exhaust, angle));
    }

    /* 2. Gas flows, all computed from the current state, then integrated.
     * What crosses the boundary is counted before the chambers change. */
    rb_chamber_clear_rates(&e->plenum);
    for (int i = 0; i < n; i++) rb_chamber_clear_rates(&e->cyl[i].gas);

    rb_orifice_flow(&e->throttle, dt);
    e->mass_in += e->throttle.mdot * dt;
    e->energy_in += e->throttle.mdot * upstream_h(&e->throttle) * dt;
    share.throttle = e->throttle.mdot * dt;

    for (int i = 0; i < n; i++) {
        rb_orifice_flow(&e->intake[i], dt);
        rb_orifice_flow(&e->exhaust[i], dt);
        share.air[i] = e->intake[i].mdot * dt;
        e->mass_out += e->exhaust[i].mdot * dt;
        e->energy_out += e->exhaust[i].mdot * upstream_h(&e->exhaust[i]) * dt;
    }

    rb_chamber_integrate(&e->plenum, dt);
    for (int i = 0; i < n; i++) rb_chamber_integrate(&e->cyl[i].gas, dt);

    /* 3. Turn the crank and move the pistons. A step's work counts as pumping
     * if the cylinder is between BDC (180) and BDC (540), on its exhaust and
     * intake strokes. */
    const double rpm = (e->rpm > 0.0) ? e->rpm : 0.0;   /* NaN too */
    const double step_deg = 6.0 * rpm * dt;
    for (int i = 0; i < n; i++) {
        const double angle = rb_engine_cylinder_angle(e, i);
        const double w = rb_cylinder_move(&e->cyl[i], (angle + step_deg) * PI / 180.0);
        e->work_on_gas += w;
        share.work += w;
        if (angle >= 180.0 && angle < 540.0) share.pump += w;
    }

    /* 4. Add the step to the cycle; a step across the end of the cycle is
     * shared in proportion to its angle on each side. */
    const double theta = e->theta + step_deg;
    if (theta < 720.0) {
        add_share(e, &share, 1.0, n);
        e->theta = theta;
    } else {
        const double f = (720.0 - e->theta) / step_deg;
        add_share(e, &share, f, n);
        finish_cycle(e);
        add_share(e, &share, 1.0 - f, n);
        e->theta = theta - 720.0;
    }
}

int rb_engine_settle(rb_engine *e, int max_cycles, double tol)
{
    double ve_prev = -1.0, map_prev = -1.0;
    int calm = 0;

    if (!(e->rpm > 0.0)) return -1;
    for (int k = 1; k <= max_cycles; k++) {
        const long c = e->cycles;
        while (e->cycles == c) rb_engine_step(e);
        const rb_engine_cycle *r = &e->result;
        if (fabs(r->ve - ve_prev) <= tol * r->ve && fabs(r->map - map_prev) <= tol * r->map) {
            if (++calm >= 3) return k;
        } else {
            calm = 0;
        }
        ve_prev = r->ve;
        map_prev = r->map;
    }
    return -1;
}
