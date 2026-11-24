/*
 * engine_build.c - the engine's description (its defaults, layout and checks)
 * and building the engine from it: its cylinders, valves and pipes, the
 * variation it starts with, and what the books read of it (see engine.h).
 * Moved from engine.c in stage 130, line for line.
 */
#include "engine_parts.h"

#include "combustion.h"
#include "heat.h"
#include "rand.h"
#include "diagnostics.h"
#include "ignition.h"
#include "fastmath.h"
#include "rb.h"   /* RB_MUFFLER_* */

#include <math.h>
#include <string.h>

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
    d->intake = rb_valve_make(2, 0.033, 0.85 * 0.033, 0.0095, 250.0, 360.0 + 110.0, 0.7);
    d->exhaust = rb_valve_make(2, 0.028, 0.85 * 0.028, 0.0090, 250.0, 360.0 - 110.0, 0.7);

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
    d->catalyst_laminar = CATALYST_LAMINAR;
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
    d->limiter_band = 150.0;   /* stage 12c's soft cut */
    d->muffler_resolution = 1.0;   /* the exhaust's own cells (stage 050's finer cans: 3) */
    d->limiter_cut = RB_LIMITER_SPARK;
    d->launch_rpm = 4000.0;    /* a 2-step's usual launch speed */
    d->launch_gap = 100.0;
    d->launch_band = 0.0;      /* hard: the 2-step's stutter */
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
    /* Each collector's pipe at most 6 times the area of the headers joining it (stage 131):
     * real collectors are 1 to 2 times their primaries. Past it a twin's pipes lost their state
     * at the slow pipe rates (fuzz seeds 131019, 132032, 132055: 6.6 to 8.6 times); the
     * presets reach 5.7 at most within the editor's ranges. */
    for (int c = 0; c < RB_MAX_COLLECTORS; c++)
        if (joined[c] && !(d->exhaust_pipe_diameter * d->exhaust_pipe_diameter
                           <= 6.0 * joined[c] * d->header_diameter * d->header_diameter)) return -1;
    if (!(d->exhaust_systems == 1 || (d->exhaust_systems == 2
        && (joined[1] || d->junction_volume > 0.0)))) return -1;
    if (!(d->center_length >= 0.0 && d->center_length <= 3.0)
        || !(d->link_length >= 0.0 && d->link_length <= 3.0)
        || !(d->junction_volume >= 0.0 && d->junction_volume <= 0.02)) return -1;
    if (d->junction_before_center != 0 && d->junction_before_center != 1) return -1;
    if (d->outlet_radiation != 0 && d->outlet_radiation != 1) return -1;
    if (!(d->muffler_resolution >= 1.0 && d->muffler_resolution <= 4.0)) return -1;
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
    if (!(d->catalyst_laminar >= 0.0 && d->catalyst_laminar <= 10000.0)) return -1;
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
    if (!(d->limiter_band >= 0.0 && d->limiter_band <= 1000.0)) return -1;
    if (d->limiter_cut != RB_LIMITER_SPARK && d->limiter_cut != RB_LIMITER_FUEL) return -1;
    if (!(d->launch_rpm >= 1000.0 && d->launch_rpm < 1e5)) return -1;
    if (!(d->launch_gap > 0.0 && d->launch_gap < d->launch_rpm)) return -1;
    if (!(d->launch_band >= 0.0 && d->launch_band <= 1000.0)) return -1;
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
    p->requested_cells = rb_pipe_cells_for(length, cell, e->pipe_dt);
    *used += n;
}

/* The system (muffler and tailpipe) collector c's exhaust pipe runs into. */
static int system_of(const rb_engine *e, int c)
{
    return (e->systems == 2) ? c : 0;
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
static void vary_init(rb_engine *e, uint64_t seed)
{
    const int n = e->desc.cylinders;
    double fuel = 0.0, burn = 0.0;
    e->variation_seed = seed;
    e->rand_cycles = seed;
    e->rand_flicker = seed ^ 0xF11C4E5ULL;
    if (!e->rand_flicker) e->rand_flicker = RB_ENGINE_SEED;
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

int rb_engine_seed(rb_engine *e, uint64_t seed)
{
    if (!e || e->physics_tick || !seed || !(e->dt > 0)) return -1;
    vary_init(e, seed);
    return 0;
}

int rb_engine_combustion_model(rb_engine *e, int model)
{
    if (!e || e->physics_tick || !(e->dt > 0) ||
        (model != RB_BURN_CHARGE && model != RB_BURN_CAPPED))
        return -1;
    e->combustion_model = model;
    return 0;
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
    e->spark_offset = 0.0;
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
    const double can_cell = long_cell / d->muffler_resolution;   /* the mufflers', the centre can's */
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
          + e->systems * (rb_pipe_cells_for(d->muffler_length, can_cell, e->pipe_dt)
                          + rb_pipe_cells_for(d->tailpipe_length, long_cell, e->pipe_dt));
    if (d->center_length > 0.0)
        want += e->collectors * (rb_pipe_cells_for(d->center_length, can_cell, e->pipe_dt)
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
        new_pipe(e, m, &used, d->muffler_length, muffler_width(d), can_cell, room, over,
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
            new_pipe(e, z, &used, d->center_length, width, can_cell, room, over,
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
        rb_pipe_resist(x, 1, d->catalyst_laminar);
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
    vary_init(e, RB_ENGINE_SEED);
    rb_engine_start_cycle(e);
    return 0;
}

int rb_engine_pipe_count(const rb_engine *e)
{
    const int n = e->desc.cylinders;
    return (e->desc.runner_length > 0.0 ? n : 0) + n + e->collectors + 2 * e->systems + 1
         + (e->desc.center_length > 0.0 ? 2 * e->collectors : 0);
}

/* A pipe's faces' guards and its cells' repairs. */
static uint64_t pipe_guards(const rb_pipe *p)
{
    uint64_t n = (uint64_t)p->end_face[0].guards + p->end_face[1].guards;
    for (int k = 0; k + 1 < p->cells; k++) n += p->face[k].guards;
    for (int k = 0; k < p->cells; k++) n += (uint64_t)p->cell[k].repairs;
    return n;
}

uint64_t rb_engine_guards(const rb_engine *e)
{
    uint64_t n = 0;
    const rb_chamber *const fixed[] = { &e->intake_air, &e->plenum, &e->exhaust_air, &e->airbox, &e->junction };
    for (size_t k = 0; k < sizeof fixed / sizeof fixed[0]; k++) n += (uint64_t)fixed[k]->repairs;
    for (int c = 0; c < RB_MAX_COLLECTORS; c++) n += (uint64_t)e->collector[c].repairs;
    /* not the orifices' equalisation limit: by design it acts on about a quarter of the
     * valves' steps in a normal idle and on the throttle wide open (a big opening between
     * small volumes equalises faster than a step), so it tells nothing apart */
    for (int i = 0; i < e->desc.cylinders; i++) n += (uint64_t)e->cyl[i].gas.repairs;
    for (int k = 0; k < rb_engine_pipe_count(e); k++) n += pipe_guards(rb_engine_pipe(e, k));
    return n;
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
