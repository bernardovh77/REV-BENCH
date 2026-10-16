/*
 * engine.c - cylinders, valves, throttle, plenum, fuel and spark stepped
 * together (see engine.h).
 */
#include "engine.h"

#include "combustion.h"
#include "heat.h"

#include <math.h>
#include <string.h>

static const double PI = 3.14159265358979323846;
static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
static const double FUEL[RB_NSPECIES] = { [RB_FUEL] = 1.0 };

/* Spark advance at start-up, degrees before firing TDC: about the best for
 * torque with the default engine wide open at 3000 rpm (stage 6). */
#define RB_SPARK_DEFAULT 25.0

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
    /* Headers and collector hold about 2 L, the mufflers and pipes about 10.
     * The two restrictions together act like one 15 cm^2 opening: about
     * 0.15 bar of back pressure at full power. */
    d->exhaust_volume = 2.0e-3;
    d->catalyst_area = 21e-4;
    d->muffler_volume = 10e-3;
    d->tailpipe_area = 21e-4;
    d->p_ambient = 101325.0;   /* standard sea-level air, 25 C */
    d->T_ambient = 298.15;

    /* A typical burn: 60 crank degrees from spark to finish, 10-90 % in about
     * 30. With the mixing spread, 95 % of the fuel burns in a stoichiometric
     * mixture (Heywood, chapters 4 and 9). */
    d->burn_duration = 60.0;
    d->wiebe_a = 5.0;
    d->wiebe_m = 2.0;
    d->mixture_spread = 0.07;
    d->combustion_efficiency = 0.98;
    d->heat_transfer = 1;
    d->wall_temperature = 430.0;   /* liner, head and piston crown, averaged */
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
    if (!(d->exhaust_volume > 0.0 && d->exhaust_volume < 1.0)) return -1;
    if (!(d->muffler_volume > 0.0 && d->muffler_volume < 1.0)) return -1;
    if (!(d->catalyst_area > 0.0 && d->catalyst_area < 0.1)) return -1;
    if (!(d->tailpipe_area > 0.0 && d->tailpipe_area < 0.1)) return -1;
    if (!(d->p_ambient > 1e3 && d->p_ambient < 1e7)) return -1;
    if (!(d->T_ambient > 100.0 && d->T_ambient < 1000.0)) return -1;
    if (!(d->burn_duration > 0.0 && d->burn_duration <= 180.0)) return -1;
    if (!(d->wiebe_a > 0.0 && d->wiebe_a < 20.0) || !(d->wiebe_m >= 0.0 && d->wiebe_m < 10.0)) return -1;
    if (!(d->mixture_spread >= 0.0 && d->mixture_spread <= 1.0)) return -1;
    if (!(d->combustion_efficiency > 0.0 && d->combustion_efficiency <= 1.0)) return -1;
    if (d->heat_transfer != 0 && d->heat_transfer != 1) return -1;
    if (!(d->wall_temperature > 250.0 && d->wall_temperature < 1000.0)) return -1;
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
    e->acc_fuel = e->acc_heat = e->acc_wall = e->acc_exh_m = e->acc_exh_mT = e->acc_pexh = 0.0;
}

int rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt)
{
    if (rb_engine_desc_check(d) != 0 || !(dt > 0.0 && dt < 1e-3)) return -1;

    memset(e, 0, sizeof *e);
    e->desc = *d;
    e->dt = dt;
    e->throttle_pos = 1.0;
    e->ignition = 0;
    e->afr = RB_AFR_STOICH;
    e->spark = RB_SPARK_DEFAULT;
    e->throttle_area_pos = NAN;   /* not computed yet */
    rb_gas_init(&e->fuel_gas, FUEL);

    rb_cyl_geom g;
    rb_cyl_geom_init(&g, d->bore, d->stroke, d->rod, d->cr);
    rb_chamber_init_reservoir(&e->intake_air, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init_reservoir(&e->exhaust_air, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init(&e->plenum, d->plenum_volume, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init(&e->manifold, d->exhaust_volume, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init(&e->muffler, d->muffler_volume, d->p_ambient, d->T_ambient, AIR);
    rb_orifice_init(&e->throttle, &e->intake_air, &e->plenum, 0.0, 1.0);
    rb_orifice_init(&e->catalyst, &e->manifold, &e->muffler, d->catalyst_area, 1.0);
    rb_orifice_init(&e->tailpipe, &e->muffler, &e->exhaust_air, d->tailpipe_area, 1.0);

    for (int i = 0; i < d->cylinders; i++) {
        const double theta = rb_engine_cylinder_angle(e, i) * PI / 180.0;
        rb_cylinder_init(&e->cyl[i], &g, theta, d->p_ambient, d->T_ambient, AIR);
        /* The areas below are effective areas (Cd included), so cd = 1. */
        rb_orifice_init(&e->intake[i], &e->plenum, &e->cyl[i].gas, 0.0, 1.0);
        rb_orifice_init(&e->exhaust[i], &e->cyl[i].gas, &e->manifold, 0.0, 1.0);
    }
    start_cycle(e);
    return 0;
}

/* The upstream chamber of an orifice's last flow: a if the flow went a -> b,
 * otherwise b. Its state is the one the flow carried until the chambers are
 * integrated. */
static const rb_chamber *upstream(const rb_orifice *o)
{
    return (o->mdot >= 0.0) ? o->a : o->b;
}

/* 1 if a crank turning `step` degrees from `angle` passes `event` (all in
 * degrees of the 720-degree cycle). An event exactly at `angle` counts. */
static int passes(double angle, double step, double event)
{
    double ahead = event - angle;
    if (ahead < 0.0) ahead += 720.0;
    return ahead < step;
}

static double cycle_angle(double a)   /* any angle into 0..720 */
{
    a = fmod(a, 720.0);
    return a < 0.0 ? a + 720.0 : a;
}

/* Degrees after firing TDC, -360..360 (negative = before). */
static double after_firing_tdc(double angle)
{
    return angle >= 360.0 ? angle - 720.0 : angle;
}

static void finish_cycle(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    rb_engine_cycle *r = &e->result;
    const double v_cyl = e->cyl[0].geom.v_disp, v_all = v_cyl * d->cylinders;
    const double rho_ambient = d->p_ambient / (e->intake_air.gas.R * d->T_ambient);
    const double rho_plenum = e->acc_rho / e->acc_time;
    double air = 0.0, p_max = 0.0, angle = 0.0, residual = 0.0;

    for (int i = 0; i < d->cylinders; i++) {
        r->ve_cyl[i] = e->acc_air[i] / (rho_ambient * v_cyl);
        air += e->acc_air[i];
        p_max += e->burn[i].last_p_max;
        angle += e->burn[i].last_angle_p_max;
        residual += e->burn[i].residual;
    }
    r->ve = air / (rho_ambient * v_all);
    r->ve_plenum = air / (rho_plenum * v_all);
    r->map = e->acc_p / e->acc_time;
    r->map_min = e->acc_pmin;
    r->map_max = e->acc_pmax;
    r->air_flow = e->acc_throttle / e->acc_time;
    r->pumping_work = e->acc_pump;
    r->pmep = e->acc_pump / v_all;
    r->net_work = -e->acc_work;
    r->imep_gross = -(e->acc_work - e->acc_pump) / v_all;
    r->imep_net = r->net_work / v_all;
    r->torque = r->net_work / (4.0 * PI);
    r->power = r->net_work / e->acc_time;
    r->fuel_flow = e->acc_fuel / e->acc_time;
    r->afr = (e->acc_fuel > 0.0) ? air / e->acc_fuel : 0.0;
    r->efficiency = (e->acc_fuel > 0.0) ? r->net_work / (e->acc_fuel * RB_FUEL_LHV) : 0.0;
    r->heat_released = e->acc_heat;
    r->wall_loss = -e->acc_wall;
    r->t_exhaust = (e->acc_exh_m > 0.0) ? e->acc_exh_mT / e->acc_exh_m : 0.0;
    r->p_exhaust = e->acc_pexh / e->acc_time;
    r->p_max = p_max / d->cylinders;
    r->angle_p_max = angle / d->cylinders;
    r->residual = residual / d->cylinders;

    e->cycles++;
    start_cycle(e);
}

/* What one step adds to the cycle's totals. */
typedef struct step_share {
    double air[RB_MAX_CYLINDERS], throttle, p, rho, pump, work, heat, wall, exh_m, exh_mT, pexh, dt;
} step_share;

static void add_share(rb_engine *e, const step_share *s, double f, int n)
{
    for (int i = 0; i < n; i++) e->acc_air[i] += f * s->air[i];
    e->acc_throttle += f * s->throttle;
    e->acc_p += f * s->p * s->dt;
    e->acc_rho += f * s->rho * s->dt;
    e->acc_pump += f * s->pump;
    e->acc_work += f * s->work;
    e->acc_heat += f * s->heat;
    e->acc_wall += f * s->wall;
    e->acc_exh_m += f * s->exh_m;
    e->acc_exh_mT += f * s->exh_mT;
    e->acc_pexh += f * s->pexh * s->dt;
    e->acc_time += f * s->dt;
    if (s->p < e->acc_pmin) e->acc_pmin = s->p;
    if (s->p > e->acc_pmax) e->acc_pmax = s->p;
}

/* Intake valve closing: the charge is trapped. Inject the fuel for the
 * trapped air, as vapour at the charge's temperature, then note the trapped
 * charge, fuel and all: Woschni's reference for the motored pressure, and the
 * share of it that is old exhaust (the residual fraction, as Heywood counts it). */
static void close_intake(rb_engine *e, int i)
{
    rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    const double fuel = (e->ignition && e->afr > 0.0) ? g->m[RB_AIR] / e->afr - g->m[RB_FUEL] : 0.0;
    if (fuel > 0.0) {
        const double u_fuel = rb_gas_u(&e->fuel_gas, g->T);
        g->m[RB_FUEL] += fuel;
        g->U += fuel * u_fuel;
        rb_chamber_update(g);
        e->fuel_in += fuel;
        e->fuel_energy_in += fuel * u_fuel;
        e->acc_fuel += fuel;
    }
    b->p_ivc = g->p;
    b->V_ivc = g->V;
    b->T_ivc = g->T;
    b->residual = g->y[RB_BURNED];
}

/* The spark: what can burn is the fuel, or as much of it as the air can burn,
 * times the combustion efficiency. */
static void spark(rb_engine *e, int i)
{
    const rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    b->burning = 0;
    if (!e->ignition) return;
    b->burnable = rb_burnable_fuel(g->m[RB_FUEL], g->m[RB_AIR], e->desc.mixture_spread,
                                   e->desc.combustion_efficiency);
    b->fraction = 0.0;
    b->burning = (b->burnable > 0.0);
    b->after_spark = b->burning;
}

/* The fuel burned during this step (kg), turned into burned gas and heat. */
static void burn_step(rb_engine *e, int i, double angle, double step, double q_fuel)
{
    rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    const rb_engine_desc *d = &e->desc;
    const double phi = cycle_angle(angle + step - (720.0 - e->spark));   /* degrees since the spark */
    const double x = rb_wiebe(phi, d->burn_duration, d->wiebe_a, d->wiebe_m);
    double fuel = (x - b->fraction) * b->burnable;
    b->fraction = x;
    if (x >= 1.0) b->burning = 0;

    /* Never more than is there (only possible if a valve is open mid-burn). */
    if (fuel > g->m[RB_FUEL]) fuel = g->m[RB_FUEL];
    if (fuel > g->m[RB_AIR] / RB_AFR_STOICH) fuel = g->m[RB_AIR] / RB_AFR_STOICH;
    if (!(fuel > 0.0)) return;

    const double dt = e->dt;
    g->dm[RB_FUEL] -= fuel / dt;
    g->dm[RB_AIR] -= RB_AFR_STOICH * fuel / dt;
    g->dm[RB_BURNED] += (1.0 + RB_AFR_STOICH) * fuel / dt;
    g->dU += q_fuel * fuel / dt;
    b->heat = q_fuel * fuel;
    e->fuel_burned += fuel;
    e->heat_released += b->heat;
}

void rb_engine_step(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const int n = d->cylinders;
    const double dt = e->dt;
    const double rpm = (e->rpm > 0.0) ? e->rpm : 0.0;   /* NaN too */
    const double step_deg = 6.0 * rpm * dt;
    const double q_fuel = rb_fuel_heat();
    const double piston_speed = 2.0 * d->stroke * rpm / 60.0;
    const double ivc = rb_valve_closes(&d->intake), evo = rb_valve_opens(&d->exhaust);
    const double spark_angle = cycle_angle(720.0 - e->spark);
    double open[RB_MAX_CYLINDERS];

    /* 1. Events the crank passes during this step. */
    for (int i = 0; i < n; i++) {
        const double angle = rb_engine_cylinder_angle(e, i);
        rb_burn *b = &e->burn[i];
        b->heat = b->wall_heat = 0.0;
        if (passes(angle, step_deg, 360.0)) {   /* overlap TDC: the last cycle's peak is known */
            b->last_p_max = b->p_max;
            b->last_angle_p_max = b->angle_p_max;
            b->p_max = 0.0;
        }
        if (passes(angle, step_deg, ivc)) close_intake(e, i);
        if (passes(angle, step_deg, spark_angle)) spark(e, i);
        if (passes(angle, step_deg, evo)) {
            b->after_spark = 0;
            b->burning = 0;        /* whatever hasn't burned by now leaves unburned */
        }
    }

    /* 2. Opening areas. The throttle's only changes with the pedal. */
    if (!(e->throttle_pos == e->throttle_area_pos)) {
        e->throttle.area = rb_throttle_area(&d->throttle, e->throttle_pos);
        e->throttle_area_pos = e->throttle_pos;
    }
    for (int i = 0; i < n; i++) {
        const double angle = rb_engine_cylinder_angle(e, i);
        e->intake[i].area = rb_valve_area(&d->intake, rb_valve_lift(&d->intake, angle));
        e->exhaust[i].area = rb_valve_area(&d->exhaust, rb_valve_lift(&d->exhaust, angle));
        open[i] = (e->intake[i].area > 0.0 || e->exhaust[i].area > 0.0);
    }

    /* 3. Everything that changes the gas, computed from the current state,
     * then integrated together. What crosses the boundary is counted before
     * the chambers change. */
    step_share share = { .p = e->plenum.p, .rho = e->plenum.rho, .pexh = e->manifold.p, .dt = dt };
    rb_chamber_clear_rates(&e->plenum);
    rb_chamber_clear_rates(&e->manifold);
    rb_chamber_clear_rates(&e->muffler);
    for (int i = 0; i < n; i++) rb_chamber_clear_rates(&e->cyl[i].gas);

    rb_orifice_flow(&e->throttle, dt);
    const rb_chamber *up = upstream(&e->throttle);
    e->mass_in += e->throttle.mdot * dt;
    e->energy_in += e->throttle.mdot * up->h * dt;
    for (int s = 0; s < RB_NSPECIES; s++) e->species_in[s] += e->throttle.mdot * up->y[s] * dt;
    share.throttle = e->throttle.mdot * dt;

    rb_orifice_flow(&e->catalyst, dt);
    rb_orifice_flow(&e->tailpipe, dt);
    up = upstream(&e->tailpipe);
    e->mass_out += e->tailpipe.mdot * dt;
    e->energy_out += e->tailpipe.mdot * up->h * dt;
    for (int s = 0; s < RB_NSPECIES; s++) e->species_out[s] += e->tailpipe.mdot * up->y[s] * dt;

    for (int i = 0; i < n; i++) {
        rb_chamber *g = &e->cyl[i].gas;
        rb_burn *b = &e->burn[i];

        rb_orifice_flow(&e->intake[i], dt);
        rb_orifice_flow(&e->exhaust[i], dt);
        share.air[i] = e->intake[i].mdot * dt;
        if (e->exhaust[i].mdot > 0.0) {
            share.exh_m += e->exhaust[i].mdot * dt;
            share.exh_mT += e->exhaust[i].mdot * dt * g->T;
        }

        if (d->heat_transfer && g->mass > 0.0) {
            const double w = rb_woschni_w(piston_speed, (int)open[i], b->after_spark,
                                          e->cyl[i].geom.v_disp, g->p, b->p_ivc, b->V_ivc,
                                          b->T_ivc, g->V);
            const double h = rb_woschni_h(d->bore, g->p, g->T, w);
            const double x = (g->V - e->cyl[i].geom.v_clear) / e->cyl[i].geom.area;
            const double q = h * rb_wall_area(&e->cyl[i].geom, x) * (d->wall_temperature - g->T);
            rb_chamber_add_heat(g, q);
            b->wall_heat = q * dt;
            e->wall_heat += b->wall_heat;
            share.wall += b->wall_heat;
        }
        if (b->burning) {
            burn_step(e, i, rb_engine_cylinder_angle(e, i), step_deg, q_fuel);
            share.heat += b->heat;
        }
    }

    rb_chamber_integrate(&e->plenum, dt);
    rb_chamber_integrate(&e->manifold, dt);
    rb_chamber_integrate(&e->muffler, dt);
    for (int i = 0; i < n; i++) rb_chamber_integrate(&e->cyl[i].gas, dt);

    /* 4. Turn the crank and move the pistons. A step's work counts as pumping
     * if the cylinder is between BDC (180) and BDC (540), on its exhaust and
     * intake strokes. Each cylinder's peak pressure is noted on the way. */
    for (int i = 0; i < n; i++) {
        const double angle = rb_engine_cylinder_angle(e, i);
        const double w = rb_cylinder_move(&e->cyl[i], (angle + step_deg) * PI / 180.0);
        e->work_on_gas += w;
        share.work += w;
        if (angle >= 180.0 && angle < 540.0) share.pump += w;
        rb_burn *b = &e->burn[i];
        if (e->cyl[i].gas.p > b->p_max) {
            b->p_max = e->cyl[i].gas.p;
            b->angle_p_max = after_firing_tdc(cycle_angle(angle + step_deg));
        }
    }

    /* 5. Add the step to the cycle; a step across the end of the cycle is
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
    double ve_prev = -1.0, map_prev = -1.0, work_prev = 1e300;
    int calm = 0;

    if (!(e->rpm >= 10.0)) return -1;
    for (int k = 1; k <= max_cycles; k++) {
        const long c = e->cycles;
        while (e->cycles == c) rb_engine_step(e);
        const rb_engine_cycle *r = &e->result;
        if (fabs(r->ve - ve_prev) <= tol * r->ve && fabs(r->map - map_prev) <= tol * r->map
            && fabs(r->net_work - work_prev) <= tol * (fabs(r->net_work) + 1.0)) {
            if (++calm >= 3) return k;
        } else {
            calm = 0;
        }
        ve_prev = r->ve;
        map_prev = r->map;
        work_prev = r->net_work;
    }
    return -1;
}
