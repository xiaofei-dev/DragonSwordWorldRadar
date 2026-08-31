# Native Conversion Plan

## Objective and current checkpoint

Replace the external transparent-window radar with one in-process UE4SS C++
mod that samples gameplay state and renders through the game's own UMG
viewport composition. The stable external radar remains an installed but
disabled rollback until the native build passes gameplay acceptance.

The final runtime must not require a custom executable, DWM transparency,
motion bridge, recurring SQL query, Lua render loop, or recurring full UObject
enumeration.

`2.1.0` is the current native-only technical release
candidate. It preserves the accepted R6 rendering and sampling schedules and
dev67 behavior as its implementation basis. R7 added the exact startup exclusion
of confirmed nonexistent treasure save ID `11230106` and repaired the fixed 49-bit
Boss/Assault accepted-death handoff across lifecycle boundaries without adding
steady work, and hardens the out-of-process release/installer path. Final 2.1.0
also merges the exact mounted-Rider underwater route and compact-only bird eggs:
two exact classes, a fixed 512-slot weak pool, an eight-position-query budget per
250 ms control tick, a nearest-16 active set, and independent F6 control with no
expanded-map representation. The feature conversion and deterministic
  three-channel ExperimentalNested packaging contracts are implemented. The
  current source is validated through a clean native `/WX` build, Setup
  `20/20`, manual-copy `2/2`, payload equivalence, and archive re-extraction.
  Its `main.dll` SHA-256 is
  `D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`;
  the authoritative package is `dist/final-2.1.0`. Deployment and Phase 4
  gameplay/stability/performance acceptance are not complete. Earlier
  `4AFE...` and `BDE21...` package sets are historical and non-authoritative.
Historical dev60 evidence diagnosed the prior dungeon-return and
encounter-enumeration defects. Later dev64 gameplay logs additionally exposed a
streaming-disappearance encounter false positive. None of those historical
sessions can accept 2.1.0 from source, build, or package results alone.

## Accepted foundation

`0.6.0-dev11-widget-owner-fix` is the accepted route proof:

- F7 creates a game-owned UMG marker above the compact minimap.
- Native player coordinates move the marker at a 16 ms cadence in dev21.
- F8 removes it and stops object/render work; F7 can attach it again.
- The runtime log records successful attachment and thousands of updates with
  no UMG fault.

This proves the render route only. It is not feature parity.

## Non-negotiable design

- One game-Blueprint viewport UMG host per valid world epoch. Preserve its
  engine-initialized WidgetTree and attach the pool to an existing CanvasPanel;
  never replace WidgetTree or RootWidget after `WidgetBlueprintLibrary.Create`.
- A fixed-capacity reusable four-piece `UBorder` chest marker pool; no per-frame widget creation,
  attachment, removal, container allocation, lookup, file access, or logging.
- Weak UObject identities only across callbacks. Pawn, Controller, World,
  Canvas, Widget, and Actor raw pointers never cross a callback boundary.
- Compact position updates are capped at 16 ms. When bindings are unchanged,
  steady state performs one root-Canvas render-translation update rather than
  one call per marker. Catalog selection and state
  filtering run only after meaningful movement, a state-version change, or a
  bounded five-second deadline.
- F8 disables feature work, detaches the compact host, collapses a still-valid
  expanded-map host, clears activation-local observations, advances lifecycle
  state, and stops coordinate, game-tick object, clock, and render work. The
  fixed UObject-create listener remains event-only so a same-world F7 can reuse
  weak layer and encounter candidates without polling or enumeration.
- Travel fails closed before old-world references are released and rebuilds
  only after stable player-position evidence in the new open world.
- Menu cursor state suppresses all compact drawing work and resumes after the
  accepted 250 ms guard.
- F7 permits one save reconciliation attempt. A failure waits for the next F7;
  runtime completion comes from native interaction and lifecycle evidence.
- Engine-tick SEH recovery is fail closed and limited to one automatic attempt
  for the entire process. World-map and Hub runtime-only recovery require later
  explicit F7/F6 input after clean detach and valid ABI. No recovery budget may
  replenish itself or become a watchdog/steady retry loop.
- No DXGI Present or D3D12 injection. The rejected GPU-crash route remains out
  of the compiled target.

## Authoritative compact contract

- Render catalog: 1,693 treasure points; actor-state catalog: 1,692 points.
- Encounters: 9 Boss and 40 Assault points.
- Minigames: 33 Fly, 40 Mole/hammer, and 10 Wave points.
- World-map ID 100 supplies compact open-world points.
- Reference geometry is 2560 x 1440, with a 360 px compact square and 170 px
  usable radius. Display scale is `min(width / 2560, height / 1440)`.
