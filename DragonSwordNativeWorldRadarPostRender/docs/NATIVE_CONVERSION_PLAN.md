# Native Conversion Plan

## Objective and current checkpoint

Replace the external transparent-window radar with one in-process UE4SS C++
mod that samples gameplay state and renders through the game's own UMG
viewport composition. The stable external radar remains an installed but
disabled rollback until the native build passes gameplay acceptance.

The final runtime must not require a custom executable, DWM transparency,
motion bridge, recurring SQL query, Lua render loop, or recurring full UObject
enumeration.

`2.2.1` is the current fixes-only native release candidate. It preserves
the accepted R6 rendering and sampling schedules and dev67 behavior as its
implementation basis. The preceding 2.1.0 release added the exact startup exclusion
of confirmed nonexistent treasure save ID `11230106` and repaired the fixed 49-bit
Boss/Assault accepted-death handoff across lifecycle boundaries without adding
steady work, and hardened the out-of-process release/installer path. Final 2.1.0
also merges the exact mounted-Rider underwater route and compact-only bird eggs:
two exact classes, a fixed 512-slot weak pool, an eight-position-query budget per
250 ms control tick, a nearest-16 active set, and independent F6 control with no
expanded-map representation. Version 2.1.1 introduced an independent Area Quest
height precursor, gave tasks an in-place closed black shaftless triangle
distinct from the full colored treasure pointer, and added controller-safe
world-map/pause suppression without reading input mappings or adding a timer.
Version 2.2.0 supersedes that precursor with 144 one- or two-band profiles, one
genuine two-band task, and three no-source rows. Authored marker Z selects the
uniquely nearest source band for the multi-band row but never supplies height;
an exact tie or missing profile stays neutral. It also adds a shared mini-game
height channel from all 83 exact map-100
`MiniGame_<kind>_<id>_NPC_Start` heights, three independent height
switches, an 11-language centered resolved-language dropdown, a compatible
`DTextBlock` path with best-effort `ForceApplyLanguageFont` and CDO font
support, exact-zero `Font.Size` seeding, and same-open base `TextBlock` fallback
after game-widget construction, target-size, or font-commit failure, real
`Font.Size` slot-safe layout and
post-viewport/prepass reapply plus readback, discrete all-visible Area Quest
  aligned/up/down shapes, an Off/On/Fault-aware responsive F6 page with top-bar
  Bug Report and Close, read-only status text plus a thin state-colored strip,
  and a separate action that keeps the page open, a bounded no-Font-mutation render-scale text fallback, generated
Korean/Traditional 2x overlays from pinned DroidSansFallback at base size 32
with a one-pixel translucent stroke and role-specific optical baselines, and the
restored inner-atlas world-map host layout after live rejection of the
full-parent experiment. Same-parent zoom/geometry observations now return
`Unchanged` without Remove/Add, reparenting, or an extra atlas render; exact-
C7B gameplay still must prove dense-Treasure stability. The current correction
fixes expanded-map ownership directly: both Mod hosts resolve
`DLayerMap.FogAbovePanel`; `ArrayIconInfo` supplies only a creation-time
instantiable icon class and is absent from steady validation/refresh. Outer host
and inner `Panel_Point` slots are full stretch, zero offset, `AutoSize=false`,
and zero aligned, with maximum Z only on the outer hosts. Only Image Canvas
slots receive `{atlas_left,atlas_top,atlas_width,atlas_height}`; Image render
translation is zero, and no forced layout prepass is used.
Refreshed 2.2.0 Core `2/2`, all static gates, release hygiene, and the clean
native `/W4 /WX` build passed for DLL
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`,
bound to compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
`Build-Release.ps1` package validation passed for that exact DLL: Setup reports
  `20/20`, the manual-copy matrix reports `2/2`, payload equivalence, manual
  layout, and clean-target policy validation pass, and all three public ZIPs
  re-extract byte-identically. Local diagnostics-enabled deployment of
  exact DLL `6AEFDACC...` passed with matching source, build, and installed
  hashes. Its rollback backup is
  `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
  The prior backup
  `dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`.
  belongs to the superseded intermediate 59529B2A deployment and is not current
  candidate evidence. This is not Setup ownership. The earlier 634D283A deployment and backup are
  historical only.

