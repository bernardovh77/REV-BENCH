# The workspace

Where REV-BENCH is worked on, and how each place is set up.

## Bernardo's machine (Windows + WSL)

- The repo is `/mnt/c/Users/berna/Personal-Projects`, branch `rev-bench`; the project is its
  `REV-BENCH/` folder. Claude Code runs in WSL, started in `REV-BENCH/`, so it reads
  `REV-BENCH/CLAUDE.md` and `REV-BENCH/.claude/settings.json`.
- graphify (the code graph in `graphify-out/`) is installed in `~/.local/bin`, version 0.9.92
  (`pip install --user graphifyy==0.9.92`). After pulling, run `graphify update .` in `REV-BENCH/`:
  it remakes its cache and manifest, which are each machine's own and not in git, and catches the
  graph up if a session couldn't. A pull that deletes them can be refused if graphify changed them
  since: `git checkout -- REV-BENCH/graphify-out` first.

## Claude's cloud sessions

- They start in the repo's root (`Personal-projects`), whose `CLAUDE.md` imports
  `REV-BENCH/CLAUDE.md`, and read the root's `.claude/settings.json`, not REV-BENCH's.
- `REV-BENCH/.claude/hooks/session-start.sh` installs the tools: `~/rb-venv` with zig 0.17, numpy,
  matplotlib and graphify 0.9.92 (on PATH), and the web app's `node_modules` in `~/rb-copy/web`.
  It skips what's there and does nothing outside the cloud. Until it's registered as a SessionStart
  hook (below), CLAUDE.md has Claude run it first.

## Registering the hooks (once, by hand)

Claude can't do this step: its permission system blocks it from editing `.claude/settings.json`,
which decides what runs around its own tool calls. Two files, from `Personal-Projects` in WSL:

```bash
cat > REV-BENCH/.claude/settings.json <<'EOF'
{
  "hooks": {
    "SessionStart": [
      { "hooks": [ { "type": "command", "timeout": 600,
          "command": "bash \"$CLAUDE_PROJECT_DIR/.claude/hooks/session-start.sh\"" } ] }
    ],
    "PreToolUse": [
      { "matcher": "Bash|Grep", "hooks": [ { "type": "command", "timeout": 10,
          "command": "sh \"$CLAUDE_PROJECT_DIR/.claude/hooks/graphify-guard.sh\" search" } ] },
      { "matcher": "Read|Glob", "hooks": [ { "type": "command", "timeout": 10,
          "command": "sh \"$CLAUDE_PROJECT_DIR/.claude/hooks/graphify-guard.sh\" read" } ] }
    ]
  },
  "permissions": {
    "allow": [ "Bash(graphify query:*)", "Bash(graphify path:*)",
               "Bash(graphify explain:*)", "Bash(graphify update:*)" ]
  }
}
EOF
mkdir -p .claude
cat > .claude/settings.json <<'EOF'
{
  "hooks": {
    "SessionStart": [
      { "hooks": [ { "type": "command", "timeout": 600,
          "command": "bash \"$CLAUDE_PROJECT_DIR/REV-BENCH/.claude/hooks/session-start.sh\"" } ] }
    ]
  },
  "permissions": {
    "allow": [ "Bash(graphify query:*)", "Bash(graphify path:*)",
               "Bash(graphify explain:*)", "Bash(graphify update:*)" ]
  }
}
EOF
git add REV-BENCH/.claude/settings.json .claude/settings.json
git commit -m "Register the cloud setup and graphify hooks"
git push
```

- The first file is for sessions started in `REV-BENCH/` (yours). The SessionStart hook does nothing
  there; graphify's reminder to query the graph before searching runs as before, but finds graphify
  on PATH or in `~/.local/bin` instead of at one machine's path.
- The second is for cloud sessions: the setup at the start, and no prompts for graphify's queries.
  graphify's reminder stays off there: CLAUDE.md asks for the graph anyway, and the reminder would
  start Python before every call.

## Optional: REV-BENCH in its own repo

Sessions would start inside the project: its CLAUDE.md and settings load first, graphify runs
without a `cd`, and Vercel (stage 25) deploys a repo's root. Claude couldn't do it: its GitHub
connection can't create repositories. The folder stays where it is, so nothing else on your
machine changes.

1. On github.com, a new repository named `REV-BENCH`, completely empty (no README, licence or
   .gitignore). Public, like Personal-projects, or private.
2. Check that Claude's GitHub app can see it: github.com/settings/installations → Claude →
   Repository access (All repositories, or add REV-BENCH).
3. In WSL:

```bash
cd /mnt/c/Users/berna/Personal-Projects
git checkout rev-bench && git pull
git subtree split --prefix=REV-BENCH -b rb-split          # REV-BENCH's history, on its own
git push https://github.com/bernardovh77/REV-BENCH.git rb-split:main
git branch -D rb-split
git rm -r -q --cached REV-BENCH                           # Personal-Projects stops tracking it ...
git rm -q --ignore-unmatch CLAUDE.md .claude/settings.json   # ... and its pointers to it
printf '/REV-BENCH/\n' >> .gitignore                      # the new clone sits at the same path
git add .gitignore && git commit -m "REV-BENCH moved to its own repo" && git push
mv REV-BENCH ../REV-BENCH-old
git clone https://github.com/bernardovh77/REV-BENCH.git REV-BENCH
cp -r ../REV-BENCH-old/renders/. REV-BENCH/renders/        # keep your renders
cd REV-BENCH/web && npm install && cd ..
graphify update .
```

Then start cloud sessions from the REV-BENCH repo, ask the first one to update CLAUDE.md's notes on
the layout (the subfolder, the root's CLAUDE.md, `cd REV-BENCH`), and delete `../REV-BENCH-old`
once everything works.
