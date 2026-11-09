/*
 * test_channels.c - the host's lock-free channels (stage 123): the audio
 * ring, the record queue (commands, acks, retired engines), the snapshot
 * triple buffer and the event ring. Each has its behaviour tests and a
 * concurrent stress test; the stress tests are what ThreadSanitizer watches
 * in Linux CI.
 */
#include "ht.h"

#include "../src/evring.h"
#include "../src/platform.h"
#include "../src/queue.h"
#include "../src/ring.h"
#include "../src/triple.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

/* ---- the audio ring ------------------------------------------------- */

/* Frame k carries (k, -k) in a 24-bit window, exact in a float. */
static void frame_fill(float *out, uint64_t first, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; i++) {
        const float v = (float)((first + i) & 0xFFFFFFu);
        out[2 * i] = v;
        out[2 * i + 1] = -v;
    }
}

static int frame_ok(const float *in, uint64_t first, uint32_t frames)
{
    for (uint32_t i = 0; i < frames; i++) {
        const float v = (float)((first + i) & 0xFFFFFFu);
        if (in[2 * i] != v || in[2 * i + 1] != -v) return 0;
    }
    return 1;
}

void test_ring_fifo_wrap_and_counts(void)
{
    rbh_ring r;
    CHECK_EQ_INT(rbh_ring_init(&r, 8, 2), 0);
    CHECK_EQ_INT(rbh_ring_fill(&r), 0);
    CHECK_EQ_INT(rbh_ring_space(&r), 8);
    float a[32], b[32];
    frame_fill(a, 0, 5);
    CHECK_EQ_INT(rbh_ring_write(&r, a, 5), 5);
    CHECK_EQ_INT(rbh_ring_read(&r, b, 3), 3);
    CHECK(frame_ok(b, 0, 3));
    /* 2 frames wait; 6 more wrap round the end of the storage */
    frame_fill(a, 5, 6);
    CHECK_EQ_INT(rbh_ring_write(&r, a, 6), 6);
    CHECK_EQ_INT(rbh_ring_fill(&r), 8);
    CHECK_EQ_INT(rbh_ring_space(&r), 0);
    /* a write into a full ring writes nothing; a longer one writes what fits */
    CHECK_EQ_INT(rbh_ring_write(&r, a, 1), 0);
    float c[32];
    CHECK_EQ_INT(rbh_ring_read(&r, c, 8), 8);
    CHECK(frame_ok(c, 3, 8));
    CHECK_EQ_INT(rbh_ring_read(&r, c, 1), 0);
    frame_fill(a, 11, 10);
    CHECK_EQ_INT(rbh_ring_write(&r, a, 10), 8);
    CHECK_EQ_INT(rbh_ring_read(&r, c, 16), 8);
    CHECK(frame_ok(c, 11, 8));
    CHECK_EQ_INT(rbh_ring_written(&r), 19);
    CHECK_EQ_INT(rbh_ring_consumed(&r), 19);
    rbh_ring_free(&r);
}

void test_ring_rejects_bad_sizes_and_flushes(void)
{
    rbh_ring r;
    CHECK_EQ_INT(rbh_ring_init(&r, 12, 2), -1);   /* not a power of two */
    CHECK_EQ_INT(rbh_ring_init(&r, 0, 2), -1);
    CHECK_EQ_INT(rbh_ring_init(&r, 8, 0), -1);
    CHECK_EQ_INT(rbh_ring_init(&r, 16, 2), 0);
    float a[32];
    frame_fill(a, 0, 10);
    rbh_ring_write(&r, a, 10);
    CHECK_EQ_INT(rbh_ring_flush(&r), 10);
    CHECK_EQ_INT(rbh_ring_fill(&r), 0);
    CHECK_EQ_INT(rbh_ring_consumed(&r), 10);   /* a flushed frame still counts as consumed */
    frame_fill(a, 10, 4);
    rbh_ring_write(&r, a, 4);
    float b[8];
    CHECK_EQ_INT(rbh_ring_read(&r, b, 4), 4);
    CHECK(frame_ok(b, 10, 4));
    rbh_ring_free(&r);
}

typedef struct ring_run {
    rbh_ring *ring;
    uint64_t  total;
    int       bad;         /* the consumer's verdict */
} ring_run;

static void ring_producer(void *arg)
{
    ring_run *run = (ring_run *)arg;
    float chunk[2 * 97];
    uint64_t sent = 0;
    uint32_t pick = 7;
    while (sent < run->total) {
        pick = pick * 1664525u + 1013904223u;
        uint32_t want = 1 + (pick >> 24) % 97;
        if (want > run->total - sent) want = (uint32_t)(run->total - sent);
        frame_fill(chunk, sent, want);
        uint32_t done = 0;
        while (done < want) {
            const uint32_t n = rbh_ring_write(run->ring, chunk + 2 * done, want - done);
            done += n;
            if (n == 0) rbh_plat_sleep_ms(0);
        }
        sent += want;
    }
}

