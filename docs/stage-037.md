# Stage 037 — Shared build, geometry, event and snapshot contracts

2026-10-23. Baseline `e7eb463a20e88fbd32c0ac05f57932f5673f4439`.
Bernardo authorizes the next planned stage. Starting worktree contains only
user-added `.agents/` and `skills-lock.json`; preserve and exclude them from
the checkpoint commit. PLAN.md owns stage status.

## Scope and frozen evidence gates

Define one versioned shared description before source/propagation, browser and
3D implementation. Deliver a normative schema, current-consumer review and
executable native round-trip/structural contract proof. Do not select a new
renderer or implement the full geometry compiler, event producer, production
bulk API, snapshot transport, worklet engine or 3D here. Preserve accepted
audio, native physics, WASM and Claude setup.

Frozen gates: all five current presets round-trip all active descriptor fields
through SI/JSON/native preparation; integer fields exactly and doubles within
relative `1e-14` / absolute `1e-15`. Removing intake runners preserves every
unrelated component/port/source identity and logical route. Unknown versions,
features, topology, stale generations and impossible native engines reject.
Reference controls cover unwrapped cycles, coherent clock/generation headers,
event identity, failed transaction preservation, reset history and a worked
static-path audible-arrival calculation. The latter are protocol/model tests,
not new production traces or concurrency/device proofs.

## Design, research and consumer review