- Range is 125 m in town and 225 m in the field.
- At most 80 nearby treasures are selected into a fixed buffer.
- Treasure selection is reconsidered only after more than 1000 UE units of
  movement, more than 10 UE units of Z change, a range/state-version change,
  or the bounded five-second selection deadline.
- Only the nearest treasure is enlarged. It and the nearest visible area quest
  own independent fixed six-piece height channels, use color-distinguished
  fills, and may appear simultaneously. The second-nearest treasure is
  intentionally not retained.
- Draw order is Encounter, normal treasure, nearest treasure, minigame, then
  clock/status.

Build-time data preparation emits immutable unified catalogs containing map ID,
save/reward ID, type, position, and required condition fields. Release archives
ship those validated catalogs and never a runtime or installation-time data
generator. The actor TSV alone is insufficient for rendering.

## Delivery phases and gates

### Phase 0 - Source and evidence baseline

- Keep the dev11 DLL and runtime evidence reproducible.
- Keep all native source changes visible in this repository path.
- Reject deployment when required generated catalogs are missing or malformed.

Exit gate: source-only core tests and the pinned native build pass without
touching another mod's source.

### Phase 1 - Real compact treasure pool

- Replace the canary class with one UMG host and a reusable marker pool.
- Feed actual nearby treasure points through the accepted compact projection.
- Implement fixed-buffer selection, one nearest enlargement, and height arrow.
- Preserve F7, F8, menu suppression, travel reset, and dungeon suppression.

Exit gate: real markers track player motion, F8/F7 works, menu and travel do
not leak markers, and steady-state updates contain no allocation or lookup.

### Phase 2 - Native state filtering (dev21 accepted coordinate foundation)

- Generate and load the unified render catalog once.
- Perform one F7 save reconciliation without recurring polling.
- Apply native treasure-open events immediately and prevent late save results
  from resurrecting an event completed in the current activation.
- Preserve override and alias behavior.

Dev17 implemented the one-shot SQLCipher worker, activation-token validation,
fail-closed initial eligibility, static duplicate ignore rule, and immediate
runtime-opened precedence. Dev18 corrects its render catalog to preserve map
IDs, detaches the cloned stock arrow, and isolates motion to the root Canvas.
Dev19 restores the 80-marker bound, clears the complete authored Blueprint
visual subtree, and selects the 125- or 225-metre projection from a guarded
once-per-second live minimap-scale read. Dev20 restores stable 16/10 sizing and
uses a temporary nearest-only independent viewport arrow to isolate the
remaining mismatch. Its runtime evidence proves the selector and radius remain
bounded while the UMG host overflows visually. Dev21 removes that witness and
converts all host-local lengths through one attachment-time viewport-DPI scalar
while retaining one root-Canvas translation in the 16 ms path. Gameplay
acceptance remains pending.

Exit gate: initial unknown state draws nothing; completed points stay hidden;
three open/leave/teleport interaction cases update without SQL polling.

### Phase 3 - Feature parity

Dev22 implements the compact content candidate without changing dev21
coordinates or steady motion. It uses the same fixed 80-slot pool for stable
white/green/orange/blue treasure chests, a nearest-only stable height pointer,
9 Boss, 40 Assault, 33 Fly, 40 Mole, and 10 Wave records. The single F7 save
snapshot also returns `tb_actor_respawn` rows; native defeat events and an
elapsed 120-minute cooldown update availability without polling. Runtime
acceptance remains pending.
Dev33 restores the fixed native clock and adds 147 generated area-quest records.
Quest eligibility is reflected from the game's standalone quest state only on
F7, travel, or quest-end triggers, one ID per game frame. Only `ACCEPTABLE` and
`PROGRESS` are rendered; all other or unavailable states fail closed. The same
numeric eligibility feeds compact selection and the one-shot expanded atlas,
without adding an idle query cadence.
- Add 9 Boss and 40 Assault markers, 120-minute cooldowns, and conditioned
  Assault fail-closed behavior.
- Add 33 Fly, 40 Mole/hammer, and 10 Wave markers with reward filtering.
- Add compact-only `Bird_Egg01_C` and `Bird_Egg02_C` markers through fixed weak
  candidates, with no enumeration, SQL, dynamic queue, or world-map work.
- Move the clock/status presentation to native data and UMG primitives.
- Keep F6 as the user-facing native visibility Hub; release behavior must not
  depend on a debug-only hotkey or overlay.
- Keep Assault display filtering explicit in F6: `AVAILABLE` preserves the
  authored state, time-window, and future-cooldown gates; `ALL` is a static
  presentation view of all 40 Assault records. Neither mode alters completion,
  cooldown, Boss, or area-quest authority.

Exit gate: counts, colors, sizes, order, filtering, and hotkeys match the stable
compact radar in representative locations.

