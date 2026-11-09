/*
 * rb_host.h - REV-BENCH's real-time audio host (stage 123).
 *
 * The host runs the core (rb.h) in real time without Node or JavaScript:
 *
 *   control thread (the caller)      render thread            device thread
 *   -------------------------------  -----------------------  ---------------
 *   rb_host_set_param ... -> commands -> applied at the start  (miniaudio's, or
 *   rb_host_poll          <- acks      of each block            your own: the
 *   rb_host_snapshot_read <- snapshots rb_render_block into     host's rb_host_pull)
 *   rb_host_read_events   <- events    the audio ring ------->  reads the ring
 *   rb_host_prepare_begin: a worker builds a new engine; rb_host_swap hands it
 *   to the render thread, which fades the old one out, switches at a block
 *   boundary, fades the new one in and acknowledges the new generation. The
 *   old engine comes back on the retired queue and rb_host_poll frees it.
 *
 * Threads. Every rb_host_* function is for ONE control thread (the caller),
 * except rb_host_pull (the device's thread: one at a time) and the readers
 * of statistics and of the clock, which any thread may call. The render
 * thread never allocates, locks or logs; the device thread never runs the
 * physics. All the channels between the threads are lock-free and
 * preallocated (docs/stage-123.md, contracts/runtime-v1.md).
 *
 * Backends. NULL is miniaudio's null device (what the tests use): a real
 * device thread that pulls audio at the pace of a real one and plays it
 * nowhere. DEFAULT is the system's real audio. MANUAL has no device thread:
 * the caller pulls with rb_host_pull, so tests make the device's timing
 * exact. OFFLINE has no thread at all: rb_host_offline_render renders the
 * blocks itself, for scripted traces and WAV export.
 */
#ifndef RB_HOST_H
#define RB_HOST_H

#include <stddef.h>
#include <stdint.h>

#include "rb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The engine renders blocks of this many frames; a command applies at the
 * start of one (rb_cli sound has always used 128). */
#define RB_HOST_BLOCK_FRAMES        128
/* The device is stereo: a mono engine plays in both ears. */
#define RB_HOST_CHANNELS            2
#define RB_HOST_MAX_EDITS           64
#define RB_HOST_MAX_PARAMS          32
#define RB_HOST_COMMAND_CAPACITY    256
#define RB_HOST_ACK_CAPACITY        256
#define RB_HOST_EVENT_CAPACITY      1024
#define RB_HOST_RETIRED_CAPACITY    4
/* A snapshot slot holds at most this many bytes of the core's snapshot
 * (contracts/runtime-v1.md's budget). */
#define RB_HOST_SNAPSHOT_MAX_BYTES  65536
#define RB_HOST_MAX_PREPARED        8
/* rb_host_swap with this expected generation skips the staleness check. */
#define RB_HOST_ANY_GENERATION      UINT64_MAX
/* The deadline histogram: bins 0..9 hold a block's render time in tenths of
 * its duration (bin 3: 30-40 %), bin 10 a time of one to two durations (the
 * deadline missed), bin 11 two or more. */
#define RB_HOST_DEADLINE_BINS       12

/* Return codes (negative: an error). */
enum {
    RB_HOST_OK          =  0,
    RB_HOST_ERR_ARG     = -1,   /* a bad argument                            */
    RB_HOST_ERR_FULL    = -2,   /* a queue is full: nothing was sent         */
    RB_HOST_ERR_NOMEM   = -3,
    RB_HOST_ERR_STATE   = -4,   /* not in a state that allows it             */
    RB_HOST_ERR_DEVICE  = -5,   /* the audio device failed                   */
    RB_HOST_ERR_NOCLOCK = -6,   /* no valid clock mapping yet                */
    RB_HOST_ERR_BUILD   = -7,   /* the engine can't be built                 */
    RB_HOST_ERR_THREAD  = -8
};

typedef enum rb_host_backend {
    RB_HOST_BACKEND_NULL = 0,   /* miniaudio's null device: tests            */
    RB_HOST_BACKEND_DEFAULT,    /* the system's real audio                   */
    RB_HOST_BACKEND_MANUAL,     /* render thread only: the caller pulls      */
    RB_HOST_BACKEND_OFFLINE,    /* no threads: rb_host_offline_render        */
    RB_HOST_BACKEND_COUNT
} rb_host_backend;