static void ring_consumer(void *arg)
{
    ring_run *run = (ring_run *)arg;
    float chunk[2 * 131];
    uint64_t got = 0;
    uint32_t pick = 99;
    while (got < run->total) {
        pick = pick * 1664525u + 1013904223u;
        uint32_t want = 1 + (pick >> 24) % 131;
        const uint32_t n = rbh_ring_read(run->ring, chunk, want);
        if (n == 0) {
            rbh_plat_sleep_ms(0);
            continue;
        }
        if (!frame_ok(chunk, got, n)) run->bad++;
        got += n;
    }
}

/* One producer thread and one consumer thread move 2 million frames in random
 * chunks through a ring far smaller than that: every frame arrives, in order,
 * whole. */
void test_ring_stress_in_order_no_loss(void)
{
    rbh_ring r;
    CHECK_EQ_INT(rbh_ring_init(&r, 256, 2), 0);
    ring_run run = { &r, 2000000, 0 };
    rbh_thread *p = rbh_plat_thread_start(ring_producer, &run);
    rbh_thread *c = rbh_plat_thread_start(ring_consumer, &run);
    CHECK(p && c);
    rbh_plat_thread_join(p);
    rbh_plat_thread_join(c);
    CHECK_EQ_INT(run.bad, 0);
    CHECK_EQ_INT(rbh_ring_written(&r), 2000000);
    CHECK_EQ_INT(rbh_ring_consumed(&r), 2000000);
    rbh_ring_free(&r);
}

/* ---- the record queue ------------------------------------------------ */

typedef struct record {
    uint64_t id;
    uint64_t check;       /* ~id: a record torn between two pushes shows here */
    double   value;
} record;

void test_queue_fifo_refuses_when_full_and_wraps(void)
{
    rbh_queue q;
    CHECK_EQ_INT(rbh_queue_init(&q, 4, sizeof(record)), 0);
    record r;
    CHECK_EQ_INT(rbh_queue_pop(&q, &r), 0);
    for (uint64_t i = 0; i < 4; i++) {
        record x = { i, ~i, (double)i };
        CHECK_EQ_INT(rbh_queue_push(&q, &x), 0);
    }
    CHECK_EQ_INT(rbh_queue_count(&q), 4);
    CHECK_EQ_INT(rbh_queue_space(&q), 0);
    record extra = { 99, ~(uint64_t)99, 99.0 };
    CHECK_EQ_INT(rbh_queue_push(&q, &extra), -1);      /* full: refused, not waited for */
    CHECK_EQ_INT(rbh_queue_peek(&q, &r), 1);
    CHECK_EQ_INT(r.id, 0);
    CHECK_EQ_INT(rbh_queue_count(&q), 4);              /* peeking takes nothing */
    /* many more than the capacity go through, each exactly once, in order */
    uint64_t next_in = 4, next_out = 0;
    for (int round = 0; round < 50; round++) {
        CHECK_EQ_INT(rbh_queue_pop(&q, &r), 1);
        CHECK_EQ_INT(r.id, next_out);
        CHECK_EQ_INT(r.check, ~next_out);
        next_out++;
        record x = { next_in, ~next_in, (double)next_in };
        CHECK_EQ_INT(rbh_queue_push(&q, &x), 0);
        next_in++;
    }
    while (rbh_queue_pop(&q, &r)) {
        CHECK_EQ_INT(r.id, next_out);
        next_out++;
    }
    CHECK_EQ_INT(next_out, next_in);
    rbh_queue_free(&q);
}

void test_queue_rejects_bad_sizes(void)
{
    rbh_queue q;
    CHECK_EQ_INT(rbh_queue_init(&q, 6, 8), -1);
    CHECK_EQ_INT(rbh_queue_init(&q, 0, 8), -1);
    CHECK_EQ_INT(rbh_queue_init(&q, 8, 0), -1);
}

typedef struct queue_run {
    rbh_queue *queue;
    uint64_t   total;
    int        bad, refused;
} queue_run;

static void queue_producer(void *arg)
{
    queue_run *run = (queue_run *)arg;
    for (uint64_t i = 0; i < run->total;) {
        record x = { i, ~i, (double)i * 0.5 };
        if (rbh_queue_push(run->queue, &x) == 0) i++;
        else {
            run->refused++;
            rbh_plat_sleep_ms(0);
        }
    }
}

