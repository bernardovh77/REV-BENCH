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
 * outputs and reduces them to one sample. The engine's pipes step less
 * often, at about `pipe_rate`, and the quality levels (rb_config_quality)
 * set both rates together.
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
 *
 * The engine
 * ----------
 * The sim holds one engine (stage 8): it starts as preset 0 and can be
 * changed field by field. Its description (geometry, layout, valves, the
 * crank, the engine computer's settings) and each cylinder's state cross the
 * API as plain numbers with names and units, so the control panel and the 3D
 * view build themselves from the core and keep no copy of the engine.
 *
 * Driving it (stage 121)
 * ----------------------
 * RB_PARAM_SOURCE = RB_SOURCE_ENGINE makes the sim play the engine instead
 * of the stage 1 test tone. The driver's controls (ignition, starter,
 * throttle, load, the crank held or free, the car on it and the clutch) and
 * the settings (variation, the listener, the sound's layers, the burn model,
 * the seed) are parameters too; the engine's own numbers (rpm, MAP, torque,
 * the cuts...) are telemetry. A dyno the sim owns can hold the crank
 * (rb_dyno_*). A host that animates the engine reads coherent snapshots
 * (rb_get_snapshot, contracts/snapshot-v2.md) and the engine's actual events
 * (rb_read_events).
 *
 * Every rebuild (rb_reset, rb_load_preset, a field that isn't RB_EF_LIVE,
 * rb_set_quality) starts the engine afresh at rest, as contracts/runtime-v1.md
 * asks: the driver's controls go back to their defaults (key off, crank held
 * at 0 rpm, no car), the dyno comes off, the events are cleared, and the
 * generation (RB_TLM_GENERATION) goes up by one. The settings stay.
 */
#ifndef RB_H
#define RB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RB_VERSION_STRING "0.13.0"

/* The output's full scale: 1.0 out of rb_render_block is this many pascals
 * at the listener (at master_gain 1), so 94 dB SPL is 0.025 (stage 11). */
#define RB_OUTPUT_FULL_SCALE_PA 40.0

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
    RB_ERR_VALUE  = -3,  /* NaN or infinite value rejected, or a value
                            that would make an impossible engine      */
    RB_ERR_CONFIG = -4,  /* invalid rb_config                         */
    RB_ERR_READONLY = -5, /* the engine field can't be set            */
    RB_ERR_SIZE   = -6,  /* a buffer too small for what it must hold  */
    RB_PENDING    =  2   /* accepted; takes effect when the engine is next
                            rebuilt (it's applied now if the engine hasn't
                            run since its last rebuild)               */
};

/* Start from rb_config_default(): later stages may add fields. */
typedef struct rb_config {
    double sample_rate;  /* audio output rate, Hz        [8000, 384000] */
    int    oversample;   /* physics substeps per audio sample   [1, 32] */
    double pipe_rate;    /* how often the pipes step, Hz [12000, 96000];
                            0 = 48000 (stage 13). Faster than the physics:
                            every physics step                          */
} rb_config;

/* Quality (stage 13): the physics and the pipes' rates together. Each level
 * down costs less (the cross-plane V8 about 30 % less, then 30 % less again)
 * and moves the engine's torque by under 1 % (docs/stage-13.md). */
enum {
    RB_QUALITY_LOW = 0,  /* physics at about 96 kHz, the pipes at 24 kHz        */
    RB_QUALITY_MEDIUM,   /* physics at about 192 kHz, the pipes at 24 kHz       */
    RB_QUALITY_HIGH,     /* physics at about 192 kHz, the pipes at 48 kHz: the
                            default, and what the tests and renders use       */
    RB_QUALITY_ULTRA,    /* optional: physics 384 kHz, pipes 96 kHz, finer grid */
    RB_QUALITY_BALANCED, /* physics at about 96 kHz, the pipes at 48 kHz (stage
                            118: the four's real-time default)               */
    RB_QUALITY_COUNT
};

/* Parameters (inputs). Append new ids just before RB_PARAM_COUNT and add
 * a row to the table in core/src/rb_params.c. */
