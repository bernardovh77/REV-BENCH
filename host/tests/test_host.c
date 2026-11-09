/*
 * test_host.c - the host through its public interface (stage 123): the render
 * thread and its ring, the commands and their acknowledgments, snapshots,
 * events, the clock, underruns, the instrumentation and the audit of what the
 * render thread must never do. The tests run the host on the manual backend
 * (the test is the device) with the core's tone for a cheap steady signal
 * and a real engine where the engine matters.
 */
#include "ht.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "host_test_util.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/* ---- the render thread and the ring --------------------------------------- */

void test_host_render_thread_fills_the_ring_to_its_target(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    CHECK(h != NULL);
    if (!h) return;
    CHECK_EQ_INT(rb_host_start(h), RB_HOST_OK);
    ht_wait_filled(h, 2000);
    rb_host_stats s;
    CHECK_EQ_INT(rb_host_get_stats(h, &s), RB_HOST_OK);
    CHECK_EQ_INT(s.ring_capacity, 8192);
    CHECK_EQ_INT(s.ring_target, 960);                              /* 20 ms at 48 kHz */
    CHECK(s.ring_fill_last <= s.ring_target + RB_HOST_BLOCK_FRAMES);
    const uint32_t fill = rbh_ring_fill(&h->ring);
    CHECK(fill >= s.ring_target && fill < s.ring_target + RB_HOST_BLOCK_FRAMES);

    /* a device pulling 10 ms at a time keeps it there, with real audio in it */
    ht_capture c;
    ht_capture_init(&c, 48000);
    CHECK(ht_pump(h, 24000, &c, 5000) >= 24000);
    CHECK(ht_rms(c.left, 4800, 24000) > 0.2);                      /* the 0.5 tone */
    CHECK(ht_rms(c.left, 4800, 24000) < 0.5);
    CHECK_EQ_INT(rb_host_get_stats(h, &s), RB_HOST_OK);
    CHECK(s.blocks >= 24000 / RB_HOST_BLOCK_FRAMES);
    CHECK(s.frames_rendered >= 24000);
    ht_capture_free(&c);
    rb_host_stop(h);
    rb_host_destroy(h);
}

void test_host_render_thread_priority_is_set_and_reported(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    CHECK_EQ_INT(rb_host_start(h), RB_HOST_OK);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.priority.applied, 1);          /* the thread asked its system for what it gets */
    CHECK(strlen(s.priority.policy) > 0);
#ifdef _WIN32
    /* MMCSS "Pro Audio" where the service runs; else the highest ordinary priority, said so */
    CHECK(s.priority.boosted == 1 || s.priority.os_priority >= THREAD_PRIORITY_HIGHEST);
    if (s.priority.boosted) CHECK(strstr(s.priority.policy, "Pro Audio") != NULL);
#else
    CHECK_EQ_INT(s.priority.boosted, 0);          /* normal until stage 126, and it says so */
    CHECK(strstr(s.priority.policy, "normal") != NULL);
#endif
    rb_host_stop(h);
    rb_host_destroy(h);
}

/* ---- commands ---------------------------------------------------------------- */

void test_host_command_is_applied_and_acknowledged(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 96000);
    ht_pump(h, 4800, &c, 5000);
    CHECK(ht_rms(c.left, 2400, 4800) > 0.2);       /* sounding */

    uint64_t id = 0;
    CHECK_EQ_INT(rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 0.0, 0, &id), RB_HOST_OK);
    CHECK(id > 0);
    const int k = ht_pump_until_ack(h, id, &c, 5000);
    CHECK(k >= 0);
    if (k >= 0) {
        const rb_host_ack *a = &c.acks[k];
        CHECK_EQ_INT(a->kind, RB_HOST_CMD_SET_PARAM);
        CHECK_EQ_INT(a->outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(a->generation, 1);
        CHECK_EQ_INT(a->epoch, 1);
        CHECK_EQ_INT(a->audio_frame % RB_HOST_BLOCK_FRAMES, 0);   /* at the start of a block */
        rb_host_clock clock;
        rb_host_clock_get(h, &clock);
        CHECK(rb_host_clock_consistent(&clock, a->physics_tick, a->audio_frame));
    }
    /* the gain is down a moment later: silent */
    ht_pump(h, 9600, &c, 5000);
    CHECK(ht_rms(c.left, c.frames - 1000, c.frames) < 1e-4);
    ht_capture_free(&c);
    rb_host_destroy(h);
}

