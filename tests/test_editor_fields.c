/*
 * test_editor_fields.c - stage 067's evidence for the editor: every engine field a control can
 * change (rb_engine_field_*, not read-only) has a measured effect, a valid range, a reversible
 * state and a coherent geometry. It goes through the core's own list, so a field added later is
 * tested too, never a hand-picked few.
 *
 * For each field, on a cross-plane V8 (two banks, two collectors) and a four: the field is set
 * to another value inside its range, the first the core accepts (an integer field tries every
 * value at every index; a field the preset's layout fixes, like a cylinder's bank, may accept
 * none, and then it is reported as fixed by the layout, see LAYOUT_FIXED). The engine is then
 * driven twice, held wide open and idling with its crank free, and the field has an effect when
 * its sound, its torque, its idle speed or its rev limiter's cuts move measurably from the plain
 * preset's (EFFECT_*). The rev limit, part of the engine computer, is driven free and wide open
 * too, and so are the limiter's character (stage 133); the 2-step's fields with it armed. A field that acts only with another part fitted is driven with that part (CONTEXT); one
 * with no effect on the sound or the running is NO_EFFECT, with why, and held to it.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "rb.h"
#include "rb_test.h"

#define RATE          48000
#define BLOCK         480
#define SETTLE_FRAMES 12000   /* 0.25 s held before measuring: the pipes fill            */
#define SOUND_FRAMES  12000   /* 0.25 s measured held                                     */
#define IDLE_FRAMES   57600   /* 1.2 s free at idle after a held start at 900 rpm         */
#define REV_FRAMES    72000   /* 1.5 s free, wide open: for the rev limiter (it is part of
                                 the engine computer, which is off while the crank is held) */
#define EFFECT_SOUND  0.01    /* relative RMS of the difference in sound                  */
#define EFFECT_TORQUE 0.005   /* relative change of the cycle's torque                    */
#define EFFECT_RPM    2.0     /* rpm of idle speed                                        */
#define PI            3.14159265358979323846

/* A field that changes nothing the engine does or the listener hears, and why; `banks` 0 for
 * any engine, else only one with that many banks. The editor hides these (EditorPanel). */
static const struct { const char *name; int banks; const char *why; } NO_EFFECT[] = {
    { "position", 0, "where the 3D view draws each cylinder along the crank; the model has no "
                     "crank vibration or port spacing for it to move (stage 070 draws it)" },
    { "bank_angle", 1, "on one bank it tilts the whole engine, which nothing in the model feels; "
                       "between two banks it sets the V's angle and matters" },
};

/* A field that acts only when another part is there: the drive fits it first, on both sides. */
static const struct { const char *name; int field; double value; } CONTEXT[] = {
    { "muffler_volume", RB_ENG_MUFFLER_TYPE, RB_MUFFLER_CHAMBERED },
    { "muffler_length", RB_ENG_MUFFLER_TYPE, RB_MUFFLER_CHAMBERED },
};

/* What one drive measured. */
typedef struct measured {
    float  sound[SOUND_FRAMES];
    double torque;
    double idle_rpm;
    double limiter_cuts;
} measured;

/* One change: a field's value at an index (field < 0: none), and a part fitted first. */
typedef struct change {
    int    field, index;
    double value;
    int    context_field;     /* -1: none */
    double context_value;
} change;

static void render(rb_sim *sim, float *out, int frames)
{
    static float scratch[BLOCK];
    for (int done = 0; done < frames; done += BLOCK) {
        const int n = frames - done < BLOCK ? frames - done : BLOCK;
        rb_render_block(sim, out ? out + done : scratch, n);
    }
}

/* A sim of the preset at the low quality with the change made, or NULL if the core refuses it. */
static rb_sim *make(int preset, const change *c)
{
    rb_sim *sim = rb_create_with_quality(RATE, RB_QUALITY_LOW);
    if (!sim) return NULL;
    rb_load_preset(sim, preset);
    if ((c->context_field >= 0 && rb_set_engine(sim, c->context_field, 0, c->context_value) < 0)
        || (c->field >= 0 && rb_set_engine(sim, c->field, c->index, c->value) < 0)) {
        rb_destroy(sim);
        return NULL;
    }
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    return sim;
}

/* Held wide open at held_rpm, then free at idle; for the rev limiter's fields, also free wide
 * open, with the 2-step armed for its own (`launch`). */
