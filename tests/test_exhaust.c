#include "presets.h"
#include "rb.h"
#include "rb_test.h"
#include <math.h>

static rb_engine engine;

void test_exhaust_researched_topology(void)
{
    for (int p = 0; p < 5; p++) {
        rb_engine_desc d;
        CHECK_EQ_INT(rb_preset_desc(p, &d), 0);
        CHECK_EQ_INT(rb_engine_init(&engine, &d, 1.0 / 192000.0), 0);
        int cells = 0;
        for (int k = 0; k < rb_engine_pipe_count(&engine); k++) {
            const rb_pipe *pipe = rb_engine_pipe(&engine, k);
            cells += pipe->cells;
            for (int end = 0; end < 2; end++) {
                int connected = pipe->end[end].kind != RB_PIPE_CLOSED;
                for (int j = 0; j < rb_engine_pipe_count(&engine); j++) {
                    const rb_pipe *peer = rb_engine_pipe(&engine, j);
                    for (int pe = 0; pe < 2; pe++)
                        connected |= peer != pipe && peer->end[pe].chamber ==
                            &pipe->cell[end ? pipe->cells - 1 : 0];
                }
                CHECK(connected);
            }
            CHECK(pipe->cell >= engine.cell_pool && pipe->cell + pipe->cells <= engine.cell_pool + RB_ENGINE_CELLS);
        }
        CHECK(cells <= RB_ENGINE_CELLS);
        if (p != 3) {
            CHECK(d.center_length > 0.0);
            CHECK_NEAR(engine.link[0].length, d.link_length, 1e-15);
        }
        if (p == 0) {
            CHECK_NEAR(engine.center[0].length, 0.2413, 1e-15);
            CHECK_NEAR(engine.muffler[0].length, 0.7239, 1e-15);
            CHECK_NEAR(engine.center[0].area * engine.center[0].length, 0.0026, 1e-15);
        }
        if (p == 1) {
            CHECK_EQ_INT(engine.systems, 2);
            CHECK(engine.link[0].end[1].chamber == rb_pipe_end_cell(&engine.muffler[0], 0));
            CHECK(engine.link[1].end[1].chamber == rb_pipe_end_cell(&engine.muffler[1], 0));
            CHECK_NEAR(engine.tailpipe[0].diameter, 0.0516, 1e-15);
        }
        if (p == 2) {
            CHECK(engine.exhaust_pipe[0].end[1].chamber == &engine.junction);
            CHECK(engine.exhaust_pipe[1].end[1].chamber == &engine.junction);
            CHECK(engine.center[0].end[0].chamber == &engine.junction);
            CHECK(engine.center[1].end[0].chamber == &engine.junction);
            CHECK_NEAR(engine.tailpipe[0].diameter, 0.0738, 1e-15);
        }
        if (p == 3) CHECK_EQ_INT(d.muffler_type, RB_MUFFLER_NONE);
        if (p == 4) {
            CHECK_EQ_INT(engine.collectors, 1);
            CHECK_EQ_INT(engine.systems, 2);
            CHECK(engine.link[0].end[1].chamber == &engine.junction);
            CHECK(engine.muffler[0].end[0].chamber == &engine.junction);
            CHECK(engine.muffler[1].end[0].chamber == &engine.junction);
            CHECK_NEAR(engine.tailpipe[1].diameter, 0.0426, 1e-15);
        }
    }
}

void test_exhaust_descriptor_guards(void)
{
    rb_engine_desc d;
    rb_preset_desc(2, &d);
    d.center_length = NAN;
    CHECK_EQ_INT(rb_engine_desc_check(&d), -1);
    rb_preset_desc(2, &d);
    d.junction_volume = 0.0;
    CHECK_EQ_INT(rb_engine_desc_check(&d), -1);
    rb_preset_desc(4, &d);
    d.junction_volume = 0.0;
    CHECK_EQ_INT(rb_engine_desc_check(&d), -1);
    rb_preset_desc(0, &d);
    d.center_volume = INFINITY;
    CHECK_EQ_INT(rb_engine_desc_check(&d), -1);
}

void test_exhaust_engine_books(void)
{
    for (int p = 0; p < 5; p++) {
        rb_engine_desc d;
        rb_preset_desc(p, &d);
        d.heat_transfer = 0;
        rb_engine_init(&engine, &d, 1.0 / 192000.0);
        engine.rpm = 3000.0;
        engine.throttle_pos = 0.15;
        double m0, u0, y0[3], m1, u1, y1[3];
        rb_engine_contents(&engine, &m0, &u0, y0);
        for (int k = 0; k < 3 * 7680; k++) rb_engine_step(&engine);
        rb_engine_contents(&engine, &m1, &u1, y1);
        CHECK_NEAR(m1 - m0, engine.mass_in - engine.mass_out, m0 * 1e-11);
        CHECK_NEAR(u1 - u0, engine.energy_in - engine.energy_out + engine.work_on_gas
                   + rb_engine_pipe_heat(&engine), fabs(engine.energy_out) * 1e-9);
        for (int s = 0; s < 3; s++)
            CHECK_NEAR(y1[s] - y0[s], engine.species_in[s] - engine.species_out[s], m0 * 1e-11);
        CHECK_EQ_INT(engine.junction.repairs, 0);
        for (int k = 0; k < rb_engine_pipe_count(&engine); k++) {
            const rb_pipe *pipe = rb_engine_pipe(&engine, k);
            int repairs = 0;
            for (int j = 0; j < pipe->cells; j++) repairs += pipe->cell[j].repairs;
            CHECK_EQ_INT(repairs, 0);
        }
    }
}

