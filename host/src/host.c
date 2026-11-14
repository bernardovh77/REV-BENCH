/*
 * host.c - the host's control side and the device's pull (stage 123): making
 * and starting the host, the commands, the rebuild transactions, the readers
 * of snapshots, events and statistics, and what the device calls.
 *
 * Everything here runs on the control thread except rb_host_pull (the
 * device's) and the readers the header names as safe from any thread.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "host_internal.h"

void rb_host_config_default(rb_host_config *c)
{
    memset(c, 0, sizeof *c);
    c->backend = RB_HOST_BACKEND_NULL;
    c->device = -1;
    c->sample_rate = 48000;
    c->ring_frames = 8192;
    c->target_fill_ms = 20;
    c->period_ms = 10;
    c->fade_ms = 15;
    c->snapshot_hz = 60;
    rb_host_build_default(&c->initial);
}

static int is_pow2(uint32_t n)
{
    return n && !(n & (n - 1));
}

/* The blocks a fade of fade_ms takes, where both 44.1 and 48 kHz keep it
 * within 10..20 ms: 4 to 6 blocks of 128 frames. */
static uint32_t fade_blocks(int fade_ms, uint32_t rate)
{
    int n = (int)floor((double)fade_ms * (double)rate / 1000.0 / RB_HOST_BLOCK_FRAMES + 0.5);
    if (n < 4) n = 4;
    if (n > 6) n = 6;
    return (uint32_t)n;
}

int rb_host_create(const rb_host_config *config, rb_host **out)
{
    if (!out) return RB_HOST_ERR_ARG;
    *out = NULL;
    rb_host_config cfg;
    if (config) cfg = *config;
    else rb_host_config_default(&cfg);
    if (cfg.sample_rate == 0) cfg.sample_rate = 48000;
    if (cfg.ring_frames == 0) cfg.ring_frames = 8192;
    if (cfg.target_fill_ms == 0) cfg.target_fill_ms = 20;
    if (cfg.period_ms == 0) cfg.period_ms = 10;
    if (cfg.fade_ms == 0) cfg.fade_ms = 15;
    if (cfg.snapshot_hz == 0) cfg.snapshot_hz = 60;
    if (cfg.backend < 0 || cfg.backend >= RB_HOST_BACKEND_COUNT) return RB_HOST_ERR_ARG;
    if (cfg.sample_rate != 44100 && cfg.sample_rate != 48000) return RB_HOST_ERR_ARG;
    if (cfg.ring_frames < 1024 || cfg.ring_frames > (1 << 20) || !is_pow2((uint32_t)cfg.ring_frames)) return RB_HOST_ERR_ARG;
    if (cfg.target_fill_ms < 10 || cfg.target_fill_ms > 80) return RB_HOST_ERR_ARG;
    if (cfg.fade_ms < 10 || cfg.fade_ms > 20) return RB_HOST_ERR_ARG;
    if (cfg.snapshot_hz < 1 || cfg.snapshot_hz > 240) return RB_HOST_ERR_ARG;
    if (cfg.period_ms < 2 || cfg.period_ms > 100) return RB_HOST_ERR_ARG;

    int rate = cfg.sample_rate;
    if (cfg.backend == RB_HOST_BACKEND_DEFAULT) {      /* the real device decides */
        rate = rbh_device_rate(cfg.backend, cfg.device);
        if (rate == 0) return RB_HOST_ERR_DEVICE;
    }
    const uint32_t target = (uint32_t)((double)rate * cfg.target_fill_ms / 1000.0 + 0.5);
    if (target + 2 * RB_HOST_BLOCK_FRAMES > (uint32_t)cfg.ring_frames) return RB_HOST_ERR_ARG;

    rb_host *h = (rb_host *)rbh_alloc(sizeof *h);
    if (!h) return RB_HOST_ERR_NOMEM;
    h->config = cfg;
    h->backend = cfg.backend;
    atomic_init(&h->rate, (uint32_t)rate);
    h->ring_frames = (uint32_t)cfg.ring_frames;
    h->target_frames = target;
    h->fade_frames = fade_blocks(cfg.fade_ms, (uint32_t)rate) * RB_HOST_BLOCK_FRAMES;
    h->dev_gain = 1.0f;
    h->next_request = 0;
    atomic_init(&h->active_generation, 0);
    atomic_init(&h->stop, 0);
    atomic_init(&h->ready, 0);
    atomic_init(&h->priority_set, 0);
    atomic_init(&h->suspended, 0);
    atomic_init(&h->device_event, 0);
    atomic_init(&h->dev_latency, 0);
    atomic_init(&h->prime_frames, 0);
    atomic_store(&h->stats.ring_fill_min, UINT32_MAX);

    int bad = 0;
    bad |= rbh_ring_init(&h->ring, h->ring_frames, RB_HOST_CHANNELS);
    bad |= rbh_queue_init(&h->commands, RB_HOST_COMMAND_CAPACITY, sizeof(rbh_cmd));
    bad |= rbh_queue_init(&h->acks, RB_HOST_ACK_CAPACITY, sizeof(rb_host_ack));
    bad |= rbh_queue_init(&h->retired, RB_HOST_RETIRED_CAPACITY, sizeof(rbh_retired));
    bad |= rbh_triple_init(&h->snapshots, RBH_SNAP_SLOT_BYTES);
    bad |= rbh_evring_init(&h->events, RB_HOST_EVENT_CAPACITY);
    h->fade_out = (float *)rbh_alloc(h->fade_frames * sizeof(float));
    h->fade_in = (float *)rbh_alloc(h->fade_frames * sizeof(float));
    h->scratch = (uint8_t *)rbh_alloc(RB_HOST_SNAPSHOT_MAX_BYTES);
    h->wake = rbh_plat_sem_create();
    if (bad || !h->fade_out || !h->fade_in || !h->scratch || !h->wake) {
        rb_host_destroy(h);
        return RB_HOST_ERR_NOMEM;
    }
    /* raised-cosine fades: the out one ends at exactly 0, the in one at exactly 1 */
    for (uint32_t p = 0; p < h->fade_frames; p++) {
        const double x = 3.14159265358979323846 * (double)(p + 1) / (double)h->fade_frames;
        h->fade_out[p] = (float)(0.5 * (1.0 + cos(x)));
        h->fade_in[p] = (float)(0.5 * (1.0 - cos(x)));
    }

    char why[96];
    int code = 0;
    rb_sim *sim = rbh_build_sim(&cfg.initial, (double)rate, &code, why, sizeof why);
    if (!sim) {
        rb_host_destroy(h);
        return RB_HOST_ERR_BUILD;
    }
    h->active_spec = cfg.initial;
    rbh_run_install(h, sim, cfg.initial.scene >= 0);
    *out = h;
    return RB_HOST_OK;
}

