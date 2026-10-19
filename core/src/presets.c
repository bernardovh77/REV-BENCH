/*
 * presets.c - the engines REV-BENCH comes with (see presets.h).
 *
 * Every preset starts from the default engine (engine.c) and changes what
 * makes it different. Numbering: cylinders count from the front; on V and
 * boxer engines the odd ones are on bank 0, the even ones on bank 1, and
 * cylinders 1 and 2 share the front of the crank.
 */
#include "presets.h"

#include "rb.h"

#include <string.h>

static const double MM = 1e-3, L = 1e-3, CM2 = 1e-4, MM2 = 1e-6;

typedef struct preset_info {
    const char *key;     /* stable identifier for the UI and the CLI */
    const char *name;    /* for people                                */
    void (*build)(rb_engine_desc *d);
} preset_info;

/* Valves: count per cylinder, head diameter, lift, duration, and the lobe
 * centerline cam-card style (intake after overlap TDC, exhaust before). The
 * port throat is 0.85 times the valve. */
static rb_valve valve(int count, double diameter, double lift, double duration, double centerline,
                      int intake)
{
    return (rb_valve){ count, diameter, 0.85 * diameter, lift, duration,
                       intake ? 360.0 + centerline : 360.0 - centerline, 0.7 };
}

/* The layout: bank angles, then for each cylinder its bank, pin and position,
 * then the firing order as cylinder numbers counted from 1. */
static void layout(rb_engine_desc *d, int banks, double angle0, double angle1, const int *bank,
                   const double *pin, const double *position, const int *order_from_1)
{
    d->banks = banks;
    d->bank_angle[0] = angle0;
    d->bank_angle[1] = angle1;
    for (int i = 0; i < d->cylinders; i++) {
        d->bank[i] = bank[i];
        d->pin[i] = pin[i];
        d->position[i] = position[i];
        d->firing_order[i] = order_from_1[i] - 1;
    }
    rb_engine_layout(d);
}

/* Intake, exhaust and the crank's own settings, sized for the engine. */
static void breathing(rb_engine_desc *d, double throttle, double leak, double plenum,
                      double manifold, double catalyst, double muffler, double tailpipe)
{
    d->throttle.diameter = throttle;
    d->throttle.shaft = throttle > 0.07 ? 0.010 : 0.008;
    d->throttle.leak_area = leak;
    d->plenum_volume = plenum;
    d->exhaust_volume = manifold;
    d->catalyst_area = catalyst;
    d->muffler_volume = muffler;
    d->tailpipe_area = tailpipe;
}

/* Each cylinder's intake runner, plenum to valve: its length sets where the
 * air in it rams the cylinder fullest, and so where the torque peaks. */
static void runners(rb_engine_desc *d, double length, double diameter)
{
    d->runner_length = length;
    d->runner_diameter = diameter;
}

/* The crank, the starter, and the engine computer's idle and rev limit. Each
 * idle valve is sized so that the warm engine idles with it about 15 % open,
 * where the engine computer starts it, and each engine's idle gains are tuned
 * on it (docs/stage-08.md, retuned in docs/stage-09.md): start, a blip of the
 * throttle and a 10 N m load at idle, with room for gains 1.5 times as strong
 * before the idle hunts. */
static void running(rb_engine_desc *d, double inertia, double starter, double starter_rpm,
                    double idle, double idle_valve, double gain_p, double gain_i, double limit)
{
    d->inertia = inertia;
    d->starter_torque = starter;
    d->starter_free_rpm = starter_rpm;
    d->idle_rpm = idle;
    d->idle_valve_area = idle_valve;
    d->idle_gain_p = gain_p;
    d->idle_gain_i = gain_i;
    d->rev_limit = limit;
    d->rev_limit_resume = limit - 100.0;
}

/* 2.0 L inline-4: the default engine of stages 5-7. */
static void inline4(rb_engine_desc *d)
{
    rb_engine_desc_default(d);
}

/* 3.0 L inline-6, 84 x 90 mm. The crank pairs 1-6, 2-5 and 3-4 at 120
 * degrees, so it fires every 120 degrees in the order 1-5-3-6-2-4, and both
 * its primary and secondary forces cancel: a smooth engine. */
static void inline6(rb_engine_desc *d)
{
    static const int bank[6] = { 0, 0, 0, 0, 0, 0 };
    static const double pin[6] = { 0.0, 240.0, 120.0, 120.0, 240.0, 0.0 };
    static const int order[6] = { 1, 5, 3, 6, 2, 4 };
    double position[6];
    rb_engine_desc_default(d);
    d->cylinders = 6;
    d->bore = 84 * MM;
    d->stroke = 90 * MM;
    d->rod = 145 * MM;
    d->cr = 10.5;
    for (int i = 0; i < 6; i++) position[i] = 0.091 * i;
    layout(d, 1, 0.0, 0.0, bank, pin, position, order);
    d->intake = valve(2, 32 * MM, 9.7 * MM, 250.0, 110.0, 1);
    d->exhaust = valve(2, 27 * MM, 9.3 * MM, 250.0, 110.0, 0);
    breathing(d, 70 * MM, 3 * MM2, 4.0 * L, 3.0 * L, 28 * CM2, 15 * L, 28 * CM2);
    runners(d, 420 * MM, 40 * MM);
    running(d, 0.20, 250.0, 250.0, 700.0, 40 * MM2, 2.0e-4, 3.0e-4, 7000.0);
}

/* 5.0 L 90-degree V8, 92 x 94 mm, with a cross-plane crank: four throws at
 * 0, 90, 270 and 180 degrees, each shared by a cylinder from each bank. It
 * fires every 90 degrees (1-8-4-3-6-5-7-2), but each bank on its own fires
 * unevenly, 90-270-180-180 degrees apart: the off-beat burble of an American
 * V8. */
