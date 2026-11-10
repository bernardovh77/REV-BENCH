# Stage 124, the add-on: execution notes

Builder A's part of [stage 124](roadmap/07-app.md#stage-124): the Node-API add-on, `app/native/`.
It implements `app/src/shared/addon.ts` (unchanged) over the real-time host (`host/`, stage 123)
and the core's metadata (`core/include/rb.h`). The rest of the stage (the app, the process model,
the channel) is builder B's, in `docs/stage-124.md`.

**Status:** built and tested on Windows (MSVC 19.51, Node 26.10) and in WSL (gcc, Node 22.23);
`python tools/run_tests.py addon` passes on both. Loading it in Electron 45 was not checked here
(see "Electron").

## What was built

```
app/native/CMakeLists.txt         the build (cmake-js 8.0.0, node-api-headers 1.9.0)
app/native/package.json           for cmake-js only: binary.napi_versions, so it builds a Node-API add-on
app/native/build-tools/           package.json + package-lock.json pinning cmake-js and node-api-headers
app/native/src/rbn.h              the shared declarations and the conventions
app/native/src/rbn_module.c       the entry point (NAPI_MODULE_INIT): the six functions of Addon
app/native/src/rbn_meta.c         version, presets, params, telemetry, listDevices
app/native/src/rbn_build.c        BuildSpec and HostConfig -> rb_host_build and rb_host_config
app/native/src/rbn_host.c         the Host class, createHost, the handle and its finalizer
app/native/src/rbn_util.c         names, errors with a string code, argument reading, result making
app/native/src/rbn_win_delay_load.c   Windows: the node.exe import is answered by the running executable
app/native/test/*.test.mjs        64 tests (node:test) on the null backend; fixtures/ for child processes
tools/bugs/stage124_addon.py      the planted bugs
tools/run_tests.py                the `addon` suite
```

**The build.** `app/native/CMakeLists.txt` does `add_subdirectory(<repo root> EXCLUDE_FROM_ALL)`
and links the root's own `rb_host` and `rb_core` targets, so the add-on is built from the same
sources, warnings (`RB_WERROR`: errors) and flags as the host's tests, and no list of sources is
kept twice; `EXCLUDE_FROM_ALL` builds only those two (the 48 objects), never the tests and tools.
The add-on's own sources are built with the same strict doubles (`/fp:precise`,
`-ffp-contract=off`), warnings as errors, and link-time optimisation like the root's libraries
(`RB_ADDON_LTO`, on). Target `rb_native_addon`, output name `rb_native.node` (the root already has
a library `rb_native`, so the import library and the debug info are named apart too). The output
is always `<build folder>/rb_native.node`, whatever folder cmake-js asks for (it differs between
Windows and Linux).

cmake-js decides it is building a Node-API add-on from `binary.napi_versions` in the package.json
of the folder it builds (`-d`); it then neither downloads Node's headers nor needs them, uses
`node-api-headers` 1.9.0 and, on Windows, makes `node.lib` from its `node_api.def`. Its own
`win_delay_load_hook.cc` is C++ and is not used (`CMAKE_JS_SRC` is ignored): the add-on is pure C
with `rbn_win_delay_load.c`, which does the same thing.

**One .node for Node and Electron.** Node-API only (`NAPI_VERSION 8`: Node 22 and every Electron
since 18), no V8, no NAN. Windows: linked against `node.lib` with `/DELAYLOAD:node.exe` (cmake-js
passes it, the CMakeLists repeats it) and a delay-load hook that returns `GetModuleHandle(NULL)`
for "node.exe", so the napi_* functions come from whatever executable runs it. Checked by copying
`node.exe` to `renamed_node.exe` and loading the add-on there. Linux: only `napi_register_module_v1`
and `node_api_module_get_api_version_v1` are exported (`-fvisibility=hidden`,
`--exclude-libs,ALL`); macOS links with `-undefined dynamic_lookup` (stage 126 tests it). The module
is context-aware: the Host class is made per environment (instance data), and the add-on loads in
a worker thread (tested).

**Running the build and tests.** `python tools/run_tests.py addon` (with `RB_TESTS_OUT` for your own
folder):
1. copies `app/native/build-tools/package.json` and its lock to `<out>/addon-node` and runs
   `npm ci --prefix` there (never inside the repo);
2. `cmake-js build -d app/native -O <out>/addon-build -G Ninja` (Windows: through
   `tools/msvc_env.py`, so MSVC; elsewhere gcc);
3. `node --test --test-concurrency=1 app/native/test/*.test.mjs` with `RB_ADDON` pointing at the
   built add-on (the files run one at a time: each makes real-time threads).

Needs node, npm, cmake and ninja on PATH (the suite's pre-flight says which is missing), and on
Windows Visual Studio's C/C++ tools. Measured on this machine: the whole suite 16 s (install 3 s,
build 7 s, tests 9 s); the .node is 563 KB (MSVC, LTO) or 1.7 MB (gcc, with debug info).

## The Addon, call by call