void test_host_acks_come_in_order_with_their_ids(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    uint64_t ids[100];
    for (int i = 0; i < 100; i++) CHECK_EQ_INT(rb_host_nop(h, 0, &ids[i]), RB_HOST_OK);
    for (int i = 1; i < 100; i++) CHECK_EQ_INT(ids[i], ids[i - 1] + 1);
    ht_capture c;
    ht_capture_init(&c, 4096);
    ht_pump_until_ack(h, ids[99], &c, 5000);
    CHECK_EQ_INT(c.n_acks, 100);
    for (int i = 0; i < c.n_acks && i < 100; i++) {
        CHECK_EQ_INT(c.acks[i].request_id, ids[i]);
        CHECK_EQ_INT(c.acks[i].outcome, RB_HOST_ACK_OK);
    }
    ht_capture_free(&c);
    rb_host_destroy(h);
}

void test_host_full_command_queue_refuses_and_never_waits(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;                                 /* not started: nothing takes commands */
    uint64_t id = 0, last = 0;
    for (int i = 0; i < RB_HOST_COMMAND_CAPACITY; i++) {
        CHECK_EQ_INT(rb_host_nop(h, 0, &id), RB_HOST_OK);
        last = id;
    }
    uint64_t refused_id = 12345;
    const uint64_t before = rb_host_time_ns();
    CHECK_EQ_INT(rb_host_nop(h, 0, &refused_id), RB_HOST_ERR_FULL);
    CHECK((rb_host_time_ns() - before) < 50000000ull);   /* an immediate answer */
    CHECK_EQ_INT(refused_id, 12345);                     /* no id was handed out */
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 4096);
    ht_pump_until_ack(h, last, &c, 5000);
    CHECK_EQ_INT(rb_host_nop(h, 0, &id), RB_HOST_OK);    /* room again; the next id follows */
    CHECK_EQ_INT(id, last + 1);
    ht_capture_free(&c);
    rb_host_destroy(h);
}

void test_host_refused_commands_say_why(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    uint64_t bad_param, bad_field, good_field, clamped;
    rb_host_set_param(h, 999, 1.0, 0, &bad_param);                           /* no such parameter */
    rb_host_set_engine_live(h, RB_ENG_BORE, 0, 0.09, 0, &bad_field);         /* a rebuild: a swap's job */
    rb_host_set_engine_live(h, RB_ENG_IDLE_RPM, 0, 900.0, 0, &good_field);   /* live */
    rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 100.0, 0, &clamped);          /* above its range */
    ht_capture c;
    ht_capture_init(&c, 4096);
    ht_pump_until_ack(h, clamped, &c, 5000);
    CHECK_EQ_INT(c.n_acks, 4);
    if (c.n_acks == 4) {
        CHECK_EQ_INT(c.acks[0].outcome, RB_HOST_ACK_REFUSED);
        CHECK_EQ_INT(c.acks[0].code, RB_ERR_ID);
        CHECK_EQ_INT(c.acks[1].outcome, RB_HOST_ACK_REFUSED);
        CHECK_EQ_INT(c.acks[1].code, RB_ERR_VALUE);
        CHECK_EQ_INT(c.acks[2].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[3].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[3].code, RB_CLAMPED);           /* the core's code comes through */
    }
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.commands_refused, 2);
    CHECK_EQ_INT(s.commands_applied, 2);
    ht_capture_free(&c);
    rb_host_destroy(h);
}