typedef enum rb_param {
    RB_PARAM_MASTER_GAIN = 0,   /* output gain (linear)                   */
    RB_PARAM_TEST_TONE_HZ,      /* stage-1 placeholder oscillator, Hz     */
    RB_PARAM_TEST_TONE_LEVEL,   /* stage-1 placeholder oscillator, 0..1   */
    /* Stage 121: what the sim plays ... */
    RB_PARAM_SOURCE,            /* RB_SOURCE_TONE (default) or _ENGINE    */
    /* ... the driver's controls (reset by every rebuild) ... */
    RB_PARAM_IGNITION,          /* 1: injectors and spark on              */
    RB_PARAM_STARTER,           /* 1: the key at START, the starter turning
                                   the free crank                         */
    RB_PARAM_THROTTLE,          /* the pedal, 0..1                        */
    RB_PARAM_LOAD_TORQUE,       /* N m the crank drives (< 0: it's driven);
                                   the car's road load comes on top       */
    RB_PARAM_HOLD,              /* 1 (default): a motor holds the crank at
                                   hold_rpm; 0: it turns free             */
    RB_PARAM_HOLD_RPM,          /* rpm while held                         */
    RB_PARAM_GEAR_INERTIA,      /* kg m^2: the crank's inertia with the car
                                   in gear (the engine's own plus the car's
                                   share); 0: no car                      */
    RB_PARAM_CLUTCH,            /* 1: the car is in gear on the crank, its
                                   inertia and road load with it; 0: the
                                   clutch is in. The crank keeps its speed
                                   either way.                            */
    /* ... and the settings (kept across rebuilds). */
    RB_PARAM_VARIATION,         /* how much no two cycles are alike: 0 (the
                                   default) none, exact repeatable numbers;
                                   1 a real engine's; up to 4             */
    RB_PARAM_LISTENER,          /* where the mono listener stands: RB_LISTENER_* */
    RB_PARAM_LAYERS,            /* which of the sound's layers: RB_LAYER_* bits */
    RB_PARAM_OVERRUN_TRANSITION,/* s from lifting off to the fuel cut      */
    RB_PARAM_BURN_MODEL,        /* RB_BURN_MODEL_* (research)             */
    RB_PARAM_SEED,              /* the variation's seed: 0 the default,
                                   else 1..4294967295                     */
    RB_PARAM_MOMENTUM_ORDER,    /* the pipes' momentum flux: 2 (default)
                                   second order, 1 first (research)       */
    RB_PARAM_EVENTS,            /* 1: record the engine's events
                                   (rb_read_events); 0 (default): none, at
                                   no cost                                */
    RB_PARAM_COUNT
} rb_param;

/* RB_PARAM_SOURCE's values. */
enum { RB_SOURCE_TONE = 0, RB_SOURCE_ENGINE = 1 };

/* RB_PARAM_LISTENER's values: where the listener stands around the car. */
enum {
    RB_LISTENER_TAILPIPE = 0,   /* a metre behind the tailpipe            */
    RB_LISTENER_BEHIND,         /* 3 m behind the car (the default)       */
    RB_LISTENER_SIDE,           /* beside it                              */
    RB_LISTENER_FRONT,          /* in front, by the engine bay            */
    RB_LISTENER_CABIN,          /* in the driver's seat                   */
    RB_LISTENER_COUNT
};

/* RB_PARAM_LAYERS's bits: the engine's sound in layers (all by default). */
enum {
    RB_LAYER_PIPES  = 1,        /* the pipe ends' flows (stages 11-12)    */
    RB_LAYER_FLOW   = 2,        /* the flow's noise: jets (stage 12b)     */
    RB_LAYER_ENGINE = 4,        /* the engine's own, mechanical sound     */
    RB_LAYER_ALL    = 7
};

/* RB_PARAM_BURN_MODEL's values. */
enum {
    RB_BURN_MODEL_CHARGE = 0,   /* the burn's length from the charge (default) */
    RB_BURN_MODEL_CAPPED = 1    /* stage 9's length, capped at 180 deg    */
};

/* Parameter flags. */
enum {
    RB_PF_LIVE    = 1 << 0,  /* takes effect immediately (smoothed)       */
    RB_PF_REBUILD = 1 << 1,  /* needs the engine to be rebuilt/re-initted:
                                rb_set_param returns RB_PENDING          */
    RB_PF_INTEGER = 1 << 2,  /* whole numbers only                        */
    RB_PF_CONTROL = 1 << 3   /* a driver's control: a rebuild resets it   */
};

