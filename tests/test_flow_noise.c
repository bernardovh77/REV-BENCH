/*
 * test_flow_noise.c - stage 053: the flow's noise, booked once, at the gap's
 * own state, with its reverse flows measured and its statistics over seeds.
 *
 * The jets' noise (noise.c, stage 12b and 115) is a share eta M^5 of a jet's
 * stream power 1/2 mdot U^2, with M and U the gap's, solved from the
 * pressures either side (test_noise_jets_at_the_gap). It is the model's
 * external source work: the sound is made from the gas's state and added to
 * what the listener hears, and nothing is taken back from the gas. These
 * tests show that the booking is consistent (the gas never feels it, and
 * what it would take is far below the books' resolution), measure what the
 * missing reverse-flow jets would carry, and check the noise's statistics.
 */
#include "engine.h"
#include "flow.h"
#include "noise.h"
#include "presets.h"
#include "rb.h"
#include "rb_test.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DT (1.0 / 192000.0)

static rb_engine e;
static rb_noise n;
static const double AIR[RB_NSPECIES] = { 1.0, 0.0, 0.0 };

/* A jet's sound share, eta M^5 1/2 |mdot| U^2, from the gap's state as a
 * textbook writes it, the flow from `up` into `down` (either way round). */
static double jet(const rb_chamber *up, const rb_chamber *down, double g, double mdot, double eta)
{
    if (!(down->p < up->p) || !(up->T > 0.0)) return 0.0;
    const double critical = pow(2.0 / (g + 1.0), g / (g - 1.0));
    const double M = (down->p / up->p <= critical) ? 1.0
                   : sqrt(2.0 / (g - 1.0) * (pow(up->p / down->p, (g - 1.0) / g) - 1.0));
    const double T = up->T / (1.0 + 0.5 * (g - 1.0) * M * M);
    const double U2 = M * M * g * up->gas.R * T;
    return eta * pow(M, 5) * 0.5 * fabs(mdot) * U2;
}

/* The four held at `rpm`, throttle `pos`, settled. */
static void held_four(double rpm, double pos)
{
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i4"), &d);
    rb_engine_init(&e, &d, DT);
    e.rpm = rpm;
    e.throttle_pos = pos;
    e.ignition = 1;
    rb_engine_settle(&e, 100, 1e-4);
}

void test_flow_noise_is_external_source_work(void)
{
    /* Booked once, as the model's own source work: with the flow's noise on
     * and off the engine is the same to the last bit (every instrument after
     * half a second at 3000 rpm wide open), only the sound differs. And what
     * it would take from the gas is small: over a cycle the jets' noise is
     * at most eta (5e-4) of their stream power (M^5 is under 1; the exhaust's
     * blowdown, choked, makes nearly all of it) and 5.9e-5 of the indicated
     * work, under 1e-4: the size of the inconsistency in not taking it from
     * the gas, stated in docs/stage-053.md. */
    enum { FRAMES = 24000 };
    static float out[2][FRAMES];
    double tlm[2][RB_TLM_COUNT];
    for (int k = 0; k < 2; k++) {
        rb_sim *sim = rb_create(NULL);
        CHECK(sim != NULL);
        if (!sim) return;
        CHECK_EQ_INT(rb_load_preset(sim, rb_preset_find("i4")), RB_OK);
        rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
        rb_set_param(sim, RB_PARAM_LAYERS, k ? (RB_LAYER_ALL & ~RB_LAYER_FLOW) : RB_LAYER_ALL);
        rb_set_param(sim, RB_PARAM_HOLD_RPM, 3000);
        rb_set_param(sim, RB_PARAM_IGNITION, 1);
        rb_set_param(sim, RB_PARAM_THROTTLE, 1);
        rb_render_block(sim, out[k], FRAMES);
        for (int id = RB_TLM_RPM; id < RB_TLM_COUNT; id++) tlm[k][id] = rb_get_telemetry(sim, id);
        rb_destroy(sim);
    }
    CHECK(memcmp(&tlm[0][RB_TLM_RPM], &tlm[1][RB_TLM_RPM], sizeof(double) * (RB_TLM_COUNT - RB_TLM_RPM)) == 0);
    CHECK(memcmp(out[0], out[1], sizeof out[0]) != 0);

    held_four(3000.0, 1.0);
    const int cycle = (int)(120.0 / 3000.0 / DT);
    double noise = 0.0, stream = 0.0;
    for (int s = 0; s < cycle; s++) {
        rb_engine_step(&e);
        for (int i = 0; i < e.desc.cylinders; i++) {
            const rb_orifice *v[2] = { &e.intake[i], &e.exhaust[i] };
            const double g[2] = { 1.40, 1.32 }, eta[2] = { 5e-5, 5e-4 };
            for (int j = 0; j < 2; j++) {
                if (!(v[j]->mdot > 0.0 && v[j]->area > 0.0)) continue;
                noise += jet(v[j]->a, v[j]->b, g[j], v[j]->mdot, eta[j]) * DT;
                stream += jet(v[j]->a, v[j]->b, g[j], v[j]->mdot, 1.0) / 1.0 * DT;   /* M^5 weighted: a bound */
            }
        }
    }
    const double indicated = e.result.imep_gross * e.desc.cylinders * 0.25 * PI * e.desc.bore * e.desc.bore * e.desc.stroke;
    printf("    the jets' noise over a cycle: %.3g J, %.2g of the M^5-weighted stream power, %.2g of the indicated work\n",
           noise, noise / stream, noise / indicated);
    CHECK(noise > 0.0);
    CHECK(noise <= 5e-4 * stream * (1.0 + 1e-9));
    CHECK(noise < 1e-4 * indicated);
}

