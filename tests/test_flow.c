/*
 * test_flow.c - nozzle formulas, chambers and orifices: conservation,
 * equalisation, blowdown and filling against exact solutions, stiff cases.
 */
#include "flow.h"
#include "rb_test.h"

#include <math.h>

static const double AIR[RB_NSPECIES]    = { [RB_AIR] = 1.0 };
static const double BURNED[RB_NSPECIES] = { [RB_BURNED] = 1.0 };
#define DT (1.0 / 192000.0)   /* the default physics step: 48 kHz x 4 */

void test_nozzle_formulas(void)
{
    const double g = 1.4, R = 287.0, T0 = 300.0, p0 = 5e5, A = 1e-4;
    const double scale = A * p0 / sqrt(R * T0);
    const double rc = pow(2.0 / (g + 1.0), g / (g - 1.0));
    const double choked = rb_nozzle_mdot(A, p0, T0, R, g, 1e5);

    /* Choked: the textbook constant 0.6847 for g = 1.4, whatever the outlet pressure. */
    CHECK_NEAR(rb_nozzle_critical_ratio(1.4), 0.528282, 1e-6);
    CHECK_NEAR(choked / scale, 0.684731, 1e-6);
    CHECK_NEAR(rb_nozzle_mdot(A, p0, T0, R, g, 0.0), choked, 1e-15);
    CHECK_NEAR(rb_nozzle_mdot(A, p0, T0, R, g, 0.999 * rc * p0), choked, 1e-15);

    /* The subsonic formula meets the choked value at the critical ratio. */
    CHECK_NEAR(rb_nozzle_mdot(A, p0, T0, R, g, rc * p0 * (1.0 + 1e-9)), choked, 1e-6 * choked);

    /* Small pressure drops behave like Bernoulli: mdot = A sqrt(2 rho dp). */
    {
        const double dp = 1e-3 * p0, rho = p0 / (R * T0);
        const double bernoulli = A * sqrt(2.0 * rho * dp);
        CHECK_NEAR(rb_nozzle_mdot(A, p0, T0, R, g, p0 - dp), bernoulli, 1e-3 * bernoulli);
    }

    /* More pressure drop, more flow, up to the critical ratio. */
    {
        int rising = 1;
        double prev = 0.0;
        for (int i = 1; i <= 1000; i++) {
            double m = rb_nozzle_mdot(A, p0, T0, R, g, p0 * (1.0 - i * (1.0 - rc) / 1000.0));
            if (!(m > prev)) rising = 0;
            prev = m;
        }
        CHECK(rising);
    }

    /* No pressure drop or nonsense inputs: no flow, and no NaN. */
    CHECK(rb_nozzle_mdot(A, p0, T0, R, g, p0) == 0.0);
    CHECK(rb_nozzle_mdot(A, p0, T0, R, g, 2.0 * p0) == 0.0);
    CHECK(rb_nozzle_mdot(0.0, p0, T0, R, g, 1e5) == 0.0);
    CHECK(rb_nozzle_mdot(A, 0.0, T0, R, g, 0.0) == 0.0);
    CHECK(rb_nozzle_mdot(A, p0, 0.0, R, g, 1e5) == 0.0);
    CHECK(rb_nozzle_mdot(A, p0, T0, R, 1.0, 1e5) == 0.0);
    CHECK(rb_nozzle_mdot(A, p0, T0, R, g, NAN) == 0.0);
    CHECK(rb_nozzle_mdot(NAN, p0, T0, R, g, 1e5) == 0.0);
}

