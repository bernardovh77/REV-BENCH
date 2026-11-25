/*
 * engine.c - cylinders, valves, throttle, plenum, fuel, spark and the crank
 * stepped together (see engine.h): the step and the cycle's books. Stage 130
 * moved building the engine to engine_build.c and the engine computer to
 * engine_computer.c; what the three share is engine_parts.h.
 */
#include "engine.h"

#include "combustion.h"
#include "heat.h"
#include "rand.h"
#include "diagnostics.h"
#include "ignition.h"
#include "fastmath.h"
#include "rb.h"   /* RB_MUFFLER_* */

#include <math.h>
#include <string.h>

#include "engine_parts.h"

void rb_engine_start_cycle(rb_engine *e)
{
    for (int i = 0; i < RB_MAX_CYLINDERS; i++) e->acc_air[i] = 0.0;
    e->acc_throttle = e->acc_p = e->acc_rho = 0.0;
    e->acc_pmin = INFINITY;
    e->acc_pmax = -INFINITY;
    e->acc_pump = e->acc_work = e->acc_time = 0.0;
    e->acc_fuel = e->acc_heat = e->acc_wall = e->acc_exh_m = e->acc_exh_mT = e->acc_pexh = 0.0;
    e->acc_friction = e->acc_tail_m = e->acc_tail_mT = 0.0;
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
    if (!(fabs(a) < 720.0)) a = fmod(a, 720.0);   /* inside a cycle fmod returns a itself */
    return a < 0.0 ? a + 720.0 : a;
}

/* Where in a step of `step` degrees from `angle` the crank passes `event`:
 * the share of the step, 0..1. */
static double event_fraction(double angle, double step, double event)
{
    return cycle_angle(event - angle) / step;
}

/* Cylinder i's spark event of this kind (ignition.h), at `fraction` of the
 * step and crank angle `angle`, for the probes' trace: nothing without one. */
static void ignition_event(rb_engine *e, int i, int kind, double fraction, double angle,
                           double fuel)
{
    if (!e->ignition_trace) return;
    const rb_burn *b = &e->burn[i];
    const rb_ignition_event event = {
        .tick = e->physics_tick,
        .charge = b->charge_sequence,
        .cylinder = i,
        .kind = kind,
        .fraction = fraction,
        .angle_deg = angle,
        .kernel_raw_delay_deg = b->kernel_raw_delay_deg,
        .kernel_delay_deg = b->kernel_delay_deg,
        .fuel_kg = fuel,
    };
    rb_ignition_emit(e->ignition_trace, &event);
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
    /* the guards over the cycle (stage 068): a sum over the pipes, once a cycle */
    const uint64_t guards = rb_engine_guards(e);
    r->guard_rate = e->acc_time > 0.0 ? (double)(guards - e->guards_then) / e->acc_time : 0.0;
    e->guards_then = guards;
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
    r->t_tailpipe = (e->acc_tail_m > 0.0) ? e->acc_tail_mT / e->acc_tail_m : 0.0;
    r->p_max = p_max / d->cylinders;
    r->angle_p_max = angle / d->cylinders;
    r->residual = residual / d->cylinders;
    double spark = 0.0, burn = 0.0;
    for (int i = 0; i < d->cylinders; i++) {
        const double a = 720.0 - e->burn[i].spark_angle;
        spark += (a > 360.0) ? a - 720.0 : a;   /* before TDC; after it is negative */
        burn += e->burn[i].duration;
    }
    r->spark = spark / d->cylinders;
    r->burn_duration = burn / d->cylinders;
    r->rpm = 120.0 / e->acc_time;               /* two turns per cycle */
    r->fmep = e->acc_friction / v_all;
    r->bmep = r->imep_net - r->fmep;
    r->brake_torque = (r->net_work - e->acc_friction) / (4.0 * PI);
    r->brake_power = (r->net_work - e->acc_friction) / e->acc_time;

    e->cycles++;
    rb_engine_start_cycle(e);
}