/* Every kind of command: the dyno's four, the live field and the scene's clock reach the engine. */
void test_host_every_command_kind_reaches_the_engine(void)
{
    rb_host_build b;
    ht_engine_build(&b, 0, 3000.0, 0.4);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    uint64_t ids[8];
    CHECK_EQ_INT(rb_host_dyno_attach(h, 3000.0, 0, &ids[0]), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_dyno_sweep(h, 4000.0, 500.0, 0, &ids[1]), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_dyno_clear(h, 0, &ids[2]), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_dyno_release(h, 0, &ids[3]), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_set_scene_time(h, -1.0, 0, &ids[4]), RB_HOST_OK);   /* no scene: refused, said so */
    CHECK_EQ_INT(rb_host_set_engine_live(h, RB_ENG_REV_LIMIT, 0, 6500.0, 0, &ids[5]), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_nop(h, 0, &ids[6]), RB_HOST_OK);
    ht_capture c;
    ht_capture_init(&c, 8192);
    ht_pump_until_ack(h, ids[6], &c, 20000);
    CHECK_EQ_INT(c.n_acks, 7);
    if (c.n_acks == 7) {
        CHECK_EQ_INT(c.acks[0].kind, RB_HOST_CMD_DYNO_ATTACH);
        CHECK_EQ_INT(c.acks[0].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[1].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[2].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[3].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[4].kind, RB_HOST_CMD_SCENE_TIME);
        CHECK_EQ_INT(c.acks[4].outcome, RB_HOST_ACK_REFUSED);
        CHECK_EQ_INT(c.acks[5].outcome, RB_HOST_ACK_OK);
    }
    ht_capture_free(&c);
    rb_host_destroy(h);
}

typedef struct sender {
    rb_host *host;
    int      total, sent;
    uint64_t last_id;
} sender;

static void send_nops(void *arg)
{
    sender *s = (sender *)arg;
    while (s->sent < s->total) {
        uint64_t id;
        if (rb_host_nop(s->host, 0, &id) == RB_HOST_OK) {
            s->last_id = id;
            s->sent++;
        } else {
            rbh_plat_sleep_ms(0);
        }
    }
}

/* Thousands of commands through a queue of 256 while the render thread runs: none lost, in order. */
void test_host_no_command_is_lost_under_load(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    sender s = { h, 5000, 0, 0 };
    rbh_thread *t = rbh_plat_thread_start(send_nops, &s);
    CHECK(t != NULL);
    uint64_t expected = 1;
    int seen = 0, out_of_order = 0;
    const uint64_t until = rb_host_time_ns() + 30000000000ull;
    while (seen < 5000 && rb_host_time_ns() < until) {
        rb_host_ack acks[64];
        const int n = rb_host_poll(h, acks, 64);
        for (int i = 0; i < n; i++) {
            if (acks[i].request_id != expected) out_of_order++;
            expected = acks[i].request_id + 1;
            seen++;
        }
        ht_pump_raw(h, RB_HOST_BLOCK_FRAMES, 20);
    }
    rbh_plat_thread_join(t);
    CHECK_EQ_INT(seen, 5000);
    CHECK_EQ_INT(out_of_order, 0);
    CHECK_EQ_INT(s.last_id, 5000);
    rb_host_destroy(h);
}

/* ---- snapshots ---------------------------------------------------------------- */

/* Pumps `ms` of audio, reading snapshots as fast as they come; returns how many were read. */
typedef struct snap_run {
    int      read, torn, backwards, bad_header;
    uint64_t last_sequence;
} snap_run;

static void read_snapshots(rb_host *h, snap_run *r)
{
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    rb_host_snapshot_info info;
    int n;
    while ((n = rb_host_snapshot_read(h, buf, sizeof buf, &info)) > 0) {
        r->read++;
        rb_snapshot_header header;
        memcpy(&header, buf, sizeof header);
        if (header.magic != RB_SNAPSHOT_MAGIC || header.version != RB_SNAPSHOT_VERSION || (int)header.size != n
            || info.size != header.size)
            r->bad_header++;
        /* one boundary: the host's frame and tick for this snapshot lie on its epoch's line, and
         * the core's own tick in the payload is the same one */
        rb_host_clock clock;
        rb_host_clock_get(h, &clock);
        if (info.epoch == clock.epoch && !rb_host_clock_consistent(&clock, info.physics_tick, info.audio_frame)) r->torn++;
        if (info.physics_tick != header.physics_tick) r->torn++;
        if (info.sequence <= r->last_sequence) r->backwards++;
        r->last_sequence = info.sequence;
    }
}

