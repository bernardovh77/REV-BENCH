/*
 * test_offline.c - the offline mode (stage 123): a scripted drive rendered with
 * no device and no thread must be exactly what the core renders when driven
 * through rb.h directly (the reference below makes only public calls, in
 * rb_cli sound's order), whatever the host publishes meanwhile. rb_cli's own
 * drives are checked by tools/host_parity.py against the trace player
 * (test_trace.c covers its parser).
 */
#include "ht.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_test_util.h"

/* One step of a scripted drive: at `frame` (a multiple of 128), a parameter. */
typedef struct step {
    int    frame;
    int    param;
    double value;
} step;

static const step SCRIPT[] = {
    { 1280, RB_PARAM_THROTTLE, 0.6 },
    { 2560, RB_PARAM_HOLD_RPM, 2500.0 },
    { 3840, RB_PARAM_THROTTLE, 0.05 },
    { 5120, RB_PARAM_HOLD_RPM, 1800.0 },
};
#define N_STEPS ((int)(sizeof SCRIPT / sizeof SCRIPT[0]))
#define DRIVE_FRAMES 7000      /* not a multiple of 128: the last block is short, as rb_cli's */

static rb_host_build drive_build(int scene)
{
    rb_host_build b;
    ht_engine_build(&b, rb_preset_find("i4"), 1500.0, 0.2);
    rb_host_build_param(&b, RB_PARAM_VARIATION, 1.0);
    rb_host_build_param(&b, RB_PARAM_LISTENER, RB_LISTENER_SIDE);
    b.scene = scene;
    return b;
}

