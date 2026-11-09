/*
 * runner.c - the render thread's work (stage 123): one block at a time it
 * takes the commands that are due, renders the engine, fades it if a swap is
 * under way, publishes a snapshot and the engine's events, and writes the
 * clock. The offline mode runs the same function on the caller's thread.
 *
 * What this file must never do, and the audit checks: allocate, lock, log.
 * It calls only the core's render and parameter functions, the lock-free
 * channels and atomics.
 *
 * A swap (contracts/runtime-v1.md's transaction, in a native app):
 *   1. the SWAP command reaches the front: the old engine keeps rendering
 *      and fades out over `fade_frames`, a whole number of blocks;
 *   2. at the block boundary where the fade ends, the new engine becomes
 *      the active one (generation + 1, a new epoch), the old goes on the
 *      retired queue, and the swap is acknowledged;
 *   3. the new engine fades in over the same number of frames.
 * While a swap is under way no other command is taken: the ones behind it
 * apply to the new engine, after the fade.
 */
#include <math.h>
#include <string.h>

#include "host_internal.h"

#define MAX_COMMANDS_PER_BLOCK 64

/* The engine's physics tick now, from the epoch's line. */
static uint64_t physics_tick_now(const rbh_run *r)
{
    return r->physics_origin + (r->audio_frame - r->audio_origin) * r->oversample;
}

static void add(atomic_uint_least64_t *counter, uint64_t n)
{
    atomic_fetch_add_explicit(counter, n, memory_order_relaxed);
}

/* An acknowledgment for command c; the caller pushes it (room was checked). */
static rb_host_ack make_ack(const rb_host *h, const rbh_cmd *c, int outcome, int code, uint32_t flags)
{
    const rbh_run *r = &h->run;
    rb_host_ack a;
    memset(&a, 0, sizeof a);
    a.request_id = c->request_id;
    a.kind = c->kind;
    a.outcome = outcome;
    a.code = code;
    a.flags = flags;
    a.generation = r->generation;
    a.epoch = r->epoch;
    a.physics_tick = physics_tick_now(r);
    a.audio_frame = r->audio_frame;
    return a;
}

static void send_ack(rb_host *h, const rbh_cmd *c, int outcome, int code, uint32_t flags)
{
    const rb_host_ack a = make_ack(h, c, outcome, code, flags);
    rbh_queue_push(&h->acks, &a);
    if (outcome == RB_HOST_ACK_OK) add(&h->stats.commands_applied, 1);
    else if (outcome == RB_HOST_ACK_STALE) add(&h->stats.commands_stale, 1);
    else if (outcome == RB_HOST_ACK_REFUSED) add(&h->stats.commands_refused, 1);
}

static void retire(rb_host *h, rb_sim *sim)
{
    const rbh_retired entry = { sim };
    rbh_queue_push(&h->retired, &entry);
}

/* Publishes a snapshot of the active engine into the triple buffer; returns
 * the engine's tick it was taken at. */
static uint64_t publish_snapshot(rb_host *h)
{
    rbh_run *r = &h->run;
    rbh_snap_header *slot = (rbh_snap_header *)rbh_triple_write_begin(&h->snapshots);
    uint8_t *payload = (uint8_t *)(slot + 1);
    const int size = rb_get_snapshot(r->sim, RB_SNAPSHOT_VERSION, payload, RB_HOST_SNAPSHOT_MAX_BYTES);
    if (size < (int)sizeof(rb_snapshot_header)) return physics_tick_now(r);   /* nothing to publish */
    rb_snapshot_header header;
    memcpy(&header, payload, sizeof header);
    slot->sequence = ++r->snap_sequence;
    slot->generation = r->generation;
    slot->epoch = r->epoch;
    slot->audio_frame = r->audio_frame;
    slot->physics_tick = header.physics_tick;
    slot->size = (uint32_t)size;
    slot->reserved = 0;
    rbh_triple_write_end(&h->snapshots);
    return header.physics_tick;
}

