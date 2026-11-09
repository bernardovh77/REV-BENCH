/*
 * noise.c - the flow's noise and the engine's mechanical sound (see noise.h).
 */
#include "noise.h"

#include "rand.h"
#include "rb.h"
#include "valve.h"
#include "fastmath.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

/* ---- the numbers: physics where there is some, calibrated where noted
 * (docs/stage-12b.md), and left for stage S5 to tune by ear ---- */

/* The tailpipe's jet: Lighthill's constant for a cold jet, the Strouhal
 * number its spectrum peaks at, and the lowest it makes (the pipes make the
 * low end themselves). */
#define JET_K          4e-5
#define JET_STROUHAL   0.2
#define JET_FLOOR_HZ   150.0
#define JET_TOP_HZ     6000.0   /* ... and above this it fades faster (12 dB an
                                   octave): the jet's smallest eddies, the air */

/* The jets inside the engine: the share of a choked jet's stream power that
 * becomes sound (times M^5 below choking), and their band: from about 1 kHz,
 * where the pipes' own sound fades, falling 6 dB an octave of spectral
 * density, and 12 above where the way out takes off the highs.
 * Calibrated in stage 115, when the jets' Mach number became the gap's:
 *   - the exhaust valves' (and a chambered can's baffles'), heard down the
 *     exhaust, against the real engines' energy above 2 kHz, idling and under
 *     load: 5e-4, within IEC 60534-8-3's range for valves (1e-4 as a gap
 *     begins to choke, about 1e-3 well beyond);
 *   - the throttle's, the idle valve's and the intake valves', heard at the
 *     intake's mouth: a tenth of that. Their noise is made downstream of the
 *     gap and has to come back up through it, against the flow (a choked gap
 *     lets no sound back at all), then through the air box. 5e-5 keeps the
 *     four's highs over the open bonnet where stage 12b calibrated them (the
 *     Honda V6 recorded in front); 5e-4 put them 7 dB higher. */
#define VALVE_ETA      5e-4
#define THROTTLE_ETA   5e-5
#define INNER_HP_HZ    800.0
#define INNER_LP_HZ    1500.0

/* How the way out shapes the internal jets' noise: what gets through of its
 * power, and where the highs start going faster. A monolith catalyst takes
 * a few dB; a chambered can's transmission loss is 20-30 dB above its first
 * passband, a packed straight-through's 5-15 dB, rising with frequency
 * (Munjal 2014); an air box and its filter element 20-25 dB at 1-4 kHz. */
#define PASS_CATALYST  0.5
#define PASS_NONE      1.0
#define PASS_STRAIGHT  0.08
#define PASS_CHAMBERED 0.004
#define PASS_AIRBOX    0.003
#define TOP_NONE_HZ      9000.0
#define TOP_STRAIGHT_HZ  6000.0
#define TOP_CHAMBERED_HZ 3500.0
#define TOP_AIRBOX_HZ    4500.0

/* The exhaust gas's speed of sound for the noise's trip down the pipes
 * (about 900 K), and the heat capacity ratios for the jets' sonic speed. */
#define EXHAUST_C      580.0
#define GAMMA_BURNED   1.32
#define GAMMA_AIR      1.40

/* The block's and head's modes, for an 86 mm bore (the four's); a bore b
 * moves them by sqrt(0.086 / b). Each: its frequency (Hz) and Q, and how
 * hard each kind of blow strikes it: the pistons' force (combustion), a
 * piston slapping the bore, a valve seating, an injector's needle. */
#define MODE_BORE 0.086
static const struct {
    double f, q, force, slap, valve, injector;
} MODES[RB_NOISE_MODES] = {
    {  950.0, 10.0, 1.0, 0.4, 0.0, 0.0 },   /* the block, bending           */
    { 1750.0, 14.0, 0.7, 1.0, 0.3, 0.0 },   /* the block's skirt: slap       */
    { 2700.0, 18.0, 0.3, 0.8, 0.5, 0.1 },   /* the liners: slap too          */
    { 3500.0, 22.0, 0.1, 0.2, 1.0, 0.3 },   /* the head                      */
    { 5500.0, 28.0, 0.0, 0.05, 0.4, 0.4 },  /* the cam cover, the valvetrain */
    { 7800.0, 30.0, 0.0, 0.0, 0.1, 0.6 },   /* the injectors' rail           */
};