### Phase 4 - Lifecycle and performance acceptance

- Test open-world travel, buildings, menu, dungeon entry, F7 inside a dungeon,
  F8/F7 recovery, resolution/UI scale, game exit, and long sessions.
- Test both exact bird-egg classes, the independent compact-only F6 toggle,
  nearest-16 bound, lifecycle cleanup, and the absence of expanded-map markers.
- Test F6 Assault `AVAILABLE` / `ALL` before, during, and after the single authored
  time window and across a real future cooldown; ALL must keep all 40 records,
  switching back to AVAILABLE must restore the live filters immediately, and
  Boss and area-quest visibility must not change.
- Compare frame-time tails and visual FPS with native F7, native F8, and the
  disabled stable rollback under the same route.
- Keep the retired PostRender/Present canary source as historical source-only
  evidence, but compile, connect, and package none of it. Production also creates
  no external shared-memory diagnostic channel.

Exit gate: no crash or stale marker, no accumulating retries or object sets,
and no recurring hitch source attributable to the radar.

Status: pending for exact-artifact 2.1.0. Source/static/package gates are not substitutes for
the dungeon-return, travel, long-session, fault-recovery, and frame-time-tail
runtime matrix.

### Phase 5 - Expanded map

The current candidate implements an explicit-session expanded-map snapshot and
two 2048-by-2048 atlases attached to the game-owned map-icon Canvas. After
attachment there is no per-marker tick; the game owns pan, zoom, clipping, and
visibility. State changes retire stale atlases and defer rebuilding until the
next exact `SetWorldMapImage` edge while the retained layer is hidden; a visibly
open exact attachment may rebuild once in the current event service. Expanded-
map work remains idle when the map is closed.

Projection uses the live `PlayerIconWidget` alignment pivot transformed into
the exact retained/witnessed native icon Canvas and scales world X/Y deltas by
that parent's current local width/height. `WorldMapUISize` is authored metadata,
not the parent extent. Initial readiness remains a separate maximum-three-
attempt service. Attach, map-image, F7-resume, and exact zoom events arm five
deadlines at 100/250/500/1,000/1,250 ms. Each due pass takes one observation;
overdue deadlines remain due and advance only one observation per later pass.
The first four passes are read-only. Only the final pass may mutate after stable
parent-local geometry. Equal extents may rebase the retained hosts, while one
stable extent change may consume one bounded full attach for the current
baseline; only a successful fresh attach establishes the next baseline. There
is no centered/desktop fallback or steady geometry poll.

Exit gate: repeated map open/close, travel, map IDs 100/200 normalization, and
all marker types work without regressing compact performance.

Status: structurally implemented and built; current-build packaging, deployment,
real 21:9/internal-black-bar/windowed gameplay, and exact-artifact 2.1.0 runtime
acceptance remain pending. Numeric 3840x1600 unit input is not runtime proof.

### Phase 6 - Native-only release

- Use `Build-Release.ps1` as the only release entry point and produce the exact
  installer, Manual-No-UE4SS, and Manual-With-UE4SS archives. Embed one
  source-bound ExperimentalNested
  runtime payload in Setup, including the native DLL, build receipt, immutable
  visibility/diagnostics defaults, exact ten generated catalogs, treasure
  override, SQLCipher runtime, metadata, licenses/notices, and generated package
  manifest. Do not install the `.example.ini` resources or any `enabled.txt`;
  preserve the two live configuration files and treasure override independently
  during an update. Both manual channels use ExperimentalNested; only the With-
  UE4SS archive includes the pinned runtime. Never restore StableRoot.
  Legacy `Stage-Release.ps1` and `Install.cmd` are retired mutation paths.
- Remove the external executable, host/watcher, transparent window, Lua render
  loop, shared-memory motion bridge, and recurring SQL/file polling.
- Retain the stable external package disabled as rollback until final user
  acceptance.
- Treat any source archive as a separate publication decision. Re-extract and
  hash all three binary archives, bind every embedded resource to its
  allowlisted source hash, and reject stale versions, unmanifested payload, and
  `.orig`, `.patch`, `.candidate`, or `.dev*` scratch files.

Status: deterministic staging, hygiene, Setup `20/20`, manual-copy `2/2`,
payload-equivalence, and archive re-extraction gates pass for the authoritative
2.1.0 package. Runtime release remains gated on the Phase 4 matrix. Public publication is
independently blocked by the third-party rights and source-provenance review in
`DEPENDENCY_SOURCES.md`.

## Iteration rule

Every runtime build must close one named uncertainty and have a binary pass or
fail observation. Cosmetic probes, repeated attachment experiments, and
unbounded retry changes are not release milestones. Once a mechanism is
proven, subsequent work is batched behind core tests before the next gameplay
test.
