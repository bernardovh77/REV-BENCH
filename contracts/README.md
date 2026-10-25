# Shared contracts — version 1

Stage 037 defines the common description and handoff rules for physics, audio,
browser editing, persistence and 3D. This is a **specified contract with a native
reference proof**, not a newly integrated engine API, geometry compiler,
worklet protocol, event producer or 3D implementation. The production renderer
remains undecided. [PLAN.md](../PLAN.md) owns execution and status.

The normative version-1 structures are below. The executable structural
reference is [build_contract.py](../tools/build_contract.py); complete engine
units/shapes are [engine-fields-v1.json](engine-fields-v1.json). These are one
schema, not a second independently maintained browser physics model. The
[five native examples](examples/builds-v1.json) are generated from current
presets, not reference-recording measurements or calibrated fabrication drawings.

## Versioning, identity and limits

All messages/files carry `schema_version: 1`. Unknown versions, unknown fields
outside `extensions`, unknown required features and unsupported topology fail
explicitly. Migration creates a newly validated version; it never silently drops
components. IDs are stable strings, not array indices, pointers or numeric pipe
handles. `<build_id>/<role>-<slot>` names a physical/logical role; the snorkel
retains `<build_id>/snorkel`. Two component ports append `:0` and `:1`; source
declarations append `/radiation`. Bank/cylinder IDs use fixed build slots,
independent of firing order. Retaining an identity is appropriate only when its
physical meaning is retained. Replacing a role with a different component kind
requires a new identity. Native pointer/index lookup tables belong to a prepared
generation and are never serialized.

