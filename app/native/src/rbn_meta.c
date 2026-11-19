/*
 * rbn_meta.c - what the engine process asks for before it has a host (stage
 * 124): the versions, the presets, the parameter and telemetry metadata, and
 * the playback devices. All of it comes from the core's and the host's own
 * tables (rb_preset_*, rb_param_*, rb_telemetry_*, rb_host_list_devices): the
 * add-on keeps no copy.
 */
#include <string.h>

#include "rbn.h"

/* A device list holds this many entries at most (a machine has a handful). */
#define RBN_MAX_DEVICES 64

napi_value rbn_version(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result;
    RBN_TRY(napi_create_object(env, &result));
    /* the add-on's version is the core's, which it is built from */
    if (rbn_set_string(env, result, "addon", rb_version()) != 0) return NULL;
    if (rbn_set_number(env, result, "snapshot", RB_SNAPSHOT_VERSION) != 0) return NULL;
    /* the highest Node-API version the runtime has */
    uint32_t node_api = 0;
    RBN_TRY(napi_get_version(env, &node_api));
    if (rbn_set_number(env, result, "nodeApi", (double)node_api) != 0) return NULL;
    return result;
}

/* A layout as {family, cylinders, crank, bankAngle} (stage 131), or NULL after a throw. */
static napi_value layout_object(napi_env env, int family, int cylinders, int crank, double angle)
{
    const char *family_name = rbn_name_of(RBN_FAMILIES, family), *crank_name = rbn_name_of(RBN_CRANKS, crank);
    if (!family_name || !crank_name) return rbn_throw_core(env, RB_ERR_ID, "reading a layout"), NULL;
    napi_value item;
    RBN_TRY(napi_create_object(env, &item));
    if (rbn_set_string(env, item, "family", family_name) != 0 || rbn_set_number(env, item, "cylinders", cylinders) != 0 ||
        rbn_set_string(env, item, "crank", crank_name) != 0 || rbn_set_number(env, item, "bankAngle", angle) != 0)
        return NULL;
    return item;
}

napi_value rbn_presets(napi_env env, napi_callback_info info)
{
    (void)info;
    const int count = rb_preset_count();
    napi_value list;
    RBN_TRY(napi_create_array_with_length(env, (size_t)count, &list));
    for (int i = 0; i < count; i++) {
        const char *key = rb_preset_key(i);
        const char *name = rb_preset_name(i);
        const int quality = rb_preset_quality(i);
        const char *quality_name = rbn_name_of(RBN_QUALITIES, quality);
        if (!key || !name) return rbn_throw_core(env, RB_ERR_ID, "reading a preset");
        if (!quality_name) return rbn_throw_core(env, quality < 0 ? quality : RB_ERR_ID, "reading a preset's quality");
        napi_value item;
        RBN_TRY(napi_create_object(env, &item));
        if (rbn_set_number(env, item, "index", i) != 0 || rbn_set_string(env, item, "key", key) != 0 ||
            rbn_set_string(env, item, "name", name) != 0 || rbn_set_string(env, item, "quality", quality_name) != 0)
            return NULL;
        napi_value layout = layout_object(env, (int)rb_preset_layout(i, RB_LAYOUT_FAMILY), (int)rb_preset_layout(i, RB_LAYOUT_CYLINDERS),
                                          (int)rb_preset_layout(i, RB_LAYOUT_CRANK), rb_preset_layout(i, RB_LAYOUT_BANK_ANGLE));
        if (!layout) return NULL;
        RBN_TRY(napi_set_named_property(env, item, "layout", layout));
        RBN_TRY(napi_set_element(env, list, (uint32_t)i, item));
    }
    return list;
}

/* The flags as words: only the ones addon.ts names ("live" is every
 * parameter's default, and says nothing). */
