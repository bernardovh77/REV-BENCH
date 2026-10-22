/*
 * presets.c - the engines REV-BENCH comes with (see presets.h).
 *
 * Every preset starts from the default engine (engine.c) and changes what
 * makes it different. Numbering: cylinders count from the front; on V and
 * boxer engines the odd ones are on bank 0, the even ones on bank 1, and
 * cylinders 1 and 2 share the front of the crank.
 */
#include "presets.h"
#include "exhaust_models.h"

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

/* The throttle and the plenum, sized for the engine. */
static void intake(rb_engine_desc *d, double throttle, double leak, double plenum)
{
    d->throttle.diameter = throttle;
    d->throttle.shaft = throttle > 0.07 ? 0.010 : 0.008;
    d->throttle.leak_area = leak;
    d->plenum_volume = plenum;
}

/* Each cylinder's intake runner, plenum to valve: its length sets where the
 * air in it rams the cylinder fullest, and so where the torque peaks. */
static void runners(rb_engine_desc *d, double length, double diameter)
{
    d->runner_length = length;
    d->runner_diameter = diameter;
}

/* The exhaust (stage 10): every cylinder's header (a length each, from
 * `header`, and the collector it joins, from `collector`), then each
 * collector's exhaust pipe, one or two systems (catalyst, muffler and
 * tailpipe each), sized for the engine. */
static void exhaust(rb_engine_desc *d, const double *header, double header_diameter,
                    const int *collector, double pipe, double pipe_diameter, int systems,
                    double catalyst, double muffler, double muffler_area, double tailpipe,
                    double tailpipe_diameter)
{
    for (int i = 0; i < d->cylinders; i++) {
        d->header_length[i] = header[i];
        d->collector[i] = collector[i];
    }
    d->header_diameter = header_diameter;
    d->exhaust_pipe_length = pipe;
    d->exhaust_pipe_diameter = pipe_diameter;
    d->exhaust_systems = systems;
    d->catalyst_area = catalyst;
    d->muffler_volume = muffler;
    d->muffler_area = muffler_area;
    d->tailpipe_length = tailpipe;
    d->tailpipe_diameter = tailpipe_diameter;
}

/* The muffler (stage 12): what it is (rb.h: rb_muffler_type) and how long;
 * the volume (exhaust()) is a chambered one's. */
static void muffler(rb_engine_desc *d, int type, double length)
{
    d->muffler_type = type;
    d->muffler_length = length;
}

/* The way in (stage 12): the air box and the snorkel it draws through. */
static void airbox(rb_engine_desc *d, double volume, double snorkel, double snorkel_diameter)
{
    d->airbox_volume = volume;
    d->snorkel_length = snorkel;
    d->snorkel_diameter = snorkel_diameter;
}

/* The crank, the starter, the accessories, and the engine computer's idle
 * and rev limit. Each idle valve is sized so that the warm engine idles with
 * it about 15 % open, where the engine computer starts it, and each engine's
 * idle gains are tuned on it (docs/stage-08.md, retuned in docs/stage-09.md,
 * docs/stage-10.md and docs/stage-12c.md): start, a blip of the throttle and
 * a 10 N m load at idle, with room for gains 1.5 times as strong before the
 * idle hunts. */
