/*
 * engine.c - cylinders, valves, throttle, plenum, fuel, spark and the crank
 * stepped together (see engine.h).
 */
#include "engine.h"

#include "combustion.h"
#include "heat.h"
#include "rand.h"
#include "rb.h"   /* RB_MUFFLER_* */

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
/* The pipes (stage 10; pipe.h). They step at RB_PIPE_RATE (48 kHz) unless
 * told otherwise (rb_engine_init_at): a wave crosses a 5 cm cell in about
 * three of those steps. The runners and headers, which tune the engine's
 * breathing, have cells of about 5 cm; the long exhaust pipes and tailpipes
 * 7.5 cm (still 10 to a wavelength at 700 Hz). At 24 kHz the shortest
 * stable cell is 5.2 cm (rb_pipe_cells_for), and the torque moves by under
 * 0.3 % (stage 13). */
#define SHORT_CELL       0.05
#define LONG_CELL        0.075
/* Losses where gas leaves a volume for a pipe, in velocity heads on top of
 * its dynamic head: from the plenum into a runner (a rounded entry, 0.3),
 * from a collector back into a header, or into its exhaust pipe through the
 * merging cone, and air drawn into a tailpipe's plain end (0.5). Flowing out
 * of a pipe into a volume the gas's speed is lost anyway. */
#define RUNNER_ENTRY     0.3
#define HEADER_ENTRY     0.5
#define COLLECTOR_ENTRY  0.3
#define TAILPIPE_ENTRY   0.5
/* A catalyst's laminar loss (pipe.h), Pa per m/s of its exhaust pipe's gas:
 * a 400-cell substrate 12 cm long and five times the pipe's area across
 * costs about 2.5 kPa at 100 m/s, near full power. */
#define CATALYST_LAMINAR 25.0
/* The mufflers (stage 12; rb.h: rb_muffler_type). A chambered one's baffles
 * are perforated: besides their restriction (muffler_area) they cost a
 * laminar loss, Pa per m/s of the tailpipe's gas, which damps the exhaust's
 * rings a little (its lowest, the boom at 1500-2500 rpm in the four, by 2 dB
 * at three times this) and costs 3.7 kPa of back pressure at the four's full
 * power. A straight-through one's packing, Pa per m/s of the swings per
 * metre, takes the swings faster than about 640 Hz (settling in 0.25 ms),
 * 10 dB over 45 cm of hot gas, and less of the slower ones. */
#define BAFFLE_LAMINAR   30.0
#define PACKING          1500.0
#define PACKING_SETTLE   2.5e-4
/* The way in (stage 12): air drawn into the snorkel's flared mouth (an entry
 * loss of 0.2 velocity heads), then through the air box's clean paper filter:
 * a laminar loss, Pa per m/s of the snorkel's air, about 1.4 kPa at the
 * four's full power. */
#define SNORKEL_ENTRY    0.2
#define FILTER_LAMINAR   60.0
/* Darcy friction factors: smooth intake runners, rougher exhaust pipes. */
#define RUNNER_FRICTION  0.02
#define EXHAUST_FRICTION 0.025
/* The pipes' walls (K), warm from the gas inside: the intake's from the
 * cylinder head, the exhaust's cooler the further from the engine. */
#define RUNNER_WALL      330.0
#define HEADER_WALL      650.0
#define EXHAUST_WALL     550.0
#define MUFFLER_WALL     450.0
#define TAILPIPE_WALL    400.0

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

    /* An 8 L air box behind a 35 cm snorkel of 70 mm (stage 12), then the
     * throttle and the plenum. */
    d->airbox_volume = 8e-3;
    d->snorkel_length = 0.35;
    d->snorkel_diameter = 0.070;
    d->throttle = (rb_throttle){ 0.060, 0.008, 7.0, 0.8, 2e-6 };
    d->plenum_volume = 2.5e-3;
    /* Intake runners, plenum to valve: 330 mm long, 40 mm across. The air in
     * them rams the cylinders fullest, and the torque peaks, at about
     * 4500 rpm (stage 9). */
    d->runner_length = 0.33;
    d->runner_diameter = 0.040;
    /* The exhaust: 450 mm headers of 36 mm into one collector, a 50 mm pipe
     * 1.8 m long to the catalyst and a chambered muffler, 10 L and 45 cm long
     * (17 cm across), and a 60 cm tailpipe. */
    for (int i = 0; i < 4; i++) {
        d->header_length[i] = 0.45;
        d->collector[i] = 0;
    }
    d->header_diameter = 0.036;
    d->exhaust_pipe_length = 1.8;
    d->exhaust_pipe_diameter = 0.050;
    d->exhaust_systems = 1;
    d->catalyst_area = 21e-4;
    d->muffler_type = RB_MUFFLER_CHAMBERED;
    d->muffler_volume = 10e-3;
    d->muffler_length = 0.45;
    d->muffler_area = 21e-4;
    d->tailpipe_length = 0.6;
    d->tailpipe_diameter = 0.050;
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
    /* Friction in Chen and Flynn's form, calibrated in stage 9 to a modern
     * engine's: about 1.0 bar at 2000 rpm and 1.55 bar at 5000, wide open
     * (Knauder et al. 2019 measured 1.2 bar at most for a modern gasoline
     * four's base engine, motored at 5000 rpm; the oil and water pumps add
     * 0.1-0.2 bar, and the gas pressure loads the rings and bearings). That's
     * 0.38 bar + 0.005 p_max + 0.053 bar per m/s + 0.0004 bar per (m/s)^2:
     * 0.6 bar at idle, 1.2 bar at 3000 rpm and 1.7 bar at 6000. The commonly
     * used coefficients of stage 7 (0.4, 0.005, 0.09, 0.0009) are an older
     * engine's: 2.5 bar at 6000 rpm. */
    d->fmep_constant = 0.38e5;
    d->fmep_pressure = 0.005;
    d->fmep_speed = 0.053e5;
    d->fmep_speed2 = 0.0004e5;
    d->accessory_power = 0.0;   /* the presets have theirs (stage 12c) */
    /* A 1.3 kW starter, geared about 50:1 to the crank: 200 N m when stalled,
     * enough to push a cylinder through its compression from rest, falling to
     * nothing at 250 rpm. It cranks the engine at about 240 rpm. */
    d->starter_torque = 200.0;
    d->starter_free_rpm = 250.0;
    /* The engine computer: idle at 800 rpm with an idle air valve of up to
     * 38 mm^2 (about a 7 mm hole; warm, the engine idles with it about 15 %
     * open), the spark cut at 6800 rpm until 6700, and a late spark while
     * cranking so the first firings don't kick back. The idle gains are tuned
     * for this engine's valve, flywheel, pipes and idle speed: on another
     * engine they may make the idle hunt or wander. */
    d->idle_rpm = 800.0;
    d->idle_valve_area = 38e-6;
    d->idle_gain_p = 3.0e-4;
    d->idle_gain_i = 3.0e-4;
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
        x.header_length[i] = d->header_length[0];   /* every header like the first, */
        x.collector[i] = 0;                         /* into one collector           */
    }
    x.exhaust_systems = 1;
    x.junction_before_center = 0;
    x.junction_volume = 0.0;
    if (rb_engine_layout(&x) != 0) return -1;
    *d = x;
    return 0;
}

