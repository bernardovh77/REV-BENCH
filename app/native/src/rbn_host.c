/*
 * rbn_host.c - the Host class and createHost (stage 124).
 *
 * A JavaScript Host is an object of a class made once per environment, with
 * the rbn_host below wrapped inside it. Every method is synchronous, runs on
 * the engine process's JavaScript thread (the host's single control thread)
 * and only sends or reads: the render thread belongs to the host, and this
 * file never renders or touches an engine.
 *
 * Lifetime. destroy() frees the host and marks the handle (`host` NULL):
 * every call after it, destroy included, throws DESTROYED, and nothing
 * touches the freed host. A Host that is garbage collected (or whose
 * environment goes away) without destroy() is freed by its finalizer.
 */
#include <stdlib.h>
#include <string.h>

#include "rbn.h"

/* What is wrapped in a Host object. */
typedef struct rbn_host {
    rb_host *host;          /* NULL once destroyed                            */
    uint8_t *scratch;       /* where a snapshot is read before it is copied   */
} rbn_host;

/* Marks the objects made here, so a method called on anything else is
 * refused before its wrapped pointer is trusted. */
static const napi_type_tag HOST_TAG = {0x52455642454e4348ull, 0x5242484f53543234ull};

/* Per environment (Electron may load the add-on in several): the class. */
typedef struct rbn_env {
    napi_ref host_class;
} rbn_env;

/* A command or a read asks for at most this many acknowledgments at once. */
#define RBN_POLL_BATCH 64

/* ---- the handle ------------------------------------------------------------- */

/* Frees what the handle owns. The host is destroyed once: the pointer is
 * cleared first, so a second path to here finds nothing. */
static void host_release(rbn_host *h)
{
    rb_host *host = h->host;
    h->host = NULL;
    if (host) rb_host_destroy(host);
}

/* The garbage collector's (or the environment teardown's) call: a Host nobody
 * destroyed. No JavaScript runs here. */
static void host_finalize(napi_env env, void *data, void *hint)
{
    (void)env;
    (void)hint;
    rbn_host *h = (rbn_host *)data;
    host_release(h);
    free(h->scratch);
    free(h);
}

/* The handle of the Host a method was called on, or NULL after throwing:
 * ARG for anything that is not a Host, DESTROYED for one that was destroyed. */
static rbn_host *unwrap(napi_env env, napi_value self)
{
    bool tagged = false;
    rbn_host *h = NULL;
    if (napi_check_object_type_tag(env, self, &HOST_TAG, &tagged) != napi_ok || !tagged ||
        napi_unwrap(env, self, (void **)&h) != napi_ok || !h) {
        rbn_throw(env, "RB_HOST_ERR_ARG", "this is not a Host");
        return NULL;
    }
    if (!h->host) {
        rbn_throw(env, "DESTROYED", "the host was destroyed");
        return NULL;
    }
    return h;
}

/* The start of every method: its arguments (at most `max`), and the live
 * handle; NULL after throwing. */
static rbn_host *begin(napi_env env, napi_callback_info info, size_t max, napi_value *args, size_t *given)
{
    napi_value self;
    size_t count = 0;
    if (rbn_get_args(env, info, max, args, &count, &self) != 0) return NULL;
    if (given) *given = count;
    return unwrap(env, self);
}

/* Throws unless `given` arguments reach `needed`. */
static int need(napi_env env, size_t given, size_t needed, const char *method)
{
    if (given >= needed) return 0;
    rbn_throw(env, "RB_HOST_ERR_ARG", "%s needs %d argument%s", method, (int)needed, needed == 1 ? "" : "s");
    return -1;
}

/* ---- lifecycle ---------------------------------------------------------------- */

static napi_value host_start(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    const int rc = rb_host_start(h->host);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "start");
    return NULL;
}

static napi_value host_stop(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    const int rc = rb_host_stop(h->host);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "stop");
    return NULL;
}

static napi_value host_destroy(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    host_release(h);
    return NULL;
}

static napi_value host_sample_rate(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    return rbn_number(env, rb_host_sample_rate(h->host));
}

static napi_value host_generation(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    return rbn_number(env, (double)rb_host_generation(h->host));
}