static napi_value flag_words(napi_env env, int flags)
{
    napi_value list;
    RBN_TRY(napi_create_array(env, &list));
    uint32_t n = 0;
    static const struct {
        int         bit;
        const char *word;
    } WORDS[] = {{RB_PF_CONTROL, "control"}, {RB_PF_REBUILD, "rebuild"}, {RB_PF_INTEGER, "integer"}};
    for (size_t i = 0; i < sizeof WORDS / sizeof WORDS[0]; i++) {
        if (!(flags & WORDS[i].bit)) continue;
        napi_value word;
        RBN_TRY(napi_create_string_utf8(env, WORDS[i].word, NAPI_AUTO_LENGTH, &word));
        RBN_TRY(napi_set_element(env, list, n++, word));
    }
    return list;
}

napi_value rbn_params(napi_env env, napi_callback_info info)
{
    (void)info;
    const int count = rb_param_count();
    napi_value list;
    RBN_TRY(napi_create_array_with_length(env, (size_t)count, &list));
    for (int id = 0; id < count; id++) {
        const char *name = rb_param_name(id);
        const char *unit = rb_param_unit(id);
        if (!name) return rbn_throw_core(env, RB_ERR_ID, "reading a parameter");
        napi_value item;
        RBN_TRY(napi_create_object(env, &item));
        if (rbn_set_number(env, item, "id", id) != 0 || rbn_set_string(env, item, "name", name) != 0 ||
            rbn_set_string(env, item, "unit", unit) != 0 || rbn_set_number(env, item, "min", rb_param_min(id)) != 0 ||
            rbn_set_number(env, item, "max", rb_param_max(id)) != 0 ||
            rbn_set_number(env, item, "default", rb_param_default(id)) != 0)
            return NULL;
        napi_value flags = flag_words(env, rb_param_flags(id));
        if (!flags || rbn_set_value(env, item, "flags", flags) != 0) return NULL;
        RBN_TRY(napi_set_element(env, list, (uint32_t)id, item));
    }
    return list;
}

napi_value rbn_telemetry(napi_env env, napi_callback_info info)
{
    (void)info;
    const int count = rb_telemetry_count();
    napi_value list;
    RBN_TRY(napi_create_array_with_length(env, (size_t)count, &list));
    for (int id = 0; id < count; id++) {
        const char *name = rb_telemetry_name(id);
        const char *unit = rb_telemetry_unit(id);
        if (!name) return rbn_throw_core(env, RB_ERR_ID, "reading a telemetry id");
        napi_value item;
        RBN_TRY(napi_create_object(env, &item));
        if (rbn_set_number(env, item, "id", id) != 0 || rbn_set_string(env, item, "name", name) != 0 ||
            rbn_set_string(env, item, "unit", unit) != 0)
            return NULL;
        RBN_TRY(napi_set_element(env, list, (uint32_t)id, item));
    }
    return list;
}

napi_value rbn_list_devices(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    if (rbn_get_args(env, info, 1, args, &given, NULL) != 0) return NULL;
    if (given < 1) return rbn_throw(env, "RB_HOST_ERR_ARG", "listDevices needs a backend");
    char name[32];
    if (rbn_get_string(env, args[0], "backend", name, sizeof name) != 0) return NULL;
    const int backend = rbn_name_value(RBN_BACKENDS, name);
    if (backend < 0) return rbn_throw(env, "RB_HOST_ERR_ARG", "backend must be \"null\" or \"default\", not \"%s\"", name);

    rb_host_device_info devices[RBN_MAX_DEVICES];
    int found = rb_host_list_devices(backend, devices, RBN_MAX_DEVICES);
    if (found < 0) return rbn_throw_host(env, found, "listing the devices");
    if (found > RBN_MAX_DEVICES) found = RBN_MAX_DEVICES;

    napi_value list;
    RBN_TRY(napi_create_array_with_length(env, (size_t)found, &list));
    for (int i = 0; i < found; i++) {
        /* the rate the host renders at on this device (rb_host_pick_rate) */
        const int32_t native = devices[i].native_rate;
        const int rate = native > 0 ? rb_host_pick_rate(&native, 1) : rb_host_pick_rate(NULL, 0);
        napi_value item;
        RBN_TRY(napi_create_object(env, &item));
        if (rbn_set_number(env, item, "index", devices[i].index) != 0 ||
            rbn_set_string(env, item, "name", devices[i].name) != 0 ||
            rbn_set_bool(env, item, "isDefault", devices[i].is_default) != 0 ||
            rbn_set_number(env, item, "rate", rate) != 0)
            return NULL;
        if (native > 0) {
            if (rbn_set_number(env, item, "nativeRate", native) != 0) return NULL;
        } else if (rbn_set_null(env, item, "nativeRate") != 0) {
            return NULL;
        }
        RBN_TRY(napi_set_element(env, list, (uint32_t)i, item));
    }
    return list;
}

