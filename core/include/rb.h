/*
 * rb.h - REV-BENCH simulation core, public API
 *
 * The core is plain C11 that only depends on libm. It never allocates
 * memory on its own: rb_init() works on memory you hand it, and
 * rb_create()/rb_destroy() are just malloc/free convenience wrappers.
 * That keeps it real-time safe and lets the exact same code run natively
 * (tests, offline WAV renders) and as WebAssembly inside the browser's
 * AudioWorklet.
 *
 * Time
 * ----
 * The physics advances in fixed substeps that are `oversample` times
 * shorter than one audio sample:
 *
 *     dt = 1 / (sample_rate * oversample)
 *
 * rb_render_block() runs `oversample` substeps for every audio sample it
 * outputs and reduces them to one sample.
 *
 * Inputs and outputs
 * ------------------
 * Everything that crosses the API is a plain number, which keeps the
 * JavaScript side trivial:
 *   - parameters (inputs):  rb_set_param() / rb_get_param(), ids in rb_param
 *   - telemetry (outputs):  rb_get_telemetry(), ids in rb_telemetry
 * Every parameter also has metadata (name, unit, range, default, flags),
 * so the UI can build its controls from the core instead of duplicating
 * that information.
 */
#ifndef RB_H
#define RB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RB_VERSION_STRING "0.6.0"

/* Limits for rb_config. */
#define RB_MIN_SAMPLE_RATE  8000.0
#define RB_MAX_SAMPLE_RATE  384000.0
#define RB_MAX_OVERSAMPLE   32

/* Return codes. */
enum {
    RB_OK         =  0,
    RB_CLAMPED    =  1,  /* value accepted but clamped into its range */
    RB_ERR_ARG    = -1,  /* NULL pointer or bad argument              */
    RB_ERR_ID     = -2,  /* unknown parameter / telemetry id          */
    RB_ERR_VALUE  = -3,  /* NaN or infinite value rejected            */
    RB_ERR_CONFIG = -4   /* invalid rb_config                         */
};

typedef struct rb_config {
    double sample_rate;  /* audio output rate, Hz        [8000, 384000] */
    int    oversample;   /* physics substeps per audio sample   [1, 32] */
} rb_config;

/* Parameters (inputs). Append new ids just before RB_PARAM_COUNT and add
 * a row to the table in core/src/rb_params.c. */
typedef enum rb_param {
    RB_PARAM_MASTER_GAIN = 0,   /* output gain (linear)                   */
    RB_PARAM_TEST_TONE_HZ,      /* stage-1 placeholder oscillator, Hz     */
    RB_PARAM_TEST_TONE_LEVEL,   /* stage-1 placeholder oscillator, 0..1   */
    RB_PARAM_COUNT
} rb_param;

/* Parameter flags. */
enum {
    RB_PF_LIVE    = 1 << 0,  /* takes effect immediately (smoothed)       */
    RB_PF_REBUILD = 1 << 1   /* needs the engine to be rebuilt/re-initted */
};

/* Telemetry (outputs). Append new ids just before RB_TLM_COUNT. */
typedef enum rb_telemetry {
    RB_TLM_TIME_S = 0,   /* simulated time since init/reset, s        */
    RB_TLM_STEPS,        /* physics substeps since init/reset          */
    RB_TLM_OUT_PEAK,     /* peak |sample| of the last rendered block   */
    RB_TLM_OUT_RMS,      /* RMS of the last rendered block             */
    RB_TLM_NONFINITE,    /* NaN/inf samples replaced by the guard      */
    RB_TLM_COUNT
} rb_telemetry;

typedef struct rb_sim rb_sim;   /* opaque simulation state */

/* ---- lifecycle ---------------------------------------------------- */
const char *rb_version(void);
void        rb_config_default(rb_config *cfg);
size_t      rb_sizeof(void);                       /* bytes for one rb_sim  */
int         rb_init(rb_sim *sim, const rb_config *cfg); /* cfg NULL = default */
void        rb_reset(rb_sim *sim);   /* restart the model, keep parameters  */
rb_sim     *rb_create(const rb_config *cfg);       /* malloc + rb_init      */
/* Same as rb_create() for hosts that can't easily build a struct, such as
 * JavaScript. Returns NULL for an invalid rate/oversample. */
rb_sim     *rb_create_with(double sample_rate, int oversample);
void        rb_destroy(rb_sim *sim);               /* free (NULL is fine)   */

/* ---- running ------------------------------------------------------ */
/* Advance the physics by one substep (sim must be valid). */
void rb_step(rb_sim *sim);
/* Render `frames` mono samples in [-1, 1] at the audio rate. Returns the
 * number of frames written, or a negative RB_ERR_* code. */
int  rb_render_block(rb_sim *sim, float *out, int frames);

/* ---- parameters --------------------------------------------------- */
int         rb_set_param(rb_sim *sim, int id, double value); /* RB_OK/RB_CLAMPED/RB_ERR_* */
double      rb_get_param(const rb_sim *sim, int id);         /* NaN if id is unknown */
int         rb_param_count(void);
const char *rb_param_name(int id);      /* NULL if id is unknown */
const char *rb_param_unit(int id);
double      rb_param_min(int id);
double      rb_param_max(int id);
double      rb_param_default(int id);
int         rb_param_flags(int id);     /* RB_PF_* bits */
int         rb_param_find(const char *name);  /* id, or RB_ERR_ID */

/* ---- telemetry ---------------------------------------------------- */
double      rb_get_telemetry(const rb_sim *sim, int id);     /* NaN if unknown */
int         rb_telemetry_count(void);
const char *rb_telemetry_name(int id);

/* ---- configuration accessors -------------------------------------- */
double rb_sample_rate(const rb_sim *sim);
int    rb_oversample(const rb_sim *sim);
double rb_physics_dt(const rb_sim *sim);

/* ---- host helpers ---------------------------------------------------
 * Scratch float buffers for hosts that cannot call malloc themselves
 * (JavaScript passes one to rb_render_block). The core never uses them. */
float *rb_alloc_floats(int count);   /* NULL if count <= 0 or no memory */
void   rb_free_floats(float *buf);   /* NULL is fine */

#ifdef __cplusplus
}
#endif

#endif /* RB_H */
