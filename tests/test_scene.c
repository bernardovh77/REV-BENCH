#include "rb.h"
#include "rb_internal.h"
#include "rb_test.h"
#include "scene.h"
#include <math.h>
#include <string.h>

static rb_scene dry, room;

void test_scene_ear_geometry(void)
{
    rb_scene_init(&dry, RB_SCENE_ASPHALT, 48000, 343, 1, 0.06);
    /* Independent geometry from the documented source and ear coordinates.
     * Merely seeing unequal L/R samples would miss a broken interaural delay
     * if head shadow happened to keep their levels different. */
    const double left = sqrt(0.01 * 0.01 + 2.95 * 2.95 + 1.25 * 1.25);
    const double right = sqrt(0.19 * 0.19 + 2.95 * 2.95 + 1.25 * 1.25);
    CHECK_NEAR(dry.path[0][0][0].delay, left / 343.0 * 48000.0, 1e-8);
    CHECK_NEAR(dry.path[1][0][0].delay, right / 343.0 * 48000.0, 1e-8);
}

void test_scene_retarded_time(void)
{
    double r[3] = {0, 34.3, 0};
    CHECK_NEAR(rb_scene_flight_time(r, 0, 343), 0.1, 1e-12);
    CHECK_NEAR(rb_scene_flight_time(r, 20, 343), 34.3 / 363.0, 1e-12);
    r[1] = -34.3;
    CHECK_NEAR(rb_scene_flight_time(r, 20, 343), 34.3 / 323.0, 1e-12);
    r[0] = 7.5;
    const double t = rb_scene_flight_time(r, 20, 343);
    CHECK_NEAR(343 * t, sqrt(r[0] * r[0] + pow(r[1] - 20 * t, 2)), 1e-10);
}

void test_scene_propagation_and_room(void)
{
    CHECK(rb_scene_init(&dry, RB_SCENE_ASPHALT, 48000, 343, 1, 0.06) == 0);
    CHECK(rb_scene_init(&room, RB_SCENE_GARAGE, 48000, 343, 1, 0.06) == 0);
    double tail_dry = 0, tail_room = 0, early = 0, late = 0, maximum = 0;
    int first = -1;
    for (int i = 0; i < 96000; i++) {
        double in[5] = {i == 0 ? 1.0 : 0.0, 0, 0, 0, 0}, a[2], b[2];
        rb_scene_step(&dry, in, a);
        rb_scene_step(&room, in, b);
        if (first < 0 && fabs(a[0]) > 1e-7)
            first = i;
        double p = b[0] * b[0] + b[1] * b[1];
        if (i > 4800) {
            tail_dry += a[0] * a[0] + a[1] * a[1];
            tail_room += p;
        }
        if (i > 5000 && i < 20000)
            early += p;
        if (i > 70000)
            late += p;
        maximum = fmax(maximum, fmax(fabs(b[0]), fabs(b[1])));
    }
    CHECK(first > 400 && first < 500); /* 3.2 m / 343 m/s, including disk spread */
    CHECK(tail_dry < 1e-12 && tail_room > 1e-8);
    CHECK(late < early * 0.01 && maximum < 2.0);
    CHECK(rb_scene_init(&dry, -1, 48000, 343, 1, .06) < 0);
    CHECK(rb_scene_init(&dry, RB_SCENE_ASPHALT, 192000, 343, 1, .06) < 0);
}

