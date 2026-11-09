/*
 * test_swap.c - the rebuild transactions (stage 123, contracts/runtime-v1.md):
 * prepare off the render thread, swap at a block boundary with a fade out and
 * in, acknowledge the new generation, free the old engine off the thread. The
 * tests cover success, failure, a stale generation, rapid successive swaps,
 * a full retired queue, the commands behind a swap, and the fades measured
 * for clicks. The signal is the core's test tone, steady and loud, so any
 * step in the output is the swap's doing.
 */
#include "ht.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_test_util.h"

/* Prepares `spec`, swaps it in, and pumps until the swap is acknowledged. The
 * ack's index in c->acks, or -1. */
static int swap_in(rb_host *h, const rb_host_build *spec, ht_capture *c, uint64_t expected_generation)
{
    uint64_t pid = 0, req = 0;
    if (rb_host_prepare_begin(h, spec, &pid) != RB_HOST_OK) return -1;
    if (rb_host_prepare_wait(h, pid, 20000) != RB_HOST_PREPARE_READY) return -1;
    if (rb_host_swap(h, pid, expected_generation, &req) != RB_HOST_OK) return -1;
    return ht_pump_until_ack(h, req, c, 20000);
}

/* The fade's length in frames, as this host has it. */
static uint32_t fade_of(const rb_host *h)
{
    return h->fade_frames;
}

/* ---- success ---------------------------------------------------------------- */

void test_swap_installs_the_new_engine_and_frees_the_old_one(void)
{
    const int baseline = rb_host_debug_live_sims();
    rb_host_build a, b;
    ht_tone_build(&a, 220.0);
    ht_tone_build(&b, 330.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 48000);
    ht_pump(h, 4800, &c, 5000);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 1);
    CHECK_EQ_INT(rb_host_generation(h), 1);

    const int k = swap_in(h, &b, &c, 0);
    CHECK(k >= 0);
    if (k >= 0) {
        const rb_host_ack *ack = &c.acks[k];
        CHECK_EQ_INT(ack->kind, RB_HOST_CMD_SWAP);
        CHECK_EQ_INT(ack->outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(ack->generation, 2);                     /* advanced exactly once */
        CHECK_EQ_INT(ack->epoch, 2);                          /* in a new epoch */
        CHECK_EQ_INT(ack->flags & RB_HOST_ACK_F_RESET, RB_HOST_ACK_F_RESET);
        CHECK_EQ_INT(ack->audio_frame % RB_HOST_BLOCK_FRAMES, 0);   /* at a block boundary */
        CHECK_EQ_INT(ack->physics_tick, 0);                         /* the new engine is cold: its own tick 0 */
    }
    CHECK_EQ_INT(rb_host_generation(h), 2);
    rb_host_clock clock;
    rb_host_clock_get(h, &clock);
    CHECK_EQ_INT(clock.generation, 2);
    CHECK_EQ_INT(clock.epoch, 2);
    CHECK(clock.audio_origin > 0);                            /* the audio clock didn't restart */
    CHECK(clock.audio_origin >= c.first_frame + 4800);
    CHECK_EQ_INT(clock.physics_origin, 0);                    /* the engine's did */
    CHECK(rb_host_clock_consistent(&clock, clock.physics_tick, clock.audio_frame));

    /* the old engine was retired by the render thread and is freed by the control thread's poll */
    rb_host_poll(h, NULL, 0);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 1);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.swaps_ok, 1);
    CHECK_EQ_INT(s.engines_freed, 1);
    CHECK_EQ_INT(s.retired_pending, 0);

    /* the sound is the new tone's and the snapshots are the new generation's */
    ht_pump(h, 9600, &c, 5000);
    CHECK(ht_rms(c.left, c.frames - 4800, c.frames) > 0.2);
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    rb_host_snapshot_info info;
    int fresh_generation = 0;
    while (rb_host_snapshot_read(h, buf, sizeof buf, &info) > 0) fresh_generation = (int)info.generation;
    CHECK_EQ_INT(fresh_generation, 2);
    ht_capture_free(&c);
    rb_host_destroy(h);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline);        /* nothing leaked */
}

/* The click measure around one swap: the largest step in the fade window against the larger of
 * the two tones' own steady steps, and the level at the moment of the switch. */
typedef struct click {
    double ratio;          /* the swap's largest step over the tones' own */
    double rms_before;     /* the last 32 frames before the switch, over the tone's steady rms */
    double rms_after;      /* the first 32 frames after it */
} click;