/* A muffler pipe's bore (m): a chambered one's can, as wide as its volume
 * over its length makes it; otherwise the tailpipe's. */
static double muffler_width(const rb_engine_desc *d)
{
    if (d->muffler_type != RB_MUFFLER_CHAMBERED) return d->tailpipe_diameter;
    return sqrt(4.0 * d->muffler_volume / (PI * d->muffler_length));
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
    if (!(d->airbox_volume > 0.0 && d->airbox_volume < 1.0)) return -1;
    if (!(d->snorkel_length >= 0.05 && d->snorkel_length <= 10.0)) return -1;
    if (!(d->snorkel_diameter > 0.01 && d->snorkel_diameter < 0.5)) return -1;
    if (rb_throttle_check(&d->throttle) != 0) return -1;
    if (!(d->plenum_volume > 0.0 && d->plenum_volume < 1.0)) return -1;
    /* No runners (0: the valves draw straight from the plenum, as in stages
     * 5-8), or at least 2 cm of them: in a shorter one the air rings faster
     * than the physics step can follow (at about 2 c / length). */
    if (!(d->runner_length == 0.0 || (d->runner_length >= 0.02 && d->runner_length < 5.0))) return -1;
    if (d->runner_length > 0.0 && !(d->runner_diameter > 0.005 && d->runner_diameter < 0.5)) return -1;
    /* The exhaust: every header at least 5 cm long, joining collector 0 or 1
     * (a second only if there's a first), and two systems only with two
     * collectors. */
    int joined[RB_MAX_COLLECTORS] = { 0 };
    for (int i = 0; i < d->cylinders; i++) {
        if (!(d->header_length[i] >= 0.05 && d->header_length[i] <= 10.0)) return -1;
        if (d->collector[i] < 0 || d->collector[i] >= RB_MAX_COLLECTORS) return -1;
        joined[d->collector[i]]++;
    }
    if (!joined[0]) return -1;
    if (!(d->header_diameter > 0.01 && d->header_diameter < 0.3)) return -1;
    if (!(d->exhaust_pipe_length >= 0.05 && d->exhaust_pipe_length <= 10.0)) return -1;
    if (!(d->exhaust_pipe_diameter > 0.01 && d->exhaust_pipe_diameter < 0.5)) return -1;
    if (!(d->exhaust_systems == 1 || (d->exhaust_systems == 2
        && (joined[1] || d->junction_volume > 0.0)))) return -1;
    if (!(d->center_length >= 0.0 && d->center_length <= 3.0)
        || !(d->link_length >= 0.0 && d->link_length <= 3.0)
        || !(d->junction_volume >= 0.0 && d->junction_volume <= 0.02)) return -1;
    if (d->junction_before_center != 0 && d->junction_before_center != 1) return -1;
    if (d->outlet_radiation != 0 && d->outlet_radiation != 1) return -1;
    if (d->junction_before_center && !(d->junction_volume > 0.0 && joined[1]
        && d->exhaust_systems == 2)) return -1;
    if (d->center_length > 0.0) {
        if (!(d->center_length >= 0.05 && d->center_diameter > 0.01
            && d->center_diameter < 0.3 && d->center_volume >= 0.0
            && d->center_volume < 0.05 && d->center_packing >= 0.0
            && d->center_packing <= 20000.0 && d->link_length >= 0.05)) return -1;
        if (d->center_volume > 0.0 && !(sqrt(4.0 * d->center_volume
            / (PI * d->center_length)) < 0.6)) return -1;
    } else if (d->link_length > 0.0 || d->junction_volume > 0.0) return -1;
    if (!(d->catalyst_area > 0.0 && d->catalyst_area < 0.1)) return -1;
    if (d->muffler_type < RB_MUFFLER_NONE || d->muffler_type > RB_MUFFLER_CHAMBERED) return -1;
    if (!(d->muffler_volume > 0.0 && d->muffler_volume < 1.0)) return -1;
    if (!(d->muffler_length >= 0.05 && d->muffler_length <= 10.0)) return -1;
    if (d->muffler_type == RB_MUFFLER_CHAMBERED
        && !(muffler_width(d) > 0.01 && muffler_width(d) < 1.0)) return -1;
    if (!(d->muffler_area > 0.0 && d->muffler_area < 0.1)) return -1;
    if (!(d->tailpipe_length >= 0.05 && d->tailpipe_length <= 10.0)) return -1;
    if (!(d->tailpipe_diameter > 0.01 && d->tailpipe_diameter < 0.5)) return -1;
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
    if (!(d->accessory_power >= 0.0 && d->accessory_power <= 2e4)) return -1;
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
    e->acc_friction = e->acc_tail_m = e->acc_tail_mT = 0.0;
}

/* Sets up a pipe with its cells from the engine's pool: about one every
 * `cell` metres (rb_pipe_cells_for), or when the pool can't give every pipe
 * that many, one and the share room / over of the rest (whole numbers, so
 * the shares never add up to more than the pool). */
static void new_pipe(rb_engine *e, rb_pipe *p, int *used, double length, double diameter,
                     double cell, int room, int over, double wall, double friction)
{
    const int n = 1 + (rb_pipe_cells_for(length, cell, e->pipe_dt) - 1) * room / over;
    rb_pipe_init(p, &e->cell_pool[*used], &e->face_pool[*used], n, length, diameter,
                 e->desc.p_ambient, e->desc.T_ambient, AIR, wall, friction);
    *used += n;
}