static void drive(int preset, const change *c, double held_rpm, int rev, int launch, measured *m)
{
    rb_sim *sim = make(preset, c);
    CHECK(sim != NULL);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, held_rpm);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1);
    render(sim, NULL, SETTLE_FRAMES);
    render(sim, m->sound, SOUND_FRAMES);
    m->torque = rb_get_telemetry(sim, RB_TLM_TORQUE);
    m->limiter_cuts = rb_get_telemetry(sim, RB_TLM_LIMITER_CUTS);
    rb_destroy(sim);

    sim = make(preset, c);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 900.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 0);
    render(sim, NULL, RATE / 5);
    rb_set_param(sim, RB_PARAM_HOLD, 0);   /* free: the engine computer holds the idle */
    render(sim, NULL, IDLE_FRAMES);
    m->idle_rpm = rb_get_telemetry(sim, RB_TLM_RPM);
    rb_destroy(sim);
    if (!rev) return;

    sim = make(preset, c);
    if (!sim) return;
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 1500.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 1);
    rb_set_param(sim, RB_PARAM_LAUNCH, launch);
    render(sim, NULL, RATE / 10);
    rb_set_param(sim, RB_PARAM_HOLD, 0);   /* free, wide open: it revs to its limit */
    render(sim, NULL, REV_FRAMES);
    m->limiter_cuts = rb_get_telemetry(sim, RB_TLM_LIMITER_CUTS);
    rb_destroy(sim);
}

/* How much two drives differ, each measure against its threshold: 1 is just measurable. */
static double effect(const measured *a, const measured *b)
{
    double num = 0.0, den = 0.0;
    for (int i = 0; i < SOUND_FRAMES; i++) {
        const double d = (double)a->sound[i] - (double)b->sound[i];
        num += d * d;
        den += (double)a->sound[i] * (double)a->sound[i];
    }
    const double sound = den > 0.0 ? sqrt(num / den) / EFFECT_SOUND : 0.0;
    const double torque = fabs(a->torque - b->torque) / fmax(fabs(a->torque), 1.0) / EFFECT_TORQUE;
    const double idle = fabs(a->idle_rpm - b->idle_rpm) / EFFECT_RPM;
    const double cuts = fabs(a->limiter_cuts - b->limiter_cuts);
    return fmax(fmax(sound, torque), fmax(idle, cuts));
}

/* Values to try for a non-integer field, largest changes first, inside its range. */
static int candidates(int field, double v0, double out[8])
{
    const double lo = rb_engine_field_min(field), hi = rb_engine_field_max(field);
    double all[8];
    int n = 0;
    if (!strcmp(rb_engine_field_unit(field), "deg")) {
        all[n++] = v0 + 20.0;
        all[n++] = v0 - 20.0;
    } else {
        all[n++] = v0 == 0.0 ? 0.5 * (lo + hi) : v0 * 2.0;
        all[n++] = v0 * 0.5;
        all[n++] = v0 * 1.25;
        all[n++] = v0 * 0.8;
        all[n++] = 0.5 * (lo + hi);
    }
    int kept = 0;
    for (int i = 0; i < n; i++)
        if (all[i] >= lo && all[i] <= hi && all[i] != v0) out[kept++] = all[i];
    return kept;
}

/* The geometry holds together: the displacement is the cylinders' swept volume, and the firing
 * order has every cylinder once. */
static void check_geometry(const rb_sim *sim, const char *what)
{
    const int n = (int)rb_get_engine(sim, RB_ENG_CYLINDERS, 0);
    const double b = rb_get_engine(sim, RB_ENG_BORE, 0), s = rb_get_engine(sim, RB_ENG_STROKE, 0);
    const double expected = n * 0.25 * PI * b * b * s;
    if (fabs(rb_get_engine(sim, RB_ENG_DISPLACEMENT, 0) - expected) > 1e-9 * expected) {
        printf("        %s: displacement %.9g, expected %.9g\n", what, rb_get_engine(sim, RB_ENG_DISPLACEMENT, 0), expected);
        CHECK(0);
    }
    int seen = 0;
    for (int k = 0; k < n; k++) seen |= 1 << (int)rb_get_engine(sim, RB_ENG_FIRING_ORDER, k);
    if (seen != (1 << n) - 1) {
        printf("        %s: the firing order lost a cylinder\n", what);
        CHECK(0);
    }
}

/* Every field's whole description is the same in both. */
static int same_description(const rb_sim *a, const rb_sim *b)
{
    for (int f = 0; f < rb_engine_field_count(); f++) {
        if (rb_engine_field_size(a, f) != rb_engine_field_size(b, f)) return 0;
        for (int k = 0; k < rb_engine_field_size(a, f); k++)
            if (rb_get_engine(a, f, k) != rb_get_engine(b, f, k)) return 0;
    }
    return 1;
}

static const char *no_effect_reason(const char *name, int banks)
{
    for (size_t i = 0; i < sizeof NO_EFFECT / sizeof NO_EFFECT[0]; i++)
        if (!strcmp(NO_EFFECT[i].name, name) && (NO_EFFECT[i].banks == 0 || NO_EFFECT[i].banks == banks))
            return NO_EFFECT[i].why;
    return NULL;
}