static void queue_consumer(void *arg)
{
    queue_run *run = (queue_run *)arg;
    uint64_t next = 0;
    while (next < run->total) {
        record r;
        if (!rbh_queue_pop(run->queue, &r)) {
            rbh_plat_sleep_ms(0);
            continue;
        }
        if (r.id != next || r.check != ~next || r.value != (double)next * 0.5) run->bad++;
        next++;
    }
}

/* Every pushed record comes out once, in order, untorn: no command is lost. */
void test_queue_stress_no_loss_no_tear(void)
{
    rbh_queue q;
    CHECK_EQ_INT(rbh_queue_init(&q, 8, sizeof(record)), 0);
    queue_run run = { &q, 1000000, 0, 0 };
    rbh_thread *p = rbh_plat_thread_start(queue_producer, &run);
    rbh_thread *c = rbh_plat_thread_start(queue_consumer, &run);
    CHECK(p && c);
    rbh_plat_thread_join(p);
    rbh_plat_thread_join(c);
    CHECK_EQ_INT(run.bad, 0);
    CHECK_EQ_INT(rbh_queue_count(&q), 0);
    rbh_queue_free(&q);
}

/* ---- the snapshot triple buffer ------------------------------------- */

#define SLOT_WORDS 64

void test_triple_newest_wins_and_nothing_stale(void)
{
    rbh_triple t;
    CHECK_EQ_INT(rbh_triple_init(&t, SLOT_WORDS * sizeof(uint64_t)), 0);
    int fresh = 1;
    CHECK(rbh_triple_read(&t, &fresh) != NULL);
    CHECK_EQ_INT(fresh, 0);                         /* nothing published yet */
    for (uint64_t n = 1; n <= 3; n++) {             /* three published, none read */
        uint64_t *w = (uint64_t *)rbh_triple_write_begin(&t);
        for (int i = 0; i < SLOT_WORDS; i++) w[i] = n;
        rbh_triple_write_end(&t);
    }
    const uint64_t *r = (const uint64_t *)rbh_triple_read(&t, &fresh);
    CHECK_EQ_INT(fresh, 1);
    CHECK_EQ_INT(r[0], 3);                          /* the newest */
    CHECK_EQ_INT(r[SLOT_WORDS - 1], 3);
    CHECK_EQ_INT(t.dropped, 2);                     /* 1 and 2 were overwritten unread */
    r = (const uint64_t *)rbh_triple_read(&t, &fresh);
    CHECK_EQ_INT(fresh, 0);                         /* the same one again, not new */
    CHECK_EQ_INT(r[0], 3);
    uint64_t *w = (uint64_t *)rbh_triple_write_begin(&t);
    for (int i = 0; i < SLOT_WORDS; i++) w[i] = 4;
    rbh_triple_write_end(&t);
    r = (const uint64_t *)rbh_triple_read(&t, &fresh);
    CHECK_EQ_INT(fresh, 1);
    CHECK_EQ_INT(r[0], 4);
    CHECK_EQ_INT(t.published, 4);
    CHECK_EQ_INT(t.dropped, 2);
    rbh_triple_free(&t);
}

typedef struct triple_run {
    rbh_triple *triple;
    uint64_t    total;
    uint64_t    torn, backwards, seen;
    atomic_int  done;
} triple_run;

static void triple_writer(void *arg)
{
    triple_run *run = (triple_run *)arg;
    for (uint64_t n = 1; n <= run->total; n++) {
        uint64_t *w = (uint64_t *)rbh_triple_write_begin(run->triple);
        for (int i = 0; i < SLOT_WORDS; i++) w[i] = n;
        rbh_triple_write_end(run->triple);
    }
    atomic_store(&run->done, 1);
}

static void triple_reader(void *arg)
{
    triple_run *run = (triple_run *)arg;
    uint64_t last = 0;
    int finished = 0;
    while (!finished) {
        finished = atomic_load(&run->done);   /* read once more after the writer is done */
        int fresh;
        const uint64_t *r = (const uint64_t *)rbh_triple_read(run->triple, &fresh);
        if (!fresh) continue;
        run->seen++;
        for (int i = 1; i < SLOT_WORDS; i++)
            if (r[i] != r[0]) {
                run->torn++;
                break;
            }
        if (r[0] < last) run->backwards++;
        last = r[0];
    }
}

/* The writer fills its slot a word at a time while the reader reads
 * continuously: a snapshot is never seen half-written, and never goes back. */