/* ---- what an engine build is ------------------------------------------ */
typedef struct rb_host_edit {
    int32_t field, index;           /* rb_engine_field, and its bank/cylinder */
    double  value;
} rb_host_edit;

typedef struct rb_host_param {
    int32_t id;                     /* rb_param                               */
    double  value;
} rb_host_param;

/* Everything needed to prepare an engine off the render thread. It is made
 * the way rb_cli sound makes one: the sim at the host's rate, the preset,
 * its fields as asked, the parameters, then the scene. Start from
 * rb_host_build_default. */
typedef struct rb_host_build {
    int32_t       preset;           /* rb_preset_find's number                */
    int32_t       quality;          /* RB_QUALITY_*, or -1: rb_config_default
                                       with `oversample`                      */
    int32_t       oversample;       /* with quality -1: 0 means the default   */
    int32_t       scene;            /* RB_SCENE_*, or -1: the mono listener   */
    double        scene_time;       /* rb_set_scene_time's, when scene >= 0   */
    int32_t       settle_blocks;    /* blocks rendered and thrown away before
                                       the engine is handed over              */
    int32_t       n_edits, n_params;
    rb_host_edit  edits[RB_HOST_MAX_EDITS];
    rb_host_param params[RB_HOST_MAX_PARAMS];
} rb_host_build;

/* Preset 0, the default quality, the mono listener, and the engine as the
 * source (RB_PARAM_SOURCE). */
void rb_host_build_default(rb_host_build *b);
/* Appends an edit or a parameter: RB_HOST_OK or RB_HOST_ERR_FULL. */
int  rb_host_build_edit(rb_host_build *b, int field, int index, double value);
int  rb_host_build_param(rb_host_build *b, int id, double value);

/* ---- configuration ---------------------------------------------------- */
typedef struct rb_host_config {
    int32_t         backend;        /* rb_host_backend                        */
    int32_t         device;         /* playback device index; -1 the default  */
    int32_t         sample_rate;    /* 44100 or 48000 (0: 48000). On a real
                                       device the rate comes from the device
                                       (rb_host_pick_rate) and this is ignored */
    int32_t         ring_frames;    /* a power of two; 0: 8192                */
    int32_t         target_fill_ms; /* the ring's fill the render thread keeps
                                       (10..80); 0: 20                        */
    int32_t         period_ms;      /* the device's period; 0: 10             */
    int32_t         fade_ms;        /* a swap's fade out and in, 10..20; 0: 15 */
    int32_t         snapshot_hz;    /* 0: 60                                  */
    rb_host_build   initial;        /* the first engine                       */
} rb_host_config;

/* The defaults: the null backend, 48 kHz, rb_host_build_default. */
void rb_host_config_default(rb_host_config *c);

typedef struct rb_host rb_host;

/* Builds the host and its first engine; no thread runs yet. */
int  rb_host_create(const rb_host_config *config, rb_host **out);
/* Stops what runs and frees everything (NULL is fine). */
void rb_host_destroy(rb_host *h);
/* Starts the render thread and, for the null and default backends, the
 * device. rb_host_stop stops them (the engine stays as it is). */
int  rb_host_start(rb_host *h);
int  rb_host_stop(rb_host *h);
/* The audio rate the host renders at: 44100 or 48000. */
double rb_host_sample_rate(const rb_host *h);

/* ---- devices ---------------------------------------------------------- */
typedef struct rb_host_device_info {
    int32_t index;                  /* what rb_host_select_device takes       */
    int32_t is_default;
    int32_t native_rate;            /* its first native rate, 0 when any      */
    char    name[256];
} rb_host_device_info;

/* Lists the backend's playback devices: how many there are (at most `max`
 * are written). The null backend has one. */
int  rb_host_list_devices(int backend, rb_host_device_info *out, int max);
/* The rate to render at for a device's native rates (0 means any): 48000
 * when it takes 48000 or anything, else 44100 when it takes that, else 48000
 * and miniaudio converts. */
