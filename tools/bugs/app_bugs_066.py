"""Stage 066's planted bugs in the app's TypeScript (the engine process and the window).

    python tools/bugs/app_bugs_066.py APP_COPY

APP_COPY is a copy of app/ with its node_modules (tools/run_tests.py app makes one at
<out>/app-copy/app). Each bug is planted in turn in the copy, the named vitest files run, and
the file is put back; a bug is caught when the run fails. (plant_bugs.py is for C: its tables
are tools/bugs/stage*.py.) Each entry: (name, file under app/, old text, new text, tests).
"""
import os, pathlib, subprocess, sys
BUGS = [
 ("guard-never-ticked", "src/engine/control.ts", "      this.guardThrottle(host);\n", "", "tests/engine-control.test.ts"),
 ("guard-timeout-reversed", "src/engine/throttle-guard.ts", "if (now - this.lastBeat < DEADMAN_TIMEOUT_MS) return null;", "if (now - this.lastBeat > DEADMAN_TIMEOUT_MS) return null;", "tests/throttle-guard.test.ts"),
 ("detach-keeps-throttle", "src/engine/control.ts", "    this.guard.release(this.now());\n", "", "tests/engine-control.test.ts"),
 ("ramp-is-a-cut", "src/engine/throttle-guard.ts", "    return ramp.value * left;", "    return 0;", "tests/throttle-guard.test.ts"),
 ("first-tick-lost", "src/ui/lib/throttle-input.ts", "    this.lastTick = now;\n    this.emit(now);", "    this.emit(now);", "tests/throttle-input.test.ts"),
 ("blur-keeps-analog", "src/ui/lib/throttle-input.ts", "      this.value = 0;\n      this.emit(now);\n    }\n  }\n\n  /** Every frame", "    }\n  }\n\n  /** Every frame", "tests/throttle-input.test.ts tests/driver.test.ts"),
 ("no-beat", "src/ui/lib/throttle-input.ts", "if (this.live && this.value > 0 && now - this.lastSent >= BEAT_MS) this.emit(now);", "", "tests/throttle-input.test.ts"),
 ("sticky-not-built", "src/engine/control.ts", "{ ...APP_BUILD_PARAMS, ...this.sticky, master_gain", "{ ...APP_BUILD_PARAMS, master_gain", "tests/engine-control.test.ts"),
 ("volume-not-built", "src/engine/control.ts", "master_gain: this.gainFor(choice, this.volume) };", "master_gain: this.gainFor(choice, 1) };", "tests/engine-control.test.ts"),
 ("recorder-keeps-stale", "src/engine/trace-recorder.ts", "    if (ack.outcome !== \"ok\") return;\n", "", "tests/trace-recorder.test.ts"),
 ("swap-at-ack-frame", "src/engine/trace-recorder.ts", "ack.audioFrame - swapFadeFrames(this.rate)", "ack.audioFrame", "tests/trace-recorder.test.ts"),
 ("device-loss-silent", "src/engine/control.ts", "    this.postDevice(\"lost\", -1);\n", "", "tests/engine-control.test.ts"),
 ("no-quit-fade", "src/engine/control.ts", "      host.setParam(param.id, 0);\n    } catch (e) {\n      this.deps.log(\"warn\", `the quit's", "    } catch (e) {\n      this.deps.log(\"warn\", `the quit's", "tests/engine-process.test.ts"),
 ("letgo-on-hide-missing", "src/ui/lib/driver.svelte.ts", "if (this.deps.document.visibilityState !== \"visible\") this.letGo();", "", "tests/driver.test.ts"),
 ("blind-not-blind", "src/ui/lib/driver.svelte.ts", "const candidateSide = this.blind ? (this.blindB ? \"B\" : \"A\") : \"B\";", "const candidateSide = \"B\";", "tests/driver.test.ts"),
 ("trims-ignored", "src/engine/control.ts", "(PRESET_TRIM[choice.preset] ?? 1)", "1", "tests/engine-control.test.ts"),
 ("frame-gap-ignored", "src/ui/lib/driver.svelte.ts", "if (this.lastFrame !== null && now - this.lastFrame > FRAME_GAP_MS) this.letGo();", "", "tests/driver.test.ts"),
 ("repeat-reopens", "src/ui/lib/driver.svelte.ts", "if (event.repeat && !this.throttlePressed) return;", "", "tests/driver.test.ts"),
 ("learn-keeps-pressed", "src/ui/lib/gamepad.ts", "if (pad.buttons[i].value - (rest.buttons[i] ?? 0) >= LEARN_THRESHOLD)", "if (pad.buttons[i].value >= LEARN_THRESHOLD)", "tests/gamepad.test.ts"),
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
