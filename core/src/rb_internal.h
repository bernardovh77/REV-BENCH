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
#include "dyno.h"
#include "engine.h"
#include "ignition.h"
#include "injection.h"
#include "noise.h"
#include "scene.h"

#include <stdint.h>

#define RB_PI      3.14159265358979323846
#define RB_TWO_PI  6.28318530717958647692

/* Time constant for smoothing live parameters, so that sudden changes
 * (a slider jump, a level change) never produce clicks. */
#define RB_SMOOTH_TAU_S 0.010

/* What the listener hears (RB_SOURCE_*) and the sound's layers (RB_LAYER_*)
 * are public since stage 121 (rb.h). */

/* Records the engine's ignition and injection traces hold between two
 * physics steps before the sim turns them into events (stage 121): a step
 * has at most a few per cylinder. */
#define RB_TRACE_RECORDS (4 * RB_MAX_CYLINDERS)

/* The seed of the sim's noise: the same every time, so renders repeat. */
#define RB_NOISE_SEED 0x12B5EED12B5EEDULL

/* The most the listener's signal may be, in full-scale units (4e7 Pa):
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

    /* ---- the sound (stages 11 and 12, audio.h) ---- */
    int           source;      /* RB_SOURCE_TONE or RB_SOURCE_ENGINE      */
    int           hear;        /* where the listener stands: RB_HEAR_*    */
    rb_listener   listener;    /* hears the engine's pipe ends            */
    long          pipe_steps_heard;   /* the engine's pipe steps it has   */
    int           layers;      /* what of the engine's sound: RB_LAYER_*  */
    rb_noise      noise;       /* stage 12b: the flow's and the engine's  */
    rb_decimator  aa;          /* physics rate -> audio rate              */
    rb_dc_blocker dc;
    int scene_active;
    rb_scene scene;
    rb_decimator scene_aa[RB_SCENE_SOURCES];
    rb_dc_blocker scene_dc[2];
    double scene_flow[3];

    /* ---- model ----
     * Stage 1: a placeholder oscillator that proves the plumbing; stage 11
     * adds the engine (see `source`). `mic` is what the listener hears at
     * the current substep, in full-scale units. */
    double    tone_phase;   /* rad, kept in [0, 2pi)                      */
    double    tone_level_s; /* smoothed level                             */
    double    mic;

    /* ---- the engine (stage 8): described and changed through the API
     * (rb_engine_api.c); it makes the sound when `source` says so (stage 11),
     * which stage 14 does for the browser. Its description (engine.desc) is
     * always a possible engine. */
    int       preset;       /* the preset it was built from               */
    rb_engine engine;       /* stays where rb_init put it (see engine.h)  */
    rb_dyno  *dyno;         /* stage 12c: a dyno on the crank, stepped
                             * before the engine every physics step; a
                             * rebuilt engine drops it. NULL: none. Since
                             * stage 121 it's own_dyno (rb_dyno_attach), or
                             * a test's own                                */

    /* ---- stage 121: the public engine API ---- */
    uint64_t  generation;   /* engine builds since rb_init                 */
    uint64_t  frames;       /* audio frames rendered since rb_init         */
    uint64_t  snapshot_sequence;
    double    own_inertia;  /* kg m^2: the engine's own, as described; the
                             * engine's desc.inertia adds the car's in gear */
    rb_dyno   own_dyno;
    /* the engine's events (RB_PARAM_EVENTS): a ring of the oldest unread */
    rb_event  event[RB_EVENT_CAPACITY];
    int       event_head, event_count;
    uint64_t  events_lost, event_sequence;
    rb_ignition_trace ignition_trace;
    rb_ignition_event ignition_records[RB_TRACE_RECORDS];
    rb_injection_trace injection_trace;
    rb_injection_event injection_records[RB_TRACE_RECORDS];
    double    event_theta;  /* the crank before the step in progress       */
    double    event_angle[RB_MAX_CYLINDERS];   /* each cylinder's then     */
    long      event_cycles;
};

/* Coefficient a for the smoother  y += a * (target - y)  run every dt. */
double rb_onepole_coef(double dt, double tau);

/* Puts the listener at sim->hear around the engine the sim has now (its
 * tailpipes, its pipes' step, its air's speed of sound), hearing the engine's
 * pipe ends as they are: call it whenever the engine is built, or the
 * listener moves. Nothing jumps. */
void rb_sim_listen(rb_sim *sim);

/* Builds the sim's engine afresh from a description (stage 121, the reset
 * recipe of contracts/runtime-v1.md): at rest, the driver's controls at their
 * defaults, the settings applied, no dyno, no events, the listener placed, the
 * generation one more. -1 (nothing changed) if d isn't a possible engine. */
int rb_sim_build(rb_sim *sim, const rb_engine_desc *d);

/* The sim's engine description as the host set it: the engine's own, without
 * the car's inertia in gear (stage 121). */
rb_engine_desc rb_sim_desc(const rb_sim *sim);

/* Applies parameter id's value (sim->param[id], already in range) to the
 * engine and the sound (rb_params.c calls it): RB_OK, or RB_PENDING for a
 * setting that waits for a rebuild. */
int rb_sim_apply(rb_sim *sim, int id);

/* Rebuilds the whole sim at another config (rb_set_quality): the parameters,
 * the engine's description and the stereo scene stay; the engine starts
 * afresh. RB_OK or RB_ERR_CONFIG. */
int rb_sim_reconfigure(rb_sim *sim, const rb_config *cfg);

/* Before each rendered block: the road's load on a car in gear (rb_drive.c). */
void rb_sim_before_block(rb_sim *sim);

/* The engine's telemetry, RB_TLM_RPM on (rb_drive.c). */
double rb_sim_engine_telemetry(const rb_sim *sim, int id);

/* The engine's events (rb_events.c): set up (or stop) recording them as
 * RB_PARAM_EVENTS says, the ring empty; and, after each physics step while
 * recording, take what the step did. */
void rb_sim_events_setup(rb_sim *sim);
void rb_sim_events_step(rb_sim *sim);

#endif /* RB_INTERNAL_H */
