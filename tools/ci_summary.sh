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

# ... and as an annotation, which the run's page lists even where the summary
# and the log don't load: the table and the first FAIL lines, newlines as %0A.
what=$( { grep -E "^(native|wasm|wasm-c|python|web|e2e|cmake) " "$log";
          grep -E "FAIL|Error|error:|Traceback|took over|isn't on PATH" "$log" | head -25; } 2>/dev/null |
        cut -c1-200 | sed 's/%/%25/g' | awk 'BEGIN{ORS="%0A"} {print}')
echo "::error title=$(basename "$log") failed::${what}"