double rb_host_sample_rate(const rb_host *h)
{
    return h ? (double)atomic_load(&h->rate) : 0.0;
}

static int open_device(rb_host *h, int index);

int rb_host_start(rb_host *h)
{
    if (!h) return RB_HOST_ERR_ARG;
    if (h->started) return RB_HOST_ERR_STATE;
    if (h->backend == RB_HOST_BACKEND_OFFLINE) {
        h->started = 1;
        return RB_HOST_OK;
    }
    atomic_store(&h->stop, 0);
    atomic_store(&h->ready, 0);
    h->render_thread = rbh_plat_thread_start(rbh_render_thread_main, h);
    if (!h->render_thread) return RB_HOST_ERR_THREAD;
    while (!atomic_load(&h->ready)) rbh_plat_sleep_ms(1);      /* its priority is set by now */
    if (h->backend == RB_HOST_BACKEND_NULL || h->backend == RB_HOST_BACKEND_DEFAULT) {
        const int rc = open_device(h, h->config.device);
        if (rc != RB_HOST_OK) {
            atomic_store(&h->stop, 1);
            rbh_plat_sem_post(h->wake);
            rbh_plat_thread_join(h->render_thread);
            h->render_thread = NULL;
            return rc;
        }
    } else {
        for (int waited = 0; waited < 1000 && rbh_ring_fill(&h->ring) < h->target_frames; waited++) rbh_plat_sleep_ms(1);
    }
    h->started = 1;
    return RB_HOST_OK;
}

int rb_host_stop(rb_host *h)
{
    if (!h) return RB_HOST_ERR_ARG;
    if (!h->started) return RB_HOST_OK;
    if (h->device) {
        rbh_device_stop(h->device);
        rbh_device_close(h->device);
        h->device = NULL;
    }
    if (h->render_thread) {
        atomic_store(&h->stop, 1);
        rbh_plat_sem_post(h->wake);
        rbh_plat_thread_join(h->render_thread);
        h->render_thread = NULL;
    }
    h->started = 0;
    return RB_HOST_OK;
}

