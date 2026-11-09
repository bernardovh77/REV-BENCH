/*
 * test_device.c - the audio device layer (stage 123), on miniaudio's null
 * backend only: no test ever plays on a real device. The null device is a
 * real device thread pulling at a real device's pace, so these tests run in
 * real time (a few hundred milliseconds each).
 */
#include "ht.h"

#include <math.h>
#include <string.h>

#include "host_test_util.h"

static rb_host *null_host(int rate)
{
    rb_host_build b;
    ht_tone_build(&b, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_NULL, &b, rate);
    if (h) CHECK_EQ_INT(rb_host_start(h), RB_HOST_OK);
    return h;
}

/* Waits for the device to have called back `more` more times. */
static void wait_callbacks(rb_host *h, uint64_t more, unsigned timeout_ms)
{
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    const uint64_t want = s.device_callbacks + more;
    const uint64_t until = rb_host_time_ns() + (uint64_t)timeout_ms * 1000000ull;
    while (s.device_callbacks < want && rb_host_time_ns() < until) {
        rbh_plat_sleep_ms(5);
        rb_host_get_stats(h, &s);
    }
}

void test_device_rate_choice(void)
{
    const int32_t any[] = { 0 };
    const int32_t only_48[] = { 48000 };
    const int32_t only_44[] = { 44100 };
    const int32_t both[] = { 44100, 48000 };
    const int32_t high[] = { 96000 };
    const int32_t odd[] = { 88200, 96000, 192000 };
    const int32_t low[] = { 8000, 22050, 44100 };
    CHECK_EQ_INT(rb_host_pick_rate(any, 1), 48000);
    CHECK_EQ_INT(rb_host_pick_rate(only_48, 1), 48000);
    CHECK_EQ_INT(rb_host_pick_rate(only_44, 1), 44100);     /* native 44.1 kHz stays native */
    CHECK_EQ_INT(rb_host_pick_rate(both, 2), 48000);
    CHECK_EQ_INT(rb_host_pick_rate(high, 1), 48000);        /* fed 48 kHz, converted by miniaudio */
    CHECK_EQ_INT(rb_host_pick_rate(odd, 3), 48000);
    CHECK_EQ_INT(rb_host_pick_rate(low, 3), 44100);
    CHECK_EQ_INT(rb_host_pick_rate(NULL, 0), 48000);
    CHECK_EQ_INT(rb_host_pick_rate(any, 0), 48000);
}

void test_device_null_backend_lists_one_device(void)
{
    rb_host_device_info devices[4];
    const int n = rb_host_list_devices(RB_HOST_BACKEND_NULL, devices, 4);
    CHECK(n >= 1);
    if (n >= 1) {
        CHECK_EQ_INT(devices[0].index, 0);
        CHECK(devices[0].is_default);
        CHECK(strlen(devices[0].name) > 0);
    }
    CHECK_EQ_INT(rb_host_list_devices(RB_HOST_BACKEND_MANUAL, devices, 4), RB_HOST_ERR_ARG);
    CHECK_EQ_INT(rb_host_list_devices(RB_HOST_BACKEND_NULL, NULL, 4), RB_HOST_ERR_ARG);
}

