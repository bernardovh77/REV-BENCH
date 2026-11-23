"""Stage 133's planted bugs in the app's TypeScript: the 2-step's key and the editor's
Rev limiter section.

    python tools/bugs/app_bugs_133.py APP_COPY

As tools/bugs/app_bugs_068.py (same APP_COPY, same way of planting and putting back).
"""
import os
import pathlib
import subprocess
import sys

DRIVER = "tests/driver.test.ts"
EDITOR = "tests/editor.test.ts"

BUGS = [
    ("launch-not-released", "src/ui/lib/driver.svelte.ts",
     "    else if (event.code === LAUNCH_KEY && this.launchHeld) this.releaseLaunch();\n", "", DRIVER),
    ("launch-held-on-blur", "src/ui/lib/driver.svelte.ts", "    if (this.launchHeld) this.releaseLaunch();\n", "", DRIVER),
    ("launch-on-repeat", "src/ui/lib/driver.svelte.ts",
     "    } else if (event.code === LAUNCH_KEY && !event.repeat) {", "    } else if (event.code === LAUNCH_KEY) {", DRIVER),
    ("no-limiter-section", "src/ui/lib/editor.svelte.ts",
     '  { title: "Rev limiter", fields: ["rev_limit", ', '  { title: "Limits", fields: ["rev_limit", ', EDITOR),
    ("cut-unnamed", "src/ui/lib/editor.svelte.ts", '  limiter_cut: ["Spark cut", "Fuel cut"],\n', "", EDITOR),
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