void test_scene_api_repeatability(void)
{
    rb_sim *a = rb_create(NULL), *b = rb_create(NULL);
    CHECK(a && b);
    if (!a || !b) {
        rb_destroy(a);
        rb_destroy(b);
        return;
    }
    CHECK(rb_set_scene(a, RB_SCENE_COUNT) == RB_ERR_ARG);
    CHECK(rb_render_stereo(a, NULL, 10) == RB_ERR_ARG);
    a->source = b->source = RB_SOURCE_ENGINE;
    a->engine.ignition = b->engine.ignition = 1;
    a->engine.rpm = b->engine.rpm = 3000;
    CHECK(rb_set_scene(a, RB_SCENE_WALL) == RB_OK);
    CHECK(rb_set_scene(b, RB_SCENE_WALL) == RB_OK);
    float x[2048], y[2048];
    CHECK_EQ_INT(rb_render_stereo(a, x, 1024), 1024);
    CHECK_EQ_INT(rb_render_stereo(b, y, 317), 317);
    CHECK_EQ_INT(rb_render_stereo(b, y + 634, 707), 707);
    CHECK(memcmp(x, y, sizeof x) == 0);
    int stereo = 0, sane = 1;
    for (int i = 0; i < 1024; i++) {
        stereo |= x[2 * i] != x[2 * i + 1];
        sane &= isfinite(x[2 * i]) && isfinite(x[2 * i + 1]) && fabs(x[2 * i]) <= 1 &&
                fabs(x[2 * i + 1]) <= 1;
    }
    CHECK(stereo && sane);
    rb_reset(a);
    CHECK(a->scene_active && a->scene.kind == RB_SCENE_WALL && a->scene.time == 0);
    rb_destroy(a);
    rb_destroy(b);
}

void test_scene_doppler_tone(void)
{
    rb_scene_init(&dry, RB_SCENE_PASSBY, 48000, 343, 1, 0.06);
    int approaching = 0, receding = 0;
    double previous = 0;
    for (int i = 0; i < 480000; i++) {
        double source[5] = {sin(2.0 * 3.14159265358979323846 * 600.0 * i / 48000.0), 0, 0, 0, 0};
        double out[2];
        rb_scene_step(&dry, source, out);
        if (previous < 0 && out[0] >= 0) {
            if (i >= 168000 && i < 216000)
                approaching++;
            if (i >= 360000 && i < 408000)
                receding++;
        }
        previous = out[0];
    }
    CHECK(approaching > 620 && approaching < 632);
    CHECK(receding > 568 && receding < 582);
}

void test_scene_aperture_radiation(void)
{
    rb_scene_init(&dry, RB_SCENE_ISO, 48000, 343, 1, 0.06);
    /* Isolate the aperture from reflections and the approximate head filter. */
    for (int e = 0; e < 2; e++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 3; k++) {
                if (j != 0 || k != 0)
                    dry.path[e][j][k].gain = 0;
                dry.path[e][j][k].shadow = 0;
            }
    const double f = 4000.0, spread = dry.path[0][0][0].spread;
    const double x = 2.0 * 3.14159265358979323846 * f * spread / 48000.0;
    double term = 1.0, airy = 1.0;
    for (int k = 1; k < 20; k++) {
        term *= (-x * x / 4.0) / (k * (k + 1));
        airy += term;
    }
    double energy = 0;
    for (int i = 0; i < 12000; i++) {
        double source[5] = {sin(2.0 * 3.14159265358979323846 * f * i / 48000.0), 0, 0, 0, 0},
               out[2];
        rb_scene_step(&dry, source, out);
        if (i >= 6000)
            energy += out[0] * out[0];
    }
    const double amplitude = sqrt(2.0 * energy / 6000.0) / dry.path[0][0][0].gain;
    CHECK(fabs(amplitude - fabs(airy)) < 0.04);
    CHECK(amplitude < 0.95); /* off-axis high-frequency beaming is not a no-op */
}

void test_ultra_spatial_resolution(void)
{
    rb_sim *a = rb_create_with_quality(48000, RB_QUALITY_HIGH);
    rb_sim *b = rb_create_with_quality(48000, RB_QUALITY_ULTRA);
    CHECK(a && b);
    if (a && b) {
        int high = 0, ultra = 0;
        for (int i = 0; i < rb_engine_pipe_count(&a->engine); i++)
            high += rb_engine_pipe(&a->engine, i)->cells;
        for (int i = 0; i < rb_engine_pipe_count(&b->engine); i++)
            ultra += rb_engine_pipe(&b->engine, i)->cells;
        CHECK(ultra > 1.8 * high && ultra <= RB_ENGINE_CELLS);
        CHECK_NEAR(b->engine.dt, 0.5 * a->engine.dt, 1e-18);
    }
    rb_destroy(a);
    rb_destroy(b);
}
