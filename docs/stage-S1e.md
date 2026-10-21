# S1e — Research and controlled overrun transitions

2026-10-21. Implementation and diagnostics for the consolidated stage-13b
checkpoint. **S1 remains open.** This is not a claim that robotic deceleration
or genuine ignition failure has been solved.

## Evidence and scope

Bernardo hears robotic V8/I6 deceleration. Graphify located `engine_computer`,
`computer_spark`, the CLI rev driver and the noise/propagation paths. The
previous ECU immediately stopped injection above idle +700 rpm with the pedal
closed, resuming below idle +400 rpm. The modeled exhaust then primarily pumps
air, with regular valve/crank timing. Missing context and controller behaviour
are plausible contributors, not a proven diagnosis of his listening impression.

[MoTeC's manufacturer guide](https://motec.zendesk.com/hc/en-gb/articles/14575174412815-Checklist-Engine-Overrun-Setup)
describes a timed overrun entry with progressive ignition retard before fuel
cut, and ignition assistance when fueling resumes.
[Motorsport Electronics' documentation](https://motorsport-electronics.co.uk/onlinehelp/html/8211.html)
also documents configurable entry and return behaviour. These establish that
the old instantaneous transition is an incomplete controller model; they do
not establish the calibration of the recorded cars.

## Model

Following a qualifying pedal lift, a saturating timer advances to
`overrun_transition`. The scheduled IVC spark loses
`overrun_retard * min(timer / transition, 1)` degrees. Once the timer expires,
the existing injection cut takes over. Pedal reopening or crossing the resume
threshold clears the timer; hysteresis retains a qualifying transition between
the two speed thresholds. Held-speed runs clear the controller's transition.

The illustrative defaults are 0.15 s and 12 degrees. They are explicitly
**uncalibrated against the supplied recordings**. A preceding open pedal arms
the transition, so it does not alter the existing key-start overspeed protection.
No audio envelope, synthetic pops, randomized fuel cuts, exhaust afterfire or
wall-film reservoir was added. All heat still comes from modeled fuel/air.
Return-to-idle still uses the existing air/spark controller; a detailed
progressive restart model remains future work.

`rb_cli sound --overrun-transition 0` selects immediate cut for a matched control.
`--overrun-transition 0.15` selects the new entry. Both retain the same seed,
gain, physics rate and driver schedule; the standard neutral rev lifts at 7.1 s.
The CLI describes its source intervals separately from the real recordings.

## Real references

Unfiltered spectrogram inspection identified descending harmonics in BMW
250–253 s, Mustang 257–263 s and Ferrari 302–310 s. These are **falling-pitch
candidates**, not verified pedal lifts, neutral revs, in-gear overrun, camera
positions or RPM traces. Motion and shifts can also change pitch. The BMW is
turbocharged; the present I6 is naturally aspirated. `tools/checkpoint_13b.py`
records exact source-relative and absolute intervals and hashes; original
reference files are never modified. Measurements use unfiltered float WAVs.
Listening aids alone receive fades and static gain, with a shared gain for
the two simulated halves. See `docs/user-sound-references.md`.

## Validation and remaining work

Focused native tests exercise entry time, hysteresis, reset, fuel exclusion
during cut, a genuinely later scheduled spark, immediate-cut control, held
operation and startup separation. The previous drive test assumed immediate
fuel exclusion: it now explicitly measures fuel injected while cut is active.
Its idle and recovery checks remain. A startup calibration regression was
found and corrected by arming the transition only after pedal use; the original
3-second idle-spark calibration test is retained.

Bernardo also requested a longer deceleration scenario. The CLI now accepts
`--coast-seconds` and a maximum `--pull-seconds` before lift. The checkpoint uses
an eight-second in-gear coast, then the clutch leaves only engine inertia on
the crank. CSV records crank inertia and the overrun timer; the manifest records
lift, clutch-in and ending times. Pitch and playback speed are unchanged.

The consolidated results are in `docs/checkpoint-13b.md`. Low-band repetition,
high V8 idle work COV and a research-supported ignition-kernel failure model
remain outstanding. The [optical-engine study by Schiffmann et al.](https://journals.sagepub.com/doi/10.1177/1468087417720558)
supports investigating early flame development; it does not justify inventing
a universal random misfire probability. Bernardo's listening acceptance stays open.
