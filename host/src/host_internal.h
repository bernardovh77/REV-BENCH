/*
 * host_internal.h - what the host's own files share (stage 123): the host's
 * state, split by who may touch it.
 *
 *   control thread   the caller of rb_host_*: the prepared builds, the
 *                    request numbers, the device handle
 *   render thread    `run`: the engine, the transition, the audio frame
 *                    and the block's scratch; the only thread that touches
 *                    a running engine
 *   device thread    the `dev_*` fields, in rb_host_pull
 *
 * What crosses between them is atomic, or one of the channels (ring.h,
 * queue.h, triple.h, evring.h, the clock's sequence lock).
 */
#ifndef RBH_HOST_INTERNAL_H
#define RBH_HOST_INTERNAL_H

#include <stdatomic.h>
#include <stdint.h>

#include "audit.h"
#include "evring.h"
#include "platform.h"
#include "queue.h"
#include "rb.h"
#include "rb_host.h"
#include "ring.h"
#include "triple.h"

/* ---- the records the channels carry ---------------------------------- */
typedef struct rbh_cmd {
    uint64_t request_id;
    uint64_t expected_generation;   /* 0 or RB_HOST_ANY_GENERATION: any       */
    int32_t  kind;                  /* rb_host_command_kind                   */
    int32_t  i0, i1;
    double   d0, d1;
    void    *ptr;                   /* a prepared rb_sim for SWAP and REBASE  */
} rbh_cmd;

typedef struct rbh_retired {
    rb_sim *sim;
} rbh_retired;

/* A snapshot slot: the host's header, then the core's snapshot. */
typedef struct rbh_snap_header {
    uint64_t sequence, generation, epoch, audio_frame, physics_tick;
    uint32_t size, reserved;
} rbh_snap_header;
#define RBH_SNAP_SLOT_BYTES (sizeof(rbh_snap_header) + RB_HOST_SNAPSHOT_MAX_BYTES)

/* ---- a sequence-locked record of 64-bit words, one writer ----------------
 * Every word is atomic, so a reader that races the writer is not a data race;
 * the sequence number (odd while writing) tells it to read again. */
#define RBH_CLOCK_WORDS 8
typedef struct rbh_seqlock {
    atomic_uint_least64_t seq;
    atomic_uint_least64_t word[RBH_CLOCK_WORDS];
} rbh_seqlock;
void rbh_seqlock_write(rbh_seqlock *s, const uint64_t *words);
void rbh_seqlock_read(const rbh_seqlock *s, uint64_t *words);

/* ---- a build waiting for or passing through its swap --------------------- */
struct rb_host;
typedef struct rbh_prep {
    int                 used;           /* control thread's                         */
    uint64_t            id;
    atomic_int          state;          /* rb_host_prepare_state; the worker sets it */
    rbh_thread         *thread;
    struct rb_host     *host;
    rb_host_build       spec;
    rb_sim             *sim;
    int                 has_scene;
    uint64_t            base_generation;
    uint64_t            swap_request;
    int                 code;
    char                why[96];
} rbh_prep;

/* ---- the render thread's state --------------------------------------------- */
enum { RBH_TR_NONE = 0, RBH_TR_OUT, RBH_TR_IN };

typedef struct rbh_run {
    rb_sim   *sim;
    int       has_scene;
    uint64_t  generation, epoch;
    uint64_t  audio_frame;              /* frames the host has rendered           */
    uint64_t  physics_origin, audio_origin;
    uint32_t  oversample;
    uint32_t  rate;
    uint32_t  snap_acc, snap_interval;
    uint64_t  snap_sequence;
    uint64_t  sim_events_lost_seen;
    int       tr_state;                 /* RBH_TR_*                                */
    uint32_t  tr_pos;
    rbh_cmd   tr_cmd;                   /* the swap in progress                    */
    float     mono[RB_HOST_BLOCK_FRAMES];
} rbh_run;

/* ---- the numbers the instrumentation keeps ----------------------------------- */
typedef struct rbh_stats {
    atomic_uint_least64_t blocks, render_ns_total, render_ns_max, render_ns_last;
    atomic_uint_least64_t deadline[RB_HOST_DEADLINE_BINS];
    atomic_uint_least64_t ring_fill_last, ring_fill_min, ring_fill_max, ring_fill_sum;
    atomic_uint_least64_t device_callbacks, frames_played, underruns, underrun_frames;
    atomic_uint_least64_t commands_applied, commands_refused, commands_stale;
    atomic_uint_least64_t swaps_ok, swaps_stale, swaps_busy;
    atomic_uint_least64_t engines_freed, device_events;
} rbh_stats;