static click measure_click(const ht_capture *c, uint64_t switch_frame, uint32_t fade)
{
    click k = { 0, 1, 1 };
    const size_t sw = (size_t)(switch_frame - c->first_frame);
    if (sw < fade + 2400 || sw + fade + 2400 > c->frames) return k;
    const double step_a = ht_max_step(c->left, sw - fade - 2400, sw - fade - 100);
    const double step_b = ht_max_step(c->left, sw + fade + 100, sw + fade + 2400);
    const double ref = step_a > step_b ? step_a : step_b;
    k.ratio = ht_max_step(c->left, sw - fade - 1, sw + fade + 1) / ref;
    const double rms_a = ht_rms(c->left, sw - fade - 2400, sw - fade - 100);
    const double rms_b = ht_rms(c->left, sw + fade + 100, sw + fade + 2400);
    k.rms_before = ht_rms(c->left, sw - 32, sw) / rms_a;
    k.rms_after = ht_rms(c->left, sw, sw + 32) / rms_b;
    return k;
}

/* Twelve swaps between two tones, at whatever phases they fall: none steps more than the tones'
 * own steady steps (a click), and each passes through near-silence at the switch. */
void test_swap_fades_without_a_click(void)
{
    rb_host_build a, b;
    ht_tone_build(&a, 220.0);
    ht_tone_build(&b, 330.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    rb_host_start(h);
    const uint32_t fade = fade_of(h);
    CHECK(fade * 1000.0 / 48000.0 >= 10.0 && fade * 1000.0 / 48000.0 <= 20.0);   /* 10 to 20 ms */
    CHECK_EQ_INT(fade % RB_HOST_BLOCK_FRAMES, 0);
    double worst_ratio = 0.0, worst_before = 0.0, worst_after = 0.0;
    int swaps = 0;
    for (int i = 0; i < 12; i++) {
        ht_capture c;
        ht_capture_init(&c, 16000);
        ht_pump(h, 3000 + (size_t)(i * 37) % 200, &c, 5000);   /* a different phase each time */
        const int k = swap_in(h, (i & 1) ? &a : &b, &c, 0);
        CHECK(k >= 0);
        if (k < 0) {
            ht_capture_free(&c);
            break;
        }
        ht_pump(h, 6000, &c, 5000);
        const click m = measure_click(&c, c.acks[k].audio_frame, fade);
        if (m.ratio > worst_ratio) worst_ratio = m.ratio;
        if (m.rms_before > worst_before) worst_before = m.rms_before;
        if (m.rms_after > worst_after) worst_after = m.rms_after;
        swaps++;
        ht_capture_free(&c);
    }
    CHECK_EQ_INT(swaps, 12);
    printf("    fade %.1f ms; largest step in a swap %.3f of the tones' own; level at the switch %.4f / %.4f\n",
           1000.0 * fade / 48000.0, worst_ratio, worst_before, worst_after);
    CHECK(worst_ratio < 1.25);        /* a hard switch between these tones steps 5 to 30 times their own */
    CHECK(worst_before < 0.03);       /* faded out when the engines change ... */
    CHECK(worst_after < 0.03);        /* ... and the new one starts from silence */
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.swaps_ok, 12);
    rb_host_destroy(h);
}

/* ---- failure ------------------------------------------------------------------- */

void test_swap_failed_preparation_leaves_the_running_engine_untouched(void)
{
    const int baseline = rb_host_debug_live_sims();
    rb_host_build a;
    ht_tone_build(&a, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 48000);
    ht_pump(h, 9600, &c, 5000);
    const double before = ht_rms(c.left, 4800, 9600);

    rb_host_build bad[3];
    ht_tone_build(&bad[0], 220.0);
    bad[0].preset = 999;                                      /* no such preset */
    ht_tone_build(&bad[1], 220.0);
    bad[1].quality = 99;                                      /* no such quality */
    ht_tone_build(&bad[2], 220.0);
    rb_host_build_edit(&bad[2], RB_ENG_BORE, 0, NAN);         /* an impossible engine */
    for (int i = 0; i < 3; i++) {
        uint64_t pid = 0;
        CHECK_EQ_INT(rb_host_prepare_begin(h, &bad[i], &pid), RB_HOST_OK);
        CHECK_EQ_INT(rb_host_prepare_wait(h, pid, 20000), RB_HOST_PREPARE_FAILED);
        int state = -1, code = 0;
        char why[96];
        CHECK_EQ_INT(rb_host_prepare_status(h, pid, &state, &code, why, sizeof why), RB_HOST_OK);
        CHECK_EQ_INT(state, RB_HOST_PREPARE_FAILED);
        CHECK(code < 0);                                      /* the core's reason ... */
        CHECK(strlen(why) > 0);                               /* ... and in words */
        CHECK_EQ_INT(rb_host_swap(h, pid, 0, NULL), RB_HOST_ERR_STATE);    /* nothing to swap */
        CHECK_EQ_INT(rb_host_prepare_discard(h, pid), RB_HOST_OK);
    }
    ht_pump(h, 9600, &c, 5000);
    /* the engine never noticed: the same generation, no ack, the same sound */
    CHECK_EQ_INT(rb_host_generation(h), 1);
    CHECK_EQ_INT(c.n_acks, 0);
    CHECK_NEAR(ht_rms(c.left, c.frames - 4800, c.frames), before, before * 0.05);
    CHECK(ht_max_step(c.left, 0, c.frames) < 0.05);           /* no click either */
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 1);    /* the failed builds were freed */
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.swaps_ok, 0);
    ht_capture_free(&c);
    rb_host_destroy(h);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline);
}