/* Telemetry (outputs). Append new ids just before RB_TLM_COUNT. */
typedef enum rb_telemetry {
    RB_TLM_TIME_S = 0,   /* simulated time since init/reset, s        */
    RB_TLM_STEPS,        /* physics substeps since init/reset          */
    RB_TLM_OUT_PEAK,     /* peak |sample| of the last rendered block   */
    RB_TLM_OUT_RMS,      /* RMS of the last rendered block             */
    RB_TLM_NONFINITE,    /* NaN/inf samples replaced by the guard      */
    RB_TLM_LIMITED,      /* samples the soft limiter bent (stage 11)   */
    /* Stage 121: the sim ... */
    RB_TLM_GENERATION,   /* engine builds since rb_init: +1 per rebuild */
    RB_TLM_AUDIO_FRAMES, /* audio frames rendered since rb_init         */
    /* ... and the engine. From here on, each is one of the snapshots'
     * instruments, in this order (RB_SNAPSHOT_INSTRUMENTS). */
    RB_TLM_RPM,          /* crank speed, rpm (now)                      */
    RB_TLM_RPM_SMOOTH,   /* the engine computer's, over a few firings   */
    RB_TLM_CYCLE_RPM,    /* mean over the last complete cycle           */
    RB_TLM_MAP,          /* Pa: the intake manifold's (plenum) pressure */
    RB_TLM_TORQUE,       /* N m: brake torque over the last cycle       */
    RB_TLM_POWER,        /* W: brake power over the last cycle          */
    RB_TLM_LAMBDA,       /* the last cycle's air-fuel ratio over the
                            stoichiometric one (0 with no fuel)         */
    RB_TLM_SPARK,        /* deg before TDC: the last cycle's mean advance */
    RB_TLM_IDLE_VALVE,   /* the idle air valve's opening, 0..1          */
    RB_TLM_THROTTLE,     /* the pedal the engine has, 0..1              */
    RB_TLM_THROTTLE_AREA,/* m^2: the throttle's open area               */
    RB_TLM_IGNITION,     /* 1: on                                       */
    RB_TLM_STARTER,      /* 1: turning                                  */
    RB_TLM_HELD,         /* 1: the crank is held (by the motor or the dyno's
                            absence: see RB_PARAM_HOLD)                 */
    RB_TLM_FUEL_CUT,     /* 1 while the overrun fuel cut is on          */
    RB_TLM_SPARK_CUT,    /* 1 while the rev limiter's hard cut is on    */
    RB_TLM_LIMITER_CUTS, /* sparks the rev limiter has cut since the build */
    RB_TLM_OVERRUN_TIME, /* s into the current lift towards the fuel cut */
    RB_TLM_LOAD_TORQUE,  /* N m on the crank: the control, the road, the dyno */
    RB_TLM_CRANK_INERTIA,/* kg m^2 on the crank: the engine's, plus the car's
                            in gear                                     */
    RB_TLM_AIR_FLOW,     /* kg/s through the throttle, last cycle       */
    RB_TLM_FUEL_FLOW,    /* kg/s injected, last cycle                   */
    RB_TLM_EXHAUST_T,    /* K: the gas leaving the exhaust valves, last cycle */
    RB_TLM_TAILPIPE_T,   /* K: the gas leaving the tailpipes, last cycle */
    RB_TLM_COUNT
} rb_telemetry;

typedef struct rb_sim rb_sim;   /* opaque simulation state */

/* Stereo acoustic scenes (8..96 kHz audio). Source positions and acoustic
 * approximations are documented in docs/stage-S4.md. Existing mono listener
 * defaults are retained until rb_set_scene is called. */
enum {
    RB_SCENE_ASPHALT=0, RB_SCENE_GRASS, RB_SCENE_WALL, RB_SCENE_GARAGE,
    RB_SCENE_DYNO, RB_SCENE_ISO, RB_SCENE_PASSBY, RB_SCENE_CABIN, RB_SCENE_COUNT
};
int rb_set_scene(rb_sim *sim, int scene);
/* Interleaved L/R; returns frames, or RB_ERR_ARG. Requires rb_set_scene. */
int rb_render_stereo(rb_sim *sim, float *out, int frames);

/* ---- lifecycle ---------------------------------------------------- */
const char *rb_version(void);
void        rb_config_default(rb_config *cfg);
size_t      rb_sizeof(void);                       /* bytes for one rb_sim  */
int         rb_init(rb_sim *sim, const rb_config *cfg); /* cfg NULL = default */
/* Restarts the model: the engine afresh at rest (a rebuild: the driver's
 * controls at their defaults), the sound's filters cleared; the settings
 * and the output gain stay. */
void        rb_reset(rb_sim *sim);
rb_sim     *rb_create(const rb_config *cfg);       /* malloc + rb_init      */
/* Same as rb_create() for hosts that can't easily build a struct, such as
 * JavaScript. Returns NULL for an invalid rate/oversample. */
rb_sim     *rb_create_with(double sample_rate, int oversample);
/* Sets cfg's oversample and pipe_rate for a quality (RB_QUALITY_*) at its
 * sample_rate. RB_OK, or RB_ERR_ARG for a bad cfg, rate or quality. */
int         rb_config_quality(rb_config *cfg, int quality);
/* rb_create_with at a quality, for JavaScript. NULL for a bad rate or quality. */
rb_sim     *rb_create_with_quality(double sample_rate, int quality);
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
const char *rb_telemetry_unit(int id);  /* "" when it has none; NULL if unknown */

