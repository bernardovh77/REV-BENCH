/*
 * rb_internal.h - the simulation state, shared by the core's .c files.
 *
 * Not part of the public API: hosts (CLI, browser) only see rb.h.
 * Native tests and debugging tools may include it to peek inside.
 */
#ifndef RB_INTERNAL_H
#define RB_INTERNAL_H

#include "rb.h"

#include "audio.h"
#include "engine.h"

#include <stdint.h>

#define RB_PI      3.14159265358979323846
#define RB_TWO_PI  6.28318530717958647692

/* Time constant for smoothing live parameters, so that sudden changes
 * (a slider jump, a level change) never produce clicks. */
#define RB_SMOOTH_TAU_S 0.010

/* What the microphone hears. Until stage 14 the browser plays stage 1's test
 * tone; native tools (rb_cli sound, the tests) switch to the engine. */
enum { RB_SOURCE_TONE = 0, RB_SOURCE_ENGINE };

/* The most the microphone's signal may be, in full-scale units (2e7 Pa):
 * beyond any real engine, it keeps the filters' sums finite. */
#define RB_MIC_MAX 1e6

struct rb_sim {
    rb_config cfg;
    double    dt;           /* physics substep, s                         */
    uint64_t  steps;        /* substeps since init/reset                  */

    double    param[RB_PARAM_COUNT];  /* targets set by the host, clamped */

    /* One-pole smoothing coefficients (see rb_onepole_coef). */
    double    smooth_a_step;          /* per physics substep */
    double    smooth_a_frame;         /* per audio frame     */

    /* ---- output stage ---- */
    double    gain_s;       /* smoothed master gain                       */
    double    out_peak;     /* stats of the last rendered block           */
    double    out_rms;
    uint64_t  nonfinite;    /* samples replaced by the NaN/inf guard      */
    uint64_t  limited;      /* samples the soft limiter bent              */

    /* ---- the sound (stage 11, audio.h) ---- */
    int           source;      /* RB_SOURCE_TONE or RB_SOURCE_ENGINE      */
    rb_mic        microphone;  /* hears the engine's tailpipes            */
    rb_decimator  aa;          /* physics rate -> audio rate              */
    rb_dc_blocker dc;

    /* ---- model ----
     * Stage 1: a placeholder oscillator that proves the plumbing; stage 11
     * adds the engine (see `source`). `mic` is what the virtual microphone
     * hears at the current substep, in full-scale units. */
    double    tone_phase;   /* rad, kept in [0, 2pi)                      */
    double    tone_level_s; /* smoothed level                             */
    double    mic;

    /* ---- the engine (stage 8): described and changed through the API
     * (rb_engine_api.c); it makes the sound when `source` says so (stage 11),
     * which stage 14 does for the browser. Its description (engine.desc) is
     * always a possible engine. */
    int       preset;       /* the preset it was built from               */
    rb_engine engine;       /* stays where rb_init put it (see engine.h)  */
};

/* Coefficient a for the smoother  y += a * (target - y)  run every dt. */
double rb_onepole_coef(double dt, double tau);

#endif /* RB_INTERNAL_H */