/* ---- commands ------------------------------------------------------------------- */

/* Gives a command's request id to JavaScript, or throws for its error. */
static napi_value command_result(napi_env env, int rc, uint64_t request_id, const char *method)
{
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, method);
    return rbn_number(env, (double)request_id);
}

static napi_value host_set_param(napi_env env, napi_callback_info info)
{
    napi_value args[3];
    size_t given = 0;
    rbn_host *h = begin(env, info, 3, args, &given);
    if (!h || need(env, given, 2, "setParam") != 0) return NULL;
    int64_t id = 0;
    double value = 0.0;
    uint64_t expected = 0, request = 0;
    if (rbn_get_int(env, args[0], "id", 0, RBN_MAX_ID_VALUE, &id) != 0 ||
        rbn_get_double(env, args[1], "value", &value) != 0 ||
        rbn_get_u64(env, args[2], "expectedGeneration", 1, 0, &expected) != 0)
        return NULL;
    const int rc = rb_host_set_param(h->host, (int)id, value, expected, &request);
    return command_result(env, rc, request, "setParam");
}

static napi_value host_set_engine_live(napi_env env, napi_callback_info info)
{
    napi_value args[4];
    size_t given = 0;
    rbn_host *h = begin(env, info, 4, args, &given);
    if (!h || need(env, given, 3, "setEngineLive") != 0) return NULL;
    int64_t field = 0, index = 0;
    double value = 0.0;
    uint64_t expected = 0, request = 0;
    if (rbn_get_int(env, args[0], "field", 0, RBN_MAX_ID_VALUE, &field) != 0 ||
        rbn_get_int(env, args[1], "index", 0, RBN_MAX_ID_VALUE, &index) != 0 ||
        rbn_get_double(env, args[2], "value", &value) != 0 ||
        rbn_get_u64(env, args[3], "expectedGeneration", 1, 0, &expected) != 0)
        return NULL;
    const int rc = rb_host_set_engine_live(h->host, (int)field, (int)index, value, expected, &request);
    return command_result(env, rc, request, "setEngineLive");
}

/* ---- rebuilds ----------------------------------------------------------------------- */

static napi_value host_prepare(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    rbn_host *h = begin(env, info, 1, args, &given);
    if (!h || need(env, given, 1, "prepare") != 0) return NULL;
    rb_host_build spec;
    if (rbn_parse_build(env, args[0], "spec", &spec) != 0) return NULL;
    uint64_t prepare_id = 0;
    const int rc = rb_host_prepare_begin(h->host, &spec, &prepare_id);
    return command_result(env, rc, prepare_id, "prepare");
}

static napi_value host_prepare_state(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    rbn_host *h = begin(env, info, 1, args, &given);
    if (!h || need(env, given, 1, "prepareState") != 0) return NULL;
    uint64_t prepare_id = 0;
    if (rbn_get_u64(env, args[0], "prepareId", 0, 0, &prepare_id) != 0) return NULL;
    int state = 0, code = 0;
    char why[128];
    why[0] = '\0';
    const int rc = rb_host_prepare_status(h->host, prepare_id, &state, &code, why, sizeof why);
    if (rc != RB_HOST_OK) return rbn_throw(env, rbn_host_code_name(rc), "no build %llu", (unsigned long long)prepare_id);
    const char *name = rbn_name_of(RBN_PREPARE_STATES, state);
    if (!name) return rbn_throw(env, "RB_HOST_ERR_STATE", "unknown build state %d", state);
    napi_value result;
    RBN_TRY(napi_create_object(env, &result));
    if (rbn_set_string(env, result, "state", name) != 0 || rbn_set_number(env, result, "code", code) != 0 ||
        rbn_set_string(env, result, "message", why) != 0)
        return NULL;
    return result;
}

static napi_value host_swap(napi_env env, napi_callback_info info)
{
    napi_value args[2];
    size_t given = 0;
    rbn_host *h = begin(env, info, 2, args, &given);
    if (!h || need(env, given, 1, "swap") != 0) return NULL;
    uint64_t prepare_id = 0, expected = 0, request = 0;
    if (rbn_get_u64(env, args[0], "prepareId", 0, 0, &prepare_id) != 0 ||
        rbn_get_u64(env, args[1], "expectedGeneration", 1, 0, &expected) != 0)
        return NULL;
    const int rc = rb_host_swap(h->host, prepare_id, expected, &request);
    return command_result(env, rc, request, "swap");
}