void test_flow_noise_reverse_jets_measured(void)
{
    /* Matched states forward and back. The model's jets blow only forward (an
     * exhaust valve into its header, an intake valve into its cylinder): a
     * gap flowing back makes no noise (noise.c's inner_jet), so the intake's
     * roar is exactly the forward valves' at every step. Measured here, the
     * reversed jets the model leaves out (docs/stage-053.md):
     *   - idling (850 rpm, throttle 4 %), the cylinder's gas blows back into
     *     the runner's vacuum as the intake valve opens, choked: a jet some
     *     7e5 times the forward intake jets' (which cross a small drop, M^5
     *     tiny), and the header's gas back into the cylinder at overlap adds
     *     54 % to the exhaust valves';
     *   - wide open at 3000 rpm, reversion adds 51 % to the intake valves'
     *     and 0.01 % to the exhaust valves'.
     * Routing them (into the runner, heard at the mouth; into the cylinder)
     * is stage 054's. */
    static const double CASES[2][2] = { { 850.0, 0.04 }, { 3000.0, 1.0 } };
    for (int c = 0; c < 2; c++) {
        held_four(CASES[c][0], CASES[c][1]);
        rb_noise_init(&n, &e, 5);
        const int cycle = (int)(120.0 / CASES[c][0] / DT);
        double fwd[2] = { 0.0, 0.0 }, rev[2] = { 0.0, 0.0 }, worst = 0.0, loudest = 0.0;
        for (int s = 0; s < cycle; s++) {
            rb_engine_step(&e);
            rb_noise_step(&n, &e, RB_NOISE_FLOW);
            double intake = 0.0;
            for (int i = 0; i < e.desc.cylinders; i++) {
                const rb_orifice *v[2] = { &e.intake[i], &e.exhaust[i] };
                const double g[2] = { 1.40, 1.32 }, eta[2] = { 5e-5, 5e-4 };
                for (int j = 0; j < 2; j++) {
                    if (!(v[j]->area > 0.0)) continue;
                    if (v[j]->mdot > 0.0) {
                        const double w = jet(v[j]->a, v[j]->b, g[j], v[j]->mdot, eta[j]);
                        fwd[j] += w * DT;
                        if (j == 0) intake += w;
                    } else if (v[j]->mdot < 0.0) {
                        rev[j] += jet(v[j]->b, v[j]->a, g[j], v[j]->mdot, eta[j]) * DT;
                    }
                }
            }
            /* against the cycle's loudest: a gap crossing almost no drop is
             * rounding either way, and libms round it differently (MSVC) */
            loudest = fmax(loudest, intake);
            if (intake > 0.0) worst = fmax(worst, fabs(n.intake_power - intake));
            else CHECK(n.intake_power == 0.0);
        }
        printf("    %4.0f rpm, throttle %.0f %%: reverse jets would add %.1f %% to the intake valves' noise, %.2f %% to the exhaust valves' (the reversed intake jets: %.3g of the forward exhaust jets)\n",
               CASES[c][0], 100.0 * CASES[c][1], 100.0 * rev[0] / fwd[0], 100.0 * rev[1] / fwd[1], rev[0] / fwd[1]);
        CHECK(worst < 1e-9 * loudest);       /* forward only, at every step */
        CHECK(fwd[0] > 0.0 && fwd[1] > 0.0);
        if (c == 0) {                        /* idle: reversion dominates the intake */
            CHECK(rev[0] > 1e4 * fwd[0]);
            CHECK(rev[1] > 0.3 * fwd[1] && rev[1] < 1.0 * fwd[1]);
        } else {                             /* wide open */
            CHECK(rev[0] > 0.3 * fwd[0] && rev[0] < 1.0 * fwd[0]);
            CHECK(rev[1] < 0.01 * fwd[1]);
        }
    }
}