Normative [build/geometry contract](../contracts/README.md),
[event/runtime contract](../contracts/runtime-v1.md),
[SI unit catalog](../contracts/engine-fields-v1.json) and
[five examples](../contracts/examples/builds-v1.json) accompany
[ADR 0003](adr/0003-versioned-build-and-observation-contracts.md).
Primary clock/numeric references are the
[W3C Web Audio specification](https://www.w3.org/TR/webaudio/) and
[RFC 8259 §6](https://www.rfc-editor.org/rfc/rfc8259.html#section-6).
They justify separating output timestamps from render-ahead time and retaining
uint64 counters as decimal strings in JSON. They do not validate our future
browser timing. Physical model/ownership research remains the existing
[gas/acoustic ADR](adr/0001-coupled-exhaust-ownership.md) and source-stage papers;
this stage introduces no changed physical equation or realism retune.

Review against `engine.h`, `rb_engine_api.c`, `pipe.h`, the existing topology
probe, `rb-core.ts`, `protocol.ts`, `rb-worklet.ts`, crank/layout conventions
and `scene.c` identifies these integration requirements:

- Existing public metadata omits important native configuration: center/link/
  junctions, catalyst area, Cd, thermal/burn/friction/starter/ECU parameters.
  The native-only bridge covers **84 active fields**; reuse the existing TS
  wrapper and add a later atomic adapter rather than rebuild physics in JS.
- Scalar physical lengths do not define fabrication routes. Current example
  centerlines and engine placement stay explicitly unresolved; a 3D route
  request rejects them. Logical chambered-can boundaries are not sealed mesh
  endpoints; candidate discretization and effective geometry remain 038/044.
- Wrapped phase alone loses cycles, spark before TDC can target the next
  combustion cycle, and command/discharge/heat cannot share one event label.
  Actual producers remain 040–042; no fake observed-event trace is introduced.
- Crank/cylinder snapshots need one physical boundary, generation/revision and
  audio epoch. Device timestamp already represents queued output; do not add
  device latency twice or count gas travel again at an aperture event.
- Internal engine pointers require stable-address handles. Initial rebuilds
  quiesce audio and prepare completely before commit; preserve old state on
  failure, invalidate histories on success and acknowledge cold reset/dyno
  detach. The actual production API and browser implementation remain 061/066.

The consumer review is an explicit check of existing interfaces and future
consumer requirements, not an independent implemented 3D/browser integration.
The contract documents unsupported regimes, capacities and snapshot costs.

## Implementation and proof boundaries

`tools/build_contract_probe.c` and its field map are native developer tools.
They export all active descriptor fields in SI, accept a bounded complete
field stream for import, validate once and initialize one real native engine,
then observe topology through the existing public pipe API. No core ABI or
production behavior changes. Makefile tracks the new object and header/include
dependencies and exposes `build-contract-check`.

`tools/build_contract.py` supplies strict versioned JSON/identity/dimension/
spatial/header checks and a serial transaction reference. Structural validation
alone does not establish physically possible engines or true producer atomicity.
`check_build_contract.py` adds the mandatory native descriptor check and checks
that the regenerated logical graph agrees, even for recognized component kinds.
Examples are committed so schema conclusions do not depend on ignored outputs.
They are physical configuration examples, not user audio or proprietary media.

## Results and failure controls

| Preset | Active SI fields | Components with runners | Unrelated components preserved after runner removal |
| --- | --- | --- | --- |
| I4 | 84 | 14 | 10 |
| I6 | 84 | 23 | 17 |
| Cross V8 | 84 | 27 | 19 |
| Flat V8 | 84 | 23 | 15 |
| Boxer4 | 84 | 16 | 12 |

All five complete SI/JSON/native round trips pass, as do all five runner-removed
native variants. Every unrelated component, both its port IDs and connections,
and all aperture source IDs/attachments are preserved exactly. Recognized but
contradictory geometry and impossible rod dimensions reject for every preset.
The same matrix passes ASan/UBSan with leak checking disabled for this sandbox.

The structural/protocol suite adds **12 tests**, covering version/features,
SI units, IDs/signs/dangling ports, unknown spatial data/arc length, malformed
JSON/counter loss, generation/revision and common clock boundaries, cycle wrap,
events, transaction preservation/history and the static audible-arrival oracle.
The full Python suite passes **57 tests**, existing web units pass **17**, and
the complete native regression suite passes **196 tests / 7897 checks**.

Five deliberately wrong copies are caught: numeric pipe-index identities,
mixed-generation snapshots, inconsistent audio-frame clocks, old history kept
after commit, and treating RPM as rad/s. Python faults compile before testing;
the C unit-conversion mutant compiles/links before the native proof rejects it.
No mutation touches the active source tree. These prove the stated checks can
fail, not all possible concurrency/model errors.

During harness development, the first contradictory-topology control assigned
a port to an already shared plenum node, so it made no change and was correctly
accepted. The harness now chooses an explicitly different existing node. That
was a control-construction mistake, not a repaired physics failure or a relaxed
acceptance threshold. The corrected adverse matrix rejects all five edits.

## Cost, preservation and reproducibility

On the existing WSL2 / Intel Core i7-1355U / GCC 15.2.0 setup, the complete
native five-engine proof takes **0.393 s wall time**, including process startup,
imports, exports and rejection controls, during a shared-host regression run.
This is cold/control-plane diagnostic cost, not real-time throughput or a
callback deadline benchmark. Production callback instrumentation added: **none**.

Compact build JSON spans **7102–12326 bytes**; the hand-defined minimal
four/eight-cylinder snapshot controls span **561–835 bytes**. At a proposed
64 KiB / 60 Hz / three-slot ceiling, payload storage is at most **192 KiB** and
copying at most **3.75 MiB/s**. Those are planning budgets; actual transport,
headers, synchronization, CPU time and worst-case deadlines remain unmeasured
until native/worklet snapshot delivery is implemented. The contract's phase,
units and unresolved geometry do not confer realism acceptance.

Reproduce from the repository root:

```sh
make BUILD=/tmp/rev-bench-s2d-next LTO=0 build-contract-check
PYTHONPATH=tools python3 -m unittest test_build_contract
python3 tools/check_contract_faults.py --probe /tmp/rev-bench-s2d-next/build_contract_probe --output renders/evidence/stage-037/faults
make BUILD=/tmp/rev-bench-s2d-next LTO=0 test
PYTHONPATH=tools .venv-analysis/bin/python -m unittest discover -s tools -p 'test_*.py'
make BUILD=/tmp/rev-bench-s2d-next-san SAN=1 LTO=0 /tmp/rev-bench-s2d-next-san/build_contract_probe
ASAN_OPTIONS=detect_leaks=0 python3 tools/check_build_contract.py --probe /tmp/rev-bench-s2d-next-san/build_contract_probe
```

The complete Python reference tests use the previously configured FFmpeg
environment; original references are not reprocessed. Web units use matching
unchanged sources in the established external dependency checkout. No production
core/TS/WASM change requires a new WASM build or sound render. Source hashes,
exact commands, logs, native/sanitizer/fault matrices, payload observations and
preservation checks live under ignored `renders/evidence/stage-037/`.

All **158 protected files** and **48 embedded listening payloads** match the
preceding checkpoint. No accepted render, original reference, production core,
WASM or Claude configuration changes. Graphify was queried before exploration
and AST-refreshed after source changes: **2597 nodes / 7007 edges / 145
communities**, retaining the known `core/include/rb.h:415` parser limitation.
No semantic-document extraction is claimed.

Documentation checks pass **113 unique stage IDs**, an acyclic dependency
graph, all **62 legacy mappings**, all **27 audit owners** and **1000 local
links**. Stage 038 is Ready only because this contract gate is complete;
none of its instrumentation is counted as implemented.

**Decision:** Stage 037's specified-contract/native-reference deliverable is
complete. Numerical correctness of these contracts is separate from realism,
production integration, concurrency and device/visual acceptance. S1/S1e/S2/S4/
full13b and C01–C06 remain open with their mapped owners. Commit/sync this
checkpoint, then await the next instruction; **038 numerical guard and flux
observability** is the next default stage, not started by this checkpoint.