/* What one step adds to the cycle's totals. */
typedef struct step_share {
    double air[RB_MAX_CYLINDERS], throttle, p, rho, pump, work, heat, wall, exh_m, exh_mT, pexh, dt;
    double friction, tail_m, tail_mT;
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
    e->acc_tail_m += f * s->tail_m;
    e->acc_tail_mT += f * s->tail_mT;
    e->acc_pexh += f * s->pexh * s->dt;
    e->acc_time += f * s->dt;
    if (s->p < e->acc_pmin) e->acc_pmin = s->p;
    if (s->p > e->acc_pmax) e->acc_pmax = s->p;
}

/* Intake valve closing: the charge is trapped. Inject the fuel for the
 * trapped air, as vapour at the charge's temperature, then note the trapped
 * charge, fuel and all: Woschni's reference for the motored pressure, and the
 * share of it that is old exhaust (the residual fraction, as Heywood counts it).
 * This cycle's burn length and spark timing are fixed here too, as an engine
 * computer would: a change of setting can't move a burn already under way,
 * or fire a cylinder twice. */
static void close_intake(rb_engine *e, int i, double fraction)
{
    rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    const double v = variation(e);
    b->charge_sequence++;
    b->discharged = b->discharged_this_step = b->kernel_pending = b->kernel_started = 0;
    b->kernel_clock = 0;
    const double retained = g->m[RB_FUEL];
    const double target = (e->afr > 0.0 && rb_finite(e->afr))
                              ? g->m[RB_AIR] / e->afr * (1.0 + v * CYL_FUEL * e->trim_fuel[i])
                              : 0.0;
    int kind = RB_INJ_DELIVERED;
    if (!e->ignition)
        kind = RB_INJ_OFF;
    else if (!(e->afr > 0.0 && rb_finite(e->afr)))
        kind = RB_INJ_AFR_DISABLED;
    else if (e->fuel_cut)
        kind = RB_INJ_DFCO;
    else if (e->desc.limiter_cut == RB_LIMITER_FUEL && rb_computer_limiter_cut(e) >= 0)
        kind = RB_INJ_LIMITER;   /* stage 133: the limiter cuts the fuel */
    else if (!(g->m[RB_AIR] > 0.0))
        kind = RB_INJ_NO_AIR;
    if (!rb_finite(target) || !rb_finite(retained) || retained < 0.0 || !rb_finite(g->T) ||
        !(g->T > 0.0))
        kind = RB_INJ_INVALID;
    if (kind == RB_INJ_LIMITER) e->limiter_cuts++;
    double fuel = kind == RB_INJ_DELIVERED ? target - retained : 0.0, energy = 0.0;
    if (kind == RB_INJ_DELIVERED && !(fuel > 0.0)) kind = RB_INJ_RETAINED;
    if (kind==RB_INJ_DELIVERED) {
        const double u_fuel = rb_gas_u(&e->fuel_gas, g->T);
        g->m[RB_FUEL] += fuel;
        g->U += fuel * u_fuel;
        rb_chamber_update(g);
        e->fuel_in += fuel;
        e->fuel_energy_in += fuel * u_fuel;
        e->acc_fuel += fuel;
        energy = fuel * u_fuel;
    }
    /* This transfer owns its source event. Skips carry zero delivery even
     * when combustible fuel remains in the cylinder. */
    const rb_injection_event event = {
        .tick = e->physics_tick,
        .charge = b->charge_sequence,
        .cylinder = i,
        .kind = kind,
        .fraction = fraction,
        .angle_deg = rb_valve_closes(&e->desc.intake),
        .retained_fuel_kg = retained,
        .target_fuel_kg = target,
        .fuel_kg = kind == RB_INJ_DELIVERED ? fuel : 0.0,
        .sensible_energy_J = energy,
    };
    e->injection[e->injection_count++] = event;
    rb_injection_emit(e->injection_trace, &event);
    b->p_ivc = g->p;
    b->V_ivc = g->V;
    b->T_ivc = g->T;
    b->residual = g->y[RB_BURNED];
    const double phi = (g->m[RB_AIR] > 0.0 && g->m[RB_FUEL] > 0.0)
                     ? RB_AFR_STOICH * (g->m[RB_FUEL] / g->m[RB_AIR]) : 0.0;
    b->phi_ivc = rb_finite(phi) ? phi : 0.0;
    const double rpm = e->hold_speed ? e->rpm : e->omega * 30.0 / PI;
    const double expected = rb_computer_burn_duration(e, b, rpm);
    const int capped = b->combustion_model == RB_BURN_CAPPED;
    const double base = capped ? expected : b->demand_duration;
    const double ceiling = capped ? 180.0 : 720.0;
    const double trim = 1.0 + v * CYL_BURN * e->trim_burn[i];
    b->mean_duration = rb_burn_mean_duration(base, v * VARY_LENGTH, trim, ceiling);
    b->mean_kernel_delay = v * VARY_START * expected * rb_kernel_delay_mean();
    const double requested = rb_computer_spark(e, b->mean_duration, b->mean_kernel_delay, rpm);
    const double advance = rb_computer_spark_advance(requested);
    if (advance != requested) b->flame_flags |= RB_FLAME_SPARK_LIMIT;
    b->expected_ca50 = b->mean_kernel_delay +
                       b->mean_duration * rb_wiebe_quantile(.5, e->desc.wiebe_a, e->desc.wiebe_m) -
                       advance;
    b->spark_angle = cycle_angle(720.0 - advance);

    /* This cycle's own burn (stage 12c): its start and length, and the
     * cylinder's own length. */
    /* Correlate the underlying normal draws, then bound each marginal.
     * Clipping before mixing changes the length distribution and makes the
     * ECU's conditional-mean calculation a different stochastic model. */
    const double start_raw = rb_rand_normal(&e->rand_cycles);
    const double length_raw = VARY_ALIKE * start_raw +
                              sqrt(1.0 - VARY_ALIKE * VARY_ALIKE) * rb_rand_normal(&e->rand_cycles);
    const double start = fmax(-VARY_MOST, fmin(VARY_MOST, start_raw));
    const double length = fmax(-VARY_MOST, fmin(VARY_MOST, length_raw));
    double duration = base * exp(v * VARY_LENGTH * length) * trim;
    if (!(duration >= 0.25 * base)) duration = 0.25 * base;
    b->duration = (duration < ceiling) ? duration : ceiling;
    /* The flame's development delay after the spark: never negative (the
     * flame can't start before the spark, stage 040), its own every cycle,
     * spread as stage 12c's start was (combustion.c,
     * rb_kernel_delay_unit). The computer's spark allows for its mean. */
    const double delay = v * VARY_START * expected * rb_kernel_delay_unit(start);
    b->kernel_raw_delay_deg = delay;
    b->kernel_delay_deg = rb_finite(delay) ? delay : 0.0; /* a NaN starts with the spark */
    b->start_angle = cycle_angle(b->spark_angle + b->kernel_delay_deg);
    b->spark_due = 1;
}

