#!/bin/sh
# PreToolUse hook: graphify's reminder to look in the graph before searching or reading
# files (`graphify hook-guard search|read`; it only nudges). It does nothing in Claude's
# cloud sessions, where CLAUDE.md asks for the graph anyway and the reminder would start
# Python before every call, or wherever graphify isn't installed.
[ "${CLAUDE_CODE_REMOTE:-}" = "true" ] && exit 0
# on PATH, or where it's installed: ~/.local/bin (WSL), the venv (Windows, Linux)
G=$(command -v graphify 2>/dev/null)
for c in "$HOME/.local/bin/graphify" "$HOME/rb-venv/Scripts/graphify.exe" "$HOME/rb-venv/bin/graphify"; do
    [ -n "$G" ] && break
    [ -x "$c" ] && G="$c"
done
[ -n "$G" ] || exit 0
exec "$G" hook-guard "$1"
