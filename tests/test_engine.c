/*
 * test_engine.c - the whole engine. Motored with adiabatic walls (stage 5):
 * conservation, steady state, the throttle, choked flow, pumping loss, no
 * step-to-step oscillation. Fired (stage 6): conservation of every species
 * and of energy, full-load numbers, spark advance and mixture strength.
 */
#include "combustion.h"
#include "engine.h"
#include "presets.h"
#include "rb.h"   /* RB_MUFFLER_* */
#include "rb_test.h"

#include <math.h>

#define DT (1.0 / 192000.0)   /* the default physics step */

static rb_engine e;           /* about 20 KB: kept off the stack */

/* The default 2.0 L four from rest, motored with adiabatic walls (stage 5),
 * at the given speed and throttle. */
static void start(double rpm, double throttle)
{
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    d.heat_transfer = 0;
    rb_engine_init(&e, &d, DT);
    e.rpm = rpm;
    e.throttle_pos = throttle;
}

/* The same engine running on fuel, walls and all (stage 6). */
static void start_fired(double rpm, double throttle)
{
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    rb_engine_init(&e, &d, DT);
    e.rpm = rpm;
    e.throttle_pos = throttle;
    e.ignition = 1;
}

/* Bounded loops, so that a broken engine fails a test instead of hanging it. */

/* Steps until the engine has finished n more cycles, giving up after 2 s of
 * simulated time per cycle. 1 if it got there. */
static int run_cycles(long n)
{
    const long target = e.cycles + n, most = lround(2.0 * n / DT);
    for (long k = 0; k < most && e.cycles < target; k++) rb_engine_step(&e);
    return e.cycles >= target;
}

/* Steps until cylinder 1's crank angle is in [lo, hi), giving up after 1 s.
 * 1 if it got there. */
static int run_to_angle(double lo, double hi)
{
    for (long k = 0; k < lround(1.0 / DT); k++) {
        const double a = rb_engine_cylinder_angle(&e, 0);
        if (a >= lo && a < hi) return 1;
        rb_engine_step(&e);
    }
    return 0;
}

/* Everything inside the engine: mass, sensible energy (and the pipes' gas's
 * kinetic energy), mass of each species and the count of repaired states. */
typedef struct contents {
    double mass, U, species[RB_NSPECIES];
    int repairs;
} contents;

/* Repairs: the times a chamber's state had to be mended. */
static int repairs(void)
{
    int n = e.airbox.repairs + e.plenum.repairs;
    for (int i = 0; i < e.desc.cylinders; i++) n += e.cyl[i].gas.repairs;
    for (int c = 0; c < e.collectors; c++) n += e.collector[c].repairs;
    for (int k = 0; k < rb_engine_pipe_count(&e); k++) {
        const rb_pipe *p = rb_engine_pipe(&e, k);
        for (int j = 0; j < p->cells; j++) n += p->cell[j].repairs;
    }
    return n;
}

static contents inside(void)
{
    contents c = { 0 };
    rb_engine_contents(&e, &c.mass, &c.U, c.species);
    c.repairs = repairs();
    return c;
}

/* What the engine's books say should have changed: energy brought in by the
 * air and fuel, taken out by the exhaust, added by the pistons, the walls
 * (the cylinders' and the pipes') and the burning. */
static double energy_books(void)
{
    return e.energy_in - e.energy_out + e.work_on_gas + e.wall_heat + rb_engine_pipe_heat(&e)
         + e.heat_released + e.fuel_energy_in;
}

/* The engine's running totals at one moment, to compare two moments. */
typedef struct books {
    contents c;
    double fuel_in, burned, heat, energy, in[RB_NSPECIES], out[RB_NSPECIES];
} books;

static books snapshot(void)
{
    books b = { .c = inside(), .fuel_in = e.fuel_in, .burned = e.fuel_burned,
                .heat = e.heat_released, .energy = energy_books() };
    for (int s = 0; s < RB_NSPECIES; s++) {
        b.in[s] = e.species_in[s];
        b.out[s] = e.species_out[s];
    }
    return b;
}

/* How many species' books don't balance between two moments: what came in,
 * minus what went out, plus what the burning made, is what's inside. */
static int species_unbalanced(const books *a, const books *b)
{
    const double burned = b->burned - a->burned, afr = RB_AFR_STOICH;
    const double made[RB_NSPECIES] = { [RB_AIR] = -afr * burned,
                                       [RB_FUEL] = b->fuel_in - a->fuel_in - burned,
                                       [RB_BURNED] = (1.0 + afr) * burned };
    int wrong = 0;
    for (int s = 0; s < RB_NSPECIES; s++) {
        const double expected = b->in[s] - a->in[s] - (b->out[s] - a->out[s]) + made[s];
        if (fabs(b->c.species[s] - a->c.species[s] - expected) > 1e-12 * a->c.mass) wrong++;
    }
    return wrong;
}

static double ambient_density(void)
{
    return e.desc.p_ambient / (e.intake_air.gas.R * e.desc.T_ambient);
}

