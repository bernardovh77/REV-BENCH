# Stage 112 — Recoverable checkpoint section publication

2026-10-23. Baseline `fba9bf04c32b0a09801caf18abd70cabd5b9bcc7`;
user-added `.agents/` and `skills-lock.json` excluded. Bernardo authorizes the
review fixes, followed by a saved checkpoint and hold. Status lives in PLAN.md.

## Scope, contract and method

Closes the newly reproduced C08 in the [completed-stage review](review-completed-stages-2026-10-23.md).
The existing public seams are `publish_section` and `compose`: together they
test publication, preservation, rebuild and retry, not physical audio accuracy.
The user request authorizes fixing those reproduced boundary failures; no new
publisher interface or sound generation method is introduced.

A producer writes a temporary complete UTF-8 fragment, verifies its SHA-256,
then installs an immutable content-addressed fragment. The manifest references
only complete files. Existing legacy fragment paths remain readable. The
page is composed before manifest replacement; a failed page installation
restores the previous manifest atomically. Temporary files are cleaned up.
Registered fragments are never overwritten to implement an update.

Calls are serialized. Ordinary exceptions preserve the previous rebuildable
checkpoint. Page and manifest remain separate atomic replacements: this is
not a multi-file power-loss transaction, concurrent-writer protocol, or guarantee
of rollback after persistent storage failure. Unreferenced immutable fragments
can remain after a failed attempt; they are local internal assets, not additional
published result pages. No reference or measurement audio is rewritten.

## Frozen checks and execution

The first regression failed against the baseline: a deliberately partial
fragment write raised an exception and the next composition failed its hash
check. The repaired implementation passes that same test. Additional filesystem
faults cover page write, manifest write, manifest installation and page
installation. Each verifies preserved page/manifest, successful recomposition,
other-section preservation, temporary cleanup and successful retry.

All **45 Python tests** pass, including the three new publisher tests (five
failure locations across their cases). The standard complete-page rebuild
passes, and Chromium decodes/plays **all 48 players offline**, checks exclusive
playback, all four top-band jump buttons and the eight retained plots. There
are no page errors or network requests. All 48 embedded audio payloads and 158
protected source/audio/reference/Claude files match the preceding checkpoint.

A third regression emulates Windows text defaults: Unicode content and newline
translation must not invalidate a fragment fingerprint or garble the result.
It first reproduced a hash failure in the initial repair; explicit UTF-8 and
unchanged newline bytes now pass the same check. This is an emulated filesystem
text-default check, not a native Windows browser/runtime qualification.

Shared validation also passes: **196 native tests / 7897 checks**, **17 web
unit tests**, and ASan/UBSan **18 candidate tests / 2593 checks**. No production
core/WASM change requires a new WASM build or changed sound render here.

Reproduction commands from the repository root:

```sh
PYTHONPATH=tools python3 -m unittest test_checkpoint_sections test_render_results
python3 -S tools/render_results.py --publish-only
```

Roadmap validation confirms 113 unique stages, an acyclic dependency graph,
all 62 legacy mappings, all 27 audit owners and 978 local links. Graphify's
native AST refresh yields 2524 nodes / 6770 edges / 151 communities, retaining
the known `rb.h:415` parser warning; no semantic-document extraction is claimed.

The original failing test log, passing suite logs, browser evidence, preservation
report and source hashes are local under ignored
`renders/evidence/fixes-112-113-2026-10-23/`. Fault injection only uses temporary
folders; the actual accepted checkpoint is never deliberately corrupted.
The partial-write test detects the original in-place overwrite behavior.

**Decision:** C08 is repaired within the exception-recovery contract above.
Keep the broader physics/product obligations open. Save/sync this checkpoint
and hold for Bernardo; Stage 037 is not started.
