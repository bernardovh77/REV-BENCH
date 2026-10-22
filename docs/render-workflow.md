# Current sound results

Latest selection (2026-10-22): restore the accepted pre-13b-r I6 and flat-V8
exhaust sounds; I6 keeps 8500 rpm, flat V8 stays 9000 rpm. Cross V8 keeps the
sharper 7500-rpm rear-delete X. See [rollback notes](stage-13b-r-rollback.md).
The single result and long-coast workflow below is retained.

2026-10-22. Bernardo accepted the researched exhaust changes: "I consider the
changes a success" and "a lot better". He requested removal of old renders and
one result file at the end of each checkpoint, including the long coasts and
other examples. Sound acceptance applies to this checkpoint; unresolved S1
combustion/repetition checks and 13b solver/performance work remain open.

## One file to open

Open `renders/RESULTS.html` in a browser. Every listening clip is embedded, so
the file can be copied and played offline on its own. Each engine starts with
its full pull and eight-second loaded coast, with pedal-lift and clutch-in
times shown. Expand its other examples for held speed, neutral revs, and the
available listener positions and exhaust alternatives. Players pause one
another so clips do not overlap.

The retained current set has **34 recordings**: 23 standard examples, five
held-3000-rpm researched configurations, five extended coasts and the separate
cross-V8 packed-muffler comparison. Stage 13b-r makes the rear-delete route
the main cross-V8 configuration; it is a physical modification, not the
catalog Borla kit. The I6 and V8s have buttons to jump directly to their last
1000 rpm. Current limits are 8500/7500/9000 rpm. Sources, numerical validation
and changed coast behavior are in `docs/stage-13b-r.md`.

The portable page contains PCM16 listening copies, using one fixed scalar
over each complete clip to put its peak at 0.6. This preserves relative level
through lift-off and the coast. There is no EQ, denoising, compression, time
stretch or pitch change. Different clips have different preview gains, so
their playback volumes are not calibrated comparisons. The original float
WAVs remain unfiltered and byte-identical; use them for all measurements.
`renders/current/results.json` and the page's embedded manifest record each
source hash, preview hash/gain, duration and coast markers.

The cleanup removed **360 superseded generated audio files**, including
duplicate copies under the previous layouts. All **34 current float WAVs**
and all **59 reference-library files** were verified byte-for-byte before
and after cleanup. Historical comparison numbers and source hashes remain
available without retaining their obsolete audio.

## Produce the next checkpoint

```
make render
```

This builds the native renderer, records the standard float set, all five
extended coast scenarios, five held examples and the cross-V8 packed-control
coast in a temporary staging folder. The publisher validates the complete
34-file inventory, finite audio, output headroom and loaded-coast timing
before installing `renders/current/` and `renders/RESULTS.html`. Only then is
the preceding current set removed. A failed render or validation retains the
last complete result. Reference folders are never passed to deletion code.
The I6 pull guard is 20 seconds so it can reach the raised limiter; the
others retain 12 seconds. The three revised engines must reach within 150 rpm
of their limiter settings, as well as retaining the loaded-coast duration.

To repackage current recordings without changing or rerendering them:

```
make results
```

Python's standard library suffices for both commands. Float-WAV container
parsing is shared with the analysis reader. Analysis still uses the existing
`requirements-analysis.txt` environment.

For a deliberate isolated research comparison, use
`make render-standard RENDER_DIR=renders/work/CONTROL RENDER_FLAGS=--float`
and `tools/exhaust_feedback.py --cli PATH --output renders/work/CONTROL-coasts`.
`--before-cli` remains optional for a temporary before/after measurement.
The analysis command accepts explicit `--previous-coasts` and
`--constant-coasts` paths; it no longer silently relies on old retained audio.
Save the numerical results and remove temporary generated audio after the
comparison is complete. Finish the checkpoint with the single current result.

## Local layout and retention

```
renders/
  RESULTS.html                # the single listening deliverable
  current/standard/           # current unfiltered float originals + telemetry
  current/feedback/           # current held and long-coast originals + provenance
  current/results.json        # source/preview hashes and processing
  real/                      # original reference excerpts — preserved
  user-reference-previews/    # reference/filtered listening library — preserved
  evidence/13b-p/             # retained physics/dyno/timing/rejected solver evidence
  evidence/history/           # older non-audio measurements, logs and binaries
  evidence/retention.json     # cleanup inventory and verification
```

All audio remains ignored by Git. Historical Markdown stage notes and saved
commands retain their original paths as provenance; superseded audio at those
paths was explicitly deleted. Retained current files have a relocation ledger.
The reference library's source intervals, filtering and cabin/exterior/flyby
uncertainty remain in `docs/user-sound-references.md`. The original user MP3s
outside the repository are untouched. Claude's setup is preserved.

## Validation of the file-management change

Native **173 tests / 3252 checks**, Python **27 tests**, and web **17 tests**
pass. The seven publishing tests cover constant-gain waveform preservation,
incomplete/short-coast rejection, failed-install rollback, successful
replacement with reference preservation, invalid audio, and operation without
optional analysis packages. A full orchestration test runs the actual Makefile
and scenario scripts with a substitute for the expensive simulation, then
validates and publishes all 34 files without optional packages. Chromium opened the standalone file offline and
played all **34** embedded clips, verified players pause one another, and made
no network requests. Source float audio and simulation code are unchanged.
The complete replacement sequence also succeeded on the actual project
drive, with current/reference hashes rechecked afterward. Graphify was queried
before exploration and refreshed via AST: **1808 nodes / 4932 edges / 92
communities**, with the previously documented `rb.h` parser limitation.

That section records the original cleanup validation. Stage 13b-r subsequently
passes 175 native tests, 29 Python tests and the updated native/WASM/browser
checks in its stage note. The final portable file is checked for all 34
players, all four top-end jump buttons, exclusive playback and rapid pause.