void test_swap_prepare_table_is_bounded_and_unknown_ids_are_refused(void)
{
    rb_host_build a;
    ht_tone_build(&a, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    uint64_t ids[RB_HOST_MAX_PREPARED + 1];
    int made = 0;
    for (int i = 0; i < RB_HOST_MAX_PREPARED; i++) {
        CHECK_EQ_INT(rb_host_prepare_begin(h, &a, &ids[i]), RB_HOST_OK);
        made++;
    }
    CHECK_EQ_INT(rb_host_prepare_begin(h, &a, &ids[RB_HOST_MAX_PREPARED]), RB_HOST_ERR_FULL);
    CHECK_EQ_INT(rb_host_prepare_wait(h, 9999, 10), RB_HOST_ERR_ARG);
    CHECK_EQ_INT(rb_host_swap(h, 9999, 0, NULL), RB_HOST_ERR_ARG);
    CHECK_EQ_INT(rb_host_prepare_discard(h, 9999), RB_HOST_ERR_ARG);
    for (int i = 0; i < made; i++) {
        CHECK_EQ_INT(rb_host_prepare_wait(h, ids[i], 20000), RB_HOST_PREPARE_READY);
        CHECK_EQ_INT(rb_host_prepare_discard(h, ids[i]), RB_HOST_OK);
    }
    CHECK_EQ_INT(rb_host_prepare_begin(h, &a, &ids[0]), RB_HOST_OK);   /* room again */
    rb_host_destroy(h);       /* destroying with a build still being made or ready frees it */
}

/* ---- a stale generation ------------------------------------------------------------ */

void test_swap_stale_generation_is_refused_and_its_engine_freed(void)
{
    const int baseline = rb_host_debug_live_sims();
    rb_host_build a, b, c2;
    ht_tone_build(&a, 220.0);
    ht_tone_build(&b, 330.0);
    ht_tone_build(&c2, 440.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 48000);
    ht_pump(h, 4800, &c, 5000);
    /* two builds prepared against generation 1; the first swaps in, so the second is stale */
    uint64_t p1 = 0, p2 = 0, r1 = 0, r2 = 0;
    rb_host_prepare_begin(h, &b, &p1);
    rb_host_prepare_begin(h, &c2, &p2);
    CHECK_EQ_INT(rb_host_prepare_wait(h, p1, 20000), RB_HOST_PREPARE_READY);
    CHECK_EQ_INT(rb_host_prepare_wait(h, p2, 20000), RB_HOST_PREPARE_READY);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 3);
    CHECK_EQ_INT(rb_host_swap(h, p1, 0, &r1), RB_HOST_OK);
    const int k1 = ht_pump_until_ack(h, r1, &c, 20000);
    CHECK(k1 >= 0 && c.acks[k1].outcome == RB_HOST_ACK_OK);
    CHECK_EQ_INT(rb_host_swap(h, p2, 0, &r2), RB_HOST_OK);   /* expects generation 1, which is gone */
    const int k2 = ht_pump_until_ack(h, r2, &c, 20000);
    CHECK(k2 >= 0);
    if (k2 >= 0) {
        CHECK_EQ_INT(c.acks[k2].outcome, RB_HOST_ACK_STALE);
        CHECK_EQ_INT(c.acks[k2].generation, 2);               /* the active one didn't change */
        CHECK_EQ_INT(c.acks[k2].flags & RB_HOST_ACK_F_RESET, 0);
    }
    CHECK_EQ_INT(rb_host_generation(h), 2);
    rb_host_poll(h, NULL, 0);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 1);    /* the stale build and the old engine freed */
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.swaps_ok, 1);
    CHECK_EQ_INT(s.swaps_stale, 1);
    /* the engine now playing is the first one's (330 Hz), not the stale one's (440 Hz) */
    ht_pump(h, 9600, &c, 5000);
    int crossings = 0;
    for (size_t i = c.frames - 4800 + 1; i < c.frames; i++)
        if (c.left[i - 1] < 0.0f && c.left[i] >= 0.0f) crossings++;
    CHECK(crossings >= 30 && crossings <= 36);               /* 330 Hz over 0.1 s: 33 cycles */
    ht_capture_free(&c);
    rb_host_destroy(h);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline);
}

