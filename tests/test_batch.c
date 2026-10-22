/* The batched pipe thermodynamics must agree with the independent scalar
 * integrator, including fit boundaries, Newton fallback and corrupt states. */
#include "chamber.h"
#include "rb_test.h"

#include <math.h>
#include <string.h>

void test_chamber_batch_matches_scalar(void)
{
    static const double temperatures[] = {1.5, 199.9, 200.1, 298.15, 999.9,
                                         1000.1, 1500, 3999.9, 4000.1};
    const int counts[] = {1, 7, 8, 9, 17, 64};
    rb_chamber batch[64], scalar[64];
    for (int trial = 0; trial < 6; trial++) {
        const int n = counts[trial];
        for (int j = 0; j < n; j++) {
            const double y[3] = {0.2 + 0.01 * j, 0.03, 0.7};
            rb_chamber_init(&batch[j], 1e-4, 101325 + j * 2000,
                            temperatures[j % 9], y);
            /* Composition changes above and below RB_REMIX; energy changes
             * exercise both the fast correction and the scalar fallback. */
            batch[j].dm[0] = (j % 2 ? 1e-9 : 0.1) * batch[j].mass;
            batch[j].dm[1] = 0.01 * batch[j].mass;
            batch[j].dm[2] = -0.005 * batch[j].mass;
            batch[j].dU = (j % 3 ? 1e3 : 1e9) * batch[j].mass;
        }
        memcpy(scalar, batch, sizeof batch);
        for (int step = 0; step < 20; step++) {
            for (int j = 0; j < n; j++) rb_chamber_integrate(&scalar[j], 1.0 / 48000);
            rb_chamber_integrate_many(batch, n, 1.0 / 48000);
            /* Every state field, cached mixture, rate and repair count. */
            CHECK(memcmp(batch, scalar, (size_t)n * sizeof batch[0]) == 0);
        }
    }
}

void test_chamber_batch_repairs(void)
{
    rb_chamber batch[17], scalar[17];
    const double y[3] = {0.8, 0.0, 0.2};
    for (int j = 0; j < 17; j++) rb_chamber_init(&batch[j], 1e-4, 101325, 350, y);
    batch[0].m[0] = NAN;
    batch[1].m[2] = INFINITY;
    batch[2].U = NAN;
    batch[3].dm[0] = -1e6;
    batch[4].fixed = 1;
    batch[5].m[0] = batch[5].m[2] = 0;
    batch[6].T = NAN;
    batch[7].cv = 0;
    batch[8].U = INFINITY;
    batch[9].dU = NAN;
    batch[10].m[1] = -1e-8; /* total mass stays positive: per-species guard matters */
    batch[11].U = 1e308; /* finite total energy, overflowing specific energy */
    memcpy(scalar, batch, sizeof batch);
    for (int j = 0; j < 17; j++) rb_chamber_integrate(&scalar[j], 1.0 / 48000);
    rb_chamber_integrate_many(batch, 17, 1.0 / 48000);
    CHECK(memcmp(batch, scalar, sizeof batch) == 0);
    CHECK(batch[0].repairs > 0 && batch[1].repairs > 0 && batch[2].repairs > 0);
    CHECK(batch[3].repairs > 0 && batch[8].repairs > 0 && batch[9].repairs > 0);
    CHECK(batch[5].mass == 0 && batch[5].per_rho == 0);
    CHECK(batch[4].mass == scalar[4].mass && batch[4].fixed);
    rb_chamber_integrate_many(batch, 0, 1.0 / 48000);
    CHECK(memcmp(batch, scalar, sizeof batch) == 0);
}
