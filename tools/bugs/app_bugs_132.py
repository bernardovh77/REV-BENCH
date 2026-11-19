"""Stage 132's planted bugs: the build saver (its files, the bridge, the window, the engine's load).

    python tools/bugs/app_bugs_132.py APP_COPY

As tools/bugs/app_bugs_067.py (same APP_COPY, same way of planting and putting back): each bug
is a way the saver could lose a build, overwrite one unasked, reach a file outside its folder,
or load something other than what was saved.
"""
import os
import pathlib
import subprocess
import sys

SAVER = "tests/saved-builds.test.ts"

BUGS = [
    # the files
    ("replaces-unasked", "src/main/library.ts", "    if (!replace && this.read(id)) return { ok: false, reason: \"exists\" };\n", "", SAVER),
    ("ids-are-paths", "src/shared/builds.ts",
     "  return typeof id === \"string\" && /^[a-z0-9]+(-[a-z0-9]+)*$/.test(id) && id.length <= NAME_MAX;",
     "  return typeof id === \"string\" && id.length > 0;", SAVER),
    ("deletes-what-isnt-a-build", "src/main/library.ts", "    if (!isId(id) || !this.read(id)) return false;", "    if (!isId(id)) return false;", SAVER),
    ("lists-junk", "src/main/library.ts", "      if (!saved) continue;\n", "      if (!saved) { out.push({ id, name: id, savedAt: \"\", preset: \"?\", edits: 0 }); continue; }\n", SAVER),
    ("edits-unchecked", "src/shared/builds.ts",
     "  if (b.edits !== undefined && !(Array.isArray(b.edits) && b.edits.every(isEdit))) return null;\n", "", SAVER),
    # the window
    ("settings-not-loaded", "src/ui/lib/builds.svelte.ts",
     "        await this.store.setParam(name, saved.settings?.[name] ?? this.store.settingDefault(name));", "", SAVER),
    ("deletes-at-once", "src/ui/lib/builds.svelte.ts", "    if (this.confirmDelete !== id) {\n      this.confirmDelete = id;\n      return;\n    }\n", "", SAVER),
    ("saves-without-layout", "src/ui/lib/builds.svelte.ts",
     "      ...(this.store.layout ? { layout: { ...this.store.layout } } : {}),\n", "", SAVER),
    # the bridge
    ("bridge-passes-anything", "src/preload/bridge-impl.ts",
     "        return Array.isArray(list) ? (list as SavedBuildSummary[]) : [];", "        return list as SavedBuildSummary[];",
     "tests/preload.test.ts"),
    # the engine's load
    ("load-ignores-layout", "src/engine/control.ts",
     "            command.layout === undefined ? undefined : this.ownOrNullFor(command.preset, command.layout));", "            undefined);",
     "tests/engine-layouts.test.ts"),
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
