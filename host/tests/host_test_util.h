/*
 * host_test_util.h - what the host's tests share (stage 123): building hosts
 * with a cheap source or a real engine, and pumping the device's side faster
 * than real time without ever starving the ring (so a test's data is exact
 * and has no underruns unless the test makes one).
 */
#ifndef HOST_TEST_UTIL_H
#define HOST_TEST_UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "../include/rb_host.h"
#include "../src/host_internal.h"

/* A build whose source is the core's test tone (cheap, steady, loud). */
void ht_tone_build(rb_host_build *b, double hz);
/* A build of a preset as an engine held at `rpm` with the key on and `throttle`. */
void ht_engine_build(rb_host_build *b, int preset, double rpm, double throttle);

/* A host on `backend` with `initial` at `rate`, not started; NULL (and a failed
 * check) if it can't be made. */
rb_host *ht_make(int backend, const rb_host_build *initial, int rate);

/* A growing record of the stereo frames a pump pulled, and the acks it took. */
typedef struct ht_capture {
    float       *left;              /* the left channel, `frames` samples          */
    size_t       frames, capacity;
    uint64_t     first_frame;       /* the host's audio frame of left[0]           */
    rb_host_ack  acks[512];
    int          n_acks;
} ht_capture;

void ht_capture_init(ht_capture *c, size_t capacity);
void ht_capture_free(ht_capture *c);

/* Pulls `frames` frames from the host in blocks of 128 whenever the ring has
 * them, recording the left channel into `c` (may be NULL), and polling acks into
 * c->acks. Gives up after timeout_ms: returns the frames pulled. */
size_t ht_pump(rb_host *h, size_t frames, ht_capture *c, unsigned timeout_ms);
/* The same without taking the acks or freeing retired engines (for tests of what a
 * control thread that doesn't poll does to the render thread). */
size_t ht_pump_raw(rb_host *h, size_t frames, unsigned timeout_ms);
/* Pumps until an ack for `request_id` arrives (in c->acks) or timeout: its index in
 * c->acks, or -1. */
int ht_pump_until_ack(rb_host *h, uint64_t request_id, ht_capture *c, unsigned timeout_ms);

/* Waits for the ring to fill to its target. */
void ht_wait_filled(rb_host *h, unsigned timeout_ms);

/* RMS of left[from..to). */
double ht_rms(const float *left, size_t from, size_t to);
/* The largest sample-to-sample step in left[from..to). */
double ht_max_step(const float *left, size_t from, size_t to);

#endif /* HOST_TEST_UTIL_H */