/* ---- quality, the variation's seed, the listener (stage 121) ------- */
/* Rebuilds the sim at a quality (RB_QUALITY_*) at its sample rate: the
 * engine starts afresh, the parameters stay. RB_OK or RB_ERR_ARG. */
int         rb_set_quality(rb_sim *sim, int quality);
/* The quality the sim was made at, or -1 if its rates match none. */
int         rb_get_quality(const rb_sim *sim);
/* The seed the engine's variation started from (RB_PARAM_SEED's, or the
 * default), 0 for NULL. */
uint64_t    rb_variation_seed(const rb_sim *sim);
/* m from the listener to a source: 0, 1 the tailpipes, 2 the intake's
 * mouth (0 for a tailpipe the engine hasn't); NaN if unknown. */
double      rb_listener_source_distance(const rb_sim *sim, int source);
/* Sets the stereo scene's clock (s; a pass-by is at its nearest at 0). A
 * host that lets the engine settle before recording starts it negative.
 * RB_ERR_ARG without a scene. */
int         rb_set_scene_time(rb_sim *sim, double seconds);

/* ---- a dyno the sim owns (stage 121) ------------------------------ */
/* An absorbing dyno on the crank (stage 9): a PI controller brakes the free
 * crank to hold a speed, up to its torque, and reads each cycle. It belongs
 * to the engine it was put on: a rebuild takes it off. */
int         rb_dyno_attach(rb_sim *sim, double rpm);  /* frees the crank; RB_OK,
                                                         RB_ERR_ARG, RB_ERR_VALUE */
int         rb_dyno_release(rb_sim *sim);             /* the crank turns free */
int         rb_dyno_attached(const rb_sim *sim);      /* 1, 0 */
/* Climbs the dyno's speed at `rate` rpm/s up to `to_rpm` (a sweep test). */
int         rb_dyno_set_sweep(rb_sim *sim, double to_rpm, double rate);
/* Forgets the readings so far (the next complete cycle is the first). */
int         rb_dyno_clear(rb_sim *sim);
typedef enum rb_dyno_field {
    RB_DYNO_READINGS = 0,       /* complete cycles read since attach/clear */
    RB_DYNO_TARGET_RPM,         /* the speed it holds now                 */
    RB_DYNO_RPM,                /* the last reading's: mean crank speed   */
    RB_DYNO_ABSORBED_TORQUE,    /* N m it took                            */
    RB_DYNO_BRAKE_TORQUE,       /* N m: absorbed, plus what sped the crank up */
    RB_DYNO_BRAKE_POWER,        /* W                                      */
    RB_DYNO_FIELD_COUNT
} rb_dyno_field;
double      rb_get_dyno(const rb_sim *sim, int field);   /* NaN without a dyno */

/* ---- snapshots (stage 121, contracts/snapshot-v2.md) --------------- */
/* One coherent picture of the engine, taken between two physics steps:
 * a header, the instruments (the telemetry from RB_TLM_RPM on, in order),
 * then RB_SNAPSHOT_CYLINDER_FIELDS doubles per cylinder. Little-endian, as
 * every platform the app runs on. */
#define RB_SNAPSHOT_VERSION          2
#define RB_SNAPSHOT_MAGIC            0x32534252u   /* "RBS2" */
#define RB_SNAPSHOT_INSTRUMENTS      (RB_TLM_COUNT - RB_TLM_RPM)
#define RB_SNAPSHOT_CYLINDER_FIELDS  5   /* pressure Pa, temperature K, burn
                                            fraction, intake and exhaust
                                            valve lift m */
typedef struct rb_snapshot_header {
    uint32_t magic, version;        /* RB_SNAPSHOT_MAGIC, _VERSION        */
    uint32_t size;                  /* bytes in the whole snapshot        */
    uint32_t cylinders, instruments, cylinder_fields;
    uint32_t reserved[2];           /* 0                                  */
    uint64_t build_id;              /* the engine description's hash     */
    uint64_t generation;            /* RB_TLM_GENERATION                  */
    uint64_t sequence;              /* +1 per snapshot taken              */
    uint64_t physics_tick;          /* steps since the build              */
    uint64_t audio_frame;           /* RB_TLM_AUDIO_FRAMES                */
    uint64_t cycle_index;           /* the crank's 720 deg cycles since the build */
    double   physics_hz, audio_hz;
    double   phase_rad;             /* in the cycle, [0, 4 pi)            */
    double   speed_rad_s;
} rb_snapshot_header;               /* 112 bytes                          */
/* Bytes a snapshot of the sim's engine takes now (0 for NULL). */
int         rb_snapshot_size(const rb_sim *sim);
/* Writes a snapshot of `version` (RB_SNAPSHOT_VERSION) into buffer: its
 * size in bytes, RB_ERR_ARG, RB_ERR_ID (a version it can't write) or
 * RB_ERR_SIZE (buffer smaller than rb_snapshot_size). */