void test_triple_stress_never_torn(void)
{
    rbh_triple t;
    CHECK_EQ_INT(rbh_triple_init(&t, SLOT_WORDS * sizeof(uint64_t)), 0);
    triple_run run;
    memset(&run, 0, sizeof run);
    run.triple = &t;
    run.total = 500000;
    rbh_thread *w = rbh_plat_thread_start(triple_writer, &run);
    rbh_thread *r = rbh_plat_thread_start(triple_reader, &run);
    CHECK(w && r);
    rbh_plat_thread_join(w);
    rbh_plat_thread_join(r);
    CHECK_EQ_INT(run.torn, 0);
    CHECK_EQ_INT(run.backwards, 0);
    CHECK(run.seen > 0);
    CHECK_EQ_INT(t.published, 500000);
    CHECK_EQ_INT(t.published, run.seen + t.dropped);   /* every one read or counted dropped */
    rbh_triple_free(&t);
}

/* ---- the event ring ------------------------------------------------- */

static rb_event make_event(uint64_t sequence)
{
    rb_event e;
    memset(&e, 0, sizeof e);
    e.generation = 1;
    e.sequence = sequence;
    e.physics_tick = sequence * 10;
    return e;
}

void test_evring_orders_and_counts_losses(void)
{
    rbh_evring e;
    CHECK_EQ_INT(rbh_evring_init(&e, 4), 0);
    for (uint64_t s = 0; s < 6; s++) {                 /* 6 into a ring of 4: 2 lost */
        const rb_event ev = make_event(s);
        CHECK_EQ_INT(rbh_evring_push(&e, &ev), s < 4 ? 1 : 0);
    }
    rb_event out[8];
    uint64_t lost = 99;
    CHECK_EQ_INT(rbh_evring_read(&e, out, 2, &lost), 2);
    CHECK_EQ_INT(out[0].sequence, 0);
    CHECK_EQ_INT(out[1].sequence, 1);
    CHECK_EQ_INT(lost, 2);
    CHECK_EQ_INT(rbh_evring_read(&e, out, 8, &lost), 2);
    CHECK_EQ_INT(out[0].sequence, 2);
    CHECK_EQ_INT(out[1].sequence, 3);
    CHECK_EQ_INT(lost, 0);                             /* told once */
    rbh_evring_add_lost(&e, 5);                        /* the core's own ring lost 5 */
    CHECK_EQ_INT(rbh_evring_read(&e, out, 8, &lost), 0);
    CHECK_EQ_INT(lost, 5);
    CHECK_EQ_INT(rbh_evring_lost(&e), 7);
    rbh_evring_free(&e);
}

typedef struct ev_run {
    rbh_evring *ring;
    uint64_t    total;
    uint64_t    taken, lost_told;
    int         bad;
    atomic_int  done;
} ev_run;

static void ev_producer(void *arg)
{
    ev_run *run = (ev_run *)arg;
    for (uint64_t s = 0; s < run->total; s++) {
        const rb_event ev = make_event(s);
        rbh_evring_push(run->ring, &ev);
        if ((s & 1023) == 0) rbh_plat_sleep_ms(0);
    }
    atomic_store(&run->done, 1);
}

static void ev_consumer(void *arg)
{
    ev_run *run = (ev_run *)arg;
    uint64_t last = 0;
    int first = 1, finished = 0;
    while (!finished) {
        finished = atomic_load(&run->done);
        rb_event out[16];
        uint64_t lost = 0;
        const int n = rbh_evring_read(run->ring, out, 16, &lost);
        run->lost_told += lost;
        for (int i = 0; i < n; i++) {
            if (!first && out[i].sequence <= last) run->bad++;
            if (out[i].physics_tick != out[i].sequence * 10) run->bad++;
            first = 0;
            last = out[i].sequence;
            run->taken++;
        }
    }
}

/* Events arrive in order, and what didn't arrive is exactly what the loss
 * count says. */
void test_evring_stress_taken_plus_lost_is_everything(void)
{
    rbh_evring e;
    CHECK_EQ_INT(rbh_evring_init(&e, 64), 0);
    ev_run run;
    memset(&run, 0, sizeof run);
    run.ring = &e;
    run.total = 300000;
    rbh_thread *p = rbh_plat_thread_start(ev_producer, &run);
    rbh_thread *c = rbh_plat_thread_start(ev_consumer, &run);
    CHECK(p && c);
    rbh_plat_thread_join(p);
    rbh_plat_thread_join(c);
    uint64_t lost = 0;
    rb_event out[16];
    int n;
    while ((n = rbh_evring_read(&e, out, 16, &lost)) > 0) run.taken += (uint64_t)n;
    run.lost_told += lost;
    CHECK_EQ_INT(run.bad, 0);
    CHECK_EQ_INT(run.taken + run.lost_told, run.total);
    rbh_evring_free(&e);
}
