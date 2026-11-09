/*
 * sim_build.c - making an engine from a build description (stage 123): what the
 * first engine and every prepare worker do, off the render thread. It makes
 * the sim the way rb_cli sound does, in the same order, so a drive through
 * the host renders what rb_cli renders.
 */
#include <stdio.h>
#include <string.h>

#include "host_internal.h"

static atomic_int live_sims;

void rb_host_build_default(rb_host_build *b)
{
    memset(b, 0, sizeof *b);
    b->preset = 0;
    b->quality = RB_QUALITY_HIGH;
    b->scene = -1;
    rb_host_build_param(b, RB_PARAM_SOURCE, RB_SOURCE_ENGINE);
}

int rb_host_build_edit(rb_host_build *b, int field, int index, double value)
{
    if (!b || b->n_edits >= RB_HOST_MAX_EDITS) return RB_HOST_ERR_FULL;
    b->edits[b->n_edits].field = field;
    b->edits[b->n_edits].index = index;
    b->edits[b->n_edits].value = value;
    b->n_edits++;
    return RB_HOST_OK;
}

int rb_host_build_param(rb_host_build *b, int id, double value)
{
    if (!b || b->n_params >= RB_HOST_MAX_PARAMS) return RB_HOST_ERR_FULL;
    b->params[b->n_params].id = id;
    b->params[b->n_params].value = value;
    b->n_params++;
    return RB_HOST_OK;
}

/* Says why a build failed. */
static rb_sim *fail(rb_sim *sim, int *code, int value, char *why, size_t why_size, const char *format, int a)
{
    if (code) *code = value;
    if (why && why_size) snprintf(why, why_size, format, a);
    rbh_sim_destroy(sim);
    return NULL;
}

rb_sim *rbh_build_sim(const rb_host_build *spec, double rate, int *code, char *why, size_t why_size)
{
    if (code) *code = 0;
    if (why && why_size) why[0] = '\0';
    if (!spec || spec->n_edits < 0 || spec->n_edits > RB_HOST_MAX_EDITS || spec->n_params < 0
        || spec->n_params > RB_HOST_MAX_PARAMS)
        return fail(NULL, code, RB_ERR_ARG, why, why_size, "a build with %d edits or parameters out of range", -1);

    rb_config cfg;
    rb_config_default(&cfg);
    cfg.sample_rate = rate;
    if (spec->quality < 0 && spec->oversample > 0) cfg.oversample = spec->oversample;
    if (spec->quality >= 0 && rb_config_quality(&cfg, spec->quality) != RB_OK)
        return fail(NULL, code, RB_ERR_ARG, why, why_size, "no quality %d at this rate", spec->quality);
    rb_sim *sim = rb_create(&cfg);
    if (!sim) return fail(NULL, code, RB_ERR_CONFIG, why, why_size, "the sim could not be created (%d)", 0);
    atomic_fetch_add(&live_sims, 1);

    int rc = rb_load_preset(sim, spec->preset);
    if (rc != RB_OK) return fail(sim, code, rc, why, why_size, "no preset %d", spec->preset);
    for (int i = 0; i < spec->n_edits; i++) {
        rc = rb_set_engine(sim, spec->edits[i].field, spec->edits[i].index, spec->edits[i].value);
        if (rc < 0) return fail(sim, code, rc, why, why_size, "edit %d was refused", i);
    }
    for (int i = 0; i < spec->n_params; i++) {
        rc = rb_set_param(sim, spec->params[i].id, spec->params[i].value);
        if (rc < 0) return fail(sim, code, rc, why, why_size, "parameter %d was refused", i);
    }
    if (spec->scene >= 0) {
        rc = rb_set_scene(sim, spec->scene);
        if (rc != RB_OK) return fail(sim, code, rc, why, why_size, "scene %d was refused", spec->scene);
        if (spec->scene_time != 0.0) rb_set_scene_time(sim, spec->scene_time);
    }
    if (spec->settle_blocks > 0) {
        float buf[2 * RB_HOST_BLOCK_FRAMES];
        for (int i = 0; i < spec->settle_blocks; i++) {
            if (spec->scene >= 0) rb_render_stereo(sim, buf, RB_HOST_BLOCK_FRAMES);
            else rb_render_block(sim, buf, RB_HOST_BLOCK_FRAMES);
        }
    }
    return sim;
}

void rbh_sim_destroy(rb_sim *sim)
{
    if (!sim) return;
    atomic_fetch_sub(&live_sims, 1);
    rb_destroy(sim);
}

int rbh_live_sims(void)
{
    return atomic_load(&live_sims);
}

int rb_host_debug_live_sims(void)
{
    return rbh_live_sims();
}

uint64_t rbh_engine_tick(rb_sim *sim, void *scratch)
{
    const int size = rb_get_snapshot(sim, RB_SNAPSHOT_VERSION, scratch, RB_HOST_SNAPSHOT_MAX_BYTES);
    if (size < (int)sizeof(rb_snapshot_header)) return 0;
    rb_snapshot_header header;
    memcpy(&header, scratch, sizeof header);
    return header.physics_tick;
}