/* Independent compact-junction scattering: four equal acoustic admittances.
 * With one incident pressure pulse, R = -1/2 and all transmissions = +1/2.
 * No reservoir or synthesized sound participates. Also close the energy,
 * mass and species books for the finite-volume junction and all four arms. */
void test_exhaust_junction_scattering_conservation(void)
{
    static rb_pipe arm[4];
    static rb_chamber cell[4][64], junction;
    static rb_face face[4][64];
    const double y[RB_NSPECIES] = { 0.8, 0.01, 0.19 };
    const double dt = 1.0 / 96000.0, pi = 3.14159265358979323846;
    const double area = pi * 0.05 * 0.05 / 4.0;
    rb_chamber_init(&junction, area * 0.04, 101325.0, 300.0, y);
    for (int a = 0; a < 4; a++) {
        rb_pipe_init(&arm[a], cell[a], face[a], 64, 2.0, 0.05, 101325.0, 300.0, y, 300.0, 0.0);
        rb_pipe_join(&arm[a], 0, &junction, 0.0, 0);
    }
    const double c = rb_chamber_sound_speed(&cell[0][0]);
    const double gamma = rb_chamber_gamma(&cell[0][0]);
    for (int k = 0; k < 64; k++) {
        const double x = (k + 0.5) * arm[0].dx;
        const double bump = 10.0 * exp(-pow((x - 1.5) / 0.22, 2.0));
        const double pressure = 101325.0 + bump;
        rb_chamber_init(&cell[0][k], area * arm[0].dx, pressure,
            300.0 * pow(pressure / 101325.0, (gamma - 1.0) / gamma), y);
    }
    for (int k = 0; k < 63; k++) {
        const double x = (k + 1.0) * arm[0].dx;
        face[0][k].mdot = -area * 10.0 * exp(-pow((x - 1.5) / 0.22, 2.0)) / c;
    }
    double m0 = junction.mass, u0 = junction.U, s0[3];
    for (int s = 0; s < 3; s++) s0[s] = junction.m[s];
    for (int a = 0; a < 4; a++) {
        double m, u, sp[3];
        rb_pipe_contents(&arm[a], &m, &u, sp);
        m0 += m; u0 += u;
        for (int s = 0; s < 3; s++) s0[s] += sp[s];
    }
    double reflected = 0.0, transmitted = 0.0, inflow = 0.0, outflow = 0.0;
    for (int n = 0; n < 1100; n++) {
        rb_chamber_clear_rates(&junction);
        for (int a = 0; a < 4; a++) rb_pipe_clear(&arm[a], 0);
        for (int a = 0; a < 4; a++) rb_pipe_flows(&arm[a], 0, dt);
        rb_chamber_integrate(&junction, dt);
        for (int a = 0; a < 4; a++) rb_pipe_integrate(&arm[a], 0, dt);
        for (int a = 0; a < 4; a++) rb_pipe_accelerate(&arm[a], 0, dt);
        const double time = (n + 1) * dt;
        if (time > 0.004 && time < 0.005) {
            inflow = fmin(inflow, arm[0].end_face[0].mdot);
            outflow = fmax(outflow, arm[1].end_face[0].mdot);
        }
        if (time > 0.005 && time < 0.009) {
            reflected = fmin(reflected, cell[0][24].p - 101325.0);
            transmitted = fmax(transmitted, cell[1][24].p - 101325.0);
        }
    }
    CHECK_NEAR(reflected / 10.0, -0.5, 0.08);
    CHECK_NEAR(transmitted / 10.0, 0.5, 0.08);
    CHECK(inflow < -1e-6 && outflow > 1e-6); /* gas enters the junction and leaves down the other arms */
    double m1 = junction.mass, u1 = junction.U, s1[3];
    for (int s = 0; s < 3; s++) s1[s] = junction.m[s];
    for (int a = 0; a < 4; a++) {
        double m, u, sp[3];
        rb_pipe_contents(&arm[a], &m, &u, sp);
        m1 += m; u1 += u;
        for (int s = 0; s < 3; s++) s1[s] += sp[s];
    }
    CHECK_NEAR(m1, m0, m0 * 1e-11);
    CHECK_NEAR(u1, u0, fabs(u0) * 2e-7); /* initial kinetic-energy bookkeeping */
    for (int s = 0; s < 3; s++) CHECK_NEAR(s1[s], s0[s], m0 * 1e-11);
}
