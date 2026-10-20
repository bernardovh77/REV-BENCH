# The workspace

Where REV-BENCH is worked on, and how each place is set up. It has its own repo,
github.com/bernardovh77/REV-BENCH (branch `main`). Until stage 13 it was the `REV-BENCH/` folder of
Bernardo's Personal-Projects repo, on its `rev-bench` branch; its history came along, every commit.

## Bernardo's machine (Windows + WSL)

- The clone is its own folder beside Personal-Projects, `/mnt/c/Users/berna/REV-BENCH`, so
  `git pull` and `git push` there sync with this repo, and Personal-Projects with its own. (Until
  stage 12b it sat inside Personal-Projects, which ignored it.) Claude Code runs in WSL, started
  there, so it reads this repo's `CLAUDE.md` and `.claude/settings.json`.
- graphify (the code graph in `graphify-out/`) is installed in `~/.local/bin`, version 0.9.92
  (`pip install --user graphifyy==0.9.92`). After pulling, run `graphify update .`: it remakes its
  cache and manifest, which are each machine's own and not in git, and catches the graph up if a
  session couldn't.
- Renders aren't in git (`renders/` is ignored): `make render` makes them, about four minutes.

### Moving it out of Personal-Projects (once)

In WSL, with an empty `REV-BENCH` folder beside Personal-Projects (or none: the clone makes it):

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

- **SessionStart:** the setup above.
- **PreToolUse:** graphify's reminder to query the graph before searching or reading
  (`.claude/hooks/graphify-guard.sh`). It finds graphify on PATH or in `~/.local/bin`, and stays
  quiet in cloud sessions: CLAUDE.md asks for the graph anyway, and the reminder would start Python
  before every call.
- **Allowed without asking:** `graphify query`, `path`, `explain` and `update`.

Claude can edit this file in a session set to Accept edits; in Auto mode, its safety check blocks
changes to it.
