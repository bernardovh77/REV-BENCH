# Stage 132: the build saver

Done 2026-11-19 by Claude (Opus 5.5), at Bernardo's request of 2026-11-18 ("a preset saver and
storage so that I'll be able to save and come back to them later"). The card is
[docs/roadmap/07-app.md](roadmap/07-app.md#stage-132). It pulls forward the saved-builds part of
stage 095 (versioned `.rbbuild` files); 095 keeps the rest (import, export, sharing, migration).

**The question:** can an engine made in the editor be kept under a name, found again in a
list, loaded back exactly, replaced and deleted, without the window ever touching a file path?

## The gaps before it

PLAN.md's open rows (066's device checks, 118's tier misses, 129's speed for the V12-V16, 039,
041, 042, 014) don't touch keeping a build: all safe to leave.

## What was built

- **The format** (`app/src/shared/builds.ts`): one JSON file per build, `version` 1, its `name`,
  when it was saved, the engine (`build`: the preset, its layout (stage 131) and edits) and the
  engine computer's `settings` (air-fuel ratio, spark offset). Not the quality (the machine's),
  the scene or the listener: a build is the engine, not how it's heard. A file's id is its name
  made safe (`idOf`: lower-case letters and digits joined by "-"), never a path; everything
  read is checked (`parseSaved`), and what doesn't fit is not a build.
- **The files** (`app/src/main/library.ts`): the main process alone, in the user-data folder's
  `builds` (on Windows `%APPDATA%\Rev-Bench\builds`): list (newest first), read, save
  (written to a temporary file and renamed; an existing name replaced only when asked), delete
  (only a file that is a saved build). Four IPC channels, each from the app's own page only.
- **The bridge** (`window.revbench.builds`): list, read, save, remove; it takes back only what
  fits the type.
- **The engine:** `load-preset` takes a layout, so a saved build loads as one rebuild (its
  preset, layout and edits), then its settings are set; the old engine plays on if the core
  refuses it, and the window says why.
- **The window:** a "Saved builds" section at the top of the editor: a name and Save (it says
  Replace when the name is taken; it waits while a draft is open, since a build saves what
  plays), the list with each build's preset or layout, changes and date, Load, and Delete
  (asked twice: "Sure?"). A new base component, `TextField`.
- **Bernardo's LS7** (his cross-plane V8 rebuild of 2026-11-18, 42 edits, AFR 14) was saved by
  hand in this format before the saver existed: it is the first entry in his list.

## The evidence

| What | How | Result |
| --- | --- | --- |
| Saved, listed, read back whole; replaced only when asked; refused names, builds and paths; hand-edited and foreign files skipped, never deleted | `tests/saved-builds.test.ts` (the library on disk, names and ids, the LS7 file's format) | pass |
| The window's saver: what a save keeps, replace, load (engine then settings), a refused load, delete asked twice, off outside the app | `tests/saved-builds.test.ts` | pass |
| The bridge's channels and what it takes back; the page's names | `tests/preload.test.ts`, `e2e/rev-bench.spec.ts` (sandbox) | pass |
| A saved build loaded as one rebuild, another preset with its layout | `tests/engine-layouts.test.ts` | pass |
| e2e on Windows | `app/e2e/builds.spec.ts`: the four saved, a bigger bore saved, each loaded back (bore 86, then 90), a save over a name replacing it, a delete asked twice removing the file | pass |
| Planted bugs | `python tools/bugs/app_bugs_132.py APP_COPY` | 10 of 10 caught |

App and app-e2e suites pass; no C changed.

## What is left

- **Import, export and sharing** (a file to send, a code or link): stage 095/096.
- **A build loaded on a newer core** whose fields moved: today an edit the core no longer has is
  left out and logged (stage 067's rule), and a refused build keeps the old engine; 095's
  migration does better.

## Files

New: `app/src/shared/builds.ts`, `app/src/main/library.ts`, `app/src/ui/lib/builds.svelte.ts`,
`app/src/ui/components/TextField.svelte`, `app/tests/saved-builds.test.ts`,
`app/e2e/builds.spec.ts`, `tools/bugs/app_bugs_132.py`.
Changed: `app/src/shared/bridge.ts`, `protocol.ts`; `app/src/preload/bridge-impl.ts`;
`app/src/main/main.ts`, `platform.ts`; `app/src/engine/control.ts`; the store, `App.svelte`,
`EditorPanel.svelte`; tests (`preload.test.ts`, `engine-layouts.test.ts`, `rev-bench.spec.ts`).
