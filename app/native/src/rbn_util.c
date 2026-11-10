/*
 * rbn_util.c - the add-on's helpers (stage 124): the name tables, errors with
 * a string `code`, reading arguments with a check on each, and making results.
 */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "rbn.h"

/* ---- names -------------------------------------------------------------- */

/* Quality names are addon.ts's Quality; the values are rb.h's RB_QUALITY_*. */
const rbn_name RBN_QUALITIES[] = {
    {"low", RB_QUALITY_LOW},       {"medium", RB_QUALITY_MEDIUM}, {"high", RB_QUALITY_HIGH},
    {"ultra", RB_QUALITY_ULTRA},   {"balanced", RB_QUALITY_BALANCED}, {NULL, 0}};

/* Scene names are RB_SCENE_* lowercased. */
const rbn_name RBN_SCENES[] = {
    {"asphalt", RB_SCENE_ASPHALT}, {"grass", RB_SCENE_GRASS}, {"wall", RB_SCENE_WALL},
    {"garage", RB_SCENE_GARAGE},   {"dyno", RB_SCENE_DYNO},   {"iso", RB_SCENE_ISO},
    {"passby", RB_SCENE_PASSBY},   {"cabin", RB_SCENE_CABIN}, {NULL, 0}};

const rbn_name RBN_BACKENDS[] = {
    {"null", RB_HOST_BACKEND_NULL}, {"default", RB_HOST_BACKEND_DEFAULT}, {NULL, 0}};

const rbn_name RBN_OUTCOMES[] = {
    {"ok", RB_HOST_ACK_OK},           {"stale", RB_HOST_ACK_STALE},
    {"refused", RB_HOST_ACK_REFUSED}, {"busy", RB_HOST_ACK_BUSY}, {NULL, 0}};

/* The acknowledgments' kinds, one per rb_host_command_kind. */
const rbn_name RBN_COMMANDS[] = {
    {"nop", RB_HOST_CMD_NOP},
    {"set-param", RB_HOST_CMD_SET_PARAM},
    {"set-engine-live", RB_HOST_CMD_SET_ENGINE_LIVE},
    {"dyno-attach", RB_HOST_CMD_DYNO_ATTACH},
    {"dyno-release", RB_HOST_CMD_DYNO_RELEASE},
    {"dyno-sweep", RB_HOST_CMD_DYNO_SWEEP},
    {"dyno-clear", RB_HOST_CMD_DYNO_CLEAR},
    {"scene-time", RB_HOST_CMD_SCENE_TIME},
    {"swap", RB_HOST_CMD_SWAP},
    {"epoch", RB_HOST_CMD_EPOCH},
    {"rebase", RB_HOST_CMD_REBASE},
    {NULL, 0}};

/* A build being swapped in counts as pending (see rbn_host.c). */
const rbn_name RBN_PREPARE_STATES[] = {
    {"pending", RB_HOST_PREPARE_PENDING},
    {"ready", RB_HOST_PREPARE_READY},
    {"failed", RB_HOST_PREPARE_FAILED},
    {"pending", RB_HOST_PREPARE_SWAPPING},
    {NULL, 0}};

int rbn_name_value(const rbn_name *table, const char *name)
{
    for (; table->name; table++)
        if (strcmp(table->name, name) == 0) return table->value;
    return -1;
}

const char *rbn_name_of(const rbn_name *table, int value)
{
    for (; table->name; table++)
        if (table->value == value) return table->name;
    return NULL;
}

/* ---- errors --------------------------------------------------------------- */

const char *rbn_host_code_name(int rc)
{
    switch (rc) {
    case RB_HOST_OK:          return "RB_HOST_OK";
    case RB_HOST_ERR_ARG:     return "RB_HOST_ERR_ARG";
    case RB_HOST_ERR_FULL:    return "RB_HOST_ERR_FULL";
    case RB_HOST_ERR_NOMEM:   return "RB_HOST_ERR_NOMEM";
    case RB_HOST_ERR_STATE:   return "RB_HOST_ERR_STATE";
    case RB_HOST_ERR_DEVICE:  return "RB_HOST_ERR_DEVICE";
    case RB_HOST_ERR_NOCLOCK: return "RB_HOST_ERR_NOCLOCK";
    case RB_HOST_ERR_BUILD:   return "RB_HOST_ERR_BUILD";
    case RB_HOST_ERR_THREAD:  return "RB_HOST_ERR_THREAD";
    default:                  return "RB_HOST_ERR_UNKNOWN";
    }
}

const char *rbn_core_code_name(int rc)
{
    switch (rc) {
    case RB_OK:           return "RB_OK";
    case RB_ERR_ARG:      return "RB_ERR_ARG";
    case RB_ERR_ID:       return "RB_ERR_ID";
    case RB_ERR_VALUE:    return "RB_ERR_VALUE";
    case RB_ERR_CONFIG:   return "RB_ERR_CONFIG";
    case RB_ERR_READONLY: return "RB_ERR_READONLY";
    case RB_ERR_SIZE:     return "RB_ERR_SIZE";
    default:              return "RB_ERR_UNKNOWN";
    }
}

napi_value rbn_throw(napi_env env, const char *code, const char *format, ...)
{
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof message, format, args);
    va_end(args);
    /* napi_throw_error makes an Error with `code` set to the string */
    napi_throw_error(env, code, message);
    return NULL;
}