static napi_value host_discard(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    rbn_host *h = begin(env, info, 1, args, &given);
    if (!h || need(env, given, 1, "discard") != 0) return NULL;
    uint64_t prepare_id = 0;
    if (rbn_get_u64(env, args[0], "prepareId", 0, 0, &prepare_id) != 0) return NULL;
    const int rc = rb_host_prepare_discard(h->host, prepare_id);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "discard");
    return NULL;
}

/* ---- what comes back ------------------------------------------------------------------- */

/* One acknowledgment as addon.ts's Ack. */
static napi_value make_ack(napi_env env, const rb_host_ack *a)
{
    const char *kind = rbn_name_of(RBN_COMMANDS, a->kind);
    const char *outcome = rbn_name_of(RBN_OUTCOMES, a->outcome);
    napi_value item;
    RBN_TRY(napi_create_object(env, &item));
    if (rbn_set_number(env, item, "requestId", (double)a->request_id) != 0 ||
        rbn_set_string(env, item, "kind", kind ? kind : "unknown") != 0 ||
        rbn_set_string(env, item, "outcome", outcome ? outcome : "unknown") != 0 ||
        rbn_set_number(env, item, "code", a->code) != 0 ||
        rbn_set_number(env, item, "generation", (double)a->generation) != 0 ||
        rbn_set_bool(env, item, "reset", (a->flags & RB_HOST_ACK_F_RESET) != 0) != 0 ||
        rbn_set_number(env, item, "physicsTick", (double)a->physics_tick) != 0 ||
        rbn_set_number(env, item, "audioFrame", (double)a->audio_frame) != 0)
        return NULL;
    return item;
}

static napi_value host_poll(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    napi_value list;
    RBN_TRY(napi_create_array(env, &list));
    uint32_t total = 0;
    rb_host_ack acks[RBN_POLL_BATCH];
    for (;;) {
        const int n = rb_host_poll(h->host, acks, RBN_POLL_BATCH);
        if (n < 0) return rbn_throw_host(env, n, "poll");
        for (int i = 0; i < n; i++) {
            napi_value item = make_ack(env, &acks[i]);
            if (!item) return NULL;
            RBN_TRY(napi_set_element(env, list, total++, item));
        }
        if (n < RBN_POLL_BATCH) break;
    }
    return list;
}

static napi_value host_read_snapshot(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    rb_host_snapshot_info meta;
    memset(&meta, 0, sizeof meta);
    const int size = rb_host_snapshot_read(h->host, h->scratch, RB_HOST_SNAPSHOT_MAX_BYTES, &meta);
    if (size < 0) return rbn_throw_host(env, size, "readSnapshot");
    if (size == 0) {
        napi_value none;
        RBN_TRY(napi_get_null(env, &none));
        return none;
    }
    /* a fresh ArrayBuffer of its own: the scratch buffer is reused by the
     * next read, and nothing is shared with the host */
    napi_value bytes;
    void *data = NULL;
    RBN_TRY(napi_create_arraybuffer(env, (size_t)size, &data, &bytes));
    memcpy(data, h->scratch, (size_t)size);
    napi_value result;
    RBN_TRY(napi_create_object(env, &result));
    if (rbn_set_value(env, result, "bytes", bytes) != 0 ||
        rbn_set_number(env, result, "sequence", (double)meta.sequence) != 0 ||
        rbn_set_number(env, result, "generation", (double)meta.generation) != 0 ||
        rbn_set_number(env, result, "audioFrame", (double)meta.audio_frame) != 0 ||
        rbn_set_number(env, result, "physicsTick", (double)meta.physics_tick) != 0)
        return NULL;
    return result;
}

