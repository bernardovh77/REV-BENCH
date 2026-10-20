/*
 * test_noise.c - stage 12b: the flow's noise and the engine's own sound
 * (noise.h), and how the listener hears them, the cabin too (audio.h).
 */
#include "audio.h"
#include "engine.h"
#include "noise.h"
#include "presets.h"
#include "rb.h"
#include "rb_internal.h"   /* the sim's layers and listener */
#include "rb_test.h"
#include "valve.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DT      (1.0 / 192000.0)
#define DT_FAST (1.0 / 96000.0)

static rb_engine e;        /* about 320 KB: kept off the stack */
static rb_noise n, n2;     /* about 70 KB each */
static rb_listener l;      /* about 200 KB */

static void preset_desc(const char *key, rb_engine_desc *d)
{
    rb_preset_desc(rb_preset_find(key), d);
}

/* The RMS of output k over `steps` steps of the flow's noise alone; the
 * engine isn't stepped, so its state is what the test made it. */
static double flow_rms(rb_noise *x, int k, long steps)
{
    double sum = 0.0;
    for (long s = 0; s < steps; s++) {
        rb_noise_step(x, &e, RB_NOISE_FLOW);
        sum += x->out[k] * x->out[k];
    }
    return sqrt(sum / steps);
}

static double db(double ratio)
{
    return 20.0 * log10(ratio);
}

/* ---- the flow ---- */

/* What Lighthill's law gives a jet of u m/s out of tailpipe s, at 1 m (Pa
 * RMS), as hot as the gas in the tailpipe's end; less what the band leaves
 * out. The jet's spectrum falls from fc = 0.2 u / D, the band takes off
 * what's below fh = 150 Hz and fades above ft = 6 kHz: one-pole filters
 * each, which let through fc ft^2 / ((fh + fc) (fc + ft) (ft + fh)) of
 * the power (the integral of x^2 / ((x^2 + a^2) (x^2 + b^2) (x^2 + c^2)) is
 * pi / (2 (a + b) (b + c) (c + a))). */
static double jet_expected(int s, double u)
{
    const rb_pipe *t = &e.tailpipe[s];
    const double D = e.desc.tailpipe_diameter;
    const double rho0 = e.exhaust_air.rho, T0 = e.exhaust_air.T;
    const double c0 = sqrt(1.4 * e.exhaust_air.gas.R * T0);
    const double w = 4e-5 * rho0 * pow(u, 8) * D * D / pow(c0, 5) * t->cell[t->cells - 1].T / T0;
    const double fc = 0.2 * u / D, fh = 150.0, ft = 6000.0;
    const double band = fc * ft * ft / ((fh + fc) * (fc + ft) * (ft + fh));
    return sqrt(rho0 * c0 * w / (4.0 * PI) * band);
}

void test_noise_jet_eighth_power(void)
{
    /* A tailpipe's jet radiates Lighthill's power, K rho0 U^8 D^2 / c0^5,
     * times T / T0 for a hot one; a metre away sqrt(rho0 c0 W / (4 pi)),
     * less what's below 150 Hz (the pipes' own) and the fading above 6 kHz:
     * at 100 m/s and 600 K, within 0.5 dB. Twice as fast is 2^8 times the
     * power, 48.2 dB, to within 1 dB (its band moves up with the speed).
     * Drawing air in, there's no jet. With two tailpipes, each one's jet is
     * its own. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    rb_pipe *t = &e.tailpipe[0];
    t->cell[t->cells - 1].T = 600.0;
    const double A = PI / 4.0 * d.tailpipe_diameter * d.tailpipe_diameter;
    double rms[2];
    for (int k = 0; k < 2; k++) {
        e.tail_flow[0] = 100.0 * (k + 1) * A;
        rb_noise_init(&n, &e, 7);
        flow_rms(&n, RB_NOISE_TAIL0, 20000);   /* the filters settle */
        rms[k] = flow_rms(&n, RB_NOISE_TAIL0, 192000);
    }
    CHECK_NEAR(db(rms[0] / jet_expected(0, 100.0)), 0.0, 0.5);
    CHECK_NEAR(db(rms[1] / rms[0]), 10.0 * log10(256.0), 1.0);
    e.tail_flow[0] = -100.0 * A;
    rb_noise_init(&n, &e, 7);
    CHECK_NEAR(flow_rms(&n, RB_NOISE_TAIL0, 1000), 0.0, 0.0);

    preset_desc("v8-cross", &d);
    CHECK_EQ_INT(d.exhaust_systems, 2);
    rb_engine_init(&e, &d, DT);
    e.tail_flow[0] = 0.0;
    e.tail_flow[1] = 100.0 * PI / 4.0 * d.tailpipe_diameter * d.tailpipe_diameter;
    rb_noise_init(&n, &e, 7);
    flow_rms(&n, RB_NOISE_TAIL1, 20000);
    CHECK_NEAR(db(flow_rms(&n, RB_NOISE_TAIL1, 192000) / jet_expected(1, 100.0)), 0.0, 0.5);
    CHECK_NEAR(flow_rms(&n, RB_NOISE_TAIL0, 1000), 0.0, 0.0);
}