void test_chamber_init_and_repairs(void)
{
    rb_chamber ch;

    rb_chamber_init(&ch, 1e-3, 1e5, 300.0, AIR);
    CHECK_NEAR(ch.p, 1e5, 1e-6);
    CHECK_NEAR(ch.T, 300.0, 1e-9);
    CHECK_NEAR(ch.mass, 1e5 * 1e-3 / (rb_species_R(RB_AIR) * 300.0), 1e-15);
    CHECK_NEAR(ch.rho, 1.161, 0.001);   /* air at 1 bar and 300 K      */
    CHECK_NEAR(ch.c, 347.2, 0.3);       /* speed of sound, m/s         */
    CHECK_NEAR(ch.h, ch.u + ch.gas.R * ch.T, 1e-9);
    CHECK_EQ_INT(ch.repairs, 0);

    /* A NaN energy is rebuilt from the last temperature instead of spreading. */
    ch.U = NAN;
    rb_chamber_update(&ch);
    CHECK_NEAR(ch.T, 300.0, 1e-9);
    CHECK_NEAR(ch.p, 1e5, 1e-6);
    CHECK_EQ_INT(ch.repairs, 1);

    /* A NaN mass is dropped: the chamber ends up empty but finite. */
    ch.m[RB_AIR] = NAN;
    rb_chamber_update(&ch);
    CHECK(ch.p == 0.0 && isfinite(ch.T));
    CHECK_EQ_INT(ch.repairs, 2);

    /* An empty chamber has no pressure and keeps its temperature. */
    rb_chamber_init(&ch, 1e-3, 0.0, 350.0, AIR);
    CHECK(ch.p == 0.0);
    CHECK_NEAR(ch.T, 350.0, 0.0);
}

void test_two_tanks_equalise(void)
{
    rb_chamber a, b;
    rb_orifice o;
    rb_chamber *all[2] = { &a, &b };
    int monotonic = 1;

    rb_chamber_init(&a, 1e-3, 2e5, 300.0, AIR);   /* 1 L at 2 bar */
    rb_chamber_init(&b, 1e-3, 1e5, 300.0, AIR);   /* 1 L at 1 bar */
    rb_orifice_init(&o, &a, &b, 1e-4, 0.8);       /* 1 cm^2 hole  */
    const double m0 = a.mass + b.mass, U0 = a.U + b.U;

    for (int n = 0; n < 38400; n++) {             /* 0.2 s */
        double pa = a.p, pb = b.p;
        rb_network_step(all, 2, &o, 1, DT);
        if (a.p > pa + 1e-6 || b.p < pb - 1e-6) monotonic = 0;
    }
    CHECK_NEAR(a.mass + b.mass, m0, 1e-12 * m0);  /* mass is conserved ... */
    CHECK_NEAR(a.U + b.U, U0, 1e-9);              /* ... and so is energy  */
    CHECK(fabs(a.p - b.p) < 1.0);                 /* the pressures meet    */
    CHECK(monotonic);                             /* without overshooting  */
    /* With constant cv, energy conservation gives the volume-weighted mean
     * pressure (2 + 1) / 2 = 1.5 bar. */
    CHECK_NEAR(a.p, 1.5e5, 1e-3 * 1.5e5);
    /* The gas left in A expanded isentropically: T = T0 (p/p0)^((g-1)/g). */
    CHECK_NEAR(a.T, 300.0 * pow(a.p / 2e5, 0.4 / 1.4), 0.3);
    CHECK(b.T > 300.0);                           /* B was compressed: warmer */
}

/* 10 L at 5 bar blowing down into the atmosphere through 1 cm^2 (Cd = 1). */
static void setup_blowdown(rb_chamber *tank, rb_chamber *atmosphere, rb_orifice *hole)
{
    rb_chamber_init(tank, 10e-3, 5e5, 300.0, AIR);
    rb_chamber_init_reservoir(atmosphere, 1e5, 300.0, AIR);
    rb_orifice_init(hole, tank, atmosphere, 1e-4, 1.0);
}

