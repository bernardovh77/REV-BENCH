/*
 * test_trace.c - the command traces of the offline mode (stage 123): the text
 * format's parser and errors, and the player. tools/host_parity.py checks
 * the player against rb_cli's own drives.
 */
#include "ht.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/rb_host_trace.h"
#include "host_test_util.h"

static const char *TONE_TRACE =
    "rbtrace 1\n"
    "# a tone, then its level changes\n"
    "rate 48000\n"
    "quality low\n"
    "param source 0\n"
    "param test_tone_hz 300\n"
    "skip 1024\n"
    "frames 2000\n"
    "at 2048 set master_gain 0\n";

typedef struct collected {
    float *samples;
    long   frames;
    int    channels;
} collected;

static int collect(void *user, const float *samples, int frames, int channels)
{
    collected *c = (collected *)user;
    c->samples = (float *)realloc(c->samples, (size_t)(c->frames + frames) * (size_t)channels * sizeof(float));
    memcpy(c->samples + (size_t)c->frames * (size_t)channels, samples, (size_t)frames * (size_t)channels * sizeof(float));
    c->frames += frames;
    c->channels = channels;
    return 0;
}

void test_trace_parses_a_build_and_its_commands(void)
{
    char error[200];
    const char *text =
        "rbtrace 1\n"
        "rate 44100\n"
        "quality -1\n"
        "oversample 4\n"
        "preset v8-cross\n"
        "edit header_length 2 0.55\n"
        "param ignition 1\n"
        "scene 6\n"
        "scene_time -1.5\n"
        "skip 128\n"
        "frames 4096\n"
        "at 0 set throttle 0.25\n"
        "at 256 live idle_rpm 0 900\n"
        "at 256 dyno_attach 3000\n"
        "at 512 dyno_sweep 4000 500\n"
        "at 512 dyno_clear\n"
        "at 640 dyno_release\n"
        "at 768 scene_time -1\n"
        "build other\n"
        "preset i4\n"
        "end\n"
        "at 1024 swap other\n";
    rb_trace *t = rb_trace_parse(text, error, sizeof error);
    CHECK(t != NULL);
    if (!t) {
        printf("    %s\n", error);
        return;
    }
    CHECK_EQ_INT(rb_trace_rate(t), 44100);
    CHECK_EQ_INT(rb_trace_skip_frames(t), 128);
    CHECK_EQ_INT(rb_trace_frames(t), 4096);
    CHECK_EQ_INT(rb_trace_command_count(t), 8);
    CHECK_EQ_INT(rb_trace_build_count(t), 2);
    CHECK_EQ_INT(rb_trace_channels(t), 2);          /* a scene: stereo */
    rb_trace_free(t);
    t = rb_trace_parse(TONE_TRACE, error, sizeof error);
    CHECK(t != NULL);
    CHECK_EQ_INT(rb_trace_channels(t), 1);
    rb_trace_free(t);
}