`build_id` is a persistent lowercase ASCII namespace (1–64 characters).
`generation` is a monotonically increasing uint64 decimal **string**; zero is
valid, leading zeros/signs are not. The same representation applies to tick,
frame, cycle, event and snapshot counters. A separate `revision` identifies the
immutable configuration within a generation: cold reset keeps it, structural
edits increment it, and future acknowledged live tuning can increment it without
resetting the physical engine. Binary transports may use uint64 or
explicit high/low uint32 words. JS consumers must not coerce these counters to
`number`: [RFC 8259 §6](https://www.rfc-editor.org/rfc/rfc8259.html#section-6)
only guarantees interoperable integer values within the binary64 exact range.
Overflow requires a new session; wrapping into an earlier identity is forbidden.

Version 1 supports current four-stroke, forward-crank descriptions: 1–16
cylinders, 1–2 banks, the eight legacy pipe roles, at most 128 components,
256 logical nodes, 256 ports, 64 aperture declarations and 128 points per
centerline. A build is at most 1 MiB UTF-8. All numbers must be finite; bools
are not numeric fields. Duplicate JSON keys reject. JSON key order/whitespace
are immaterial. The reference serializer is deterministic within its Python
implementation; it is **not** a cross-language canonical-hash standard.

These capacities are storage limits, not physically qualified operating ranges.
Forward crank does not prohibit signed/reversing gas flow; the latter still
requires the appropriate solver/thermal/interface validation gates.
Native `rb_engine_desc_check` and candidate-specific topology/domain/capacity
checks remain mandatory before use. C07 finite B0 guards remain mandatory too.
Turbo, damage, two-stroke, moving coefficients and arbitrary new component kinds
are unsupported version-1 required features; future stages add capability-
negotiated typed payloads and migrations. `extensions` accepts only `x-*`
annotations, preserved on round trip and prohibited from activating physics.

## Engine build

| Field | Required shape / meaning |
| --- | --- |
| `schema_version`, `kind` | Integer 1, string `engine-build`. |
| `build_id`, `generation`, `revision` | Namespace, prepared-state generation and immutable configuration revision above. |
| `parameters` | Exactly the 84 fields in the unit catalog; each value is a numeric array, including a one-element scalar. Bank/cylinder arrays contain exactly the active count. Integer fields stay integers. |
| `geometry` | The logical component/port graph below, derived from these parameters in version 1. |
| `sources` | Aperture declaration array below; future causal source types require capability/schema additions. |
| `required_features` | Empty array in the current executable proof; unknown features reject. |
| `extensions` | Namespaced nonphysical annotations only. |

SI is authoritative: m, m², m³, Pa, K, s, kg, W, N·m, radians and rad/s.
Crank phase advances through **4π radians per four-stroke cycle**. The native
core keeps degrees/RPM internally; the native-only bridge converts once on
entry/exit, including idle proportional/integral gains. Schema names/units do
not change the existing core ABI. `build_contract_fields.inc` maps every
active `rb_engine_desc` member, including valve/throttle Cd, catalyst area,
intermediate exhausts, ambient/thermal/burn/friction/starter/ECU settings.
Inactive fixed-array storage and C padding have no physical serialization meaning.
No controller, current crank, gas inventory, noise seed state or DSP history is
smuggled into a build; those belong to running state or a future explicit
restart-state format. Model constants outside the description remain locked to
the implementation revision and validated model, not user-editable parameters.

Catalog `shape` determines array length, not semantic reindexing:
`firing_order[k]` is the cylinder slot firing in chronological rank k;
`firing_angle[i]`, `bank[i]`, `pin[i]`, `position[i]`, `header_length[i]` and
`collector[i]` remain indexed by persistent cylinder slot i. Bank arrays use
persistent bank slots. Shuffling a firing order must not rename cylinders.

Round-trip means preserving active parameter values (integer equality and
relative `1e-14` / absolute `1e-15` for SI↔native doubles), then reproducing the
same logical graph. It is not a sequence of `rb_set_engine` calls: that would
rebuild/reset multiple times and may temporarily create impossible layouts.

## Geometry, connections and frames

`geometry` has exactly `representation: "legacy-logical"`, `frame: "engine"`,
`engine_to_vehicle`, `components` and `nodes`. A component contains `id`,
`kind`, positive `length_m`, positive `area_m2`, two `ports` and
`centerline_m`. The role set is runner, header, exhaust, tailpipe, muffler,
snorkel, center and link. The physical recipe also retains the corresponding
finite volumes and restriction/packing parameters in `parameters`.

A port contains `id`, `node_id` and `axis_sign`: +1 at end 0, −1 at end 1.
Axial pipe flow goes from 0 to 1; `Q_into_component = axis_sign * Q_axial`.
At a lossless zero-volume connecting node, sum of component inflows is zero;
a finite junction owns an explicit inventory and its conservation law instead.
The B0 API's flow **into a junction** has the opposite orientation at that
connection. Units are pressure Pa and volume flow m³/s; gas mass flow/species,
momentum and enthalpy/total-energy transfers retain their separate SI books.
The active solver owns each physical region/load exactly once, per
[ADR 0001](../docs/adr/0001-coupled-exhaust-ownership.md).

Nodes have `id` and a supported semantic `kind`: plenum, intake/exhaust valve,
collector, muffler inlet/outlet, airbox, center inlet/outlet, junction, or
ambient intake/exhaust. Intake and exhaust ambient terminals do not become a
zero-delay acoustic shortcut through a shared ambient node. Every node is used
and every port resolves; duplicate IDs and reversed end signs reject. The
native proof also rejects a graph that is structurally valid but contradicts
the graph regenerated from the complete description. List order is not intended
as identity; the version-1 proof preserves producer order for deterministic
comparisons. Reordering alone requires normalization before this strict proof.

This is a **logical boundary graph**, not the production solver's staggered
cell graph: chambered cans have sealed mesh endpoints and internal cell
attachments that must not be mistaken for missing physical connections. Cell
indices, discretization, guard-modified effective geometry, all-port scattering,
finite junctions, losses and their observability remain Stages 038/043–052.
The [Stage044 compiler](../docs/stage-044.md) now prepares the bounded legacy
subset into gas region/port/cell incidence and an executable lossless B0
geometry control. Catalyst/packing/baffle metadata and finite volumes survive;
full component physics, general graph coupling and production promotion remain
later gates. The engine recipe stays authoritative and derived geometry cannot
be edited independently. The Stage037 reference alone remains a contract proof.

Frames are right-handed with metres: vehicle +X right, +Y toward the front,
+Z up; engine +Y toward the crank front and +Z upward. The engine origin is
the crank center at cylinder-zero's axial station. Positive cylinder axial
position measured from the front maps along **−Y**:
`y_i = -(position[i] - position[0])` for this engine origin. A bank axis is
`[sin(bank_angle), 0, cos(bank_angle)]`. The drawing crank angle is
`theta + pin[i] + bank_angle[bank[0]] - pin[0]`; this retains the core's
cylinder-zero TDC origin instead of treating crank-pin angle as a firing phase.
Wrapping only supplies local geometry; it never destroys the cycle counter.

`engine_to_vehicle` is null when placement is unresolved, otherwise a metre
translation and unit quaternion `{translation_m: [x,y,z], rotation_xyzw:
[x,y,z,w]}`. `centerline_m` is null when unresolved, otherwise 2–128 points in
the engine frame, in port-0→port-1 order. Its polyline arc length must equal
the declared physical length within `1e-8` relative / `1e-10 m` absolute.
That is a consistency tolerance, not metrology accuracy. Smooth splines require
an explicitly controlled arc-length interpretation in the later compiler.

The current preset examples leave placement/centerlines **null**: existing
effective lengths and hardcoded listener positions do not establish a surveyed
exhaust installation. Physics may use its validated effective lengths; a
fabrication/3D route request rejects unresolved geometry. No automatically drawn
straight pipe is claimed to be the real routed exhaust. Three.js axis conversion
belongs in one rendering adapter; it must not alter physical frame data.

A source declaration contains `id`, `component_id`, `port_id`, `kind:
"aperture"`, `implemented: false`. Its future physical aperture data are
pressure/volume velocity at that identified terminal; it must distinguish
loading from radiated pressure at a listener. False means the **shared contract
binding** is not yet implemented; existing native sound sources still exist.
Future cylinder combustion/jet/injector/structure sources use stable cylinder/
component identities and typed physical quantities, not a generic gain array.

## Consumer review before implementation

| Consumer reviewed against current code | Contract decision / follow-up |
| --- | --- |
| Physics: `engine.h`, `rb_engine_api.c`, `pipe.h`, native topology probe | Complete SI descriptor and logical connectivity cover all five presets; preserve C's checks, internal pointer ownership, units and separate gas/acoustic books. 038 instruments effective geometry; 044 compiles physical geometry; 061 supplies production bulk prepare/commit. |
| Browser: `rb-core.ts` (`EngineDescription`, `RbCore`, `RbSim`), `protocol.ts`, `rb-worklet.ts` | Reuse existing metadata/read/stereo bindings. Add one adapter to the versioned description and an eventual atomic bulk API; do not maintain a parallel physics field table or run per-field rebuild loops. Current worklet still emits mono tone and unversioned telemetry. Runtime/lifecycle delivery is 066–069. |
| 3D: native crank/layout conventions, `scene.c`, planned 070–077 | Stable identities plus coherent snapshots drive kinematics. Require explicit spatial placement/centerlines before routing; do not infer them from cell order or legacy source positions. Gas/flame visuals remain labeled 0D/1D illustrations. A consumer-contract review is recorded here, not an independent implemented 3D test. |
| Save/share: planned 095–096 | Serialize validated builds, not native memory. Preserve annotations, reject missing required features, migrate explicitly and enforce size/count limits. Do not treat this developer proof as completed user import/export. |

Event/time/snapshot/rebuild rules and their bounded reference tests are in
[runtime-v1.md](runtime-v1.md). Execution evidence is [Stage 037](../docs/stage-037.md).