int         rb_get_snapshot(rb_sim *sim, int version, void *buffer, int size);

/* ---- events (stage 121, contracts/runtime-v1.md) ------------------- */
/* What actually happens in the engine, recorded while RB_PARAM_EVENTS is
 * 1, in the order it happens, into a ring of RB_EVENT_CAPACITY. */
enum {
    RB_EVENT_SPARK_COMMAND = 0, /* the crank reaches the spark's angle    */
    RB_EVENT_SPARK_DISCHARGE,   /* the plug fires                         */
    RB_EVENT_FLAME_START,       /* the flame starts, after its delay      */
    RB_EVENT_FUEL_INJECTION,    /* fuel delivered, at IVC                 */
    RB_EVENT_INTAKE_OPEN,       /* the intake valve starts to open        */
    RB_EVENT_EXHAUST_OPEN,      /* the exhaust valve starts to open       */
    RB_EVENT_FUEL_CUT,          /* no fuel: the overrun fuel cut          */
    RB_EVENT_LIMITER_CUT,       /* no spark: the rev limiter              */
    RB_EVENT_KIND_COUNT
};
#define RB_EVENT_CAPACITY 256
typedef struct rb_event {
    uint64_t generation;        /* the engine build it happened in        */
    uint64_t sequence;          /* 0, 1, 2... in each generation          */
    uint64_t physics_tick;      /* the step it happened in ...            */
    double   subtick;           /* ... and how far through it, [0, 1)     */
    uint64_t cycle_index;       /* the crank's cycle and phase then       */
    double   phase_rad;
    uint64_t cycle_id;          /* the cylinder's charge it belongs to    */
    int32_t  kind;              /* RB_EVENT_*                             */
    int32_t  cylinder;          /* from 0                                 */
} rb_event;
const char *rb_event_kind_name(int kind);    /* "spark-command", ...; NULL if unknown */
/* Takes up to `max` of the oldest events out of the ring into out: how
 * many, or RB_ERR_ARG. */
int         rb_read_events(rb_sim *sim, rb_event *out, int max);
/* Events lost because the ring was full, since the build. */
uint64_t    rb_events_lost(const rb_sim *sim);

/* ---- the engine (stage 8) -------------------------------------------
 * Units are SI (m, m^2, m^3, Pa, K, kg m^2), except angles in degrees and
 * speeds in rpm. Crank angles are measured in the direction the crank turns.
 * A cylinder's cycle is its own 720 degrees from its firing TDC (at 360 it
 * is at the TDC between the exhaust and intake strokes). Cylinders and banks
 * are counted from 0 here: cylinder 1 is cylinder 0.
 *
 * The sim plays the stage 1 test tone until RB_PARAM_SOURCE asks for the
 * engine (stage 121). */

/* Presets: the engines that come with REV-BENCH. Loading one rebuilds the
 * engine, at rest. */
int         rb_preset_count(void);
const char *rb_preset_key(int preset);    /* "i4", "v8-cross", ...; NULL if unknown */
const char *rb_preset_name(int preset);   /* "2.0 L inline-4", ...; NULL if unknown */
int         rb_preset_find(const char *key);           /* preset number, or RB_ERR_ID */
/* The preset's real-time default quality (RB_QUALITY_*, stage 118's tier,
 * chosen on the baseline machine: docs/stage-118.md says what each one
 * misses), or RB_ERR_ID. A faster machine may run a higher one. */
int         rb_preset_quality(int preset);
int         rb_load_preset(rb_sim *sim, int preset);   /* RB_OK, RB_ERR_ARG or RB_ERR_ID */
/* The preset the engine was built from (changing fields since doesn't change
 * it), or RB_ERR_ARG. */
int         rb_get_preset(const rb_sim *sim);

/* The engine's description, field by field. Append new ids just before
 * RB_ENG_COUNT and add a row to the table in core/src/rb_engine_api.c. */