/* ---- rapid successive swaps ------------------------------------------------------------ */

void test_swap_rapid_successive_swaps_each_get_their_own_fade_and_ack(void)
{
    const int baseline = rb_host_debug_live_sims();
    rb_host_build tones[3];
    ht_tone_build(&tones[0], 220.0);
    ht_tone_build(&tones[1], 330.0);
    ht_tone_build(&tones[2], 440.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &tones[0], 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 96000);
    ht_pump(h, 4800, &c, 5000);
    /* three builds queued for swapping one straight after another, none waiting for an ack */
    uint64_t pid[3], req[3];
    for (int i = 0; i < 3; i++) {
        rb_host_prepare_begin(h, &tones[(i + 1) % 3], &pid[i]);
        CHECK_EQ_INT(rb_host_prepare_wait(h, pid[i], 20000), RB_HOST_PREPARE_READY);
    }
    for (int i = 0; i < 3; i++) CHECK_EQ_INT(rb_host_swap(h, pid[i], RB_HOST_ANY_GENERATION, &req[i]), RB_HOST_OK);
    ht_pump_until_ack(h, req[2], &c, 20000);
    ht_pump(h, 4800, &c, 5000);
    int idx[3];
    for (int i = 0; i < 3; i++) {
        idx[i] = -1;
        for (int k = 0; k < c.n_acks; k++)
            if (c.acks[k].request_id == req[i]) idx[i] = k;
        CHECK(idx[i] >= 0);
    }
    if (idx[0] >= 0 && idx[1] >= 0 && idx[2] >= 0) {
        CHECK(idx[0] < idx[1] && idx[1] < idx[2]);             /* in order */
        for (int i = 0; i < 3; i++) {
            CHECK_EQ_INT(c.acks[idx[i]].outcome, RB_HOST_ACK_OK);
            CHECK_EQ_INT(c.acks[idx[i]].generation, 2 + i);    /* one generation each */
            CHECK_EQ_INT(c.acks[idx[i]].epoch, 2 + i);
        }
        /* each swap fades out and in: two fades between one switch and the next, no overlap */
        const uint64_t gap1 = c.acks[idx[1]].audio_frame - c.acks[idx[0]].audio_frame;
        const uint64_t gap2 = c.acks[idx[2]].audio_frame - c.acks[idx[1]].audio_frame;
        CHECK(gap1 >= 2 * fade_of(h));
        CHECK(gap2 >= 2 * fade_of(h));
        for (int i = 0; i < 3; i++) {
            const click m = measure_click(&c, c.acks[idx[i]].audio_frame, fade_of(h));
            (void)m;     /* too close together to measure against steady windows: the step checked below */
        }
        const size_t first = (size_t)(c.acks[idx[0]].audio_frame - c.first_frame) - fade_of(h) - 2;
        const size_t last = (size_t)(c.acks[idx[2]].audio_frame - c.first_frame) + fade_of(h) + 2;
        CHECK(ht_max_step(c.left, first, last) < 0.03);        /* the tones' own steps are at most 0.0289 at 440 Hz */
    }
    rb_host_poll(h, NULL, 0);
    CHECK_EQ_INT(rb_host_generation(h), 4);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 1);
    ht_capture_free(&c);
    rb_host_destroy(h);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline);
}

/* The retired queue holds 4. A swap that would overflow it isn't taken (BUSY): the build stays with
 * the control thread, and works once the retired engines are freed. */
