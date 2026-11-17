# Stage 067: the engine, intake and exhaust editor

Done 2026-11-17 by Claude (Opus 5.5). The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-067).

**The question:** can the app's left panel edit the engine (its geometry, cams, intake,
exhaust, the engine computer's limits and settings) from the core's own description, with every
change atomic and undoable, and is every control it offers one that actually changes the engine
or its sound?

## The gaps before it

PLAN.md's open rows: 066 waits only on Bernardo's checks of his devices and keyboard, 118 on the
four's 1.05 dB at idle; 039, 041, 042 and 014 belong to phase B or old stages. None touches what
the editor changes: it builds on 066's rebuild path (tested end to end there), so all are safe
to leave.

## What was built

**The core** (appended, as CLAUDE.md asks; no new public function):
- `RB_PARAM_AFR` (10..20, default 14.596) and `RB_PARAM_SPARK_OFFSET` (deg, −15..15, default 0),
  both live settings that rebuilds keep. The engine had the air-fuel ratio inside but nothing
  reached it, and had no spark offset at all: `computer_spark` now adds the offset to the map's
  advance (idle and overrun too; not the cranking spark, not a fixed spark). At the defaults the
  renders are bit for bit what they were (`cli_parity`, 11 of 11).

**The host:** `rb_host_engine_build` / `rb_host_engine_free`, an engine built exactly as a
prepare builds it (the same edits in the same order, refused whole if one is), never played: the
editor reads a build from it before asking for the build.

