/*
 * clock.c - the clock mapping (stage 123, contracts/runtime-v1.md): a
 * sequence-locked record the render thread writes and any thread reads, and
 * the arithmetic from physics ticks to audio frames to the speakers.
 */
#include "host_internal.h"

void rbh_seqlock_write(rbh_seqlock *s, const uint64_t *words)
{
    const uint64_t seq = atomic_load_explicit(&s->seq, memory_order_relaxed);
    atomic_store_explicit(&s->seq, seq + 1, memory_order_relaxed);      /* odd: being written */
    atomic_thread_fence(memory_order_release);
    for (int i = 0; i < RBH_CLOCK_WORDS; i++) atomic_store_explicit(&s->word[i], words[i], memory_order_relaxed);
    atomic_store_explicit(&s->seq, seq + 2, memory_order_release);
}

void rbh_seqlock_read(const rbh_seqlock *s, uint64_t *words)
{
    for (;;) {
        const uint64_t before = atomic_load_explicit(&s->seq, memory_order_acquire);
        if (before & 1u) continue;
        for (int i = 0; i < RBH_CLOCK_WORDS; i++) words[i] = atomic_load_explicit(&s->word[i], memory_order_relaxed);
        atomic_thread_fence(memory_order_acquire);
        if (atomic_load_explicit(&s->seq, memory_order_relaxed) == before) return;
    }
}

/* The clock's words, in order. */
enum { C_EPOCH, C_GENERATION, C_PHYSICS_ORIGIN, C_AUDIO_ORIGIN, C_PHYSICS_HZ, C_AUDIO_HZ, C_PHYSICS_TICK, C_AUDIO_FRAME };

/* The device's words. */
enum { D_CONSUMED, D_TIME_NS, D_LATENCY, D_EPOCH };

void rbh_clock_publish(rb_host *h)
{
    const rbh_run *r = &h->run;
    uint64_t w[RBH_CLOCK_WORDS];
    w[C_EPOCH] = r->epoch;
    w[C_GENERATION] = r->generation;
    w[C_PHYSICS_ORIGIN] = r->physics_origin;
    w[C_AUDIO_ORIGIN] = r->audio_origin;
    w[C_PHYSICS_HZ] = (uint64_t)r->rate * r->oversample;
    w[C_AUDIO_HZ] = r->rate;
    w[C_PHYSICS_TICK] = r->physics_origin + (r->audio_frame - r->audio_origin) * r->oversample;
    w[C_AUDIO_FRAME] = r->audio_frame;
    rbh_seqlock_write(&h->clock, w);
}

int rb_host_clock_get(const rb_host *h, rb_host_clock *out)
{
    if (!h || !out) return RB_HOST_ERR_ARG;
    uint64_t w[RBH_CLOCK_WORDS];
    rbh_seqlock_read(&h->clock, w);
    out->epoch = w[C_EPOCH];
    out->generation = w[C_GENERATION];
    out->physics_origin = w[C_PHYSICS_ORIGIN];
    out->audio_origin = w[C_AUDIO_ORIGIN];
    out->physics_hz = w[C_PHYSICS_HZ];
    out->audio_hz = w[C_AUDIO_HZ];
    out->physics_tick = w[C_PHYSICS_TICK];
    out->audio_frame = w[C_AUDIO_FRAME];
    return RB_HOST_OK;
}

double rb_host_clock_audio_frame(const rb_host_clock *c, uint64_t physics_tick, double subtick)
{
    if (!c || c->physics_hz == 0) return 0.0;
    const double ticks = (double)(physics_tick - c->physics_origin) + subtick;
    return (double)c->audio_origin + ticks * (double)c->audio_hz / (double)c->physics_hz;
}

int rb_host_clock_consistent(const rb_host_clock *c, uint64_t physics_tick, uint64_t audio_frame)
{
    if (!c || physics_tick < c->physics_origin || audio_frame < c->audio_origin) return 0;
    /* (tick - tick0) / physics_hz == (frame - frame0) / audio_hz, by cross-multiplication */
    const uint64_t left = (physics_tick - c->physics_origin) * c->audio_hz;
    const uint64_t right = (audio_frame - c->audio_origin) * c->physics_hz;
    return left == right;
}

uint64_t rb_host_time_ns(void)
{
    return rbh_plat_time_ns();
}

int rb_host_output_time_ns(const rb_host *h, uint64_t audio_frame, uint64_t *time_ns)
{
    if (!h || !time_ns) return RB_HOST_ERR_ARG;
    uint64_t d[RBH_CLOCK_WORDS], c[RBH_CLOCK_WORDS];
    rbh_seqlock_read(&h->devclock, d);
    rbh_seqlock_read(&h->clock, c);
    /* the device must have called back since the epoch began: its mapping
     * from before a device change, a rate change or a sleep isn't valid */
    if (d[D_TIME_NS] == 0 || d[D_EPOCH] != c[C_EPOCH]) return RB_HOST_ERR_NOCLOCK;
    const int64_t ahead = (int64_t)(audio_frame - d[D_CONSUMED]) + (int64_t)d[D_LATENCY];
    const double rate = (double)atomic_load(&h->rate);
    const int64_t later = (int64_t)((double)ahead * 1e9 / rate);
    *time_ns = (uint64_t)((int64_t)d[D_TIME_NS] + later);
    return RB_HOST_OK;
}

void rb_host_set_device_latency(rb_host *h, uint32_t frames)
{
    if (h) atomic_store(&h->dev_latency, frames);
}
