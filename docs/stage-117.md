# Stage 117 — Docs you can read, and up to date

**Status:** done (2026-11-01)

Item 3 of Claude's review of Codex's work. Bernardo is the reader: a CS student, strongest in C,
who has to follow where his project is and where it's going. Codex's records had become hard to
read, and several docs described a project that no longer existed. No code in `core/` changed.

## The missing spaces

From stage 038 on, Codex's docs lost the space between words and numbers: "Bernardo
authorized114 then049", "Full native248tests/4951907checks", "95Python,17web/build". A scan
found 1,164 such places in 30 of the 78 Markdown files Codex wrote or changed, against none in
the docs before it.

A script restored 1,626 spaces across those files. It touches only prose: code, inline code, link
targets, URLs, paths, identifiers with underscores and hashes are left exactly as they were.
- **Glued to a number:** a word or a number glued to the other gets a space ("authorized 114",
  "95 Python").
- **After a comma or semicolon:** a number glued after one gets a space too ("files, 76").
- **Names kept whole:** names that end in digits stay as they are (`bernardovh77`, `mario1298`,
  `pypdfium2`, `mp3`, `utf8`, `boxer4`, ...), and so do units after a number (`48kHz`, `3000rpm`).
- **Version tags:** a lost space before one goes back before the "v" ("usesv3": "uses v3").
- **By hand:** a dozen words glued to each other were split ("stillstage", "WorstGaussian",
  "matchedreturn").

The scan now finds 59 places, mostly a full stop before a number in CODEX.md, the historical log.
A full stop there can be part of a decimal (".35" is 0.35), so no script can fix them safely.
PLAN.md's "Current state" got the rest of its fixes by hand.

## What changed besides

- **CLAUDE.md:**
  - **Stage numbers:** PLAN.md now numbers the stages from 001, and this file's older numbers are
    the old ones. For example, the browser engine was the old 14 and is now 066–069; PLAN.md's
    "stage 14" is a finished early stage.
  - **Codex:** it works on this repo too, keeping AGENTS.md and CODEX.md.
  - **Bernardo's references:** they live on his machine only.
  - **Codex's additions:** the new modules (`ignition.c`, `injection.c`, `radiation.c`,
    `exhaust_models.c`, `scene.c`, `diagnostics.c`), the research solvers in
    `experiments/acoustics` (not part of the engine) and the contracts.
  - **Stage 115:** its sound numbers and the cycles' variation model.
  - **The suite's time:** now 5–9 min.
- **README.md:** its status said stages 001–113 and "held at stage 040". It now gives what
  exists today, the latest stages, and what's planned. Bernardo's own lines are as he wrote them.
- **docs/workspace.md:**
  - The Personal-Projects repo is gone, and the move out of it is marked done.
  - It notes where his reference recordings are and how a session can reach them: attach them,
    or run `claude remote-control` in the REV-BENCH folder from WSL. The Windows desktop app's
    Remote Control folders aren't offered to cloud sessions.
- **GLOSSARY.md:** the abbreviations the research stages use without spelling them out: A0, A1,
  A2, B0, CFL, EOS, GCL and SSPRK2.
- **`rb_cli`'s text:**
  - **Help:** it had lost its spaces too ("amplitude0..4;0", "seed1..4294967295"), and the fuzz
    command's `--quality` left out "ultra". It now reads properly.
  - **Compiler warning:** its sound options were one string literal over the 4,095 characters C
    compilers must accept (`-Woverlength-strings`). The string is split, and the warning is gone.
  - **`--scene`:** the summary now names the stereo scene it renders. Before, it named the mono
    listener, which `--scene` doesn't use.

## Results

- **Native tests:** 250 tests and 4,950,077 checks, all pass.
- **Python tools:** 95 of 95 pass.
- **Web:** `npm test` passes.
- **Links:** the docs' links all resolve, except the six Codex left to git-ignored audio in
  `renders/`.
