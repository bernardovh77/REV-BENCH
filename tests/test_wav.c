/*
 * test_wav.c - WAV writer/reader tests: exact header bytes, round trips,
 * clipping and error handling.
 */
#include "rb_test.h"
#include "wav.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long le16(const unsigned char *p)
{
    return (long)p[0] | ((long)p[1] << 8);
}

static long le32(const unsigned char *p)
{
    return (long)p[0] | ((long)p[1] << 8) | ((long)p[2] << 16) | ((long)p[3] << 24);
}

/* Reads a whole file into a malloc'd buffer (NULL on failure). */
static unsigned char *slurp(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *buf;

    *size = 0;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(*size > 0 ? (size_t)*size : 1);
    if (buf && fread(buf, 1, (size_t)*size, f) != (size_t)*size) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    return buf;
}

void test_wav_pcm16_roundtrip(void)
{
    enum { N = 1000 };
    char path[512];
    float x[N];
    long size;
    unsigned char *raw;
    wav_data d;
    wav_writer *w;

    rbt_tmp_path(path, sizeof path, "rbt_pcm16.wav");
    for (int i = 0; i < N; i++)
        x[i] = (float)(0.8 * sin(2.0 * 3.14159265358979 * 440.0 * i / 44100.0));

    w = wav_open(path, 44100, 1, WAV_PCM16);
    CHECK(w != NULL);
    if (!w) return;
    CHECK_EQ_INT(wav_write(w, x, 400), WAV_OK);          /* in two pieces */
    CHECK_EQ_INT(wav_write(w, x + 400, N - 400), WAV_OK);
    CHECK_EQ_INT(wav_close(w), WAV_OK);

    /* The classic 44-byte header, field by field. */
    raw = slurp(path, &size);
    CHECK(raw != NULL);
    if (raw) {
        CHECK_EQ_INT(size, 44 + 2 * N);
        CHECK(memcmp(raw, "RIFF", 4) == 0);
        CHECK_EQ_INT(le32(raw + 4), size - 8);
        CHECK(memcmp(raw + 8, "WAVEfmt ", 8) == 0);
        CHECK_EQ_INT(le32(raw + 16), 16);       /* fmt chunk size  */
        CHECK_EQ_INT(le16(raw + 20), 1);        /* PCM             */
        CHECK_EQ_INT(le16(raw + 22), 1);        /* mono            */
        CHECK_EQ_INT(le32(raw + 24), 44100);    /* sample rate     */
        CHECK_EQ_INT(le32(raw + 28), 88200);    /* bytes / second  */
        CHECK_EQ_INT(le16(raw + 32), 2);        /* bytes / frame   */
        CHECK_EQ_INT(le16(raw + 34), 16);       /* bits / sample   */
        CHECK(memcmp(raw + 36, "data", 4) == 0);
        CHECK_EQ_INT(le32(raw + 40), 2 * N);
        free(raw);
    }

    CHECK_EQ_INT(wav_read(path, &d), WAV_OK);
    CHECK_EQ_INT(d.sample_rate, 44100);
    CHECK_EQ_INT(d.channels, 1);
    CHECK_EQ_INT(d.format, WAV_PCM16);
    CHECK_EQ_INT(d.frames, N);
    if (d.samples && d.frames == N) {
        double maxerr = 0.0;
        for (int i = 0; i < N; i++) {
            double e = fabs((double)d.samples[i] - (double)x[i]);
            if (e > maxerr) maxerr = e;
        }
        CHECK(maxerr <= 2.0 / 32767.0);         /* quantisation error only */
    }
    wav_free(&d);
    remove(path);
}