/* The mouth's hiss over `steps` steps with the throttle's gap held at p0
 * into p (mdot scaled by `scale`): its mean square. */
static double held_hiss(uint64_t seed, double p0, double p, double scale, long steps)
{
    rb_orifice *o = &e.throttle;
    rb_chamber_init(o->a, o->a->V, p0, 300.0, AIR);
    rb_chamber_init(o->b, o->b->V, p, 300.0, AIR);
    o->mdot = scale * rb_nozzle_mdot(o->area * o->cd, p0, 300.0, o->a->gas.R, 1.4, p);
    for (int i = 0; i < e.desc.cylinders; i++) e.intake[i].mdot = 0.0;
    e.idle_air.mdot = 0.0;
    rb_noise_init(&n, &e, seed);
    for (long s = 0; s < 4800; s++) rb_noise_step(&n, &e, RB_NOISE_FLOW);   /* the filters fill */
    double sum = 0.0;
    for (long s = 0; s < steps; s++) {
        rb_noise_step(&n, &e, RB_NOISE_FLOW);
        sum += n.out[RB_NOISE_MOUTH] * n.out[RB_NOISE_MOUTH];
    }
    return sum / steps;
}

void test_flow_noise_statistics_over_seeds(void)
{
    /* The noise is a random process whose expectation the law sets: over
     * eight seeds the hiss of a held throttle gap (1 bar into 0.5 bar:
     * choked) varies by under 6 % (0.5 s each: a band about a kHz wide, so
     * some 500 independent values, 4.5 % expected), and over them the mean
     * for twice the flow is twice the power (to 5 %: the power is linear in
     * mdot), and as the pressure drop across the gap vanishes the hiss goes
     * continuously to nothing (M^5: a hundredth of a bar's drop is 1e-7 of
     * a choked gap's). */
    rb_engine_desc d;
    rb_preset_desc(rb_preset_find("i4"), &d);
    rb_engine_init(&e, &d, DT);
    e.throttle.area = 3e-4;
    enum { SEEDS = 8, STEPS = 96000 };
    double one[SEEDS], two = 0.0, mean = 0.0, spread = 0.0;
    for (int k = 0; k < SEEDS; k++) {
        one[k] = held_hiss(100 + k, 1e5, 5e4, 1.0, STEPS);
        mean += one[k] / SEEDS;
        two += held_hiss(200 + k, 1e5, 5e4, 2.0, STEPS) / SEEDS;
    }
    for (int k = 0; k < SEEDS; k++) spread = fmax(spread, fabs(one[k] / mean - 1.0));
    printf("    held hiss over %d seeds: within %.1f %% of their mean; twice the flow: %.3f times the power\n",
           SEEDS, 100.0 * spread, two / mean);
    CHECK(mean > 0.0);
    CHECK(spread < 0.06);
    CHECK_NEAR(two / mean, 2.0, 0.10);
    const double small = held_hiss(100, 1e5, 0.99e5, 1.0, STEPS);
    CHECK(small < 1e-6 * mean);
    CHECK(held_hiss(100, 1e5, 1e5, 1.0, STEPS) == 0.0);
}