void test_noise_valve_jets(void)
{
    /* An exhaust valve blowing down. Its noise comes out of the tailpipe
     * when it gets there (header, exhaust pipe, muffler and tailpipe at
     * 580 m/s), not a step before. Below choking its power goes as
     * M^5 mdot U^2: through the same gap, twice the flow is 2^8 the power,
     * 24.1 dB. Choked, the gas can't go faster: twice the flow, twice the
     * power, 3 dB. Choked through a chambered can, it's at the level of
     * its law: a share 2e-3 of the jet's stream power 1/2 mdot c^2, half
     * of that through the catalyst and 0.004 through the can, within
     * 0.5 dB. The muffler lets through its share of the power: a straight
     * pipe's pressure is sqrt(1 / 0.08) times a straight-through's (11 dB),
     * which is sqrt(0.08 / 0.004) times a chambered can's (13 dB). With two
     * systems, a cylinder's noise comes out of its own tailpipe. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    double r[RB_MUFFLER_CHAMBERED + 1][4], law = 0.0;
    for (int m = RB_MUFFLER_NONE; m <= RB_MUFFLER_CHAMBERED; m++) {
        d.muffler_type = m;
        rb_engine_init(&e, &d, DT);
        rb_orifice *v = &e.exhaust[0];
        v->area = 2e-4;   /* barely open */
        const rb_chamber *cyl = v->a;
        const double sonic = cyl->rho * v->area * v->cd * sqrt(1.32 * cyl->gas.R * cyl->T);
        const double flows[4] = { 0.05 * sonic, 0.1 * sonic, 2.0 * sonic, 4.0 * sonic };
        if (m == RB_MUFFLER_CHAMBERED) {
            const double c = sqrt(1.32 * cyl->gas.R * cyl->T), rho0 = e.exhaust_air.rho;
            const double c0 = sqrt(1.4 * e.exhaust_air.gas.R * e.exhaust_air.T);
            law = sqrt(rho0 * c0 / (4.0 * PI) * 2e-3 * 0.5 * flows[2] * c * c * 0.5 * 0.004);
        }
        for (int k = 0; k < 4; k++) {
            v->mdot = flows[k];
            rb_noise_init(&n, &e, 3);
            if (m == RB_MUFFLER_CHAMBERED && k == 0) {
                const double path = d.header_length[0] + d.exhaust_pipe_length + d.muffler_length
                                  + d.tailpipe_length;
                CHECK_EQ_INT(n.blow_delay[0], lround(path / 580.0 / DT));
                int first = -1;
                for (int s = 0; s < n.blow_delay[0] + 10 && first < 0; s++) {
                    rb_noise_step(&n, &e, RB_NOISE_FLOW);
                    if (n.out[RB_NOISE_TAIL0] != 0.0) first = s;
                }
                CHECK_EQ_INT(first, n.blow_delay[0]);
            }
            flow_rms(&n, RB_NOISE_TAIL0, n.blow_delay[0] + 20000);
            r[m][k] = flow_rms(&n, RB_NOISE_TAIL0, 192000);
        }
    }
    const int c = RB_MUFFLER_CHAMBERED;
    CHECK_NEAR(db(r[c][1] / r[c][0]), db(16.0), 0.5);
    CHECK_NEAR(db(r[c][3] / r[c][2]), 10.0 * log10(2.0), 0.5);
    CHECK_NEAR(db(r[c][2] / law), 0.0, 0.5);
    CHECK_NEAR(db(r[RB_MUFFLER_NONE][2] / r[RB_MUFFLER_STRAIGHT][2]), 10.0 * log10(1.0 / 0.08), 0.5);
    CHECK_NEAR(db(r[RB_MUFFLER_STRAIGHT][2] / r[c][2]), 10.0 * log10(0.08 / 0.004), 0.5);

    preset_desc("v8-cross", &d);
    rb_engine_init(&e, &d, DT);
    int i = 0;
    while (i < d.cylinders && d.collector[i] != 1) i++;
    CHECK(i < d.cylinders);
    rb_orifice *v = &e.exhaust[i];
    v->area = 2e-4;
    v->mdot = 2.0 * v->a->rho * v->area * v->cd * sqrt(1.32 * v->a->gas.R * v->a->T);
    rb_noise_init(&n, &e, 3);
    flow_rms(&n, RB_NOISE_TAIL1, n.blow_delay[i] + 20000);
    CHECK(flow_rms(&n, RB_NOISE_TAIL1, 20000) > 0.0);
    CHECK_NEAR(flow_rms(&n, RB_NOISE_TAIL0, 20000), 0.0, 0.0);
}