void test_host_snapshots_are_whole_and_published_about_60_times_a_second(void)
{
    rb_host_build b;
    ht_engine_build(&b, 0, 1500.0, 0.2);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    snap_run r;
    memset(&r, 0, sizeof r);
    const int seconds = 2;
    for (int block = 0; block < seconds * 48000 / RB_HOST_BLOCK_FRAMES; block++) {
        ht_pump_raw(h, RB_HOST_BLOCK_FRAMES, 20000);
        read_snapshots(h, &r);
    }
    /* 60 a second for 2 s of audio (the first second's worth, and the ring's lead, aside) */
    CHECK(r.read >= 100 && r.read <= 135);
    CHECK_EQ_INT(r.torn, 0);
    CHECK_EQ_INT(r.backwards, 0);
    CHECK_EQ_INT(r.bad_header, 0);
    CHECK(rb_host_snapshots_published(h) >= (uint64_t)r.read);
    rb_host_destroy(h);
}

void test_host_a_slow_reader_gets_the_newest_and_drops_are_counted(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_pump(h, 48000, NULL, 20000);                  /* a second, never read */
    const uint64_t published = rb_host_snapshots_published(h);
    CHECK(published >= 55);
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    rb_host_snapshot_info info;
    CHECK(rb_host_snapshot_read(h, buf, sizeof buf, &info) > 0);
    CHECK(info.sequence + 4 >= published);          /* the newest, give or take the ring's lead */
    CHECK(rb_host_snapshots_dropped(h) >= published - 6);
    CHECK_EQ_INT(rb_host_snapshot_read(h, buf, sizeof buf, &info), 0);   /* nothing new until another is published */
    char tiny[8];
    ht_pump(h, 4800, NULL, 5000);
    CHECK_EQ_INT(rb_host_snapshot_read(h, tiny, (int)sizeof tiny, &info), RB_HOST_ERR_ARG);   /* a buffer too small */
    rb_host_destroy(h);
}

/* ---- events -------------------------------------------------------------------- */

void test_host_events_arrive_in_order_and_losses_are_the_gaps(void)
{
    rb_host_build b;
    ht_engine_build(&b, 0, 3000.0, 0.5);
    rb_host_build_param(&b, RB_PARAM_EVENTS, 1);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    rb_event ev[64];
    uint64_t lost_told = 0, taken = 0, gaps = 0, last = 0;
    int first = 1, out_of_order = 0;
    /* half a second, reading often: events flow, none lost */
    for (int i = 0; i < 24; i++) {
        ht_pump(h, 1000, NULL, 20000);
        uint64_t lost = 0;
        int n;
        while ((n = rb_host_read_events(h, ev, 64, &lost)) > 0) {
            for (int k = 0; k < n; k++) {
                if (!first && ev[k].sequence <= last) out_of_order++;
                if (!first && ev[k].sequence > last + 1) gaps += ev[k].sequence - last - 1;
                last = ev[k].sequence;
                first = 0;
                taken++;
            }
            lost_told += lost;
        }
        lost_told += lost;
    }
    CHECK(taken > 50);
    CHECK_EQ_INT(out_of_order, 0);
    CHECK_EQ_INT(lost_told, 0);
    CHECK_EQ_INT(gaps, 0);
    /* then not reading for a long while: the ring fills and what it dropped is counted, and
     * the count equals the gap in the engine's own numbering */
    ht_pump(h, 48000 * 2, NULL, 60000);
    uint64_t lost = 0;
    int n;
    uint64_t gap_after = 0;
    while ((n = rb_host_read_events(h, ev, 64, &lost)) > 0) {
        for (int k = 0; k < n; k++) {
            if (ev[k].sequence > last + 1) gap_after += ev[k].sequence - last - 1;
            last = ev[k].sequence;
        }
        lost_told += lost;
    }
    lost_told += lost;
    CHECK(lost_told > 0);                              /* the ring held 1024 and more happened */
    CHECK(lost_told >= gap_after);                     /* the events dropped at the front of the gap ... */
    rb_host_destroy(h);
}