/* The core driven directly, the way rb_cli sound drives it. out: stereo interleaved. */
static void reference_drive(int scene, float *out)
{
    rb_config cfg;
    rb_config_default(&cfg);
    cfg.sample_rate = 48000.0;
    rb_config_quality(&cfg, RB_QUALITY_LOW);
    rb_sim *sim = rb_create(&cfg);
    rb_load_preset(sim, rb_preset_find("i4"));
    rb_set_param(sim, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
    rb_set_param(sim, RB_PARAM_IGNITION, 1);
    rb_set_param(sim, RB_PARAM_HOLD, 1);
    rb_set_param(sim, RB_PARAM_HOLD_RPM, 1500.0);
    rb_set_param(sim, RB_PARAM_THROTTLE, 0.2);
    rb_set_param(sim, RB_PARAM_VARIATION, 1.0);
    rb_set_param(sim, RB_PARAM_LISTENER, RB_LISTENER_SIDE);
    if (scene >= 0) rb_set_scene(sim, scene);
    int next = 0;
    float mono[128], stereo[256];
    for (int frame = 0; frame < DRIVE_FRAMES; frame += 128) {
        while (next < N_STEPS && SCRIPT[next].frame <= frame) {
            rb_set_param(sim, SCRIPT[next].param, SCRIPT[next].value);
            next++;
        }
        const int n = DRIVE_FRAMES - frame < 128 ? DRIVE_FRAMES - frame : 128;
        if (scene >= 0) {
            rb_render_stereo(sim, stereo, n);
            memcpy(out + 2 * frame, stereo, (size_t)n * 2 * sizeof(float));
        } else {
            rb_render_block(sim, mono, n);
            for (int i = 0; i < n; i++) out[2 * (frame + i)] = out[2 * (frame + i) + 1] = mono[i];
        }
    }
    rb_destroy(sim);
}

/* The same drive through the host's offline mode: commands queued between renders. */
static void offline_drive(const rb_host_build *build, int snapshot_hz, float *out)
{
    rb_host_config c;
    rb_host_config_default(&c);
    c.backend = RB_HOST_BACKEND_OFFLINE;
    c.snapshot_hz = snapshot_hz;
    c.initial = *build;
    rb_host *h = NULL;
    CHECK_EQ_INT(rb_host_create(&c, &h), RB_HOST_OK);
    if (!h) return;
    CHECK_EQ_INT(rb_host_start(h), RB_HOST_OK);
    int frame = 0;
    for (int i = 0; i <= N_STEPS; i++) {
        const int until = i < N_STEPS ? SCRIPT[i].frame : DRIVE_FRAMES;
        if (until > frame) CHECK_EQ_INT(rb_host_offline_render(h, out + 2 * frame, until - frame), until - frame);
        frame = until;
        if (i < N_STEPS) CHECK_EQ_INT(rb_host_set_param(h, SCRIPT[i].param, SCRIPT[i].value, 0, NULL), RB_HOST_OK);
        rb_host_ack acks[16];
        rb_host_poll(h, acks, 16);
    }
    rb_host_destroy(h);
}

void test_offline_mono_drive_is_the_cores_own_bit_for_bit(void)
{
    static float want[2 * DRIVE_FRAMES], got[2 * DRIVE_FRAMES];
    reference_drive(-1, want);
    const rb_host_build b = drive_build(-1);
    offline_drive(&b, 60, got);
    CHECK_EQ_INT(memcmp(want, got, sizeof want), 0);
    CHECK(ht_rms(got, 0, 2 * DRIVE_FRAMES) > 1e-4);    /* and it is sound, not two silences that agree */
    for (int i = 0; i < DRIVE_FRAMES; i++)
        if (got[2 * i] != got[2 * i + 1]) {                /* a mono engine plays in both ears alike */
            CHECK(0);
            break;
        }
}

void test_offline_stereo_scene_drive_is_the_cores_own_bit_for_bit(void)
{
    static float want[2 * DRIVE_FRAMES], got[2 * DRIVE_FRAMES];
    reference_drive(RB_SCENE_DYNO, want);
    const rb_host_build b = drive_build(RB_SCENE_DYNO);
    offline_drive(&b, 60, got);
    CHECK_EQ_INT(memcmp(want, got, sizeof want), 0);
    int differ = 0;
    for (int i = 0; i < DRIVE_FRAMES; i++) differ += got[2 * i] != got[2 * i + 1];
    CHECK(differ > 100);                                   /* the two ears really differ */
}

/* What the host publishes on the side (snapshots at any rate, events) changes nothing it renders. */
void test_offline_publishing_snapshots_and_events_changes_nothing(void)
{
    static float a[2 * DRIVE_FRAMES], b2[2 * DRIVE_FRAMES], c[2 * DRIVE_FRAMES];
    rb_host_build plain = drive_build(-1), watched = drive_build(-1);
    rb_host_build_param(&watched, RB_PARAM_EVENTS, 1);
    offline_drive(&plain, 1, a);
    offline_drive(&watched, 240, b2);
    offline_drive(&plain, 240, c);
    CHECK_EQ_INT(memcmp(a, b2, sizeof a), 0);
    CHECK_EQ_INT(memcmp(a, c, sizeof a), 0);
}

void test_offline_runs_repeat_exactly(void)
{
    static float a[2 * DRIVE_FRAMES], b2[2 * DRIVE_FRAMES];
    const rb_host_build b = drive_build(-1);
    offline_drive(&b, 60, a);
    offline_drive(&b, 60, b2);
    CHECK_EQ_INT(memcmp(a, b2, sizeof a), 0);
}

void test_offline_refuses_what_only_a_device_host_does(void)
{
    rb_host_build t;
    ht_tone_build(&t, 220.0);
    rb_host *h = ht_make(RB_HOST_BACKEND_MANUAL, &t, 48000);
    if (!h) return;
    float out[256];
    CHECK_EQ_INT(rb_host_offline_render(h, out, 128), RB_HOST_ERR_STATE);
    rb_host_destroy(h);
    rb_host_config c;
    rb_host_config_default(&c);
    c.backend = RB_HOST_BACKEND_OFFLINE;
    c.initial = t;
    CHECK_EQ_INT(rb_host_create(&c, &h), RB_HOST_OK);
    if (!h) return;
    CHECK_EQ_INT(rb_host_offline_render(h, out, -1), RB_HOST_ERR_ARG);
    CHECK_EQ_INT(rb_host_offline_render(NULL, out, 128), RB_HOST_ERR_ARG);
    rb_host_destroy(h);
}

/* A swap in the offline mode: the same fades, deterministic, and silent at the switch. */
void test_offline_swap_fades_and_repeats_exactly(void)
{
    enum { TOTAL = 9600, SWAP_AT = 2560 };
    static float run[2][2 * TOTAL];
    for (int pass = 0; pass < 2; pass++) {
        rb_host_build a, b;
        ht_tone_build(&a, 220.0);
        ht_tone_build(&b, 330.0);
        rb_host_config c;
        rb_host_config_default(&c);
        c.backend = RB_HOST_BACKEND_OFFLINE;
        c.initial = a;
        rb_host *h = NULL;
        CHECK_EQ_INT(rb_host_create(&c, &h), RB_HOST_OK);
        if (!h) return;
        rb_host_start(h);
        rb_host_offline_render(h, run[pass], SWAP_AT);
        uint64_t pid = 0, req = 0;
        rb_host_prepare_begin(h, &b, &pid);
        CHECK_EQ_INT(rb_host_prepare_wait(h, pid, 20000), RB_HOST_PREPARE_READY);
        CHECK_EQ_INT(rb_host_swap(h, pid, 0, &req), RB_HOST_OK);
        rb_host_offline_render(h, run[pass] + 2 * SWAP_AT, TOTAL - SWAP_AT);
        rb_host_ack acks[8];
        const int n = rb_host_poll(h, acks, 8);
        CHECK_EQ_INT(n, 1);
        if (n == 1) {
            CHECK_EQ_INT(acks[0].request_id, req);
            CHECK_EQ_INT(acks[0].outcome, RB_HOST_ACK_OK);
            /* the swap was taken at the first block after SWAP_AT, faded out over `fade` frames, then in */
            const uint32_t fade = h->fade_frames;
            CHECK_EQ_INT(acks[0].audio_frame, SWAP_AT + fade);
            float quiet = 0.0f;
            for (uint32_t i = 0; i < 32; i++) quiet = fmaxf(quiet, fabsf(run[pass][2 * (SWAP_AT + fade - 32 + i)]));
            CHECK(quiet < 0.01f);
        }
        rb_host_destroy(h);
    }
    CHECK_EQ_INT(memcmp(run[0], run[1], sizeof run[0]), 0);
}