/* The part a field needs fitted, or none. */
static void context_for(const char *name, change *c)
{
    c->context_field = -1;
    for (size_t i = 0; i < sizeof CONTEXT / sizeof CONTEXT[0]; i++)
        if (!strcmp(CONTEXT[i].name, name)) {
            c->context_field = CONTEXT[i].field;
            c->context_value = CONTEXT[i].value;
        }
}

/* Finds a change of the field the core accepts and keeps, checking the geometry and that the
 * old value back restores the engine. 1 if found (in *c), 0 if the layout allows none. */
static int find_change(int preset, int f, change *c, const char *key)
{
    const change none = { -1, 0, 0.0, c->context_field, c->context_value };
    rb_sim *plain = make(preset, &none);
    if (!plain) return 0;
    const int size = rb_engine_field_size(plain, f);
    const int integer = (rb_engine_field_flags(f) & RB_EF_INTEGER) != 0;
    int found = 0;
    for (int index = size > 1 ? 1 : 0; index < size && !found; index++) {
        const double v0 = rb_get_engine(plain, f, index);
        double tries[64];
        int n = 0;
        if (integer) {
            for (double v = rb_engine_field_min(f); v <= rb_engine_field_max(f) && n < 64; v += 1.0)
                if (v != v0) tries[n++] = v;
        } else {
            n = candidates(f, v0, tries);
        }
        for (int i = 0; i < n && !found; i++) {
            rb_sim *probe = make(preset, &none);
            if (!probe) break;
            if (rb_set_engine(probe, f, index, tries[i]) >= 0 && rb_get_engine(probe, f, index) != v0) {
                found = 1;
                c->field = f;
                c->index = index;
                c->value = tries[i];
                check_geometry(probe, rb_engine_field_name(f));
                /* reversible: the old value back gives the plain preset's description */
                CHECK(rb_set_engine(probe, f, index, v0) >= 0);
                if (!same_description(probe, plain)) {
                    printf("        %s %s: setting it back doesn't restore the engine\n", key, rb_engine_field_name(f));
                    CHECK(0);
                }
            }
            rb_destroy(probe);
        }
    }
    rb_destroy(plain);
    return found;
}

static void each_field(const char *key)
{
    const int preset = rb_preset_find(key);
    static measured base, edited;
    int tested = 0, fixed = 0;
    for (int f = 0; f < rb_engine_field_count(); f++) {
        if (rb_engine_field_flags(f) & RB_EF_READONLY) continue;
        const char *name = rb_engine_field_name(f);
        CHECK(rb_engine_field_min(f) < rb_engine_field_max(f));   /* a valid range */
        change c = { -1, 0, 0.0, -1, 0.0 };
        context_for(name, &c);
        if (!find_change(preset, f, &c, key)) {
            /* fixed by the preset's layout: a cylinder's bank, a firing order the crank can't
             * have, a second exhaust with one collector; it changes only with other fields */
            printf("        %s %s: fixed by the layout (no single change accepted)\n", key, name);
            fixed++;
            continue;
        }
        const change plain = { -1, 0, 0.0, c.context_field, c.context_value };
        /* the rev limit is measured between the two limits: only one of them cuts */
        rb_sim *before = make(preset, &plain);
        const int banks = before ? (int)rb_get_engine(before, RB_ENG_BANKS, 0) : 0;
        rb_destroy(before);
        const int launch = f == RB_ENG_LAUNCH_RPM || f == RB_ENG_LAUNCH_GAP || f == RB_ENG_LAUNCH_BAND;
        const int rev = f == RB_ENG_REV_LIMIT || f == RB_ENG_LIMITER_GAP || f == RB_ENG_LIMITER_BAND
                     || f == RB_ENG_LIMITER_CUT || launch;
        drive(preset, &plain, 3000.0, rev, launch, &base);
        drive(preset, &c, 3000.0, rev, launch, &edited);
        const double e = effect(&base, &edited);
        const char *why = no_effect_reason(name, banks);
        if (why) {
            /* stated as having none: hold the statement to it */
            if (e >= 1.0) printf("        %s %s now has an effect (%.2f): NO_EFFECT is out of date\n", key, name, e);
            CHECK(e < 1.0);
        } else if (e < 1.0) {
            printf("        %s %s[%d] -> %g: no measurable effect (%.2f of the threshold)\n", key, name, c.index, c.value, e);
            CHECK(0);
        }
        tested++;
    }
    printf("        %s: %d fields with an effect checked, %d fixed by the layout\n", key, tested, fixed);
    CHECK(tested >= 28);
    CHECK(fixed <= 3);
}

void test_editor_every_field_has_an_effect(void)
{
    each_field("v8-cross");
    each_field("i4");
}