/* Pa at 1 m from the engine bay, calibrated (docs/stage-12b.md): over the
 * open bonnet the four's own sound is about 63 dBA idling and 83 at 3000 rpm
 * wide open, its share of the energy above 1, 2 and 4 kHz like the recorded
 * Honda V6's in front, and piston slap strongest around 2 kHz:
 *   - per N the pistons' total force changes in a step (combustion);
 *   - per N of axial force on a piston as it crosses the bore (slap);
 *   - a valve seating at 1000 rpm (the cam's ramps go as the rpm), lifting
 *     off at a third of that, each valve of a cylinder adding its own;
 *   - an injector's click. */
#define K_FORCE    8e-6
#define K_SLAP     1.5e-5
#define K_VALVE    0.06
#define K_INJECTOR 0.05

/* No two impacts are the same (the oil film, the clearances, the cam's
 * wear): each slap and valve varies by this much, at random (RMS). */
#define IMPACT_SPREAD 0.25

/* A piston, its pin and the rod's small end, as a bore's cube: about 0.45 kg
 * at 86 mm. */
#define RECIP_DENSITY 707.0   /* kg per m^3 of bore^3 */

/* The mounts: the gas torque's swings, 20-200 Hz, through the mounts into
 * the body (Pa in the cabin per N m); its mean they don't pass. */
#define K_MOUNT      2e-3
#define MOUNT_HP_HZ  20.0
#define MOUNT_LP_HZ  200.0

/* ---- the generator: white noise of RMS 1 (rand.h) ---- */

static double white(uint64_t *state)
{
    return rb_rand_white(state);
}

/* One-pole filter coefficient for a corner at f (Hz), stepping every dt. */
static double one_pole(double f, double dt)
{
    return 1.0 - exp(-2.0 * PI * f * dt);
}

/* One step of a band: the high-pass is x less its own low-pass; then the two
 * low-passes. */
static double band_step(rb_noise_band *b, double x)
{
    b->hp_lp += b->hp * (x - b->hp_lp);
    b->mid += b->lp * ((x - b->hp_lp) - b->mid);
    b->out += b->top * (b->mid - b->out);
    return b->out;
}

/* A band from hp_hz, falling from lp_hz and faster from top_hz, scaled to an
 * RMS of 1 for white noise of RMS 1: measured once, on 2^15 steps from a
 * generator of its own, so the sim's noise stays the same. */
static void band_init(rb_noise_band *b, double hp_hz, double lp_hz, double top_hz, double dt)
{
    uint64_t state = 0x9E3779B97F4A7C15ULL;
    b->hp = one_pole(hp_hz, dt);
    b->lp = one_pole(lp_hz, dt);
    b->top = one_pole(top_hz, dt);
    b->hp_lp = b->mid = b->out = 0.0;
    double sum = 0.0;
    for (int k = 0; k < 32768; k++) {
        const double y = band_step(b, white(&state));
        if (k >= 4096) sum += y * y;
    }
    b->norm = (sum > 0.0) ? 1.0 / sqrt(sum / (32768 - 4096)) : 0.0;
    b->hp_lp = b->mid = b->out = 0.0;
}

/* ---- set up ---- */