static napi_value host_stats(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    rb_host_stats s;
    const int rc = rb_host_get_stats(h->host, &s);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "stats");
    const double rate = rb_host_sample_rate(h->host);
    const double blocks = (double)s.blocks;

    napi_value result;
    RBN_TRY(napi_create_object(env, &result));
    if (rbn_set_number(env, result, "blocks", blocks) != 0 ||
        rbn_set_number(env, result, "renderMsMean", s.blocks ? (double)s.render_ns_total / blocks / 1e6 : 0.0) != 0 ||
        rbn_set_number(env, result, "renderMsMax", (double)s.render_ns_max / 1e6) != 0)
        return NULL;

    /* the deadline histogram as shares of the blocks that were counted */
    double counted = 0.0;
    for (int i = 0; i < RB_HOST_DEADLINE_BINS; i++) counted += (double)s.deadline[i];
    napi_value deadline;
    RBN_TRY(napi_create_array_with_length(env, RB_HOST_DEADLINE_BINS, &deadline));
    for (int i = 0; i < RB_HOST_DEADLINE_BINS; i++) {
        napi_value share = rbn_number(env, counted > 0.0 ? (double)s.deadline[i] / counted : 0.0);
        if (!share) return NULL;
        RBN_TRY(napi_set_element(env, deadline, (uint32_t)i, share));
    }
    if (rbn_set_value(env, result, "deadline", deadline) != 0) return NULL;

    if (rbn_set_number(env, result, "ringFillMs", rate > 0.0 ? (double)s.ring_fill_last * 1000.0 / rate : 0.0) != 0 ||
        rbn_set_number(env, result, "ringTargetMs", rate > 0.0 ? (double)s.ring_target * 1000.0 / rate : 0.0) != 0 ||
        rbn_set_number(env, result, "underruns", (double)s.underruns) != 0 ||
        rbn_set_number(env, result, "commandsApplied", (double)s.commands_applied) != 0 ||
        rbn_set_number(env, result, "commandsRefused", (double)s.commands_refused) != 0 ||
        rbn_set_number(env, result, "swapsOk", (double)s.swaps_ok) != 0 ||
        rbn_set_number(env, result, "eventsLost", (double)s.events_lost) != 0 ||
        rbn_set_string(env, result, "priority", s.priority.policy) != 0 ||
        rbn_set_bool(env, result, "boosted", s.priority.boosted != 0) != 0 ||
        rbn_set_number(env, result, "forbiddenCalls", (double)(s.audit_alloc + s.audit_lock + s.audit_log)) != 0 ||
        rbn_set_number(env, result, "processMb", (double)s.process_bytes / (1024.0 * 1024.0)) != 0)
        return NULL;
    return result;
}

/* ---- devices and power ---------------------------------------------------------------------- */

static napi_value host_select_device(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    rbn_host *h = begin(env, info, 1, args, &given);
    if (!h || need(env, given, 1, "selectDevice") != 0) return NULL;
    int64_t index = 0;
    if (rbn_get_int(env, args[0], "index", -1, RBN_MAX_ID_VALUE, &index) != 0) return NULL;
    const int rc = rb_host_select_device(h->host, (int)index);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "selectDevice");
    return NULL;
}

static napi_value host_suspend(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    const int rc = rb_host_suspend(h->host);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "suspend");
    return NULL;
}

static napi_value host_resume(napi_env env, napi_callback_info info)
{
    rbn_host *h = begin(env, info, 0, NULL, NULL);
    if (!h) return NULL;
    const int rc = rb_host_resume(h->host);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "resume");
    return NULL;
}

/* ---- the class and createHost ---------------------------------------------------------------- */

/* The constructor does nothing: createHost wraps the handle after it. */
static napi_value host_construct(napi_env env, napi_callback_info info)
{
    napi_value self;
    RBN_TRY(napi_get_cb_info(env, info, NULL, NULL, &self, NULL));
    return self;
}

static void env_finalize(napi_env env, void *data, void *hint)
{
    (void)hint;
    rbn_env *e = (rbn_env *)data;
    if (e->host_class) napi_delete_reference(env, e->host_class);
    free(e);
}

