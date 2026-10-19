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
        /* Rounding and dither, up to 1.5 LSB, and the reader's 1/32768 for
         * the writer's 32767 (0.8 LSB at this level). */
        CHECK(maxerr <= 2.5 / 32767.0);
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
    wav_set_dither(w, 0);                       /* exact values */
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

/* Writes a 1 kHz tone of `lsb` LSB at 48 kHz, 1 s of it, as 16-bit PCM,
 * dithered (1), not (0) or as wav_open leaves it (-1), and reads the
 * integers back into q[]. */
static int write_tone(const char *name, double lsb, int dither, long *q)
{
    enum { N = 48000 };
    static float x[N];
    char path[512];
    long size;
    for (int i = 0; i < N; i++) x[i] = (float)(lsb / 32767.0 * sin(2.0 * 3.14159265358979323846 * i / 48.0));
    rbt_tmp_path(path, sizeof path, name);
    wav_writer *w = wav_open(path, 48000, 1, WAV_PCM16);
    if (!w) return -1;
    if (dither >= 0) wav_set_dither(w, dither);
    wav_write(w, x, N);
    if (wav_close(w) != WAV_OK) return -1;
    unsigned char *raw = slurp(path, &size);
    if (!raw || size != 44 + 2 * N) {
        free(raw);
        return -1;
    }
    for (int i = 0; i < N; i++) {
        q[i] = le16(raw + 44 + 2 * i);
        if (q[i] >= 32768) q[i] -= 65536;
    }
    free(raw);
    remove(path);
    return 0;
}

/* The amplitude (LSB) of harmonic h of 1 kHz in q[] minus the tone: its
 * Fourier coefficient over exactly 1000 periods. */
static double harmonic(const long *q, double lsb, int h)
{
    double re = 0.0, im = 0.0;
    for (int i = 0; i < 48000; i++) {
        const double e = (double)q[i] - lsb * sin(2.0 * 3.14159265358979323846 * i / 48.0);
        re += e * cos(2.0 * 3.14159265358979323846 * h * i / 48.0);
        im += e * sin(2.0 * 3.14159265358979323846 * h * i / 48.0);
    }
    return 2.0 * sqrt(re * re + im * im) / 48000.0;
}

void test_wav_pcm16_dither(void)
{
    /* A 1 kHz tone 2.5 LSB high, the quietest thing a 16-bit file can say.
     * Rounded as it is, the error follows the tone: harmonics of a tenth of
     * an LSB and more. Dithered, the error is a steady hiss of 0.5 LSB RMS
     * (1/6 + 1/12 LSB^2), with no harmonics above its own floor (0.005 LSB
     * per coefficient over a second), no offset, and the tone intact. The
     * same samples make the same file, and that's what wav_open does unless
     * told otherwise. */
    static long plain[48000], dith[48000], again[48000];
    CHECK_EQ_INT(write_tone("rbt_plain.wav", 2.5, 0, plain), 0);
    CHECK_EQ_INT(write_tone("rbt_dither.wav", 2.5, 1, dith), 0);
    CHECK_EQ_INT(write_tone("rbt_again.wav", 2.5, -1, again), 0);

    double worst_plain = 0.0, worst_dither = 0.0;
    for (int h = 2; h <= 9; h++) {
        worst_plain = fmax(worst_plain, harmonic(plain, 2.5, h));
        worst_dither = fmax(worst_dither, harmonic(dith, 2.5, h));
    }
    CHECK(worst_plain > 0.1);
    CHECK(worst_dither < 0.02);
    double sum = 0.0, sumsq = 0.0, re = 0.0;
    for (int i = 0; i < 48000; i++) {
        const double e = dith[i] - 2.5 * sin(2.0 * 3.14159265358979323846 * i / 48.0);
        sum += e;
        sumsq += e * e;
        re += dith[i] * sin(2.0 * 3.14159265358979323846 * i / 48.0);
    }
    CHECK_NEAR(sqrt(sumsq / 48000), 0.5, 0.01);
    CHECK(fabs(sum / 48000) < 0.01);
    CHECK_NEAR(2.0 * re / 48000, 2.5, 0.01);
    CHECK(memcmp(dith, again, sizeof dith) == 0);
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
