/*
 * wav.c - minimal WAV (RIFF) writer and reader.
 *
 * The writer emits 16-bit PCM or 32-bit float with any channel count. The
 * header is written with zero sizes when the file is opened and written
 * again with the real sizes by wav_close(). Every multi-byte field is
 * little-endian and written byte by byte, so this works on any host.
 *
 * Layout (PCM16, the classic 44-byte header):
 *   "RIFF" <file size - 8> "WAVE"
 *   "fmt " <16> format channels rate byte_rate block_align bits
 *   "data" <data bytes> samples...
 * Float files add a cbSize field (fmt chunk of 18 bytes) and a "fact"
 * chunk holding the frame count, as the spec requires for non-PCM data.
 *
 * Reference: http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
 */
#include "wav.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WAV_MAX_HEADER 64

struct wav_writer {
    FILE      *f;
    int        sample_rate;
    int        channels;
    wav_format fmt;
    uint32_t   data_bytes;
    int        failed;
};

/* ---- little-endian helpers ---------------------------------------- */

static void put_u16(unsigned char *p, uint16_t v)
{
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)(v >> 8);
}

static void put_u32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
    p[2] = (unsigned char)((v >> 16) & 0xFFu);
    p[3] = (unsigned char)(v >> 24);
}

static uint16_t get_u16(const unsigned char *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t get_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int bytes_per_sample(wav_format fmt)
{
    return fmt == WAV_PCM16 ? 2 : 4;
}

/* ---- writer --------------------------------------------------------- */

/* Builds the complete header for the data written so far; returns its size. */
static size_t build_header(unsigned char *h, const wav_writer *w)
{
    const int      is_float = (w->fmt == WAV_FLOAT32);
    const uint32_t bps      = (uint32_t)bytes_per_sample(w->fmt);
    const uint32_t align    = (uint32_t)w->channels * bps;   /* bytes per frame */
    const uint32_t fmt_size = is_float ? 18u : 16u;
    size_t n = 0;

    memcpy(h + n, "RIFF", 4);                          n += 4;
    n += 4;                               /* RIFF size, filled in at the end */
    memcpy(h + n, "WAVE", 4);                          n += 4;

    memcpy(h + n, "fmt ", 4);                          n += 4;
    put_u32(h + n, fmt_size);                          n += 4;
    put_u16(h + n, (uint16_t)w->fmt);                  n += 2;  /* 1 PCM, 3 float */
    put_u16(h + n, (uint16_t)w->channels);             n += 2;
    put_u32(h + n, (uint32_t)w->sample_rate);          n += 4;
    put_u32(h + n, (uint32_t)w->sample_rate * align);  n += 4;  /* bytes/second  */
    put_u16(h + n, (uint16_t)align);                   n += 2;
    put_u16(h + n, (uint16_t)(bps * 8u));              n += 2;  /* bits/sample   */

    if (is_float) {
        put_u16(h + n, 0);                             n += 2;  /* cbSize        */
        memcpy(h + n, "fact", 4);                      n += 4;
        put_u32(h + n, 4);                             n += 4;
        put_u32(h + n, w->data_bytes / align);         n += 4;  /* frames        */
    }

    memcpy(h + n, "data", 4);                          n += 4;
    put_u32(h + n, w->data_bytes);                     n += 4;

    /* RIFF size = file size - 8, including the pad byte for odd data. */
    put_u32(h + 4, (uint32_t)(n - 8) + w->data_bytes + (w->data_bytes & 1u));
    return n;
}

wav_writer *wav_open(const char *path, int sample_rate, int channels, wav_format fmt)
{
    unsigned char h[WAV_MAX_HEADER];
    wav_writer *w;
    size_t n;

    if (!path || sample_rate <= 0 || channels <= 0 || channels > 32) return NULL;
    if (fmt != WAV_PCM16 && fmt != WAV_FLOAT32) return NULL;

    w = calloc(1, sizeof *w);
    if (!w) return NULL;
    w->sample_rate = sample_rate;
    w->channels    = channels;
    w->fmt         = fmt;

    w->f = fopen(path, "wb");
    if (!w->f) {
        free(w);
        return NULL;
    }

    n = build_header(h, w);   /* placeholder sizes, fixed by wav_close() */
    if (fwrite(h, 1, n, w->f) != n) {
        fclose(w->f);
        free(w);
        return NULL;
    }
    return w;
}

int wav_write(wav_writer *w, const float *samples, size_t frames)
{
    unsigned char buf[4096];   /* a multiple of 2 and 4 bytes */
    size_t bps, total, used = 0;

    if (!w || (!samples && frames)) return WAV_ERR_IO;
    if (w->failed) return WAV_ERR_IO;

    bps   = (size_t)bytes_per_sample(w->fmt);
    total = frames * (size_t)w->channels;

    /* Stay below the 4 GiB size limit of RIFF. */
    if ((uint64_t)total * bps > (uint64_t)0xFFFFFF00u - w->data_bytes) {
        w->failed = 1;
        return WAV_ERR_IO;
    }

    for (size_t i = 0; i < total; i++) {
        float x = samples[i];
        if (!isfinite(x)) x = 0.0f;              /* never write NaN/inf */

        if (w->fmt == WAV_PCM16) {
            if (x > 1.0f)  x = 1.0f;
            if (x < -1.0f) x = -1.0f;
            long q = lrintf(x * 32767.0f);        /* round to nearest */
            put_u16(buf + used, (uint16_t)(int16_t)q);
        } else {
            uint32_t bits;
            memcpy(&bits, &x, sizeof bits);       /* raw IEEE-754 bits */
            put_u32(buf + used, bits);
        }
        used += bps;

        if (used == sizeof buf || i + 1 == total) {
            if (fwrite(buf, 1, used, w->f) != used) {
                w->failed = 1;
                return WAV_ERR_IO;
            }
            w->data_bytes += (uint32_t)used;
            used = 0;
        }
    }
    return WAV_OK;
}

int wav_close(wav_writer *w)
{
    unsigned char h[WAV_MAX_HEADER];
    int rc;
    size_t n;

    if (!w) return WAV_ERR_IO;
    rc = w->failed ? WAV_ERR_IO : WAV_OK;

    if ((w->data_bytes & 1u) && fputc(0, w->f) == EOF)  /* word-align chunk */
        rc = WAV_ERR_IO;

    n = build_header(h, w);
    if (fseek(w->f, 0, SEEK_SET) != 0 || fwrite(h, 1, n, w->f) != n)
        rc = WAV_ERR_IO;
    if (fclose(w->f) != 0)
        rc = WAV_ERR_IO;
    free(w);
    return rc;
}

/* ---- reader ---------------------------------------------------------- */

static int parse_wav(const unsigned char *buf, size_t size, wav_data *out)
{
    const unsigned char *fmt_chunk = NULL, *data_chunk = NULL;
    uint32_t fmt_len = 0, data_len = 0;
    int tag, channels, rate, bits, bps;
    size_t pos, frames, count;

    if (size < 12 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0)
        return WAV_ERR_FORMAT;

    /* Walk the chunks; unknown ones (LIST, cue, ...) are skipped. */
    for (pos = 12; pos + 8 <= size; ) {
        uint32_t len   = get_u32(buf + pos + 4);
        size_t   body  = pos + 8;
        size_t   avail = size - body;
        if (len > avail) len = (uint32_t)avail;   /* truncated file */

        if (memcmp(buf + pos, "fmt ", 4) == 0) {
            fmt_chunk = buf + body;
            fmt_len   = len;
        } else if (memcmp(buf + pos, "data", 4) == 0) {
            data_chunk = buf + body;
            data_len   = len;
        }
        pos = body + len + (len & 1u);
    }
    if (!fmt_chunk || fmt_len < 16 || !data_chunk) return WAV_ERR_FORMAT;

    tag      = get_u16(fmt_chunk);
    channels = get_u16(fmt_chunk + 2);
    rate     = (int)get_u32(fmt_chunk + 4);
    bits     = get_u16(fmt_chunk + 14);
    if (tag == 0xFFFE && fmt_len >= 40)        /* WAVE_FORMAT_EXTENSIBLE:   */
        tag = get_u16(fmt_chunk + 24);         /* real format is in the GUID */
    if (channels <= 0 || rate <= 0) return WAV_ERR_FORMAT;

    if (tag == WAV_PCM16 && bits == 16)        bps = 2;
    else if (tag == WAV_FLOAT32 && bits == 32) bps = 4;
    else return WAV_ERR_UNSUPPORTED;

    frames = data_len / ((size_t)channels * (size_t)bps);
    count  = frames * (size_t)channels;
    out->samples = malloc((count ? count : 1) * sizeof(float));
    if (!out->samples) return WAV_ERR_NOMEM;

    for (size_t i = 0; i < count; i++) {
        const unsigned char *p = data_chunk + i * (size_t)bps;
        if (bps == 2) {
            int v = get_u16(p);
            if (v >= 32768) v -= 65536;          /* two's complement */
            out->samples[i] = (float)(v / 32768.0);
        } else {
            uint32_t u = get_u32(p);
            float x;
            memcpy(&x, &u, sizeof x);
            out->samples[i] = x;
        }
    }
    out->sample_rate = rate;
    out->channels    = channels;
    out->format      = tag;
    out->frames      = frames;
    return WAV_OK;
}

int wav_read(const char *path, wav_data *out)
{
    FILE *f;
    long size;
    unsigned char *buf;
    int rc;

    if (!out) return WAV_ERR_IO;
    memset(out, 0, sizeof *out);
    if (!path) return WAV_ERR_IO;

    f = fopen(path, "rb");
    if (!f) return WAV_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return WAV_ERR_IO;
    }
    buf = malloc(size > 0 ? (size_t)size : 1);
    if (!buf) {
        fclose(f);
        return WAV_ERR_NOMEM;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(buf);
        return WAV_ERR_IO;
    }
    fclose(f);

    rc = parse_wav(buf, (size_t)size, out);
    free(buf);
    if (rc != WAV_OK) wav_free(out);
    return rc;
}

void wav_free(wav_data *d)
{
    if (!d) return;
    free(d->samples);
    memset(d, 0, sizeof *d);
}