void rb_noise_init(rb_noise *n, const rb_engine *e, uint64_t seed)
{
    const rb_engine_desc *d = &e->desc;
    memset(n, 0, sizeof *n);
    n->state = seed ? seed : 0x12B12B12B12BULL;   /* xorshift's state can't be 0 */
    n->detail_state = n->state ^ 0x52324149524ULL;
    if (!n->detail_state) n->detail_state = 1;
    n->dt = e->dt;
    n->injection_tick = e->physics_tick; /* no replay when attached after delivery */

    /* The flow. The tailpipes' jets; the exhaust valves' noise, each
     * cylinder's on its way to its system's tailpipe; the throttle's. */
    n->tail_d = d->tailpipe_diameter;
    for (int s = 0; s < RB_MAX_COLLECTORS; s++) n->tail_area[s] = PI / 4.0 * n->tail_d * n->tail_d;
    n->jet_hp = one_pole(JET_FLOOR_HZ, n->dt);
    n->jet_top = one_pole(JET_TOP_HZ, n->dt);
    double pass, top_hz;
    switch (d->muffler_type) {
    case RB_MUFFLER_NONE:     pass = PASS_NONE;      top_hz = TOP_NONE_HZ;      break;
    case RB_MUFFLER_STRAIGHT: pass = PASS_STRAIGHT;  top_hz = TOP_STRAIGHT_HZ;  break;
    default:                  pass = PASS_CHAMBERED; top_hz = TOP_CHAMBERED_HZ; break;
    }
    /* The catalyst takes its share only if there is one: catalyst_laminar is
     * 0 for bypass pipes with no substrate (engine.h). */
    n->blow_pass = (d->catalyst_laminar > 0.0 ? PASS_CATALYST : 1.0) * pass;
    for (int s = 0; s < RB_MAX_COLLECTORS; s++)
        band_init(&n->blow[s], INNER_HP_HZ, INNER_LP_HZ, top_hz, n->dt);

    /* Where each cylinder's noise comes out. Its gas goes down its header,
     * its collector's exhaust pipe, the centre pipe and link if the exhaust
     * has them, a muffler and its tailpipe. Without a junction that is its
     * own system's tailpipe. Where the banks meet in a junction, an X before
     * the centre pipes or a Y before the mufflers, it goes on down every
     * pipe that leaves the junction: the same noise (the same phase) at each
     * tailpipe it reaches, its power shared equally among them. The paths
     * beyond a junction are equally long, so it arrives at all of them at
     * once. */
    int beyond[RB_MAX_COLLECTORS] = { 0 };   /* the tailpipes a junction feeds */
    if (d->junction_volume > 0.0) {
        if (d->junction_before_center)       /* X: every centre pipe leaves it */
            for (int c = 0; c < e->collectors; c++) beyond[(e->systems == 2) ? c : 0] = 1;
        else                                 /* Y: every muffler starts there  */
            for (int s = 0; s < e->systems; s++) beyond[s] = 1;
    }
    const double middle = (d->center_length > 0.0) ? d->center_length + d->link_length : 0.0;
    for (int i = 0; i < d->cylinders; i++) {
        const double path = d->header_length[i] + d->exhaust_pipe_length + middle
                          + d->muffler_length + d->tailpipe_length;
        double steps = path / EXHAUST_C / n->dt;
        if (!(steps < RB_NOISE_DELAY - 1)) steps = RB_NOISE_DELAY - 1;
        n->blow_delay[i] = (int)lround(steps);
        int reach[RB_MAX_COLLECTORS] = { 0 }, count = 0;
        if (d->junction_volume > 0.0) memcpy(reach, beyond, sizeof reach);
        else reach[(e->systems == 2) ? d->collector[i] : 0] = 1;
        for (int s = 0; s < RB_MAX_COLLECTORS; s++) count += reach[s];
        for (int s = 0; s < RB_MAX_COLLECTORS; s++)
            n->blow_share[i][s] = reach[s] ? 1.0 / sqrt((double)count) : 0.0;
    }
    band_init(&n->hiss, INNER_HP_HZ, INNER_LP_HZ, TOP_AIRBOX_HZ, n->dt);
    band_init(&n->intake_roar, 150.0, 1200.0, TOP_AIRBOX_HZ, n->dt);

    /* The engine: its modes, moved by the bore; where its pistons and crank
     * are now, so nothing strikes at the start. */
    const double scale = sqrt(MODE_BORE / d->bore);
    for (int k = 0; k < RB_NOISE_MODES; k++) {
        const double f = MODES[k].f * scale, w = 2.0 * PI * f * n->dt;
        const double r = exp(-PI * f / MODES[k].q * n->dt);   /* bandwidth f / Q */
        n->mode[k].a1 = 2.0 * r * cos(w);
        n->mode[k].a2 = r * r;
        n->mode[k].in = sin(w);
    }
    n->recip_mass = RECIP_DENSITY * d->bore * d->bore * d->bore;
    for (int i = 0; i < d->cylinders; i++) {
        n->force += (e->cyl[i].gas.p - d->p_ambient) * e->cyl[i].geom.area;
        n->angle[i] = rb_engine_cylinder_angle(e, i);
    }
    n->mount_hp = rb_finite(e->gas_torque) ? e->gas_torque : 0.0;   /* settled */
    n->mount_hp_a = one_pole(MOUNT_HP_HZ, n->dt);
    n->mount_lp_a = one_pole(MOUNT_LP_HZ, n->dt);
}

/* ---- the flow ---- */

