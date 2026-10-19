/*
 * engine.c - cylinders, valves, throttle, plenum, fuel, spark and the crank
 * stepped together (see engine.h).
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

/* Below this crank speed the gas sets its own pace: the flame burns at least
 * as fast, and Woschni's gas velocity stays at least as high, as at this speed.
 * It matters only while starting and stopping: a crank that stalls with a
 * burn under way still finishes it, and the hot gas still cools. */
#define RB_GAS_PACE_RPM 100.0

void rb_engine_desc_default(rb_engine_desc *d)
{
    memset(d, 0, sizeof *d);
    d->cylinders = 4;
    d->bore = 0.086;
    d->stroke = 0.086;
    d->rod = 0.143;
    d->cr = 10.0;
    /* Inline, a flat crank (pins 1 and 4 up, 2 and 3 down), firing order
     * 1-3-4-2: cylinder 3 fires 180 degrees after cylinder 1, then 4, then 2.
     * The cylinders sit 96 mm apart. */
    d->banks = 1;
    static const double pins[4] = { 0.0, 180.0, 180.0, 0.0 };
    static const int order[4] = { 0, 2, 3, 1 };
    for (int i = 0; i < 4; i++) {
        d->pin[i] = pins[i];
        d->firing_order[i] = order[i];
        d->position[i] = 0.096 * i;
    }
    rb_engine_layout(d);

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

    /* The crank: an estimate for a 2.0 L car engine with its flywheel (about
     * 0.09 kg m^2), clutch (0.04) and crankshaft with pulley and the rods'
     * rotating ends (0.03). */
    d->inertia = 0.16;
    /* Friction in Chen and Flynn's form with commonly used coefficients
     * (0.4 bar + 0.005 p_max + 0.09 bar per m/s + 0.0009 bar per (m/s)^2):
     * about 0.7 bar at idle, 1.5 bar at 3000 rpm and 2.5 bar at 6000, wide open. */
    d->fmep_constant = 0.4e5;
    d->fmep_pressure = 0.005;
    d->fmep_speed = 0.09e5;
    d->fmep_speed2 = 0.0009e5;
    /* A 1.3 kW starter, geared about 50:1 to the crank: 200 N m when stalled,
     * enough to push a cylinder through its compression from rest, falling to
     * nothing at 250 rpm. It cranks the engine at about 240 rpm. */
    d->starter_torque = 200.0;
    d->starter_free_rpm = 250.0;
    /* The engine computer: idle at 800 rpm with an idle air valve of up to
     * 40 mm^2 (about a 7 mm hole), the spark cut at 6800 rpm until 6700, and
     * a late spark while cranking so the first firings don't kick back. The
     * idle gains are tuned for this engine's valve, flywheel and idle speed:
     * on another engine they may make the idle hunt or wander. */
    d->idle_rpm = 800.0;
    d->idle_valve_area = 40e-6;
    d->idle_gain_p = 3.75e-4;
    d->idle_gain_i = 1.0e-3;
    d->rev_limit = 6800.0;
    d->rev_limit_resume = 6700.0;
    d->cranking_rpm = 400.0;
    d->cranking_spark = 5.0;
}

/* Crank angle (0..360) where cylinder i is at a TDC, relative to cylinder 1. */
static double tdc_angle(const rb_engine_desc *d, int i)
{
    const double a = (d->bank_angle[d->bank[i]] - d->pin[i])
                   - (d->bank_angle[d->bank[0]] - d->pin[0]);
    const double m = fmod(a, 360.0);
    return (m < 0.0) ? m + 360.0 : m;
}