/* ---- the clock ------------------------------------------------------------------ */

void test_host_clock_maps_physics_ticks_to_audio_frames(void)
{
    rb_host_build b;
    ht_engine_build(&b, 0, 1000.0, 0.2);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_pump(h, 9600, NULL, 20000);
    rb_host_clock c;
    CHECK_EQ_INT(rb_host_clock_get(h, &c), RB_HOST_OK);
    CHECK_EQ_INT(c.epoch, 1);
    CHECK_EQ_INT(c.generation, 1);
    CHECK_EQ_INT(c.audio_hz, 48000);
    CHECK_EQ_INT(c.physics_hz, 48000 * (uint64_t)rb_oversample(h->run.sim));
    CHECK_EQ_INT(c.physics_hz % c.audio_hz, 0);
    CHECK(c.audio_frame >= 9600);
    CHECK(rb_host_clock_consistent(&c, c.physics_tick, c.audio_frame));
    CHECK(!rb_host_clock_consistent(&c, c.physics_tick + 1, c.audio_frame));      /* a tick off the line */
    CHECK(!rb_host_clock_consistent(&c, c.physics_origin - 1, c.audio_origin));   /* before the epoch */
    /* the origin maps to the origin, and an audio frame is oversample ticks long */
    CHECK_NEAR(rb_host_clock_audio_frame(&c, c.physics_origin, 0.0), (double)c.audio_origin, 1e-9);
    const uint64_t ov = c.physics_hz / c.audio_hz;
    CHECK_NEAR(rb_host_clock_audio_frame(&c, c.physics_origin + 3 * ov, 0.0), (double)c.audio_origin + 3.0, 1e-9);
    CHECK_NEAR(rb_host_clock_audio_frame(&c, c.physics_origin, 0.5), (double)c.audio_origin + 0.5 / (double)ov, 1e-12);
    /* it agrees with the engine's own tick in a snapshot */
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    rb_host_snapshot_info info;
    ht_pump(h, 4800, NULL, 5000);
    CHECK(rb_host_snapshot_read(h, buf, sizeof buf, &info) > 0);
    rb_host_clock_get(h, &c);
    CHECK(rb_host_clock_consistent(&c, info.physics_tick, info.audio_frame));
    rb_host_destroy(h);
}

void test_host_44100_renders_natively(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 44100);
    if (!h) return;
    CHECK_NEAR(rb_host_sample_rate(h), 44100.0, 0.0);
    rb_host_start(h);
    ht_pump(h, 4800, NULL, 5000);
    rb_host_clock c;
    rb_host_clock_get(h, &c);
    CHECK_EQ_INT(c.audio_hz, 44100);
    CHECK_EQ_INT(c.physics_hz % 44100, 0);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.ring_target, 882);       /* 20 ms at 44.1 kHz */
    rb_host_destroy(h);
}

