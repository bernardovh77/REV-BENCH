/*
 * rbn_module.c - the add-on's entry point (stage 124): `require("rb_native.node")`
 * returns the Addon of app/src/shared/addon.ts.
 *
 * Node-API only (no V8, no NAN): one .node loads in plain Node and in
 * Electron's utility process alike. The module is initialised once per
 * environment, so the Host class lives in that environment's instance data.
 */
#include "rbn.h"

/* Puts one function on the exports object. */
static int export_function(napi_env env, napi_value exports, const char *name, napi_callback function)
{
    napi_value value;
    RBN_TRY_INT(napi_create_function(env, name, NAPI_AUTO_LENGTH, function, NULL, &value));
    RBN_TRY_INT(napi_set_named_property(env, exports, name, value));
    return 0;
}

NAPI_MODULE_INIT()
{
    if (rbn_host_class_init(env) != 0) return NULL;
    if (export_function(env, exports, "version", rbn_version) != 0 ||
        export_function(env, exports, "presets", rbn_presets) != 0 ||
        export_function(env, exports, "params", rbn_params) != 0 ||
        export_function(env, exports, "telemetry", rbn_telemetry) != 0 ||
        export_function(env, exports, "listDevices", rbn_list_devices) != 0 ||
        export_function(env, exports, "createHost", rbn_create_host) != 0)
        return NULL;
    return exports;
}