struct rbh_device;

typedef struct rb_host {
    rb_host_config     config;
    int                backend;
    atomic_uint_least32_t rate;         /* 44100 or 48000                          */
    uint32_t           ring_frames, target_frames, fade_frames;

    /* the channels */
    rbh_ring           ring;            /* render -> device, stereo frames         */
    rbh_queue          commands;        /* control -> render                       */
    rbh_queue          acks;            /* render -> control                       */
    rbh_queue          retired;         /* render -> control: engines to free      */
    rbh_triple         snapshots;       /* render -> control                       */
    rbh_evring         events;          /* render -> control                       */
    rbh_seqlock        clock;           /* render's: epoch, origins, frame         */
    rbh_seqlock        devclock;        /* device's: consumed, time, latency, epoch */
    atomic_uint_least64_t active_generation;
    float             *fade_out, *fade_in;   /* fade_frames gains each             */

    /* the render thread */
    rbh_run            run;
    rbh_thread        *render_thread;
    rbh_sem           *wake;
    atomic_int         stop, ready;
    rbh_priority       priority;
    atomic_int         priority_set;
    int                started, running_offline;

    /* the device side (rb_host_pull) */
    float              dev_gain;
    float              dev_held[2];
    atomic_uint_least32_t dev_latency;
    atomic_uint_least32_t prime_frames;   /* extra ring fill while the device starts */
    atomic_int         suspended;

    rbh_stats          stats;

    /* the control side */
    rbh_prep           prepared[RB_HOST_MAX_PREPARED];
    uint64_t           next_request, next_prepare;
    rb_host_build      active_spec;
    struct rbh_device *device;
    atomic_int         device_event;    /* what the device said: 1 lost, 2 rerouted */
    uint8_t           *scratch;         /* RB_HOST_SNAPSHOT_MAX_BYTES, for the control thread */
    rb_host_ack        stash[RB_HOST_ACK_CAPACITY];   /* acks an internal wait took off the queue */
    int                stash_n;
    int                debug_next_rate;   /* a test's stand-in for the next device's rate */
} rb_host;

/* ---- build.c: engines -------------------------------------------------------- */
/* Builds spec's engine at `rate`: the sim, or NULL with the core's code and
 * a reason. Used by the first engine and the prepare workers. */
rb_sim *rbh_build_sim(const rb_host_build *spec, double rate, int *code, char *why, size_t why_size);
void    rbh_sim_destroy(rb_sim *sim);
int     rbh_live_sims(void);
/* The engine's tick (since its build), read from a snapshot into scratch
 * (RB_HOST_SNAPSHOT_MAX_BYTES bytes); 0 if it can't be read. */
uint64_t rbh_engine_tick(rb_sim *sim, void *scratch);

/* ---- runner.c: the render thread's work --------------------------------------- */
/* Renders one block (frames <= RB_HOST_BLOCK_FRAMES) of stereo: takes the
 * commands due, renders, fades, publishes, writes the clock. */
/* The deadline histogram's bin for a block that took `ns` of a `budget` ns duration: tenths
 * (0..9), then one to two budgets (10, the deadline missed), then two or more (11). */
int  rbh_deadline_bin(uint64_t ns, uint64_t budget);
void rbh_run_block(rb_host *h, float *out, int frames);
void rbh_render_thread_main(void *arg);
/* The first engine, from the control thread before the thread starts. */
void rbh_run_install(rb_host *h, rb_sim *sim, int has_scene);
void rbh_clock_publish(rb_host *h);

/* ---- device.c --------------------------------------------------------------------- */
int      rbh_device_list(int backend, rb_host_device_info *out, int max);
/* The rate to render at on a device (index -1: the default), or 0 on failure. */
int      rbh_device_rate(int backend, int index);
int      rbh_device_open(rb_host *h, int backend, int index, int rate, int period_ms, struct rbh_device **out);
int      rbh_device_start(struct rbh_device *d);
void     rbh_device_stop(struct rbh_device *d);
void     rbh_device_close(struct rbh_device *d);
/* The device layer reports what happened to the device. */
void     rbh_host_device_event(rb_host *h, int kind);

#endif /* RBH_HOST_INTERNAL_H */
