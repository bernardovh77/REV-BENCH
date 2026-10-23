# REV-BENCH

A browser-based combustion engine test bench: pick an engine, change displacement,
compression, cams, headers and turbo size, and hear and see the difference in real time,
with a 3D view of the engine running. The sound is generated from a physics simulation of
the gas moving through the engine, not from recordings.

The whole idea behind this project is to unite my automotive passion and afinity to my developing
software engineering skills to result in a program that will catch the eyes of those who share the same
interests as me and give a purpose to my learnings.

**Note:** due to the size and complexity of this project, the developing is being made with the assistance of AI models such as Claude Code and Codex that are being tasked with the heavier coding and oriented through my feedback regarding the quality of their output. For further information, feel free to check the .md files that are disposed on the repo.


**Status:** [the canonical roadmap](PLAN.md) now uses Stages **001–113**,
with a [crosswalk for every old ID](docs/roadmap/legacy-map.md). Native engine
physics, five presets, rendering and bounded acoustic experiments exist. The
latest [open-duct coupling proof](docs/stage-S2d-0b-B1a.md) passes its stated
numerical fixtures; it is below real time and does not yet connect the full
production engine to the acoustic graph. The browser is still the tone scaffold;
product 3D, turbo, knock/damage and expanded layouts remain planned.

The [review repairs](docs/roadmap/14-review-fixes.md) harden checkpoint
publication and acoustic numerical failure handling. Accepted engine audio
stays unchanged; broader sound-physics/integration gates remain open.

**Next: Stage 037**, shared build/geometry/event/snapshot contracts, followed by
causal source and propagation qualification before the playable browser and 3D.
The [stage protocol](docs/roadmap/README.md) separates experiment results,
production integration, runtime and Bernardo's realism acceptance. Full release
includes the original feature scope; learned synthesis and extras are conditional.

Current accepted sound remains the [selective rollback](docs/stage-13b-r-rollback.md):
I6 earlier twin exhaust at 8500 rpm, flat V8 earlier exhaust at 9000 rpm, cross V8
sharper rear-delete X at 7500 rpm. These limits are not durability certification.

The C core plays in the browser through an
AudioWorklet (still a test tone); natively it renders the engine's own sound to WAV files, as a
listener around the car hears it: behind it, beside it, over the bonnet, a metre from a tailpipe
or in the driver's seat, each tailpipe and the intake's mouth at their own distance and off the
ground. On top of the pipes' sound come the noise of the gas's jets (at the tailpipes, the
exhaust valves and the throttle) and the engine's own mechanical sound (combustion, piston slap,
the valves, the injectors), struck into the block's resonances. No two cycles are alike: each
burn starts and runs its own way, the cylinders differ a little and their headers aren't all
alike, as a real engine's; the rev limiter stutters on a soft cut, and let go the engine falls
back to idle. The physics so
far: gas properties, chambers and flow through openings,
a cylinder with a crank and piston, an engine breathing through its cams, valves, throttle
and intake plenum, burning fuel with heat lost to the walls, and a crank that turns on its
own: friction, a starter motor, and an engine computer that idles it, cuts the fuel when
coasting and stops it at the rev limit. Its intake snorkel and runners, exhaust headers,
exhaust pipes, mufflers and tailpipes are 1D pipes, where pressure waves run at the speed of
sound and tune the engine's breathing and its sound. A muffler can be a chambered can, a
straight-through packed with wool, or a straight pipe. Engines can be inline, V or boxer
with any crank and firing order, and five come ready-made: a 2.0 L four, a 3.0 L six, a
cross-plane and a flat-plane V8, and a 2.0 L flat four with unequal headers.
The researched exhausts include model-specific routing, center/rear sections,
an X crossover or a rear split where appropriate, and a causal outlet radiation
load. Published dimensions and estimated internals are distinguished in the
stage notes. On the dyno, the four, the
six and the flat four make close to the torque and power of the real engines they stand for
(Toyota's 1AZ-FE, BMW's M54B30 and Subaru's FB20). A quality setting trades a little accuracy
for speed; current shared-host measurements and their limits are recorded in
the stage notes. A fuzzer drives random engines to find
what breaks. See [PLAN.md](PLAN.md) for the roadmap and progress,
[RESEARCH.md](RESEARCH.md) for background, and `docs/` for notes and equations per stage.





Stack: C11 physics core → WebAssembly in an AudioWorklet · Vite + TypeScript UI ·
static hosting (Vercel).
