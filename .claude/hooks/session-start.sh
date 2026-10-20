#!/bin/bash
# SessionStart hook for Claude's cloud sessions (Claude Code on the web sets
# CLAUDE_CODE_REMOTE=true): installs the tools REV-BENCH's workflow uses. Anywhere
# else (Bernardo's machine, the device bridge) it does nothing. Each step is skipped
# when it's already done, so a container that kept its state starts quickly.
#
#   ~/rb-venv        zig 0.17 for make wasm, numpy + matplotlib for tools/,
#                    graphify 0.9.92 (the version graphify-out/ was built with)
#   ~/rb-copy/web    the web app's node_modules, outside the checkout: CLAUDE.md runs
#                    npm in a copy (sync the sources there before the web tests)
set -euo pipefail
[ "${CLAUDE_CODE_REMOTE:-}" = "true" ] || exit 0

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"   # REV-BENCH, wherever the repo starts
VENV="$HOME/rb-venv"

if ! "$VENV/bin/python" -c "import ziglang, numpy, matplotlib, graphify" 2>/dev/null; then
    [ -x "$VENV/bin/python" ] || python3 -m venv "$VENV"
    "$VENV/bin/pip" install -q --disable-pip-version-check \
        ziglang==0.17.0 numpy matplotlib graphifyy==0.9.92
fi
# graphify on PATH: a link where the system looks, or else the venv at the end of PATH.
if ! ln -sf "$VENV/bin/graphify" /usr/local/bin/graphify 2>/dev/null \
   && [ -n "${CLAUDE_ENV_FILE:-}" ]; then
    echo "export PATH=\"\$PATH:$VENV/bin\"" >> "$CLAUDE_ENV_FILE"
fi

WEB="$HOME/rb-copy/web"
if [ ! -d "$WEB/node_modules" ] || ! cmp -s "$ROOT/web/package-lock.json" "$WEB/package-lock.json"; then
    mkdir -p "$WEB"
    cp "$ROOT/web/package.json" "$ROOT/web/package-lock.json" "$WEB/"
    (cd "$WEB" && npm ci --no-audit --no-fund --loglevel=error)
fi

echo "REV-BENCH tools ready: ~/rb-venv (zig, numpy, matplotlib, graphify) and ~/rb-copy/web/node_modules."
