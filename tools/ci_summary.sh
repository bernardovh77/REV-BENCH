#!/bin/sh
# Puts what failed into a GitHub Actions run's summary page (stage 122): the
# result table, every FAIL line and the log's last lines, from a test log.
# The summary shows on the run's page without opening the job's log.
#
#   tools/ci_summary.sh LOG
log="$1"
{
    echo "### Failed: $(basename "$log")"
    echo '```'
    grep -E "^(suite|native|wasm|wasm-c|python|web|e2e|cmake) " "$log" || true
    echo
    grep -E "FAIL|Error|error:|Traceback|took over|isn't on PATH" "$log" | head -60 || true
    echo
    echo "... the last lines:"
    tail -60 "$log"
    echo '```'
} >> "${GITHUB_STEP_SUMMARY:-/dev/stdout}"
