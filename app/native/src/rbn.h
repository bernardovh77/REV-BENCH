/*
 * rbn.h - the Node-API add-on's shared declarations (stage 124).
 *
 * The add-on is a thin layer: it turns JavaScript values into the calls of
 * the real-time host (host/include/rb_host.h) and the core's metadata
 * (core/include/rb.h), and the results back into JavaScript values. It never
 * renders and never touches an engine: the host's render thread does.
 * app/src/shared/addon.ts is the contract it implements.
 *
 * Conventions:
 *   - a function that returns napi_value returns NULL after it threw;
 *   - a function that returns int returns 0 when it worked and -1 after it
 *     threw (so the caller returns NULL and lets the exception go);
 *   - every error is an Error with a string `code`: the C name of the
 *     host's or the core's return code, or "DESTROYED".
 */
#ifndef RBN_H
#define RBN_H

/* Node-API version 8 is in Node 22 and every Electron since 18 */
#define NAPI_VERSION 8
#include <node_api.h>

#include <stdint.h>

#include "rb.h"
#include "rb_host.h"

/* The largest id, field or index JavaScript may pass (the core checks the real range). */
#define RBN_MAX_ID_VALUE 65535

/* ---- names -------------------------------------------------------------- */

typedef struct rbn_name {
    const char *name;
    int         value;
} rbn_name;

extern const rbn_name RBN_QUALITIES[];      /* "low" ... "balanced"          */
extern const rbn_name RBN_SCENES[];         /* "asphalt" ... "cabin"         */
extern const rbn_name RBN_BACKENDS[];       /* "null", "default"             */
extern const rbn_name RBN_OUTCOMES[];       /* "ok", "stale", "refused", "busy" */
extern const rbn_name RBN_COMMANDS[];       /* the acknowledgments' kinds    */
extern const rbn_name RBN_PREPARE_STATES[]; /* "pending", "ready", "failed"  */

/* The value of a name, or -1 when the table has none. */
int         rbn_name_value(const rbn_name *table, const char *name);
/* The name of a value, or NULL. */
const char *rbn_name_of(const rbn_name *table, int value);

/* ---- errors --------------------------------------------------------------- */

/* Throws an Error whose `code` is code and whose message is formatted;
 * always returns NULL. */
napi_value rbn_throw(napi_env env, const char *code, const char *format, ...);
/* Throws for a host return code (RB_HOST_ERR_*): what was being done, and
 * the code's C name. Always returns NULL. */
napi_value rbn_throw_host(napi_env env, int rc, const char *what);
/* The same for a core return code (RB_ERR_*). */
napi_value rbn_throw_core(napi_env env, int rc, const char *what);
/* Throws for a Node-API call that failed (unless an exception is already
 * pending, which then goes on); always returns NULL. */
napi_value rbn_throw_status(napi_env env, napi_status status);
/* The C name of a host or core return code ("RB_HOST_ERR_FULL" ...). */
const char *rbn_host_code_name(int rc);
const char *rbn_core_code_name(int rc);

/* Runs a Node-API call; on failure throws and returns NULL from the caller. */
#define RBN_TRY(call)                                                          \
    do {                                                                       \
        napi_status rbn_status_ = (call);                                      \
        if (rbn_status_ != napi_ok) return rbn_throw_status(env, rbn_status_); \
    } while (0)
/* The same for a caller that returns int. */
#define RBN_TRY_INT(call)                                                      \
    do {                                                                       \
        napi_status rbn_status_ = (call);                                      \
        if (rbn_status_ != napi_ok) {                                          \
            rbn_throw_status(env, rbn_status_);                                \
            return -1;                                                         \
        }                                                                      \
    } while (0)

/* ---- reading arguments ------------------------------------------------------ */

/* Each throws RB_HOST_ERR_ARG with the argument's name when the value isn't
 * what is asked for. */
int rbn_get_double(napi_env env, napi_value v, const char *what, double *out);
/* A whole number in [lo, hi]. */
int rbn_get_int(napi_env env, napi_value v, const char *what, int64_t lo, int64_t hi, int64_t *out);
/* A whole number in [0, 2^53], for ids and generations; undefined or null
 * is `fallback` when `optional`. */
int rbn_get_u64(napi_env env, napi_value v, const char *what, int optional, uint64_t fallback, uint64_t *out);
int rbn_get_string(napi_env env, napi_value v, const char *what, char *out, size_t capacity);
/* The object's property, and whether it is there (not undefined or null). */
int rbn_get_prop(napi_env env, napi_value object, const char *key, napi_value *out, int *present);
/* A value that must be an object (an array is one). */
int rbn_expect_object(napi_env env, napi_value v, const char *what);
/* The arguments of a call: up to `max` (the rest undefined); `count` gets how
 * many were passed. */
int rbn_get_args(napi_env env, napi_callback_info info, size_t max, napi_value *args, size_t *count,
                 napi_value *self);

/* ---- making results --------------------------------------------------------- */

napi_value rbn_number(napi_env env, double value);
/* Sets object[key]; each returns 0 or throws and returns -1. */
int rbn_set_number(napi_env env, napi_value object, const char *key, double value);
int rbn_set_string(napi_env env, napi_value object, const char *key, const char *value);
int rbn_set_bool(napi_env env, napi_value object, const char *key, int value);
int rbn_set_null(napi_env env, napi_value object, const char *key);
int rbn_set_value(napi_env env, napi_value object, const char *key, napi_value value);

/* ---- BuildSpec and HostConfig ------------------------------------------------ */

/* Reads addon.ts's BuildSpec into a build description (starting from
 * rb_host_build_default). */
int rbn_parse_build(napi_env env, napi_value spec, const char *what, rb_host_build *out);
/* Reads addon.ts's HostConfig. */
int rbn_parse_config(napi_env env, napi_value config, rb_host_config *out);

/* ---- the pieces ---------------------------------------------------------------- */

/* The module's functions (rbn_meta.c): metadata and devices. */
napi_value rbn_version(napi_env env, napi_callback_info info);
napi_value rbn_presets(napi_env env, napi_callback_info info);
napi_value rbn_params(napi_env env, napi_callback_info info);
napi_value rbn_telemetry(napi_env env, napi_callback_info info);
napi_value rbn_list_devices(napi_env env, napi_callback_info info);

/* The Host class and createHost (rbn_host.c). */
napi_value rbn_create_host(napi_env env, napi_callback_info info);
/* Defines the Host class once for this environment and keeps it. */
int rbn_host_class_init(napi_env env);

#endif /* RBN_H */