/* The spark: what can burn is the fuel, or as much of it as the air can burn,
 * times the combustion efficiency. */
static void spark(rb_engine *e, int i, double fraction, double angle)
{
    const rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    b->burning = 0;
    b->kernel_pending = b->kernel_started = b->discharged = b->after_spark = 0;
    if (!e->ignition) {
        ignition_event(e, i, RB_IGN_OFF, fraction, angle, g->m[RB_FUEL]);
        return;
    }
    /* a fuel-cut limiter (stage 133) leaves the sparks alone */
    const int cut = e->desc.limiter_cut == RB_LIMITER_SPARK ? rb_computer_limiter_cut(e) : -1;
    if (cut >= 0) { /* suppression is not an actual discharge */
        e->limiter_cuts++;
        ignition_event(e, i, cut, fraction, angle, g->m[RB_FUEL]);
        return;
    }
    ignition_event(e, i, RB_IGN_COMMAND, fraction, angle, g->m[RB_FUEL]);
    ignition_event(e, i, RB_IGN_DISCHARGE, fraction, angle, g->m[RB_FUEL]);
    b->discharged = b->discharged_this_step = b->after_spark = 1;
    b->discharge_fraction = fraction;
    b->kernel_clock = 0;
    b->burnable = rb_burnable_fuel(g->m[RB_FUEL], g->m[RB_AIR], e->desc.mixture_spread,
                                   e->desc.combustion_efficiency);
    b->fraction = 0.0;
    b->kernel_pending = (b->burnable > 0.0);
    if (!b->kernel_pending) ignition_event(e, i, RB_IGN_NO_REACTANTS, fraction, angle, 0);
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
    if (!b->discharged) return; /* chemical heat requires an actual discharge */
    const double pace = 6.0 * RB_GAS_PACE_RPM * e->dt;   /* degrees per step */
    const double advance = (step > pace) ? step : pace;
    const double first = b->discharged_this_step ? b->discharge_fraction : 0;
    const double previous = b->kernel_clock;
    b->kernel_clock += advance * (1 - first);
    if (b->kernel_pending) {
        if (b->kernel_clock < b->kernel_delay_deg) return;
        const double fraction = first + (b->kernel_delay_deg - previous) / advance;
        ignition_event(e, i, RB_IGN_KERNEL, fraction, cycle_angle(angle + step * fraction),
                       b->burnable);
        b->kernel_pending = 0;
        b->kernel_started = b->burning = 1;
    }
    b->burn_angle = b->kernel_clock - b->kernel_delay_deg;
    const double x = rb_wiebe(b->burn_angle, b->duration, d->wiebe_a, d->wiebe_m);
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

/* The accessories (stage 12c): the alternator, for the car's electrics, and
 * the pumps. Above ACCESSORY_FULL_RPM they take `accessory_power`: a torque
 * that falls as the speed rises. Below it, a steady torque (power at that
 * speed): an alternator can't give its full output that slowly, and a power
 * steering pump takes about the same torque at any speed. That also keeps
 * them from destabilising the idle, as a load that grows as the speed sags
 * would. While the engine cranks (below ACCESSORY_ON_RPM) the alternator
 * isn't charging yet: they come in by 500 rpm. Idling, they are a third of
 * what the engine works against, and they set how much air it needs: without
 * them the four idled at 25 kPa in its manifold, where real ones run at
 * 30-40, its cylinders below the exhaust's pressure when their exhaust
 * valves opened; with its 800 W, at 28 kPa. */
#define ACCESSORY_FULL_RPM 1500.0
#define ACCESSORY_ON_RPM   300.0

static double accessory_torque(const rb_engine_desc *d, double omega)
{
    if (!(d->accessory_power > 0.0)) return 0.0;
    const double rpm = fabs(omega) * 30.0 / PI;
    const double full = d->accessory_power / (((rpm > ACCESSORY_FULL_RPM) ? rpm : ACCESSORY_FULL_RPM) * PI / 30.0);
    const double on = (rpm - ACCESSORY_ON_RPM) / (500.0 - ACCESSORY_ON_RPM);
    return (on >= 1.0) ? full : (on > 0.0) ? on * full : 0.0;
}

double rb_engine_fmep(const rb_engine *e, double omega)
{
    const rb_engine_desc *d = &e->desc;
    double p_max = 0.0;
    for (int i = 0; i < d->cylinders; i++) p_max += e->burn[i].last_p_max;
    p_max /= d->cylinders;
    const double sp = d->stroke * fabs(omega) / PI;    /* mean piston speed: 2 stroke rev/s */
    return d->fmep_constant + d->fmep_pressure * p_max + d->fmep_speed * sp
         + d->fmep_speed2 * sp * sp + accessory_torque(d, omega) * 4.0 * PI / rb_engine_displacement(e);
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

/* Mean pressure in the exhaust collectors. */
static double exhaust_pressure(const rb_engine *e)
{
    double p = 0.0;
    for (int c = 0; c < e->collectors; c++) p += e->collector[c]->p;
    return p / e->collectors;
}

/* The slow gas: the pipes but for their fast cells, and the collectors,
 * stepped every pipe_every steps (pipe.h, two rates). When a block of steps
 * begins (step 3), their rates: all of them cleared before any flows, since
 * one pipe's gas goes into another's cells (an exhaust pipe opens into its
 * muffler's first cell, or goes straight on into it). */
static void slow_flows(rb_engine *e)
{
    const double dt = e->pipe_dt;
    const int pipes = rb_engine_pipe_count(e);
    for (int k = 0; k < e->net.nodes; k++) rb_chamber_clear_rates(&e->xnode[k]);
    for (int k = 0; k < pipes; k++) rb_pipe_clear((rb_pipe *)rb_engine_pipe(e, k), 0);
    for (int k = 0; k < pipes; k++) rb_pipe_flows((rb_pipe *)rb_engine_pipe(e, k), 0, dt);
}

/* When the block ends (step 5): what came in through the snorkel's mouth and
 * what left through the tailpipes' open ends (either way) is counted,
 * everything slow is integrated over the block, and the faces between slow
 * cells get their new flows. Each open end's flow is also kept as a volume,
 * at the density of the gas at the pipe's end (as it was while it moved: the
 * cells are integrated below), whichever way it flows: the sound's sources.
 * What a reversing flow draws back in at first is the gas that has just
 * left, not the outside air: switching to the air's density there would kink
 * the volume flow at every reversal, and the sound (its derivative) would
 * click. */
static void slow_step(rb_engine *e, step_share *share)
{
    const double dt = e->pipe_dt;
    const int pipes = rb_engine_pipe_count(e);
    const rb_face *mouth = &e->snorkel.end_face[0];   /* from the air (a) into the pipe (b) */
    const double m_in = mouth->moved * dt;
    e->mass_in += m_in;
    e->energy_in += m_in * mouth->moved_h;
    for (int j = 0; j < RB_NSPECIES; j++) e->species_in[j] += m_in * mouth->moved_y[j];
    e->mouth_flow = (mouth->b->rho > 0.0) ? -mouth->moved / mouth->b->rho : 0.0;
    for (int s = 0; s < e->systems; s++) {
        const rb_face *out = &e->tailpipe[s]->end_face[1];
        const double m = out->moved * dt;
        e->mass_out += m;
        e->energy_out += m * out->moved_h;
        for (int j = 0; j < RB_NSPECIES; j++) e->species_out[j] += m * out->moved_y[j];
        if (m > 0.0) {
            share->tail_m += m;
            share->tail_mT += m * out->a->T;
        }
        e->tail_flow[s] = (out->a->rho > 0.0) ? out->moved / out->a->rho : 0.0;
    }
    e->pipe_steps++;
    double radiated = 0.0;
    for (int k = 0; k < pipes; k++) radiated += rb_engine_pipe(e, k)->radiated;
    e->energy_out += radiated - e->acoustic_energy_out;
    e->acoustic_energy_out = radiated;
    for (int k = 0; k < e->net.nodes; k++) rb_chamber_integrate(&e->xnode[k], dt);
    for (int k = 0; k < pipes; k++) rb_pipe_integrate((rb_pipe *)rb_engine_pipe(e, k), 0, dt);
    for (int k = 0; k < pipes; k++) rb_pipe_accelerate((rb_pipe *)rb_engine_pipe(e, k), 0, dt);
}

void rb_engine_step(rb_engine *e)
{
    e->injection_count = 0;
    if (e->diag) e->diag->tick++;
    const rb_engine_desc *d = &e->desc;
    const int n = d->cylinders;
    const double dt = e->dt;
    const double q_fuel = rb_fuel_heat();
    const double ivc = rb_valve_closes(&d->intake), evo = rb_valve_opens(&d->exhaust);
    const double v = variation(e);
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
        e->overrun_time = 0.0;
        e->overrun_armed = 0;
        e->rpm_smooth = e->rpm_trend = e->rpm_recent = rpm;
        e->friction_torque = rb_engine_fmep(e, e->omega) * rb_engine_displacement(e) / (4.0 * PI);
        turn = e->omega * dt;
    } else {
        rb_computer_step(e);
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
        b->discharged_this_step = 0;
        if (passes(angle, step_deg, 360.0)) {   /* overlap TDC: the last cycle's peak is known */
            b->last_p_max = b->p_max;
            b->last_angle_p_max = b->angle_p_max;
            b->p_max = 0.0;
            b->last_work = b->work;
            b->last_pumping_work = b->pumping_work;
            b->last_gross_work = b->last_work + b->last_pumping_work;
            b->work = 0.0;
            b->pumping_work = 0.0;
        }
        if (passes(angle, step_deg, ivc)) {
            close_intake(e, i, event_fraction(angle, step_deg, ivc));
            ignition_event(e, i, RB_IGN_SCHEDULED, event_fraction(angle, step_deg, ivc), ivc, 0);
        }
        if (b->spark_due && passes(angle, step_deg, b->spark_angle)) {
            b->spark_due = 0;
            const double fraction = event_fraction(angle, step_deg, b->spark_angle);
            spark(e, i, fraction, cycle_angle(angle + step_deg * fraction));
        }
        if (passes(angle, step_deg, evo)) {
            if (b->kernel_pending || b->burning)
                ignition_event(e, i, b->kernel_pending ? RB_IGN_EVO_PENDING : RB_IGN_EVO_PARTIAL,
                               event_fraction(angle, step_deg, evo), evo,
                               b->burnable * (1 - b->fraction));
            b->spark_due = 0;
            b->after_spark = 0;
            b->burning = 0;        /* whatever hasn't burned by now leaves unburned */
            b->kernel_pending = 0;
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
        if (v > 0.0 && e->exhaust[i].area > 0.0) {   /* its jet flickers (stage 12c) */
            e->flicker[i] += e->flicker_a * (rb_rand_white(&e->rand_flicker) - e->flicker[i]);
            double f = 1.0 + v * FLICKER * e->flicker_norm * e->flicker[i];
            f = (f > 0.25) ? ((f < 1.75) ? f : 1.75) : 0.25;
            e->exhaust[i].area *= f;
        }
    }

    /* 3. Everything that changes the fast gas (the cylinders, the air box,
     * the plenum and the pipes' cells at the valves), computed from the
     * current state, then integrated together. The slow gas's rates first,
     * if a block of steps begins. */
    step_share share = { .p = e->plenum.p, .rho = e->plenum.rho, .pexh = exhaust_pressure(e), .dt = dt };
    const int runners = d->runner_length > 0.0;
    if (e->pipe_phase == 0) slow_flows(e);
    rb_chamber_clear_rates(&e->airbox);
    rb_chamber_clear_rates(&e->plenum);
    rb_pipe_clear(&e->snorkel, 1);
    for (int i = 0; i < n; i++) {
        rb_chamber_clear_rates(&e->cyl[i].gas);
        if (runners) rb_pipe_clear(&e->runner[i], 1);
        rb_pipe_clear(&e->header[i], 1);
    }

    rb_orifice_flow(&e->throttle, dt);
    rb_orifice_flow(&e->idle_air, dt);
    share.throttle = (e->throttle.mdot + e->idle_air.mdot) * dt;
    rb_pipe_flows(&e->snorkel, 1, dt);   /* through the filter into the air box */

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
            const double w = rb_woschni_w(piston_speed, (int)open[i], b->after_spark && b->kernel_started,
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
        if (b->burning || b->kernel_pending) {
            burn_step(e, i, rb_engine_cylinder_angle(e, i), step_deg, q_fuel);
            share.heat += b->heat;
        }
        /* the pipes' ends at the valves (and at the plenum) */
        if (runners) rb_pipe_flows(&e->runner[i], 1, dt);
        rb_pipe_flows(&e->header[i], 1, dt);
    }

    rb_chamber_integrate(&e->airbox, dt);
    rb_chamber_integrate(&e->plenum, dt);
    rb_pipe_integrate(&e->snorkel, 1, dt);
    for (int i = 0; i < n; i++) {
        rb_chamber_integrate(&e->cyl[i].gas, dt);
        if (runners) rb_pipe_integrate(&e->runner[i], 1, dt);
        rb_pipe_integrate(&e->header[i], 1, dt);
    }

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
        b->work -= w;
        if (angle >= 180.0 && angle < 540.0) b->pumping_work += w;
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

    /* 5. The slow gas at the end of its block of steps; then the new flows
     * at the pipes' fast ends, seeing the slow cells' pressures as they are
     * by now. */
    if (++e->pipe_phase >= e->pipe_every) {
        e->pipe_phase = 0;
        slow_step(e, &share);
    }
    rb_pipe_accelerate(&e->snorkel, 1, dt);
    for (int i = 0; i < n; i++) {
        if (runners) rb_pipe_accelerate(&e->runner[i], 1, dt);
        rb_pipe_accelerate(&e->header[i], 1, dt);
    }

    /* 6. Add the step to the cycle; a step across the end of the cycle is
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
    e->physics_tick++;
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