void test_blowdown_matches_analytic(void)
{
    rb_chamber tank, atmosphere;
    rb_orifice hole;
    rb_chamber *all[2] = { &tank, &atmosphere };
    double worst_p = 0.0, worst_T = 0.0, worst_jump = 0.0, prev_mdot = 0.0;
    int below_ambient = 0;

    setup_blowdown(&tank, &atmosphere, &hole);
    /* While choked, with constant gamma, the exact solution is
     *   p(t) = p0 (1 + K t)^(-2g/(g-1)),  K = (g-1)/2 * C A sqrt(R T0) / V
     * with C = sqrt(g) (2/(g+1))^((g+1)/(2(g-1))). */
    const double g = tank.gamma, R = tank.gas.R;
    const double C = sqrt(g) * pow(2.0 / (g + 1.0), (g + 1.0) / (2.0 * (g - 1.0)));
    const double K = 0.5 * (g - 1.0) * C * 1e-4 * sqrt(R * 300.0) / 10e-3;

    for (long n = 1; n <= 2L * 192000; n++) {     /* 2 s */
        rb_network_step(all, 2, &hole, 1, DT);
        double t = n * DT;
        double p_exact = 5e5 * pow(1.0 + K * t, -2.0 * g / (g - 1.0));
        if (p_exact > 2.0e5) {                     /* choked needs p > 1 bar / 0.528 */
            worst_p = fmax(worst_p, fabs(tank.p - p_exact) / p_exact);
            worst_T = fmax(worst_T, fabs(tank.T - 300.0 * pow(tank.p / 5e5, (g - 1.0) / g)));
        }
        if (hole.mdot > 1e-2 && prev_mdot > 1e-2)  /* smooth, also at the choke switch */
            worst_jump = fmax(worst_jump, fabs(hole.mdot - prev_mdot) / prev_mdot);
        prev_mdot = hole.mdot;
        if (tank.p < 1e5 - 1e-6) below_ambient = 1;
    }
    CHECK(worst_p < 1e-3);           /* within 0.1 % of the exact solution     */
    CHECK(worst_T < 0.2);            /* and of the isentropic temperature (K)  */
    CHECK(worst_jump < 1e-3);
    CHECK(!below_ambient);           /* never undershoots the outside pressure */
    CHECK_NEAR(tank.p, 1e5, 10.0);   /* and empties down to it                 */
    CHECK_NEAR(atmosphere.p, 1e5, 0.0);
    CHECK_EQ_INT(tank.repairs, 0);
}

static double blowdown_time_to_2bar(double dt)
{
    rb_chamber tank, atmosphere;
    rb_orifice hole;
    rb_chamber *all[2] = { &tank, &atmosphere };

    setup_blowdown(&tank, &atmosphere, &hole);
    for (long n = 1; n < 10000000L; n++) {
        rb_network_step(all, 2, &hole, 1, dt);
        if (tank.p < 2e5) return n * dt;
    }
    return NAN;
}

void test_blowdown_converges_with_time_step(void)
{
    const double t_48k  = blowdown_time_to_2bar(1.0 / 48000.0);
    const double t_192k = blowdown_time_to_2bar(1.0 / 192000.0);
    const double t_384k = blowdown_time_to_2bar(1.0 / 384000.0);
    /* exact: (0.4^(-(g-1)/(2g)) - 1) / K with g = 1.4: 0.3480 s */
    CHECK_NEAR(t_384k, 0.3480, 0.001);
    CHECK_NEAR(t_192k, t_384k, 1e-4 * t_384k);   /* the default step is converged ...    */
    CHECK_NEAR(t_48k, t_384k, 1e-3 * t_384k);    /* ... and even 48 kHz is within 0.1 %  */
}

void test_stiff_equalisation_has_no_overshoot(void)
{
    /* A 1 cm^3 pocket of hot burned gas behind a big 5 cm^2 opening into a
     * litre of air: without the limit, one explicit step would overshoot. */
    rb_chamber pocket, room;
    rb_orifice o;
    rb_chamber *all[2] = { &pocket, &room };
    double min_dp = INFINITY;
    int limited_at_first = 0;

    rb_chamber_init(&pocket, 1e-6, 3e5, 2000.0, BURNED);
    rb_chamber_init(&room, 1e-3, 1e5, 300.0, AIR);
    rb_orifice_init(&o, &pocket, &room, 5e-4, 0.7);
    const double air0 = pocket.m[RB_AIR] + room.m[RB_AIR];
    const double burned0 = pocket.m[RB_BURNED] + room.m[RB_BURNED];
    const double U0 = pocket.U + room.U;

    for (int n = 0; n < 384; n++) {               /* 2 ms */
        rb_network_step(all, 2, &o, 1, DT);
        if (n == 0) limited_at_first = o.limited;
        min_dp = fmin(min_dp, pocket.p - room.p);
    }
    CHECK(limited_at_first);                      /* the limit was needed ...      */
    CHECK(min_dp > -1e-3);                        /* ... and the pressures never crossed */
    CHECK(fabs(pocket.p - room.p) < 1.0);
    CHECK_NEAR(pocket.m[RB_AIR] + room.m[RB_AIR], air0, 1e-12 * air0);
    CHECK_NEAR(pocket.m[RB_BURNED] + room.m[RB_BURNED], burned0, 1e-12 * burned0);
    CHECK_NEAR(pocket.U + room.U, U0, 1e-11);
    CHECK(isfinite(pocket.T) && isfinite(room.T));
}

