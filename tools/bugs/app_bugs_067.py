"""Stage 067's planted bugs in the app's TypeScript: the editor, its engine side, the autosave.

    python tools/bugs/app_bugs_067.py APP_COPY

As tools/bugs/app_bugs_066.py (same APP_COPY, same way of planting and putting back): each bug
is a way the editor could leave a half-applied build, lie about what plays, or lose an edit.
"""
import os
import pathlib
import subprocess
import sys

BUGS = [
    # the engine side: one transaction, never half-applied
    ("set-not-checked-first", "src/engine/control.ts", "    if (kept && kept.length) this.describeChoice(choice);\n", "",
     "tests/engine-editor.test.ts"),
    ("live-during-rebuild", "src/engine/control.ts",
     '    // during a rebuild the edit would reach the engine on its way out and miss the new one\n'
     '    if (this.rebuild) return this.reply(id, { error: { code: "BUSY", message: "a rebuild is in progress" } });\n', "",
     "tests/engine-editor.test.ts"),
    ("rebuild-during-live", "src/engine/control.ts",
     '      if (p.liveEdit) return this.reply(id, { error: { code: "BUSY", message: "a live edit is still on its way" } });\n', "",
     "tests/engine-editor.test.ts"),
    ("same-preset-drops-edits", "src/engine/control.ts",
     "const kept = edits ?? (preset === this.choice.preset ? this.choice.edits : undefined);", "const kept = edits;",
     "tests/engine-editor.test.ts"),
    ("live-edit-not-kept", "src/engine/control.ts",
     "        this.choice = { ...this.choice, edits: EngineControl.withEdit(this.choice.edits ?? [], pending.liveEdit) };\n", "",
     "tests/engine-editor.test.ts"),
    ("live-on-a-rebuild-field", "src/engine/control.ts", '    if (!field.flags.includes("live")) {', "    if (false) {",
     "tests/engine-editor.test.ts"),
    ("sticky-afr-lost", "src/engine/control.ts",
     '["listener", "variant", "afr", "spark_offset", "layers"]', '["listener", "variant", "spark_offset", "layers"]',
     "tests/engine-control.test.ts tests/engine-editor.test.ts"),
    # the window's editor
    ("preview-race", "src/ui/lib/editor.svelte.ts", "      if (seq !== this.previewSeq) return; // a newer change asked again meanwhile\n", "",
     "tests/editor.test.ts"),
    ("redo-kept-after-change", "src/ui/lib/editor.svelte.ts", "    this.undoStack = [...this.undoStack, before];\n    this.redoStack = [];",
     "    this.undoStack = [...this.undoStack, before];", "tests/editor.test.ts"),
    ("refused-apply-drops-draft", "src/ui/lib/editor.svelte.ts", "    } catch (e) {\n      this.error = describe(e);\n    } finally {\n      this.busy = false;\n    }\n  }\n\n  async undo",
     "    } catch (e) {\n      this.error = describe(e);\n      this.draft = {};\n    } finally {\n      this.busy = false;\n    }\n  }\n\n  async undo", "tests/editor.test.ts"),
    ("history-across-presets", "src/ui/lib/editor.svelte.ts", "    return this.store.preset === this.historyPreset;", "    return true;",
     "tests/editor.test.ts"),
    ("hidden-shown", "src/ui/lib/editor.svelte.ts", "    const all = this.store.engineFields.filter((f) => !hidden(f));", "    const all = this.store.engineFields;",
     "tests/editor.test.ts"),
    ("inches-wrong", "src/ui/lib/units.ts", "const INCH = 0.0254;", "const INCH = 0.025;", "tests/units.test.ts tests/app.test.ts"),
    # the autosave keeps the whole build
    ("supervisor-saves-part", "src/main/supervisor.ts", "        const { type: _type, ...choice } = message;",
     "        const choice: BuildChoice = { preset: message.preset };", "tests/supervisor.test.ts"),
    ("autosave-drops-edits", "src/main/autosave.ts", "...(Array.isArray(edits) && edits.length && edits.every(isEdit) ? { edits: edits as EngineEdit[] } : {}),", "",
     "tests/main-pieces.test.ts tests/supervisor.test.ts"),
]


def main():
    """Plants each bug in the copy named on the command line, runs its tests, puts it back."""
    os.chdir(sys.argv[1])
    for name, path, old, new, tests in BUGS:
        p = pathlib.Path(path)
        orig = p.read_text(encoding="utf-8")
        if orig.count(old) != 1:
            print(f"{name}: NOT APPLIED ({orig.count(old)} matches)")
            continue
        p.write_text(orig.replace(old, new), encoding="utf-8")
        try:
            r = subprocess.run("npx vitest run " + tests, shell=True, capture_output=True, timeout=300)
            print(f"{name}: {'caught' if r.returncode else 'MISSED'}", flush=True)
        finally:
            p.write_text(orig, encoding="utf-8")


if __name__ == "__main__":
    main()