/* The system (muffler and tailpipe) collector c's exhaust pipe runs into. */
static int system_of(const rb_engine *e, int c)
{
    return (e->systems == 2) ? c : 0;
}

/* No two cycles alike (stage 12c). In a real engine each burn starts and
 * runs its own way: the turbulence and the mixture at the plug when it fires
 * set how soon the flame gets going and how fast it then spreads (Heywood,
 * section 9.4; Ozdor, Dulger and Sher, SAE 940987). Its first 10 % varies
 * most, and a slow start mostly means a slow burn. The slower and more
 * diluted the burn (idling, a light load), the more crank degrees it varies
 * by: the cylinders' work varies by 1-3 % of the mean (the COV of IMEP) wide
 * open, and 4-8 % idling, where the spark is held back from its best and the
 * work changes faster with the burn's timing. So each cycle, at intake valve
 * closing, the burn's start moves from the spark by VARY_START of the burn's
 * length, and its length changes by VARY_LENGTH, both times `variation`, from
 * two normal numbers VARY_ALIKE alike. The engine computer times the spark
 * for the burn it expects, not this one.
 * The cylinders also differ a little, for good: the injectors and the ports'
 * air give each its own mixture (CYL_FUEL) and swirl, so its own burn length
 * (CYL_BURN); the oxygen sensor keeps their mean on the target, and on
 * average they burn as one. And the exhaust valves' jets are turbulent: the
 * gas through each flickers by FLICKER (RMS) below FLICKER_HZ, a rasp the
 * pipes carry on. Calibrated in docs/stage-12c.md. */
#define VARY_START   0.07      /* of the burn's length, RMS                        */
#define VARY_LENGTH  0.06      /* relative, RMS                                    */
#define VARY_ALIKE   0.5       /* the correlation of the two                       */
#define VARY_MOST    3.0       /* no normal number goes further than this          */
#define VARY_TOP     4.0       /* the most `variation` counts                      */
#define CYL_FUEL     0.02      /* RMS                                              */
#define CYL_BURN     0.04      /* RMS                                              */
#define FLICKER      0.15      /* RMS: as real engines, the sound between 0.5 and
                                  2 kHz then changes more than it repeats from
                                  one cycle to the next (docs/stage-12c.md)        */
#define FLICKER_HZ   2000.0
#define RB_ENGINE_SEED  0x5EED12C0FFEEULL

/* The variation to use: 0 for a NaN or anything negative, at most VARY_TOP. */
static double variation(const rb_engine *e)
{
    const double v = e->variation;
    return (v > 0.0) ? ((v < VARY_TOP) ? v : VARY_TOP) : 0.0;
}

/* A normal number, kept within VARY_MOST. */
static double vary_normal(uint64_t *state)
{
    const double z = rb_rand_normal(state);
    return (z > VARY_MOST) ? VARY_MOST : (z < -VARY_MOST) ? -VARY_MOST : z;
}

/* The generators, and each cylinder's own mixture and burn: drawn whatever
 * `variation` is, so turning it on later finds them there. Their mean over
 * the cylinders is 0 (a single cylinder has none). */
static void vary_init(rb_engine *e)
{
    const int n = e->desc.cylinders;
    double fuel = 0.0, burn = 0.0;
    e->rand_cycles = RB_ENGINE_SEED;
    e->rand_flicker = RB_ENGINE_SEED ^ 0xF11C4E5ULL;
    for (int i = 0; i < n; i++) {
        e->trim_fuel[i] = vary_normal(&e->rand_cycles);
        e->trim_burn[i] = vary_normal(&e->rand_cycles);
        fuel += e->trim_fuel[i] / n;
        burn += e->trim_burn[i] / n;
    }
    for (int i = 0; i < n; i++) {
        e->trim_fuel[i] -= fuel;
        e->trim_burn[i] -= burn;
    }
    e->flicker_a = 1.0 - exp(-2.0 * PI * FLICKER_HZ * e->dt);
    e->flicker_norm = sqrt((2.0 - e->flicker_a) / e->flicker_a);   /* a one-pole's RMS */
}

int rb_engine_init(rb_engine *e, const rb_engine_desc *d, double dt)
{
    return rb_engine_init_at(e, d, dt, RB_PIPE_RATE);
}

