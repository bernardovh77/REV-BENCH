# Persistence, final acceptance and release

[Canonical tracker](../../PLAN.md) · [Stage protocol](README.md) · [Legacy crosswalk](legacy-map.md) · [The app's design](../app-design.md)

These are stage specifications. Current statuses live only in PLAN.md. Future exit evidence is required, not already achieved. Historical completions retain their original validity limits and test dates.

Since 2026-11-08 the release is a desktop application for Windows, macOS and Linux
([ADR 0004](../adr/0004-desktop-application.md)). These stages keep their numbers. Builds are
files and codes instead of URLs; the release is signed installers and an update channel instead
of a static web deployment.

<a id="stage-095"></a>
## Stage 095 — Versioned build files and the session

**Dependencies:** [037](03-contracts-and-sources.md#stage-037), [067](07-app.md#stage-067), [082](09-turbo.md#stage-082), [088](10-limits.md#stage-088), [094](11-engine-catalog.md#stage-094)

**Deliverable:**
- **`.rbbuild` files:** versioned JSON under contracts/, holding the configuration, the preset's
  revision, tuning, listener, scene and quality. Open, save and save-as, with recent files.
  The OS file type is registered, and drag-and-drop works.
- **Loading is atomic and validated:** the core validates the build, version migrations run in
  order, and a load is one rebuild through the host's transaction.
- **The session autosaves** (the current build and the window's state), and the app reopens where
  it was, even after a crash.
- **A saved build is kept distinct** from a live snapshot or a recording.

**Required exit evidence:**
- Every supported layout and feature round-trips.
- Old and unknown versions, malformed files, a file being written when the app dies, and default
  cases are all tested.
- Loading doesn't rebuild once per field or bring back stale physical state.
- e2e on all three systems.

**Stop/reject rule:** Numeric pipe indices are never the saved identity; an unsupported exhaust is
refused, not silently substituted.

**Legacy coverage:** 24 (save).

**Starting evidence/specification:** [GLOSSARY.md](../../GLOSSARY.md), [contracts/README.md](../../contracts/README.md).

**Execution record:** `docs/stage-095.md`.

<a id="stage-096"></a>
## Stage 096 — Sharing builds

**Dependencies:** [095](#stage-095)

**Deliverable:**
- **Build codes:** a build compressed and base64url-encoded, with its schema version and a size
  limit, copied to the clipboard and pasted back into the app.
- **`rbench://` links** that open a code in the app; the protocol is registered on all three
  systems.
- **Imports are validated by the core**, with clear errors.

**Required exit evidence:**
- A code and a link round-trip on a second, clean machine.
- Size limits, hostile, malformed and unknown data, and migration are tested.
- The same build gives the same canonical description and the same seeded render.

**Stop/reject rule:** No local reference recordings, private file paths or runtime histories in a
shared build. A link only opens a build; it never runs anything else.

**Legacy coverage:** 24 (share).

**Starting evidence/specification:** [docs/app-design.md](../app-design.md) §12.

**Execution record:** `docs/stage-096.md`.

<a id="stage-097"></a>
## Stage 097 — Usability, accessibility and visual polish

**Dependencies:** [067](07-app.md#stage-067), [068](07-app.md#stage-068), [070](08-3d.md#stage-070), [077](08-3d.md#stage-077), [082](09-turbo.md#stage-082), [089](10-limits.md#stage-089), [094](11-engine-catalog.md#stage-094), [096](#stage-096)

**Deliverable:**
- **Look and feel:** finish the layout and both themes, onboarding (a first run that gets you
  playing), and the menus and keyboard shortcuts as each system expects.
- **Controls:** metric and imperial units, and front, side, cutaway and follow-exhaust camera
  presets.
- **Accessibility:** reduced motion, accessible legends, screen-reader labels, and UI scale from
  100 to 200 % and high-DPI on all three systems.
- **Help:** the quality tiers and unsupported hardware explained; visual detail is reduced before
  any declared physics tier changes.

**Required exit evidence:**
- A new user can start and understand the bench within the original 30-second target in
  observed trials.
- Keyboard, focus, screen-reader, scaling and no-3D modes checked on each system.
- Bernardo approves the appearance and interaction.

**Stop/reject rule:** No hidden automatic sound-fidelity downgrade.

**Legacy coverage:** 24 (UX).

**Starting evidence/specification:** [docs/app-design.md](../app-design.md) §11.

**Execution record:** `docs/stage-097.md`.

<a id="stage-098"></a>
## Stage 098 — Final per-preset listening and realism acceptance

**Dependencies:** [039](03-contracts-and-sources.md#stage-039), [063](05-engine-qualification.md#stage-063), [084](09-turbo.md#stage-084), [089](10-limits.md#stage-089), [094](11-engine-catalog.md#stage-094), [097](#stage-097)

**Deliverable:** Review the complete catalog in the app and the consolidated RESULTS.html,
including turbo and all supported listeners, layers, limiters and long coasts. Tune only
physically justified or calibrated defaults, and revisit changed qualifications.

**Required exit evidence:** Record Bernardo's per-preset/scenario verdict, matched-reference
evidence and repeated listening controls. Complete the relevant S1/S1e/S2/S4/S5 obligations
explicitly; retain unresolved findings rather than averaging them away.

**Stop/reject rule:** Final listening cannot waive a failed physical, safety-envelope or runtime
gate; late tuning requires rerunning affected tests.

**Reference entry gate:** Check current claim-specific coverage against the [039 source-search review](../stage-039-reference-search.md) and later qualification records. An implemented reference protocol does not close matched-recording acquisition, real-engine calibration, recording rights, or the per-preset listening gates.

**Legacy coverage:** S5; S1/S1e/S2/S4 (final).

**Starting evidence/specification:** [docs/render-workflow.md](../render-workflow.md).

**Execution record:** `docs/stage-098.md`.

<a id="stage-099"></a>
## Stage 099 — Release qualification on the supported systems and hardware

**Dependencies:** [077](08-3d.md#stage-077), [084](09-turbo.md#stage-084), [089](10-limits.md#stage-089), [094](11-engine-catalog.md#stage-094), [096](#stage-096), [097](#stage-097), [098](#stage-098)

**Deliverable:** Qualify the whole product (3D, turbo, damage, controls, saving and sharing) on
the declared support matrix:
- Windows 10 and 11;
- macOS on Apple Silicon and Intel, at the pinned Electron's minimum and the newest;
- Linux: Ubuntu LTS and one other distribution, X11 and Wayland, PipeWire and PulseAudio;
- the named machines and GPUs.

Measure the audio deadline tails and maxima, frame times, memory, and robustness through the
lifecycle: device changes, sleep and wake, display changes, GPU loss, long sessions.

**Required exit evidence:**
- The declared long-session and stress matrix shows zero observed underruns and no missed audio
  deadlines, bounded memory, and the supported frame and latency targets met.
- Installed-build e2e on each system.
- Cross-compiler parity within stage 122's tolerance.
- The remaining legacy 4×/2× goals settled with Bernardo if unachieved.

**Stop/reject rule:** No claim of universal support or glitch freedom from a finite run. Failed
targets need a bounded fix or an explicit scope or hardware decision before release.

**Legacy coverage:** 13b-c; 13b; 14; 19; 24; R17.

**Starting evidence/specification:** [docs/sound-robustness-plan.md](../sound-robustness-plan.md).

**Execution record:** `docs/stage-099.md`.

<a id="stage-100"></a>
## Stage 100 — Release documentation, licences and asset review

**Dependencies:** [099](#stage-099)

**Deliverable:**
- **Documentation:** the how-it-works guide (the equations and their limits), installation
  instructions per system (including what to do about an unsigned build's warnings), the
  supported hardware, screenshots, building from source, saving and sharing, and the
  stage-evidence index.
- **Licences:** choose the app's licence, write `THIRD_PARTY_NOTICES` from every bundled
  dependency (Electron and Chromium, miniaudio, Svelte, Three.js, uPlot, Inter, ...), and check
  the redistribution of every public asset. Local user recordings stay ignored.

**Required exit evidence:**
- A clean checkout builds the installers on each system from the documented, pinned toolchain.
- The docs' links and commands, the notices' completeness (checked by a script against the
  lockfiles and vendored code) and the public asset inventory are all verified.
- Historical experiments are kept apart from shipped claims.

**Stop/reject rule:** No unverified realism claim, copyrighted local reference clip or stale test
count as release evidence.

**Decision (Bernardo):** the app's licence.

**Legacy coverage:** 25 (documentation).

**Starting evidence/specification:** [README.md](../../README.md), [RESEARCH.md](../../RESEARCH.md).

**Execution record:** `docs/stage-100.md`.

<a id="stage-101"></a>
## Stage 101 — Signed installers and the update channel

**Dependencies:** [100](#stage-100)

**Deliverable:**
- **Release builds** of the installers for all three systems, from CI, from a tagged commit.
- **macOS** signed with a Developer ID and notarised; **Windows** Authenticode-signed if Bernardo
  takes a certificate. Unsigned otherwise, with the warnings documented.
- **electron-updater** against GitHub Releases: an update check, a download, install on quit, and
  a setting to turn checks off. Updates are verified (signatures or hashes).
- **A draft GitHub release** with checksums, and a rollback path (the previous release kept).

**Required exit evidence:**
- Each installer installs, launches, updates from the previous build and uninstalls cleanly on
  each system.
- The macOS build passes Gatekeeper.
- No secrets or local references in any package (checked by a script).
- Publication authorisation is verified from the actual request; this plan authorises nothing.

**Stop/reject rule:** A local build that installs is not a release. Publishing needs Bernardo's
explicit authorisation.

**Decision (Bernardo), deferred until the app works (2026-11-08):** the Apple Developer
Program ($99 a year) and a Windows certificate. Until this stage every build is unsigned.

**Legacy coverage:** 25 (deployment).

**Starting evidence/specification:** [docs/app-design.md](../app-design.md) §13.

**Execution record:** `docs/stage-101.md`.

<a id="stage-102"></a>
## Stage 102 — Public release and clean-machine acceptance

**Dependencies:** [101](#stage-101)

**Deliverable:** Publish the release, install it from the public page on clean machines for each
system (Bernardo's and a second machine per system where available), and complete the public
README, demo media and portfolio links with the supported envelope stated, plus a final
acceptance record.

**Required exit evidence:**
- Download, install, start, play, controls, dyno, 3D, saving and sharing, turbo, limits and an
  update all work from the published release on each system.
- Listening, performance and first-run onboarding checked on the clean machines.

**Stop/reject rule:** A published release alone does not finish the project. Open mandatory
feature, physics, listening or hardware gates block full-release status.

**Legacy coverage:** 25 (launch).

**Starting evidence/specification:** [docs/roadmap/README.md](README.md).

**Execution record:** `docs/stage-102.md`.