The deployed outer-atlas-rectangle candidate, exact DLL
`CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2` from
compiled source
`433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`, is
runtime rejected. Its negative atlas-left outer slot changed the native parent
extent from `3000x3000` to `3191.520996x3000`; that Mod-induced change caused 6
attaches and 5 detaches, flashing, and a blank map. The run still populated
1,632 markers and reported no data, texture, or ABI fault. Backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`
remains exact-byte rollback evidence only. The current WM-06 immutable-slot candidate
passes source/static gates, Core `2/2`, release hygiene, and the local native
build at DLL
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`, from
compiled source
`0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`, size
1,107,968 bytes. Rollback-backed diagnostics-enabled local developer deployment
passes for those exact bytes. Installed identity is exact, the single controlling
Mod entry is enabled (`mods=1`), and `debug_logging=true`. Backup:
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
Package and installer validation pass for the exact WM-06 bytes: Setup `20/20`,
Manual `2/2`, payload equivalence, layout, clean-target, and all three archive
re-extractions. Gameplay, visual, performance, and resolution acceptance remain
`NOT_VALIDATED`.

The packaged game-1.0.11 owner RVA and member offset `0x128` remain fast paths.
One FullActivation shares a total budget of at most 24 active-`.db` key
validations across packaged and structural owner routes. Packaged-owner failure
permits one unique structural scan that counts only targets inside the mapped
image and scans executable sections through `min(SizeOfRawData, VirtualSize)`.
Structurally incompatible updates fail closed, and no all-future-version
guarantee is made. Current F6 source applies measured desired-size text reflow
to exact font-layout TextBlocks after first open, language changes, status
changes, and language-popup display. `GetDesiredSize` must return the known
`Vector2D` structure identity. Invalid evidence and the render-scale fallback
keep authored text geometry; button hit boxes, map geometry, and the per-frame path
are unchanged. Live visual behavior remains `NOT_VALIDATED`.
Phase 4 controller,
gameplay, responsive-layout, localization-glyph, stability, and performance
acceptance is `NOT_VALIDATED`. Historical 2.1.1 results remain bound to
`dist/final-2.1.1` and packaged DLL SHA-256
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
Historical dev60 evidence diagnosed the prior dungeon-return and
encounter-enumeration defects. Later dev64 gameplay logs additionally exposed a
streaming-disappearance encounter false positive. None of those historical
sessions can accept 2.1.1; source and build results alone are not runtime proof.

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
- Controller-safe menu state suppresses all compact drawing work. Exact
  `SetWorldMapImage` delivery latches immediately, while validated map-layer
  visibility and optional paused state share the existing 250 ms guard. The
  runtime does not read controller bindings or mapping settings, adds no timer,
  and publishes only Boolean suppression to the 16 ms motion path.
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
- Only the nearest treasure is enlarged. The nearest Treasure and nearest
  visible Fly/Mole/Wave marker own independent fixed height allocations, while every visible
  Area Quest evaluates its own height-band profile in the bounded marker pass.
  Treasure retains the full category-colored shafted pointer. A multi-band
  profile uses authored marker Z to choose the uniquely nearest existing source
  band; marker Z never becomes height. Inside the selected band's inclusive
  +/-500 margin, each Area Quest keeps its normal black frame and shows three
  white dots. Below it becomes an up triangle and above it becomes a down
  triangle. An exact-distance tie or missing profile keeps the frame with no
  dots or direction. The shared nearest-mini-game channel uses all 83 trusted
  Fly/Mole/Wave `NPC_Start` heights and a shaftless, actual-kind-colored
  triangle centered below the selected icon. Target Z above points up, target
  Z below points down, and the inclusive +/-500 band hides the triangle.
  Treasure, Area Quest, and mini-game height all compare against
  `playerZ - 150`; Treasure retains its existing dead-zone behavior. Missing
  height hides only that guidance. All three controls default ON on a
  clean install; valid existing choices are preserved. The second-nearest
  treasure is intentionally not retained.
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
- Open and close the world map by controller without a visible cursor, then
  pause and resume. Compact markers must collapse and restore through the exact
  event plus existing 250 ms state edge, with no input-mapping read or new
  timer.
- Verify the task channel uses its independent height-band profile. The authored
  marker Z must choose the uniquely nearest existing source band for the multi-
  band row without becoming height data. Verify the black frame with three
  white dots inside the selected band's inclusive +/-500 margin, up below, down
  above, and neutral for an exact-distance tie or missing profile. Treasure must
  retain its full category-colored pointer.
- Verify the shared Fly/Mole/Wave channel against representative exact actor
  heights above, below, and inside the inclusive +/-500 interval, including
  simultaneous three-channel overlap. Every channel must use comparable
  `playerZ - 150`. Missing or ambiguous mini-game height must hide only the
  shared triangle, not its marker.