static void v8_cross(rb_engine_desc *d)
{
    static const int bank[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    static const double pin[8] = { 0.0, 0.0, 90.0, 90.0, 270.0, 270.0, 180.0, 180.0 };
    static const int order[8] = { 1, 8, 4, 3, 6, 5, 7, 2 };
    double position[8];
    rb_engine_desc_default(d);
    d->cylinders = 8;
    d->bore = 92 * MM;
    d->stroke = 94 * MM;
    d->rod = 155 * MM;
    d->cr = 10.5;
    for (int i = 0; i < 8; i++) position[i] = 0.111 * (i / 2) + 0.022 * (i % 2);
    layout(d, 2, 45.0, -45.0, bank, pin, position, order);
    d->intake = valve(2, 36 * MM, 11.0 * MM, 260.0, 112.0, 1);
    d->exhaust = valve(2, 31 * MM, 10.5 * MM, 260.0, 112.0, 0);
    breathing(d, 80 * MM, 4 * MM2, 6.0 * L, 4.0 * L, 40 * CM2, 20 * L, 40 * CM2);
    runners(d, 450 * MM, 45 * MM);
    running(d, 0.30, 350.0, 220.0, 650.0, 68 * MM2, 2.0e-4, 5.0e-4, 7000.0);
}

/* 4.5 L 90-degree V8, 94 x 81 mm, with a flat-plane crank: two inline-4s on
 * one flat crank, the banks taking turns. Every bank fires evenly every 180
 * degrees (1-6-5-8-7-4-3-2 overall): the high-revving, rasping kind. Short
 * stroke, light flywheel, bigger cams and 12.5:1 compression. */
static void v8_flat(rb_engine_desc *d)
{
    static const int bank[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    static const double pin[8] = { 0.0, 0.0, 180.0, 180.0, 180.0, 180.0, 0.0, 0.0 };
    static const int order[8] = { 1, 6, 5, 8, 7, 4, 3, 2 };
    double position[8];
    rb_engine_desc_default(d);
    d->cylinders = 8;
    d->bore = 94 * MM;
    d->stroke = 81 * MM;
    d->rod = 135 * MM;
    d->cr = 12.5;
    for (int i = 0; i < 8; i++) position[i] = 0.104 * (i / 2) + 0.020 * (i % 2);
    layout(d, 2, 45.0, -45.0, bank, pin, position, order);
    d->intake = valve(2, 38 * MM, 12.0 * MM, 270.0, 108.0, 1);
    d->exhaust = valve(2, 32 * MM, 11.0 * MM, 270.0, 108.0, 0);
    breathing(d, 90 * MM, 4 * MM2, 6.0 * L, 4.0 * L, 45 * CM2, 18 * L, 45 * CM2);
    runners(d, 300 * MM, 50 * MM);
    running(d, 0.18, 300.0, 250.0, 1000.0, 82 * MM2, 2.5e-4, 6.0e-4, 9000.0);
}

/* 2.0 L flat-4 (boxer), 84 x 90 mm: the banks 180 degrees apart, opposite
 * pistons moving in and out together on throws 180 degrees apart. It fires
 * evenly every 180 degrees, 1-3-2-4. */
static void boxer4(rb_engine_desc *d)
{
    static const int bank[4] = { 0, 1, 0, 1 };
    static const double pin[4] = { 0.0, 180.0, 180.0, 0.0 };
    static const double position[4] = { 0.0, 0.040, 0.113, 0.153 };
    static const int order[4] = { 1, 3, 2, 4 };
    rb_engine_desc_default(d);
    d->bore = 84 * MM;
    d->stroke = 90 * MM;
    d->rod = 130 * MM;
    d->cr = 10.5;
    layout(d, 2, 90.0, -90.0, bank, pin, position, order);
    d->intake = valve(2, 32 * MM, 9.5 * MM, 250.0, 110.0, 1);
    d->exhaust = valve(2, 27 * MM, 9.0 * MM, 250.0, 110.0, 0);
    runners(d, 320 * MM, 40 * MM);
    running(d, 0.15, 200.0, 250.0, 750.0, 27 * MM2, 3.0e-4, 4.0e-4, 6500.0);
}

static const preset_info PRESETS[] = {
    { "i4", "2.0 L inline-4", inline4 },
    { "i6", "3.0 L inline-6", inline6 },
    { "v8-cross", "5.0 L cross-plane V8", v8_cross },
    { "v8-flat", "4.5 L flat-plane V8", v8_flat },
    { "boxer4", "2.0 L flat-4 (boxer)", boxer4 },
};
enum { N_PRESETS = sizeof PRESETS / sizeof PRESETS[0] };

int rb_preset_count(void)
{
    return N_PRESETS;
}

const char *rb_preset_key(int preset)
{
    return (preset >= 0 && preset < N_PRESETS) ? PRESETS[preset].key : NULL;
}

const char *rb_preset_name(int preset)
{
    return (preset >= 0 && preset < N_PRESETS) ? PRESETS[preset].name : NULL;
}

int rb_preset_find(const char *key)
{
    if (!key) return RB_ERR_ARG;
    for (int i = 0; i < N_PRESETS; i++)
        if (!strcmp(key, PRESETS[i].key)) return i;
    return RB_ERR_ID;
}

int rb_preset_desc(int preset, rb_engine_desc *d)
{
    if (!d || preset < 0 || preset >= N_PRESETS) return -1;
    PRESETS[preset].build(d);
    return 0;
}