/* Every way a trace can be wrong says which line and why, and makes no trace. */
void test_trace_errors_name_the_line(void)
{
    static const struct { const char *text, *line; } BAD[] = {
        { "", "empty" },
        { "hello\n", "line 1" },
        { "rbtrace 2\n", "line 1" },
        { "rbtrace 1\nrate 22050\n", "line 2" },
        { "rbtrace 1\nparam nonsense 1\n", "line 2" },
        { "rbtrace 1\nparam throttle abc\n", "line 2" },
        { "rbtrace 1\npreset nothing\n", "line 2" },
        { "rbtrace 1\nedit bore 0\n", "line 2" },
        { "rbtrace 1\nquality ultraviolet\n", "line 2" },
        { "rbtrace 1\nscene 99\n", "line 2" },
        { "rbtrace 1\nframes -3\n", "line 2" },
        { "rbtrace 1\nat x set throttle 1\n", "line 2" },
        { "rbtrace 1\nat 10 jump\n", "line 2" },
        { "rbtrace 1\nat 10 set throttle 0.5\nat 5 set throttle 0.6\n", "line 3" },
        { "rbtrace 1\nat 10 swap nobody\n", "line 2" },
        { "rbtrace 1\nat 10 dyno_release 7\n", "line 2" },
        { "rbtrace 1\nwhatever 1\n", "line 2" },
    };
    for (size_t i = 0; i < sizeof BAD / sizeof BAD[0]; i++) {
        char error[200] = "";
        rb_trace *t = rb_trace_parse(BAD[i].text, error, sizeof error);
        CHECK(t == NULL);
        if (t) rb_trace_free(t);
        CHECK(strstr(error, BAD[i].line) != NULL);
        if (!strstr(error, BAD[i].line)) printf("    case %d said: %s\n", (int)i, error);
    }
    char error[200];
    CHECK(rb_trace_load("this/file/does/not/exist.rbtrace", error, sizeof error) == NULL);
    CHECK(strstr(error, "cannot read") != NULL);
}

void test_trace_plays_what_a_host_renders_skipping_the_skip(void)
{
    char error[200];
    rb_trace *t = rb_trace_parse(TONE_TRACE, error, sizeof error);
    CHECK(t != NULL);
    if (!t) return;
    collected got = { NULL, 0, 0 };
    CHECK_EQ_INT(rb_trace_play(t, collect, &got, error, sizeof error), RB_HOST_OK);
    CHECK_EQ_INT(got.frames, 2000);
    CHECK_EQ_INT(got.channels, 1);
    rb_trace_free(t);

    /* the same thing through a host by hand: the trace's frames start after its skip */
    rb_host_build b;
    ht_tone_build(&b, 300.0);
    rb_host_config c;
    rb_host_config_default(&c);
    c.backend = RB_HOST_BACKEND_OFFLINE;
    c.initial = b;
    rb_host *h = NULL;
    CHECK_EQ_INT(rb_host_create(&c, &h), RB_HOST_OK);
    static float ref[2 * 3024];
    rb_host_start(h);
    rb_host_offline_render(h, ref, 2048);
    rb_host_set_param(h, RB_PARAM_MASTER_GAIN, 0.0, 0, NULL);
    rb_host_offline_render(h, ref + 2 * 2048, 3024 - 2048);
    for (int i = 0; i < 2000; i++)
        if (got.samples && got.samples[i] != ref[2 * (1024 + i)]) {
            CHECK_EQ_INT(i, -1);
            break;
        }
    CHECK(ht_rms(got.samples, 0, 400) > 0.2);                          /* sounding before the gain command */
    CHECK(ht_rms(got.samples, 1800, 2000) < 0.3 * ht_rms(got.samples, 0, 400));   /* and falling after it */
    rb_host_destroy(h);
    free(got.samples);
}

void test_trace_swap_plays_through_a_fade(void)
{
    char error[200];
    rb_trace *t = rb_trace_parse(
        "rbtrace 1\nquality low\nparam source 0\nparam test_tone_hz 220\n"
        "build second\nquality low\nparam source 0\nparam test_tone_hz 330\nend\n"
        "frames 4096\nat 1024 swap second\n", error, sizeof error);
    CHECK(t != NULL);
    if (!t) return;
    collected got = { NULL, 0, 0 };
    CHECK_EQ_INT(rb_trace_play(t, collect, &got, error, sizeof error), RB_HOST_OK);
    CHECK_EQ_INT(got.frames, 4096);
    /* the old tone fades out over 640 frames from frame 1024, the new one in: silence at 1024 + 640 */
    CHECK(ht_rms(got.samples, 512, 1024) > 0.2);
    CHECK(ht_rms(got.samples, 1024 + 640 - 24, 1024 + 640 + 24) < 0.02);
    CHECK(ht_rms(got.samples, 3000, 4096) > 0.2);
    rb_trace_free(t);
    free(got.samples);
}