napi_value rbn_throw_host(napi_env env, int rc, const char *what)
{
    return rbn_throw(env, rbn_host_code_name(rc), "%s failed: %s (%d)", what, rbn_host_code_name(rc), rc);
}

napi_value rbn_throw_core(napi_env env, int rc, const char *what)
{
    return rbn_throw(env, rbn_core_code_name(rc), "%s failed: %s (%d)", what, rbn_core_code_name(rc), rc);
}

napi_value rbn_throw_status(napi_env env, napi_status status)
{
    bool pending = false;
    napi_is_exception_pending(env, &pending);
    if (!pending) rbn_throw(env, "RBN_NAPI", "a Node-API call failed (status %d)", (int)status);
    return NULL;
}

/* ---- reading arguments ------------------------------------------------------ */

static int bad_argument(napi_env env, const char *what, const char *wanted)
{
    rbn_throw(env, "RB_HOST_ERR_ARG", "%s must be %s", what, wanted);
    return -1;
}

int rbn_get_double(napi_env env, napi_value v, const char *what, double *out)
{
    napi_valuetype type;
    RBN_TRY_INT(napi_typeof(env, v, &type));
    if (type != napi_number) return bad_argument(env, what, "a number");
    RBN_TRY_INT(napi_get_value_double(env, v, out));
    return 0;
}

int rbn_get_int(napi_env env, napi_value v, const char *what, int64_t lo, int64_t hi, int64_t *out)
{
    double d;
    if (rbn_get_double(env, v, what, &d) != 0) return -1;
    if (!(d == floor(d)) || d < (double)lo || d > (double)hi) {
        rbn_throw(env, "RB_HOST_ERR_ARG", "%s must be a whole number from %lld to %lld", what, (long long)lo,
                  (long long)hi);
        return -1;
    }
    *out = (int64_t)d;
    return 0;
}

int rbn_get_u64(napi_env env, napi_value v, const char *what, int optional, uint64_t fallback, uint64_t *out)
{
    napi_valuetype type;
    RBN_TRY_INT(napi_typeof(env, v, &type));
    if (optional && (type == napi_undefined || type == napi_null)) {
        *out = fallback;
        return 0;
    }
    int64_t n;
    /* 2^53: the largest whole number a double holds exactly */
    if (rbn_get_int(env, v, what, 0, 9007199254740992LL, &n) != 0) return -1;
    *out = (uint64_t)n;
    return 0;
}

int rbn_get_string(napi_env env, napi_value v, const char *what, char *out, size_t capacity)
{
    napi_valuetype type;
    RBN_TRY_INT(napi_typeof(env, v, &type));
    if (type != napi_string) return bad_argument(env, what, "a string");
    size_t length = 0;
    RBN_TRY_INT(napi_get_value_string_utf8(env, v, out, capacity, &length));
    if (length + 1 >= capacity) {
        rbn_throw(env, "RB_HOST_ERR_ARG", "%s is too long", what);
        return -1;
    }
    return 0;
}

int rbn_expect_object(napi_env env, napi_value v, const char *what)
{
    napi_valuetype type;
    RBN_TRY_INT(napi_typeof(env, v, &type));
    if (type != napi_object) return bad_argument(env, what, "an object");
    return 0;
}

int rbn_get_prop(napi_env env, napi_value object, const char *key, napi_value *out, int *present)
{
    RBN_TRY_INT(napi_get_named_property(env, object, key, out));
    napi_valuetype type;
    RBN_TRY_INT(napi_typeof(env, *out, &type));
    *present = type != napi_undefined && type != napi_null;
    return 0;
}

int rbn_get_args(napi_env env, napi_callback_info info, size_t max, napi_value *args, size_t *count,
                 napi_value *self)
{
    size_t given = max;
    RBN_TRY_INT(napi_get_cb_info(env, info, &given, args, self, NULL));
    if (count) *count = given;
    return 0;
}

/* ---- making results --------------------------------------------------------- */

napi_value rbn_number(napi_env env, double value)
{
    napi_value result;
    RBN_TRY(napi_create_double(env, value, &result));
    return result;
}

int rbn_set_value(napi_env env, napi_value object, const char *key, napi_value value)
{
    RBN_TRY_INT(napi_set_named_property(env, object, key, value));
    return 0;
}

int rbn_set_number(napi_env env, napi_value object, const char *key, double value)
{
    napi_value v = rbn_number(env, value);
    return v ? rbn_set_value(env, object, key, v) : -1;
}

int rbn_set_string(napi_env env, napi_value object, const char *key, const char *value)
{
    napi_value v;
    RBN_TRY_INT(napi_create_string_utf8(env, value ? value : "", NAPI_AUTO_LENGTH, &v));
    return rbn_set_value(env, object, key, v);
}

int rbn_set_bool(napi_env env, napi_value object, const char *key, int value)
{
    napi_value v;
    RBN_TRY_INT(napi_get_boolean(env, value != 0, &v));
    return rbn_set_value(env, object, key, v);
}

int rbn_set_null(napi_env env, napi_value object, const char *key)
{
    napi_value v;
    RBN_TRY_INT(napi_get_null(env, &v));
    return rbn_set_value(env, object, key, v);
}
