# How to build a REV-BENCH stage: a playbook for the model doing it

Written by Claude (Fable 5.2) on 2026-11-15, at Bernardo's request, for any model that picks up
a stage of this project. It says how I would approach each remaining stage and why.

It does not replace anything. PLAN.md says which stage is next and what is done. The stage's
card in `docs/roadmap/` says what it must deliver and what evidence closes it. CLAUDE.md has
the rules. This file is the method: how to get from the card to a finished stage without the
mistakes this project has already made once. Where it disagrees with those three, they win,
and this file should be corrected.

It has four parts:
1. the loop every stage follows;
2. what goes wrong, from this repo's own history;
3. a recipe for each kind of stage;
4. each remaining stage: where to start, what proves it, what to watch for.

---

## Part 1. The loop

Every stage, whatever its kind, goes through these eight steps in order. The order matters
more than any one step.

### 1. Read before touching

Read PLAN.md's "Where the project is" and the stage's row, the stage's card, CODEX.md's last
entry, and the records of the stages it depends on (their "What is left" sections above all).
For code, ask the graph first (`graphify query "..."`) and read only the files it points to.

Then write down, in two or three sentences, **the question the stage answers**. "Add the
editor" is a task. "Can a user change any engine field while it runs, hear the change within
half a second, and never hear a click or a crash?" is a question: it tells you what to test
and when you are done. If you can't write the question, you haven't understood the card.

### 2. Check the gaps

Bernardo's rule (2026-11-09): list every open gap (PLAN.md's 🟡 rows, "What is left" in the
stage records) and say for each whether it could influence this stage. Any that could is
fixed first. The rest are named as safe to leave, with the reason. Tell him before starting.

This is not ceremony. Stage 066 could not have been built honestly on top of V8s that fell
behind real time without anyone knowing; the gap check is what found that.

### 3. Decide what is Bernardo's and ask it once

Some choices are his: anything he will see or hear (a look, a level, a behaviour), anything
that spends his money or publishes his work, anything that trades sound for speed. Collect
them at the start and ask them together, each with a recommendation. Everything else (names,
file layout, which test first) you decide and note in the record.

Do not ask what you can find out. Do not start what he has not approved.

### 4. Build the smallest proof first

Find the smallest thing that can show the stage's idea is wrong. For physics that is a
fixture: one pipe, one cylinder, one pulse, with an answer known from a formula. For the app
it is the thinnest path through every layer: one command from a key to the core and one value
back to the screen. Build that, and only that, first.

If the proof fails, you have saved the stage. Say so, and stop or change the plan. A rejected
idea, recorded with its numbers, is a finished piece of work here (stages 031 and 043 are).

### 5. Test first, and prove the test can fail

