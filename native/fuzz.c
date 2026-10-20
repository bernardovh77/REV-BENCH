/*
 * fuzz.c - random engines, run on their own crank and checked (see fuzz.h).
 */
#include "fuzz.h"

#include "engine.h"
#include "rb.h"
#include "rb_internal.h"   /* the sim's engine and its sound source, until stage 14 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define FRAMES 256                    /* audio frames per block */
#define AUDIO_RATE 48000.0

/* A random number in [0, 1): xorshift64*, its state seeded by splitmix64 so
 * neighbouring seeds give unrelated runs. */
static double uniform(unsigned long long *s)
{
    *s ^= *s >> 12;
    *s ^= *s << 25;
    *s ^= *s >> 27;
    return (double)((*s * 2685821657736338717ULL) >> 11) * (1.0 / 9007199254740992.0);
}

static unsigned long long seed_state(unsigned long long seed)
{
    unsigned long long z = seed + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= z >> 31;
    return z ? z : 1;
}

static int fail(rb_fuzz_result *r, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(r->what, sizeof r->what, fmt, ap);
    va_end(ap);
    r->ok = 0;
    return 0;
}

/* Checks one chamber and notes its extremes; 0 if it has gone wrong. */
static int check_chamber(rb_fuzz_result *r, const rb_chamber *ch, const char *name, int k, double t)
{
    if (ch->p > r->max_p) r->max_p = ch->p;
    if (ch->T > r->max_T) r->max_T = ch->T;
    if (ch->p > 0.0 && ch->p <= RB_FUZZ_MAX_P && ch->T > 0.0 && ch->T <= RB_FUZZ_MAX_T) return 1;
    return fail(r, "%s %d at %.3f s: p = %g Pa, T = %g K", name, k, t, ch->p, ch->T);
}

static int check_engine(rb_fuzz_result *r, const rb_engine *e, double t)
{
    if (fabs(e->rpm) > r->max_rpm) r->max_rpm = fabs(e->rpm);
    if (!(fabs(e->rpm) <= RB_FUZZ_MAX_RPM)) return fail(r, "the speed at %.3f s: %g rpm", t, e->rpm);
    for (int i = 0; i < e->desc.cylinders; i++)
        if (!check_chamber(r, &e->cyl[i].gas, "cylinder", i, t)) return 0;
    if (!check_chamber(r, &e->plenum, "plenum", 0, t) || !check_chamber(r, &e->airbox, "air box", 0, t))
        return 0;
    for (int c = 0; c < e->collectors; c++)
        if (!check_chamber(r, &e->collector[c], "collector", c, t)) return 0;
    for (int k = 0; k < rb_engine_pipe_count(e); k++) {
        const rb_pipe *p = rb_engine_pipe(e, k);
        for (int j = 0; j < p->cells; j++)
            if (!check_chamber(r, &p->cell[j], "pipe cell (pipe, cell)", k * 100 + j, t)) return 0;
    }
    return 1;
}

static long repairs(const rb_engine *e)
{
    long n = e->plenum.repairs + e->airbox.repairs;
    for (int i = 0; i < e->desc.cylinders; i++) n += e->cyl[i].gas.repairs;
    for (int c = 0; c < e->collectors; c++) n += e->collector[c].repairs;
    for (int k = 0; k < rb_engine_pipe_count(e); k++) {
        const rb_pipe *p = rb_engine_pipe(e, k);
        for (int j = 0; j < p->cells; j++) n += p->cell[j].repairs;
    }
    return n;
}

int rb_fuzz_run(unsigned long long seed, double seconds, int oversample, int quality,
                rb_fuzz_result *r)
{
    memset(r, 0, sizeof *r);
    r->ok = 1;
    r->preset = -1;
    unsigned long long s = seed_state(seed);
    rb_sim *sim = (quality >= 0) ? rb_create_with_quality(AUDIO_RATE, quality)
                                 : rb_create_with(AUDIO_RATE, oversample);
    if (!sim) return fail(r, "can't make a sim with oversample %d, quality %d", oversample, quality);

    /* The engine: a preset with a few fields changed, anywhere in their ranges. */
    r->preset = (int)(uniform(&s) * rb_preset_count());
    rb_load_preset(sim, r->preset);
    const int tries = 1 + (int)(uniform(&s) * 12.0);
    for (int k = 0; k < tries && k < RB_FUZZ_MAX_CHANGES; k++) {
        const int f = (int)(uniform(&s) * rb_engine_field_count());
        const int flags = rb_engine_field_flags(f);
        if (flags & RB_EF_READONLY) continue;
        const int size = rb_engine_field_size(sim, f);
        const int index = (size > 1) ? (int)(uniform(&s) * size) : 0;
        const double lo = rb_engine_field_min(f), hi = rb_engine_field_max(f);
        double v = lo + uniform(&s) * (hi - lo);
        if (flags & RB_EF_INTEGER) v = floor(v + 0.5);
        const int rc = rb_set_engine(sim, f, index, v);
        if (rc == RB_OK || rc == RB_CLAMPED) r->changes++;
        r->change[r->tries++] = (rb_fuzz_change){ .field = f, .index = index, .rc = rc, .value = v };
    }

    /* The listener, anywhere it can stand. */
    r->hear = (int)(uniform(&s) * RB_HEAR_COUNT);
    sim->hear = r->hear;
    rb_sim_listen(sim);

    /* The driver: the key to START, then the pedal anywhere, loads on the
     * crank and the ignition cut now and then (then started again if it
     * stalled). */
    rb_engine *e = &sim->engine;
    r->cylinders = e->desc.cylinders;
    sim->source = RB_SOURCE_ENGINE;
    e->hold_speed = 0;
    e->ignition = 1;
    e->starter = 1;
    float out[FRAMES];
    const double block = FRAMES / AUDIO_RATE;
    double t = 0.0, next_pedal = 0.0, cut_until = -1.0, cranking_since = 0.0;
    while (t < seconds && r->ok) {
        if (t >= next_pedal) {
            const double x = uniform(&s);
            e->throttle_pos = (x < 0.3) ? 0.0 : (x < 0.6) ? 1.0 : uniform(&s);
            e->load_torque = (uniform(&s) < 0.3) ? 100.0 * uniform(&s) : 0.0;
            next_pedal = t + 0.1 + 0.9 * uniform(&s);
        }
        if (e->starter && (e->rpm > 0.75 * e->desc.idle_rpm || t - cranking_since > 3.0))
            e->starter = 0;
        if (cut_until < 0.0 && uniform(&s) < block / 4.0)   /* about every 4 s */
            cut_until = t + 0.5 * uniform(&s);
        if (cut_until >= 0.0 && t >= cut_until) {
            cut_until = -1.0;
            if (e->rpm < 0.5 * e->desc.idle_rpm) {
                e->starter = 1;
                cranking_since = t;
            }
        }
        e->ignition = !(cut_until >= 0.0);

        rb_render_block(sim, out, FRAMES);
        t += block;
        for (int i = 0; i < FRAMES; i++)
            if (!(fabsf(out[i]) <= 1.0f)) {
                fail(r, "a sample at %.3f s: %g", t, (double)out[i]);
                break;
            }
        if (r->ok && rb_get_telemetry(sim, RB_TLM_NONFINITE) > 0.0)
            fail(r, "the sound had %g samples that weren't numbers by %.3f s",
                 rb_get_telemetry(sim, RB_TLM_NONFINITE), t);
        if (r->ok) check_engine(r, e, t);
    }
    r->seconds = t;
    r->repairs = repairs(e);
    rb_destroy(sim);
    return r->ok;
}