/* Makes `sim` the active engine: a new generation and epoch, the old engine
 * retired, a snapshot of the new one published at once. */
static void switch_engine(rb_host *h, rb_sim *sim, int has_scene)
{
    rbh_run *r = &h->run;
    retire(h, r->sim);
    r->sim = sim;
    r->has_scene = has_scene;
    r->generation++;
    r->epoch++;
    r->oversample = (uint32_t)rb_oversample(sim);
    r->audio_origin = r->audio_frame;
    r->snap_acc = 0;
    r->sim_events_lost_seen = rb_events_lost(sim);
    r->physics_origin = publish_snapshot(h);       /* the new engine's tick, from its snapshot */
    atomic_store_explicit(&h->active_generation, r->generation, memory_order_release);
}

void rbh_run_install(rb_host *h, rb_sim *sim, int has_scene)
{
    rbh_run *r = &h->run;
    memset(r, 0, sizeof *r);
    r->sim = sim;
    r->has_scene = has_scene;
    r->generation = 1;
    r->epoch = 1;
    r->rate = atomic_load(&h->rate);
    r->oversample = (uint32_t)rb_oversample(sim);
    r->snap_interval = (uint32_t)((double)r->rate / (double)h->config.snapshot_hz + 0.5);
    r->physics_origin = rbh_engine_tick(sim, h->scratch);
    r->audio_origin = 0;
    r->sim_events_lost_seen = rb_events_lost(sim);
    atomic_store(&h->active_generation, r->generation);
    rbh_clock_publish(h);
}

/* Applies one command at the start of a block and acknowledges it. */
static void execute(rb_host *h, const rbh_cmd *c)
{
    rbh_run *r = &h->run;
    const int stale = c->expected_generation != 0 && c->expected_generation != RB_HOST_ANY_GENERATION
                      && c->expected_generation != r->generation;
    int code = 0;
    switch (c->kind) {
    case RB_HOST_CMD_NOP:
        send_ack(h, c, stale ? RB_HOST_ACK_STALE : RB_HOST_ACK_OK, 0, 0);
        return;
    case RB_HOST_CMD_SWAP:
        /* both a good swap and a stale one put an engine on the retired queue */
        if (rbh_queue_space(&h->retired) < 1) {
            add(&h->stats.swaps_busy, 1);
            send_ack(h, c, RB_HOST_ACK_BUSY, 0, 0);
        } else if (stale) {
            retire(h, (rb_sim *)c->ptr);
            add(&h->stats.swaps_stale, 1);
            send_ack(h, c, RB_HOST_ACK_STALE, 0, 0);
        } else {
            r->tr_state = RBH_TR_OUT;       /* the ack comes when the engines change */
            r->tr_pos = 0;
            r->tr_cmd = *c;
        }
        return;
    case RB_HOST_CMD_REBASE:
        if (rbh_queue_space(&h->retired) < 1) {
            send_ack(h, c, RB_HOST_ACK_BUSY, 0, 0);
            return;
        }
        r->rate = (uint32_t)c->i1;
        atomic_store(&h->rate, (uint32_t)c->i1);
        r->snap_interval = (uint32_t)((double)r->rate / (double)h->config.snapshot_hz + 0.5);
        switch_engine(h, (rb_sim *)c->ptr, c->i0);
        send_ack(h, c, RB_HOST_ACK_OK, 0, RB_HOST_ACK_F_RESET);
        return;
    case RB_HOST_CMD_EPOCH:
        {
            const uint64_t tick = physics_tick_now(r);   /* before the origin moves */
            r->epoch++;
            r->audio_origin = r->audio_frame;
            r->physics_origin = tick;
        }
        send_ack(h, c, RB_HOST_ACK_OK, 0, 0);
        return;
    default:
        break;
    }
    if (stale) {
        send_ack(h, c, RB_HOST_ACK_STALE, 0, 0);
        return;
    }
    switch (c->kind) {
    case RB_HOST_CMD_SET_PARAM:
        code = rb_set_param(r->sim, c->i0, c->d0);
        break;
    case RB_HOST_CMD_SET_ENGINE_LIVE:
        /* a field that isn't live would rebuild the engine, which is a swap's job */
        if (!(rb_engine_field_flags(c->i0) & RB_EF_LIVE)) code = RB_ERR_VALUE;
        else code = rb_set_engine(r->sim, c->i0, c->i1, c->d0);
        break;
    case RB_HOST_CMD_DYNO_ATTACH:
        code = rb_dyno_attach(r->sim, c->d0);
        break;
    case RB_HOST_CMD_DYNO_RELEASE:
        code = rb_dyno_release(r->sim);
        break;
    case RB_HOST_CMD_DYNO_SWEEP:
        code = rb_dyno_set_sweep(r->sim, c->d0, c->d1);
        break;
    case RB_HOST_CMD_DYNO_CLEAR:
        code = rb_dyno_clear(r->sim);
        break;
    case RB_HOST_CMD_SCENE_TIME:
        code = rb_set_scene_time(r->sim, c->d0);
        break;
    default:
        code = RB_ERR_ID;
        break;
    }
    send_ack(h, c, code < 0 ? RB_HOST_ACK_REFUSED : RB_HOST_ACK_OK, code, 0);
}