void test_mixing_tracks_species(void)
{
    rb_chamber hot, cold;
    rb_orifice o;
    rb_chamber *all[2] = { &hot, &cold };

    rb_chamber_init(&hot, 1e-3, 2e5, 1500.0, BURNED);
    rb_chamber_init(&cold, 1e-3, 1e5, 300.0, AIR);
    rb_orifice_init(&o, &hot, &cold, 2e-4, 0.8);
    const double air0 = hot.m[RB_AIR] + cold.m[RB_AIR];
    const double burned0 = hot.m[RB_BURNED] + cold.m[RB_BURNED];
    const double U0 = hot.U + cold.U;

    for (int n = 0; n < 38400; n++) rb_network_step(all, 2, &o, 1, DT);   /* 0.2 s */

    CHECK_NEAR(hot.m[RB_AIR] + cold.m[RB_AIR], air0, 1e-12 * air0);
    CHECK_NEAR(hot.m[RB_BURNED] + cold.m[RB_BURNED], burned0, 1e-12 * burned0);
    CHECK_NEAR(hot.U + cold.U, U0, 1e-9);
    CHECK(fabs(hot.p - cold.p) < 1.0);
    /* About a fifth of the hot gas moved over: ~7.5 % of the cold tank's mass. */
    CHECK(cold.y[RB_BURNED] > 0.05 && cold.y[RB_BURNED] < 0.12);
    CHECK(hot.y[RB_AIR] < 1e-12);                 /* nothing flowed back */

    /* The receiving tank's gas properties follow its new mixture: they're
     * rebuilt whenever it has moved by 1e-8 or more in some mass fraction
     * (stage 10, chamber.c), so they're within about that of a fresh mix. */
    rb_gas mix;
    rb_gas_init(&mix, cold.y);
    int stale = 0;
    for (int s = 0; s < RB_NSPECIES; s++) stale += !(fabs(cold.gas_y[s] - cold.y[s]) < 1e-8);
    CHECK_EQ_INT(stale, 0);
    CHECK_NEAR(cold.gas.R, mix.R, 1e-7 * mix.R);
    CHECK_NEAR(rb_gas_cv(&cold.gas, 1000.0), rb_gas_cv(&mix, 1000.0), 1e-7 * rb_gas_cv(&mix, 1000.0));
    CHECK_NEAR(cold.p, cold.mass * cold.gas.R * cold.T / cold.V, 1e-9 * cold.p);
}

void test_filling_heats_the_tank(void)
{
    /* Textbook: filling an empty rigid tank from a reservoir at T0 ends at
     * T = gamma * T0 (420 K from 300 K): the inflow's enthalpy h = u + RT
     * all becomes internal energy, u(T) = h(T0). */
    rb_chamber tank, atmosphere;
    rb_orifice hole;
    rb_chamber *all[2] = { &tank, &atmosphere };
    rb_gas air;

    rb_chamber_init(&tank, 10e-3, 1.0, 300.0, AIR);   /* 1 Pa: practically empty */
    rb_chamber_init_reservoir(&atmosphere, 1e5, 300.0, AIR);
    rb_orifice_init(&hole, &atmosphere, &tank, 1e-4, 1.0);
    for (long n = 0; n < 192000L; n++) rb_network_step(all, 2, &hole, 1, DT);   /* 1 s */

    rb_gas_init(&air, AIR);
    CHECK_NEAR(tank.p, 1e5, 10.0);
    CHECK_NEAR(tank.T, rb_gas_T_from_u(&air, rb_gas_h(&air, 300.0), 400.0), 0.5);
    CHECK_NEAR(tank.T, 420.0, 2.0);
    CHECK(hole.mdot >= 0.0);                           /* flowed in, never out */
}