void test_device_null_backend_plays_the_render_threads_audio_in_real_time(void)
{
    rb_host *h = null_host(48000);
    if (!h) return;
    const uint64_t t0 = rb_host_time_ns();
    rbh_plat_sleep_ms(500);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    const double seconds = (double)(rb_host_time_ns() - t0) / 1e9;
    CHECK(s.device_callbacks >= 20);
    /* the device consumed about as many frames as real time asks, and no more than was rendered */
    CHECK(s.frames_played >= (uint64_t)(0.6 * 48000 * seconds));
    CHECK(s.frames_played <= (uint64_t)(1.5 * 48000 * seconds) + 4096);
    CHECK(s.frames_rendered >= s.frames_played);
    CHECK(s.underruns <= 2);           /* a shared CI runner may stall a thread: the real-device run is the evidence for none */
    CHECK_EQ_INT(s.audit_alloc + s.audit_lock + s.audit_log, 0);
    CHECK_EQ_INT(s.priority.applied, 1);
    /* the clock maps a frame to the speakers once the device has called back */
    uint64_t t;
    rb_host_clock clock;
    rb_host_clock_get(h, &clock);
    CHECK_EQ_INT(rb_host_output_time_ns(h, clock.audio_frame, &t), RB_HOST_OK);
    CHECK(t > rb_host_time_ns());                           /* the audio just rendered plays in the future */
    CHECK(t < rb_host_time_ns() + 500000000ull);
    rb_host_stop(h);
    CHECK_EQ_INT(rb_host_stop(h), RB_HOST_OK);              /* stopping twice is fine */
    rb_host_get_stats(h, &s);
    const uint64_t played = s.frames_played;
    rbh_plat_sleep_ms(50);
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.frames_played, played);                  /* stopped is stopped */
    CHECK_EQ_INT(rb_host_start(h), RB_HOST_OK);             /* and it restarts */
    wait_callbacks(h, 5, 2000);
    rb_host_get_stats(h, &s);
    CHECK(s.frames_played > played);
    rb_host_destroy(h);
}

void test_device_44100_runs_natively(void)
{
    rb_host *h = null_host(44100);
    if (!h) return;
    wait_callbacks(h, 10, 3000);
    rb_host_clock c;
    rb_host_clock_get(h, &c);
    CHECK_EQ_INT(c.audio_hz, 44100);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.ring_target, 882);
    CHECK(s.underruns <= 2);
    rb_host_destroy(h);
}

void test_device_change_starts_a_new_epoch_and_the_sound_goes_on(void)
{
    rb_host *h = null_host(48000);
    if (!h) return;
    wait_callbacks(h, 10, 3000);
    rb_host_clock before, after;
    rb_host_clock_get(h, &before);
    CHECK_EQ_INT(before.epoch, 1);
    CHECK_EQ_INT(rb_host_select_device(h, 0), RB_HOST_OK);
    rb_host_clock_get(h, &after);
    CHECK_EQ_INT(after.epoch, 2);
    CHECK_EQ_INT(after.generation, before.generation);          /* the same engine, on another device */
    CHECK(after.audio_origin >= before.audio_frame);            /* the audio clock goes on */
    wait_callbacks(h, 10, 3000);
    uint64_t t;
    CHECK_EQ_INT(rb_host_output_time_ns(h, after.audio_frame + 2000, &t), RB_HOST_OK);   /* valid again after a callback */
    rb_host_ack acks[8];
    const int n = rb_host_poll(h, acks, 8);
    CHECK_EQ_INT(n, 0);                     /* the host's own epoch ack was consumed by the change */
    CHECK_EQ_INT(rb_host_select_device(h, 99), RB_HOST_ERR_DEVICE);   /* no such device: refused, not crashed */
    rb_host_destroy(h);
}

void test_device_at_another_rate_rebuilds_the_engine_in_a_new_epoch(void)
{
    rb_host *h = null_host(48000);
    if (!h) return;
    wait_callbacks(h, 10, 3000);
    const int live = rb_host_debug_live_sims();
    rb_host_debug_next_device_rate(h, 44100);       /* as if the new device runs at 44.1 kHz */
    CHECK_EQ_INT(rb_host_select_device(h, 0), RB_HOST_OK);
    rb_host_clock c;
    rb_host_clock_get(h, &c);
    CHECK_EQ_INT(c.audio_hz, 44100);
    CHECK_EQ_INT(c.epoch, 2);
    CHECK_EQ_INT(c.generation, 2);                  /* a rebuilt engine is a new generation */
    CHECK_NEAR(rb_host_sample_rate(h), 44100.0, 0.0);
    wait_callbacks(h, 10, 3000);
    rb_host_poll(h, NULL, 0);
    CHECK_EQ_INT(rb_host_debug_live_sims(), live);  /* the old engine was freed */
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.ring_target, 960);               /* the ring's target was set at 48 kHz: 20 ms is now a little more */
    rb_host_destroy(h);
}