void test_wav_float32_stereo_roundtrip(void)
{
    enum { FRAMES = 300 };
    char path[512];
    float x[FRAMES * 2];
    long size;
    unsigned char *raw;
    wav_data d;
    wav_writer *w;

    rbt_tmp_path(path, sizeof path, "rbt_float32.wav");
    for (int i = 0; i < FRAMES; i++) {
        x[2 * i]     = (float)i / FRAMES;           /* left: a ramp         */
        x[2 * i + 1] = -1.5f * (float)i / FRAMES;   /* right: goes past -1  */
    }

    w = wav_open(path, 96000, 2, WAV_FLOAT32);
    CHECK(w != NULL);
    if (!w) return;
    CHECK_EQ_INT(wav_write(w, x, FRAMES), WAV_OK);
    CHECK_EQ_INT(wav_close(w), WAV_OK);

    raw = slurp(path, &size);
    CHECK(raw != NULL);
    if (raw) {
        CHECK_EQ_INT(size, 58 + FRAMES * 2 * 4);
        CHECK_EQ_INT(le32(raw + 4), size - 8);
        CHECK_EQ_INT(le32(raw + 16), 18);       /* fmt chunk with cbSize */
        CHECK_EQ_INT(le16(raw + 20), 3);        /* IEEE float            */
        CHECK_EQ_INT(le16(raw + 22), 2);        /* stereo                */
        CHECK_EQ_INT(le32(raw + 24), 96000);
        CHECK_EQ_INT(le16(raw + 32), 8);        /* bytes / frame         */
        CHECK_EQ_INT(le16(raw + 34), 32);
        CHECK(memcmp(raw + 38, "fact", 4) == 0);
        CHECK_EQ_INT(le32(raw + 46), FRAMES);
        CHECK(memcmp(raw + 50, "data", 4) == 0);
        CHECK_EQ_INT(le32(raw + 54), FRAMES * 2 * 4);
        free(raw);
    }

    CHECK_EQ_INT(wav_read(path, &d), WAV_OK);
    CHECK_EQ_INT(d.channels, 2);
    CHECK_EQ_INT(d.format, WAV_FLOAT32);
    CHECK_EQ_INT(d.frames, FRAMES);
    if (d.samples && d.frames == FRAMES)        /* float is lossless, no clipping */
        CHECK(memcmp(d.samples, x, sizeof x) == 0);
    wav_free(&d);
    remove(path);
}

void test_wav_pcm16_clipping(void)
{
    const float x[6]      = { 1.5f, -1.5f, 1.0f, -1.0f, 0.0f, NAN };
    const int   expect[6] = { 32767, -32767, 32767, -32767, 0, 0 };
    char path[512];
    long size;
    unsigned char *raw;
    wav_writer *w;

    rbt_tmp_path(path, sizeof path, "rbt_clip.wav");
    w = wav_open(path, 48000, 1, WAV_PCM16);
    CHECK(w != NULL);
    if (!w) return;
    CHECK_EQ_INT(wav_write(w, x, 6), WAV_OK);
    CHECK_EQ_INT(wav_close(w), WAV_OK);

    raw = slurp(path, &size);
    CHECK(raw != NULL && size == 44 + 12);
    if (raw && size == 44 + 12) {
        for (int i = 0; i < 6; i++) {
            long v = le16(raw + 44 + 2 * i);
            if (v >= 32768) v -= 65536;
            CHECK_EQ_INT(v, expect[i]);
        }
    }
    free(raw);
    remove(path);
}

void test_wav_rejects_garbage(void)
{
    char path[512];
    wav_data d;
    FILE *f;

    rbt_tmp_path(path, sizeof path, "rbt_garbage.wav");
    f = fopen(path, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs("this is not a wav file", f);
        fclose(f);
    }
    CHECK_EQ_INT(wav_read(path, &d), WAV_ERR_FORMAT);
    CHECK(d.samples == NULL);
    CHECK_EQ_INT(wav_read("/nonexistent/dir/x.wav", &d), WAV_ERR_IO);
    CHECK(wav_open("/nonexistent/dir/x.wav", 48000, 1, WAV_PCM16) == NULL);
    CHECK(wav_open(path, 0, 1, WAV_PCM16) == NULL);
    CHECK(wav_open(path, 48000, 0, WAV_PCM16) == NULL);
    remove(path);
}