void test_engine_description(void)
{
    rb_engine_desc d, bad;
    rb_engine_desc_default(&d);

    CHECK_EQ_INT(rb_engine_desc_check(&d), 0);
    CHECK_EQ_INT(rb_engine_init(&e, &d, DT), 0);
    CHECK_NEAR(rb_engine_displacement(&e), 4 * 499.557e-6, 1e-9);   /* a 2.0 L four */
    CHECK_NEAR(rb_engine_cylinder_angle(&e, 1), 180.0, 1e-12);       /* fires last: 540 after cylinder 1 */
    CHECK_NEAR(e.plenum.p, d.p_ambient, 1e-9 * d.p_ambient);         /* everything starts at ambient */
    CHECK_NEAR(e.cyl[3].gas.T, d.T_ambient, 1e-9);

    bad = d; bad.cylinders = 0;                CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.cylinders = RB_MAX_CYLINDERS + 1; CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.firing[2] = 720.0;            CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.firing[3] = NAN;              CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.rod = 0.05;                   CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.intake.diameter = 0.05;       /* four valves no longer fit in the bore */
    bad.intake.port_diameter = 0.04;           CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.exhaust.duration = 0.0;       CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.throttle.cd = 2.0;            CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.plenum_volume = 0.0;          CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.runner_length = 0.0;          CHECK_EQ_INT(rb_engine_desc_check(&bad), 0);   /* no runners */
    bad = d; bad.runner_length = 0.02;         CHECK_EQ_INT(rb_engine_desc_check(&bad), 0);
    bad = d; bad.runner_length = 0.019;        CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);  /* rings too fast */
    bad = d; bad.runner_length = NAN;          CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.runner_diameter = 0.0;        CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.header_length[2] = 0.04;      CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);  /* under 5 cm */
    bad = d; bad.collector[1] = 2;             CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);  /* two at most */
    bad = d; bad.exhaust_systems = 2;          CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);  /* twin, one collector */
    bad.collector[2] = bad.collector[3] = 1;   CHECK_EQ_INT(rb_engine_desc_check(&bad), 0);
    for (int i = 0; i < 4; i++) bad.collector[i] = 1;   /* a second collector, but no first */
    CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.muffler_area = 0.0;           CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.tailpipe_diameter = NAN;      CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    bad = d; bad.p_ambient = NAN;              CHECK_EQ_INT(rb_engine_desc_check(&bad), -1);
    CHECK_EQ_INT(rb_engine_desc_check(NULL), -1);
    CHECK_EQ_INT(rb_engine_init(&e, &bad, DT), -1);
    CHECK_EQ_INT(rb_engine_init(&e, &d, 0.0), -1);
    CHECK_EQ_INT(rb_engine_init(&e, &d, NAN), -1);

    /* Sixteen cylinders with 4.9 m runners and 5 cm headers, and 10 m of
     * exhaust pipe and tailpipe, want 1181 cells (with the muffler and the
     * snorkel): more than the pool holds. Every pipe still gets one, the rest
     * share what's left, and it runs. */
    bad = d;
    CHECK_EQ_INT(rb_engine_layout_inline(&bad, 16), 0);
    bad.runner_length = 4.9;
    for (int i = 0; i < 16; i++) bad.header_length[i] = 0.05;
    bad.exhaust_pipe_length = bad.tailpipe_length = 10.0;
    CHECK_EQ_INT(rb_engine_init(&e, &bad, DT), 0);
    CHECK_EQ_INT(rb_engine_pipe_count(&e), 36);
    int cells = 0, fewest = RB_PIPE_MAX_CELLS;
    for (int k = 0; k < rb_engine_pipe_count(&e); k++) {
        cells += rb_engine_pipe(&e, k)->cells;
        if (rb_engine_pipe(&e, k)->cells < fewest) fewest = rb_engine_pipe(&e, k)->cells;
    }
    CHECK(cells <= RB_ENGINE_CELLS && fewest == 1);
    CHECK(rb_engine_pipe(&e, 0)->cells > 20);   /* the runners still have some */
    e.rpm = 3000.0;
    for (int k = 0; k < 2000; k++) rb_engine_step(&e);
    CHECK(e.plenum.p > 0.0 && e.cyl[15].gas.p > 0.0);
}

