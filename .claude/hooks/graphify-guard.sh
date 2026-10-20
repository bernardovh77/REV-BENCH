#!/bin/sh
# PreToolUse hook: graphify's reminder to look in the graph before searching or reading
# files (`graphify hook-guard search|read`; it only nudges). It does nothing in Claude's
# cloud sessions, where CLAUDE.md asks for the graph anyway and the reminder would start
# Python before every call, or wherever graphify isn't installed.
[ "${CLAUDE_CODE_REMOTE:-}" = "true" ] && exit 0
G=$(command -v graphify 2>/dev/null || echo "$HOME/.local/bin/graphify")
[ -x "$G" ] || exit 0
exec "$G" hook-guard "$1"
