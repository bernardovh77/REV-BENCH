# The workspace

Where REV-BENCH is worked on, and how each place is set up. It has its own repo,
github.com/bernardovh77/REV-BENCH (branch `main`). Until stage 13 it was the `REV-BENCH/` folder of
Bernardo's Personal-Projects repo, on its `rev-bench` branch; its history came along, every commit.

## Bernardo's machine (Windows + WSL)

- The clone is its own folder, `/mnt/c/Users/berna/REV-BENCH`: `git pull` and `git push` there
  sync with this repo. (Until stage 12b it sat inside the old Personal-Projects repo, which no
  longer exists.) Claude Code runs in the Windows desktop app's Code tab, on this folder as
  `C:\Users\berna\REV-BENCH` (since 2026-11-06; before that in WSL), and reads this repo's
  `CLAUDE.md` and `.claude/settings.json` either way. Its shell is Git Bash or PowerShell; WSL is
  there for Linux checks, and nothing needs it.
- Bernardo's reference recordings and videos of real engines are only here, in `renders/`
  (ignored by git; docs/user-sound-references.md lists them). A cloud session can't see them.
  To use them, either attach them to a message in the session, or start the session from WSL:
  `claude remote-control` in this folder lets claude.ai/code start sessions that run here. The
  Windows desktop app's Remote Control folders aren't offered to cloud sessions.
- graphify (the code graph in `graphify-out/`), version 0.9.92, is in the venv on Windows
  (`C:\Users\berna\rb-venv\Scripts\graphify.exe`, from tools/requirements-dev.txt) and in
  `~/.local/bin` in WSL. The reminder hook finds it in either. After pulling, run `graphify update .`: it remakes its
  cache and manifest, which are each machine's own and not in git, and catches the graph up if a
  session couldn't.
- Renders aren't in git (`renders/` is ignored): `make render` makes them, about four minutes.

### Running every test, on Windows or in WSL (2026-11-06)

The repo lives on the Windows drive, `C:\Users\berna\REV-BENCH`, which WSL sees as
`/mnt/c/Users/berna/REV-BENCH`. Every suite runs on either system by itself, so neither depends
on the other: `tools/run_tests.py` runs them all (native, wasm, wasm-c, python, web, e2e) and
prints a table. Both give the same results: 251 native tests and 4,950,067 checks, the same
WebAssembly byte for byte, 102 Python tests with none skipped, 17 web tests and the 2 e2e tests in
both modes.