void test_engine_pistons_follow_the_crank(void)
{
    /* After any number of steps, every piston is exactly where its crank
     * angle puts it: the gas volume is V(theta - firing). */
    int wrong = 0;
    start(4321.0, 0.5);
    for (int k = 0; k < 12345; k++) rb_engine_step(&e);
    for (int i = 0; i < 4; i++) {
        const double theta = rb_engine_cylinder_angle(&e, i) * 3.14159265358979323846 / 180.0;
        const double V = rb_cyl_volume(&e.cyl[i].geom, theta);
        if (fabs(e.cyl[i].gas.V - V) > 1e-12 * V) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK(e.theta >= 0.0 && e.theta < 720.0);
}

void test_engine_conserves_mass_and_energy(void)
{
    /* Part throttle from rest: the plenum is still pumping down, so a lot is
     * changing. Every kg and every J must be accounted for. */
    start(3000.0, 0.1);
    const contents c0 = inside();
    for (int k = 0; k < 3 * 7680; k++) rb_engine_step(&e);   /* three cycles */
    const contents c1 = inside();

    CHECK(e.plenum.p < 0.8 * e.desc.p_ambient);   /* the plenum is pumping down */
    CHECK_NEAR(c1.mass - c0.mass, e.mass_in - e.mass_out, 1e-12 * c0.mass);
    CHECK_NEAR(c1.U - c0.U, energy_books(), 1e-9 * fabs(e.energy_out));
    CHECK(e.wall_heat == 0.0 && e.heat_released == 0.0 && e.fuel_in == 0.0);
    CHECK_EQ_INT(c1.repairs, 0);
    CHECK_EQ_INT((int)e.cycles, 3);
}

void test_engine_settles_and_balances(void)
{
    /* Wide open at 3000 rpm: it settles within a few cycles, the air through
     * the throttle equals the air into the cylinders, and the volumetric
     * efficiency is in the range of real naturally aspirated engines near
     * their torque peak, where the intake runners' ram fills the cylinders
     * nearly full (stage 9). */
    start(3000.0, 1.0);
    const int cycles = rb_engine_settle(&e, 100, 1e-6);
    const rb_engine_cycle *r = &e.result;
    const double expected_flow = r->ve * ambient_density() * rb_engine_displacement(&e) * 3000.0 / 120.0;

    CHECK(cycles > 0 && cycles < 50);
    CHECK(r->ve > 0.90 && r->ve < 1.0);
    CHECK_NEAR(r->air_flow, expected_flow, 1e-5 * expected_flow);
    CHECK(r->map < e.desc.p_ambient && r->map > 0.98 * e.desc.p_ambient);
    CHECK(r->map_min < r->map && r->map < r->map_max);
    CHECK(r->pmep > 0.0);           /* turning it over costs work ... */
    CHECK(r->net_work < 0.0);       /* ... which the motor supplies */
}

void test_engine_cycle_totals_dont_jitter(void)
{
    /* At 7000 rpm a cycle is 3291.4 steps, not a whole number. The step that
     * crosses the end of a cycle is shared between the two cycles, so the
     * totals still settle to a steady value (without sharing, they wobble
     * by about one step in 3291, 3e-4). */
    start(7000.0, 1.0);
    CHECK(rb_engine_settle(&e, 100, 1e-5) > 0);
}

void test_engine_cylinders_breathe_alike(void)
{
    /* An even-firing four: every cylinder sees the same plenum pressure
     * history, 180 degrees apart, so once settled they breathe the same,
     * even at part throttle where the plenum pulses hard. */
    double worst = 0.0;
    start(2000.0, 0.15);
    CHECK(rb_engine_settle(&e, 80, 1e-6) > 0);
    for (int i = 0; i < 4; i++)
        worst = fmax(worst, fabs(e.result.ve_cyl[i] - e.result.ve));
    CHECK(worst < 1e-5 * e.result.ve);
    CHECK(e.result.map_max - e.result.map_min > 0.03 * e.result.map);   /* it does pulse */
}

void test_engine_pipes_two_rates(void)
{
    /* The pipes step at 48 kHz, the cylinders and the cells at the valves at
     * the physics rate (pipe.h, two rates). Between the pipes' steps, the
     * valves' cells see the slow cells' pressures as they are by now: with
     * the gas they have taken in or given, and their own rates running since
     * the last slow step. So the engine breathes the same at 192 and 384 kHz,
     * 4 and 8 steps to a pipe step: to 0.05 %. (Seeing the pressures of the
     * last slow step, it was 0.3 % apart, and the cylinders took turns.) */
    double ve[2];
    for (int k = 0; k < 2; k++) {
        rb_engine_desc d;
        rb_engine_desc_default(&d);
        d.heat_transfer = 0;
        rb_engine_init(&e, &d, k ? DT / 2.0 : DT);
        e.rpm = 1500.0;
        e.throttle_pos = 1.0;
        CHECK(rb_engine_settle(&e, 100, 1e-6) > 0);
        ve[k] = e.result.ve;
        for (int i = 1; i < 4; i++) CHECK_NEAR(e.result.ve_cyl[i], e.result.ve_cyl[0], 1e-6);
    }
    CHECK_NEAR(ve[1], ve[0], 5e-4 * ve[0]);
}

void test_engine_throttle_response(void)
{
    /* Closing the throttle at 2000 rpm: the plenum pressure and the air drawn
     * in fall, and the pumping loss grows. (Settled to 1e-4: throttled, the
     * pumping work heats the little air that flows, and it takes the exhaust
     * pipes and muffler, 17 L, more than 100 cycles to warm through.) Wide
     * open, the plenum is about 1 kPa down, mostly across the air filter
     * (stage 12). */
    static const double pos[4] = { 1.0, 0.15, 0.04, 0.0 };
    double map[4], ve[4], pmep[4];
    int wrong = 0;

    for (int k = 0; k < 4; k++) {
        start(2000.0, pos[k]);
        if (rb_engine_settle(&e, 100, 1e-4) < 0) wrong++;
        map[k] = e.result.map;
        ve[k] = e.result.ve;
        pmep[k] = e.result.pmep;
        if (k > 0 && !(map[k] < map[k - 1] && ve[k] < ve[k - 1] && pmep[k] > pmep[k - 1])) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK(map[0] > 0.985 * e.desc.p_ambient);   /* wide open: hardly any vacuum */
    CHECK(map[3] < 0.20 * e.desc.p_ambient);   /* shut: deep vacuum */
    CHECK(ve[3] < 0.05);
}

void test_engine_intake_losses(void)
{
    /* Stage 12's way in: the air drawn into the snorkel's mouth speeds up
     * (and loses a fifth of a velocity head more), then pushes through the
     * air box's filter (a laminar loss). Wide open at 6000 rpm, 22 m/s in
     * the snorkel, that's 0.4 kPa and 1.35 kPa: the air box sits 1.75 kPa
     * (1-2.5) below the outside air over a cycle, and the plenum lower
     * still. */
    start(6000.0, 1.0);
    CHECK(rb_engine_settle(&e, 100, 1e-4) > 0);
    double box = 0.0;
    long n = 0;
    const long c0 = e.cycles;
    while (e.cycles == c0) {
        rb_engine_step(&e);
        box += e.airbox.p;
        n++;
    }
    const double drop = e.desc.p_ambient - box / n;
    CHECK(drop > 1000.0 && drop < 2500.0);
    CHECK(e.result.map < box / n);
}

void test_engine_wide_tailpipes(void)
{
    /* Stage 13's fuzzer: a cross-plane V8 with 75 mm tailpipes (so 75 mm
     * straight-through mufflers) after its 55 mm exhaust pipes blew up once the
     * exhaust's pulses grew, the mufflers going straight on from the pipes.
     * Now they open into them as into a volume, and wide open at 6000 rpm
     * every pipe stays within a few bar. */
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("v8-cross"), &d);
    d.tailpipe_diameter = 0.075;
    static rb_engine x;
    CHECK_EQ_INT(rb_engine_init(&x, &d, 1.0 / 96000.0), 0);
    x.rpm = 6000.0;
    x.throttle_pos = 1.0;
    x.ignition = 1;
    double worst = 0.0;
    for (long k = 0; k < 96000 && worst < 1e7; k++) {
        rb_engine_step(&x);
        if (k % 64) continue;
        for (int j = 0; j < rb_engine_pipe_count(&x); j++) {
            const rb_pipe *p = rb_engine_pipe(&x, j);
            for (int c = 0; c < p->cells; c++)
                if (!(p->cell[c].p < worst)) worst = p->cell[c].p;
        }
    }
    CHECK(worst > 1.5e5 && worst < 8e5);
}

void test_engine_vacuum(void)
{
    /* Stage 12b's fuzzer: held at 6000 rpm with the throttle shut, the plenum
     * near a vacuum (5-9 kPa), the cylinders pull the runners' gas so hard
     * that their faces' flows ran away (58 times the sonic flow) and the gas
     * in the V8's and the boxer's pipes fell to 13 and 24 K. Each face is
     * held under twice its gas's sonic flow now (RB_PIPE_MOST_SONIC), and
     * over half a second no pipe's gas falls below 100 K. */
    const char *keys[2] = { "v8-cross", "boxer4" };
    static rb_engine x;
    for (int k = 0; k < 2; k++) {
        rb_engine_desc d;
        rb_preset_desc(rb_preset_find(keys[k]), &d);
        CHECK_EQ_INT(rb_engine_init(&x, &d, 1.0 / 192000.0), 0);
        x.rpm = 6000.0;
        x.throttle_pos = 0.0;
        x.ignition = 1;
        double coldest = INFINITY;
        for (long s = 0; s < 96000 && coldest > 1.0; s++) {
            rb_engine_step(&x);
            if (s % 16) continue;
            for (int j = 0; j < rb_engine_pipe_count(&x); j++) {
                const rb_pipe *p = rb_engine_pipe(&x, j);
                for (int c = 0; c < p->cells; c++)
                    if (!(p->cell[c].T > coldest)) coldest = p->cell[c].T;
            }
        }
        CHECK(coldest > 100.0);
        CHECK(x.plenum.p < 12000.0);   /* it is near a vacuum */
    }
}

void test_engine_muffler_back_pressure(void)
{
    /* Wide open at 6000 rpm (fired, held), the gas leaving the four at about
     * 100 m/s: through a straight pipe the collector is 16 kPa over the
     * outside air; a chambered muffler, the gas spreading into its can and
     * squeezing out through the baffles, costs 7 kPa more (at least 5), and
     * a straight-through, its packing taking nothing from a steady flow, no
     * more than the straight pipe (to 1 kPa). (Joined to the next pipe as
     * through a volume, a straight pipe cost as much as the can.) */
    double back[3];
    for (int type = RB_MUFFLER_NONE; type <= RB_MUFFLER_CHAMBERED; type++) {
        rb_engine_desc d;
        rb_engine_desc_default(&d);
        d.muffler_type = type;
        CHECK_EQ_INT(rb_engine_init(&e, &d, DT), 0);
        e.rpm = 6000.0;
        e.throttle_pos = 1.0;
        e.ignition = 1;
        CHECK(rb_engine_settle(&e, 100, 1e-4) > 0);
        back[type] = e.result.p_exhaust - d.p_ambient;
    }
    CHECK(back[RB_MUFFLER_CHAMBERED] > back[RB_MUFFLER_NONE] + 5000.0);
    CHECK_NEAR(back[RB_MUFFLER_STRAIGHT], back[RB_MUFFLER_NONE], 1000.0);
}

void test_engine_choked_throttle(void)
{
    /* Nearly shut, the plenum stays below 0.528 bar the whole time, so the
     * throttle is choked: the air flow is fixed by the throttle alone,
     * Cd A p0 sqrt(g / (R T0)) (2/(g+1))^((g+1)/(2(g-1))), whatever the speed.
     * (Settled to 1e-4, as in test_engine_throttle_response.) */
    const double g = 1.4, R = 287.1;   /* air near 300 K; stage 3 */
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    const double area = rb_throttle_area(&d.throttle, 0.03);
    const double choked = area * d.p_ambient * sqrt(g / (R * d.T_ambient))
                        * pow(2.0 / (g + 1.0), (g + 1.0) / (2.0 * (g - 1.0)));
    const double rpms[2] = { 1500.0, 4000.0 };

    for (int k = 0; k < 2; k++) {
        start(rpms[k], 0.03);
        CHECK(rb_engine_settle(&e, 100, 1e-4) > 0);
        CHECK(e.result.map_max < 0.528 * d.p_ambient);
        CHECK_NEAR(e.result.air_flow, choked, 2e-3 * choked);   /* gamma of real air: 1.4 +- 0.1 % */
    }
}

void test_engine_pumping_loss(void)
{
    /* At low speed the pumping loss is simply the pressure difference the
     * pistons work against: they push out against the atmosphere and pull in
     * against the plenum, so PMEP = p_ambient - MAP (Heywood, chapter 5).
     * Slow enough that the gas in the pipes doesn't matter: motored at such
     * light loads, the cylinder is at about a quarter of a bar when its
     * exhaust valve opens, the header's gas rushes in, and the piston has to
     * push it back out. At 4 % throttle that costs 4 % more at 1000 rpm, 8 %
     * at 2000 rpm. */
    start(500.0, 0.015);
    CHECK(rb_engine_settle(&e, 100, 1e-5) > 0);
    const double expected = e.desc.p_ambient - e.result.map;
    CHECK(expected > 0.5e5);
    CHECK_NEAR(e.result.pmep, expected, 0.02 * expected);
}

/* Pressure of chamber j: the four cylinders, then the plenum, the collector,
 * the muffler's first cell (where the exhaust pipe opens into it), cylinder
 * 1's runner and header cells at its valves, and the air box. */
static double watched_p(int j)
{
    if (j < 4) return e.cyl[j].gas.p;
    switch (j) {
    case 4:  return e.plenum.p;
    case 5:  return e.collector[0].p;
    case 6:  return rb_pipe_end_cell(&e.muffler[0], 0)->p;
    case 7:  return rb_pipe_end_cell(&e.runner[0], 1)->p;
    case 8:  return rb_pipe_end_cell(&e.header[0], 0)->p;
    default: return e.airbox.p;
    }
}

void test_engine_pressures_dont_zigzag(void)
{
    /* Explicit flow steps can make a small chamber's pressure flip up and
     * down every step when several openings feed it. Watch every chamber for
     * three changes of direction in a row, motored and fired. Changes under
     * 1e-5 of the pressure don't count: the pipes' cells at the valves keep a
     * ripple of a few tenths of a Pa from their slow neighbours' steps, every
     * fourth step, which shows where a pressure turns over (0.2 Pa at 1.5 bar,
     * in the header at 6500 rpm). */
    static const double cases[4][3] = { { 1200.0, 0.02, 0 }, { 3000.0, 1.0, 0 }, { 6500.0, 1.0, 0 },
                                        { 3000.0, 1.0, 1 } };
    int zigzags = 0;

    for (int k = 0; k < 4; k++) {
        if (cases[k][2] > 0.0) start_fired(cases[k][0], cases[k][1]);
        else start(cases[k][0], cases[k][1]);
        rb_engine_settle(&e, 100, 1e-4);
        double prev[10], dprev[10] = { 0 };
        int flips[10] = { 0 };
        for (int j = 0; j < 10; j++) prev[j] = watched_p(j);
        const long c = e.cycles;
        for (long n = 0; e.cycles == c && n < lround(2.0 / DT); n++) {
            rb_engine_step(&e);
            for (int j = 0; j < 10; j++) {
                const double p = watched_p(j);
                double dp = p - prev[j];
                if (fabs(dp) < 1e-5 * p) dp = 0.0;   /* too small to count */
                flips[j] = (dp * dprev[j] < 0.0) ? flips[j] + 1 : 0;
                if (flips[j] >= 3) zigzags++;
                prev[j] = p;
                dprev[j] = dp;
            }
        }
    }
    CHECK_EQ_INT(zigzags, 0);
}

/* ---- fired (stage 6) ---------------------------------------------------- */

void test_engine_fired_conserves_everything(void)
{
    /* From a standing start with the ignition on, the books must balance for
     * every species and for energy: air and fuel in, exhaust out, burning,
     * walls and pistons. */
    start_fired(3000.0, 1.0);
    const books b0 = snapshot();
    const contents c0 = b0.c;
    for (int k = 0; k < 4 * 7680; k++) rb_engine_step(&e);   /* four cycles */
    const books b1 = snapshot();
    const contents c1 = b1.c;
    const double burned = e.fuel_burned;
    CHECK_EQ_INT(species_unbalanced(&b0, &b1), 0);
    CHECK(burned > 0.0 && e.heat_released > 0.0 && e.wall_heat < 0.0);
    CHECK_NEAR(c1.mass - c0.mass, e.mass_in - e.mass_out + e.fuel_in, 1e-12 * c0.mass);
    CHECK_NEAR(c1.U - c0.U, energy_books(), 1e-9 * e.heat_released);
    CHECK_NEAR(e.heat_released, burned * rb_fuel_heat(), 1e-9 * e.heat_released);
    CHECK_EQ_INT(c1.repairs, 0);
}

void test_engine_fired_full_load(void)
{
    /* The plan's goal for stage 6: wide open at 3000 rpm with the spark at
     * best torque, a 2.0 L four makes 11-13 bar IMEP with its peak pressure
     * 10-20 degrees after TDC. The rest should look like a real engine too. */
    start_fired(3000.0, 1.0);
    CHECK(rb_engine_settle(&e, 100, 1e-4) > 0);
    const rb_engine_cycle *r = &e.result;
    const double fuel_energy = r->fuel_flow * (120.0 / 3000.0) * RB_FUEL_LHV;   /* per cycle */

    CHECK(r->imep_net > 11e5 && r->imep_net < 13e5);
    CHECK(r->angle_p_max > 10.0 && r->angle_p_max < 20.0);
    CHECK(r->p_max > 45e5 && r->p_max < 75e5);
    CHECK(r->efficiency > 0.34 && r->efficiency < 0.42);    /* indicated */
    CHECK(r->ve > 0.82 && r->ve < 0.95);
    CHECK_NEAR(r->afr, 14.6, 0.15);
    CHECK(r->t_exhaust > 1000.0 && r->t_exhaust < 1400.0);
    CHECK(r->residual > 0.02 && r->residual < 0.08);
    CHECK(r->wall_loss > 0.10 * fuel_energy && r->wall_loss < 0.25 * fuel_energy);
    CHECK(r->p_exhaust > e.desc.p_ambient && r->p_exhaust < 1.15 * e.desc.p_ambient);
    CHECK_NEAR(r->imep_net, r->imep_gross - r->pmep, 1e-6 * r->imep_net);
    CHECK_NEAR(r->torque, r->net_work / (4.0 * 3.14159265358979323846), 1e-9);
}

void test_engine_spark_advance(void)
{
    /* With the spark set by hand (the engine computer's map off): advancing
     * it raises the peak pressure and moves it earlier, and the torque peaks
     * in between ("MBT"): too early or too late both lose. */
    static const double spark[3] = { 10.0, 25.0, 40.0 };
    double torque[3], p_max[3], angle[3];
    for (int k = 0; k < 3; k++) {
        start_fired(3000.0, 1.0);
        e.spark_map = 0;
        e.spark = spark[k];
        rb_engine_settle(&e, 100, 1e-4);
        torque[k] = e.result.torque;
        p_max[k] = e.result.p_max;
        angle[k] = e.result.angle_p_max;
    }
    CHECK(torque[1] > torque[0] && torque[1] > torque[2]);
    CHECK(p_max[0] < p_max[1] && p_max[1] < p_max[2]);
    CHECK(angle[0] > angle[1] && angle[1] > angle[2]);
}

void test_engine_mixture_strength(void)
{
    /* A slightly rich mixture makes the most power and a lean one the most
     * efficient, and a rich engine blows unburned fuel out of its exhaust. */
    static const double afr[3] = { 13.0, 14.6, 17.0 };
    double imep[3], eff[3], unburned[3];
    for (int k = 0; k < 3; k++) {
        start_fired(3000.0, 1.0);
        e.afr = afr[k];
        rb_engine_settle(&e, 100, 1e-4);
        const double fuel0 = e.species_out[RB_FUEL], all0 = e.mass_out;
        CHECK(run_cycles(1));
        imep[k] = e.result.imep_net;
        eff[k] = e.result.efficiency;
        unburned[k] = (e.species_out[RB_FUEL] - fuel0) / (e.mass_out - all0);
    }
    CHECK(imep[0] > imep[1] && imep[1] > imep[2]);
    CHECK(eff[2] > eff[1] && eff[1] > eff[0]);
    CHECK(unburned[0] > 2.0 * unburned[1]);
}

void test_engine_walls_and_residuals(void)
{
    /* Warm walls heat the incoming charge, so the engine draws in less air
     * than with adiabatic walls. Throttled, more of the trapped charge is old
     * exhaust, and the injectors still hit their target: the engine doesn't
     * breathe fresh air in through its tailpipe. (Wide open at 2000 rpm, the
     * headers' pressure wave keeps 5.8 % of the charge burned gas, 4.2 % with
     * stage 6's lumped exhaust: twice that is still a clear difference.) */
    rb_engine_desc d;
    start(1000.0, 1.0);
    rb_engine_settle(&e, 100, 1e-4);
    const double ve_adiabatic = e.result.ve;
    rb_engine_desc_default(&d);
    rb_engine_init(&e, &d, DT);
    e.rpm = 1000.0;
    rb_engine_settle(&e, 100, 1e-4);
    CHECK(e.result.ve < ve_adiabatic - 0.02);

    start_fired(2000.0, 1.0);
    rb_engine_settle(&e, 100, 1e-4);
    const double residual_wot = e.result.residual;
    start_fired(2000.0, 0.06);
    rb_engine_settle(&e, 150, 1e-4);
    CHECK(e.result.residual > 2.0 * residual_wot);
    CHECK_NEAR(e.result.afr, 14.6, 0.15);
    CHECK(e.result.pmep > 0.5e5);
    CHECK(e.result.efficiency > 0.2 && e.result.efficiency < 0.34);
}

void test_engine_trapped_charge(void)
{
    /* At intake valve closing the engine notes the trapped charge, fuel and
     * all, as Woschni's reference state: throttled, it is well below
     * atmospheric pressure. */
    start_fired(2000.0, 0.06);
    rb_engine_settle(&e, 150, 1e-4);
    const double ivc = rb_valve_closes(&e.desc.intake);
    double a = rb_engine_cylinder_angle(&e, 0), before = a;
    /* The step that passes IVC: like the engine, count an event exactly at
     * the step's start, not one exactly at its end. */
    for (int k = 0; k < 12000 && !(before <= ivc && ivc < a); k++) {
        before = a;
        rb_engine_step(&e);
        a = rb_engine_cylinder_angle(&e, 0);
    }
    const rb_chamber *g = &e.cyl[0].gas;
    const rb_burn *b = &e.burn[0];
    CHECK(before <= ivc && ivc < a);
    CHECK_NEAR(b->p_ivc, g->p, 0.002 * g->p);    /* one step later: a hair more squeezed */
    CHECK_NEAR(b->V_ivc, g->V, 0.002 * g->V);
    CHECK_NEAR(b->T_ivc, g->T, 0.002 * g->T);
    CHECK_NEAR(b->residual, g->y[RB_BURNED], 1e-9);
    CHECK(g->m[RB_FUEL] > 0.0 && b->p_ivc < 0.6 * e.desc.p_ambient);
}

void test_engine_burn_stops_at_evo(void)
{
    /* Spark 90 degrees after TDC: the exhaust valve opens with the flame still
     * going, and whatever hasn't burned by then leaves unburned. The burned
     * share is where the Wiebe curve, as long as this charge burns (stage 9),
     * has got to at the valve's opening, and the books still balance. */
    start_fired(3000.0, 1.0);
    e.spark_map = 0;
    e.spark = -90.0;
    rb_engine_settle(&e, 100, 1e-4);
    const books b0 = snapshot();
    CHECK(run_cycles(1));
    const books b1 = snapshot();
    const rb_engine_desc *d = &e.desc;
    const double evo = rb_valve_opens(&d->exhaust), a = rb_engine_cylinder_angle(&e, 0);
    const double x_evo = rb_wiebe(evo - 90.0, e.burn[0].duration, d->wiebe_a, d->wiebe_m);
    const double fuel = b1.fuel_in - b0.fuel_in, unburned = b1.out[RB_FUEL] - b0.out[RB_FUEL];

    CHECK(x_evo > 0.3 && x_evo < 0.95);          /* the valve opens mid-burn */
    CHECK(a > evo || a < 89.0);                   /* cylinder 1 is between its EVO and spark */
    CHECK_NEAR(e.burn[0].fraction, x_evo, 0.01);
    CHECK(!e.burn[0].burning);
    CHECK(unburned > 0.1 * fuel);
    CHECK_EQ_INT(species_unbalanced(&b0, &b1), 0);
    CHECK_NEAR(b1.c.U - b0.c.U, b1.energy - b0.energy, 1e-9 * (b1.heat - b0.heat));
}

void test_engine_settles_light_load(void)
{
    /* Nearly shut at 800 rpm, the plenum fills slowly and the engine takes
     * the longest to settle. When rb_engine_settle says it has, the work of
     * the next cycle agrees too, not just the air. */
    start_fired(800.0, 0.02);
    const int cycles = rb_engine_settle(&e, 150, 1e-4);
    const double w = e.result.net_work;
    CHECK(run_cycles(1));
    CHECK(cycles > 0 && cycles < 100);
    CHECK(fabs(e.result.net_work - w) <= 2e-4 * (fabs(w) + 1.0));
}

void test_engine_spark_change_mid_burn(void)
{
    /* The spark timing is fixed for each cycle at intake valve closing, as an
     * engine computer does: changing the setting (by hand, the map off) while
     * a cylinder burns neither shifts its burn nor lights it again. The next
     * cycle uses it. */
    start_fired(3000.0, 1.0);
    e.spark_map = 0;
    rb_engine_settle(&e, 100, 1e-4);
    const double p_max_25 = e.result.p_max;
    const rb_burn *b = &e.burn[0];
    CHECK(run_to_angle(705.0, 706.0));            /* 10 degrees after a 25-degree spark */
    CHECK(b->burning && b->fraction > 0.0);

    const double burnable = b->burnable;
    double x = b->fraction, biggest = 0.0, smallest = 1.0;
    e.spark = 40.0;                               /* the burn mustn't jump 15 degrees ahead */
    for (long k = 0; k < 20000 && rb_engine_cylinder_angle(&e, 0) < 712.0; k++) {
        rb_engine_step(&e);
        if (b->fraction - x > biggest) biggest = b->fraction - x;
        if (b->fraction - x < smallest) smallest = b->fraction - x;
        x = b->fraction;
    }
    e.spark = 5.0;                                /* nor light again at 5 degrees before TDC */
    for (long k = 0; k < 20000 && rb_engine_cylinder_angle(&e, 0) > 700.0; k++) {
        rb_engine_step(&e);
        if (b->fraction - x > biggest) biggest = b->fraction - x;
        if (b->fraction - x < smallest) smallest = b->fraction - x;
        x = b->fraction;
    }
    CHECK(biggest < 0.005);                       /* a normal step's worth: at most 0.0031 */
    CHECK(smallest >= 0.0);
    CHECK(b->burnable == burnable);               /* never lit again */

    e.spark = 40.0;                               /* from the next cycle on, it counts */
    CHECK(run_cycles(3));
    CHECK(e.result.p_max > p_max_25 + 10e5);
}

/* ---- the free crank (stage 7) ---- */

/* The default engine with its crank free and the throttle shut, as a car's
 * engine sits before it's started. */
static void start_free(void)
{
    rb_engine_desc d;
    rb_engine_desc_default(&d);
    rb_engine_init(&e, &d, DT);
    e.hold_speed = 0;
    e.throttle_pos = 0.0;
}

/* What the crank's books say its kinetic energy should be. */
static double crank_books(void)
{
    return e.crank_gas_work - e.crank_case_work + e.starter_work - e.friction_work - e.load_work
         + e.stop_energy + e.hold_work;
}

/* Runs the engine for t seconds; returns the lowest and highest rpm seen. */
static void run_for(double t, double *lo, double *hi)
{
    const long steps = lround(t / DT);
    double a = INFINITY, b = 0.0;
    for (long k = 0; k < steps; k++) {
        rb_engine_step(&e);
        if (e.rpm < a) a = e.rpm;
        if (e.rpm > b) b = e.rpm;
    }
    if (lo) *lo = a;
    if (hi) *hi = b;
}

void test_engine_friction(void)
{
    /* Friction follows Chen and Flynn's formula, and the brake numbers are
     * the indicated ones minus it. Wide open at 3000 rpm that's about 1.2 bar
     * (a modern engine, stage 9), and about half as much again at twice the
     * speed. */
    start_fired(3000.0, 1.0);
    rb_engine_settle(&e, 100, 1e-5);
    const rb_engine_cycle *r = &e.result;
    const rb_engine_desc *d = &e.desc;
    const double sp = 2.0 * d->stroke * 3000.0 / 60.0;
    const double expected = d->fmep_constant + d->fmep_pressure * r->p_max + d->fmep_speed * sp
                          + d->fmep_speed2 * sp * sp;
    const double w = 3000.0 * 3.14159265358979323846 / 30.0;
    CHECK_NEAR(r->fmep, expected, 1e-4 * expected);
    CHECK_NEAR(rb_engine_fmep(&e, w), expected, 1e-4 * expected);
    CHECK(r->fmep > 1.05e5 && r->fmep < 1.3e5);
    CHECK(rb_engine_fmep(&e, 2.0 * w) > 1.4 * r->fmep && rb_engine_fmep(&e, 2.0 * w) < 1.6 * r->fmep);
    CHECK_NEAR(r->bmep, r->imep_net - r->fmep, 1e-9 * r->imep_net);
    CHECK_NEAR(r->brake_torque, r->torque - r->fmep * rb_engine_displacement(&e) / (4.0 * 3.14159265358979323846),
               1e-6 * r->torque);
    CHECK_NEAR(r->brake_power, r->brake_torque * w, 1e-3 * r->brake_power);
    CHECK_NEAR(r->rpm, 3000.0, 1e-6);
}

void test_engine_crank_books(void)
{
    /* Starting from rest, the crank's kinetic energy is exactly the work done
     * on it: by the gas through the pistons, the crankcase air under them, the
     * starter, friction and any load. */
    start_free();
    e.ignition = 1;
    e.starter = 1;
    e.load_torque = 5.0;
    const books b0 = snapshot();
    double v0 = 0.0;
    for (int i = 0; i < e.desc.cylinders; i++) v0 += e.cyl[i].gas.V;
    /* While cranking, the computer sets a late spark (no kickback). */
    int due = -1;
    for (long k = 0; k < lround(1.0 / DT) && due < 0; k++) {
        rb_engine_step(&e);
        for (int i = 0; i < e.desc.cylinders; i++)
            if (e.burn[i].spark_due) due = i;
    }
    CHECK(due >= 0);
    if (due < 0) return;
    CHECK(e.rpm < e.desc.cranking_rpm);
    CHECK_NEAR(e.burn[due].spark_angle, 720.0 - e.desc.cranking_spark, 1e-9);
    double idle_air = 0.0;
    for (long k = 0; k < 192000; k++) {
        rb_engine_step(&e);
        if (e.idle_air.mdot > idle_air) idle_air = e.idle_air.mdot;
    }
    CHECK(e.rpm > 500.0);
    /* The gas's books balance too, idle air valve and all. */
    const books b1 = snapshot();
    CHECK(idle_air > 0.0);
    CHECK_EQ_INT(species_unbalanced(&b0, &b1), 0);
    CHECK_NEAR(b1.c.U - b0.c.U, b1.energy - b0.energy, 1e-9 * (b1.heat - b0.heat));
    CHECK_NEAR(e.kinetic, crank_books(), 1e-9 * fabs(e.crank_gas_work));
    /* The pistons' undersides push the crankcase air (at ambient pressure) out
     * of the way of the cylinders' total volume. */
    double v1 = 0.0;
    for (int i = 0; i < e.desc.cylinders; i++) v1 += e.cyl[i].gas.V;
    CHECK(fabs(v1 - v0) > 1e-6);
    CHECK_NEAR(e.crank_case_work, e.desc.p_ambient * (v1 - v0), 1e-12 * e.desc.p_ambient);
    CHECK_NEAR(e.crank_gas_work, -e.work_on_gas, 1e-9 * fabs(e.crank_gas_work));
    CHECK_NEAR(e.kinetic, 0.5 * e.desc.inertia * e.omega * e.omega, 1e-12 * e.kinetic);
    CHECK(e.starter_work > 0.0 && e.friction_work > 0.0 && e.load_work > 0.0);
    CHECK(e.hold_work == 0.0);
}

void test_engine_cranks(void)
{
    /* With the ignition off, the starter turns the engine at a typical
     * cranking speed, slowing on every compression and speeding up after. */
    double lo, hi;
    start_free();
    e.starter = 1;
    run_for(1.0, NULL, NULL);
    const long c = e.cycles;
    run_for(1.0, &lo, &hi);
    CHECK(e.cycles > c);
    CHECK(e.result.rpm > 150.0 && e.result.rpm < 350.0);
    CHECK(hi - lo > 50.0);
    CHECK(lo > 50.0);
    CHECK_NEAR(e.kinetic, crank_books(), 1e-9 * fabs(e.crank_gas_work));
}

void test_engine_drives_itself(void)
{
    /* The plan's check for stage 7, as a driver would do it: turn the key,
     * let go when it runs, let it idle, blip the throttle, floor it into the
     * rev limiter, let go, switch off. */
    double lo, hi;
    const double limit = 6800.0;
    start_free();
    e.ignition = 1;
    e.starter = 1;
    for (int k = 0; k < 192000 && e.starter; k++) {           /* at most a second */
        rb_engine_step(&e);
        if (e.rpm_smooth > 600.0) e.starter = 0;
    }
    CHECK(!e.starter);                                         /* it started */
    run_for(4.0, &lo, &hi);
    CHECK(lo > 600.0);                                         /* no stall settling down */
    run_for(1.0, &lo, &hi);
    CHECK_NEAR(e.result.rpm, 800.0, 15.0);                     /* idles ... */
    CHECK(hi - lo < 100.0);                                    /* ... smoothly */
    CHECK(e.result.bmep > -0.2e5 && e.result.bmep < 0.2e5);    /* doing no work */

    e.throttle_pos = 0.3;                                      /* a blip */
    run_for(0.3, NULL, &hi);
    CHECK(hi > 2500.0);
    e.throttle_pos = 0.0;
    int fuel_cut = 0;
    double fuel_during_cut = 0.0;
    for (long k = 0; k < 96000; k++) {                         /* coasting down, */
        const double before_fuel = e.fuel_in;
        rb_engine_step(&e);
        if (e.fuel_cut) {
            fuel_cut = 1;
            fuel_during_cut += e.fuel_in - before_fuel;
        }
    }
    CHECK(fuel_cut);
    CHECK_NEAR(fuel_during_cut, 0.0, 1e-14); /* after timed entry, injectors are off */
    run_for(2.5, &lo, NULL);
    CHECK(lo > 600.0);                                         /* back down without stalling */
    run_for(0.5, NULL, NULL);
    CHECK_NEAR(e.result.rpm, 800.0, 30.0);

    e.throttle_pos = 1.0;                                      /* floored */
    const double trim = e.idle_trim;
    int cut = 0;
    for (long k = 0; k < 2 * 192000; k++) {
        rb_engine_step(&e);
        if (e.spark_cut) cut = 1;
    }
    run_for(0.5, &lo, &hi);
    CHECK(cut);
    CHECK(e.idle_trim == trim);                                /* learns only with the foot off */
    CHECK(hi < limit + 150.0 && lo > limit - 400.0);           /* bouncing off the limiter, */
    CHECK(lo < limit - 60.0);                                  /* down to where the spark resumes */
    e.throttle_pos = 0.0;                                      /* about 2.9 s down with */
    run_for(4.0, &lo, NULL);                                   /* the fuel cut, then */
    CHECK(lo > 600.0);                                         /* back to idle */
    run_for(0.5, NULL, NULL);
    CHECK_NEAR(e.result.rpm, 800.0, 40.0);                     /* idling again */

    e.ignition = 0;                                            /* switched off */
    run_for(3.0, NULL, NULL);
    CHECK(e.rpm == 0.0 && e.omega == 0.0);
    const double theta = e.theta;
    run_for(0.2, NULL, &hi);
    CHECK(hi == 0.0 && e.theta == theta);                      /* and it stays stopped, */
    double torque = 0.0;                                       /* the gas pushing on each piston */
    for (int i = 0; i < e.desc.cylinders; i++)
        torque += (e.cyl[i].gas.p - e.desc.p_ambient) * rb_cyl_dvolume(&e.cyl[i].geom, e.cyl[i].theta);
    CHECK_NEAR(e.gas_torque, torque, 1e-3 * fabs(torque) + 1e-3);
    CHECK(torque < e.friction_torque);                         /* (friction holds it; it never turns back) */
    CHECK_NEAR(e.kinetic, crank_books(), 1e-9 * fabs(e.crank_gas_work));
    CHECK(isfinite(e.plenum.p) && isfinite(e.cyl[0].gas.T));
}

void test_engine_free_matches_held(void)
{
    /* Held at 3000 rpm wide open by a dyno, the engine delivers some brake
     * torque. Let go of the crank with a load that takes exactly that torque
     * at 3000 rpm (and more above, less below, like a fan), and the crank
     * stays at 3000 rpm. */
    start_fired(3000.0, 1.0);
    rb_engine_settle(&e, 100, 1e-5);
    const double torque = e.result.brake_torque;
    const double w0 = 3000.0 * 3.14159265358979323846 / 30.0;
    e.hold_speed = 0;
    rb_engine_step(&e);
    CHECK(e.idle_valve == 0.0);                   /* the computer knows the speed at once */
    const long steps = 192000;
    double sum = 0.0;
    for (long k = 0; k < 2 * steps; k++) {
        e.load_torque = torque * (e.omega / w0) * (e.omega / w0);
        rb_engine_step(&e);
        if (k >= steps) sum += e.rpm;
    }
    CHECK_NEAR(sum / steps, 3000.0, 15.0);
    CHECK_NEAR(e.kinetic, crank_books(), 1e-9 * fabs(e.crank_gas_work));

    /* Free into the rev limiter, then held by the dyno again: nothing stays cut. */
    e.load_torque = 0.0;
    double hi;
    run_for(1.0, NULL, &hi);
    CHECK(e.spark_cut || hi > e.desc.rev_limit);
    e.hold_speed = 1;
    e.rpm = 3000.0;
    rb_engine_step(&e);
    CHECK(!e.spark_cut && !e.fuel_cut);
    CHECK(run_cycles(3));
    CHECK(e.result.imep_net > 10e5);
}

void test_engine_burns_at_rest(void)
{
    /* Below 100 rpm the gas sets its own pace: a crank that stops with a burn
     * under way still finishes it, and the hot gas still cools on the walls. */
    start_fired(3000.0, 1.0);
    rb_engine_settle(&e, 100, 1e-4);
    CHECK(run_to_angle(705.0, 706.0));            /* 10 degrees after the spark */
    const rb_burn *b = &e.burn[0];
    CHECK(b->burning && b->fraction < 0.2);
    e.rpm = 0.0;                                  /* the crank stops dead */
    run_for(0.05, NULL, NULL);
    const double T = e.cyl[0].gas.T;
    run_for(0.15, NULL, NULL);
    CHECK(b->fraction == 1.0 && !b->burning);     /* 60 degrees at 100 rpm take 0.1 s */
    CHECK(e.cyl[0].gas.T < T - 100.0);

    /* A charge stopped part-way through its compression, no flame in it, cools too. */
    start_fired(3000.0, 1.0);
    rb_engine_settle(&e, 100, 1e-4);
    CHECK(run_to_angle(680.0, 681.0));
    CHECK(!e.burn[0].after_spark && e.cyl[0].gas.T > 550.0);
    e.rpm = 0.0;
    const double T0 = e.cyl[0].gas.T;
    run_for(0.5, NULL, NULL);
    CHECK(e.cyl[0].gas.T < T0 - 25.0);
}

void test_engine_starter_motor(void)
{
    /* The starter is a DC motor geared to the crank: its full torque when
     * stalled, falling in a straight line to nothing at its free speed, and
     * nothing above that (its one-way clutch lets the engine run away). */
    static const double share[4] = { 0.0, 0.5, 0.9, 1.2 };
    start_free();
    const double free_w = e.desc.starter_free_rpm * 3.14159265358979323846 / 30.0;
    const double stall = e.desc.starter_torque;
    for (int k = 0; k < 4; k++) {
        e.starter = 1;
        e.omega = share[k] * free_w;
        e.kinetic = 0.5 * e.desc.inertia * e.omega * e.omega;
        rb_engine_step(&e);
        CHECK_NEAR(e.starter_torque_now, share[k] < 1.0 ? stall * (1.0 - share[k]) : 0.0, 1e-9 * stall);
    }
    e.starter = 0;
    e.omega = 0.0;
    e.kinetic = 0.0;
    rb_engine_step(&e);
    CHECK(e.starter_torque_now == 0.0);
}