static void running(rb_engine_desc *d, double inertia, double starter, double starter_rpm,
                    double accessories, double idle, double idle_valve, double gain_p,
                    double gain_i, double limit)
{
    d->accessory_power = accessories;
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

/* 2.0 L inline-4: the default engine of stages 5-7, but for its headers.
 * Like a production manifold's, they aren't all alike (stage 12c): from 58 cm
 * at the front to 30 cm, as they reach a collector behind cylinder 3, so each
 * cylinder's pulse arrives its own way and they don't all sound the same.
 * Equal headers made the four a pure note: what's between its firing orders
 * was 26 dB under them at 3000 rpm, where real engines' are 2-12 dB under. */
static void inline4(rb_engine_desc *d)
{
    static const double header[4] = { 0.58, 0.40, 0.30, 0.52 };
    rb_engine_desc_default(d);
    for (int i = 0; i < 4; i++) d->header_length[i] = header[i];
    d->accessory_power = 800.0;
    d->idle_valve_area = 47 * MM2;   /* for the accessories: about 15 % open idling, */
    d->idle_gain_p = 2.4e-4;         /* the gains less as the valve is bigger        */
    d->idle_gain_i = 2.4e-4;
}

/* 3.0 L inline-6, 84 x 90 mm. The crank pairs 1-6, 2-5 and 3-4 at 120
 * degrees, so it fires every 120 degrees in the order 1-5-3-6-2-4, and both
 * its primary and secondary forces cancel: a smooth engine. Its headers
 * meet in two collectors of three, the front and the rear cylinders, each
 * firing every 240 degrees, so one cylinder's exhaust pulse doesn't push
 * against the next one's overlap. Each collector sits between the two
 * middle cylinders' ports, so the outer cylinders' headers are longest. */
static void inline6(rb_engine_desc *d)
{
    static const int bank[6] = { 0, 0, 0, 0, 0, 0 };
    static const double pin[6] = { 0.0, 240.0, 120.0, 120.0, 240.0, 0.0 };
    static const int order[6] = { 1, 5, 3, 6, 2, 4 };
    static const double header[6] = { 0.48, 0.40, 0.32, 0.32, 0.40, 0.48 };   /* stage 12c */
    static const int collector[6] = { 0, 0, 0, 1, 1, 1 };
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
    airbox(d, 10 * L, 400 * MM, 90 * MM);
    intake(d, 70 * MM, 3 * MM2, 4.0 * L);
    runners(d, 420 * MM, 40 * MM);
    exhaust(d, header, 36 * MM, collector, 1.6, 50 * MM, 1, 16 * CM2, 15 * L, 28 * CM2, 0.7, 60 * MM);
    muffler(d, RB_MUFFLER_CHAMBERED, 550 * MM);
    running(d, 0.20, 250.0, 250.0, 950.0, 700.0, 56 * MM2, 4.4e-4, 3.2e-4, 7000.0);
}

/* 5.0 L 90-degree V8, 92 x 94 mm, with a cross-plane crank: four throws at
 * 0, 90, 270 and 180 degrees, each shared by a cylinder from each bank. It
 * fires every 90 degrees (1-8-4-3-6-5-7-2), but each bank on its own fires
 * unevenly, 90-180-270-180 degrees apart: the off-beat burble of an American
 * V8, which its twin exhaust (each bank its own collector, pipe, muffler and
 * tailpipe) keeps apart. Each bank's manifold leaves at its back, so the
 * front cylinders' headers are longest (stage 12c). The default sound setup
 * now uses unfilled pipes, following Bernardo's straight-piped Mustang
 * reference; --muffler straight retains the former packed setup. This is
 * an open-exhaust example, not a calibrated replica of that Mustang. */
static void v8_cross(rb_engine_desc *d)
{
    static const int bank[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    static const double pin[8] = { 0.0, 0.0, 90.0, 90.0, 270.0, 270.0, 180.0, 180.0 };
    static const int order[8] = { 1, 8, 4, 3, 6, 5, 7, 2 };
    static const double header[8] = { 0.52, 0.52, 0.44, 0.44, 0.36, 0.36, 0.28, 0.28 };
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
    airbox(d, 12 * L, 400 * MM, 90 * MM);
    intake(d, 80 * MM, 4 * MM2, 6.0 * L);
    runners(d, 450 * MM, 45 * MM);
    exhaust(d, header, 40 * MM, bank, 2.0, 55 * MM, 2, 20 * CM2, 10 * L, 20 * CM2, 0.8, 60 * MM);
    muffler(d, RB_MUFFLER_NONE, 450 * MM);
    running(d, 0.30, 350.0, 220.0, 1250.0, 650.0, 95 * MM2, 5.1e-4, 2.6e-4, 7000.0);
}

/* 4.5 L 90-degree V8, 94 x 81 mm, with a flat-plane crank: two inline-4s on
 * one flat crank, the banks taking turns. Every bank fires evenly every 180
 * degrees (1-6-5-8-7-4-3-2 overall): the high-revving, rasping kind. Short
 * stroke, light flywheel, bigger cams and 12.5:1 compression, and long tuned
 * headers into each bank's own exhaust. */
static void v8_flat(rb_engine_desc *d)
{
    static const int bank[8] = { 0, 1, 0, 1, 0, 1, 0, 1 };
    static const double pin[8] = { 0.0, 0.0, 180.0, 180.0, 180.0, 180.0, 0.0, 0.0 };
    static const int order[8] = { 1, 6, 5, 8, 7, 4, 3, 2 };
    static const double header[8] = { 0.60, 0.60, 0.60, 0.60, 0.60, 0.60, 0.60, 0.60 };
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
    airbox(d, 12 * L, 300 * MM, 90 * MM);
    intake(d, 90 * MM, 4 * MM2, 6.0 * L);
    runners(d, 300 * MM, 50 * MM);
    exhaust(d, header, 42 * MM, bank, 1.4, 60 * MM, 2, 23 * CM2, 9 * L, 23 * CM2, 0.5, 70 * MM);
    muffler(d, RB_MUFFLER_STRAIGHT, 400 * MM);
    running(d, 0.18, 300.0, 250.0, 950.0, 1000.0, 135 * MM2, 4.2e-4, 3.4e-4, 9000.0);
}

/* 2.0 L flat-4 (boxer), 84 x 90 mm: the banks 180 degrees apart, opposite
 * pistons moving in and out together on throws 180 degrees apart. It fires
 * evenly every 180 degrees, 1-3-2-4, two cylinders of one bank and then two
 * of the other. Its headers meet in one collector under the engine, the
 * second bank's 30 cm longer (they cross under it): their pulses arrive
 * unevenly, the rumble of a flat four with unequal-length headers. */
static void boxer4(rb_engine_desc *d)
{
    static const int bank[4] = { 0, 1, 0, 1 };
    static const double pin[4] = { 0.0, 180.0, 180.0, 0.0 };
    static const double position[4] = { 0.0, 0.040, 0.113, 0.153 };
    static const int order[4] = { 1, 3, 2, 4 };
    static const double header[4] = { 0.45, 0.75, 0.45, 0.75 };
    static const int collector[4] = { 0, 0, 0, 0 };
    rb_engine_desc_default(d);
    d->bore = 84 * MM;
    d->stroke = 90 * MM;
    d->rod = 130 * MM;
    d->cr = 10.5;
    layout(d, 2, 90.0, -90.0, bank, pin, position, order);
    d->intake = valve(2, 32 * MM, 9.5 * MM, 250.0, 110.0, 1);
    d->exhaust = valve(2, 27 * MM, 9.0 * MM, 250.0, 110.0, 0);
    runners(d, 320 * MM, 40 * MM);
    exhaust(d, header, 36 * MM, collector, 1.6, 50 * MM, 1, 21 * CM2, 10 * L, 21 * CM2, 0.6, 50 * MM);
    /* New split rear: calibrate the equivalent idle-air actuator from the
     * measured startup response (13b-p), retaining the air-area PI gains. */
    running(d, 0.15, 200.0, 250.0, 800.0, 750.0, 46 * MM2,
            2.6e-4 * 43.0 / 46.0, 3.1e-4 * 43.0 / 46.0, 6500.0);
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
    rb_exhaust_model(d, preset);
    return 0;
}