void test_noise_throttle_hiss(void)
{
    /* The throttle's jet is heard at the intake's mouth, choked as at idle:
     * twice the flow, twice the power (3 dB); none, no hiss. The exhaust's
     * noise stays at the tailpipe. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    rb_orifice *t = &e.throttle;
    t->area = 1e-5;
    const rb_chamber *box = t->a;
    const double sonic = box->rho * t->area * t->cd * sqrt(1.4 * box->gas.R * box->T);
    double r[2];
    for (int k = 0; k < 2; k++) {
        t->mdot = 2.0 * (k + 1) * sonic;
        rb_noise_init(&n, &e, 11);
        flow_rms(&n, RB_NOISE_MOUTH, 20000);
        r[k] = flow_rms(&n, RB_NOISE_MOUTH, 192000);
        CHECK_NEAR(flow_rms(&n, RB_NOISE_TAIL0, 100), 0.0, 0.0);
    }
    CHECK(r[0] > 0.0);
    CHECK_NEAR(db(r[1] / r[0]), 10.0 * log10(2.0), 0.5);
    t->mdot = 0.0;
    rb_noise_init(&n, &e, 11);
    CHECK_NEAR(flow_rms(&n, RB_NOISE_MOUTH, 1000), 0.0, 0.0);
    /* The idle valve beside the throttle, choked too, hisses as the
     * throttle would with the same flow. */
    rb_orifice *iv = &e.idle_air;
    iv->area = t->area;
    iv->mdot = 2.0 * sonic;
    rb_noise_init(&n, &e, 11);
    flow_rms(&n, RB_NOISE_MOUTH, 20000);
    CHECK_NEAR(db(flow_rms(&n, RB_NOISE_MOUTH, 192000) / r[0]), 0.0, 0.5);
}

/* ---- the engine ---- */

/* Steps the engine and its noise together `steps` times. */
static void run(rb_noise *x, long steps, int what)
{
    for (long s = 0; s < steps; s++) {
        rb_engine_step(&e);
        rb_noise_step(x, &e, what);
    }
}

void test_noise_impacts_per_cycle(void)
{
    /* Every cycle each cylinder's piston slaps the bore four times (at each
     * dead centre), its valves seat twice and lift off twice, and when it's
     * fired its injector clicks once; motored, never. Three whole cycles of
     * the four at 3000 rpm, counted from part-way through one. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    for (int fired = 0; fired < 2; fired++) {
        rb_engine_init(&e, &d, DT_FAST);
        e.rpm = 3000.0;
        e.throttle_pos = 1.0;
        e.ignition = fired;
        rb_noise_init(&n, &e, 5);
        run(&n, 1001, RB_NOISE_ENGINE);
        memset(n.impacts, 0, sizeof n.impacts);
        run(&n, 11520, RB_NOISE_ENGINE);   /* 0.1875 degrees a step: 2160 degrees */
        CHECK_EQ_INT(n.impacts[RB_IMPACT_SLAP], 3 * 4 * d.cylinders);
        CHECK_EQ_INT(n.impacts[RB_IMPACT_SEAT], 3 * 2 * d.cylinders);
        CHECK_EQ_INT(n.impacts[RB_IMPACT_LIFT], 3 * 2 * d.cylinders);
        CHECK_EQ_INT(n.impacts[RB_IMPACT_INJECTOR], fired ? 3 * d.cylinders : 0);
    }
}

/* The four posed by hand: its gas at p_ambient + dp everywhere, turning at
 * omega, its crank about to cross `angle` (cylinder 1's cycle angle):
 * rb_noise_init, then one step of the noise with the crank moved 0.1 degree
 * on and nothing else stepped. */
static void pose(double omega, double dp, double angle)
{
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    for (int i = 0; i < d.cylinders; i++) e.cyl[i].gas.p = d.p_ambient + dp;
    e.omega = omega;
    e.ignition = 1;
    e.theta = fmod(e.desc.firing[0] + angle - 0.05 + 720.0, 720.0);
    rb_noise_init(&n, &e, 1);
    e.theta = fmod(e.theta + 0.1, 720.0);
    rb_noise_step(&n, &e, RB_NOISE_ENGINE);
}

