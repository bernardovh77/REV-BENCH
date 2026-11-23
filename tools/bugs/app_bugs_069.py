"""Stage 069's planted bugs in the app's TypeScript: the keep-up warning's advice, the steadied
boost-or-vacuum readout, the battery question and the builds' rest.

    python tools/bugs/app_bugs_069.py APP_COPY

As tools/bugs/app_bugs_068.py (same APP_COPY, same way of planting and putting back).
"""
import os
import pathlib
import subprocess
import sys

KEEP_UP = "tests/keep-up.test.ts"
INSTRUMENTS = "tests/instruments.test.ts"
APP = "tests/app.test.ts"
PRELOAD = "tests/preload.test.ts"
ENGINE = "tests/engine-control.test.ts"

BUGS = [
    # the keep-up advice
    ("too-slow-never", "src/ui/lib/keep-up.ts", "  if (share !== null && share >= TOO_SLOW_SHARE) {", "  if (share !== null && share > 1) {", KEEP_UP),
    ("latency-goes-down", "src/ui/lib/keep-up.ts", "  return LATENCIES.find((l) => l > ms) ?? null;", "  return LATENCIES.find((l) => l < ms) ?? null;", KEEP_UP),
    ("battery-unsaid", "src/ui/lib/keep-up.ts", "  const battery = onBattery ? \" It's on battery", "  const battery = false ? \" It's on battery", KEEP_UP),
    ("bar-offers-quality-always", "src/ui/panels/TopBar.svelte", "      {:else if advice.action?.kind === \"latency\"}", "      {:else if false}", APP),
    # the readout
    ("boost-on-a-hair", "src/ui/lib/manifold.ts", "  if (above > BOOST_DEADBAND_PA) return", "  if (above > 0) return", INSTRUMENTS),
    ("vacuum-negative", "src/ui/lib/manifold.ts", "  return { label: \"Vacuum\", value: Math.max(0, -above) };", "  return { label: \"Vacuum\", value: -above };", INSTRUMENTS),
    # the battery question and the builds
    ("battery-anything-truthy", "src/preload/bridge-impl.ts", "    onBattery: async () => (await env.ipc.invoke(IPC.onBattery)) === true,", "    onBattery: async () => Boolean(await env.ipc.invoke(IPC.onBattery)),", PRELOAD),
    ("builds-never-rest", "src/engine/control.ts", "export const APP_BUILD_PARAMS: Record<string, number> = { hold: 0, rest: 1 };", "export const APP_BUILD_PARAMS: Record<string, number> = { hold: 0 };", ENGINE),
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