/* ---- the engine's description (stage 067) ------------------------------------------- */

/* An engine field's flags as words (addon.ts's EngineFieldInfo.flags). */
static napi_value engine_flag_words(napi_env env, int flags)
{
    napi_value list;
    RBN_TRY(napi_create_array(env, &list));
    uint32_t n = 0;
    static const struct {
        int         bit;
        const char *word;
    } WORDS[] = {{RB_EF_READONLY, "readonly"},       {RB_EF_INTEGER, "integer"},
                 {RB_EF_PER_BANK, "per-bank"},       {RB_EF_PER_CYLINDER, "per-cylinder"},
                 {RB_EF_IN_ORDER, "in-order"},       {RB_EF_LIVE, "live"}};
    for (size_t i = 0; i < sizeof WORDS / sizeof WORDS[0]; i++) {
        if (!(flags & WORDS[i].bit)) continue;
        napi_value word;
        RBN_TRY(napi_create_string_utf8(env, WORDS[i].word, NAPI_AUTO_LENGTH, &word));
        RBN_TRY(napi_set_element(env, list, n++, word));
    }
    return list;
}

/* engineFields(): every field the core describes an engine by, with its unit,
 * range (null for a read-only one) and flags, in id order. */
napi_value rbn_engine_fields(napi_env env, napi_callback_info info)
{
    (void)info;
    const int count = rb_engine_field_count();
    napi_value list;
    RBN_TRY(napi_create_array_with_length(env, (size_t)count, &list));
    for (int id = 0; id < count; id++) {
        const char *name = rb_engine_field_name(id);
        if (!name) return rbn_throw_core(env, RB_ERR_ID, "reading an engine field");
        napi_value item;
        RBN_TRY(napi_create_object(env, &item));
        if (rbn_set_number(env, item, "id", id) != 0 || rbn_set_string(env, item, "name", name) != 0 ||
            rbn_set_string(env, item, "unit", rb_engine_field_unit(id)) != 0)
            return NULL;
        const double lo = rb_engine_field_min(id), hi = rb_engine_field_max(id);
        if (lo == lo && hi == hi) {   /* not NaN: the field can be set */
            if (rbn_set_number(env, item, "min", lo) != 0 || rbn_set_number(env, item, "max", hi) != 0) return NULL;
        } else if (rbn_set_null(env, item, "min") != 0 || rbn_set_null(env, item, "max") != 0) {
            return NULL;
        }
        napi_value flags = engine_flag_words(env, rb_engine_field_flags(id));
        if (!flags || rbn_set_value(env, item, "flags", flags) != 0) return NULL;
        RBN_TRY(napi_set_element(env, list, (uint32_t)id, item));
    }
    return list;
}

/* describe(spec): the engine a build makes, built as the host's prepare builds
 * it (rb_host_engine_build) but never played: every field's values, by id, as
 * [[v0, v1, ...], ...]. A build that can't be made throws, with the core's code
 * and why, and nothing anywhere has changed. */
