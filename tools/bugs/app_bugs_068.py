"""Stage 068's planted bugs in the app's TypeScript: the meter, the model's warning, the dyno's
runs, the store's wiring, the engine process's dyno commands and their trace.

    python tools/bugs/app_bugs_068.py APP_COPY

As tools/bugs/app_bugs_067.py (same APP_COPY, same way of planting and putting back).
"""
import os
import pathlib
import subprocess
import sys

INSTRUMENTS = "tests/instruments.test.ts"
STORE = "tests/engine-store.test.ts"
ENGINE = "tests/engine-dyno.test.ts"

BUGS = [
    # the meter's ballistics
    ("meter-falls-slowly", "src/ui/lib/meter.ts", "export const FALL_DB_PER_S = 20;", "export const FALL_DB_PER_S = 2;", INSTRUMENTS),
    ("meter-no-hold", "src/ui/lib/meter.ts", "      if (p >= this.holdDb) {\n        this.holdDb = p;\n        this.holdAt = seconds;\n      } else {",
     "      if (p >= this.holdDb) {\n        this.holdDb = p;\n        this.holdAt = -Infinity;\n      } else {", INSTRUMENTS),
    ("meter-clip-never", "src/ui/lib/meter.ts", "    if (peak >= 1) this.clipAt = seconds;\n", "", INSTRUMENTS),
    # the model's own warning
    ("guard-window-a-snapshot", "src/ui/lib/guard-watch.ts", "export const WINDOW_S = 1;", "export const WINDOW_S = 0.001;", INSTRUMENTS),
    ("guard-threshold-drifts", "src/ui/lib/guard-watch.ts", "export const GUARD_RATE_WARN = 1000;", "export const GUARD_RATE_WARN = 10000;", INSTRUMENTS),
    # the dyno's runs
    ("run-never-ends", "src/ui/lib/dyno.svelte.ts", "    if (s.attached !== 1) {\n      this.finish();", "    if (s.attached !== 1) {", INSTRUMENTS),
    ("every-snapshot-a-point", "src/ui/lib/dyno.svelte.ts", "    if (s.readings > this.lastReadings && s.readings > 0) {", "    if (s.readings > 0) {", INSTRUMENTS),
    ("csv-power-in-kw", "src/ui/lib/dyno.svelte.ts", "p.power.toFixed(1)]", "(p.power / 1000).toFixed(1)]", INSTRUMENTS),
    # the store's wiring
    ("store-meter-by-the-clock", "src/ui/lib/engine-store.svelte.ts", "        const seconds = meta.audioFrame / rate;", "        const seconds = 0 * rate;", STORE),
    ("store-no-dyno", "src/ui/lib/engine-store.svelte.ts",
     "        this.dyno.update({ attached: i.dyno_attached, readings: i.dyno_readings,", "        void ({ attached: i.dyno_attached, readings: i.dyno_readings,", STORE),
    # the engine process
    ("dyno-unchecked", "src/engine/control.ts", "          const bad = EngineControl.badDyno(command);", "          const bad = null;", ENGINE),
    ("dyno-during-rebuild", "src/engine/control.ts",
     '          if (this.rebuild) return this.reply(id, { error: { code: "BUSY", message: "a rebuild is in progress" } });\n          const request =\n            command.kind === "dyno-attach"',
     '          const request =\n            command.kind === "dyno-attach"', ENGINE),
    ("trace-forgets-the-dyno", "src/engine/trace-recorder.ts", "    else if (p.kind === \"line\") this.commands.push(`at ${ack.audioFrame} ${p.line}`);\n", "", ENGINE),
    ("snapshot-without-meter", "src/engine/control.ts", "        outPeak: read.outPeak, outRms: read.outRms,", "        outPeak: -1, outRms: read.outRms,", ENGINE),
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