- Verify the single responsive F6 page at fullscreen/windowed, DPI, 16:9,
  16:10, and 21:9 sizes with all 11 explicit languages and no displayed AUTO or
  Use Game Language choice. A legacy AUTO value must migrate on the next actual
  F6 opening or F7 activation through `DGameUserSettings.LanguageText`, Kismet,
  and English to one persisted explicit language; no recurring language or font
  work is permitted. Verify Common/TC/JP/TH loaded-game-font selection and safe
  fallback in every script. Verify generated Korean/Traditional 2x overlays use
  pinned DroidSansFallback at base size 32, a one-pixel translucent stroke, and
  role-specific optical baselines. Static no-clipping checks do not replace the
  live visual review. Real
  `Font.Size` must remain within each slot's safe line height, the font must be
  reapplied and read back after `AddToViewport` and prepass, and any failure
  must close F6 fail closed.
- Verify F6 Off/On/Fault status, Enable/Disable/Retry, playable-world activation
  guard, and fixed Nexus Posts Bug Report action.
- Compare frame-time tails and visual FPS with native F7, native F8, and the
  disabled stable rollback under the same route.
- Keep the retired PostRender/Present canary source as historical source-only
  evidence, but compile, connect, and package none of it. Production also creates
  no external shared-memory diagnostic channel.

Exit gate: no crash or stale marker, no accumulating retries or object sets,
and no recurring hitch source attributable to the radar.