/* The sound power (W) of a jet through an orifice into a pipe: a share
 * eta M^5 of its stream power 1/2 mdot U^2, U the gas's speed in the gap
 * and M its Mach number there, gamma the gas's heat capacity ratio.
 *
 * U and M come from the pressures either side, as the nozzle's flow does
 * (flow.c): the gas expands from the upstream chamber's p0 and T0 down to
 * the pressure downstream, or only to the critical pressure if that is
 * lower: then the gap is choked and M = 1. With x = (p / p0)^((g-1)/g),
 * the gap's T / T0,
 *     U^2 = 2 g / (g - 1) R T0 (1 - x),    M^2 = 2 / (g - 1) (1 / x - 1),
 * and choked x = 2 / (g + 1): U is the sonic speed there, sqrt(2 g R T0 /
 * (g + 1)). Until stage 115, U was mdot / (rho0 A cd), with the upstream
 * gas's density rather than the gap's, and M = U / c0: a choked gap read
 * M = 0.58 and 15.8 dB too little power (the audit's D02,
 * docs/sound-physics-audit-2026-10-22.md). */
static double inner_jet(const rb_orifice *o, double gamma, double eta)
{
    if (!(o->area > 0.0 && o->mdot > 0.0)) return 0.0;   /* shut, or flowing back */
    const rb_chamber *up = o->a, *down = o->b;
    /* No jet unless the pressure falls across the gap (and nothing is NaN). */
    if (!(up->T > 0.0 && down->p >= 0.0 && down->p < up->p)) return 0.0;
    const double choked = 2.0 / (gamma + 1.0);
    double x = pow(down->p / up->p, (gamma - 1.0) / gamma);
    if (x < choked) x = choked;   /* choked: sonic in the gap */
    const double u2 = 2.0 * gamma / (gamma - 1.0) * up->gas.R * up->T * (1.0 - x);
    const double m2 = 2.0 / (gamma - 1.0) * (1.0 / x - 1.0);
    return eta * m2 * m2 * sqrt(m2) * 0.5 * o->mdot * u2;
}

static void flow_noise(rb_noise *n, const rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    const rb_chamber *air = &e->exhaust_air;
    const double rho0 = air->rho, T0 = air->T;
    const double c0 = sqrt(GAMMA_AIR * air->gas.R * T0);
    const double radiate = rho0 * c0 / (4.0 * PI);   /* p^2 at 1 m per W radiated */
    n->intake_power = n->baffle_power = 0.0;
    for (int i = 0; i < d->cylinders; i++)
        n->intake_power += inner_jet(&e->intake[i], GAMMA_AIR, THROTTLE_ETA);

    /* The exhaust valves: each one's noise into the tailpipes it reaches,
     * arriving when it gets there (a ring the cylinders write ahead into). */
    for (int i = 0; i < d->cylinders; i++) {
        const double w = inner_jet(&e->exhaust[i], GAMMA_BURNED, VALVE_ETA);
        if (w > 0.0) {
            const int at = (n->blow_pos + n->blow_delay[i]) & (RB_NOISE_DELAY - 1);
            const double x = sqrt(radiate * w * n->blow_pass) * white(&n->state);
            for (int s = 0; s < e->systems; s++)
                if (n->blow_share[i][s] > 0.0) n->blow_ring[s][at] += (float)(x * n->blow_share[i][s]);
        }
    }

    for (int s = 0; s < e->systems; s++) {
        /* Chambered baffles are a downstream constriction. Their generated
         * noise bypasses the catalyst; use the modeled effective hole area
         * and the cells either side, with the same jet law. */
        if (d->muffler_type == RB_MUFFLER_CHAMBERED) {
            const rb_face *f = &e->tailpipe[s].end_face[0];
            rb_orifice hole = {0};
            hole.a = f->a;
            hole.b = f->b;
            hole.area = d->muffler_area;
            hole.cd = 1.0;
            hole.mdot = f->moved;
            const double w = inner_jet(&hole, GAMMA_BURNED, VALVE_ETA);
            n->baffle_power += w;
            int delay = (int)fmin(d->tailpipe_length / EXHAUST_C / n->dt,
                                  RB_NOISE_DELAY - 1.0);
            const int at = (n->blow_pos + delay) & (RB_NOISE_DELAY - 1);
            n->blow_ring[s][at] += (float)(sqrt(radiate * w) * white(&n->detail_state));
        }
        /* The tailpipe's jet: Lighthill's U^8, hot. */
        double jet = 0.0;
        const double q = e->tail_flow[s];
        if (q > 0.0) {
            const rb_pipe *t = &e->tailpipe[s];
            const double u = q / n->tail_area[s], u2 = u * u, u4 = u2 * u2;
            const double hot = t->cell[t->cells - 1].T / T0;
            const double w = JET_K * rho0 * u4 * u4 * n->tail_d * n->tail_d / (c0 * c0 * c0 * c0 * c0) * hot;
            /* Its spectrum: flat up to the Strouhal peak, then falling, a
             * one-pole low-pass there whose noise gain a / (2 - a) is undone;
             * faster above JET_TOP_HZ. */
            const double wc = 2.0 * PI * JET_STROUHAL * u / n->tail_d * n->dt;
            const double a = wc / (1.0 + wc);
            const double x = white(&n->state);
            n->jet_hp_lp[s] += n->jet_hp * (x - n->jet_hp_lp[s]);
            n->jet_mid[s] += a * ((x - n->jet_hp_lp[s]) - n->jet_mid[s]);
            n->jet_out[s] += n->jet_top * (n->jet_mid[s] * sqrt((2.0 - a) / a) - n->jet_out[s]);
            jet = sqrt(radiate * w) * n->jet_out[s];
        }
        /* The exhaust valves' noise, arriving now. */
        float *slot = &n->blow_ring[s][n->blow_pos];
        const double blow = band_step(&n->blow[s], *slot) * n->blow[s].norm;
        *slot = 0.0f;
        n->out[RB_NOISE_TAIL0 + s] = jet + blow;
    }
    n->blow_pos = (n->blow_pos + 1) & (RB_NOISE_DELAY - 1);

    /* The throttle and the idle valve beside it, heard at the mouth. */
    const double w = inner_jet(&e->throttle, GAMMA_AIR, THROTTLE_ETA)
                   + inner_jet(&e->idle_air, GAMMA_AIR, THROTTLE_ETA);
    const double hiss = band_step(&n->hiss, sqrt(radiate * w * PASS_AIRBOX) * white(&n->state));
    const double roar = band_step(&n->intake_roar,
        sqrt(radiate * n->intake_power * PASS_AIRBOX) * white(&n->detail_state));
    n->out[RB_NOISE_MOUTH] = hiss * n->hiss.norm + roar * n->intake_roar.norm;
}