int  rb_host_pick_rate(const int32_t *native_rates, int count);
/* Moves the sound to another device (-1: the default): a new epoch, and a
 * rebuilt engine when the rate changes. The playing device is stopped first. */
int  rb_host_select_device(rb_host *h, int index);
/* The system is going to sleep, and has woken: the device stops, and
 * restarts after a flush, in a new epoch. */
int  rb_host_suspend(rb_host *h);
int  rb_host_resume(rb_host *h);

/* ---- commands --------------------------------------------------------- */
/* What a command can be. */
typedef enum rb_host_command_kind {
    RB_HOST_CMD_NOP = 0,
    RB_HOST_CMD_SET_PARAM,          /* rb_set_param                           */
    RB_HOST_CMD_SET_ENGINE_LIVE,    /* rb_set_engine, a RB_EF_LIVE field only */
    RB_HOST_CMD_DYNO_ATTACH,
    RB_HOST_CMD_DYNO_RELEASE,
    RB_HOST_CMD_DYNO_SWEEP,
    RB_HOST_CMD_DYNO_CLEAR,
    RB_HOST_CMD_SCENE_TIME,
    RB_HOST_CMD_SWAP,               /* swap in a prepared build               */
    RB_HOST_CMD_EPOCH,              /* a new clock epoch (device changes)     */
    RB_HOST_CMD_REBASE,             /* a rebuilt engine at a new rate, no fade */
    RB_HOST_CMD_COUNT
} rb_host_command_kind;

/* How the render thread answered a command. */
typedef enum rb_host_outcome {
    RB_HOST_ACK_OK = 0,
    RB_HOST_ACK_STALE,              /* for another generation: not applied    */
    RB_HOST_ACK_REFUSED,            /* the core refused it: see `code`        */
    RB_HOST_ACK_BUSY                /* a swap that couldn't be taken yet (the
                                       retired queue was full): try again     */
} rb_host_outcome;

/* The acknowledgment of one command. */
typedef struct rb_host_ack {
    uint64_t request_id;            /* the id the command was sent with       */
    int32_t  kind;                  /* rb_host_command_kind                   */
    int32_t  outcome;               /* rb_host_outcome                        */
    int32_t  code;                  /* the core's return code, when it has one */
    uint32_t flags;                 /* RB_HOST_ACK_F_*                        */
    uint64_t generation;            /* the active generation after it         */
    uint64_t epoch;
    uint64_t physics_tick;          /* the engine's tick it took effect at    */
    uint64_t audio_frame;           /* the host's audio frame it took effect at */
} rb_host_ack;
enum {
    RB_HOST_ACK_F_RESET = 1u << 0   /* the engine was replaced: a new generation */
};

/* The commands. `expected_generation` 0: any; else the command is stale
 * (acknowledged RB_HOST_ACK_STALE, not applied) unless that generation is
 * active when it reaches the front. `request_id`, if not NULL, gets the id
 * the acknowledgment will carry. A full queue refuses with RB_HOST_ERR_FULL
 * and never waits. */
int rb_host_set_param(rb_host *h, int param, double value, uint64_t expected_generation,
                      uint64_t *request_id);
int rb_host_set_engine_live(rb_host *h, int field, int index, double value,
                            uint64_t expected_generation, uint64_t *request_id);
int rb_host_dyno_attach(rb_host *h, double rpm, uint64_t expected_generation, uint64_t *request_id);
int rb_host_dyno_release(rb_host *h, uint64_t expected_generation, uint64_t *request_id);
int rb_host_dyno_sweep(rb_host *h, double to_rpm, double rate, uint64_t expected_generation,
                       uint64_t *request_id);
int rb_host_dyno_clear(rb_host *h, uint64_t expected_generation, uint64_t *request_id);
int rb_host_set_scene_time(rb_host *h, double seconds, uint64_t expected_generation,
                           uint64_t *request_id);
int rb_host_nop(rb_host *h, uint64_t expected_generation, uint64_t *request_id);

/* Takes up to `max` acknowledgments, in order. It also frees the retired
 * engines, so call it regularly. It doesn't wait, except to put the device
 * right when it was reported lost (the next device is then opened). Returns
 * how many. */