Write the test before the code, at each public seam (the function in rb.h, the command in
`protocol.ts`, the tool's command line). Cover every seam; don't ask which.

Then plant bugs. For each new behaviour, break it on purpose in a copy
(`tools/plant_bugs.py`) and check a test goes red. A bug no test catches means a missing
test: write it. A bug that cannot be caught because it behaves exactly like the right code is
removed from the table with a note saying so. A stage's table goes in `tools/bugs/`.

Loops that wait for the engine must be bounded. A planted bug that stops the crank must fail
a test, not hang the suite.

### 6. Integrate through the real boundary

A mock proves the mock. Before calling anything done, run it through the path a user's
action takes: the real add-on, the real host on the null device, the installed build if the
stage ships one. Stage 124's short-path bug existed only in the installed app on CI.

### 7. Measure, and say what you did not

- **Code in `core/`:** `make wasm`, the sound check (render, `tools/compare_real.py`,
  `tools/orders.py`), and `tools/speed_check.py` (a median over 3 % slower fails).
- **A change meant to leave results alone:** `tools/cli_parity.py` must be bit-identical.
- **Sound:** numbers in range are necessary and never sufficient. Bernardo's ears decide.
- **Everything:** `python tools/run_tests.py`, all suites, before the commit. Run the full
  suite once, at the end; run the suite you are working in as you go.

Timings on the laptop swing 10 % from day to day. Compare two builds interleaved in the same
run, pinned to the fast cores, best of several. Never compare today's number with last
week's.

### 8. Close it

Write `docs/stage-NNN.md`: the question, what was built, the evidence with its commands, the
planted bugs and what caught them, what is left. Update the stage's row in PLAN.md, add a
short CODEX.md entry, update CLAUDE.md only if a rule or a module's contract changed, run
`graphify update .`. Run every suite. Only then commit (one commit, Bernardo's author line,
no co-author lines) and push, and check CI afterwards.

A stage with a part still open is 🟡 with the open part named. Never ✅ with a footnote.

---

## Part 2. What goes wrong here

Each of these happened in this repo. Read them as a checklist.

| What happened | The rule it teaches |
| --- | --- |
| A commit was pushed with the host suite failing (a use-after-free in `rb_host_render`). | The push comes after the full run, never beside it. |
| Planted bugs went stale after code moved: 29 at stage 120, more since. | After moving code, run `plant_bugs.py --check` and retarget in the same stage. |
| A "bit-identical" speed-up was 1.9 % slower. | Measure every speed idea. Revert what doesn't pay, and record it so nobody retries it. |
| Slower pipes passed the speed and engine checks and failed the sound by up to 44 dB. | Speed, engine numbers and sound are three separate checks. Passing two says nothing of the third. |
| The app's page check broke on Windows' 8.3 short paths, only when installed. | Test the installed build. Compare canonical paths, never strings. |
| An object file stayed stale after a header changed (`rb_bench.o`, later the add-on). | A new program's objects go in the Makefile's `ALL_OBJ`. After a struct changes, a clean build before trusting a crash. |
| C strings written through a shell heredoc lost their `\n` escapes. | Edit source with the editing tool, not through a shell. |
| The idle controllers hunted after a friction change. | Anything that moves the idle's torque response (burn, friction, pipes, cams) means retuning the idle. Watch it for 30 s. |
| At 96 kHz the valves' cells were too stiff; a wide tailpipe blew up. | The scheme is at its edge. A new part gets fuzzed (`rb_cli fuzz`) before it is trusted. |
| The four sounded quiet and the V8 crackled, found by Bernardo in minutes. | Get the build to him early. One listen finds what a thousand checks don't. |
| 16-bit WAVs showed spurs near −97 dB. | Analyse float renders (`--float`). |
| Two subagents edited the same file. | One writer per file. Give each agent its files by name. |

Three habits that prevent most of them:

- **Say what you did not verify.** "Not run on macOS", "not listened to", "CI not checked" in
  the record and in the message to Bernardo. A gap that is written down gets closed; a hidden
  one gets shipped.
- **Don't fix what you weren't asked to.** A problem outside the stage is reported and given
  a place in the plan. It is not folded in.
- **Never trade sound for speed silently.** A faster setting states what it changes, in dB.

---

## Part 3. Recipes by kind of stage

### A physics stage (anything in `core/`)

1. **State the physics** in the record before the code: the equations, the units, what owns
   each quantity (which module holds the mass, who counts the work), the range it is valid in.
   Cite the source. A paper justifies a model; it does not test our code.
2. **A fixture with a known answer**: an analytic case (a closed pipe's resonance, an
   isentropic nozzle, an adiabatic compression) or a conservation law (mass, energy, the
   crank's books to 1e-9).
3. **Write it the core's way**: C11, SI, doubles, no allocation, NaN guards, no GNU-only
   constructs (MSVC builds it too), one statement a line, a comment on what each function
   does. Bernardo reads this code: write it to be read.
4. **Put it in the engine behind a default that changes nothing.** New behaviour is off, or at
   a neutral value, until it has passed. Then the old tests still hold their numbers and the
   comparison is controlled: one change, everything else equal.
5. **Expose what the app and the 3D view will need** through rb.h: a parameter (append to
   `rb_param` and `PARAMS[]`, apply in `rb_sim_apply`), an engine field (append to
   `rb_engine_field` and `FIELDS[]` with a range), telemetry (before `RB_TLM_COUNT`: a new
   snapshot version). New public functions go in `WASM_EXPORTS`. Geometry is never copied
   into JavaScript.
6. **Check the idle** on all five presets, then fuzz.
7. Step 7 of the loop: wasm, sound, speed.

### A sound stage

1. **Start from a complaint or a measurement**, never from a technique. "Idle from behind is
   −38 dB above 2 kHz where recordings have −25" is a start. "Add convolution" is not.
2. **Find the cause before the cure.** Render each layer alone (`rb_cli sound --layers`).
   Switch off one mechanism at a time. A change that fixes the number without a known cause
   will break something else later.
3. **One change, with a switch.** Render before and after with everything else equal, same
   seed. Compare with the tools, in float.
4. **Listen yourself as far as numbers allow** (spectrograms, orders, repeat, crest factor),
   then hand Bernardo a blind pair: same level, random order, he says which is more real and
   why. His words go in the record verbatim.
5. **Keep what is physical apart from what is fitted.** A constant tuned to a recording is
   marked as fitted, with the recording named.
6. Recordings "at the exhaust" have the jet blowing on the microphone: their lows vary more
   than a listener's would. Unmatched recordings can reject, not confirm.

### A speed stage

1. **Profile first** (`tools/win_sampler.c` with MSVC, gprof with gcc). Stage 118's profile is
   the map: no hot spot over 12 %; the cost is the gas in about 400 cells.
2. **Two kinds of change, never mixed in one commit:**
   - *exact:* same results to the last bit. Proven by `tools/cli_parity.py` and unchanged
     render hashes.
   - *approximate:* a tier. Measured against high quality with `tools/tier_compare.py` (engine
     within 1 %, sound within 1 dB), declared with its misses.
3. **Time it properly:** interleaved, pinned, best of several, on the baseline laptop.
4. **Revert what doesn't pay** and write it under "tried and rejected".

### A host or real-time stage (`host/`, `app/native/`)

1. The render thread never allocates, locks, logs or waits. `host/src/audit.c` checks it: add
   new calls to its audit.
2. Everything crosses to and from that thread through the queue, the ring, the triple buffer
   or the event ring. A new kind of data gets one of those, not a mutex.
3. Rebuilds are prepared off the thread and swapped at a block boundary with a fade.
4. Tests run on the null device and in offline mode, where output must match `rb_cli` bit for
   bit. Run them under ThreadSanitizer.
5. **Never open a real audio device without Bernardo there.** Real-device runs are his to
   start, and their results (underruns, the worst block) go in the record.
6. System-specific code only in `host/src/platform_*.c`.

### An app stage (`app/`)

1. **Contract first.** `app/src/shared/protocol.ts` and `addon.ts` change on both sides in
   the same commit. Design the command and its ack before the screen.
2. **The window knows nothing about engines.** Fields, ranges, units and names come from the
   core's metadata. If you are typing an engine constant into TypeScript, stop.
3. **Three layers of test:** unit (the reducer, the decoder), the add-on's own suite, and
   end to end on the real add-on with the null device. Then the installed build.
4. **Design with the tokens** in the design system; anything visual that is new is shown to
   Bernardo before it spreads.
5. **Security stays as built:** sandbox, context isolation, the content policy, the fuses, the
   page check on canonical paths. A new capability for the window is a new, narrow message,
   not a wider bridge.
6. Dependencies pinned exactly, at least two weeks old, licence noted. Never `npm install` in
   the repo: work in the copy and bring `package-lock.json` back.
7. The engine process builds every engine with `hold` 0.

### A 3D stage

1. Geometry is generated from the core's description, every time. Nothing is modelled by
   hand, so every engine the editor can make can be drawn.
2. Motion comes from snapshots, delayed by the audio's output latency so the picture and the
   sound agree. Check it with a slow-motion capture: the spark's flash against the pulse.
3. The audio wins. The view sheds frames before the host's ring runs low.
4. A look (materials, lighting, the cutaway) is Bernardo's: show two or three, let him pick.

### A research stage

1. It lives in `experiments/`, is linked only into tests and probes, and changes no sound.
2. Decide what would reject it before running it: thresholds, cases, cost.
3. A rejection is a result. Record the numbers and stop.
4. "Works on a fixture" is not "works in the engine", and neither is "runs in real time".
   They are three claims with three kinds of evidence.

### Using subagents

Bernardo allows Sonnet subagents when they make the work better or faster. They do when the
stage splits into parts that touch different files and can each be checked alone (stages 118
and 123 ran side by side; 124's add-on, window and packaging did too).

- Give each a written brief: the goal, its files by name, the files it must not touch, the
  tests that define done, the repo's rules that apply, and "query graphify first".
- One writer per file. Shared contracts (`protocol.ts`, `addon.ts`, rb.h) are written by you
  first, then frozen while they work.
- Medium effort for mechanical work, high for anything with judgement in it.
- Their report is a claim. Read the diff and run the tests yourself before believing it.
- Design, physics decisions, anything Bernardo hears: do those yourself.
- Run the full suite once, after merging their work, not once per agent.

---

## Part 4. The remaining stages

For each: the question, where to start, what proves it, and the trap. The card in
`docs/roadmap/` has the full specification; this is the approach.

### Phase A: the playable app

**066, the playable engine.**
*Question:* can Bernardo start, drive and stop any preset from the app, on his devices, with
the sound that `rb_cli` would render?
- Start with the command trace test from the card: a seeded trace through the UI, captured
  from the host's tap, equal to the host's offline render. Everything else hangs off it.
- Then the throttle styles as one small state machine in the engine process (on/off, ramped,
  analog), tested without a window. The window only sends raw input.
- Add a dead-man rule: live input that goes silent ramps the throttle to zero
  (docs/review-2026-11-15.md, problem 8). Test it by hiding the window with a key held.
- Levels: one gain for all engines, chosen so the loudest preset's hardest rev stays under
  the limiter's knee, plus a volume control. Measure all five; don't equalise them.
- Devices: lose the device, keep playing on the new default, say which.
- The V8s: show plainly that this machine can't keep up; never drop the tier silently.
- The Razer keyboard: first as an emulated controller. Check for a native SDK; don't assume
  one. It needs his keyboard, so plan a session with him.
*Trap:* a tone, a mono copy or a benchmark is not the engine playing.

**067, the editor.**
*Question:* can every field be changed live, safely, and undone?
- Generate the form from `FIELDS[]` (name, unit, range, live or rebuild). No hand-made form.
- Live fields change in place. Rebuild fields are batched (a short debounce), built off the
  render thread, swapped with the fade.
- Undo is a list of descriptions, not of edits: restoring is one `rb_set_engine` batch.
- A refused description shows the core's reason next to the field.
- Before it ships, fuzz the fields' ranges (review, problem 5): the editor offers only what
  survived.
*Trap:* dragging a slider must not queue forty rebuilds. Test a drag.

**068, instruments and the dyno.**
*Question:* do the gauges and the dyno curve show the core's own numbers, in time with the
sound?
- Everything read from snapshots and the sim's own dyno; nothing computed in the window but
  drawing.
- Draw at display rate from the latest snapshot; never queue snapshots.
- Dyno overlays compare builds: store the curve with the description that made it.
*Trap:* a gauge smoothed in JavaScript is a second model of the engine. Smooth in the core
or not at all.

**069, qualification and the first pre-release.**
*Question:* does it hold for fifteen minutes on real devices, and is it good enough to hand
to someone?
- The stress runs with Bernardo present: every preset, device changes, sleep and wake, a
  heavy fake drawing load in the window (review, problem 7).
- Record underruns, the worst block, memory over time. Zero underruns on the declared tiers.
- His listening, per preset, written down.
- The pre-release is his decision. Ask; don't publish.

### Phase B: the sound

Read docs/review-2026-11-15.md, problems 1–4, first. My order inside this phase would be:
the matched recording (039), the complaints list with the live A/B switch, the hand-off test,
then the cards as written.

**039, matched references.** One recording of a known car beats fifty unknowns. The recipe is
in the review, problem 2. The stage closes when a preset has been set to a real engine's
geometry and compared one-to-one.

**050, catalysts, mufflers, perforates, packing.**
*Question:* does each exhaust part pass and reflect sound as the real part does?
- Fixture: each part alone in a pipe, its transmission loss against frequency, compared with
  the textbook result (an expansion chamber's lobes; a perforate's and absorber's published
  curves). Munjal's *Acoustics of Ducts and Mufflers* is the reference.
- Then in the engine: back pressure and torque must not move more than the part explains.
*Trap:* tuning the part until the car sounds right. Tune it until the part is right.

**051, radiation at the tailpipe.**
*Question:* does the open end reflect and radiate with the right frequency dependence, and is
what the listener hears the same energy that left the pipe?
- Fixture: the reflection coefficient of an unflanged pipe against ka (Levine and Schwinger).
- The energy check: what the end radiates equals what the pipe lost there.
*Trap:* adding a radiation filter on top of an end that already reflects: the same physics
counted twice.

**052, the mixed-rate scheduler.**
*Question:* can parts run at different rates without clicks, drift or lost mass?
- Start with the hand-off test (review, problem 4): a sine across a rate boundary, no images.
- Then mass and energy across the boundary to rounding, and a rate change while running
  without a click.

**053, flow excitation.** *Question:* who owns the turbulent energy, and what happens when
the flow reverses? Start from stage 115's jet model and its energy books. Fixture: a steady
jet's spectrum against Lighthill's scaling; then a reversing flow with no click and no energy
made from nothing.

**055, structure and accessories.** *Question:* does each engine's block ring as that engine
does? The modes are per engine (size, material, layout), struck by forces the core already
computes. Without a matched close recording this is fitted: say so. *Trap:* the mechanical
layer masking a weak exhaust.

**056, listener, vehicle and recording transfer.** *Question:* is the path from the sources
to the ear (ground, body, cabin, a phone's microphone) right? Use the matched recording at two
distances. This is where a measured body response belongs, named as measured.

**054, unified pulse and noise routing.** *Question:* do pulses and noise take the same path
through the same parts? One path description feeds both. Fixture: a click and a noise burst
through the same muffler come out with the same transfer.

**057, candidate integration.**
*Question:* does any research solver, placed in the real engine, beat today's pipes?
- Before integrating all of them, measure where the exhaust's waves are weak (review,
  problem 3, step 1). Integrate a candidate only where it is valid.
- Behind a research selector, default off. Books and guards alongside every render.
*Trap:* summing the old and the new copy of the same pulse.

**058, deceleration and lift-off.** *Question:* what makes the overrun robotic and boomy?
A diagnosis stage: its deliverable is a cause, shown by switching it off. Use the layers,
the overrun spark retard, the fuel cut's timing, the limiter. Fix only after the cause is
shown. The V8's crackle belongs here.

**059, five-engine qualification.** *Question:* over what range is each preset right, and how
do we know? Maps (torque, VE, back pressure) against published figures; sound against matched
references where they exist; unmatched ones named as such. The deliverable is the supported
envelope with its uncertainty, not a pass mark.

**060, renderer decision.** An ADR. Equal conditions for every candidate, the costs measured,
the rejected data kept. "None of them" is a valid outcome and opens a new bounded experiment.

**061, promotion.** Move the winner into the core, the host and the app. Old and new never
both sound. A rollback path that is tested. Parity suites on every compiler.

**062, tiers and optimisation.** The speed recipe above, on the promoted renderer. Begin with
stage 118's leads (the hand-off, slower pipes). Re-declare the tier table.

**063, base sound acceptance.** Bernardo listens: per preset, per scenario, blind where it
can be, against references. His verdicts are the record. Listening cannot waive a failed
conservation or real-time check, and passing checks cannot stand in for his yes.

### Phase C: the 3D engine (070–077)

- **070:** the scene, the style and what the machine can draw. Decide WebGPU with the WebGL
  fallback by measuring on the laptop, with the audio running. Show Bernardo styles early.
- **071, 072:** bottom end, then top end, generated from the description. Prove the kinematics
  against the core: the piston's position from the snapshot's crank angle must equal the
  core's own to rounding. Turn it by hand (a slider) before it runs.
- **073:** live motion from snapshots, delayed to match the sound. The test is timing, not
  looks.
- **074:** slow motion. Decide what the sound does when time slows (the card's gate): ask.
- **075:** pipes and the intake in 3D, from the same exhaust model the sound uses.
- **076:** gas, flame and heat drawn from per-cell state. Scale colours to physical units,
  with a legend.
- **077:** audio and 3D together for fifteen minutes on the laptop. The audio's underruns
  decide. Do the fake-load version of this in 069 so 077 holds no surprise.

### Phase D: turbo (078–084)

- **078:** compressor and turbine as map components. Fixture: a published map reproduced
  point by point; surge and choke lines respected. Extrapolation outside the map is stated.
- **079:** the shaft joins the gas system. Check the energy books: turbine work = compressor
  work + bearing loss + the shaft's change in kinetic energy.
- **080:** boost control, wastegate, intercooler, blow-off. Controllers are tuned as the idle
  was: a step, a load, room for stronger gains. Watch for hunting over 30 s.
- **081:** the turbo's sound: whistle, the turbine muffling the pulses, the blow-off. Start
  from recordings of turbo engines (Bernardo's BMW and Golf are turbocharged: here they
  finally match).
- **082, 083:** controls and 3D for it, by the app and 3D recipes.
- **084:** qualify. Speed above all: a turbo adds volumes. Run the speed check at 079 already,
  not only here.

### Phase E: knock, limits, damage (085–089)

- **085:** end-gas knock. A model with a published validation (an autoignition integral over
  the unburned gas's history). Fixture: knock onset against spark advance and octane for a
  documented engine. *Trap:* a knock model that fires from compression ratio alone.
- **086:** the computer retards on knock; knock is heard. The sound of knock is a struck
  cylinder resonance: frequency from bore and gas temperature.
- **087:** piston speed, valve float, limits. Each limit has a source and shows a warning
  before it acts. This answers the criticism of sites where "redline is unaffected by
  anything".
- **088:** damage, optional, resettable. Bernardo decides how punishing.
- **089:** how knock and damage look and sound in the app.

### Phase F: more engines (090–094)

- Each new layout: firing order and crank pins from a named real engine, checked by
  `rb_engine_desc_check`; the torque curve against that engine's published one; idle tuned by
  stage 8's method; then fuzzed.
- **Speed first.** Before building a V10 or V12, time a description with that many cylinders.
  If it cannot run in real time on the declared tier, say so before the work.
- **094:** each engine's own exhaust, researched and cited, as stage 027 did for the five.

### Phase G: release (126, 095–102)

- **126, the port.** If the thin port of the review (problem 9) was done, this is devices,
  thread priorities, packaging and signing. If not, expect compiler, path and loader
  surprises first. Real machines are needed: ask which Bernardo has.
- **095, 096:** build files and codes. A versioned schema, validated on load, with old
  versions migrated by tests that keep sample files of every version. Imported data is
  untrusted: bounds-check everything, never execute or resolve paths from it.
- **097:** polish and accessibility. Keyboard reach for everything, contrast, reduced motion,
  screen-reader names.
- **098:** final listening. As 063, for everything added since.
- **099:** qualification on each system and on the range of hardware: say which machines play
  which engines at which tier.
- **100:** documentation and licences. THIRD_PARTY_NOTICES from the pinned dependencies. The
  project's own licence must exist by now (review, problem 11).
- **101:** signing and updates. Certificates, keys and accounts are Bernardo's to create and
  hold. Never put a secret in the repo; CI reads them from its secret store.
- **102:** the release, on a clean machine, by someone who has never seen it.

---

## The short version

1. Write the question the stage answers.
2. Check the gaps; ask Bernardo what is his to decide, once.
3. Build the smallest thing that could prove the idea wrong.
4. Test first, at every seam; plant bugs to prove the tests bite.
5. Go through the real boundary, not a mock.
6. Measure sound, speed and correctness separately. Say what you did not check.
7. Let him hear it early.
8. Record it, run everything, then commit.