void test_device_suspend_stops_the_device_and_resume_restarts_it_in_a_new_epoch(void)
{
    rb_host *h = null_host(48000);
    if (!h) return;
    wait_callbacks(h, 10, 3000);
    CHECK_EQ_INT(rb_host_suspend(h), RB_HOST_OK);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    const uint64_t played = s.frames_played;
    rbh_plat_sleep_ms(100);
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.frames_played, played);                   /* nothing plays while the system sleeps */
    CHECK_EQ_INT(rb_host_resume(h), RB_HOST_OK);
    rb_host_clock c;
    rb_host_clock_get(h, &c);
    CHECK_EQ_INT(c.epoch, 2);
    uint64_t t;
    CHECK_EQ_INT(rb_host_resume(h), RB_HOST_ERR_STATE);      /* not asleep any more */
    wait_callbacks(h, 10, 3000);
    rb_host_get_stats(h, &s);
    CHECK(s.frames_played > played);
    CHECK_EQ_INT(rb_host_output_time_ns(h, c.audio_frame + 1000, &t), RB_HOST_OK);
    rb_host_destroy(h);
}

void test_device_lost_is_noticed_and_the_default_device_is_opened_again(void)
{
    rb_host *h = null_host(48000);
    if (!h) return;
    wait_callbacks(h, 10, 3000);
    rb_host_debug_device_event(h, 1);                         /* the headphones were unplugged */
    rb_host_poll(h, NULL, 0);                                 /* the control loop notices and recovers */
    rb_host_clock c;
    rb_host_clock_get(h, &c);
    CHECK_EQ_INT(c.epoch, 2);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.device_events, 1);
    const uint64_t played = s.frames_played;
    wait_callbacks(h, 10, 3000);
    rb_host_get_stats(h, &s);
    CHECK(s.frames_played > played);
    rb_host_debug_device_event(h, 2);                         /* the default moved: a reroute */
    rb_host_poll(h, NULL, 0);
    wait_callbacks(h, 5, 3000);
    rb_host_ack acks[4];
    int epoch_acks = 0;
    for (int i = 0; i < 50 && epoch_acks == 0; i++) {
        const int n = rb_host_poll(h, acks, 4);
        for (int k = 0; k < n; k++) epoch_acks += acks[k].kind == RB_HOST_CMD_EPOCH;
        rbh_plat_sleep_ms(5);
    }
    CHECK_EQ_INT(epoch_acks, 1);
    rb_host_clock_get(h, &c);
    CHECK_EQ_INT(c.epoch, 3);
    rb_host_destroy(h);
}

/* The stop/reject rule: no physics on the device thread. A device that pulls while the render
 * thread isn't running moves the engine not at all. */
void test_device_thread_never_runs_the_physics(void)
{
    rb_host_build b;
    ht_engine_build(&b, 0, 3000.0, 0.5);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &b, 48000);     /* never started: no render thread */
    if (!h) return;
    const double steps0 = rb_get_telemetry(h->run.sim, RB_TLM_STEPS);
    float out[2 * 480];
    for (int i = 0; i < 200; i++) rb_host_pull(h, out, 480);
    CHECK_NEAR(rb_get_telemetry(h->run.sim, RB_TLM_STEPS), steps0, 0.0);
    rb_host_stats s;
    rb_host_get_stats(h, &s);
    CHECK_EQ_INT(s.device_callbacks, 200);
    CHECK_EQ_INT(s.underruns, 200);                              /* nothing to play: all silence, all counted */
    for (int i = 0; i < 2 * 480; i++) CHECK(out[i] == 0.0f);
    rb_host_destroy(h);
}