void test_noise_strikes(void)
{
    /* Each blow as big as its physics says, on the four posed by hand.
     * Its pistons cross their dead centres together, two at TDC, two at BDC.
     * With the gas at the outside's pressure only the parts' inertia pushes
     * them, m r w^2 (1 + lambda) at TDC and m r w^2 (1 - lambda) at BDC:
     * twice the speed, 16 times the energy. A gas force G as big as the
     * inertia I adds to one and takes from the other, so the energy goes
     * from (1 + lambda)^2 + (1 - lambda)^2 to lambda^2 + (2 - lambda)^2
     * (in I^2). Cylinder 1 crossing each of its valve events makes one
     * impact: a lift at IVO (and the injector's click) and EVO, a seat at
     * IVC and EVC. */
    pose(300.0, 0.0, 0.0);
    CHECK_EQ_INT(n.impacts[RB_IMPACT_SLAP], 4);
    const double slow = n.impact_energy[RB_IMPACT_SLAP];
    pose(600.0, 0.0, 0.0);
    CHECK(slow > 0.0);
    CHECK_NEAR(n.impact_energy[RB_IMPACT_SLAP] / slow, 16.0, 1e-9);
    const double lambda = 0.5 * e.desc.stroke / e.desc.rod;
    const double inertia = n.recip_mass * 0.5 * e.desc.stroke * 300.0 * 300.0;
    pose(300.0, inertia / e.cyl[0].geom.area, 0.0);
    CHECK_NEAR(n.impact_energy[RB_IMPACT_SLAP] / slow,
               (lambda * lambda + (2.0 - lambda) * (2.0 - lambda))
               / ((1.0 + lambda) * (1.0 + lambda) + (1.0 - lambda) * (1.0 - lambda)), 1e-9);

    const rb_engine_desc *d = &e.desc;
    const double events[4] = { rb_valve_opens(&d->intake), rb_valve_closes(&d->intake),
                               rb_valve_opens(&d->exhaust), rb_valve_closes(&d->exhaust) };
    const int kinds[4] = { RB_IMPACT_LIFT, RB_IMPACT_SEAT, RB_IMPACT_LIFT, RB_IMPACT_SEAT };
    for (int k = 0; k < 4; k++) {
        pose(300.0, 0.0, events[k]);
        for (int j = 0; j < RB_IMPACT_KINDS; j++)
            CHECK_EQ_INT(n.impacts[j], (j == kinds[k]) || (j == RB_IMPACT_INJECTOR && k == 0));
    }

    /* Combustion: a step in the pistons' force strikes the modes once, as
     * hard as the step: twice the step, twice the ring. Nothing changing,
     * nothing strikes, and 50 ms on the ring has died away. */
    double peak[2] = { 0.0, 0.0 }, late = 0.0;
    for (int k = 0; k < 2; k++) {
        pose(0.0, 0.0, 100.0);
        CHECK(n.out[RB_NOISE_BAY] == 0.0);
        e.cyl[0].gas.p += 1e5 * (k + 1);
        for (int s = 0; s < 9600 + 1920; s++) {
            rb_noise_step(&n, &e, RB_NOISE_ENGINE);
            if (s < 1920) peak[k] = fmax(peak[k], fabs(n.out[RB_NOISE_BAY]));
            else if (s >= 9600 && k == 0) late = fmax(late, fabs(n.out[RB_NOISE_BAY]));
        }
    }
    CHECK(peak[0] > 0.0);
    CHECK_NEAR(peak[1] / peak[0], 2.0, 1e-9);
    CHECK(late < 1e-4 * peak[0]);
}

/* The mean size squared of the impacts of `kind` over 4 cycles of the four,
 * settled at rpm and throttle, fired. */
static double impact_energy(double rpm, double throttle, int kind)
{
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.rpm = rpm;
    e.throttle_pos = throttle;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-4);
    rb_noise_init(&n, &e, 9);
    run(&n, lround(4.0 * 120.0 / rpm / DT_FAST), RB_NOISE_ENGINE);
    return n.impacts[kind] ? n.impact_energy[kind] / n.impacts[kind] : 0.0;
}

void test_noise_ticks_and_clatter(void)
{
    /* A valve seats as fast as the cam's ramp brings it down, which goes as
     * the rpm: twice the rpm, twice each tick (4 times its energy). Piston
     * slap goes with the force on the piston: wide open, the slaps are 1.5
     * times as hard (on average) as with the throttle nearly shut; only one
     * of each piston's four is after firing TDC, where the load counts. */
    CHECK_NEAR(impact_energy(3000.0, 1.0, RB_IMPACT_SEAT) / impact_energy(1500.0, 1.0, RB_IMPACT_SEAT),
               4.0, 1e-9);
    const double open = impact_energy(3000.0, 1.0, RB_IMPACT_SLAP);
    const double shut = impact_energy(3000.0, 0.05, RB_IMPACT_SLAP);
    CHECK(sqrt(open / shut) > 1.5);
}