void test_host_output_time_needs_a_device_callback_in_the_epoch(void)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_wait_filled(h, 2000);
    uint64_t t0, t1;
    CHECK_EQ_INT(rb_host_output_time_ns(h, 1000, &t0), RB_HOST_ERR_NOCLOCK);   /* no callback yet */
    rb_host_set_device_latency(h, 480);                                          /* 10 ms of device buffer */
    ht_pump(h, 1280, NULL, 5000);
    /* the last callback's first frame plays one device latency after the callback ran */
    {
        const uint64_t before = rb_host_time_ns();
        float block[2 * RB_HOST_BLOCK_FRAMES];
        while (rbh_ring_fill(&h->ring) < RB_HOST_BLOCK_FRAMES) rbh_plat_sleep_ms(0);
        const uint64_t first = rbh_ring_consumed(&h->ring);
        rb_host_pull(h, block, RB_HOST_BLOCK_FRAMES);
        const uint64_t after = rb_host_time_ns();
        uint64_t when = 0;
        CHECK_EQ_INT(rb_host_output_time_ns(h, first, &when), RB_HOST_OK);
        CHECK(when >= before + 10000000ull && when <= after + 10000000ull);
    }
    CHECK_EQ_INT(rb_host_output_time_ns(h, 5000, &t0), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_output_time_ns(h, 5000 + 480, &t1), RB_HOST_OK);
    CHECK_NEAR((double)(t1 - t0), 10e6, 1.0);        /* 480 frames at 48 kHz: 10 ms */
    /* a frame the ring is still holding plays later than the device's callback, by its distance */
    const uint64_t consumed = rbh_ring_consumed(&h->ring);
    uint64_t t_now, t_ahead;
    rb_host_output_time_ns(h, consumed, &t_now);
    rb_host_output_time_ns(h, consumed + 960, &t_ahead);
    CHECK_NEAR((double)(t_ahead - t_now), 20e6, 1.0);
    rb_host_destroy(h);
}

/* ---- underruns ----------------------------------------------------------------- */

/* A host whose ring holds a constant 1.0 that the test puts there itself (no render thread
 * runs), so every step in what the device plays is the device's own doing. */
static rb_host *dc_host(uint32_t frames_in_ring)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return NULL;
    float chunk[2 * 128];
    for (int i = 0; i < 256; i++) chunk[i] = 1.0f;
    for (uint32_t left = frames_in_ring; left > 0;) {
        const uint32_t n = left < 128 ? left : 128;
        rbh_ring_write(&h->ring, chunk, n);
        left -= n;
    }
    return h;
}

void test_host_underrun_fades_out_in_2ms_then_silence_then_fades_in(void)
{
    rb_host *h = dc_host(1000);
    if (!h) return;
    static float out[2 * 4000];
    float *p = out;
    /* 480 frames at a time: two full callbacks, then one the ring can't fill (40 left) */
    rb_host_pull(h, p, 480); p += 2 * 480;
    rb_host_pull(h, p, 480); p += 2 * 480;
    rb_host_pull(h, p, 480); p += 2 * 480;      /* 1000 - 960 = 40 frames of data for 480 asked */
    rb_host_pull(h, p, 480); p += 2 * 480;      /* nothing at all */
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.device_callbacks, 4);
    CHECK_EQ_INT(s.underruns, 2);
    CHECK_EQ_INT(s.underrun_frames, 440 + 480);
    const float step = 1.0f / 96.0f;          /* 2 ms at 48 kHz */
    /* 960 frames of 1.0, then the 40 real frames faded from 1 towards 0 by one step a frame */
    for (int i = 0; i < 960; i++) CHECK_EQ_INT(out[2 * i] == 1.0f, 1);
    float worst = 0.0f;
    for (int i = 1; i < 1920; i++) {
        const float d = fabsf(out[2 * i] - out[2 * (i - 1)]);
        if (d > worst) worst = d;
    }
    CHECK(worst <= step * 1.001f);            /* no step anywhere bigger than the 2 ms ramp's */
    CHECK(out[2 * 960] < 1.0f);               /* the fade started with the first frame the ring couldn't back up */
    CHECK_EQ_INT(out[2 * 1919] == 0.0f, 1);   /* ... and ends at exactly 0 */
    /* silence, not the last audio again: every frame after the fade is zero */
    for (int i = 960 + 96; i < 1920; i++) CHECK_EQ_INT(out[2 * i] == 0.0f && out[2 * i + 1] == 0.0f, 1);
    /* audio comes back: a fade in, never a step */
    float chunk[2 * 128];
    for (int i = 0; i < 256; i++) chunk[i] = 1.0f;
    rbh_ring_write(&h->ring, chunk, 128);
    rbh_ring_write(&h->ring, chunk, 128);
    rbh_ring_write(&h->ring, chunk, 128);
    rb_host_pull(h, p, 300);
    CHECK_EQ_INT(p[0] <= step * 1.001f && p[0] > 0.0f, 1);
    for (int i = 1; i < 300; i++) CHECK(fabsf(p[2 * i] - p[2 * (i - 1)]) <= step * 1.001f);
    CHECK_EQ_INT(p[2 * 299] == 1.0f, 1);      /* back to full level after 2 ms */
    rb_host_destroy(h);
}

