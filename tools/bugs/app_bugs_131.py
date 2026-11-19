"""Stage 131's planted bugs in the app's TypeScript: layouts in the engine process, the editor,
the shared rule, the autosave and the trace.

    python tools/bugs/app_bugs_131.py APP_COPY

As tools/bugs/app_bugs_067.py (same APP_COPY, same way of planting and putting back): each bug
is a way a layout change could keep an edit of the old layout's cylinders, lose the layout, or
say something else plays.
"""
import os
import pathlib
import subprocess
import sys

LAYOUTS = "tests/engine-layouts.test.ts"
EDITOR = "tests/editor.test.ts"

BUGS = [
    # the engine process
    ("set-layout-keeps-indexed-edits", "src/engine/control.ts",
     "    const kept = sameLayout(wanted ?? undefined, this.choice.layout, own) ? edits : editsForLayout(edits, this.fields).kept;",
     "    const kept = edits;", LAYOUTS),
    ("other-preset-keeps-layout", "src/engine/control.ts",
     "(preset === this.choice.preset ? this.choice.layout : undefined)", "this.choice.layout", LAYOUTS),
    ("layout-quality-unsaid", "src/engine/control.ts",
     " : choice.layout ? { quality: this.layoutQuality(choice.layout) } : {}),", " : {}),", LAYOUTS),
    ("own-layout-kept", "src/engine/control.ts",
     "    return layout && !sameLayout(layout, undefined, own) ? layout : null;", "    return layout;", LAYOUTS),
    ("trim-by-preset-only", "src/engine/control.ts",
     "    if (choice.layout) return choice.layout.cylinders <= 4 ? PRESET_TRIM.i4 : 1;\n", "", LAYOUTS),
    ("saved-layout-not-checked", "src/engine/control.ts",
     "    if (this.choice.layout || this.choice.edits?.length) {\n      try {", "    if (false) {\n      try {", LAYOUTS),
    ("recorder-forgets-layout", "src/engine/trace-recorder.ts", "    if (spec.layout) {\n      const l = spec.layout;",
     "    if (false) {\n      const l = spec.layout!;", LAYOUTS),
    ("autosave-drops-layout", "src/main/autosave.ts",
     "      ...(layout !== undefined && badLayout(layout) === null ? { layout: layout as LayoutSpec } : {}),", "", LAYOUTS),
    # the rule both sides share
    ("indexed-edits-kept", "src/shared/layout-edits.ts",
     "  return field !== undefined && !field.flags.some((f) => INDEXED.includes(f));", "  return field !== undefined;", LAYOUTS + " " + EDITOR),
    # the window's editor
    ("angle-outside-its-range", "src/ui/lib/editor.svelte.ts",
     "      bankAngle = asked >= crank.angleMin && asked <= crank.angleMax ? asked : crank.angleUsual;", "      bankAngle = asked;", EDITOR),
    ("draft-keeps-dropped", "src/ui/lib/editor.svelte.ts",
     "    return { edits: editsForLayout(edits, this.store.engineFields).kept, layout };", "    return { edits, layout };", EDITOR),
    ("undo-forgets-layout", "src/ui/lib/editor.svelte.ts",
     "      await this.store.applyEdits(target.edits, target.layout);", "      await this.store.applyEdits(target.edits);", EDITOR),
    ("old-cylinders-shown", "src/ui/lib/editor.svelte.ts",
     "    if (this.layoutChanging && !keptAcrossLayouts(field)) return base;\n", "", EDITOR),
    ("discard-keeps-layout", "src/ui/lib/editor.svelte.ts",
     "  discard(): void {\n    this.draft = {};\n    this.layoutDraft = null;", "  discard(): void {\n    this.draft = {};", EDITOR),
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