int rb_host_poll(rb_host *h, rb_host_ack *acks, int max);

/* ---- rebuild transactions --------------------------------------------- */
typedef enum rb_host_prepare_state {
    RB_HOST_PREPARE_PENDING = 0,    /* a worker is building it                */
    RB_HOST_PREPARE_READY,          /* built, waiting for a swap              */
    RB_HOST_PREPARE_FAILED,         /* it couldn't be built: the engine runs on */
    RB_HOST_PREPARE_SWAPPING        /* handed to the render thread            */
} rb_host_prepare_state;

/* Starts a worker that builds `spec` at the host's rate, off every other
 * thread. *prepare_id names it. The running engine plays on. */
int rb_host_prepare_begin(rb_host *h, const rb_host_build *spec, uint64_t *prepare_id);
/* Where it stands: the state, and for a failure the core's code and why. */
int rb_host_prepare_status(rb_host *h, uint64_t prepare_id, int *state, int *code,
                           char *why, size_t why_size);
/* Waits up to timeout_ms for it to leave PENDING; its state, or an error. */
int rb_host_prepare_wait(rb_host *h, uint64_t prepare_id, unsigned timeout_ms);
/* Throws a READY or FAILED build away. */
int rb_host_prepare_discard(rb_host *h, uint64_t prepare_id);
/* Sends "swap in build N". expected_generation 0 means the generation that
 * was active when the build began (so a swap that lost a race is stale);
 * RB_HOST_ANY_GENERATION skips the check. */
int rb_host_swap(rb_host *h, uint64_t prepare_id, uint64_t expected_generation, uint64_t *request_id);
/* The generation of the engine that is active (1 for the first engine, +1
 * for each swap; any thread may ask). */
uint64_t rb_host_generation(const rb_host *h);

/* ---- snapshots, events ------------------------------------------------ */
typedef struct rb_host_snapshot_info {
    uint64_t sequence;              /* the host's, +1 per published snapshot  */
    uint64_t generation;            /* the host's generation                  */
    uint64_t epoch;
    uint64_t audio_frame;           /* the host's audio frame it was taken at */
    uint64_t physics_tick;          /* the engine's tick                      */
    uint32_t size;                  /* bytes of the core's snapshot           */
    uint32_t reserved;
} rb_host_snapshot_info;

/* The newest snapshot nobody has read: copies the core's snapshot
 * (snapshot-v2) into buffer and fills info. Returns its size, 0 when there
 * is no new one, or an error (RB_HOST_ERR_ARG when buffer is too small). A
 * snapshot is never torn: the host drops whole ones if the reader is slow. */
int rb_host_snapshot_read(rb_host *h, void *buffer, int capacity, rb_host_snapshot_info *info);
/* How many snapshots were published, and how many were overwritten unread. */
uint64_t rb_host_snapshots_published(const rb_host *h);
uint64_t rb_host_snapshots_dropped(const rb_host *h);

/* Takes up to `max` of the engine's oldest events (RB_PARAM_EVENTS on).
 * *lost, if not NULL, gets the events that were lost since the last call:
 * the ring was full, or the core's own ring was. Returns how many. */
int rb_host_read_events(rb_host *h, rb_event *out, int max, uint64_t *lost);

/* ---- the clock -------------------------------------------------------- */
/* contracts/runtime-v1.md: an epoch maps physics ticks to audio frames,
 *   (physics_tick - physics_origin) / physics_hz = (audio_frame - audio_origin) / audio_hz,
 * and a changed engine, rate, device or sleep starts a new one. */
typedef struct rb_host_clock {
    uint64_t epoch;                 /* +1 per swap, rebase, device change     */
    uint64_t generation;
    uint64_t physics_origin;        /* the engine's tick at the epoch's start */
    uint64_t audio_origin;          /* the host's audio frame there           */
    uint64_t physics_hz, audio_hz;  /* whole numbers                          */
    uint64_t physics_tick;          /* now: the engine's tick ...             */
    uint64_t audio_frame;           /* ... at the end of the last block       */
} rb_host_clock;