void test_noise_modes_follow_the_bore(void)
{
    /* A bigger block rings lower: each mode moves by sqrt(0.086 / bore).
     * The pole of y = x + a1 y1 - a2 y2 is at r e^(i w dt), so cos(w dt) =
     * a1 / (2 sqrt(a2)). */
    rb_engine_desc d;
    double f[2][RB_NOISE_MODES], bore[2];
    const char *keys[2] = { "i4", "v8-cross" };
    for (int j = 0; j < 2; j++) {
        preset_desc(keys[j], &d);
        bore[j] = d.bore;
        rb_engine_init(&e, &d, DT);
        rb_noise_init(&n, &e, 1);
        for (int k = 0; k < RB_NOISE_MODES; k++)
            f[j][k] = acos(n.mode[k].a1 / (2.0 * sqrt(n.mode[k].a2))) / (2.0 * PI * DT);
    }
    CHECK(bore[0] != bore[1]);
    for (int k = 0; k < RB_NOISE_MODES; k++) CHECK_NEAR(f[1][k] / f[0][k], sqrt(bore[0] / bore[1]), 1e-9);
    CHECK_NEAR(f[0][0], 950.0 * sqrt(0.086 / bore[0]), 1e-6);
    /* ... and each rings for Q / (pi f): its pole's radius is e^(-pi f dt / Q)
     * (the last engine's, the V8's). */
    CHECK_NEAR(-log(sqrt(n.mode[0].a2)) / (PI * DT), f[1][0] / 10.0, 1e-6);
}

void test_noise_repeatable(void)
{
    /* The same seed makes the same noise; another seed, other noise. A
     * stopped engine makes none at all, and a broken state upstream (a NaN
     * pressure) makes silence, not NaNs, and the noise carries on after. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT_FAST);
    e.rpm = 3000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-4);
    rb_noise_init(&n, &e, 21);
    rb_noise_init(&n2, &e, 21);
    int same = 1;
    double diff = 0.0, sum = 0.0;
    static rb_noise other;
    rb_noise_init(&other, &e, 22);
    for (int s = 0; s < 20000; s++) {
        rb_engine_step(&e);
        rb_noise_step(&n, &e, RB_NOISE_FLOW | RB_NOISE_ENGINE);
        rb_noise_step(&n2, &e, RB_NOISE_FLOW | RB_NOISE_ENGINE);
        rb_noise_step(&other, &e, RB_NOISE_FLOW | RB_NOISE_ENGINE);
        for (int k = 0; k < RB_NOISE_OUTPUTS; k++) same &= (n.out[k] == n2.out[k]);
        diff += pow(n.out[RB_NOISE_TAIL0] - other.out[RB_NOISE_TAIL0], 2);
        sum += pow(n.out[RB_NOISE_TAIL0], 2);
    }
    CHECK(same);
    CHECK(sum > 0.0 && diff > 0.5 * sum);

    rb_engine_init(&e, &d, DT_FAST);   /* at rest, held at 0 rpm: no impacts, and */
    e.rpm = 0.0;                        /* the gas settling makes 100 uPa at most  */
    rb_noise_init(&n, &e, 21);
    double loudest = 0.0;
    for (int s = 0; s < 10000; s++) {
        rb_engine_step(&e);
        rb_noise_step(&n, &e, RB_NOISE_FLOW | RB_NOISE_ENGINE);
        for (int k = 0; k < RB_NOISE_OUTPUTS; k++) loudest = fmax(loudest, fabs(n.out[k]));
    }
    CHECK(loudest < 1e-4);
    for (int k = 0; k < RB_IMPACT_KINDS; k++) CHECK_EQ_INT(n.impacts[k], 0);

    rb_engine_init(&e, &d, DT_FAST);
    e.rpm = 3000.0;
    e.throttle_pos = 1.0;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-4);
    rb_noise_init(&n, &e, 21);
    const double p = e.cyl[0].gas.p, theta = e.theta;
    e.cyl[0].gas.p = NAN;
    int finite = 1;
    for (int s = 0; s < 2000; s++) {   /* the crank by hand through 200 degrees: */
        e.theta = fmod(theta + 0.1 * (s + 1), 720.0);   /* cylinder 1 slaps, NaN */
        rb_noise_step(&n, &e, RB_NOISE_FLOW | RB_NOISE_ENGINE);
        for (int k = 0; k < RB_NOISE_OUTPUTS; k++) finite &= isfinite(n.out[k]);
    }
    CHECK(finite);
    e.cyl[0].gas.p = p;
    e.theta = theta;
    double after = 0.0;
    for (int s = 0; s < 20000; s++) {
        rb_engine_step(&e);
        rb_noise_step(&n, &e, RB_NOISE_FLOW | RB_NOISE_ENGINE);
        after = fmax(after, fabs(n.out[RB_NOISE_BAY]));
    }
    CHECK(after > 0.0 && isfinite(after));
}

/* |H| at f of a one-pole low-pass y += a (x - y) stepping every dt, or of
 * the high-pass x less it. */
static double one_pole_gain(double f, double fc, double dt, int high)
{
    const double a = 1.0 - exp(-2.0 * PI * fc * dt), w = 2.0 * PI * f * dt;
    const double dr = 1.0 - (1.0 - a) * cos(w), di = (1.0 - a) * sin(w), d2 = dr * dr + di * di;
    const double re = a * dr / d2, im = -a * di / d2;   /* a / (1 - (1 - a) e^(-i w)) */
    return high ? hypot(1.0 - re, im) : hypot(re, im);
}