int rbn_host_class_init(napi_env env)
{
    static const napi_property_descriptor METHODS[] = {
        {"start", NULL, host_start, NULL, NULL, NULL, napi_default_method, NULL},
        {"stop", NULL, host_stop, NULL, NULL, NULL, napi_default_method, NULL},
        {"destroy", NULL, host_destroy, NULL, NULL, NULL, napi_default_method, NULL},
        {"sampleRate", NULL, host_sample_rate, NULL, NULL, NULL, napi_default_method, NULL},
        {"generation", NULL, host_generation, NULL, NULL, NULL, napi_default_method, NULL},
        {"setParam", NULL, host_set_param, NULL, NULL, NULL, napi_default_method, NULL},
        {"setEngineLive", NULL, host_set_engine_live, NULL, NULL, NULL, napi_default_method, NULL},
        {"prepare", NULL, host_prepare, NULL, NULL, NULL, napi_default_method, NULL},
        {"prepareState", NULL, host_prepare_state, NULL, NULL, NULL, napi_default_method, NULL},
        {"swap", NULL, host_swap, NULL, NULL, NULL, napi_default_method, NULL},
        {"discard", NULL, host_discard, NULL, NULL, NULL, napi_default_method, NULL},
        {"poll", NULL, host_poll, NULL, NULL, NULL, napi_default_method, NULL},
        {"readSnapshot", NULL, host_read_snapshot, NULL, NULL, NULL, napi_default_method, NULL},
        {"stats", NULL, host_stats, NULL, NULL, NULL, napi_default_method, NULL},
        {"selectDevice", NULL, host_select_device, NULL, NULL, NULL, napi_default_method, NULL},
        {"suspend", NULL, host_suspend, NULL, NULL, NULL, napi_default_method, NULL},
        {"resume", NULL, host_resume, NULL, NULL, NULL, napi_default_method, NULL},
    };
    rbn_env *e = (rbn_env *)calloc(1, sizeof *e);
    if (!e) {
        rbn_throw(env, "RB_HOST_ERR_NOMEM", "out of memory");
        return -1;
    }
    napi_value host_class;
    napi_status status = napi_define_class(env, "Host", NAPI_AUTO_LENGTH, host_construct, NULL,
                                           sizeof METHODS / sizeof METHODS[0], METHODS, &host_class);
    if (status == napi_ok) status = napi_create_reference(env, host_class, 1, &e->host_class);
    if (status == napi_ok) status = napi_set_instance_data(env, e, env_finalize, NULL);
    if (status != napi_ok) {
        if (e->host_class) napi_delete_reference(env, e->host_class);
        free(e);
        rbn_throw_status(env, status);
        return -1;
    }
    return 0;
}

napi_value rbn_create_host(napi_env env, napi_callback_info info)
{
    napi_value args[1];
    size_t given = 0;
    if (rbn_get_args(env, info, 1, args, &given, NULL) != 0 || need(env, given, 1, "createHost") != 0) return NULL;
    rb_host_config config;
    if (rbn_parse_config(env, args[0], &config) != 0) return NULL;

    rbn_env *e = NULL;
    RBN_TRY(napi_get_instance_data(env, (void **)&e));
    if (!e) return rbn_throw(env, "RB_HOST_ERR_STATE", "the add-on is not initialised");

    rb_host *host = NULL;
    const int rc = rb_host_create(&config, &host);
    if (rc != RB_HOST_OK) return rbn_throw_host(env, rc, "createHost");

    rbn_host *h = (rbn_host *)calloc(1, sizeof *h);
    if (h) h->scratch = (uint8_t *)malloc(RB_HOST_SNAPSHOT_MAX_BYTES);
    if (!h || !h->scratch) {
        if (h) free(h);
        rb_host_destroy(host);
        return rbn_throw(env, "RB_HOST_ERR_NOMEM", "out of memory");
    }
    h->host = host;

    /* from here the handle belongs to the object's finalizer, once wrapped */
    napi_value host_class, object;
    napi_status status = napi_get_reference_value(env, e->host_class, &host_class);
    if (status == napi_ok) status = napi_new_instance(env, host_class, 0, NULL, &object);
    if (status == napi_ok) status = napi_wrap(env, object, h, host_finalize, NULL, NULL);
    if (status != napi_ok) {
        host_release(h);
        free(h->scratch);
        free(h);
        return rbn_throw_status(env, status);
    }
    status = napi_type_tag_object(env, object, &HOST_TAG);
    if (status != napi_ok) return rbn_throw_status(env, status);
    return object;
}