void test_swap_busy_when_engines_are_not_being_freed_then_retried(void)
{
    const int baseline = rb_host_debug_live_sims();
    rb_host_build t;
    ht_tone_build(&t, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &t, 48000);
    if (!h) return;
    rb_host_start(h);
    uint64_t pid[RB_HOST_RETIRED_CAPACITY + 2], req[RB_HOST_RETIRED_CAPACITY + 2];
    const int n = RB_HOST_RETIRED_CAPACITY + 2;
    for (int i = 0; i < n; i++) {
        rb_host_prepare_begin(h, &t, &pid[i]);
        CHECK_EQ_INT(rb_host_prepare_wait(h, pid[i], 20000), RB_HOST_PREPARE_READY);
        CHECK_EQ_INT(rb_host_swap(h, pid[i], RB_HOST_ANY_GENERATION, &req[i]), RB_HOST_OK);
    }
    /* pump the audio, but look at acks only through the render thread's stats: no poll frees anything */
    const uint64_t until = rb_host_time_ns() + 30000000000ull;
    rb_host_stats s;
    do {
        ht_pump_raw(h, RB_HOST_BLOCK_FRAMES * 4, 5000);
        rb_host_get_stats(h, &s);
    } while (s.swaps_busy < 2 && rb_host_time_ns() < until);
    CHECK_EQ_INT(s.swaps_ok, RB_HOST_RETIRED_CAPACITY);       /* four fit in the retired queue */
    CHECK_EQ_INT(s.swaps_busy, 2);
    CHECK_EQ_INT(s.retired_pending, RB_HOST_RETIRED_CAPACITY);
    /* now poll: the engines are freed, the two busy swaps are back in hand, and swap again */
    ht_capture c;
    ht_capture_init(&c, 4096);
    ht_pump(h, 1280, &c, 5000);
    int busy = 0, ok = 0;
    for (int k = 0; k < c.n_acks; k++) {
        busy += c.acks[k].outcome == RB_HOST_ACK_BUSY;
        ok += c.acks[k].outcome == RB_HOST_ACK_OK;
    }
    CHECK_EQ_INT(busy, 2);
    CHECK_EQ_INT(ok, RB_HOST_RETIRED_CAPACITY);
    int state = -1;
    rb_host_prepare_status(h, pid[n - 1], &state, NULL, NULL, 0);
    CHECK_EQ_INT(state, RB_HOST_PREPARE_READY);               /* still ours */
    uint64_t again[2];
    CHECK_EQ_INT(rb_host_swap(h, pid[n - 2], RB_HOST_ANY_GENERATION, &again[0]), RB_HOST_OK);
    CHECK_EQ_INT(rb_host_swap(h, pid[n - 1], RB_HOST_ANY_GENERATION, &again[1]), RB_HOST_OK);
    ht_pump_until_ack(h, again[1], &c, 20000);
    int k0 = -1, k1 = -1;
    for (int k = 0; k < c.n_acks; k++) {
        if (c.acks[k].request_id == again[0]) k0 = k;
        if (c.acks[k].request_id == again[1]) k1 = k;
    }
    CHECK(k0 >= 0 && k1 >= 0);
    if (k0 >= 0 && k1 >= 0) {
        CHECK_EQ_INT(c.acks[k0].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[k1].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[k1].generation, 2 + RB_HOST_RETIRED_CAPACITY + 1);
    }
    ht_capture_free(&c);
    rb_host_destroy(h);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline);
}

/* ---- commands around a swap ---------------------------------------------------------------- */

void test_swap_commands_behind_it_wait_and_apply_to_the_new_engine(void)
{
    rb_host_build a, b;
    ht_tone_build(&a, 220.0);
    ht_tone_build(&b, 330.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    rb_host_start(h);
    ht_capture c;
    ht_capture_init(&c, 48000);
    ht_pump(h, 2400, &c, 5000);
    uint64_t pid, swap_req, any_req, old_req, new_req;
    rb_host_prepare_begin(h, &b, &pid);
    rb_host_prepare_wait(h, pid, 20000);
    CHECK_EQ_INT(rb_host_swap(h, pid, 0, &swap_req), RB_HOST_OK);
    rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 0.8, 0, &any_req);          /* any generation: the new engine's */
    rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 0.5, 1, &old_req);          /* meant for the old one: stale */
    rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 0.6, 2, &new_req);          /* meant for the new one */
    ht_pump_until_ack(h, new_req, &c, 20000);
    int ks = -1, ka = -1, ko = -1, kn = -1;
    for (int k = 0; k < c.n_acks; k++) {
        if (c.acks[k].request_id == swap_req) ks = k;
        if (c.acks[k].request_id == any_req) ka = k;
        if (c.acks[k].request_id == old_req) ko = k;
        if (c.acks[k].request_id == new_req) kn = k;
    }
    CHECK(ks >= 0 && ka >= 0 && ko >= 0 && kn >= 0);
    if (ks >= 0 && ka >= 0 && ko >= 0 && kn >= 0) {
        CHECK(ks < ka && ka < ko && ko < kn);                 /* in the order sent: the swap went first */
        CHECK(c.acks[ka].audio_frame >= c.acks[ks].audio_frame);   /* applied after the switch */
        CHECK_EQ_INT(c.acks[ka].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[ka].generation, 2);
        CHECK_EQ_INT(c.acks[ko].outcome, RB_HOST_ACK_STALE);
        CHECK_EQ_INT(c.acks[kn].outcome, RB_HOST_ACK_OK);
        CHECK_EQ_INT(c.acks[kn].generation, 2);
    }
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.commands_stale, 1);
    ht_capture_free(&c);
    rb_host_destroy(h);
}

