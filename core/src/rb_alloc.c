/*
 * rb_alloc.c - every malloc/free of the core lives here.
 *
 * The simulation itself never allocates. These wrappers exist for hosts:
 * rb_create() for native code, rb_create_with() and the float buffers for
 * JavaScript, which can't build C structs or call malloc on its own.
 * Hosts that must not use malloc can call rb_sizeof() + rb_init() on
 * their own memory instead.
 */
#include "rb_internal.h"

#include <stdlib.h>

rb_sim *rb_create(const rb_config *cfg)
{
    rb_sim *sim = malloc(sizeof *sim);
    if (!sim) return NULL;
    if (rb_init(sim, cfg) != RB_OK) {
        free(sim);
        return NULL;
    }
    return sim;
}

rb_sim *rb_create_with(double sample_rate, int oversample)
{
    rb_config cfg;
    cfg.sample_rate = sample_rate;
    cfg.oversample  = oversample;
    return rb_create(&cfg);
}

void rb_destroy(rb_sim *sim)
{
    free(sim);
}

float *rb_alloc_floats(int count)
{
    if (count <= 0) return NULL;
    return calloc((size_t)count, sizeof(float));
}

void rb_free_floats(float *buf)
{
    free(buf);
}