/* Joins a prepare worker that has finished. */
static void join_worker(rbh_prep *p)
{
    if (p->thread) {
        rbh_plat_thread_join(p->thread);
        p->thread = NULL;
    }
}

static void free_prep(rbh_prep *p)
{
    join_worker(p);
    rbh_sim_destroy(p->sim);
    p->sim = NULL;
    p->used = 0;
}

/* Frees the engines the render thread has retired. */
static void reap_retired(rb_host *h)
{
    rbh_retired r;
    while (rbh_queue_pop(&h->retired, &r)) {
        rbh_sim_destroy(r.sim);
        atomic_fetch_add_explicit(&h->stats.engines_freed, 1, memory_order_relaxed);
    }
}

void rb_host_destroy(rb_host *h)
{
    if (!h) return;
    rb_host_stop(h);
    for (int i = 0; i < RB_HOST_MAX_PREPARED; i++)
        if (h->prepared[i].used) free_prep(&h->prepared[i]);
    reap_retired(h);
    /* a swap command the render thread never took still owns its engine */
    rbh_cmd c;
    while (rbh_queue_pop(&h->commands, &c))
        if ((c.kind == RB_HOST_CMD_SWAP || c.kind == RB_HOST_CMD_REBASE) && c.ptr) rbh_sim_destroy((rb_sim *)c.ptr);
    if (h->run.tr_state == RBH_TR_OUT && h->run.tr_cmd.ptr) rbh_sim_destroy((rb_sim *)h->run.tr_cmd.ptr);
    rbh_sim_destroy(h->run.sim);
    rbh_ring_free(&h->ring);
    rbh_queue_free(&h->commands);
    rbh_queue_free(&h->acks);
    rbh_queue_free(&h->retired);
    rbh_triple_free(&h->snapshots);
    rbh_evring_free(&h->events);
    rbh_free(h->fade_out);
    rbh_free(h->fade_in);
    rbh_free(h->scratch);
    rbh_plat_sem_destroy(h->wake);
    rbh_free(h);
}

/* ---- the device's side ---------------------------------------------------- */

void rb_host_pull(rb_host *h, float *out, int frames)
{
    if (!h || !out || frames <= 0) return;
    const uint64_t now = rbh_plat_time_ns();
    const uint32_t need = (uint32_t)frames;
    const uint64_t consumed = rbh_ring_consumed(&h->ring);
    const uint32_t n = rbh_ring_read(&h->ring, out, need);
    const int underrun = n < need;

    /* the clock's record of this callback: where the ring had got to, and when */
    {
        uint64_t c[RBH_CLOCK_WORDS] = { 0 };
        uint64_t epoch[RBH_CLOCK_WORDS];
        rbh_seqlock_read(&h->clock, epoch);
        c[0] = consumed;
        c[1] = now;
        c[2] = atomic_load_explicit(&h->dev_latency, memory_order_relaxed);
        c[3] = epoch[0];
        rbh_seqlock_write(&h->devclock, c);
    }

    /* A callback the ring can't fill fades the last 2 ms of what there is out and plays
     * silence for the rest; the first real frames after it fade back in. Nothing repeats. */
    const uint32_t rate = atomic_load_explicit(&h->rate, memory_order_relaxed);
    float fade = (float)((double)rate * 0.002);
    if (fade < 1.0f) fade = 1.0f;
    const float step = 1.0f / fade;
    float g = h->dev_gain;
    if (!underrun && g == 1.0f) {
        if (n > 0) {
            h->dev_held[0] = out[2 * (n - 1)];
            h->dev_held[1] = out[2 * (n - 1) + 1];
        }
    } else {
        for (uint32_t i = 0; i < need; i++) {
            const int real = i < n;
            const float target = (!underrun || (real && (float)(i + 1) + fade <= (float)n)) ? 1.0f : 0.0f;
            if (g < target) g = g + step < target ? g + step : target;
            else if (g > target) g = g - step > target ? g - step : target;
            if (real) {
                h->dev_held[0] = out[2 * i];
                h->dev_held[1] = out[2 * i + 1];
            }
            out[2 * i] = h->dev_held[0] * g;
            out[2 * i + 1] = h->dev_held[1] * g;
        }
        h->dev_gain = g;
    }

    atomic_fetch_add_explicit(&h->stats.device_callbacks, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&h->stats.frames_played, need, memory_order_relaxed);
    if (underrun) {
        atomic_fetch_add_explicit(&h->stats.underruns, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&h->stats.underrun_frames, need - n, memory_order_relaxed);
    }
    rbh_plat_sem_post(h->wake);
}