/* ---- concurrency ------------------------------------------------------------------------------- */

typedef struct device_thread {
    rb_host   *host;
    atomic_int stop;
    uint64_t   pulled;
    int        torn_audio;      /* a step bigger than a swap's fades allow */
} device_thread;

static void device_main(void *arg)
{
    device_thread *d = (device_thread *)arg;
    float block[2 * 128], last = 0.0f;
    while (!atomic_load(&d->stop)) {
        if (rbh_ring_fill(&d->host->ring) < 128) {
            rbh_plat_sleep_ms(0);
            continue;
        }
        rb_host_pull(d->host, block, 128);
        for (int i = 0; i < 128; i++) {
            if (fabsf(block[2 * i] - last) > 0.05f) d->torn_audio++;
            last = block[2 * i];
        }
        d->pulled += 128;
    }
}

/* A device thread pulls flat out while the control thread swaps over and over, sets
 * parameters and reads snapshots: no click, no torn snapshot, no leak, no forbidden call. */
void test_swap_concurrent_with_the_device_thread_and_readers(void)
{
    const int baseline = rb_host_debug_live_sims();
    rb_host_build a, b;
    ht_tone_build(&a, 220.0);
    ht_tone_build(&b, 330.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &a, 48000);
    if (!h) return;
    rb_host_start(h);
    device_thread dev;
    memset(&dev, 0, sizeof dev);
    dev.host = h;
    atomic_init(&dev.stop, 0);
    rbh_thread *t = rbh_plat_thread_start(device_main, &dev);
    CHECK(t != NULL);
    static uint8_t buf[RB_HOST_SNAPSHOT_MAX_BYTES];
    int torn = 0, swaps = 0;
    for (int i = 0; i < 40; i++) {
        uint64_t pid = 0, req = 0;
        rb_host_prepare_begin(h, (i & 1) ? &a : &b, &pid);
        if (rb_host_prepare_wait(h, pid, 20000) != RB_HOST_PREPARE_READY) break;
        rb_host_swap(h, pid, RB_HOST_ANY_GENERATION, &req);
        for (int j = 0; j < 6; j++) rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 0.7 + 0.01 * j, 0, NULL);
        const uint64_t until = rb_host_time_ns() + 20000000000ull;
        int done = 0;
        while (!done && rb_host_time_ns() < until) {
            rb_host_ack acks[16];
            const int n = rb_host_poll(h, acks, 16);
            for (int k = 0; k < n; k++)
                if (acks[k].request_id == req) done = 1;
            rb_host_snapshot_info info;
            while (rb_host_snapshot_read(h, buf, sizeof buf, &info) > 0) {
                rb_snapshot_header hdr;
                memcpy(&hdr, buf, sizeof hdr);
                if (hdr.physics_tick != info.physics_tick || hdr.magic != RB_SNAPSHOT_MAGIC) torn++;
            }
            rbh_plat_sleep_ms(1);
        }
        swaps += done;
    }
    atomic_store(&dev.stop, 1);
    rbh_plat_thread_join(t);
    rb_host_poll(h, NULL, 0);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(swaps, 40);
    CHECK_EQ_INT(torn, 0);
    CHECK_EQ_INT(dev.torn_audio, 0);
    CHECK_EQ_INT(rb_host_generation(h), 41);
    CHECK_EQ_INT(s.swaps_ok, 40);
    CHECK_EQ_INT(s.audit_alloc + s.audit_lock + s.audit_log, 0);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline + 1);
    rb_host_destroy(h);
    CHECK_EQ_INT(rb_host_debug_live_sims(), baseline);
}
