/*
 * wav.h - minimal WAV file writer and reader (native tools only).
 */
#ifndef RB_WAV_H
#define RB_WAV_H

#include <stddef.h>

typedef enum wav_format {
    WAV_PCM16   = 1,   /* 16-bit integer PCM, the most compatible  */
    WAV_FLOAT32 = 3    /* 32-bit IEEE float, no quantisation/clip   */
} wav_format;

enum {
    WAV_OK              =  0,
    WAV_ERR_IO          = -1,   /* cannot open/read/write the file    */
    WAV_ERR_FORMAT      = -2,   /* not a valid RIFF/WAVE file         */
    WAV_ERR_UNSUPPORTED = -3,   /* valid WAV, unsupported sample type */
    WAV_ERR_NOMEM       = -4
};

/* ---- writing ---- */
typedef struct wav_writer wav_writer;

/* Returns NULL on failure. */
wav_writer *wav_open(const char *path, int sample_rate, int channels, wav_format fmt);
/* Appends `frames` frames of interleaved samples. NaN/inf are written as
 * 0; PCM16 clamps to [-1, 1]. Returns WAV_OK or WAV_ERR_IO. */
int wav_write(wav_writer *w, const float *interleaved, size_t frames);
/* Finalises the header and closes the file. Always frees `w`. */
int wav_close(wav_writer *w);

/* ---- reading ---- */
typedef struct wav_data {
    int     sample_rate;
    int     channels;
    int     format;      /* WAV_PCM16 or WAV_FLOAT32                    */
    size_t  frames;
    float  *samples;     /* interleaved, frames * channels, malloc'd    */
} wav_data;

/* Reads a 16-bit PCM or 32-bit float WAV. Returns WAV_OK or WAV_ERR_*. */
int  wav_read(const char *path, wav_data *out);
void wav_free(wav_data *d);

#endif /* RB_WAV_H */