int rb_engine_layout(rb_engine_desc *d)
{
    if (!d || d->cylinders < 1 || d->cylinders > RB_MAX_CYLINDERS) return -1;
    if (d->banks != 1 && d->banks != 2) return -1;
    for (int b = 0; b < d->banks; b++)
        if (!(d->bank_angle[b] >= -180.0 && d->bank_angle[b] <= 180.0)) return -1;
    int seen[RB_MAX_CYLINDERS] = { 0 };
    for (int k = 0; k < d->cylinders; k++) {
        const int i = d->firing_order[k];
        if (i < 0 || i >= d->cylinders || seen[i]++) return -1;
        if (d->bank[i] < 0 || d->bank[i] >= d->banks) return -1;
        if (!(d->pin[i] >= -720.0 && d->pin[i] <= 720.0)) return -1;
        if (!(fabs(d->position[i]) < 10.0)) return -1;
    }
    if (d->firing_order[0] != 0) return -1;

    double firing[RB_MAX_CYLINDERS] = { 0.0 }, last = 0.0;
    for (int k = 1; k < d->cylinders; k++) {
        const int i = d->firing_order[k];
        double a = tdc_angle(d, i);                /* its first TDC after the last firing */
        while (!(a > last + 1e-9)) a += 360.0;
        if (!(a < 720.0 - 1e-9)) return -1;        /* doesn't fit in the cycle */
        firing[i] = last = a;
    }
    for (int i = 0; i < d->cylinders; i++) d->firing[i] = firing[i];
    return 0;
}

int rb_engine_layout_inline(rb_engine_desc *d, int n)
{
    if (!d || n < 1 || n > RB_MAX_CYLINDERS) return -1;
    rb_engine_desc x = *d;
    x.cylinders = n;
    x.banks = 1;
    x.bank_angle[0] = x.bank_angle[1] = 0.0;
    for (int i = 0; i < n; i++) {
        double tdc = fmod(i * 720.0 / n, 360.0);   /* fires every 720/n degrees */
        x.bank[i] = 0;
        x.pin[i] = (tdc > 0.0) ? 360.0 - tdc : 0.0;
        x.firing_order[i] = i;
        x.position[i] = i * (x.bore + 0.010);
    }
    if (rb_engine_layout(&x) != 0) return -1;
    *d = x;
    return 0;
}

int rb_engine_desc_check(const rb_engine_desc *d)
{
    rb_cyl_geom g;
    rb_engine_desc laid;
    if (!d || d->cylinders < 1 || d->cylinders > RB_MAX_CYLINDERS) return -1;
    if (rb_cyl_geom_init(&g, d->bore, d->stroke, d->rod, d->cr) != 0) return -1;
    laid = *d;
    if (rb_engine_layout(&laid) != 0) return -1;
    for (int i = 0; i < d->cylinders; i++)
        if (!(fabs(d->firing[i] - laid.firing[i]) < 1e-9)) return -1;   /* NaN fails too */
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
    if (!(d->inertia > 1e-4 && d->inertia < 100.0)) return -1;
    if (!(d->fmep_constant >= 0.0 && d->fmep_constant < 1e6)) return -1;
    if (!(d->fmep_pressure >= 0.0 && d->fmep_pressure < 0.1)) return -1;
    if (!(d->fmep_speed >= 0.0 && d->fmep_speed < 1e6)) return -1;
    if (!(d->fmep_speed2 >= 0.0 && d->fmep_speed2 < 1e5)) return -1;
    if (!(d->starter_torque >= 0.0 && d->starter_torque < 1e4)) return -1;
    if (!(d->starter_free_rpm > 0.0 && d->starter_free_rpm < 1e4)) return -1;
    if (!(d->idle_rpm >= 0.0 && d->idle_rpm < 1e4)) return -1;
    if (!(d->idle_valve_area >= 0.0 && d->idle_valve_area < 0.01)) return -1;
    if (!(d->idle_gain_p >= 0.0 && d->idle_gain_p < 0.1)) return -1;
    if (!(d->idle_gain_i >= 0.0 && d->idle_gain_i < 0.1)) return -1;
    if (!(d->rev_limit_resume > 0.0 && d->rev_limit > d->rev_limit_resume && d->rev_limit < 1e5)) return -1;
    if (!(d->cranking_rpm >= 0.0 && d->cranking_rpm < 1e4)) return -1;
    if (!(d->cranking_spark >= -90.0 && d->cranking_spark <= 90.0)) return -1;
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
    e->acc_friction = 0.0;
}

int rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt)
{
    if (rb_engine_desc_check(d) != 0 || !(dt > 0.0 && dt < 1e-3)) return -1;

    memset(e, 0, sizeof *e);
    e->desc = *d;
    e->dt = dt;
    e->hold_speed = 1;
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
    rb_orifice_init(&e->idle_air, &e->intake_air, &e->plenum, 0.0, 1.0);

    for (int i = 0; i < d->cylinders; i++) {
        const double theta = rb_engine_cylinder_angle(e, i) * PI / 180.0;
        rb_cylinder_init(&e->cyl[i], &g, theta, d->p_ambient, d->T_ambient, AIR);
        /* The areas below are effective areas (Cd included), so cd = 1. */
        rb_orifice_init(&e->intake[i], &e->plenum, &e->cyl[i].gas, 0.0, 1.0);
        rb_orifice_init(&e->exhaust[i], &e->cyl[i].gas, &e->manifold, 0.0, 1.0);
        e->burn[i].spark_angle = 720.0 - RB_SPARK_DEFAULT;
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
    r->rpm = 120.0 / e->acc_time;               /* two turns per cycle */
    r->fmep = e->acc_friction / v_all;
    r->bmep = r->imep_net - r->fmep;
    r->brake_torque = (r->net_work - e->acc_friction) / (4.0 * PI);
    r->brake_power = (r->net_work - e->acc_friction) / e->acc_time;

    e->cycles++;
    start_cycle(e);
}

/* What one step adds to the cycle's totals. */
typedef struct step_share {
    double air[RB_MAX_CYLINDERS], throttle, p, rho, pump, work, heat, wall, exh_m, exh_mT, pexh, dt;
    double friction;
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
    e->acc_friction += f * s->friction;
    e->acc_exh_m += f * s->exh_m;
    e->acc_exh_mT += f * s->exh_mT;
    e->acc_pexh += f * s->pexh * s->dt;
    e->acc_time += f * s->dt;
    if (s->p < e->acc_pmin) e->acc_pmin = s->p;
    if (s->p > e->acc_pmax) e->acc_pmax = s->p;
}

/* The spark advance to use (degrees before firing TDC): the setting, kept
 * within 90 degrees of TDC so the spark always falls between the valves'
 * events (NaN counts as 0). */
static double spark_advance(double s)
{
    if (s > 90.0) return 90.0;
    if (s < -90.0) return -90.0;
    return (s == s) ? s : 0.0;
}

/* Intake valve closing: the charge is trapped. Inject the fuel for the
 * trapped air, as vapour at the charge's temperature, then note the trapped
 * charge, fuel and all: Woschni's reference for the motored pressure, and the
 * share of it that is old exhaust (the residual fraction, as Heywood counts it).
 * The spark timing for this cycle is fixed here too, as an engine computer
 * would: a change of setting can't move a burn already under way, or fire a
 * cylinder twice. */
static void close_intake(rb_engine *e, int i)
{
    rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    const double fuel = (e->ignition && e->afr > 0.0 && !e->fuel_cut)
                      ? g->m[RB_AIR] / e->afr - g->m[RB_FUEL] : 0.0;
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
    const int cranking = !e->hold_speed && e->omega * 30.0 / PI < e->desc.cranking_rpm;
    b->spark_angle = cycle_angle(720.0 - spark_advance(cranking ? e->desc.cranking_spark : e->spark));
    b->spark_due = 1;
}

/* The spark: what can burn is the fuel, or as much of it as the air can burn,
 * times the combustion efficiency. */
static void spark(rb_engine *e, int i)
{
    const rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    b->burning = 0;
    if (!e->ignition || e->spark_cut) return;   /* the rev limiter cuts the spark */
    b->burnable = rb_burnable_fuel(g->m[RB_FUEL], g->m[RB_AIR], e->desc.mixture_spread,
                                   e->desc.combustion_efficiency);
    b->fraction = 0.0;
    b->burning = (b->burnable > 0.0);
    b->after_spark = b->burning;
    b->burn_angle = 0.0;
}

/* The fuel burned during this step (kg), turned into burned gas and heat. The
 * burn's clock is the crank angle since the spark (in the spark's own step,
 * only the part after it), but it never runs slower than at RB_GAS_PACE_RPM. */
static void burn_step(rb_engine *e, int i, double angle, double step, double q_fuel)
{
    rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    const rb_engine_desc *d = &e->desc;
    const double pace = 6.0 * RB_GAS_PACE_RPM * e->dt;   /* degrees per step */
    if (b->burn_angle == 0.0) b->burn_angle = cycle_angle(angle + step - b->spark_angle);
    else b->burn_angle += (step > pace) ? step : pace;
    const double x = rb_wiebe(b->burn_angle, d->burn_duration, d->wiebe_a, d->wiebe_m);
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

double rb_engine_fmep(const rb_engine *e, double omega)
{
    const rb_engine_desc *d = &e->desc;
    double p_max = 0.0;
    for (int i = 0; i < d->cylinders; i++) p_max += e->burn[i].last_p_max;
    p_max /= d->cylinders;
    const double sp = d->stroke * fabs(omega) / PI;    /* mean piston speed: 2 stroke rev/s */
    return d->fmep_constant + d->fmep_pressure * p_max + d->fmep_speed * sp
         + d->fmep_speed2 * sp * sp;
}

/* Starter motor torque at the crank (N m): a DC motor's straight line from
 * its stall torque down to nothing at its free speed. Above that, its one-way
 * clutch lets the engine run away from it. */
static double starter_torque(const rb_engine *e)
{
    const double free_w = e->desc.starter_free_rpm * PI / 30.0;
    if (!e->starter || !(e->omega < free_w)) return 0.0;
    return e->desc.starter_torque * (1.0 - e->omega / free_w);
}

/* The gas's torque on the crank right now (N m): each piston's net force
 * (cylinder pressure minus the crankcase's, which is at ambient) times how
 * fast its cylinder grows with the crank angle. Only for predicting the next
 * step: the energy books use the work each step actually does. */
static double gas_torque_now(const rb_engine *e)
{
    double t = 0.0;
    for (int i = 0; i < e->desc.cylinders; i++) {
        const rb_cylinder *c = &e->cyl[i];
        t += (c->gas.p - e->desc.p_ambient) * rb_cyl_dvolume(&c->geom, c->theta);
    }
    return t;
}

/* How far a free crank turns in this step (rad), from its speed and the
 * torques on it now: omega dt + alpha dt^2 / 2. Friction and a load only
 * resist motion, so a crank at rest stays there until the gas and the starter
 * overcome them; it never turns backwards. */
static double crank_turn(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const double w = e->omega, dt = e->dt;
    const double load = (e->load_torque == e->load_torque) ? e->load_torque : 0.0;   /* NaN: none */
    e->gas_torque = gas_torque_now(e);
    e->starter_torque_now = starter_torque(e);
    e->friction_torque = rb_engine_fmep(e, w) * rb_engine_displacement(e) / (4.0 * PI);
    e->load_torque_now = load;
    const double drive = e->gas_torque + e->starter_torque_now - (load < 0.0 ? load : 0.0);
    const double resist = e->friction_torque + (load > 0.0 ? load : 0.0);

    const double alpha = (drive - resist) / d->inertia;
    if (alpha < 0.0 && w + alpha * dt < 0.0) return w * w / (-2.0 * alpha);   /* stops, or stays */
    const double turn = w * dt + 0.5 * alpha * dt * dt;
    return (turn > 0.0) ? turn : 0.0;
}

/* The engine computer (free crank only, ignition on), every step: the rev
 * limiter, the fuel cut on overrun, and the idle air valve holding the idle
 * speed. They look at the crank speed smoothed over a few firing pulses.
 * Coasting with the foot off the pedal more than 700 rpm above idle, the
 * injectors stop until the engine is back within 400 rpm of idle: no fuel is
 * wasted, and the engine slows down faster. The idle valve opens in
 * proportion to how far the engine is below its idle speed (idle_gain_p),
 * around a position it learns while the engine idles (idle_gain_i): running,
 * foot off the pedal, and its speed steady (the speed smoothed over 0.05 s
 * within 50 rpm of the speed smoothed over 0.5 s). The learning is slow and
 * counts at most 100 rpm of error, so coasting down from high rpm doesn't
 * unlearn the idle position. */
#define IDLE_VALVE_BASE 0.15      /* opening to start from                     */
#define IDLE_LEARN_MAX  100.0     /* rpm: most error the learning counts       */
#define IDLE_STEADY     50.0      /* rpm: how steady the speed must be to learn */
#define RPM_SMOOTHING   0.05      /* s: the speed's smoothing ...              */
#define RPM_TREND       0.5       /* s: ... and a slower one, for its trend    */
#define FUEL_CUT_ABOVE  700.0     /* rpm above idle: fuel cut on overrun ...   */
#define FUEL_BACK_ABOVE 400.0     /* ... until the engine is back below this   */

static void engine_computer(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const double rpm = e->omega * 30.0 / PI;
    e->rpm_smooth += (rpm - e->rpm_smooth) * (e->dt / RPM_SMOOTHING);
    e->rpm_trend += (e->rpm_smooth - e->rpm_trend) * (e->dt / RPM_TREND);
    if (!e->ignition) {            /* switched off: the valve stays where it is */
        e->spark_cut = e->fuel_cut = 0;
        return;
    }

    if (rpm > d->rev_limit) e->spark_cut = 1;
    else if (rpm < d->rev_limit_resume) e->spark_cut = 0;

    if (!(d->idle_rpm > 0.0)) {
        e->idle_valve = 0.0;
        e->fuel_cut = 0;
        return;
    }
    const int foot_off = e->throttle_pos < 0.01;   /* NaN counts as on */
    if (foot_off && e->rpm_smooth > d->idle_rpm + FUEL_CUT_ABOVE) e->fuel_cut = 1;
    else if (!foot_off || e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) e->fuel_cut = 0;

    const double error = d->idle_rpm - e->rpm_smooth;
    if (e->rpm_smooth > d->cranking_rpm && foot_off
        && fabs(e->rpm_smooth - e->rpm_trend) < IDLE_STEADY) {
        const double counted = (error > IDLE_LEARN_MAX) ? IDLE_LEARN_MAX
                             : (error < -IDLE_LEARN_MAX) ? -IDLE_LEARN_MAX : error;
        e->idle_trim += d->idle_gain_i * counted * e->dt;
        if (e->idle_trim < -IDLE_VALVE_BASE) e->idle_trim = -IDLE_VALVE_BASE;
        if (e->idle_trim > 1.0 - IDLE_VALVE_BASE) e->idle_trim = 1.0 - IDLE_VALVE_BASE;
    }
    const double v = IDLE_VALVE_BASE + e->idle_trim + d->idle_gain_p * error;
    e->idle_valve = (v > 0.0) ? ((v < 1.0) ? v : 1.0) : 0.0;
}

/* Books the air that came in through an inlet (throttle or idle valve) in
 * this step; returns its mass (kg). */
static double count_inflow(rb_engine *e, const rb_orifice *o)
{
    const rb_chamber *up = upstream(o);
    const double m = o->mdot * e->dt;
    e->mass_in += m;
    e->energy_in += m * up->h;
    for (int s = 0; s < RB_NSPECIES; s++) e->species_in[s] += m * up->y[s];
    return m;
}

void rb_engine_step(rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const int n = d->cylinders;
    const double dt = e->dt;
    const double q_fuel = rb_fuel_heat();
    const double ivc = rb_valve_closes(&d->intake), evo = rb_valve_opens(&d->exhaust);
    double open[RB_MAX_CYLINDERS];

    /* 0. How far the crank turns: rpm dt when it's held; when it's free, from
     * its speed and the torques on it, after the engine computer's say. */
    const double kinetic0 = e->kinetic;
    double turn;   /* rad */
    if (e->hold_speed) {
        const double rpm = (e->rpm > 0.0) ? e->rpm : 0.0;   /* NaN too */
        e->omega = rpm * PI / 30.0;
        e->kinetic = 0.5 * d->inertia * e->omega * e->omega;
        e->starter_torque_now = e->load_torque_now = 0.0;
        /* The engine computer is off while held: nothing of its stays cut, and
         * its view of the speed is ready for when the crank is let go. */
        e->spark_cut = e->fuel_cut = 0;
        e->rpm_smooth = e->rpm_trend = rpm;
        e->friction_torque = rb_engine_fmep(e, e->omega) * rb_engine_displacement(e) / (4.0 * PI);
        turn = e->omega * dt;
    } else {
        engine_computer(e);
        turn = crank_turn(e);
    }
    const double step_deg = turn * 180.0 / PI;
    const double pace = RB_GAS_PACE_RPM * PI / 30.0;
    const double piston_speed = d->stroke * (e->omega > pace ? e->omega : pace) / PI;

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
        if (b->spark_due && passes(angle, step_deg, b->spark_angle)) {
            b->spark_due = 0;
            spark(e, i);
        }
        if (passes(angle, step_deg, evo)) {
            b->spark_due = 0;
            b->after_spark = 0;
            b->burning = 0;        /* whatever hasn't burned by now leaves unburned */
        }
    }

    /* 2. Opening areas. The throttle's only changes with the pedal. */
    if (!(e->throttle_pos == e->throttle_area_pos)) {
        e->throttle.area = rb_throttle_area(&d->throttle, e->throttle_pos);
        e->throttle_area_pos = e->throttle_pos;
    }
    e->idle_air.area = e->idle_valve * d->idle_valve_area;
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
    rb_orifice_flow(&e->idle_air, dt);
    share.throttle = count_inflow(e, &e->throttle) + count_inflow(e, &e->idle_air);

    rb_orifice_flow(&e->catalyst, dt);
    rb_orifice_flow(&e->tailpipe, dt);
    const rb_chamber *up = upstream(&e->tailpipe);
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
     * intake strokes. Each cylinder's peak pressure is noted on the way. The
     * pistons' undersides push on the crankcase air, at ambient pressure. */
    double gas_work = 0.0, case_work = 0.0;
    for (int i = 0; i < n; i++) {
        const double angle = rb_engine_cylinder_angle(e, i);
        const double v0 = e->cyl[i].gas.V;
        const double w = (turn > 0.0) ? rb_cylinder_move(&e->cyl[i], (angle + step_deg) * PI / 180.0)
                                      : 0.0;
        e->work_on_gas += w;
        share.work += w;
        gas_work -= w;
        case_work += d->p_ambient * (e->cyl[i].gas.V - v0);
        if (angle >= 180.0 && angle < 540.0) share.pump += w;
        rb_burn *b = &e->burn[i];
        if (e->cyl[i].gas.p > b->p_max) {
            b->p_max = e->cyl[i].gas.p;
            b->angle_p_max = after_firing_tdc(cycle_angle(angle + step_deg));
        }
    }

    /* The crank: its kinetic energy changes by exactly the work done on it.
     * Held, the motor or dyno makes up the difference. */
    const double friction_w = e->friction_torque * turn;
    const double starter_w = e->starter_torque_now * turn;
    const double load_w = e->load_torque_now * turn;
    const double crank_in = gas_work - case_work + starter_w - friction_w - load_w;
    e->crank_gas_work += gas_work;
    e->crank_case_work += case_work;
    e->starter_work += starter_w;
    e->friction_work += friction_w;
    e->load_work += load_w;
    share.friction = friction_w;
    if (turn > 0.0) e->gas_torque = (gas_work - case_work) / turn;
    if (e->hold_speed) {
        e->hold_work += (e->kinetic - kinetic0) - crank_in;
    } else {
        double k = e->kinetic + crank_in;
        if (!(k > 0.0)) {                  /* it stopped part-way through the step */
            if (k == k) e->stop_energy -= k;
            k = 0.0;
        }
        e->kinetic = k;
        e->omega = sqrt(2.0 * k / d->inertia);
        e->rpm = e->omega * 30.0 / PI;
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
