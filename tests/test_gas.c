/*
 * test_gas.c - gas properties against the NASA data they were fitted to,
 * textbook values, and their own consistency (u and T(u), mixing).
 */
#include "gas.h"
#include "rb_test.h"

#include <math.h>
#include <stddef.h>

/* Reference values from the NASA Glenn database (Cantera 3.2): cp in
 * J/(kg K) and u(T) - u(298.15 K) in J/kg. */
static const double REF_T[13] = {
    200, 250, 300, 400, 600, 800, 1000, 1500, 2000, 2500, 3000, 3500, 4000,
};
static const double REF_CP[RB_NSPECIES][13] = {
    [RB_AIR]    = { 1003.206, 1003.041, 1004.91, 1014.225, 1050.465, 1098.588, 1140.619,
                    1208.574, 1251.85, 1278.761, 1295.953, 1308.406, 1319.432 },
    [RB_FUEL]   = { 1029.106, 1256.516, 1483.475, 1921.323, 2663.959, 3187.848, 3580.345,
                    4180.7, 4535.28, 4728.28, 4826.633, 4880.008, 4920.808 },
    [RB_BURNED] = { 1034.615, 1046.206, 1058.155, 1083.536, 1141.074, 1204.032, 1260.084,
                    1356.751, 1417.207, 1453.253, 1474.484, 1488.285, 1499.837 },
};
static const double REF_U[RB_NSPECIES][13] = {
    [RB_AIR]    = { -70300.31, -34507.78, 1327.86, 73516.02, 222241.86, 379665.02, 546441.84,
                    991414.12, 1463813.11, 1953447.21, 2453864.89, 2961521.11, 3474943.75 },
    [RB_FUEL]   = { -115054.29, -61921.03, 2588.7, 165077.62, 611151.91, 1183727.12, 1845180.7,
                    3757660.71, 5905006.51, 8186098.58, 10537599.84, 12925323.36, 15335600.18 },
    [RB_BURNED] = { -74537.08, -36849.06, 1426.91, 79830.75, 244821.74, 421991.69, 611344.04,
                    1124023.69, 1675446.52, 2250549.18, 2839617.27, 3437181.61, 4040914.49 },
};

static void pure(rb_gas *g, int species)
{
    double y[RB_NSPECIES] = { 0 };
    y[species] = 1.0;
    rb_gas_init(g, y);
}

void test_gas_matches_nasa_data(void)
{
    for (int s = 0; s < RB_NSPECIES; s++) {
        rb_gas g;
        pure(&g, s);
        for (int i = 0; i < 13; i++) {
            CHECK_NEAR(rb_gas_cp(&g, REF_T[i]), REF_CP[s][i], 1e-4 * REF_CP[s][i]);
            CHECK_NEAR(rb_gas_u(&g, REF_T[i]), REF_U[s][i], 1e-4 * fabs(REF_U[s][i]) + 2.0);
        }
    }
    CHECK_NEAR(rb_species_R(RB_AIR), 287.0985, 0.001);
    CHECK_NEAR(rb_species_R(RB_FUEL), 80.0079, 0.001);
    CHECK_NEAR(rb_species_R(RB_BURNED), 286.6256, 0.001);
    CHECK(rb_species_name(RB_FUEL) != NULL);
    CHECK(rb_species_name(RB_NSPECIES) == NULL);
    CHECK(isnan(rb_species_R(-1)));
}

void test_gas_textbook_air(void)
{
    /* Ideal-gas air from a thermodynamics textbook (Cengel & Boles, table A-2). */
    rb_gas air;
    pure(&air, RB_AIR);
    CHECK_NEAR(air.R, 287.0, 0.2);
    CHECK_NEAR(rb_gas_cp(&air, 300.0), 1005.0, 3.0);
    CHECK_NEAR(rb_gas_gamma(&air, 300.0), 1.400, 0.002);
    CHECK_NEAR(rb_gas_cp(&air, 1000.0), 1142.0, 3.0);
    CHECK_NEAR(rb_gas_gamma(&air, 1000.0), 1.336, 0.002);
}