typedef enum rb_engine_field {
    RB_ENG_CYLINDERS = 0,       /* how many (comes with the preset)                */
    RB_ENG_BORE,                /* m                                               */
    RB_ENG_STROKE,              /* m                                               */
    RB_ENG_ROD,                 /* m: connecting rod length, centre to centre      */
    RB_ENG_COMPRESSION_RATIO,
    RB_ENG_DISPLACEMENT,        /* m^3, all the cylinders together (worked out)    */
    RB_ENG_BANKS,               /* 1 (inline) or 2 (V or boxer; comes with the preset) */
    RB_ENG_BANK_ANGLE,          /* deg, per bank: its cylinders' axis from vertical */
    RB_ENG_BANK,                /* per cylinder: the bank it's in                  */
    RB_ENG_PIN,                 /* deg, per cylinder: its crank pin's angle on the
                                   crank. The piston is at TDC when the pin points
                                   along the bank: at crank angle bank_angle - pin */
    RB_ENG_POSITION,            /* m, per cylinder: along the crank from the front */
    RB_ENG_FIRING_ORDER,        /* per place in the firing order: the cylinder
                                   that fires (cylinder 0 comes first)             */
    RB_ENG_FIRING_ANGLE,        /* deg, per cylinder: how long after cylinder 0's
                                   its firing TDC comes (worked out)               */
    RB_ENG_INTAKE_VALVES,       /* per cylinder, 1..4                              */
    RB_ENG_INTAKE_DIAMETER,     /* m: valve head                                   */
    RB_ENG_INTAKE_PORT,         /* m: the port's throat behind the valve           */
    RB_ENG_INTAKE_LIFT,         /* m: the most the valve lifts                     */
    RB_ENG_INTAKE_DURATION,     /* deg from opening to closing                     */
    RB_ENG_INTAKE_CENTER,       /* deg in the cycle where it lifts the most        */
    RB_ENG_INTAKE_OPENS,        /* deg in the cycle (worked out)                   */
    RB_ENG_INTAKE_CLOSES,       /* deg in the cycle (worked out)                   */
    RB_ENG_EXHAUST_VALVES,      /* the same for the exhaust valves                 */
    RB_ENG_EXHAUST_DIAMETER,
    RB_ENG_EXHAUST_PORT,
    RB_ENG_EXHAUST_LIFT,
    RB_ENG_EXHAUST_DURATION,
    RB_ENG_EXHAUST_CENTER,
    RB_ENG_EXHAUST_OPENS,
    RB_ENG_EXHAUST_CLOSES,
    RB_ENG_THROTTLE_DIAMETER,   /* m                                               */
    RB_ENG_PLENUM_VOLUME,       /* m^3: the intake manifold's                      */
    RB_ENG_INERTIA,             /* kg m^2: crankshaft, flywheel and clutch         */
    RB_ENG_IDLE_RPM,            /* the engine computer's idle speed (0 = no idle
                                   control and no fuel cut on overrun)             */
    RB_ENG_REV_LIMIT,           /* rpm where the rev limiter cuts the spark        */
    RB_ENG_RUNNER_LENGTH,       /* m: each intake runner, plenum to valve; its air
                                   rams the cylinder fullest at one speed, so it
                                   moves the torque peak                          */
    RB_ENG_RUNNER_DIAMETER,     /* m: the runners' bore                            */
    RB_ENG_HEADER_LENGTH,       /* m, per cylinder: its exhaust header (primary
                                   pipe), exhaust valve to collector (stage 10)    */
    RB_ENG_HEADER_DIAMETER,     /* m: the headers' bore                            */
    RB_ENG_COLLECTOR,           /* per cylinder: the collector its header joins, 0
                                   or 1 (a second only if there's a first)         */
    RB_ENG_EXHAUST_PIPE_LENGTH, /* m: each collector's pipe to its catalyst and
                                   muffler                                         */
    RB_ENG_EXHAUST_PIPE_DIAMETER, /* m                                             */
    RB_ENG_EXHAUST_SYSTEMS,     /* 1: every exhaust pipe into one muffler and
                                   tailpipe; 2 (with two collectors): each its own,
                                   a twin exhaust                                  */
    RB_ENG_MUFFLER_VOLUME,      /* m^3, each muffler                               */
    RB_ENG_TAILPIPE_LENGTH,     /* m, each                                         */
    RB_ENG_TAILPIPE_DIAMETER,   /* m                                               */
    RB_ENG_MUFFLER_TYPE,        /* an rb_muffler_type (stage 12)                   */
    RB_ENG_MUFFLER_LENGTH,      /* m, each: with the volume, a chambered one's
                                   width                                           */
    RB_ENG_AIRBOX_VOLUME,       /* m^3: the air box the throttle draws through its
                                   filter (stage 12)                               */
    RB_ENG_SNORKEL_LENGTH,      /* m: the air box's intake pipe, from the open air */
    RB_ENG_SNORKEL_DIAMETER,    /* m                                               */
    RB_ENG_ACCESSORY_POWER,     /* W the accessories take once the engine runs:
                                   the alternator, the pumps (stage 12c)           */
    RB_ENG_CATALYST_LAMINAR,    /* Pa per m/s: linear substrate loss; 0: bypass    */
    RB_ENG_OUTLET_RADIATION,    /* 1: the tailpipes' outlets radiate as unflanged
                                   pipes (causal load); 0: a constant end (stage 121
                                   made it public)                                */
    RB_ENG_COUNT
} rb_engine_field;