void test_host_short_data_before_an_underrun_still_fades_to_zero(void)
{
    /* fewer frames than the fade is long: the last real sample is held and faded, not cut */
    rb_host *h = dc_host(130);
    if (!h) return;
    static float out[2 * 600];
    rb_host_pull(h, out, 100);                 /* a clean 100 */
    rb_host_pull(h, out + 200, 500);           /* 30 real frames, then nothing */
    for (int i = 1; i < 600; i++) CHECK(fabsf(out[2 * i] - out[2 * (i - 1)]) <= 1.0f / 96.0f * 1.001f);
    CHECK_EQ_INT(out[2 * 599] == 0.0f, 1);
    rb_host_destroy(h);
}

/* ---- the render thread's rules, checked ----------------------------------------- */

void test_audit_counts_forbidden_calls_only_on_the_render_thread(void)
{
    const uint64_t a0 = rbh_audit_count(RBH_AUDIT_ALLOC), l0 = rbh_audit_count(RBH_AUDIT_LOCK), g0 = rbh_audit_count(RBH_AUDIT_LOG);
    rbh_mutex *m = rbh_plat_mutex_create();
    void *p = rbh_alloc(16);
    rbh_plat_mutex_lock(m);
    rbh_plat_mutex_unlock(m);
    rbh_free(p);
    CHECK_EQ_INT(rbh_audit_count(RBH_AUDIT_ALLOC), a0);       /* an ordinary thread may */
    CHECK_EQ_INT(rbh_audit_count(RBH_AUDIT_LOCK), l0);
    rbh_audit_enter_render();
    p = rbh_alloc(16);
    rbh_free(p);
    rbh_plat_mutex_lock(m);
    rbh_plat_mutex_unlock(m);
    rbh_audit_leave_render();
    CHECK_EQ_INT(rbh_audit_count(RBH_AUDIT_ALLOC), a0 + 2);   /* the render thread may not: counted */
    CHECK_EQ_INT(rbh_audit_count(RBH_AUDIT_LOCK), l0 + 1);
    CHECK_EQ_INT(rbh_audit_count(RBH_AUDIT_LOG), g0);
    rbh_plat_mutex_destroy(m);
    rbh_audit_reset();           /* the calls this test made on purpose aren't the host's */
}

void test_render_thread_never_allocates_locks_or_logs(void)
{
    const uint64_t a0 = rbh_audit_count(RBH_AUDIT_ALLOC), l0 = rbh_audit_count(RBH_AUDIT_LOCK), g0 = rbh_audit_count(RBH_AUDIT_LOG);
    rb_host_build b;
    ht_engine_build(&b, 0, 2500.0, 0.3);
    rb_host_build_param(&b, RB_PARAM_EVENTS, 1);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 4096);
    /* commands, snapshots, events and a swap with its retirement all happen on the render thread */
    for (int i = 0; i < 20; i++) rb_host_set_param(h, RB_PARAM_THROTTLE, 0.1 + 0.03 * i, 0, NULL);
    uint64_t pid = 0, req = 0;
    rb_host_build next;
    ht_engine_build(&next, 0, 1800.0, 0.2);
    rb_host_prepare_begin(h, &next, &pid);
    CHECK_EQ_INT(rb_host_prepare_wait(h, pid, 20000), RB_HOST_PREPARE_READY);
    CHECK_EQ_INT(rb_host_swap(h, pid, 0, &req), RB_HOST_OK);
    ht_pump_until_ack(h, req, &c, 20000);
    ht_pump(h, 9600, NULL, 20000);
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    rb_host_snapshot_info info;
    rb_host_snapshot_read(h, buf, sizeof buf, &info);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK(s.blocks > 60);
    CHECK_EQ_INT(s.swaps_ok, 1);
    CHECK_EQ_INT(s.audit_alloc, a0);
    CHECK_EQ_INT(s.audit_lock, l0);
    CHECK_EQ_INT(s.audit_log, g0);
    ht_capture_free(&c);
    rb_host_destroy(h);
}