- `version()`: `addon` is the core's `rb_version()` (the add-on is built from it, one version);
  `snapshot` is `RB_SNAPSHOT_VERSION` (2); `nodeApi` is `napi_get_version`.
- `presets()`, `params()`, `telemetry()`: read from the core's tables on each call (no copy kept).
  The parameter `flags` are the words of addon.ts only (`control`, `rebuild`, `integer`);
  `RB_PF_LIVE`, which nearly every parameter has, is left out.
- `listDevices(backend)`: `rb_host_list_devices`; `rate` is `rb_host_pick_rate` of the device's
  first native rate (48000 when it takes anything); `nativeRate` is null when the device reports
  none. Only `"null"` and `"default"` are accepted (manual and offline are the host tests').
- `createHost(config)`: `rb_host_create`; the engine process calls `start()` itself.
- Names: preset by key (`rb_preset_find`), quality `low|medium|high|ultra|balanced`, scene
  `asphalt|grass|wall|garage|dyno|iso|passby|cabin`. An unknown name is `RB_ERR_ID`; a value of the
  wrong type or out of range is `RB_HOST_ERR_ARG`; more than 64 edits or 31 params (the source
  parameter takes one of 32) is `RB_HOST_ERR_FULL`; an engine the core cannot build (a bad field
  or parameter id in the initial build) is `RB_HOST_ERR_BUILD`.
- Commands (`setParam`, `setEngineLive`): return the request id; an absent `expectedGeneration`
  is 0, any. Acks come from `poll()` (all that have arrived, in order, 64 at a time inside).
  kinds are the `rb_host_command_kind`s in words (`set-param`, `set-engine-live`, `dyno-attach`,
  ..., `swap`, `epoch`, `rebase`); outcomes `ok|stale|refused|busy`; `reset` is
  `RB_HOST_ACK_F_RESET`.
- Rebuilds: `prepare(spec)` returns the prepare id (a worker builds; the engine plays on).
  `prepareState` gives `pending|ready|failed`, the core's code and the reason ("edit 0 was
  refused"); a build that has been handed to the render thread (`SWAPPING`) reads as `pending`,
  and once its swap ack has been polled the id is gone (`RB_HOST_ERR_ARG`). `swap(id, gen?)`:
  default generation is the one when the build began, so a swap that lost a race is `stale`.
- `readSnapshot()`: reads into a per-host 64 KiB scratch buffer, then makes a fresh ArrayBuffer of
  exactly the snapshot's size and copies (an external ArrayBuffer would alias the scratch buffer,
  and Electron forbids external buffers under its memory cage). `null` when there is no new one.
- `stats()`: `renderMsMean` = total / blocks; `deadline` = the 12 bins as shares of the counted
  blocks; `ringFillMs` = the last block's fill; `ringTargetMs`; `forbiddenCalls` = alloc + lock +
  log counts; `processMb` = private bytes.
- `destroy()` marks the handle first and frees the host; every call after it, `destroy()` included,
  throws `DESTROYED` (the handle is checked before the arguments). A Host nobody destroyed is
  freed by the finalizer when garbage collected or when the environment goes away.
- Errors are `Error`s with a string `code`: the C name (`RB_HOST_ERR_ARG`, `RB_ERR_ID`, ...) or
  `DESTROYED`.

## Decisions (nobody could be asked)

- **cmake-js's package.json hook:** `app/native/package.json` exists only for cmake-js's
  Node-API detection. It is private and has no dependencies; `app/package.json` is B's.
- **An unknown preset key in `prepare()` throws at once** (`RB_ERR_ID`) instead of becoming a
  failed prepare: nothing was started, and the engine process can reply with the error. A build
  the core cannot make (a bad edit or parameter) is the failed state of `prepareState`.
- **`createHost` with a build that fails says `RB_HOST_ERR_BUILD` only:** `rb_host_create` throws
  away the core's reason. (Host change, not made: could return the `why`.)
- **A second `destroy()` throws** as addon.ts says ("any later call"); the engine process should
  call it once.
- **`SWAPPING` reads as `pending`** (addon.ts has three states).
- **Methods called on a non-Host:** V8 itself throws `TypeError: Illegal invocation` for a receiver
  that is not an instance of the class (no `code`); an instance made by the class without
  `createHost` gets `RB_HOST_ERR_ARG`.
- **Dependencies pinned and aged:** cmake-js 8.0.0 (2026-01-27, MIT), node-api-headers 1.9.0
  (2026-05-22, MIT), and cmake-js's 37 dependencies locked; the newest resolution of `semver`
  (7.8.6) was 7 days old, so `overrides` pins 7.8.5 (2026-06-19, ISC). All others are at least
  two weeks old. Licences are in the lock (MIT, ISC, BlueOak-1.0.0, BSD-2-Clause OR MIT OR
  Apache-2.0); the build tools are not shipped.

## Tests