void test_noise_mounts(void)
{
    /* The mounts pass the gas torque's swings, 20-200 Hz, not its mean.
     * Fed by hand, a tone in the torque on top of a mean comes out as their
     * filters (a one-pole high-pass at 20 Hz, then a low-pass at 200 Hz)
     * pass it: 2 kHz against 100 Hz to 0.5 % of their responses' ratio, and
     * the mean not at all. On the four, wide open, its firing (100 Hz at
     * 3000 rpm) comes through far stronger than at idle-like light load,
     * and what's left of 2 kHz is under a hundredth of it. */
    rb_engine_desc d;
    preset_desc("i4", &d);
    rb_engine_init(&e, &d, DT);
    e.omega = 0.0;   /* nothing strikes */
    double amp[2], dc = 0.0;
    const double tone[2] = { 100.0, 2000.0 };
    for (int k = 0; k < 2; k++) {
        rb_noise_init(&n, &e, 13);
        const long settle = lround(0.5 / DT), count = lround(0.2 / DT);
        double re = 0.0, im = 0.0;
        dc = 0.0;
        for (long s = 0; s < settle + count; s++) {
            e.gas_torque = 50.0 + 100.0 * sin(2.0 * PI * tone[k] * s * DT);
            rb_noise_step(&n, &e, RB_NOISE_ENGINE);
            if (s < settle) continue;
            re += n.out[RB_NOISE_MOUNT] * cos(2.0 * PI * tone[k] * s * DT);
            im += n.out[RB_NOISE_MOUNT] * sin(2.0 * PI * tone[k] * s * DT);
            dc += n.out[RB_NOISE_MOUNT] / count;
        }
        amp[k] = 2.0 * hypot(re, im) / count;
    }
    double expect[2];
    for (int k = 0; k < 2; k++)
        expect[k] = one_pole_gain(tone[k], 20.0, DT, 1) * one_pole_gain(tone[k], 200.0, DT, 0);
    CHECK(amp[0] > 0.0);
    CHECK_NEAR(amp[1] / amp[0], expect[1] / expect[0], 0.005 * expect[1] / expect[0]);
    CHECK(fabs(dc) < 1e-3 * amp[0]);

    double rms[2];
    for (int k = 0; k < 2; k++) {
        rb_engine_init(&e, &d, DT_FAST);
        e.rpm = 3000.0;
        e.throttle_pos = k ? 1.0 : 0.05;
        e.ignition = 1;
        rb_engine_settle(&e, 100, 1e-4);
        rb_noise_init(&n, &e, 13);
        run(&n, 9600, RB_NOISE_ENGINE);   /* the mean settles */
        enum { N = 9600 };   /* 0.1 s: five firings of 100 Hz ... */
        static double x[N];
        double sum = 0.0, mean = 0.0;
        for (int s = 0; s < N; s++) {
            rb_engine_step(&e);
            rb_noise_step(&n, &e, RB_NOISE_ENGINE);
            x[s] = n.out[RB_NOISE_MOUNT];
            sum += x[s] * x[s];
            mean += x[s] / N;
        }
        rms[k] = sqrt(sum / N);
        CHECK(fabs(mean) < 0.05 * rms[k]);
        if (k) {   /* ... and 200 periods of 2 kHz */
            double re1 = 0, im1 = 0, re2 = 0, im2 = 0;
            for (int s = 0; s < N; s++) {
                re1 += x[s] * cos(2.0 * PI * 100.0 * s * DT_FAST);
                im1 += x[s] * sin(2.0 * PI * 100.0 * s * DT_FAST);
                re2 += x[s] * cos(2.0 * PI * 2000.0 * s * DT_FAST);
                im2 += x[s] * sin(2.0 * PI * 2000.0 * s * DT_FAST);
            }
            CHECK(hypot(re2, im2) < 0.01 * hypot(re1, im1));
        }
    }
    CHECK(rms[1] > 2.0 * rms[0]);
}

/* ---- how they're heard ---- */

