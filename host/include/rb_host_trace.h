/*
 * rb_host_trace.h - scripted command traces for the host's offline mode
 * (stage 123). A trace is a text file: an engine build, and the commands a
 * driver gave it at audio frames. `rb_cli sound --trace FILE` writes the one
 * its drive made; rb_host_render plays one through the host with no device,
 * and tools/host_parity.py checks the two render the same bits.
 *
 *   rbtrace 1
 *   rate 48000
 *   quality -1                  low, medium, high, ultra or -1 (the default)
 *   oversample 4
 *   preset i4                   a preset key
 *   edit header_length 0 0.5    an engine field (name, index, value), in order
 *   param master_gain 0.7       a parameter, in order, before the first block
 *   scene 6                     a stereo scene (rb_set_scene)
 *   scene_time -1.5             rb_set_scene_time at the build
 *   settle 0                    blocks rendered and discarded at the build
 *   skip 3072                   frames rendered but not recorded
 *   frames 48000                frames recorded after the skip
 *   at 1280 set throttle 0.6    a parameter at a frame (multiples of 128 are exact)
 *   at 1280 live idle_rpm 0 900 a live engine field
 *   at 2560 dyno_attach 3000    also dyno_release, dyno_sweep TO RATE, dyno_clear
 *   at 2560 scene_time -1.0
 *   at 4096 swap second         swaps in the build named `second`
 *   build second                another build: its own preset, edit, param, scene
 *   ...                         lines, until `end`
 *   end
 *
 * Numbers are written with 17 digits so that they read back exactly. Lines
 * starting with # are comments.
 */
#ifndef RB_HOST_TRACE_H
#define RB_HOST_TRACE_H

#include <stddef.h>

#include "rb_host.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rb_trace rb_trace;

/* Parses a trace; NULL, with "line N: why" in `error`, if it isn't one. */
rb_trace *rb_trace_parse(const char *text, char *error, size_t error_size);
rb_trace *rb_trace_load(const char *path, char *error, size_t error_size);
void      rb_trace_free(rb_trace *t);

/* What a trace is. */
int       rb_trace_rate(const rb_trace *t);
long long rb_trace_skip_frames(const rb_trace *t);
long long rb_trace_frames(const rb_trace *t);
int       rb_trace_command_count(const rb_trace *t);
int       rb_trace_build_count(const rb_trace *t);
/* 2 if any build has a stereo scene (the output has two channels), else 1. */
int       rb_trace_channels(const rb_trace *t);

/* Receives the recorded frames, a block at a time (channels 1: mono samples,
 * 2: interleaved stereo). Returns 0, or non-zero to stop. */
typedef int (*rb_trace_sink)(void *user, const float *samples, int frames, int channels);

/* Renders the trace through a host in the offline mode: the skip, then the
 * recorded frames into the sink. 0, or a negative RB_HOST_ERR_* with `error`
 * saying what failed. */
int rb_trace_play(const rb_trace *t, rb_trace_sink sink, void *user, char *error, size_t error_size);

#ifdef __cplusplus
}
#endif

#endif /* RB_HOST_TRACE_H */