int rb_engine_init_at(rb_engine *e, const rb_engine_desc *d, double dt, double pipe_rate)
{
    if (rb_engine_desc_check(d) != 0 || !(dt > 0.0 && dt < 1e-3)
        || !(pipe_rate > 0.0 && pipe_rate < INFINITY))
        return -1;

    memset(e, 0, sizeof *e);
    e->desc = *d;
    e->dt = dt;
    e->hold_speed = 1;
    e->throttle_pos = 1.0;
    e->ignition = 0;
    /* An illustrative ECU transition, not a measured calibration for the
     * reference cars. See docs/stage-S1e.md; zero reproduces immediate cut. */
    e->overrun_transition = 0.15;
    e->overrun_retard = 12.0;
    e->afr = RB_AFR_STOICH;
    e->spark = RB_SPARK_DEFAULT;
    e->spark_map = 1;
    e->throttle_area_pos = NAN;   /* not computed yet */
    rb_gas_init(&e->fuel_gas, FUEL);
    e->pipe_every = (int)lround(1.0 / (pipe_rate * dt));
    if (e->pipe_every < 1) e->pipe_every = 1;
    e->pipe_dt = e->pipe_every * dt;
    for (int i = 0; i < d->cylinders; i++)
        if (d->collector[i] + 1 > e->collectors) e->collectors = d->collector[i] + 1;
    e->systems = d->exhaust_systems;

    rb_cyl_geom g;
    rb_cyl_geom_init(&g, d->bore, d->stroke, d->rod, d->cr);
    rb_chamber_init_reservoir(&e->intake_air, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init_reservoir(&e->exhaust_air, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init(&e->airbox, d->airbox_volume, d->p_ambient, d->T_ambient, AIR);
    rb_chamber_init(&e->plenum, d->plenum_volume, d->p_ambient, d->T_ambient, AIR);
    rb_orifice_init(&e->throttle, &e->airbox, &e->plenum, 0.0, 1.0);
    rb_orifice_init(&e->idle_air, &e->airbox, &e->plenum, 0.0, 1.0);

    /* Optional ultra quality doubles the spatial resolution with the pipe
     * rate. The pipe CFL limiter and fixed pool still bound every build. */
    const double grid_scale = fmin(1.0, RB_PIPE_RATE * e->pipe_dt);
    const double short_cell = SHORT_CELL * grid_scale;
    const double long_cell = LONG_CELL * grid_scale;
    /* The pipes' cells. If they'd want more than the pool has, each still
     * gets one, and a share of the rest of what it wants (new_pipe): never
     * more than the pool, however many short pipes there are (at most 39). */
    const int runners = d->runner_length > 0.0;
    const int pipes = rb_engine_pipe_count(e);
    int want = rb_pipe_cells_for(d->snorkel_length, short_cell, e->pipe_dt), used = 0;
    for (int i = 0; i < d->cylinders; i++)
        want += rb_pipe_cells_for(d->header_length[i], short_cell, e->pipe_dt)
              + (runners ? rb_pipe_cells_for(d->runner_length, short_cell, e->pipe_dt) : 0);
    want += e->collectors * rb_pipe_cells_for(d->exhaust_pipe_length, long_cell, e->pipe_dt)
          + e->systems * (rb_pipe_cells_for(d->muffler_length, long_cell, e->pipe_dt)
                          + rb_pipe_cells_for(d->tailpipe_length, long_cell, e->pipe_dt));
    if (d->center_length > 0.0)
        want += e->collectors * (rb_pipe_cells_for(d->center_length, long_cell, e->pipe_dt)
                              + rb_pipe_cells_for(d->link_length, long_cell, e->pipe_dt));
    const int room = (want > RB_ENGINE_CELLS) ? RB_ENGINE_CELLS - pipes : 1;
    const int over = (want > RB_ENGINE_CELLS) ? want - pipes : 1;

    /* The way in (stage 12): the snorkel draws from the open air through its
     * mouth and opens into the air box through the filter at its end, a
     * laminar loss. The air box is stepped with the plenum and the throttle
     * every physics step, and so is the snorkel's face into it. */
    new_pipe(e, &e->snorkel, &used, d->snorkel_length, d->snorkel_diameter, short_cell, room, over,
             d->T_ambient, RUNNER_FRICTION);
    rb_pipe_join(&e->snorkel, 0, &e->intake_air, SNORKEL_ENTRY, 0);
    rb_pipe_join(&e->snorkel, 1, &e->airbox, 0.0, 1);
    rb_pipe_resist(&e->snorkel, 1, FILTER_LAMINAR);

    /* Each collector merges its headers into its exhaust pipe through a cone
     * about two pipe diameters long. */
    const double header_area = 0.25 * PI * d->header_diameter * d->header_diameter;
    const double pipe_area = 0.25 * PI * d->exhaust_pipe_diameter * d->exhaust_pipe_diameter;
    for (int c = 0; c < e->collectors; c++) {
        int headers = 0;
        for (int i = 0; i < d->cylinders; i++) headers += (d->collector[i] == c);
        const double volume = d->exhaust_pipe_diameter * (headers * header_area + pipe_area);
        rb_chamber_init(&e->collector[c], volume, d->p_ambient, d->T_ambient, AIR);
    }

    /* The areas of the valves, catalysts and mufflers are effective areas
     * (Cd included), so cd = 1. */
    for (int i = 0; i < d->cylinders; i++) {
        const double theta = rb_engine_cylinder_angle(e, i) * PI / 180.0;
        rb_cylinder_init(&e->cyl[i], &g, theta, d->p_ambient, d->T_ambient, AIR);
        rb_chamber *gas = &e->cyl[i].gas;
        /* The runner: from the plenum to the intake valve, its last cell at
         * the valve stepped with the cylinder. */
        if (runners) {
            rb_pipe *r = &e->runner[i];
            new_pipe(e, r, &used, d->runner_length, d->runner_diameter, short_cell, room, over,
                     RUNNER_WALL, RUNNER_FRICTION);
            rb_pipe_join(r, 0, &e->plenum, RUNNER_ENTRY, 1);
            rb_orifice_init(&e->intake[i], rb_pipe_end_cell(r, 1), gas, 0.0, 1.0);
            rb_pipe_attach(r, 1, gas, &e->intake[i]);
            rb_pipe_set_fast(r, 1);
        } else {
            rb_orifice_init(&e->intake[i], &e->plenum, gas, 0.0, 1.0);
        }
        /* The header: from the exhaust valve, its first cell stepped with the
         * cylinder, to its collector. */
        rb_pipe *h = &e->header[i];
        new_pipe(e, h, &used, d->header_length[i], d->header_diameter, short_cell, room, over,
                 HEADER_WALL, EXHAUST_FRICTION);
        rb_orifice_init(&e->exhaust[i], gas, rb_pipe_end_cell(h, 0), 0.0, 1.0);
        rb_pipe_attach(h, 0, gas, &e->exhaust[i]);
        rb_pipe_set_fast(h, 0);
        rb_pipe_join(h, 1, &e->collector[d->collector[i]], HEADER_ENTRY, 0);
        e->burn[i].spark_angle = e->burn[i].start_angle = 720.0 - RB_SPARK_DEFAULT;
        e->burn[i].duration = d->burn_duration;
    }
    /* Each system's muffler (stage 12; rb.h: rb_muffler_type) is a pipe:
     * none, a straight one the tailpipe's size; straight-through, that pipe
     * packed (pipe.h); chambered, a wide can. */
    for (int s = 0; s < e->systems; s++) {
        rb_pipe *m = &e->muffler[s];
        new_pipe(e, m, &used, d->muffler_length, muffler_width(d), long_cell, room, over,
                 MUFFLER_WALL, EXHAUST_FRICTION);
        if (d->muffler_type == RB_MUFFLER_STRAIGHT) rb_pipe_pack(m, PACKING, PACKING_SETTLE);
    }
    /* Each collector's exhaust pipe runs through the catalyst at its end
     * into its system's muffler, and each muffler into its tailpipe, which
     * opens into the open air. Into a chambered muffler's can the gas spreads
     * out, losing its speed, and leaves it through the baffles: the pipes are
     * joined to the can's end cells as to volumes. A straight pipe or a
     * straight-through goes straight on from its exhaust pipe (unless two
     * exhaust pipes meet there: then they open into its first cell) and on
     * into its tailpipe (pipe.h). The catalyst and the baffles are
     * restrictions with a laminar loss at the pipes' ends. */
    const int straight = d->muffler_type != RB_MUFFLER_CHAMBERED;
    const int alone = e->systems == e->collectors;   /* each muffler has one exhaust pipe */
    const int extended = d->center_length > 0.0;
    if (d->junction_volume > 0.0)
        rb_chamber_init(&e->junction, d->junction_volume, d->p_ambient, d->T_ambient, AIR);
    for (int c = 0; c < e->collectors; c++) {
        rb_pipe *x = &e->exhaust_pipe[c], *m = &e->muffler[system_of(e, c)];
        new_pipe(e, x, &used, d->exhaust_pipe_length, d->exhaust_pipe_diameter, long_cell, room, over,
                 EXHAUST_WALL, EXHAUST_FRICTION);
        rb_pipe_join(x, 0, &e->collector[c], COLLECTOR_ENTRY, 0);
        if (extended) {
            rb_pipe *z = &e->center[c], *l = &e->link[c];
            const double width = d->center_volume > 0.0
                ? sqrt(4.0 * d->center_volume / (PI * d->center_length)) : d->center_diameter;
            new_pipe(e, z, &used, d->center_length, width, long_cell, room, over,
                     EXHAUST_WALL, EXHAUST_FRICTION);
            new_pipe(e, l, &used, d->link_length, d->exhaust_pipe_diameter, long_cell, room, over,
                     EXHAUST_WALL, EXHAUST_FRICTION);
            if (d->center_packing > 0.0) rb_pipe_pack(z, d->center_packing, PACKING_SETTLE);
            if (d->junction_before_center) {
                rb_pipe_join(x, 1, &e->junction, 0.0, 0);
                rb_pipe_join(z, 0, &e->junction, 0.0, 0);
            } else if (d->center_volume > 0.0 || rb_pipe_onward(x, z) != 0)
                rb_pipe_join(x, 1, rb_pipe_end_cell(z, 0), 0.0, 0);
            if (d->center_volume > 0.0 || rb_pipe_onward(z, l) != 0)
                rb_pipe_join(l, 0, rb_pipe_end_cell(z, 1), 0.0, 0);
            if (d->junction_volume > 0.0 && !d->junction_before_center)
                rb_pipe_join(l, 1, &e->junction, 0.0, 0);
            else if (!(straight && alone && rb_pipe_onward(l, m) == 0))
                rb_pipe_join(l, 1, rb_pipe_end_cell(m, 0), 0.0, 0);
        } else if (!(straight && alone && rb_pipe_onward(x, m) == 0))
            rb_pipe_join(x, 1, rb_pipe_end_cell(m, 0), 0.0, 0);
        rb_pipe_restrict(x, 1, d->catalyst_area);
        rb_pipe_resist(x, 1, CATALYST_LAMINAR);
    }
    for (int s = 0; s < e->systems; s++) {
        rb_pipe *t = &e->tailpipe[s], *m = &e->muffler[s];
        if (d->junction_volume > 0.0 && !d->junction_before_center)
            rb_pipe_join(m, 0, &e->junction, 0.0, 0);
        new_pipe(e, t, &used, d->tailpipe_length, d->tailpipe_diameter, long_cell, room, over,
                 TAILPIPE_WALL, EXHAUST_FRICTION);
        if (!(straight && rb_pipe_onward(m, t) == 0)) {
            rb_pipe_join(t, 0, rb_pipe_end_cell(m, 1), 0.0, 0);
            if (!straight) {
                rb_pipe_restrict(t, 0, d->muffler_area);
                rb_pipe_resist(t, 0, BAFFLE_LAMINAR);
            }
        }
        rb_pipe_join(t, 1, &e->exhaust_air, TAILPIPE_ENTRY, 0);
        if (d->outlet_radiation)
            rb_pipe_radiate(t, 1, e->exhaust_air.rho, rb_chamber_sound_speed(&e->exhaust_air));
    }
    vary_init(e);
    start_cycle(e);
    return 0;
}

int rb_engine_pipe_count(const rb_engine *e)
{
    const int n = e->desc.cylinders;
    return (e->desc.runner_length > 0.0 ? n : 0) + n + e->collectors + 2 * e->systems + 1
         + (e->desc.center_length > 0.0 ? 2 * e->collectors : 0);
}

const rb_pipe *rb_engine_pipe(const rb_engine *e, int k)
{
    const int n = e->desc.cylinders;
    if (k < 0) return NULL;
    if (e->desc.runner_length > 0.0) {
        if (k < n) return &e->runner[k];
        k -= n;
    }
    if (k < n) return &e->header[k];
    k -= n;
    if (k < e->collectors) return &e->exhaust_pipe[k];
    k -= e->collectors;
    if (k < e->systems) return &e->tailpipe[k];
    k -= e->systems;
    if (k < e->systems) return &e->muffler[k];
    k -= e->systems;
    if (k == 0) return &e->snorkel;
    k--;
    if (e->desc.center_length > 0.0) {
        if (k < e->collectors) return &e->center[k];
        k -= e->collectors;
        if (k < e->collectors) return &e->link[k];
    }
    return NULL;
}

void rb_engine_contents(const rb_engine *e, double *mass, double *energy,
                        double species[RB_NSPECIES])
{
    double m = 0.0, u = 0.0, y[RB_NSPECIES] = { 0.0 };
    const rb_chamber *volumes[RB_MAX_CYLINDERS + 3 + RB_MAX_COLLECTORS];
    int count = 0;
    volumes[count++] = &e->airbox;
    volumes[count++] = &e->plenum;
    for (int i = 0; i < e->desc.cylinders; i++) volumes[count++] = &e->cyl[i].gas;
    for (int c = 0; c < e->collectors; c++) volumes[count++] = &e->collector[c];
    if (e->desc.junction_volume > 0.0) volumes[count++] = &e->junction;
    for (int k = 0; k < count; k++) {
        m += volumes[k]->mass;
        u += volumes[k]->U;
        for (int j = 0; j < RB_NSPECIES; j++) y[j] += volumes[k]->m[j];
    }
    for (int k = 0; k < rb_engine_pipe_count(e); k++) {
        double pm, pu, py[RB_NSPECIES];
        rb_pipe_contents(rb_engine_pipe(e, k), &pm, &pu, py);
        m += pm;
        u += pu;
        for (int j = 0; j < RB_NSPECIES; j++) y[j] += py[j];
    }
    if (mass) *mass = m;
    if (energy) *energy = u;
    if (species)
        for (int j = 0; j < RB_NSPECIES; j++) species[j] = y[j];
}

double rb_engine_pipe_heat(const rb_engine *e)
{
    double q = 0.0;
    for (int k = 0; k < rb_engine_pipe_count(e); k++) q += rb_engine_pipe(e, k)->heat;
    return q;
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
    start_cycle(e);
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

/* The spark advance to use (degrees before firing TDC): the setting, kept
 * within 90 degrees of TDC so the spark always falls between the valves'
 * events (NaN counts as 0). */
static double spark_advance(double s)
{
    if (s > 90.0) return 90.0;
    if (s < -90.0) return -90.0;
    return (s == s) ? s : 0.0;
}

/* How the burn's length changes with speed and the charge (stage 9). Two
 * things carry the flame through the cylinder (Heywood, chapter 9; the
 * entrainment-and-burn-up picture of Blizard, Keck and Tabaczynski):
 *   - the turbulent flame front is swept along by the charge's turbulence,
 *     which grows with piston speed, so that part of the burn takes about
 *     the same crank angle at any speed;
 *   - each eddy it swallows then burns up from the inside at the laminar
 *     flame speed S_L, taking about lambda / S_L, where the eddies' Taylor
 *     microscale lambda goes as sqrt(nu L / u'): longer when the charge is
 *     thin (low density, high kinematic viscosity nu) or diluted with burned
 *     gas, which slows S_L. In crank degrees that part goes as
 *     sqrt(rpm nu) / S_L.
 * At the reference (3000 rpm, a wide-open charge) the second part is 40 % of
 * the burn, which makes the burn's crank angle grow as about rpm^0.2 there
 * (with the same charge): slowly, as real engines' burn angles do (Heywood).
 * S_L is gasoline's, slowed by burned gas (Metghalchi and Keck's fit, Turns
 * eq. 8.33); nu = mu / rho with mu ~ T^0.7. The charge is taken at intake
 * valve closing: compressed to the spark, its ratios to the reference stay
 * about the same. The deterministic calibration keeps the burn between half
 * and twice the reference. With life enabled (S1b), keep the raw demand:
 * EVO truncates a burn that cannot finish. This is a finite-time partial
 * burn model; it does not model spark-kernel failure. */
#define BURN_REF_RPM    3000.0
#define BURN_REF_T      360.0     /* K, at IVC: the default four at 3000 rpm, wide open */
#define BURN_REF_P      1.22e5    /* Pa                                                */
#define BURN_REF_X      0.04      /* burned-gas share                                  */
#define BURN_EDDY_SHARE 0.4       /* the burn-up part at the reference                 */
#define MBT_CA50        8.0       /* deg after TDC: where the computer puts the middle
                                     of the burn (best torque, SAE 852090: 7-8)        */

/* Gasoline-air laminar flame speed (cm/s) at T (K), p (Pa), burned-gas mass
 * share x and equivalence ratio phi. */
static double flame_speed(double T, double p, double x, double phi)
{
    double s0 = 27.58 - 78.34 * (phi - 1.13) * (phi - 1.13);
    if (s0 < 2.0) s0 = 2.0;                        /* far outside the fit's range */
    double dilution = 1.0 - 2.1 * x;
    if (dilution < 0.1) dilution = 0.1;
    else if (dilution > 1.0) dilution = 1.0;
    return s0 * pow(T / 298.0, 2.18 - 0.8 * (phi - 1.0))
              * pow(p / 101325.0, -0.16 + 0.22 * (phi - 1.0)) * dilution;
}

/* This cycle's burn, crank degrees from the spark to the end, for cylinder
 * i's trapped charge at `rpm`. */
static double burn_duration(const rb_engine *e, rb_burn *b, double rpm)
{
    const double target_phi = (e->afr > 0.0) ? RB_AFR_STOICH / e->afr : 1.0;
    /* Life follows the mixture actually trapped, including cylinder trims
     * and fuel left by earlier weak burns. Keep life-off's calibrated path.
     * Without both reactants no flame can release heat; use its old finite
     * reference duration rather than interpreting a missing ratio as a
     * measured flammability limit. */
    const double phi = (variation(e) > 0.0 && b->phi_ivc > 0.0) ? b->phi_ivc : target_phi;
    const double s_l = flame_speed(b->T_ivc, b->p_ivc, b->residual, phi)
                     / flame_speed(BURN_REF_T, BURN_REF_P, BURN_REF_X, 1.0);
    const double nu = pow(b->T_ivc / BURN_REF_T, 1.7) * (BURN_REF_P / b->p_ivc);
    const double n = ((rpm > RB_GAS_PACE_RPM) ? rpm : RB_GAS_PACE_RPM) / BURN_REF_RPM;
    double f = (1.0 - BURN_EDDY_SHARE) + BURN_EDDY_SHARE * sqrt(n * nu) / s_l;
    if (!(f >= 0.5)) f = (f < 0.5) ? 0.5 : 1.0;     /* NaN: the reference */
    /* Keep the charge model's demand before the old calibration limits.
     * With life enabled a dilute/lean charge can take longer than the time
     * until EVO: the existing valve event then leaves the rest unburned.
     * One cycle is a numerical ceiling, not a claimed misfire threshold. */
    const double demand = e->desc.burn_duration * f;
    b->demand_duration = (demand < 720.0) ? demand : 720.0;
    if (f > 2.0) f = 2.0;
    const double duration = e->desc.burn_duration * f;
    return (duration < 180.0) ? duration : 180.0;
}

/* The spark advance for the best torque: the middle of the burn (where the
 * Wiebe curve reaches 1/2) at MBT_CA50 after TDC. */
static double mbt_advance(const rb_engine *e, double duration)
{
    const rb_engine_desc *d = &e->desc;
    return duration * pow(log(2.0) / d->wiebe_a, 1.0 / (d->wiebe_m + 1.0)) - MBT_CA50;
}

/* At idle the engine computer acts on the speed the engine is heading for:
 * IDLE_LOOKAHEAD on, at the rate it's changing (rpm_smooth - rpm_recent is
 * about how much it changed over the last RPM_RECENT seconds). */
#define RPM_RECENT      0.15      /* s: the speed's smoothing for its rate      */
#define IDLE_LOOKAHEAD  0.5       /* s: how far ahead the idle control looks    */

static double idle_heading(const rb_engine *e)
{
    return e->rpm_smooth + IDLE_LOOKAHEAD / RPM_RECENT * (e->rpm_smooth - e->rpm_recent);
}

/* At idle the computer holds the spark back from its best, a torque reserve,
 * and moves it with the speed the engine is heading for: advanced when it's
 * heading below its idle speed, retarded when above. A spark change acts
 * within a cycle, far sooner than the idle valve's air reaches the cylinders,
 * so the spark steadies the idle while the valve does the slow work (stage 9).
 * It needs to be strong: with the runners (stage 10), an engine idling a few
 * rpm faster pushes less exhaust back into its runners at each overlap (the
 * overlap is over sooner), so it breathes better and makes more torque before
 * the plenum pressure can follow. The six, on the stage 9 spark of 0.1 deg per
 * rpm and with its idle valve held still, swung about 95 rpm every 2.8 s
 * (stage 9's six held steady with its spark held still too). It idles this
 * way with the foot off, running with its crank free, and within
 * IDLE_SPARK_BAND of its idle speed. */
#define IDLE_SPARK      15.0      /* deg before TDC at the idle speed            */
#define IDLE_SPARK_GAIN 0.3       /* deg more per rpm below it                   */
#define IDLE_SPARK_BAND 400.0     /* rpm above the idle speed where idling ends  */

/* The spark advance the engine computer chooses for a cycle. */
static double computer_spark(const rb_engine *e, double duration, double rpm)
{
    const rb_engine_desc *d = &e->desc;
    if (!e->hold_speed && rpm < d->cranking_rpm) return d->cranking_spark;
    if (!e->spark_map) return e->spark;
    const double best = mbt_advance(e, duration);
    if (!e->hold_speed && d->idle_rpm > 0.0 && e->throttle_pos < 0.01
        && e->rpm_smooth < d->idle_rpm + IDLE_SPARK_BAND) {
        const double s = IDLE_SPARK + IDLE_SPARK_GAIN * (d->idle_rpm - idle_heading(e));
        return (s < 0.0) ? 0.0 : (s < best) ? s : best;
    }
    /* Overrun entry removes torque through ignition before cutting fuel.
     * Scheduling still happens once at IVC, never by reshaping the audio. */
    const double phase = e->overrun_transition > 0.0
                       ? fmin(e->overrun_time / e->overrun_transition, 1.0) : 0.0;
    return best - e->overrun_retard * phase;
}

/* Intake valve closing: the charge is trapped. Inject the fuel for the
 * trapped air, as vapour at the charge's temperature, then note the trapped
 * charge, fuel and all: Woschni's reference for the motored pressure, and the
 * share of it that is old exhaust (the residual fraction, as Heywood counts it).
 * This cycle's burn length and spark timing are fixed here too, as an engine
 * computer would: a change of setting can't move a burn already under way,
 * or fire a cylinder twice. */
static void close_intake(rb_engine *e, int i)
{
    rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    const double v = variation(e);
    const double fuel = (e->ignition && e->afr > 0.0 && !e->fuel_cut)
                      ? g->m[RB_AIR] / e->afr * (1.0 + v * CYL_FUEL * e->trim_fuel[i]) - g->m[RB_FUEL]
                      : 0.0;
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
    const double phi = (g->m[RB_AIR] > 0.0 && g->m[RB_FUEL] > 0.0)
                     ? RB_AFR_STOICH * (g->m[RB_FUEL] / g->m[RB_AIR]) : 0.0;
    b->phi_ivc = isfinite(phi) ? phi : 0.0;
    const double rpm = e->hold_speed ? e->rpm : e->omega * 30.0 / PI;
    const double expected = burn_duration(e, b, rpm);
    const double advance = spark_advance(computer_spark(e, expected, rpm));
    b->spark_angle = cycle_angle(720.0 - advance);

    /* This cycle's own burn (stage 12c): its start and length, and the
     * cylinder's own length. */
    const double start = vary_normal(&e->rand_cycles);
    const double length = VARY_ALIKE * start
                        + sqrt(1.0 - VARY_ALIKE * VARY_ALIKE) * vary_normal(&e->rand_cycles);
    const double base = (v > 0.0) ? b->demand_duration : expected;
    double duration = base * exp(v * VARY_LENGTH * length) * (1.0 + v * CYL_BURN * e->trim_burn[i]);
    if (!(duration >= 0.25 * base)) duration = 0.25 * base;
    const double ceiling = (v > 0.0) ? 720.0 : 180.0;
    b->duration = (duration < ceiling) ? duration : ceiling;
    b->start_angle = cycle_angle(720.0 - spark_advance(advance - v * VARY_START * expected * start));
    b->spark_due = 1;
}

/* The rev limiter's soft cut (stage 12c): within LIMIT_SOFT rpm below the
 * rev limit the engine computer skips single sparks at random, more of them
 * the nearer the limit (a share that grows from none to all across the
 * band), as a racing ECU's soft cut does: held there in gear the engine
 * stutters at the limit, firing on some of its cylinders. Above the limit
 * the hard cut stops every spark (engine_computer). Before stage 12c only
 * the hard cut was there: in gear, with a car's inertia on the crank, it
 * took 0.6 s to fall the 100 rpm back to where it fires again; real engines
 * bounce off their limiters 10-30 times a second. */
#define LIMIT_SOFT 150.0   /* rpm */

static int soft_cut(rb_engine *e)
{
    if (e->hold_speed) return 0;   /* the engine computer is off while held */
    const double into = (e->omega * 30.0 / PI - (e->desc.rev_limit - LIMIT_SOFT)) / LIMIT_SOFT;
    if (!(into > 0.0)) return 0;
    return rb_rand_uniform(&e->rand_cycles) < into;
}

/* The spark: what can burn is the fuel, or as much of it as the air can burn,
 * times the combustion efficiency. */
static void spark(rb_engine *e, int i)
{
    const rb_chamber *g = &e->cyl[i].gas;
    rb_burn *b = &e->burn[i];
    b->burning = 0;
    if (!e->ignition) return;
    if (e->spark_cut || soft_cut(e)) {   /* the rev limiter cuts the spark */
        e->limiter_cuts++;
        return;
    }
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
    if (b->burn_angle == 0.0) b->burn_angle = cycle_angle(angle + step - b->start_angle);
    else b->burn_angle += (step > pace) ? step : pace;
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

/* The engine computer (free crank only, ignition on), every step: the rev
 * limiter, the fuel cut on overrun, and the idle air valve holding the idle
 * speed. They look at the crank speed smoothed over a few firing pulses.
 * Coasting with the foot off the pedal more than 700 rpm above idle, the
 * injectors stop until the engine is back within 400 rpm of idle: no fuel is
 * wasted, and the engine slows down faster. The idle valve opens in
 * proportion to how far below its idle speed the engine is heading
 * (idle_gain_p): the speed half a second on, at the rate it's changing. The
 * air takes about that long to fill the plenum and runners (4 L in the four),
 * so falling towards idle the valve opens before the speed gets there; looking
 * at the speed now, the four dipped 150 rpm below idle after a blip, the
 * plenum still emptied from the fuel cut (stage 10). The rate is the speed's
 * change over the last 0.15 s: over the 0.5 s of the trend, a long coast down
 * from the rev limiter still looked steep when the fuel came back on, and held
 * the valve wide open above idle. All that around a position the valve learns
 * while the engine idles (idle_gain_i): running,
 * foot off the pedal, and its speed steady (the speed smoothed over 0.05 s
 * within 50 rpm of the speed smoothed over 0.5 s). The learning is slow and
 * counts at most 100 rpm of error, so coasting down from high rpm doesn't
 * unlearn the idle position. Below the idle speed it learns even while the
 * speed is still changing: sagging below idle always calls for more air, and
 * waiting for the speed to steady let a load thrown on at idle swing the
 * engine around for seconds before it learned anything (stage 9). */
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
    e->rpm_recent += (e->rpm_smooth - e->rpm_recent) * (e->dt / RPM_RECENT);
    if (!e->ignition) {            /* switched off: the valve stays where it is */
        e->spark_cut = e->fuel_cut = 0;
        e->overrun_time = 0.0;
        e->overrun_armed = 0;
        return;
    }

    if (rpm > d->rev_limit) e->spark_cut = 1;   /* the hard cut (the soft one: soft_cut) */
    else if (rpm < d->rev_limit_resume) e->spark_cut = 0;

    if (!(d->idle_rpm > 0.0)) {
        e->idle_valve = 0.0;
        e->fuel_cut = 0;
        e->overrun_time = 0.0;
        return;
    }
    const int foot_off = e->throttle_pos < 0.01;   /* NaN counts as on */
    /* This transition belongs to a pedal lift, not the key-start transient.
     * Retain startup's existing overspeed protection until the pedal is used. */
    if (!foot_off) e->overrun_armed = 1;
    else if (e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) e->overrun_armed = 0;
    if (!foot_off || e->rpm_smooth < d->idle_rpm + FUEL_BACK_ABOVE) {
        e->fuel_cut = 0;
        e->overrun_time = 0.0;
    } else if (e->rpm_smooth > d->idle_rpm + FUEL_CUT_ABOVE
               || e->overrun_time > 0.0 || e->fuel_cut) {
        const double transition = e->overrun_armed && e->overrun_transition > 0.0
                                && isfinite(e->overrun_transition)
                                ? e->overrun_transition : 0.0;
        e->overrun_time = fmin(e->overrun_time + e->dt, transition);
        e->fuel_cut = e->overrun_time >= transition;
    }

    const double error = d->idle_rpm - e->rpm_smooth;
    if (e->rpm_smooth > d->cranking_rpm && foot_off
        && (fabs(e->rpm_smooth - e->rpm_trend) < IDLE_STEADY || error > 0.0)) {
        const double counted = (error > IDLE_LEARN_MAX) ? IDLE_LEARN_MAX
                             : (error < -IDLE_LEARN_MAX) ? -IDLE_LEARN_MAX : error;
        e->idle_trim += d->idle_gain_i * counted * e->dt;
        if (e->idle_trim < -IDLE_VALVE_BASE) e->idle_trim = -IDLE_VALVE_BASE;
        if (e->idle_trim > 1.0 - IDLE_VALVE_BASE) e->idle_trim = 1.0 - IDLE_VALVE_BASE;
    }
    const double v = IDLE_VALVE_BASE + e->idle_trim + d->idle_gain_p * (d->idle_rpm - idle_heading(e));
    e->idle_valve = (v > 0.0) ? ((v < 1.0) ? v : 1.0) : 0.0;
}

/* Mean pressure in the exhaust collectors. */
static double exhaust_pressure(const rb_engine *e)
{
    double p = 0.0;
    for (int c = 0; c < e->collectors; c++) p += e->collector[c].p;
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
    for (int c = 0; c < e->collectors; c++) rb_chamber_clear_rates(&e->collector[c]);
    if (e->desc.junction_volume > 0.0) rb_chamber_clear_rates(&e->junction);
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
        const rb_face *out = &e->tailpipe[s].end_face[1];
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
    for (int c = 0; c < e->collectors; c++) rb_chamber_integrate(&e->collector[c], dt);
    if (e->desc.junction_volume > 0.0) rb_chamber_integrate(&e->junction, dt);
    for (int k = 0; k < pipes; k++) rb_pipe_integrate((rb_pipe *)rb_engine_pipe(e, k), 0, dt);
    for (int k = 0; k < pipes; k++) rb_pipe_accelerate((rb_pipe *)rb_engine_pipe(e, k), 0, dt);
}

void rb_engine_step(rb_engine *e)
{
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
            b->last_work = b->work;
            b->work = 0.0;
        }
        if (passes(angle, step_deg, ivc)) close_intake(e, i);
        if (b->spark_due && passes(angle, step_deg, b->start_angle)) {
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