void test_audio_noise_sources_placed(void)
{
    /* rb_listener_place_noise puts each tailpipe's and the mouth's noise
     * where their flows are, heard at the same distance and as late (the
     * same delay in seconds, physics steps against pipe steps), and the
     * engine bay muffled by the body except over the open bonnet. In the
     * cabin the engine bay is nearer than any flow source, so the flows
     * wait for it: every path's delay still counts its length. */
    const double c = 343.0, pipe_dt = 1.0 / 48000.0, dt = 1.0 / 192000.0;
    for (int where = 0; where < RB_HEAR_COUNT; where++) {
        CHECK_EQ_INT(rb_listener_place(&l, where, 2, c, pipe_dt), 0);
        CHECK_EQ_INT(rb_listener_place_noise(&l, where, 2, c, pipe_dt, dt), 0);
        CHECK_EQ_INT(l.inside, where == RB_HEAR_CABIN);
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 2; k++) {
                CHECK_NEAR(l.ngain[j][k], l.gain[j][k], 1e-12);
                CHECK_NEAR(l.ndelay[j][k] * dt, l.delay[j][k] * pipe_dt, 1e-9);
            }
        CHECK(l.ngain[3][0] > 0.0);
        double nearest = INFINITY;   /* the nearest path, whichever source's, is heard first */
        for (int j = 0; j < RB_LISTEN_NOISES; j++) nearest = fmin(nearest, l.ndelay[j][0]);
        CHECK_NEAR(nearest, 0.0, 1e-9);
    }
    /* The bay, 0.3 as loud behind the car as it would be in the open. */
    rb_listener_place(&l, RB_HEAR_FRONT, 1, c, pipe_dt);
    rb_listener_place_noise(&l, RB_HEAR_FRONT, 1, c, pipe_dt, dt);
    CHECK(l.ngain[1][0] == 0.0 && l.ngain[1][1] == 0.0);   /* one tailpipe */
    const double front = l.ngain[3][0];
    rb_listener_place(&l, RB_HEAR_BEHIND, 1, c, pipe_dt);
    rb_listener_place_noise(&l, RB_HEAR_BEHIND, 1, c, pipe_dt, dt);
    const double bay[3] = { 0.0, 3.3, 0.6 }, front_at[3] = { -0.1, 3.4, 1.3 }, behind_at[3] = { 0.6, -3.0, 1.6 };
    const double r_front = hypot(hypot(bay[0] - front_at[0], bay[1] - front_at[1]), bay[2] - front_at[2]);
    const double r_behind = hypot(hypot(bay[0] - behind_at[0], bay[1] - behind_at[1]), bay[2] - behind_at[2]);
    CHECK_NEAR(front, 1.0 / r_front, 1e-12);
    CHECK_NEAR(l.ngain[3][0], 0.3 / r_behind, 1e-12);
    /* A set listener (the tests' own) hears no noise sources at all. */
    const double src[3] = { 0.0, 0.0, 1.0 }, at[3] = { 0.0, 1.0, 1.0 };
    const double *pos[RB_LISTEN_SOURCES] = { src, NULL, NULL };
    rb_listener_set(&l, pos, at, 0.0, c, pipe_dt);
    for (int j = 0; j < RB_LISTEN_NOISES; j++) CHECK(l.ngain[j][0] == 0.0 && l.ngain[j][1] == 0.0);
    CHECK_EQ_INT(rb_listener_place_noise(&l, RB_HEAR_COUNT, 1, c, pipe_dt, dt), -1);
    CHECK_EQ_INT(rb_listener_place_noise(&l, RB_HEAR_FRONT, 1, c, pipe_dt, 0.0), -1);
}

/* What the cabin makes of a tone of f Hz from noise source j (direct path
 * only) or the mounts (j < 0), against the same tone outside: the ratio of
 * the amplitudes. */
static double cabin_gain(int j, double f)
{
    const double c = 343.0, pipe_dt = 1.0 / 48000.0, dt = 1.0 / 192000.0;
    rb_listener_place(&l, RB_HEAR_CABIN, 1, c, pipe_dt);
    rb_listener_place_noise(&l, RB_HEAR_CABIN, 1, c, pipe_dt, dt);
    for (int k = 0; k < RB_LISTEN_NOISES; k++) l.ngain[k][1] = 0.0;   /* no ground */
    const double outside = (j < 0) ? 1.0 : l.ngain[j][0];
    const long settle = lround(0.5 / dt), count = lround(0.2 / dt);   /* whole periods of 55 Hz up */
    double re = 0.0, im = 0.0;
    for (long s = 0; s < settle + count; s++) {
        double p[RB_LISTEN_NOISES] = { 0.0 };
        const double x = sin(2.0 * PI * f * s * dt);
        if (j >= 0) p[j] = x;
        rb_listener_push_noise(&l, p, (j < 0) ? x : 0.0);
        const double y = rb_listener_hear(&l, 1.2, dt);
        if (s >= settle) {
            re += y * cos(2.0 * PI * f * s * dt);
            im += y * sin(2.0 * PI * f * s * dt);
        }
    }
    return 2.0 * hypot(re, im) / count / outside;
}

/* |H| at f of the cabin's one-pole low-pass at fc (stepping every dt) and
 * of its boom. */
