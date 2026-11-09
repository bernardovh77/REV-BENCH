/*
 * host_test_util.c - see host_test_util.h.
 */
#include "host_test_util.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ht.h"

void ht_tone_build(rb_host_build *b, double hz)
{
    rb_host_build_default(b);
    b->quality = RB_QUALITY_LOW;
    b->n_params = 0;
    rb_host_build_param(b, RB_PARAM_SOURCE, RB_SOURCE_TONE);
    rb_host_build_param(b, RB_PARAM_TEST_TONE_HZ, hz);
    rb_host_build_param(b, RB_PARAM_TEST_TONE_LEVEL, 0.5);
}

void ht_engine_build(rb_host_build *b, int preset, double rpm, double throttle)
{
    rb_host_build_default(b);
    b->preset = preset;
    b->quality = RB_QUALITY_LOW;
    rb_host_build_param(b, RB_PARAM_IGNITION, 1);
    rb_host_build_param(b, RB_PARAM_HOLD, 1);
    rb_host_build_param(b, RB_PARAM_HOLD_RPM, rpm);
    rb_host_build_param(b, RB_PARAM_THROTTLE, throttle);
}

rb_host *ht_make(int backend, const rb_host_build *initial, int rate)
{
    rb_host_config c;
    rb_host_config_default(&c);
    c.backend = backend;
    c.sample_rate = rate;
    if (initial) c.initial = *initial;
    rb_host *h = NULL;
    const int rc = rb_host_create(&c, &h);
    CHECK_EQ_INT(rc, RB_HOST_OK);
    return h;
}

void ht_capture_init(ht_capture *c, size_t capacity)
{
    memset(c, 0, sizeof *c);
    c->left = (float *)calloc(capacity, sizeof(float));
    c->capacity = capacity;
}

void ht_capture_free(ht_capture *c)
{
    free(c->left);
    c->left = NULL;
}

/* One poll's acks into the capture. */
static void take_acks(rb_host *h, ht_capture *c)
{
    rb_host_ack acks[32];
    const int n = rb_host_poll(h, acks, 32);
    if (!c) return;
    for (int i = 0; i < n && c->n_acks < (int)(sizeof c->acks / sizeof c->acks[0]); i++) c->acks[c->n_acks++] = acks[i];
}

size_t ht_pump(rb_host *h, size_t frames, ht_capture *c, unsigned timeout_ms)
{
    const uint64_t until = rb_host_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    size_t done = 0;
    float block[2 * RB_HOST_BLOCK_FRAMES];
    while (done < frames) {
        if (rb_host_time_ns() > until) break;
        take_acks(h, c);
        if (rbh_ring_fill(&h->ring) < RB_HOST_BLOCK_FRAMES) {
            rbh_plat_sleep_ms(0);
            continue;
        }
        const uint64_t frame = rbh_ring_consumed(&h->ring);
        rb_host_pull(h, block, RB_HOST_BLOCK_FRAMES);
        if (c && c->frames + RB_HOST_BLOCK_FRAMES <= c->capacity) {
            if (c->frames == 0) c->first_frame = frame;
            for (int i = 0; i < RB_HOST_BLOCK_FRAMES; i++) c->left[c->frames + (size_t)i] = block[2 * i];
            c->frames += RB_HOST_BLOCK_FRAMES;
        }
        done += RB_HOST_BLOCK_FRAMES;
    }
    take_acks(h, c);
    return done;
}

size_t ht_pump_raw(rb_host *h, size_t frames, unsigned timeout_ms)
{
    const uint64_t until = rb_host_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    size_t done = 0;
    float block[2 * RB_HOST_BLOCK_FRAMES];
    while (done < frames && rb_host_time_ns() < until) {
        if (rbh_ring_fill(&h->ring) < RB_HOST_BLOCK_FRAMES) {
            rbh_plat_sleep_ms(0);
            continue;
        }
        rb_host_pull(h, block, RB_HOST_BLOCK_FRAMES);
        done += RB_HOST_BLOCK_FRAMES;
    }
    return done;
}

int ht_pump_until_ack(rb_host *h, uint64_t request_id, ht_capture *c, unsigned timeout_ms)
{
    const uint64_t until = rb_host_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    while (rb_host_time_ns() < until) {
        for (int i = 0; i < c->n_acks; i++)
            if (c->acks[i].request_id == request_id) return i;
        ht_pump(h, RB_HOST_BLOCK_FRAMES, c, 50);
    }
    for (int i = 0; i < c->n_acks; i++)
        if (c->acks[i].request_id == request_id) return i;
    return -1;
}

void ht_wait_filled(rb_host *h, unsigned timeout_ms)
{
    const uint64_t until = rb_host_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    while (rbh_ring_fill(&h->ring) < h->target_frames && rb_host_time_ns() < until) rbh_plat_sleep_ms(1);
}

double ht_rms(const float *left, size_t from, size_t to)
{
    double sum = 0.0;
    for (size_t i = from; i < to; i++) sum += (double)left[i] * left[i];
    return to > from ? sqrt(sum / (double)(to - from)) : 0.0;
}

double ht_max_step(const float *left, size_t from, size_t to)
{
    double worst = 0.0;
    for (size_t i = from + 1; i < to; i++) {
        const double step = fabs((double)left[i] - left[i - 1]);
        if (step > worst) worst = step;
    }
    return worst;
}
