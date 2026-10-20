# The workspace

Where REV-BENCH is worked on, and how each place is set up. It has its own repo,
github.com/bernardovh77/REV-BENCH (branch `main`). Until stage 13 it was the `REV-BENCH/` folder of
Bernardo's Personal-Projects repo, on its `rev-bench` branch; its history came along, every commit.

## Bernardo's machine (Windows + WSL)

- The clone sits where the folder was, `/mnt/c/Users/berna/Personal-Projects/REV-BENCH`, and
  Personal-Projects' `.gitignore` skips it. Claude Code runs in WSL, started there, so it reads
  this repo's `CLAUDE.md` and `.claude/settings.json`.
- graphify (the code graph in `graphify-out/`) is installed in `~/.local/bin`, version 0.9.92
  (`pip install --user graphifyy==0.9.92`). After pulling, run `graphify update .`: it remakes its
  cache and manifest, which are each machine's own and not in git, and catches the graph up if a
  session couldn't.

### Moving your copy over (once)

In WSL:

```bash
cd /mnt/c/Users/berna/Personal-Projects
git checkout rev-bench
git status --short REV-BENCH          # anything not committed? It isn't in the new repo: copy it aside
git pull                              # REV-BENCH's files leave Personal-Projects; ignored ones stay behind
mv REV-BENCH ../REV-BENCH-old
git clone https://github.com/bernardovh77/REV-BENCH.git REV-BENCH
cp -r ../REV-BENCH-old/renders/. REV-BENCH/renders/    # keep your renders
cd REV-BENCH/web && npm install && cd ..
graphify update .
```

If `git pull` refuses because of local changes in REV-BENCH (graphify rewrites its graph files when
it runs, for one), copy aside anything you want to keep, run `git checkout -- REV-BENCH`, and pull
again. Then open Claude Code in `REV-BENCH` as before, and delete `../REV-BENCH-old` once everything
works.

## Claude's cloud sessions

- Start them on the REV-BENCH repo. They start in its root and read its `CLAUDE.md` and
  `.claude/settings.json`.
- At the start, `.claude/hooks/session-start.sh` installs the tools: `~/rb-venv` with zig 0.17,
  numpy, matplotlib and graphify 0.9.92 (on PATH), and the web app's `node_modules` in
  `~/rb-copy/web`. It skips what's there and does nothing outside the cloud. A session starts once
  it's done: about a minute on a fresh container, a moment after that.

## .claude/settings.json

- **SessionStart:** the setup above.
- **PreToolUse:** graphify's reminder to query the graph before searching or reading
  (`.claude/hooks/graphify-guard.sh`). It finds graphify on PATH or in `~/.local/bin`, and stays
  quiet in cloud sessions: CLAUDE.md asks for the graph anyway, and the reminder would start Python
  before every call.
- **Allowed without asking:** `graphify query`, `path`, `explain` and `update`.

Claude can edit this file in a session set to Accept edits; in Auto mode, its safety check blocks
changes to it.
