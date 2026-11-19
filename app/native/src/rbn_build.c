/*
 * rbn_build.c - reading addon.ts's BuildSpec and HostConfig (stage 124) into
 * the host's rb_host_build and rb_host_config.
 *
 * Names are looked up here (a preset's key, a quality, a scene); the numbers
 * in edits and params are the core's ids, checked by the core when the engine
 * is built, so a bad one fails the build and not this reading. A wrong type
 * or a name nobody knows is an error at once.
 */
#include <string.h>

#include "rbn.h"

/* A generous ceiling for the blocks a build may settle. */
#define RBN_MAX_SETTLE 1000000

/* The property `key` as a string, if it is there: *present says so. */
static int optional_name(napi_env env, napi_value object, const char *key, char *out, size_t capacity, int *present)
{
    napi_value v;
    if (rbn_get_prop(env, object, key, &v, present) != 0) return -1;
    if (!*present) return 0;
    return rbn_get_string(env, v, key, out, capacity);
}

/* One {field, index, value}. */
static int read_edit(napi_env env, napi_value item, rb_host_build *out)
{
    if (rbn_expect_object(env, item, "an edit") != 0) return -1;
    napi_value v;
    int present;
    int64_t field = 0, index = 0;
    double value = 0.0;
    if (rbn_get_prop(env, item, "field", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "an edit needs a field"), -1;
    if (rbn_get_int(env, v, "edit.field", 0, RBN_MAX_ID_VALUE, &field) != 0) return -1;
    if (rbn_get_prop(env, item, "index", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "an edit needs an index"), -1;
    if (rbn_get_int(env, v, "edit.index", 0, RBN_MAX_ID_VALUE, &index) != 0) return -1;
    if (rbn_get_prop(env, item, "value", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "an edit needs a value"), -1;
    if (rbn_get_double(env, v, "edit.value", &value) != 0) return -1;
    const int rc = rb_host_build_edit(out, (int)field, (int)index, value);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "adding an edit"), -1;
    return 0;
}

/* A layout {family, cylinders, crank, bankAngle} (stage 131), checked by the core. */
static int read_layout(napi_env env, napi_value layout, rb_host_build *out)
{
    if (rbn_expect_object(env, layout, "layout") != 0) return -1;
    napi_value v;
    int present;
    char text[32];
    int64_t cylinders = 0;
    double angle = 0.0;
    if (rbn_get_prop(env, layout, "family", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "a layout needs a family"), -1;
    if (rbn_get_string(env, v, "layout.family", text, sizeof text) != 0) return -1;
    const int family = rbn_name_value(RBN_FAMILIES, text);
    if (family < 0) return rbn_throw(env, "RB_ERR_ID", "no family \"%s\"", text), -1;
    if (rbn_get_prop(env, layout, "cylinders", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "a layout needs cylinders"), -1;
    if (rbn_get_int(env, v, "layout.cylinders", 0, RBN_MAX_ID_VALUE, &cylinders) != 0) return -1;
    if (rbn_get_prop(env, layout, "crank", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "a layout needs a crank"), -1;
    if (rbn_get_string(env, v, "layout.crank", text, sizeof text) != 0) return -1;
    const int crank = rbn_name_value(RBN_CRANKS, text);
    if (crank < 0) return rbn_throw(env, "RB_ERR_ID", "no crank \"%s\"", text), -1;
    if (rbn_get_prop(env, layout, "bankAngle", &v, &present) != 0) return -1;
    if (present && rbn_get_double(env, v, "layout.bankAngle", &angle) != 0) return -1;
    const char *why = rb_layout_why(family, (int)cylinders, crank, angle);
    if (why) return rbn_throw(env, "RB_ERR_VALUE", "that layout isn't built: %s", why), -1;
    out->layout_family = family;
    out->layout_cylinders = (int32_t)cylinders;
    out->layout_crank = crank;
    out->layout_bank_angle = angle;
    return 0;
}

/* One {id, value}. */
static int read_param(napi_env env, napi_value item, rb_host_build *out)
{
    if (rbn_expect_object(env, item, "a param") != 0) return -1;
    napi_value v;
    int present;
    int64_t id = 0;
    double value = 0.0;
    if (rbn_get_prop(env, item, "id", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "a param needs an id"), -1;
    if (rbn_get_int(env, v, "param.id", 0, RBN_MAX_ID_VALUE, &id) != 0) return -1;
    if (rbn_get_prop(env, item, "value", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "a param needs a value"), -1;
    if (rbn_get_double(env, v, "param.value", &value) != 0) return -1;
    const int rc = rb_host_build_param(out, (int)id, value);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "adding a parameter"), -1;
    return 0;
}

/* Reads the array under `key` (if there), one element at a time. */
static int read_list(napi_env env, napi_value spec, const char *key, rb_host_build *out,
                     int (*one)(napi_env, napi_value, rb_host_build *))
{
    napi_value list;
    int present;
    if (rbn_get_prop(env, spec, key, &list, &present) != 0) return -1;
    if (!present) return 0;
    bool is_array = false;
    RBN_TRY_INT(napi_is_array(env, list, &is_array));
    if (!is_array) return rbn_throw(env, "RB_HOST_ERR_ARG", "%s must be an array", key), -1;
    uint32_t length = 0;
    RBN_TRY_INT(napi_get_array_length(env, list, &length));
    for (uint32_t i = 0; i < length; i++) {
        napi_value item;
        RBN_TRY_INT(napi_get_element(env, list, i, &item));
        if (one(env, item, out) != 0) return -1;
    }
    return 0;
}

int rbn_parse_build(napi_env env, napi_value spec, const char *what, rb_host_build *out)
{
    if (rbn_expect_object(env, spec, what) != 0) return -1;
    rb_host_build_default(out);

    napi_value v;
    int present;
    char text[64];

    /* the preset, by key */
    if (rbn_get_prop(env, spec, "preset", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "%s.preset is required", what), -1;
    if (rbn_get_string(env, v, "preset", text, sizeof text) != 0) return -1;
    const int preset = rb_preset_find(text);
    if (preset < 0) return rbn_throw(env, "RB_ERR_ID", "no preset \"%s\"", text), -1;
    out->preset = preset;

    /* the layout (stage 131): {family, cylinders, crank, bankAngle}, or the preset's own */
    if (rbn_get_prop(env, spec, "layout", &v, &present) != 0) return -1;
    if (present && read_layout(env, v, out) != 0) return -1;

    /* the quality: the preset's (or the layout's) real-time default unless given */
    out->quality = out->layout_family >= 0
                 ? rb_layout_quality(out->layout_family, out->layout_cylinders, out->layout_crank)
                 : rb_preset_quality(preset);
    if (optional_name(env, spec, "quality", text, sizeof text, &present) != 0) return -1;
    if (present) {
        const int quality = rbn_name_value(RBN_QUALITIES, text);
        if (quality < 0) return rbn_throw(env, "RB_ERR_ID", "no quality \"%s\"", text), -1;
        out->quality = quality;
    }

    /* the stereo scene, or the mono listener */
    if (optional_name(env, spec, "scene", text, sizeof text, &present) != 0) return -1;
    if (present) {
        const int scene = rbn_name_value(RBN_SCENES, text);
        if (scene < 0) return rbn_throw(env, "RB_ERR_ID", "no scene \"%s\"", text), -1;
        out->scene = scene;
    }

    if (rbn_get_prop(env, spec, "settleBlocks", &v, &present) != 0) return -1;
    if (present) {
        int64_t blocks = 0;
        if (rbn_get_int(env, v, "settleBlocks", 0, RBN_MAX_SETTLE, &blocks) != 0) return -1;
        out->settle_blocks = (int32_t)blocks;
    }

    if (read_list(env, spec, "edits", out, read_edit) != 0) return -1;
    if (read_list(env, spec, "params", out, read_param) != 0) return -1;
    return 0;
}

int rbn_parse_config(napi_env env, napi_value config, rb_host_config *out)
{
    if (rbn_expect_object(env, config, "config") != 0) return -1;
    rb_host_config_default(out);

    napi_value v;
    int present;
    char text[32];

    if (optional_name(env, config, "backend", text, sizeof text, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "config.backend is required"), -1;
    const int backend = rbn_name_value(RBN_BACKENDS, text);
    if (backend < 0) return rbn_throw(env, "RB_HOST_ERR_ARG", "backend must be \"null\" or \"default\", not \"%s\"", text), -1;
    out->backend = backend;

    /* -1 or absent: the system's default */
    if (rbn_get_prop(env, config, "device", &v, &present) != 0) return -1;
    if (present) {
        int64_t device = 0;
        if (rbn_get_int(env, v, "device", -1, RBN_MAX_ID_VALUE, &device) != 0) return -1;
        out->device = (int32_t)device;
    }

    if (rbn_get_prop(env, config, "sampleRate", &v, &present) != 0) return -1;
    if (present) {
        int64_t rate = 0;
        if (rbn_get_int(env, v, "sampleRate", 44100, 48000, &rate) != 0) return -1;
        if (rate != 44100 && rate != 48000)
            return rbn_throw(env, "RB_HOST_ERR_ARG", "sampleRate must be 44100 or 48000"), -1;
        out->sample_rate = (int32_t)rate;
    }

    if (rbn_get_prop(env, config, "targetFillMs", &v, &present) != 0) return -1;
    if (present) {
        int64_t ms = 0;
        if (rbn_get_int(env, v, "targetFillMs", 10, 80, &ms) != 0) return -1;
        out->target_fill_ms = (int32_t)ms;
    }

    /* the capture tap (stage 066): its ring, in frames; rb_host_create checks it is a power of two */
    if (rbn_get_prop(env, config, "captureFrames", &v, &present) != 0) return -1;
    if (present) {
        int64_t frames = 0;
        if (rbn_get_int(env, v, "captureFrames", 0, 1 << 24, &frames) != 0) return -1;
        out->capture_frames = (int32_t)frames;
    }

    if (rbn_get_prop(env, config, "initial", &v, &present) != 0) return -1;
    if (!present) return rbn_throw(env, "RB_HOST_ERR_ARG", "config.initial is required"), -1;
    return rbn_parse_build(env, v, "initial", &out->initial);
}