napi_value rbn_describe(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    if (rbn_get_args(env, info, 1, args, &given, NULL) != 0) return NULL;
    if (given < 1) return rbn_throw(env, "RB_HOST_ERR_ARG", "describe needs a build");
    rb_host_build spec;
    if (rbn_parse_build(env, args[0], "spec", &spec) != 0) return NULL;
    int code = 0;
    char why[160];
    rb_sim *sim = rb_host_engine_build(&spec, 48000, &code, why, sizeof why);
    if (!sim) return rbn_throw(env, rbn_core_code_name(code), "the engine can't be built: %s", why);
    const int count = rb_engine_field_count();
    napi_value list;
    if (napi_create_array_with_length(env, (size_t)count, &list) != napi_ok) {
        rb_host_engine_free(sim);
        return NULL;
    }
    for (int id = 0; id < count; id++) {
        const int size = rb_engine_field_size(sim, id);
        napi_value values;
        if (napi_create_array_with_length(env, (size_t)(size > 0 ? size : 0), &values) != napi_ok) break;
        for (int k = 0; k < size; k++) {
            napi_value v = rbn_number(env, rb_get_engine(sim, id, k));
            if (!v || napi_set_element(env, values, (uint32_t)k, v) != napi_ok) break;
        }
        if (napi_set_element(env, list, (uint32_t)id, values) != napi_ok) break;
    }
    rb_host_engine_free(sim);
    return list;
}

/* layouts() (stage 131): what the core builds, a family and count at a time:
 * [{family, familyName, cylinders, usualCrank, cranks: [{crank, name, angleMin,
 * angleMax, angleUsual, quality}]}], from rb_layout_* (nothing listed here). */
napi_value rbn_layouts(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value list;
    RBN_TRY(napi_create_array(env, &list));
    uint32_t n = 0;
    for (int family = 0; family < RB_FAMILY_COUNT; family++) {
        for (int cylinders = 1; cylinders <= 16; cylinders++) {
            const int usual = rb_layout_usual_crank(family, cylinders);
            if (usual < 0) continue;
            napi_value item, cranks;
            RBN_TRY(napi_create_object(env, &item));
            RBN_TRY(napi_create_array(env, &cranks));
            uint32_t k = 0;
            for (int crank = 0; crank < RB_CRANK_COUNT; crank++) {
                const double usual_angle = rb_layout_angle(family, cylinders, crank, RB_ANGLE_USUAL);
                if (usual_angle != usual_angle) continue;   /* NaN: not this count's */
                const char *quality = rbn_name_of(RBN_QUALITIES, rb_layout_quality(family, cylinders, crank));
                napi_value c;
                RBN_TRY(napi_create_object(env, &c));
                if (rbn_set_string(env, c, "crank", rbn_name_of(RBN_CRANKS, crank)) != 0 ||
                    rbn_set_string(env, c, "name", rb_crank_name(crank)) != 0 ||
                    rbn_set_number(env, c, "angleMin", rb_layout_angle(family, cylinders, crank, RB_ANGLE_MIN)) != 0 ||
                    rbn_set_number(env, c, "angleMax", rb_layout_angle(family, cylinders, crank, RB_ANGLE_MAX)) != 0 ||
                    rbn_set_number(env, c, "angleUsual", usual_angle) != 0 ||
                    rbn_set_string(env, c, "quality", quality ? quality : "low") != 0)
                    return NULL;
                RBN_TRY(napi_set_element(env, cranks, k++, c));
            }
            if (rbn_set_string(env, item, "family", rbn_name_of(RBN_FAMILIES, family)) != 0 ||
                rbn_set_string(env, item, "familyName", rb_family_name(family)) != 0 ||
                rbn_set_number(env, item, "cylinders", cylinders) != 0 ||
                rbn_set_string(env, item, "usualCrank", rbn_name_of(RBN_CRANKS, usual)) != 0)
                return NULL;
            RBN_TRY(napi_set_named_property(env, item, "cranks", cranks));
            RBN_TRY(napi_set_element(env, list, n++, item));
        }
    }
    return list;
}