int    rb_host_clock_get(const rb_host *h, rb_host_clock *out);
/* The audio frame (fractional) of a physics tick and sub-tick of the epoch. */
double rb_host_clock_audio_frame(const rb_host_clock *c, uint64_t physics_tick, double subtick);
/* 1 when the pair lies on the epoch's line, by integer cross-multiplication. */
int    rb_host_clock_consistent(const rb_host_clock *c, uint64_t physics_tick, uint64_t audio_frame);
/* When an audio frame reaches the speakers, in nanoseconds of rb_host_time_ns:
 * the last device callback's time, the frames still in the ring before it
 * and the device's own latency. RB_HOST_ERR_NOCLOCK until a device callback
 * has run in the current epoch. */
int    rb_host_output_time_ns(const rb_host *h, uint64_t audio_frame, uint64_t *time_ns);
uint64_t rb_host_time_ns(void);

/* ---- the device's side ------------------------------------------------ */
/* What the device calls to fill `frames` stereo frames (interleaved floats).
 * If the ring holds fewer, the real frames play with their last 2 ms faded
 * out and the rest is silence: an underrun, counted, never hidden by
 * repeating audio. The null and default backends call it from their device
 * threads; the manual backend's caller does. */
void rb_host_pull(rb_host *h, float *out, int frames);
/* Tells the clock the device's own latency, in frames (the device layer does
 * this itself; manual tests can set it). */
void rb_host_set_device_latency(rb_host *h, uint32_t frames);

/* ---- offline mode ----------------------------------------------------- */
/* Renders `frames` stereo frames (interleaved) on the calling thread, a
 * block at a time, exactly as the render thread would, taking its commands
 * and swaps from the same queues. OFFLINE backend only. */
int rb_host_offline_render(rb_host *h, float *out, int frames);
/* 1 if the engine now playing has the stereo scene, else 0. */
int rb_host_offline_has_scene(const rb_host *h);

/* ---- instrumentation -------------------------------------------------- */
typedef struct rb_host_priority {
    int32_t  applied;               /* the render thread asked the system for a boost */
    int32_t  boosted;               /* it got one (Windows: MMCSS "Pro Audio") */
    int32_t  os_priority;           /* what the system says its priority is    */
    uint32_t task_index;
    char     policy[64];            /* in words                                */
} rb_host_priority;

typedef struct rb_host_stats {
    uint64_t blocks;                /* blocks the render thread rendered       */
    uint64_t frames_rendered;       /* the host's audio frame                  */
    uint64_t render_ns_total, render_ns_max, render_ns_last;
    uint64_t deadline[RB_HOST_DEADLINE_BINS];
    uint64_t deadline_missed;       /* blocks that took a whole block's duration or more */
    uint32_t ring_capacity, ring_target, ring_fill_last, ring_fill_min, ring_fill_max;
    double   ring_fill_mean;
    uint64_t device_callbacks, frames_played;
    uint64_t underruns;             /* device callbacks the ring couldn't fill */
    uint64_t underrun_frames;       /* frames played as silence for lack of data */
    uint64_t commands_applied, commands_refused, commands_stale;
    uint64_t acks_pending;
    uint64_t swaps_ok, swaps_stale, swaps_busy;
    uint64_t retired_pending, engines_freed;
    uint64_t events_lost, device_events;
    uint64_t audit_alloc, audit_lock, audit_log;   /* forbidden calls made on the render thread */
    uint64_t process_bytes, process_peak_bytes;    /* the process's memory, 0 if unknown */
    rb_host_priority priority;
} rb_host_stats;

int rb_host_get_stats(const rb_host *h, rb_host_stats *out);

/* ---- for tests --------------------------------------------------------- */
/* Pretends the device reported an event (1: it stopped unplugged, 2: the
 * default changed): the next rb_host_poll recovers. */
void rb_host_debug_device_event(rb_host *h, int kind);
/* The next rb_host_select_device behaves as if the new device ran at `rate`. */
void rb_host_debug_next_device_rate(rb_host *h, int rate);
/* How many sims exist now (each prepared, active or retired engine). */
int  rb_host_debug_live_sims(void);

#ifdef __cplusplus
}
#endif

#endif /* RB_HOST_H */