/* What each muffler is (stage 12), between the catalyst and the tailpipe. */
typedef enum rb_muffler_type {
    RB_MUFFLER_NONE = 0,        /* a straight pipe the tailpipe's size: no muffler */
    RB_MUFFLER_STRAIGHT,        /* straight-through: that pipe perforated, in a can
                                   of wool that takes the fast swings (pipe.h:
                                   packing), loud and deep                         */
    RB_MUFFLER_CHAMBERED        /* a wide can (the muffler's volume over its
                                   length) the gas leaves through baffles: quiet   */
} rb_muffler_type;

/* Engine field flags. */
enum {
    RB_EF_READONLY     = 1 << 0,  /* worked out, or comes with the preset       */
    RB_EF_INTEGER      = 1 << 1,  /* whole numbers only                          */
    RB_EF_PER_BANK     = 1 << 2,  /* one value per bank: index = bank            */
    RB_EF_PER_CYLINDER = 1 << 3,  /* one per cylinder: index = cylinder          */
    RB_EF_IN_ORDER     = 1 << 4,  /* one per place in the firing order           */
    RB_EF_LIVE         = 1 << 5   /* changes the running engine; changing any
                                     other field rebuilds it, at rest            */
};

/* A field's value. `index` is the bank, cylinder or place in the firing order
 * for the fields that have one value each, and 0 for the others. NaN for an
 * unknown field or index. */
double      rb_get_engine(const rb_sim *sim, int field, int index);
/* Changes a field: RB_OK; RB_CLAMPED (the value was outside the field's
 * range, rb_engine_field_min/max, and the nearest end was used); RB_ERR_ARG
 * (bad sim or index); RB_ERR_ID (unknown field); RB_ERR_READONLY; or
 * RB_ERR_VALUE when the value isn't a number (or a whole one where needed)
 * or would make an impossible engine (the engine stays as it was). Crank pin
 * and bank angles wrap around instead of being clamped. Putting cylinder c at
 * place k of the firing order swaps it with the cylinder that was there, so
 * the order always has every cylinder once. Changing the rev limit moves the
 * speed where the spark comes back by the same amount. */
int         rb_set_engine(rb_sim *sim, int field, int index, double value);
/* How many values a field has: 1, the banks or the cylinders (0 if unknown). */
int         rb_engine_field_size(const rb_sim *sim, int field);
int         rb_engine_field_count(void);
const char *rb_engine_field_name(int field);      /* NULL if unknown       */
const char *rb_engine_field_unit(int field);      /* "" when it has none   */
int         rb_engine_field_flags(int field);     /* RB_EF_* bits          */
double      rb_engine_field_min(int field);       /* the range a control   */
double      rb_engine_field_max(int field);       /* offers; NaN: read-only */
int         rb_engine_field_find(const char *name);   /* id, or RB_ERR_ID  */

/* Each cylinder's state, now. */
typedef enum rb_cylinder_field {
    RB_CYL_ANGLE = 0,       /* deg: where it is in its cycle, 0..720            */
    RB_CYL_PISTON,          /* m: the piston's distance below TDC               */
    RB_CYL_PRESSURE,        /* Pa                                               */
    RB_CYL_TEMPERATURE,     /* K                                                */
    RB_CYL_BURN,            /* 0..1: share of the fuel burned since the spark,
                               until the exhaust valve opens (0 the rest of the
                               cycle, and when the spark is cut)                */
    RB_CYL_BURNED_GAS,      /* 0..1: burned gas in the cylinder, by mass        */
    RB_CYL_INTAKE_LIFT,     /* m                                                */
    RB_CYL_EXHAUST_LIFT,    /* m                                                */
    RB_CYL_COUNT
} rb_cylinder_field;

double      rb_get_cylinder(const rb_sim *sim, int cylinder, int field);   /* NaN if unknown */
int         rb_cylinder_field_count(void);
const char *rb_cylinder_field_name(int field);    /* NULL if unknown       */
const char *rb_cylinder_field_unit(int field);

/* ---- the pipes (stage 10) --------------------------------------------
 * Pressure waves run along the engine's pipes. In order: the intake runners
 * (one per cylinder, if the engine has runners), the exhaust headers (one
 * per cylinder), the exhaust pipes (one per collector), the tailpipes (one
 * per exhaust system), and since stage 12 the mufflers (one per system) and
 * the intake's snorkel. Each runs from its end 0 to its end 1, cut into cells
 * of equal length, cell 0 at end 0. Gas flows from end 0 to end 1 in the
 * normal way of things: from the open air to the intake valve, from the
 * exhaust valve to the open air. */