void test_gas_u_and_T_are_inverse(void)
{
    static const double MIXES[4][RB_NSPECIES] = {
        { 1.0, 0.0, 0.0 },       /* air                   */
        { 0.0, 0.0, 1.0 },       /* burned gas            */
        { 0.936, 0.064, 0.0 },   /* stoichiometric charge */
        { 0.3, 0.02, 0.68 },     /* a bit of everything   */
    };
    static const double JOINTS[3] = { RB_T_LO, RB_T_MID, RB_T_HI };
    int bad = 0;

    for (int k = 0; k < 4; k++) {
        rb_gas g;
        double prev = -INFINITY;
        rb_gas_init(&g, MIXES[k]);
        for (double T = 100.0; T <= 5000.0; T += 37.0) {
            double u = rb_gas_u(&g, T);
            if (!(u > prev)) bad++;                                  /* u rises with T */
            prev = u;
            if (fabs(rb_gas_T_from_u(&g, u, 300.0) - T) > 1e-8 * T) bad++;   /* far guesses */
            if (fabs(rb_gas_T_from_u(&g, u, 3000.0) - T) > 1e-8 * T) bad++;  /* converge too */
            if (fabs(T - RB_T_LO) > 1.0 && fabs(T - RB_T_MID) > 1.0 && fabs(T - RB_T_HI) > 1.0) {
                double slope = (rb_gas_u(&g, T + 0.01) - rb_gas_u(&g, T - 0.01)) / 0.02;
                if (fabs(slope - rb_gas_cv(&g, T)) > 1e-5 * rb_gas_cv(&g, T)) bad++;  /* du/dT = cv */
            }
        }
        for (int j = 0; j < 3; j++)                                  /* no jumps at the joints */
            if (fabs(rb_gas_u(&g, JOINTS[j] + 1e-7) - rb_gas_u(&g, JOINTS[j] - 1e-7)) > 1e-3) bad++;
        if (!isnan(rb_gas_T_from_u(&g, NAN, 300.0))) bad++;
    }
    CHECK_EQ_INT(bad, 0);
}

void test_gas_mixing_is_mass_weighted(void)
{
    const double y[RB_NSPECIES] = { 0.6, 0.05, 0.35 };
    const double y_scaled[RB_NSPECIES] = { 6.0, 0.5, 3.5 };   /* not normalised */
    const double garbage[RB_NSPECIES] = { NAN, -1.0, 0.0 };
    rb_gas mix, scaled, junk, g[RB_NSPECIES];

    rb_gas_init(&mix, y);
    rb_gas_init(&scaled, y_scaled);
    rb_gas_init(&junk, garbage);
    for (int s = 0; s < RB_NSPECIES; s++) pure(&g[s], s);

    for (double T = 250.0; T <= 3500.0; T += 250.0) {
        double cp = 0.0, u = 0.0, R = 0.0;
        for (int s = 0; s < RB_NSPECIES; s++) {
            cp += y[s] * rb_gas_cp(&g[s], T);
            u  += y[s] * rb_gas_u(&g[s], T);
            R  += y[s] * g[s].R;
        }
        CHECK_NEAR(rb_gas_cp(&mix, T), cp, 1e-9 * cp);
        CHECK_NEAR(rb_gas_u(&mix, T), u, 1e-9 * fabs(u) + 1e-6);
        CHECK_NEAR(mix.R, R, 1e-12 * R);
        CHECK_NEAR(rb_gas_h(&mix, T), rb_gas_u(&mix, T) + mix.R * T, 1e-6);
        CHECK_NEAR(rb_gas_cp(&scaled, T), rb_gas_cp(&mix, T), 1e-9 * cp);
    }
    CHECK_NEAR(junk.R, g[RB_AIR].R, 1e-12);   /* invalid fractions: pure air */
}
