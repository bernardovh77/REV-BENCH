/*
 * test_combustion.c - fuel energy, the Wiebe burn, how much of a mixture can
 * burn, constant-volume combustion against chemical equilibrium, and Woschni
 * wall heat transfer (including a motored cylinder's thermodynamic loss angle).
 */
#include "combustion.h"
#include "cylinder.h"
#include "gas.h"
#include "heat.h"
#include "rb_test.h"

#include <math.h>

static const double PI = 3.14159265358979323846;

void test_fuel_heat(void)
{
    /* 43.116 MJ/kg (liquid fuel's heating value) plus 59.5 kJ/kg because the
     * burned gas has more molecules than the air and fuel it came from. */
    CHECK_NEAR(rb_fuel_heat(), 43.1755e6, 0.0005e6);
    CHECK(rb_fuel_heat() > RB_FUEL_LHV);
}

void test_wiebe_burn(void)
{
    int wrong = 0;
    CHECK(rb_wiebe(-5.0, 60.0, 5.0, 2.0) == 0.0 && rb_wiebe(0.0, 60.0, 5.0, 2.0) == 0.0);
    CHECK(rb_wiebe(60.0, 60.0, 5.0, 2.0) == 1.0 && rb_wiebe(75.0, 60.0, 5.0, 2.0) == 1.0);
    CHECK(rb_wiebe(NAN, 60.0, 5.0, 2.0) == 0.0);
    for (double phi = 0.5; phi <= 60.0; phi += 0.5)
        if (!(rb_wiebe(phi, 60.0, 5.0, 2.0) > rb_wiebe(phi - 0.5, 60.0, 5.0, 2.0))) wrong++;
    CHECK_EQ_INT(wrong, 0);

    /* Find the 10, 50 and 90 % points: slow start, fast middle. */
    double p10 = 0.0, p50 = 0.0, p90 = 0.0;
    for (double phi = 0.0; phi <= 60.0; phi += 0.001) {
        const double x = rb_wiebe(phi, 60.0, 5.0, 2.0);
        if (p10 == 0.0 && x >= 0.1) p10 = phi;
        if (p50 == 0.0 && x >= 0.5) p50 = phi;
        if (p90 == 0.0 && x >= 0.9) p90 = phi;
    }
    /* x = 1 - exp(-5 s^3) (scaled): s = (-ln(1 - x) / 5)^(1/3) */
    CHECK_NEAR(p50, 60.0 * cbrt(-log(1.0 - 0.5 * (1.0 - exp(-5.0))) / 5.0), 0.002);
    CHECK(p90 - p10 > 28.0 && p90 - p10 < 31.0);   /* about half the duration */
    CHECK(p10 > 15.0);                              /* the slow start */
}

void test_burnable_fuel(void)
{
    const double air = 1.0, stoich = 1.0 / RB_AFR_STOICH;
    /* Perfectly mixed: lean burns all its fuel, rich all its air. */
    CHECK_NEAR(rb_burnable_fuel(0.5 * stoich, air, 0.0, 1.0), 0.5 * stoich, 1e-15);
    CHECK_NEAR(rb_burnable_fuel(2.0 * stoich, air, 0.0, 1.0), stoich, 1e-15);
    /* Imperfectly mixed (spread 0.07) with 98 % efficiency: 95.3 % burns at
     * phi = 1, 98 % lean, 81.7 % at phi = 1.2, the trend Heywood shows. */
    CHECK_NEAR(rb_burnable_fuel(stoich, air, 0.07, 0.98) / stoich, 0.95263, 1e-5);
    CHECK_NEAR(rb_burnable_fuel(0.8 * stoich, air, 0.07, 0.98) / (0.8 * stoich), 0.9799, 1e-4);
    CHECK_NEAR(rb_burnable_fuel(1.2 * stoich, air, 0.07, 0.98) / (1.2 * stoich), 0.8166, 1e-4);
    /* Very rich: limited by the air. */
    CHECK_NEAR(rb_burnable_fuel(5.0 * stoich, air, 0.07, 0.98), 0.98 * stoich, 1e-9);
    /* More fuel never burns less, and never more than there is. */
    int wrong = 0;
    for (double phi = 0.05; phi < 3.0; phi += 0.05) {
        const double b = rb_burnable_fuel(phi * stoich, air, 0.07, 0.98);
        if (!(b >= rb_burnable_fuel((phi - 0.05) * stoich, air, 0.07, 0.98)) || b > phi * stoich) wrong++;
    }
    CHECK_EQ_INT(wrong, 0);
    CHECK(rb_burnable_fuel(0.0, air, 0.07, 0.98) == 0.0 && rb_burnable_fuel(stoich, 0.0, 0.07, 0.98) == 0.0);
    CHECK(rb_burnable_fuel(NAN, air, 0.07, 0.98) == 0.0);
    /* An efficiency over 1 (a bad setting) still burns no more than there is. */
    CHECK(rb_burnable_fuel(0.5 * stoich, air, 0.07, 1.5) == 0.5 * stoich);
}