/* Takes the commands that are waiting, while there is room for their acks. */
static void drain_commands(rb_host *h)
{
    rbh_run *r = &h->run;
    for (int n = 0; n < MAX_COMMANDS_PER_BLOCK && r->tr_state == RBH_TR_NONE; n++) {
        if (rbh_queue_space(&h->acks) < 2) break;      /* an ack for it, and one for a swap's end */
        rbh_cmd c;
        if (!rbh_queue_pop(&h->commands, &c)) break;
        execute(h, &c);
    }
}

/* Moves the engine's events into the event ring. */
static void drain_events(rb_host *h)
{
    rbh_run *r = &h->run;
    if (rb_get_param(r->sim, RB_PARAM_EVENTS) == 0.0) return;
    rb_event batch[32];
    int n;
    while ((n = rb_read_events(r->sim, batch, 32)) > 0)
        for (int i = 0; i < n; i++) rbh_evring_push(&h->events, &batch[i]);
    const uint64_t lost = rb_events_lost(r->sim);      /* the core's own ring overflowed */
    if (lost > r->sim_events_lost_seen) {
        rbh_evring_add_lost(&h->events, lost - r->sim_events_lost_seen);
        r->sim_events_lost_seen = lost;
    }
}

/* The fade's gain for a position in it. */
static void fade_block(float *out, int frames, const float *table, uint32_t pos, uint32_t length, float beyond)
{
    for (int j = 0; j < frames; j++) {
        const uint32_t p = pos + (uint32_t)j;
        const float g = p < length ? table[p] : beyond;
        out[2 * j] *= g;
        out[2 * j + 1] *= g;
    }
}

int rbh_deadline_bin(uint64_t ns, uint64_t budget)
{
    int bin = (int)(ns * 10u / budget);
    if (bin > 9) bin = ns < 2 * budget ? 10 : 11;
    return bin;
}