**The add-on:** `engineFields()` (each field's name, unit, range or none, flags: read-only,
integer, per bank, per cylinder, in firing order, live) and `describe(spec)` (every field's
values for a build, or the core's refusal). The fake add-on has a few fields of its own.

**The engine process** (`control.ts`): a build's edits, kept by the core's field names (names stay
meaningful across versions; ids may not) and in the autosave:
- `apply-edits`: the preset rebuilt with exactly these edits, checked whole with `describe` first,
  then prepared and swapped with the fade. One refused edit refuses the set; the engine playing
  is untouched.
- `edit-live`: a live field (idle speed, rev limit, accessory power) on the running engine, kept
  for every later build.
- `describe`: what a set of edits would make, without building it.
- **Never half-applied:** a live edit during a rebuild is refused (it would land on the engine
  being replaced), and so is a rebuild while a live edit is on its way (it would be left out);
  a second set while one is being built is refused (BUSY), the first lands whole.
- The air-fuel ratio, the spark offset and the layers heard join the settings every build keeps.

**The window:** the editor panel (`EditorPanel.svelte`, `editor.svelte.ts`):
- the core's fields in sections (Engine, Cams and valves, Intake, Exhaust, Limits and load; a
  field the core adds later shows under Other), each with its unit, range (in its tooltip),
  values per bank or cylinder (numbered), and whether it is live, needs a rebuild or is
  read-only; integer choices by name (the muffler's type, the collector, single or twin exhaust);
- **the draft:** rebuild fields wait together, with a preview of what they make (the displacement
  worked out by the core) or why the core refuses them, until Apply or Discard;
- **undo and redo** of every applied change, live ones included; a new preset starts its own;
- **units:** metric or imperial (mm/in, L/cu in, kW/hp, kg m²/lb ft²), display only;
- **Fuel and spark:** the air-fuel ratio and the spark offset;
- **Listen**, apart from the tuning: which layers are heard (pipes, flow noise, mechanical);
- a new base component, `NumberField` (commits on Enter or leaving it, never per keystroke).

The quality and volume the card lists are the centre panel's since 066; the listener's place and
the scene stay in the top bar.

## The evidence

| Card's evidence | How | Result |
| --- | --- | --- |
| Each control has a measured physical or audible effect, a valid range, reversible state and coherent geometry | `tests/test_editor_fields.c`: every field the core marks editable, on the cross-plane V8 and the four, set to another value in its range (every value of an integer field at every index, larger steps first), driven held wide open and free at idle (the rev limit also free and wide open: the limiter is the engine computer's, off while held) | 43 fields with an effect on each engine. Two fixed by the layout (no single change accepted): a cylinder's `bank`, `exhaust_systems`. Two with none, stated and held to (the test fails if one gains an effect): `position` (the 3D view's) and `bank_angle` on one bank; the editor hides them. The muffler's volume and length act only with a muffler (the V8 has a straight pipe): tested with one fitted. 2 min 40 s |
| Rapid mixed edits, stale commands and failed multi-field loads can't leave a half-applied build or double rendering | `engine-editor.test.ts` (12 tests): a set refused whole, nothing prepared; BUSY both ways between live edits and rebuilds; a second set refused while the first builds; a quality change keeps the edits, another preset doesn't. The host's swap already renders one engine at a time (stage 123's tests) | pass |
| Changing scenes or layers resets the listener's histories as specified | `test_api_listen_changes_restart_the_histories`: the pipes' layer off, and the place moved, mid-run: no step past the engine's own | pass; a scene change is a new engine behind the swap's fade |
| e2e on Windows | `app/e2e/editor.spec.ts`: the real app on the real add-on: a two-field draft previewed and applied, a live idle change, undo twice back to the preset, a twin exhaust on the four refused in the preview and kept out | pass |

**Found on the way (fixed):**
- **The autosave kept only the preset and quality** (since 066): the supervisor rebuilt the
  saved build from those two fields, so a restart lost the scene and the latency. Now it keeps
  the whole build, edits included (`supervisor.test.ts`).
- **The editor's sections snapped shut on every rebuild** (their open state was re-applied):
  they now stay as the user left them.
- **A reactive slip:** the editor's draft count was first read before a preset was tracked, so
  Svelte never learned it depended on the draft; the tracked preset is state now.

**Planted bugs.** C (`tools/bugs/stage067.py`, rb_tests): 6 of 6 caught: the
air-fuel ratio never applied, the spark offset ignored or backwards, the listener not restarted,
a field that silently stops working (the catalyst's resistance never applied) and a stated
no-effect field that starts having one (the cylinders' position moving the catalyst). Host (`stage067h.py`,
rb_host_tests): 3 of 3 caught (any rate taken, a leaked engine, the edits skipped).
TypeScript (`python tools/bugs/app_bugs_067.py APP_COPY`): 15 of 15 caught, after two missed at
first gained tests (the air-fuel ratio surviving a rebuild; the autosave keeping edits).

**Speed (`tools/speed_check.py HEAD --quality low,balanced,lean,high`, MSVC, the fast cores):**
median +0.1 % over 20 cells, inside the 3 %. One cell read −6.8 % (the six at low), alone among
the 20: the kind of single-cell noise stage 127 saw (a re-run there read the same before and
after). The absolute speeds were low that day (the V8s at lean 1.15 and 1.27 times real time,
the flat-plane's 1.53 of stage 127 being gcc on another day): the laptop's speed moves about
10 % from day to day, and only the ratios inside one run mean anything.

## What is left

- **Layout-fixed fields** (`bank`, `exhaust_systems`): they change only with other fields at
  once (a cylinder to the other bank with one coming back; two collectors before a twin
  exhaust). The editor offers them, and the preview says why a single change is refused; a
  layout editor that moves several at once is for later (the 3D view's stages, 070-077).
- **Bernardo's listening:** whether each control sounds as it should is phase B's question, not
  this stage's; the stop rule holds: an accepted combination is not claimed credible.

## Files

New: `app/src/ui/lib/editor.svelte.ts`, `units.ts`, `app/src/ui/components/NumberField.svelte`,
`app/tests/editor.test.ts`, `engine-editor.test.ts`, `units.test.ts`, `app/e2e/editor.spec.ts`,
`tests/test_editor_fields.c`, `tools/bugs/stage067.py`, `stage067h.py`, `app_bugs_067.py`.
Changed: `core/include/rb.h` (two parameters), `rb_params.c`, `rb_drive.c`, `engine.c`/`.h`
(the spark offset); `host/include/rb_host.h`, `host/src/sim_build.c`; the add-on
(`rbn_meta.c`, `rbn.h`, `rbn_module.c`); the contracts (`addon.ts`, `protocol.ts`,
`process-messages.ts`); the engine process, the store and client, the editor panel, `App.svelte`,
the supervisor and the autosave; `web/src/wasm` (`make wasm`).