typedef enum rb_pipe_kind {
    RB_PIPE_RUNNER = 0,         /* plenum -> intake valve                          */
    RB_PIPE_HEADER,             /* exhaust valve -> collector                      */
    RB_PIPE_EXHAUST,            /* collector -> catalyst and muffler               */
    RB_PIPE_TAILPIPE,           /* muffler -> the open air                         */
    RB_PIPE_MUFFLER,            /* the muffler: its inlet -> its outlet (stage 12) */
    RB_PIPE_SNORKEL,            /* the open air -> the air box (stage 12)          */
    RB_PIPE_CENTER,             /* intermediate silencer/pipe (13b-p)             */
    RB_PIPE_LINK                /* intermediate section -> rear system           */
} rb_pipe_kind;

/* What a pipe's end opens into. */
typedef enum rb_pipe_place {
    RB_PLACE_AIR = 0,           /* the open air                                    */
    RB_PLACE_PLENUM,            /* the intake plenum                               */
    RB_PLACE_INTAKE_VALVE,      /* a cylinder's intake valves (number: cylinder)   */
    RB_PLACE_EXHAUST_VALVE,     /* a cylinder's exhaust valves (number: cylinder)  */
    RB_PLACE_COLLECTOR,         /* an exhaust collector (number: collector)        */
    RB_PLACE_MUFFLER,           /* a muffler's inlet (number: exhaust system): its
                                   exhaust pipes open into it through their
                                   catalysts                                       */
    RB_PLACE_MUFFLER_OUTLET,    /* a muffler's outlet (number: system), where its
                                   tailpipe starts (stage 12)                      */
    RB_PLACE_AIRBOX,            /* the air box, through its filter (stage 12)      */
    RB_PLACE_CENTER,
    RB_PLACE_CENTER_OUTLET,
    RB_PLACE_JUNCTION           /* finite X/Y exhaust junction                    */
} rb_pipe_place;

/* A pipe's description. */
typedef enum rb_pipe_field {
    RB_PIPE_KIND = 0,           /* an rb_pipe_kind                                 */
    RB_PIPE_NUMBER,             /* the cylinder, collector or system it serves     */
    RB_PIPE_LENGTH,             /* m                                               */
    RB_PIPE_DIAMETER,           /* m                                               */
    RB_PIPE_CELLS,              /* how many                                        */
    RB_PIPE_FROM,               /* an rb_pipe_place: where end 0 opens ...         */
    RB_PIPE_FROM_NUMBER,        /* ... and which of them                           */
    RB_PIPE_TO,                 /* the same for end 1                              */
    RB_PIPE_TO_NUMBER,
    RB_PIPE_FIELD_COUNT
} rb_pipe_field;

int         rb_pipe_count(const rb_sim *sim);                     /* 0 for NULL  */
double      rb_get_pipe(const rb_sim *sim, int pipe, int field);  /* NaN if unknown */
int         rb_pipe_field_count(void);
const char *rb_pipe_field_name(int field);        /* NULL if unknown       */
const char *rb_pipe_field_unit(int field);

/* Each pipe cell's state, now. */
typedef enum rb_pipe_cell_field {
    RB_CELL_PRESSURE = 0,       /* Pa                                              */
    RB_CELL_TEMPERATURE,        /* K                                               */
    RB_CELL_VELOCITY,           /* m/s along the pipe (> 0: towards end 1)         */
    RB_CELL_BURNED_GAS,         /* 0..1: burned gas, by mass                       */
    RB_CELL_COUNT
} rb_pipe_cell_field;

double      rb_get_pipe_cell(const rb_sim *sim, int pipe, int cell, int field);  /* NaN if unknown */
int         rb_pipe_cell_field_count(void);
const char *rb_pipe_cell_field_name(int field);   /* NULL if unknown       */
const char *rb_pipe_cell_field_unit(int field);

/* ---- configuration accessors -------------------------------------- */
double rb_sample_rate(const rb_sim *sim);
int    rb_oversample(const rb_sim *sim);
double rb_physics_dt(const rb_sim *sim);
/* How often the engine's pipes step (Hz): the config's pipe_rate, rounded
 * to a whole number of physics steps (stage 13). NaN for a NULL sim. */
double rb_pipe_rate(const rb_sim *sim);

/* ---- host helpers ---------------------------------------------------
 * Scratch float buffers for hosts that cannot call malloc themselves
 * (JavaScript passes one to rb_render_block). The core never uses them. */
float *rb_alloc_floats(int count);   /* NULL if count <= 0 or no memory */
void   rb_free_floats(float *buf);   /* NULL is fine */

#ifdef __cplusplus
}
#endif

#endif /* RB_H */
