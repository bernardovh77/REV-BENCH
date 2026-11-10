/*
 * host_render.c - rb_host_render (stage 123): plays a command trace through the
 * host's offline mode, with no device and no thread beyond the preparing
 * workers, and writes the sound to a WAV file.
 *
 *   rb_host_render TRACE.rbtrace -o OUT.wav [--float]
 *
 * `rb_cli sound --trace FILE` writes the trace of its own drive; the two
 * renders are the same bit for bit when built by the same compiler
 * (tools/host_parity.py).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rb_host_trace.h"
#include "wav.h"

typedef struct wav_sink {
    wav_writer *wav;
    long long   frames;
    int         failed;
} wav_sink;

static int to_wav(void *user, const float *samples, int frames, int channels)
{
    (void)channels;
    wav_sink *s = (wav_sink *)user;
    if (wav_write(s->wav, samples, (size_t)frames) != WAV_OK) {
        s->failed = 1;
        return 1;
    }
    s->frames += frames;
    return 0;
}

int main(int argc, char **argv)
{
    const char *trace_path = NULL, *out_path = "renders/host.wav";
    int use_float = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--out")) {
            if (++i >= argc) {
                fprintf(stderr, "rb_host_render: -o needs a path\n");
                return 2;
            }
            out_path = argv[i];
        } else if (!strcmp(argv[i], "--float")) {
            use_float = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "rb_host_render: unknown option '%s'\n", argv[i]);
            return 2;
        } else {
            trace_path = argv[i];
        }
    }
    if (!trace_path) {
        fprintf(stderr, "usage: rb_host_render TRACE.rbtrace -o OUT.wav [--float]\n");
        return 2;
    }
    char error[256];
    rb_trace *trace = rb_trace_load(trace_path, error, sizeof error);
    if (!trace) {
        fprintf(stderr, "rb_host_render: %s\n", error);
        return 2;
    }
    /* read before the trace is freed: the summary below still needs them */
    const int channels = rb_trace_channels(trace);
    const int rate = rb_trace_rate(trace);
    wav_sink sink = { wav_open(out_path, rate, channels, use_float ? WAV_FLOAT32 : WAV_PCM16), 0, 0 };
    if (!sink.wav) {
        fprintf(stderr, "rb_host_render: cannot write %s (does the folder exist?)\n", out_path);
        rb_trace_free(trace);
        return 1;
    }
    const int rc = rb_trace_play(trace, to_wav, &sink, error, sizeof error);
    const int closed = wav_close(sink.wav);
    rb_trace_free(trace);
    if (rc != RB_HOST_OK) {
        fprintf(stderr, "rb_host_render: %s\n", error);
        return 1;
    }
    if (closed != WAV_OK || sink.failed) {
        fprintf(stderr, "rb_host_render: write error\n");
        return 1;
    }
    printf("rendered %lld frames at %d Hz (%d channel%s) through the host's offline mode\n", sink.frames,
           rate, channels, channels > 1 ? "s" : "");
    printf("wrote %s (%s)\n", out_path, use_float ? "32-bit float" : "16-bit PCM, dithered");
    return 0;
}