**Windows** (the desktop app's sessions run here):
- **C:** w64devkit's gcc and make (`C:\w64devkit\bin`). It has no LTO; the Makefile notices and
  builds without it. The native suite takes about 8 minutes this way.
- **Python:** a venv with the tools, set up once:
  ```
  python -m venv C:\Users\berna\rb-venv
  C:\Users\berna\rb-venv\Scripts\pip install -r tools\requirements-dev.txt
  ```
- **Node:** Windows' Node (22.12 or later).
- **e2e:** the installed Chrome (or Edge), so nothing is downloaded.

Then: `C:\Users\berna\rb-venv\Scripts\python tools\run_tests.py`.

**The MSVC build** (stage 122, the `cmake` suite; the app's compiler on Windows):
- **Visual Studio Build Tools 2026** (18.x) with the "Desktop development with C++" workload:
  MSVC 14.51 (`cl` 19.51) and the Windows SDK. Here it's in
  `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools`.
- **CMake 4.4.4 and Ninja 1.13.2** come from `tools/requirements-dev.txt` into the venv: no
  system install.
- No developer prompt needed: `tools/msvc_env.py` finds the Build Tools through `vswhere`,
  loads `vcvars64.bat`'s environment and runs the command, e.g.
  `python tools\msvc_env.py cmake -S . -B %USERPROFILE%\rb-build\msvc -G Ninja`, then
  `... cmake --build ...` and `... ctest --test-dir ...`. `run_tests.py cmake` does all three.
- `python tools\compiler_parity.py check MSVC_RB_CLI GCC_RB_CLI WORKDIR` checks the two
  compilers' renders agree within the frozen tolerances (docs/stage-122.md).

The `cmake` suite runs in WSL too, with gcc (the venv's CMake and Ninja).

**WSL** (Ubuntu):
- **C:** gcc and make.
- **Python:** `~/rb-venv`, set up once with
  `python3 -m venv ~/rb-venv && ~/rb-venv/bin/pip install -r tools/requirements-dev.txt`.
- **Node:** Node 22 in `~/.local/node22`, from nodejs.org's Linux tarball, as there is no sudo;
  the runner adds it to `PATH`.
- **e2e:** Playwright's own Chromium, downloaded once with `npx playwright install chromium`
  from the web app (about 190 MB).

Then: `~/rb-venv/bin/python tools/run_tests.py`.

Notes:
- **zig:** the requirements pin zig 0.17.0, which reproduces the committed `rb_core.wasm`;
  0.17.1 builds a different one.
- **FFmpeg:** the reference tools' tests use imageio-ffmpeg's static FFmpeg
  (`tools/ffmpeg_path.py`). WSL's own FFmpeg is a snap, which can't read files in `/tmp`.
- **Builds** go to `~/rb-build/tests` on each system, never into the repo.
- **The runner** (stage 120) first checks that make and Node 22+ are on PATH, stops at the first
  failed suite (`--keep-going` runs the rest), and kills a suite that runs past its limit,
  with every process it started.
- **Agent skills** in `.agents/` (and `skills-lock.json`) are this machine's own and git-ignored.
- **If WSL stops responding** (it once hung for minutes, even for `echo`), Windows alone runs
  everything.

### Moving it out of Personal-Projects (done, 2026-10-20)

Kept for the record; the steps were:

```bash
cd /mnt/c/Users/berna
git clone https://github.com/bernardovh77/REV-BENCH.git REV-BENCH
git -C Personal-Projects/REV-BENCH status --short   # anything listed (graphify-out/ aside) isn't on GitHub: copy it over
cp -r Personal-Projects/REV-BENCH/renders/. REV-BENCH/renders/   # your old renders
mv Personal-Projects/REV-BENCH REV-BENCH-old        # delete it once the new one works
git -C Personal-Projects checkout main              # its rev-bench branch only points here now
cd REV-BENCH/web && npm ci && cd ..
graphify update .
make test
```

Then open Claude Code in `/mnt/c/Users/berna/REV-BENCH` (in the desktop app, pick that folder).
Claude Code lists past sessions by the folder they ran in: to `/resume` the old ones from here, copy
`~/.claude/projects/-mnt-c-Users-berna-Personal-Projects-REV-BENCH` to
`~/.claude/projects/-mnt-c-Users-berna-REV-BENCH`.

## Claude's cloud sessions

- Start them on the REV-BENCH repo. They start in its root and read its `CLAUDE.md` and
  `.claude/settings.json`.
- At the start, `.claude/hooks/session-start.sh` installs the tools: `~/rb-venv` with zig 0.17,
  numpy, matplotlib and graphify 0.9.92 (on PATH), and the web app's `node_modules` in
  `~/rb-copy/web`. It skips what's there and does nothing outside the cloud. A session starts once
  it's done: about a minute on a fresh container, a moment after that.

## .claude/settings.json

- **SessionStart:** the setup above (tools/requirements-dev.txt, reinstalled when it changes).
- **PreToolUse:** graphify's reminder to query the graph before searching or reading
  (`.claude/hooks/graphify-guard.sh`). It finds graphify on PATH, in `~/.local/bin` or in
  `~/rb-venv` (`Scripts` on Windows, `bin` on Linux), and stays
  quiet in cloud sessions: CLAUDE.md asks for the graph anyway, and the reminder would start Python
  before every call.
- **Allowed without asking:** `graphify query`, `path`, `explain` and `update`.

Claude can edit this file in a session set to Accept edits; in Auto mode, its safety check blocks
changes to it.