#ifdef RB_HOST_MALLOC_WRAP
extern uint64_t rbh_wrap_render_calls(void);
/* The C library's own allocator, wrapped by the linker in this build: the render thread makes no call. */
void test_render_thread_never_calls_the_c_libraries_allocator(void)
{
    const uint64_t before = rbh_wrap_render_calls();
    rb_host_build b;
    ht_engine_build(&b, 0, 2500.0, 0.3);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    for (int i = 0; i < 10; i++) rb_host_set_param(h, RB_PARAM_THROTTLE, 0.1 * i, 0, NULL);
    ht_pump(h, 19200, NULL, 20000);
    CHECK_EQ_INT(rbh_wrap_render_calls(), before);
    rb_host_destroy(h);
}
#endif

/* ---- the instrumentation ------------------------------------------------------------ */

void test_host_deadline_bins_are_tenths_then_missed(void)
{
    const uint64_t budget = 2666666;        /* a block of 128 frames at 48 kHz */
    CHECK_EQ_INT(rbh_deadline_bin(0, budget), 0);
    CHECK_EQ_INT(rbh_deadline_bin(budget / 10 - 1, budget), 0);
    CHECK_EQ_INT(rbh_deadline_bin((budget + 9) / 10, budget), 1);
    CHECK_EQ_INT(rbh_deadline_bin(budget / 2, budget), 5);
    CHECK_EQ_INT(rbh_deadline_bin(budget - 1, budget), 9);
    CHECK_EQ_INT(rbh_deadline_bin(budget, budget), 10);            /* a whole block's time: missed */
    CHECK_EQ_INT(rbh_deadline_bin(2 * budget - 1, budget), 10);
    CHECK_EQ_INT(rbh_deadline_bin(2 * budget, budget), 11);
    CHECK_EQ_INT(rbh_deadline_bin(50 * budget, budget), 11);
}

void test_host_statistics_deadline_histogram_and_ring_fill(void)
{
    rb_host_build b;
    ht_engine_build(&b, 0, 2000.0, 0.3);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_pump(h, 48000, NULL, 30000);
    rb_host_stats s;
    CHECK_EQ_INT(rb_host_get_stats(h, &s), RB_HOST_OK);
    uint64_t sum = 0;
    for (int i = 0; i < RB_HOST_DEADLINE_BINS; i++) sum += s.deadline[i];
    CHECK_EQ_INT(sum, s.blocks);                           /* every block is in exactly one bin */
    CHECK(s.blocks >= 375);
    CHECK(s.render_ns_total > 0);
    CHECK(s.render_ns_max >= s.render_ns_last);
    CHECK(s.render_ns_max * s.blocks >= s.render_ns_total);        /* the max is at least the mean */
    CHECK(s.ring_fill_min <= s.ring_fill_last || s.ring_fill_last == 0);
    CHECK(s.ring_fill_max <= s.ring_target);               /* a block is rendered only below the target */
    CHECK(s.ring_fill_mean > 0.0 && s.ring_fill_mean <= (double)s.ring_target);
    CHECK_EQ_INT(s.deadline_missed, s.deadline[10] + s.deadline[11]);
    CHECK(s.process_bytes > 0);
    CHECK_EQ_INT(s.underruns, 0);                          /* the pump never let it run dry */
    rb_host_destroy(h);
}