static double cabin_expected(double f, double fc, int body)
{
    const double dt = 1.0 / 192000.0, w = 2.0 * PI * f * dt;
    double h = 1.0;
    if (body) {
        const double a = 1.0 - exp(-2.0 * PI * fc * dt);
        h = RB_CABIN_PASS * a / hypot(1.0 - (1.0 - a) * cos(w), (1.0 - a) * sin(w));
    }
    const double A = pow(10.0, RB_CABIN_BOOM_DB / 40.0), w0 = 2.0 * PI * RB_CABIN_BOOM_HZ * dt;
    const double alpha = sin(w0) / (2.0 * RB_CABIN_BOOM_Q), cw = cos(w0);
    const double b0 = 1.0 + alpha * A, b1 = -2.0 * cw, b2 = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A, a1 = -2.0 * cw, a2 = 1.0 - alpha / A;
    const double nr = b0 + b1 * cos(w) + b2 * cos(2.0 * w), ni = -b1 * sin(w) - b2 * sin(2.0 * w);
    const double dr = a0 + a1 * cos(w) + a2 * cos(2.0 * w), di = -a1 * sin(w) - a2 * sin(2.0 * w);
    return h * hypot(nr, ni) / hypot(dr, di);
}

void test_audio_cabin(void)
{
    /* In the cabin a source outside comes through the body: RB_CABIN_PASS
     * of it at low frequencies, falling 6 dB an octave above the corner,
     * 100 Hz behind and 200 Hz through the firewall (the engine bay's and
     * the intake mouth's); then the boom, 8 dB up at 55 Hz. So 2 kHz from
     * the tailpipe is down about 36 dB, from the engine bay about 30. The mounts' pressure only gets the boom. Each to
     * 0.5 % of the filters' own response. */
    const double cases[][3] = { { 0, 2000.0, 100.0 }, { 0, 55.0, 100.0 }, { 3, 2000.0, 200.0 },
                                { 3, 55.0, 200.0 }, { 2, 2000.0, 200.0 } };
    for (int k = 0; k < 5; k++) {
        const double got = cabin_gain((int)cases[k][0], cases[k][1]);
        CHECK_NEAR(got, cabin_expected(cases[k][1], cases[k][2], 1), 0.005 * got);
    }
    CHECK(db(cabin_gain(0, 2000.0)) < -33.0);
    CHECK(db(cabin_gain(3, 2000.0)) > db(cabin_gain(0, 2000.0)) + 5.0);
    const double boom = cabin_gain(-1, 55.0);
    CHECK_NEAR(boom, cabin_expected(55.0, 0.0, 0), 0.005 * boom);
    CHECK_NEAR(db(boom), RB_CABIN_BOOM_DB, 0.1);
}

/* The RMS between 2 and 8 kHz (its DFT's bins there, every 4 Hz) of what a
 * sim hears from `where`, the four held at 3000 rpm wide open, only
 * `layers`. */
static double heard_highs(int where, int layers)
{
    rb_sim *sim = rb_create(NULL);
    if (!sim) return NAN;
    sim->source = RB_SOURCE_ENGINE;
    sim->hear = where;
    sim->layers = layers;
    rb_sim_listen(sim);
    sim->engine.rpm = 3000.0;
    sim->engine.throttle_pos = 1.0;
    sim->engine.ignition = 1;
    enum { SETTLE = 24000, N = 12000 };
    static float x[SETTLE + N];
    for (int done = 0; done < SETTLE + N; done += 480) rb_render_block(sim, x + done, 480);
    rb_destroy(sim);
    /* Parseval: each bin k of N samples carries 2 |X_k|^2 / N^2 of the
     * signal's mean square. Bin k is k * 48000 / N Hz: 4 Hz apart. */
    const float *y = x + SETTLE;
    double sum = 0.0;
    for (int k = 2000 / 4; k < 8000 / 4; k++) {
        const double w = 2.0 * PI * k / N, c = cos(w), s = sin(w);
        double re = 0.0, im = 0.0, pr = 1.0, pi = 0.0;
        for (int i = 0; i < N; i++) {   /* a phasor turned step by step */
            re += y[i] * pr;
            im += y[i] * pi;
            const double t = pr * c - pi * s;
            pi = pr * s + pi * c;
            pr = t;
        }
        sum += 2.0 * (re * re + im * im) / ((double)N * N);
    }
    return sqrt(sum);
}

void test_audio_engine_bay(void)
{
    /* Over the open bonnet the engine's own sound is what's heard above
     * 2 kHz, over 10 dB above the pipes' and the flow's together; behind
     * the car, muffled by the body and further away, it's under them. */
    const double front_engine = heard_highs(RB_HEAR_FRONT, RB_LAYER_ENGINE);
    const double front_rest = heard_highs(RB_HEAR_FRONT, RB_LAYER_PIPES | RB_LAYER_FLOW);
    const double behind_engine = heard_highs(RB_HEAR_BEHIND, RB_LAYER_ENGINE);
    const double behind_rest = heard_highs(RB_HEAR_BEHIND, RB_LAYER_PIPES | RB_LAYER_FLOW);
    CHECK(front_rest > 0.0 && behind_engine > 0.0);
    CHECK(db(front_engine / front_rest) > 10.0);
    CHECK(behind_engine < behind_rest);
}