void test_constant_volume_burn(void)
{
    /* A stoichiometric charge at the end of compression (700 K, 20 bar)
     * burned completely at constant volume, against Cantera's chemical
     * equilibrium with the same energy (docs/stage-06.md): 2913 K and 88.8 bar.
     * Frozen products, without dissociation, would reach 3224 K and 96.4 bar.
     * The model's pressure comes out a little low: its burned gas has the
     * complete products' molecule count, and dissociating adds about 2 %. */
    const double fuel = 1.0, air = RB_AFR_STOICH;
    const double y[RB_NSPECIES] = { [RB_AIR] = air / (air + fuel), [RB_FUEL] = fuel / (air + fuel) };
    rb_gas mix, burned;
    rb_gas_init(&mix, y);
    rb_gas_init(&burned, (const double[RB_NSPECIES]){ [RB_BURNED] = 1.0 });
    const double m = air + fuel, T0 = 700.0, p0 = 20e5;
    const double V = m * mix.R * T0 / p0;
    const double U = m * rb_gas_u(&mix, T0) + fuel * rb_fuel_heat();
    const double T = rb_gas_T_from_u(&burned, U / m, 2500.0);
    const double p = m * burned.R * T / V;
    CHECK_NEAR(T, 2913.1, 0.02 * 2913.1);
    CHECK_NEAR(p, 88.77e5, 0.045 * 88.77e5);
    CHECK(T < 3150.0);   /* well below the frozen 3224 K */
}

void test_woschni(void)
{
    /* 86 mm bore, 50 bar, 2000 K, 10 m/s: 3.26 B^-0.2 p^0.8 T^-0.55 w^0.8. */
    CHECK_NEAR(rb_woschni_h(0.086, 50e5, 2000.0, 10.0), 467.657, 0.001);
    CHECK_NEAR(rb_woschni_h(0.086, 100e5, 2000.0, 10.0) / rb_woschni_h(0.086, 50e5, 2000.0, 10.0),
               pow(2.0, 0.8), 1e-12);
    CHECK(rb_woschni_h(0.086, 0.0, 2000.0, 10.0) == 0.0 && rb_woschni_h(0.086, 50e5, NAN, 10.0) == 0.0);

    /* Gas speed: 6.18 Sp with a valve open, 2.28 Sp shut, more once burning. */
    CHECK_NEAR(rb_woschni_w(10.0, 1, 0, 5e-4, 1e5, 1e5, 5e-4, 330.0, 3e-4), 61.8, 1e-12);
    CHECK_NEAR(rb_woschni_w(10.0, 0, 0, 5e-4, 1e5, 1e5, 5e-4, 330.0, 3e-4), 22.8, 1e-12);
    const double V = 6e-5, p_motored = 1e5 * pow(5e-4 / V, 1.3);
    const double burning = rb_woschni_w(10.0, 0, 1, 5e-4, 2.0 * p_motored, 1e5, 5e-4, 330.0, V);
    CHECK_NEAR(burning, 22.8 + 3.24e-3 * 5e-4 * 330.0 / (1e5 * 5e-4) * p_motored, 1e-9);
    /* No extra stirring when the pressure is at or below the motored one. */
    CHECK_NEAR(rb_woschni_w(10.0, 0, 1, 5e-4, 0.9 * p_motored, 1e5, 5e-4, 330.0, V), 22.8, 1e-12);

    rb_cyl_geom g;
    rb_cyl_geom_init(&g, 0.086, 0.086, 0.143, 10.0);
    /* At TDC: head + crown + the clearance band of liner. */
    CHECK_NEAR(rb_wall_area(&g, 0.0), 2.0 * g.area + PI * g.bore * g.v_clear / g.area, 1e-15);
}

void test_motored_loss_angle(void)
{
    /* A motored cylinder that loses heat to its walls reaches its peak
     * pressure slightly before TDC (the thermodynamic loss angle, under a
     * degree), and a lower peak than without heat loss. */
    static const double AIR[RB_NSPECIES] = { [RB_AIR] = 1.0 };
    const double dt = 1.0 / 192000.0, rpm = 3000.0, T_wall = 430.0;
    const double step = rpm / 60.0 * 2.0 * PI * dt;
    double peak[2] = { 0.0, 0.0 }, at[2] = { 0.0, 0.0 };

    for (int walls = 0; walls < 2; walls++) {
        rb_cyl_geom g;
        rb_cylinder c;
        rb_cyl_geom_init(&g, 0.086, 0.086, 0.143, 10.0);
        rb_cylinder_init(&c, &g, -PI, 1e5, 300.0, AIR);   /* BDC */
        const double p_ivc = c.gas.p, V_ivc = c.gas.V, T_ivc = c.gas.T;
        const long n = lround(2.0 * PI / step);
        for (long k = 1; k <= n; k++) {
            rb_chamber_clear_rates(&c.gas);
            if (walls) {
                const double w = rb_woschni_w(2.0 * g.stroke * rpm / 60.0, 0, 0, g.v_disp,
                                              c.gas.p, p_ivc, V_ivc, T_ivc, c.gas.V);
                const double h = rb_woschni_h(g.bore, c.gas.p, c.gas.T, w);
                const double x = (c.gas.V - g.v_clear) / g.area;
                rb_chamber_add_heat(&c.gas, h * rb_wall_area(&g, x) * (T_wall - c.gas.T));
            }
            rb_chamber_integrate(&c.gas, dt);
            const double theta = -PI + k * step;
            rb_cylinder_move(&c, theta);
            if (c.gas.p > peak[walls]) {
                peak[walls] = c.gas.p;
                at[walls] = theta * 180.0 / PI;
            }
        }
    }
    CHECK(fabs(at[0]) < 0.06);                     /* adiabatic: at TDC, to a step */
    CHECK(at[1] < -0.1 && at[1] > -1.0);           /* with heat loss: a little before */
    CHECK(peak[1] < peak[0]);
}