`app/native/test/`, `node --test`, the null backend only (`listDevices("default")` only lists,
and accepts "no sound card" as an answer). Expected values come from independent sources: the
header `rb.h` (version, engine field ids), `rb_params.c`'s rows (parameter literals),
`contracts/snapshot-v2.md` (layout, instruments, sizes 304 + 40 n), the host's contract
(capacities, states).

| File | Seam | Tests |
| --- | --- | --- |
| `meta.test.mjs` | the module: exports, version, presets, params, telemetry, listDevices (null, default, bad backend) | 8 |
| `create.test.mjs` | createHost: qualities (physics rate in the snapshot), preset default quality, presets, scenes, edits (and their index), params, settleBlocks, sample rate/fill/device, unknown names, unbuildable engines, FULL, a table of bad configs, independence of hosts | 15 |
| `commands.test.mjs` | setParam/setEngineLive: ids and acks, order, before start, clamped, refused, stale, live vs non-live fields, full queue, bad arguments, stats counts | 11 |
| `snapshot.test.mjs` | readSnapshot: null before/twice, snapshot-v2 header and sizes, info fields, copies, rpm rising after the starter, wholeness | 6 |
| `rebuild.test.mjs` | prepare, prepareState, swap, discard: ready, swap ack (generation, reset, new bytes), controls reset, edits and params, failed build (code, message, engine untouched), stale swaps, eight builds, bad arguments | 12 |
| `lifecycle.test.mjs` | start/stop, stats, swap count, selectDevice, suspend/resume, destroy and DESTROYED for every method, not-a-Host, GC of undestroyed hosts (memory falls back), a process that ends with a running host, a worker thread | 12 |

## Planted bugs

`python tools/bugs/stage124_addon.py [NAME...]` (`--list`, `--check`): the sources are copied to a
temporary folder, built once (baseline: builds and passes), then each bug is one text change to a
file in `app/native/src`, an incremental rebuild and the named test files.

30 bugs, all caught (Windows, MSVC; a run takes about 6 minutes). Each line is the first failing
test of the named files.

| Bug | Caught by |
| --- | --- |
| snapshot-bytes-not-copied (zeroed ArrayBuffer) | snapshot: the bytes are snapshot-v2 |
| snapshot-is-a-view-of-the-scratch-buffer (external ArrayBuffer) | snapshot: each read is a copy of its own |
| snapshot-info-sequence-is-the-generation | snapshot: the info beside the bytes |
| ack-stale-and-refused-mixed-up | commands: a command the core refuses |
| ack-reset-flag-dropped | rebuild: swap hands the engine over |
| ack-swap-kind-misnamed | rebuild: swap hands the engine over |
| ack-code-is-the-outcome | commands: a parameter the core clamps |
| request-id-off-by-one | commands: a command returns a request id |
| set-param-ignores-the-expected-generation | commands: a command for another generation is stale |
| set-engine-live-ignores-the-expected-generation | commands: setEngineLive |
| swap-ignores-the-expected-generation | rebuild: a swap for another generation is stale |
| set-param-value-truncated | commands: the parameter takes effect |
| destroy-does-not-mark-the-handle | lifecycle (the file crashes: use after free) |
| finalizer-leaks-the-host | lifecycle: hosts nobody destroyed are freed by the garbage collector |
| host-methods-do-not-check-the-handle | lifecycle: after destroy() every call throws DESTROYED |
| start-error-ignored | lifecycle: start() twice |
| select-device-index-ignored | lifecycle: selectDevice |
| quality-name-mapped-to-the-wrong-enum | create: each quality name makes the engine at its physics rate |
| quality-default-ignores-the-preset | create: an engine built without a quality |
| preset-quality-is-always-high | create: the same test (first missed by meta.test.mjs, which only checked the names; the physics-rate assertion was added) |
| full-code-misnamed | commands, create, rebuild: the FULL tests |
| prepare-state-failed-reads-as-ready | rebuild: a build that fails |
| prepare-failure-message-dropped | rebuild: a build that fails |
| discard-does-nothing | rebuild: a build that fails (discard, then the id is gone) |
| edit-index-ignored | create: an edit's index picks the cylinder (added for it) |
| settle-blocks-ignored | create: settleBlocks |
| sample-rate-ignored | create: the sample rate, the ring's fill target |
| snapshot-version-wrong | meta: version() |
| integer-flag-misnamed | meta: params() |
| stats-ring-target-uses-48k | create: the sample rate (44.1 kHz) test |

Not detectable from JavaScript (so no bug planted): `forbiddenCalls` summing only one of the
host's three counters (the tests can only see it stays 0), the stereo scene being the right one
(the snapshot does not show the scene), the device list's rate for a real device's native rate
(the null device reports none).

## Left for the others

- Electron 45: loading the same `.node` in the utility process (B, phase 2). The delay-load hook is
  checked with a renamed `node.exe`; a real Electron may differ in what its executable exports
  (it must export the `napi_*` functions of node_api.def, which node-gyp add-ons also rely on).
- macOS: builds with `-undefined dynamic_lookup`, untested until stage 126.
- `graphify update .` was not run (the graph is committed and is a whole-file diff).