void rbh_run_block(rb_host *h, float *out, int frames)
{
    rbh_run *r = &h->run;
    const uint64_t t0 = rbh_plat_time_ns();
    const uint32_t fade = h->fade_frames;

    drain_commands(h);

    /* the engine's block, in both ears */
    if (r->has_scene) {
        if (rb_render_stereo(r->sim, out, frames) < 0) memset(out, 0, (size_t)frames * 2 * sizeof(float));
    } else {
        if (rb_render_block(r->sim, r->mono, frames) < 0) memset(r->mono, 0, sizeof r->mono);
        for (int i = 0; i < frames; i++) out[2 * i] = out[2 * i + 1] = r->mono[i];
    }

    /* a swap's fades */
    if (r->tr_state == RBH_TR_OUT) {
        fade_block(out, frames, h->fade_out, r->tr_pos, fade, 0.0f);
        r->tr_pos += (uint32_t)frames;
    } else if (r->tr_state == RBH_TR_IN) {
        fade_block(out, frames, h->fade_in, r->tr_pos, fade, 1.0f);
        r->tr_pos += (uint32_t)frames;
        if (r->tr_pos >= fade) r->tr_state = RBH_TR_NONE;
    }

    drain_events(h);
    r->audio_frame += (uint32_t)frames;
    r->snap_acc += (uint32_t)frames;
    if (r->snap_acc >= r->snap_interval) {
        r->snap_acc -= r->snap_interval;
        publish_snapshot(h);
    }

    /* the old engine has faded out: change engines at this block boundary */
    if (r->tr_state == RBH_TR_OUT && r->tr_pos >= fade) {
        const rbh_cmd c = r->tr_cmd;
        switch_engine(h, (rb_sim *)c.ptr, c.i0);
        r->tr_state = RBH_TR_IN;
        r->tr_pos = 0;
        add(&h->stats.swaps_ok, 1);
        send_ack(h, &c, RB_HOST_ACK_OK, 0, RB_HOST_ACK_F_RESET);
    }
    rbh_clock_publish(h);

    /* the block's cost against its duration: the deadline histogram */
    const uint64_t ns = rbh_plat_time_ns() - t0;
    const uint64_t budget = (uint64_t)frames * 1000000000ull / r->rate;
    add(&h->stats.deadline[rbh_deadline_bin(ns, budget)], 1);
    add(&h->stats.blocks, 1);
    add(&h->stats.render_ns_total, ns);
    atomic_store_explicit(&h->stats.render_ns_last, ns, memory_order_relaxed);
    if (ns > atomic_load_explicit(&h->stats.render_ns_max, memory_order_relaxed))
        atomic_store_explicit(&h->stats.render_ns_max, ns, memory_order_relaxed);
}

/* The render thread: keeps the ring filled to its target. */
void rbh_render_thread_main(void *arg)
{
    rb_host *h = (rb_host *)arg;
    rbh_audit_enter_render();
    rbh_priority priority;
    rbh_plat_render_enter(&priority);
    h->priority = priority;
    atomic_store(&h->priority_set, 1);
    atomic_store(&h->ready, 1);

    float block[2 * RB_HOST_BLOCK_FRAMES];
    while (!atomic_load(&h->stop)) {
        rbh_plat_sem_wait(h->wake, 4);
        for (int i = 0; i < 16 && !atomic_load(&h->stop); i++) {
            const uint32_t fill = rbh_ring_fill(&h->ring);
            const uint32_t target = h->target_frames + atomic_load_explicit(&h->prime_frames, memory_order_relaxed);
            if (fill >= target || rbh_ring_space(&h->ring) < RB_HOST_BLOCK_FRAMES) break;
            atomic_store_explicit(&h->stats.ring_fill_last, fill, memory_order_relaxed);
            if (fill < atomic_load_explicit(&h->stats.ring_fill_min, memory_order_relaxed))
                atomic_store_explicit(&h->stats.ring_fill_min, fill, memory_order_relaxed);
            if (fill > atomic_load_explicit(&h->stats.ring_fill_max, memory_order_relaxed))
                atomic_store_explicit(&h->stats.ring_fill_max, fill, memory_order_relaxed);
            add(&h->stats.ring_fill_sum, fill);
            rbh_run_block(h, block, RB_HOST_BLOCK_FRAMES);
            rbh_ring_write(&h->ring, block, RB_HOST_BLOCK_FRAMES);
        }
    }
    rbh_plat_render_leave();
    rbh_audit_leave_render();
}