/* ---- the engine ---- */

/* 1 if a crank turning `step` degrees from `angle` passes `event` (degrees in
 * the 720-degree cycle), as engine.c decides its events. */
static int passes(double angle, double step, double event)
{
    double ahead = event - angle;
    if (ahead < 0.0) ahead += 720.0;
    return ahead < step;
}

/* An impulse of `size` (Pa at 1 m) into every mode, as hard as `by` says. */
enum { BY_FORCE, BY_SLAP, BY_VALVE, BY_INJECTOR };
static void strike(double in[RB_NOISE_MODES], int by, double size)
{
    for (int k = 0; k < RB_NOISE_MODES; k++) {
        const double weight = (by == BY_FORCE) ? MODES[k].force
                            : (by == BY_SLAP)  ? MODES[k].slap
                            : (by == BY_VALVE) ? MODES[k].valve : MODES[k].injector;
        in[k] += weight * size;
    }
}

/* An impact of `kind` (RB_IMPACT_*), striking as `by` says: counted, then
 * its size varied by IMPACT_SPREAD (but an injector's: always the same). */
static void impact(rb_noise *n, double in[RB_NOISE_MODES], int by, int kind, double size)
{
    n->impacts[kind]++;
    n->impact_energy[kind] += size * size;
    if (kind != RB_IMPACT_INJECTOR) size *= 1.0 + IMPACT_SPREAD * white(&n->state);
    strike(in, by, size);
}