Status: the WM-06 immutable-slot architecture passes refreshed source/
static gates, Core `2/2`, release hygiene, and the local native build at DLL
`6435E100...C723A1` from compiled source `0A1A4CE3...B5E5BC5`, size 1,107,968 bytes.
Rollback-backed diagnostics-enabled local developer deployment passes with exact
installed identity, `mods=1`, `debug_logging=true`, and backup
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
Package, installer, and archive evidence remain required. Earlier
2.2.1 technical results remain bound to superseded bytes. The independent-
viewport/extreme-Z deployment is runtime rejected for severe lag, wrong
placement, and delayed updates; the first-valid-parent candidate crossed the fog
boundary; the full-stretch-outer/Image-translation candidate used a mismatched
zoom pivot; and the deployed outer-atlas-rectangle DLL `CD41F0E1...6FBB2`
created its own `3000x3000` to `3191.520996x3000` extent change and 6-attach/
5-detach flashing-empty rebuild loop. Its backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy` is
rejected-byte evidence only. Historical 2.2.0 evidence is not 2.2.1 evidence.
Automated
gates are not substitutes for the dungeon-return, travel, long-session, fault-
recovery, expanded-map visual, resolution, every-language font/glyph, and frame-
time-tail runtime matrix, which remains `NOT_VALIDATED`.

### Phase 5 - Expanded map

The current candidate implements an explicit-session expanded-map snapshot with
4,096 fixed slots and two 2048-by-2048 atlases hosted by hit-test-invisible,
layout-neutral children of the directly resolved `DLayerMap.FogAbovePanel`. The accepted
ceiling is 2,500 Treasure rows plus 279 fixed non-
Treasure rows, or 2,779 total, leaving 1,317 spare slots. After
attachment there is no per-marker tick; the game owns pan, zoom, clipping, and
visibility. State changes retire stale atlases and defer rebuilding until the
next exact `SetWorldMapImage` edge while the retained layer is hidden; a visibly
open exact attachment may rebuild once in the current event service while
preserving the marker snapshot. The rebuild receives its own hard-capped three-
attempt attach/geometry budget, so one open-map session is bounded to initial 3
plus rebuild 3. Expanded-
map work remains idle when the map is closed.

Fresh attachment samples the live `PlayerIconWidget` alignment pivot and the
directly resolved `FogAbovePanel`, then produces native-parent-local atlas
placement. Retained refresh never reads that sibling witness.
`WorldMapUISize` is authored metadata, not the parent extent. Each outer native
Canvas slot uses full-stretch anchors, zero offsets, `AutoSize=false`, zero
alignment, and maximum Z. The cloned host's inner `Panel_Point` Canvas slot uses
the same contract without the Z override. Each locally sized atlas Image Canvas
slot receives `{atlas_left,atlas_top,atlas_width,atlas_height}`, while Image
render translation remains zero; the host receives no render transform and no
forced layout prepass. Pan, zoom, clipping, visibility,
and RetainerBox composition are inherited directly. Initial readiness remains a
separate maximum-three-attempt service. Attach, map-image, F7-resume, and exact
zoom events arm five deadlines at 100/250/500/1,000/1,250 ms from a fresh post-
attach completion time. Attach itself submits no empty Retainer `RequestRender`.
`ArrayIconInfo` is
consulted only if a missing host needs an instantiable icon class; retained-host
validation and refresh never scan it. Each due pass revalidates `FogAbovePanel`
and reads only its live extent. A fully unchanged same-parent pass, including
the final pass, performs no layout, transform, visibility, restack, Remove/Add,
or `RequestRender` work. Anchor-only changes are ignored and the attach-time
Image Canvas slot stays immutable. A real `FogAbovePanel` replacement reports
`RebuildRequired`; only the scheduler performs a fresh attachment. Extent drift must
produce two matching successful observations before returning `RebuildRequired`.
That request reports the condition without pre-collapsing a valid payload; only
an accepted schedule owns detach/rebuild. Retained-RetainerBox or owned-payload
replacement/invalidity may report `RebuildRequired` directly; a final-tail
`RetryLater` closes through the same single bounded rebuild transaction.
There is no authored `3000`/`8000` extent, centered/desktop fallback, or steady
geometry poll. Two decoded BGRA atlases occupy about 32 MiB raw and retain atlas
style revision 51. Persistent cache envelope `DSNWRA52` quantizes its normalized
fingerprint at 1/4096 UMG logical unit. A hit requires exact dimensions/header/
magic/fingerprint/visible count, full RLE decode to exactly 2048-by-2048 pixels,
encoded-payload checksum validation, and exact EOF; revision-51, corrupt,
truncated, or trailing-byte files miss. Writes use a same-directory temporary
file and atomically publish through `MoveFileExW` with replace-existing and
write-through flags, removing the temporary file on failure. Bounded cache I/O
and texture import remain attachment work. Marker
coordinates remain unchanged. Temporary topology diagnostics captured the old
heuristic alternating between `FogAbovePanel` and `FogUnderPanel` as zoom rebuilt
native icon widgets, causing four host reattachments, fog occlusion, hitching,
and flashing. This is development evidence only. The observed 1,632-marker/1,501-Treasure snapshot was
below the old 1,785 limit, so capacity was not the flicker root. The historical
exact 84A360B0 log shows one attach and no repeated detach/rebuild sequence. Expanded-map alignment
and dense-Treasure behavior remain pending live acceptance.

Exit gate: repeated map open/close, travel, map IDs 100/200 normalization, and
all marker types work without regressing compact performance.

Status: structurally specified for the WM-06 immutable-slot 2.2.1
architecture. Source/static gates, Core `2/2`, release hygiene, and the local
build pass at
`6435E100...C723A1` from compiled source `0A1A4CE3...B5E5BC5`, size 1,107,968 bytes.
Rollback-backed diagnostics-enabled local developer deployment passes for the
exact DLL with backup
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
Package/installer evidence is pending. Real
4K/21:9/internal-black-bar/16:10/
windowed gameplay, attach-time, memory, and exact-artifact runtime acceptance
remain `NOT_VALIDATED`. Numeric 3840x1600 unit input and static verification are
not live visual proof.

### Phase 6 - Native-only release

- Use `Build-Release.ps1` as the only release entry point and produce the exact
  installer, Manual-No-UE4SS, and Manual-With-UE4SS archives. Embed one
  source-bound ExperimentalNested
  runtime payload in Setup, including the native DLL, build receipt, immutable
  visibility/diagnostics defaults, exact ten generated catalogs, treasure
  override, SQLCipher runtime, metadata, licenses/notices, and generated package
  manifest. Do not install the `.example.ini` resources or any `enabled.txt`;
  preserve `config/visibility.ini`, `config/diagnostics.ini`, and
  `data/defaults/treasure_overrides.txt` byte-for-byte during developer deploy
  and recognized Update / Repair. Both manual channels use ExperimentalNested; only the With-
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

Status: the WM-06 immutable-slot architecture passes source/static gates,
Core `2/2`, release hygiene, the local native build at `6435E100...C723A1`, and a
rollback-backed diagnostics-enabled local developer deployment with exact
installed identity, `mods=1`, `debug_logging=true`, and backup
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
It still requires Setup, Manual,
payload/copy/clean-target, three-archive byte-identical re-extraction, package
hashing, and final package gates. Earlier 2.2.1 results
remain bound to superseded candidate bytes; the independent-viewport/extreme-Z,
first-valid-parent, Image-translation, and outer-atlas-rectangle deployments are
runtime rejected and accept no hybrid behavior.
Historical 2.2.0 DLL `6AEFDACC...` and earlier backups remain historical only.
Controller, height visual, responsive UI,
localization glyph, gameplay, exit, and performance acceptance remain
`NOT_VALIDATED`. Historical 2.1.1 results remain bound to
`dist/final-2.1.1` and DLL SHA-256
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
Runtime release remains gated on the
Phase 4 matrix. Public publication is independently blocked by the third-party
rights and source-provenance review in `DEPENDENCY_SOURCES.md`.

## Iteration rule

Every runtime build must close one named uncertainty and have a binary pass or
fail observation. Cosmetic probes, repeated attachment experiments, and
unbounded retry changes are not release milestones. Once a mechanism is
proven, subsequent work is batched behind core tests before the next gameplay
test.