/* ---- commands ---------------------------------------------------------------- */

static int send_command(rb_host *h, rbh_cmd *c, uint64_t expected_generation, uint64_t *request_id)
{
    c->expected_generation = expected_generation;
    c->request_id = h->next_request + 1;
    if (rbh_queue_push(&h->commands, c) != 0) return RB_HOST_ERR_FULL;
    h->next_request++;
    if (request_id) *request_id = c->request_id;
    return RB_HOST_OK;
}

int rb_host_set_param(rb_host *h, int param, double value, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_SET_PARAM;
    c.i0 = param;
    c.d0 = value;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_set_engine_live(rb_host *h, int field, int index, double value, uint64_t expected_generation,
                            uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_SET_ENGINE_LIVE;
    c.i0 = field;
    c.i1 = index;
    c.d0 = value;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_dyno_attach(rb_host *h, double rpm, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_DYNO_ATTACH;
    c.d0 = rpm;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_dyno_release(rb_host *h, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_DYNO_RELEASE;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_dyno_sweep(rb_host *h, double to_rpm, double rate, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_DYNO_SWEEP;
    c.d0 = to_rpm;
    c.d1 = rate;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_dyno_clear(rb_host *h, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_DYNO_CLEAR;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_set_scene_time(rb_host *h, double seconds, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_SCENE_TIME;
    c.d0 = seconds;
    return send_command(h, &c, expected_generation, request_id);
}

int rb_host_nop(rb_host *h, uint64_t expected_generation, uint64_t *request_id)
{
    if (!h) return RB_HOST_ERR_ARG;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_NOP;
    return send_command(h, &c, expected_generation, request_id);
}

/* ---- acknowledgments --------------------------------------------------------- */

/* What a swap's acknowledgment means for the build that was swapped. */
static void note_swap_ack(rb_host *h, const rb_host_ack *a)
{
    for (int i = 0; i < RB_HOST_MAX_PREPARED; i++) {
        rbh_prep *p = &h->prepared[i];
        if (!p->used || p->swap_request != a->request_id || atomic_load(&p->state) != RB_HOST_PREPARE_SWAPPING) continue;
        if (a->outcome == RB_HOST_ACK_BUSY) {
            atomic_store(&p->state, RB_HOST_PREPARE_READY);      /* ours still: try again */
        } else {
            if (a->outcome == RB_HOST_ACK_OK) h->active_spec = p->spec;
            p->sim = NULL;                                       /* the render thread has it */
            p->used = 0;
        }
        return;
    }
}

/* Takes the acks off the queue, noting their effects, into the stash. */
static void gather_acks(rb_host *h)
{
    rb_host_ack a;
    while (h->stash_n < RB_HOST_ACK_CAPACITY && rbh_queue_pop(&h->acks, &a)) {
        if (a.kind == RB_HOST_CMD_SWAP) note_swap_ack(h, &a);
        h->stash[h->stash_n++] = a;
    }
}

/* Waits for the acknowledgment of request_id; the others stay stashed. */
static int wait_ack(rb_host *h, uint64_t request_id, unsigned timeout_ms, rb_host_ack *out)
{
    const uint64_t until = rbh_plat_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    for (;;) {
        gather_acks(h);
        for (int i = 0; i < h->stash_n; i++)
            if (h->stash[i].request_id == request_id) {
                if (out) *out = h->stash[i];
                memmove(&h->stash[i], &h->stash[i + 1], (size_t)(h->stash_n - i - 1) * sizeof h->stash[0]);
                h->stash_n--;
                return RB_HOST_OK;
            }
        if (rbh_plat_time_ns() > until) return RB_HOST_ERR_STATE;
        rbh_plat_sleep_ms(1);
    }
}

static int recover_device(rb_host *h, int event);

int rb_host_poll(rb_host *h, rb_host_ack *acks, int max)
{
    if (!h) return RB_HOST_ERR_ARG;
    const int event = atomic_exchange(&h->device_event, 0);
    if (event) recover_device(h, event);
    gather_acks(h);
    int n = 0;
    if (acks && max > 0) {
        n = h->stash_n < max ? h->stash_n : max;
        memcpy(acks, h->stash, (size_t)n * sizeof h->stash[0]);
        memmove(h->stash, h->stash + n, (size_t)(h->stash_n - n) * sizeof h->stash[0]);
        h->stash_n -= n;
    }
    reap_retired(h);
    return n;
}

/* ---- rebuild transactions ---------------------------------------------------- */

static void prepare_main(void *arg)
{
    rbh_prep *p = (rbh_prep *)arg;
    const double rate = (double)atomic_load(&p->host->rate);
    p->sim = rbh_build_sim(&p->spec, rate, &p->code, p->why, sizeof p->why);
    p->has_scene = p->spec.scene >= 0;
    atomic_store(&p->state, p->sim ? RB_HOST_PREPARE_READY : RB_HOST_PREPARE_FAILED);
}

static rbh_prep *find_prep(rb_host *h, uint64_t id)
{
    for (int i = 0; i < RB_HOST_MAX_PREPARED; i++)
        if (h->prepared[i].used && h->prepared[i].id == id) return &h->prepared[i];
    return NULL;
}

int rb_host_prepare_begin(rb_host *h, const rb_host_build *spec, uint64_t *prepare_id)
{
    if (!h || !spec) return RB_HOST_ERR_ARG;
    rbh_prep *p = NULL;
    for (int i = 0; i < RB_HOST_MAX_PREPARED && !p; i++)
        if (!h->prepared[i].used) p = &h->prepared[i];
    if (!p) return RB_HOST_ERR_FULL;
    memset(p, 0, sizeof *p);
    p->used = 1;
    p->id = ++h->next_prepare;
    p->host = h;
    p->spec = *spec;
    p->base_generation = atomic_load_explicit(&h->active_generation, memory_order_acquire);
    atomic_init(&p->state, RB_HOST_PREPARE_PENDING);
    p->thread = rbh_plat_thread_start(prepare_main, p);
    if (!p->thread) {
        p->used = 0;
        return RB_HOST_ERR_THREAD;
    }
    if (prepare_id) *prepare_id = p->id;
    return RB_HOST_OK;
}

int rb_host_prepare_status(rb_host *h, uint64_t prepare_id, int *state, int *code, char *why, size_t why_size)
{
    rbh_prep *p = h ? find_prep(h, prepare_id) : NULL;
    if (!p) return RB_HOST_ERR_ARG;
    const int s = atomic_load(&p->state);
    if (s != RB_HOST_PREPARE_PENDING) join_worker(p);
    if (state) *state = s;
    if (code) *code = (s == RB_HOST_PREPARE_FAILED) ? p->code : 0;
    if (why && why_size) {
        snprintf(why, why_size, "%s", s == RB_HOST_PREPARE_FAILED ? p->why : "");
    }
    return RB_HOST_OK;
}

int rb_host_prepare_wait(rb_host *h, uint64_t prepare_id, unsigned timeout_ms)
{
    rbh_prep *p = h ? find_prep(h, prepare_id) : NULL;
    if (!p) return RB_HOST_ERR_ARG;
    const uint64_t until = rbh_plat_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    while (atomic_load(&p->state) == RB_HOST_PREPARE_PENDING && rbh_plat_time_ns() < until) rbh_plat_sleep_ms(1);
    const int s = atomic_load(&p->state);
    if (s != RB_HOST_PREPARE_PENDING) join_worker(p);
    return s;
}

int rb_host_prepare_discard(rb_host *h, uint64_t prepare_id)
{
    rbh_prep *p = h ? find_prep(h, prepare_id) : NULL;
    if (!p) return RB_HOST_ERR_ARG;
    const int s = atomic_load(&p->state);
    if (s != RB_HOST_PREPARE_READY && s != RB_HOST_PREPARE_FAILED) return RB_HOST_ERR_STATE;
    free_prep(p);
    return RB_HOST_OK;
}

int rb_host_swap(rb_host *h, uint64_t prepare_id, uint64_t expected_generation, uint64_t *request_id)
{
    rbh_prep *p = h ? find_prep(h, prepare_id) : NULL;
    if (!p) return RB_HOST_ERR_ARG;
    join_worker(p);
    if (atomic_load(&p->state) != RB_HOST_PREPARE_READY) return RB_HOST_ERR_STATE;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_SWAP;
    c.ptr = p->sim;
    c.i0 = p->has_scene;
    if (expected_generation == 0) expected_generation = p->base_generation;
    uint64_t id = 0;
    const int rc = send_command(h, &c, expected_generation, &id);
    if (rc != RB_HOST_OK) return rc;
    p->swap_request = id;
    atomic_store(&p->state, RB_HOST_PREPARE_SWAPPING);
    if (request_id) *request_id = id;
    return RB_HOST_OK;
}

uint64_t rb_host_generation(const rb_host *h)
{
    return h ? atomic_load_explicit(&h->active_generation, memory_order_acquire) : 0;
}

/* ---- snapshots and events ------------------------------------------------------ */

int rb_host_snapshot_read(rb_host *h, void *buffer, int capacity, rb_host_snapshot_info *info)
{
    if (!h || !buffer) return RB_HOST_ERR_ARG;
    int fresh = 0;
    const rbh_snap_header *s = (const rbh_snap_header *)rbh_triple_read(&h->snapshots, &fresh);
    if (!fresh) return 0;
    if (capacity < (int)s->size) return RB_HOST_ERR_ARG;
    memcpy(buffer, s + 1, s->size);
    if (info) {
        info->sequence = s->sequence;
        info->generation = s->generation;
        info->epoch = s->epoch;
        info->audio_frame = s->audio_frame;
        info->physics_tick = s->physics_tick;
        info->size = s->size;
        info->reserved = 0;
    }
    return (int)s->size;
}

uint64_t rb_host_snapshots_published(const rb_host *h)
{
    return h ? atomic_load(&h->snapshots.published) : 0;
}

uint64_t rb_host_snapshots_dropped(const rb_host *h)
{
    return h ? atomic_load(&h->snapshots.dropped) : 0;
}

int rb_host_read_events(rb_host *h, rb_event *out, int max, uint64_t *lost)
{
    if (!h || !out || max < 0) return RB_HOST_ERR_ARG;
    return rbh_evring_read(&h->events, out, max, lost);
}

/* ---- the offline mode ---------------------------------------------------------- */

int rb_host_offline_render(rb_host *h, float *out, int frames)
{
    if (!h || !out || frames < 0) return RB_HOST_ERR_ARG;
    if (h->backend != RB_HOST_BACKEND_OFFLINE) return RB_HOST_ERR_STATE;
    for (int done = 0; done < frames;) {
        const int n = frames - done < RB_HOST_BLOCK_FRAMES ? frames - done : RB_HOST_BLOCK_FRAMES;
        rbh_run_block(h, out + 2 * done, n);
        done += n;
    }
    return frames;
}

int rb_host_offline_has_scene(const rb_host *h)
{
    return h ? h->run.has_scene : 0;
}

/* ---- statistics --------------------------------------------------------------------- */

int rb_host_get_stats(const rb_host *h, rb_host_stats *out)
{
    if (!h || !out) return RB_HOST_ERR_ARG;
    memset(out, 0, sizeof *out);
    const rbh_stats *s = &h->stats;
#define LOAD(field) atomic_load_explicit(&s->field, memory_order_relaxed)
    out->blocks = LOAD(blocks);
    out->render_ns_total = LOAD(render_ns_total);
    out->render_ns_max = LOAD(render_ns_max);
    out->render_ns_last = LOAD(render_ns_last);
    for (int i = 0; i < RB_HOST_DEADLINE_BINS; i++) out->deadline[i] = LOAD(deadline[i]);
    out->deadline_missed = out->deadline[10] + out->deadline[11];
    out->ring_capacity = h->ring_frames;
    out->ring_target = h->target_frames;
    out->ring_fill_last = (uint32_t)LOAD(ring_fill_last);
    out->ring_fill_min = (uint32_t)LOAD(ring_fill_min);
    out->ring_fill_max = (uint32_t)LOAD(ring_fill_max);
    out->ring_fill_mean = out->blocks ? (double)LOAD(ring_fill_sum) / (double)out->blocks : 0.0;
    out->device_callbacks = LOAD(device_callbacks);
    out->frames_played = LOAD(frames_played);
    out->underruns = LOAD(underruns);
    out->underrun_frames = LOAD(underrun_frames);
    out->commands_applied = LOAD(commands_applied);
    out->commands_refused = LOAD(commands_refused);
    out->commands_stale = LOAD(commands_stale);
    out->acks_pending = rbh_queue_count(&h->acks);
    out->swaps_ok = LOAD(swaps_ok);
    out->swaps_stale = LOAD(swaps_stale);
    out->swaps_busy = LOAD(swaps_busy);
    out->retired_pending = rbh_queue_count(&h->retired);
    out->engines_freed = LOAD(engines_freed);
    out->events_lost = rbh_evring_lost(&h->events);
    out->device_events = LOAD(device_events);
#undef LOAD
    out->frames_rendered = rbh_ring_written(&h->ring);
    {
        uint64_t c[RBH_CLOCK_WORDS];
        rbh_seqlock_read(&h->clock, c);
        if (h->backend == RB_HOST_BACKEND_OFFLINE) out->frames_rendered = c[7];
    }
    out->audit_alloc = rbh_audit_count(RBH_AUDIT_ALLOC);
    out->audit_lock = rbh_audit_count(RBH_AUDIT_LOCK);
    out->audit_log = rbh_audit_count(RBH_AUDIT_LOG);
    size_t now = 0, peak = 0;
    rbh_plat_process_memory(&now, &peak);
    out->process_bytes = now;
    out->process_peak_bytes = peak;
    if (atomic_load_explicit(&h->priority_set, memory_order_acquire)) {
        out->priority.applied = h->priority.applied;
        out->priority.boosted = h->priority.boosted;
        out->priority.os_priority = h->priority.os_priority;
        out->priority.task_index = h->priority.task_index;
        out->priority.fast_cores = h->priority.fast_cores;
        out->priority.all_cores = h->priority.all_cores;
        snprintf(out->priority.policy, sizeof out->priority.policy, "%s", h->priority.policy);
    }
    return RB_HOST_OK;
}

/* ---- devices, sleep and wake ------------------------------------------------------ */

void rbh_host_device_event(rb_host *h, int kind)
{
    atomic_store(&h->device_event, kind);
    atomic_fetch_add_explicit(&h->stats.device_events, 1, memory_order_relaxed);
}

void rb_host_debug_device_event(rb_host *h, int kind)
{
    if (h) rbh_host_device_event(h, kind);
}

void rb_host_debug_next_device_rate(rb_host *h, int rate)
{
    if (h) h->debug_next_rate = rate;
}

static int device_backend(const rb_host *h)
{
    return h->backend == RB_HOST_BACKEND_NULL || h->backend == RB_HOST_BACKEND_DEFAULT;
}

/* Asks the render thread for a new epoch (the audio's timing was interrupted)
 * and waits for it. The device is stopped, so the ring is flushed first for
 * the render thread to be able to run at all. */
static int new_epoch(rb_host *h)
{
    rbh_ring_flush(&h->ring);
    rbh_plat_sem_post(h->wake);
    uint64_t id = 0;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_EPOCH;
    int rc = send_command(h, &c, 0, &id);
    if (rc != RB_HOST_OK) return rc;
    for (int tries = 0; tries < 200; tries++) {
        rbh_ring_flush(&h->ring);      /* keep it flushed until the render thread has seen the command */
        rbh_plat_sem_post(h->wake);
        rb_host_ack a;
        if (wait_ack(h, id, 10, &a) == RB_HOST_OK) {
            rbh_ring_flush(&h->ring);
            return RB_HOST_OK;
        }
    }
    return RB_HOST_ERR_STATE;
}

/* The engine rebuilt at a new rate, swapped in at once (the device is stopped). */
static int rebase(rb_host *h, int rate)
{
    char why[96];
    int code = 0;
    rb_sim *sim = rbh_build_sim(&h->active_spec, (double)rate, &code, why, sizeof why);
    if (!sim) return RB_HOST_ERR_BUILD;
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_REBASE;
    c.ptr = sim;
    c.i0 = h->active_spec.scene >= 0;
    c.i1 = rate;
    uint64_t id = 0;
    int rc = send_command(h, &c, 0, &id);
    if (rc != RB_HOST_OK) {
        rbh_sim_destroy(sim);
        return rc;
    }
    for (int tries = 0; tries < 300; tries++) {
        rbh_ring_flush(&h->ring);
        rbh_plat_sem_post(h->wake);
        rb_host_ack a;
        if (wait_ack(h, id, 10, &a) == RB_HOST_OK) {
            rbh_ring_flush(&h->ring);
            return a.outcome == RB_HOST_ACK_OK ? RB_HOST_OK : RB_HOST_ERR_STATE;
        }
    }
    return RB_HOST_ERR_STATE;
}

/* Opens the device and starts it with the ring primed: the device's first callback asks for
 * its whole buffer at once (more than the ring's steady fill), so the render thread fills the
 * ring that much higher until the device is running. */
static int open_device(rb_host *h, int index)
{
    int rc = rbh_device_open(h, h->backend, index, (int)atomic_load(&h->rate), h->config.period_ms, &h->device);
    if (rc == RB_HOST_OK) {
        const uint32_t prime = atomic_load(&h->dev_latency);
        atomic_store(&h->prime_frames, prime);
        rbh_plat_sem_post(h->wake);
        for (int waited = 0; waited < 1000 && rbh_ring_fill(&h->ring) < h->target_frames + prime; waited++)
            rbh_plat_sleep_ms(1);
        rc = rbh_device_start(h->device);
        atomic_store(&h->prime_frames, 0);
    }
    if (rc != RB_HOST_OK) {
        rbh_device_close(h->device);
        h->device = NULL;
        return RB_HOST_ERR_DEVICE;
    }
    return RB_HOST_OK;
}

int rb_host_select_device(rb_host *h, int index)
{
    if (!h || !device_backend(h) || !h->started) return RB_HOST_ERR_STATE;
    /* a device that isn't there is refused before the playing one is touched */
    const int probed = rbh_device_rate(h->backend, index);
    if (probed == 0) return RB_HOST_ERR_DEVICE;
    const int current = (int)atomic_load(&h->rate);
    int wanted = h->backend == RB_HOST_BACKEND_NULL ? current : probed;
    if (h->debug_next_rate) {            /* a test's stand-in for a device that runs at another rate */
        wanted = h->debug_next_rate;
        h->debug_next_rate = 0;
    }
    if (h->device) {
        rbh_device_stop(h->device);
        rbh_device_close(h->device);
        h->device = NULL;
    }
    int rc = wanted != current ? rebase(h, wanted) : new_epoch(h);
    if (rc != RB_HOST_OK) return rc;
    h->dev_gain = 1.0f;
    rc = open_device(h, index);
    if (rc == RB_HOST_OK) h->config.device = index;
    return rc;
}

int rb_host_suspend(rb_host *h)
{
    if (!h || !h->started) return RB_HOST_ERR_STATE;
    if (atomic_load(&h->suspended)) return RB_HOST_OK;
    if (h->device) {
        rbh_device_stop(h->device);
        rbh_device_close(h->device);   /* a device may not survive sleep: it is opened afresh on wake */
        h->device = NULL;
    }
    atomic_store(&h->suspended, 1);
    return RB_HOST_OK;
}

int rb_host_resume(rb_host *h)
{
    if (!h || !h->started || !atomic_load(&h->suspended)) return RB_HOST_ERR_STATE;
    int rc = new_epoch(h);       /* the audio in the ring is from before the sleep: dropped */
    if (rc != RB_HOST_OK) return rc;
    h->dev_gain = 1.0f;
    if (device_backend(h)) {
        rc = open_device(h, h->config.device);
        if (rc != RB_HOST_OK) return rc;
    }
    atomic_store(&h->suspended, 0);
    return RB_HOST_OK;
}

/* The device stopped on its own (unplugged), or the default moved. */
static int recover_device(rb_host *h, int event)
{
    if (!h->started || !device_backend(h) || atomic_load(&h->suspended)) return RB_HOST_OK;
    if (event == 1) return rb_host_select_device(h, -1);       /* lost: the default, afresh */
    /* rerouted: miniaudio moved the stream itself; the timing was interrupted, so a new epoch
     * (the ack comes with a later poll) */
    rbh_cmd c;
    memset(&c, 0, sizeof c);
    c.kind = RB_HOST_CMD_EPOCH;
    return send_command(h, &c, 0, NULL);
}