static void engine_noise(rb_noise *n, const rb_engine *e)
{
    const rb_engine_desc *d = &e->desc;
    double in[RB_NOISE_MODES] = { 0.0 };

    /* Combustion: the pistons' force, as it changes. */
    double force = 0.0;
    for (int i = 0; i < d->cylinders; i++) force += (e->cyl[i].gas.p - d->p_ambient) * e->cyl[i].geom.area;
    if (rb_finite(force)) {
        strike(in, BY_FORCE, K_FORCE * (force - n->force));
        n->force = force;
    }

    /* What each cylinder passed in this step. */
    const double omega = rb_finite(e->omega) ? fabs(e->omega) : 0.0;
    const double rpm = omega * 30.0 / PI;
    const double crank_r = 0.5 * d->stroke, lambda = crank_r / d->rod;
    const double inertia = n->recip_mass * crank_r * omega * omega;
    const double ivo = rb_valve_opens(&d->intake), ivc = rb_valve_closes(&d->intake);
    const double evo = rb_valve_opens(&d->exhaust), evc = rb_valve_closes(&d->exhaust);
    const double seat_in = K_VALVE * rpm / 1000.0 * sqrt((double)d->intake.count);
    const double seat_ex = K_VALVE * rpm / 1000.0 * sqrt((double)d->exhaust.count);
    /* Consume actual doses, independently of the optional trace's capacity.
     * Existing modal sampling places onset at the interval's end, never early. */
    if (n->injection_tick != e->physics_tick) {
        /* each dose of the engine's last step clicks once: injection_tick
         * marks that step as heard */
        for (int j = 0; j < e->injection_count; j++) {
            const rb_injection_event *event = &e->injection[j];
            if (event->tick + 1 == e->physics_tick && event->kind == RB_INJ_DELIVERED &&
                event->fuel_kg > 0.0 && rb_finite(event->fuel_kg)) {
                impact(n, in, BY_INJECTOR, RB_IMPACT_INJECTOR, K_INJECTOR);
                n->injected_fuel_kg += event->fuel_kg;
            }
        }
    }
    for (int i = 0; i < d->cylinders; i++) {
        const double from = n->angle[i], angle = rb_engine_cylinder_angle(e, i);
        double step = angle - from;
        if (step < 0.0) step += 720.0;
        n->angle[i] = angle;
        if (!(step > 0.0 && step < 90.0)) continue;   /* still, or a jump (a reset) */
        const double gas = (e->cyl[i].gas.p - d->p_ambient) * e->cyl[i].geom.area;
        /* Slap: at each dead centre, as hard as the axial force on the piston
         * then, the gas's less the parts' inertia (which pulls it up at TDC
         * and down at BDC: m r w^2 (cos theta + lambda cos 2 theta)). */
        if (passes(from, step, 0.0) || passes(from, step, 360.0))
            impact(n, in, BY_SLAP, RB_IMPACT_SLAP, K_SLAP * fabs(gas - inertia * (1.0 + lambda)));
        if (passes(from, step, 180.0) || passes(from, step, 540.0))
            impact(n, in, BY_SLAP, RB_IMPACT_SLAP, K_SLAP * fabs(gas + inertia * (1.0 - lambda)));
        /* The valves: seating, and lifting off at a third of that. */
        if (passes(from, step, ivc)) impact(n, in, BY_VALVE, RB_IMPACT_SEAT, seat_in);
        if (passes(from, step, evc)) impact(n, in, BY_VALVE, RB_IMPACT_SEAT, seat_ex);
        if (passes(from, step, evo)) impact(n, in, BY_VALVE, RB_IMPACT_LIFT, seat_ex / 3.0);
        if (passes(from, step, ivo)) {
            impact(n, in, BY_VALVE, RB_IMPACT_LIFT, seat_in / 3.0);
        }
    }

    /* The modes ring, and the bay radiates them all. */
    double bay = 0.0;
    for (int k = 0; k < RB_NOISE_MODES; k++) {
        rb_noise_mode *m = &n->mode[k];
        const double y = m->in * in[k] + m->a1 * m->y1 - m->a2 * m->y2;
        m->y2 = m->y1;
        m->y1 = y;
        bay += y;
    }
    n->out[RB_NOISE_BAY] = bay;

    /* The mounts: the gas torque's swings, 20-200 Hz (the high-pass is the
     * torque less its own low-pass, which starts at the torque's mean). */
    const double x = rb_finite(e->gas_torque) ? e->gas_torque : n->mount_hp;
    n->mount_hp += n->mount_hp_a * (x - n->mount_hp);
    n->mount_lp += n->mount_lp_a * ((x - n->mount_hp) - n->mount_lp);
    n->out[RB_NOISE_MOUNT] = K_MOUNT * n->mount_lp;
}

/* ---- each step ---- */

void rb_noise_step(rb_noise *n, const rb_engine *e, int what)
{
    for (int k = 0; k < RB_NOISE_OUTPUTS; k++) n->out[k] = 0.0;
    if (what & RB_NOISE_FLOW) flow_noise(n, e);
    if (what & RB_NOISE_ENGINE) engine_noise(n, e);
    n->injection_tick = e->physics_tick; /* discard disabled/stale source intervals */

    /* A broken state upstream mustn't stay in the filters: anything that
     * isn't a number now starts the noise over, quiet. */
    for (int k = 0; k < RB_NOISE_OUTPUTS; k++) {
        if (!rb_finite(n->out[k])) {
            const uint64_t state = n->state;
            rb_noise_init(n, e, state);
            for (int j = 0; j < RB_NOISE_OUTPUTS; j++) n->out[j] = 0.0;
            break;
        }
    }
}
