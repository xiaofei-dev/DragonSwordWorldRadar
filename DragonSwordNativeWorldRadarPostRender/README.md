# DragonSwordNativeWorldRadarPostRender

Native UE4SS C++ successor to DragonSword World Radar. It renders inside the
game's UMG composition tree and ships no external runtime executable or Lua
runtime. The Windows Setup executable is installation tooling only.

Current release candidate: `2.2.1`.

Runtime label: `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1`.

Current START identity:

```text
START version=2.2.1 runtime_label=DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1
```

Version `1.2.0` was never published. Its candidate changes are included in
`2.1.0`; there is no separate public `1.2.0` package.

Version 2.2.1 is a fixes-only world-map stability candidate. The game-native
map-icon Canvas is now read only and supplies geometry plus the live player-
icon witness only. Both Mod atlas hosts are independent hit-test-invisible
viewport widgets, so Mod-owned geometry cannot affect native desired size,
prepass, layout, icon positions, or click targets. Same-parent pan, zoom, DPI,
and layout observations update only host transforms; they do not rerasterize
atlases, rebuild marker data, remove/add, or reparent.

The exact 2.2.0 `main.dll` SHA-256
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`, compiled-
source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`, Core
`2/2`, static gates, clean `/W4 /WX` build, Setup `20/20`, Manual `2/2`,
payload-equivalence, archive, package, and rollback-backed developer-deployment
results remain historical 2.2.0 evidence. They are not 2.2.1 evidence.

Exact-artifact 2.2.1 source review, Core `2/2`, static gates, native build
`444/444`, package validation, Setup `20/20`, Manual `2/2`, payload equivalence,
layout, clean-target policy, three-archive re-extraction, and rollback-backed
developer deployment passed for DLL SHA-256
`C21823088E38D2BD1635651981187AB4C01C2FFD0DCD4804CB9FFDB1899FABB9` and
compiled-source SHA-256
`DE0100B2D4DE894FA94C6911AD328F7699C55D50EC21193C688B44F7F2588BA2`.
Gameplay, native-icon stability, click-target alignment, controller behavior,
DPI/aspect-ratio presentation, exit behavior, and external performance remain
`NOT_VALIDATED`. Binary publication remains blocked by the recorded provenance
and rights reviews. See
[Runtime Feedback Audit for 2.2.1](docs/RUNTIME_FEEDBACK_AUDIT_2_2_1.md).

## Current feature set

Version 2.2.0 adds three independently controlled compact-radar height
indicators, all enabled on a clean install. Treasure retains its unchanged
category-colored six-piece full arrow to the left of the selected chest. Every visible Area Quest uses a generated one- or
  two-band height profile. A multi-band profile uses authored marker Z to select
  the uniquely nearest existing source band; marker Z is selection evidence,
  never a synthetic height. Its original black frame shows three white dots
  while the comparable player Z is within the selected band's inclusive +/-500
  vertical-unit margin. A player below that band gets an upward triangle and a
  player above it gets a downward triangle. An exact-distance tie or missing
  source profile shows neither dots nor direction.
There is no separate Area Quest arrow. Fly, Mole, and Wave share one nearest-
mini-game height channel. Its shaftless triangle is always centered directly
below the selected mini-game icon, uses that marker's actual Fly, Mole, or Wave
  palette, and has a near-black contrast outline. The outline changes neither
  triangle size nor position nor projection. A target more than 500 vertical units above the comparable player
  Z shows an up triangle; a target more than 500 units below shows a down
  triangle; the inclusive +/-500 band hides the triangle. All three height
  channels use the same comparable `playerZ - 150`; Treasure keeps its existing
  arrow geometry and dead-zone behavior. Exact `NPC_Start`
heights cover all 83 map-100 mini-games: 33 Fly, 40 Mole, and 10 Wave. If a
trusted height is missing or ambiguous, only the mini-game triangle fails
closed hidden; the marker remains. The persisted configuration key remains
`mole` for compatibility.

F6 is one responsive settings page with Mod Status, Language, Marker
Visibility, Height Indicators (Radar Only), and Filter Modes. Bug Report and
Close are separate controls in the top bar. It can open while Radar is Off, On,
or Faulted and fits/clamps to the available viewport instead of assuming one
fixed resolution. Status itself is read-only text with a thin state-colored
strip. Enable, Disable, or Retry is a separate action; using it
keeps the page open, and enabling still requires a loaded playable world.
Translucent section cards, equal-width filter choices, aligned text, and bounded
non-overlapping hit regions are presentation-only.
The centered language dropdown contains only English, Japanese, Korean,
Simplified Chinese, Traditional Chinese, French, German, Spanish (Spain),
Russian, Thai, and Portuguese (Brazil). It does not display an AUTO or Use Game
Language choice. A legacy persisted AUTO value is migration input only: the
next actual F6 opening or F7 activation resolves `DGameUserSettings.LanguageText`,
falls back to Kismet and then English, and converts it to one explicit persisted
language. An explicit selection persists and takes precedence. F6 uses the
already-loaded game fonts `DsCompositFont_CommonSystem` for Korean and
Latin/Cyrillic languages, `DsCompositFont_TCSystem` for both Chinese choices,
`DsCompositFont_JPSystem` for Japanese, and `DsCompositFont_THSystem` for Thai.
If the required loaded object is unavailable, text falls back safely without
changing the selected language, guessing an asset path, or replacing
FontMaterial. This exact-artifact glyph behavior still requires live acceptance.
The four weak font identities are retried only on an actual F6 open when one is
missing or expired; there is no closed-panel or tick scan. The external
  `DS_HYFont_P.pak` overrides Common/TC and does not provide complete glyph
  coverage. Disable or replace that PAK for localization QA. Raw Pretendard
  FontFace assets are not valid `UFont` substitutes and are not bundled or routed.
  Korean and Traditional Chinese fixed labels also use generated 2x overlays
  from the pinned DroidSansFallback source. The canonical payload under
  `assets/ui/f6` contains `ko-{off,on,fault}.tga`,
  `zh-hant-{off,on,fault}.tga`, `language-popup.tga`, and `manifest.json`. Each
  status-specific main overlay replaces all 30 fixed main-panel text slots for
  its language; the shared popup replaces only the Korean and Traditional-
  Chinese language names. The other nine languages continue through native game
  fonts. The overlays are regenerated for the current top-bar, status, and
  filter-row coordinates and use base size 32, a one-pixel translucent stroke,
  and role-specific optical baselines. Static generation checks prove the slots
  do not clip; live in-game size, weight, and alignment review is still required.
If the newly constructed text
widget reports exactly `Font.Size == 0`, one bounded reference size is seeded.
The reflected `FSlateFontInfo.Size` path accepts floating-point metrics, while
integer metrics such as `LetterSpacing` retain bounded integer handling.
If game-widget construction, target-size calculation, or font commit fails,
the same F6 open retries that text once as base UMG `TextBlock`. The real
reflected `Font.Size` participates in layout and
is limited by each slot's safe line height. After `AddToViewport` and layout
prepass, exact-size records are reapplied through a copied `SetFont` parameter
and read back from the widget. If both exact-size
paths fail, a fresh DTextBlock then base TextBlock leaves Font untouched and
uses the same bounded viewport/DPI render scale with a justification-aware
pivot; reaching that emergency path is not visual acceptance. Core UMG
creation, `SetText`, tree insertion, and viewport attachment remain fail
closed. There is no per-frame language or font work.

- Compact map: 80 preallocated marker slots on one moving Canvas root,
  including vertical-oval runtime-only bird-egg markers for exact `Bird_Egg01_C`
  and `Bird_Egg02_C` actors.
- Expanded map: two co-located 3072-by-3072 event-built atlases remain fixed at
  4,096 entries, but their outer hosts are independent hit-test-invisible
  viewport widgets. The game-owned map-icon Canvas is a read-only geometry and
  live-player witness; the Mod never inserts, removes, sizes, orders, prepasses,
  or hit-tests a child there. Native Canvas and player cached Slate geometry are
  transformed through `LocalToAbsolute`, then into viewport-local space through
  the game viewport's `AbsoluteToLocal`. The existing DPI, zoom, player-anchor,
  independent-X/Y, and aspect-ratio projection chain is retained. Same-parent
  pan, zoom, DPI, or layout observations update only the two viewport-host
  transforms; they do not rerasterize atlases, rebuild or reproject marker data,
  remove/add hosts, or mutate the native Canvas. No `3000`/`8000` geometry
  constant substitutes for live geometry. The accepted maximum remains 2,500
  Treasures plus 279 fixed non-Treasure rows, or 2,779 total, leaving 1,317
  spare slots. The observed 1,632-marker/1,501-Treasure snapshot was below the
  old 1,785 limit, so capacity was not the dense-map flicker root. World-map
  glyph style revision 50 retains 50 percent more linear raster density and
  about 72 MiB raw for two decoded BGRA atlases versus about 32 MiB at 2048.
  The renderer observes the exact
  `/Script/DSClient.DPanelWorldMap:OnSliderValueChanged` event and schedules the
  existing finite five-deadline tail at 100, 250, 500, 1,000, and 1,250 ms.
  Each due game-thread pass takes one fresh numeric observation; only numeric
  geometry, transform, stability, and timing state cross observations. Missing,
  implausible, mismatched, or final-unstable geometry collapses the Mod-owned
  hosts and fails closed or defers. No steady layering poll is added. Exact-
  artifact 2.2.1 alignment, native-icon stability, click-target, dense-Treasure,
  aspect-ratio, DPI, controller, exit, and performance acceptance remain pending.
- Catalogs: 1,693 immutable treasure render records, 1,692 actor-backed
  treasure records, 9 Bosses, 40 Assaults, 33 Fly, 40 Mole, 10 Wave, and 147
  area quests. The sole render-only treasure, save ID `11230106`, is excluded
  exactly at startup.
- Native state: current player position, treasure interaction/disappearance,
  exact observed encounter death events with a conservative disappearance
  fallback, one full F7 save reconciliation, bounded event-driven completion
  confirmation, game time, area-quest state, and compact-only bird-egg presence.
- Controls: F6 opens the native visibility Hub; F7 activates; F8 disables
  rendering and activation-local task state,
  so F8 followed by F7 performs an explicit resynchronization. Proven native
  treasure and Boss/Assault cooldown deltas remain process-local so an older
  one-shot save snapshot cannot revive them. A confirmed non-open-world
  activity suppresses both renderers. Open-world interiors remain eligible.
  F7 re-reads save-backed and current runtime state; it does not reinstall the
  Mod, extract game PAKs, or regenerate immutable catalogs.
  Entering the exact `TitleMap` is a separate cross-save hard boundary: the
  radar disables, detaches both renderers, and clears all mutable save-owned
  runtime state. Loading another save does not reactivate it automatically;
  press F7 only after the new save has reached a loaded open world.

Controller menu suppression does not read controller mappings or bindings. An
exact `SetWorldMapImage` edge latches world-map suppression immediately; the
current layer's `IsVisible` state and optional `IsGamePaused` state are sampled
only through the existing shared 250 ms activity service for bounded catch-up
and release. The 16 ms compact path consumes the resulting Booleans only. This
adds no controller poll, focus hook, new timer, UObject scan, allocation, or
recurring diagnostic record.

F6 also stores one `AREA QUEST MODE` choice. `AVAILABLE` keeps the strict
prerequisite-proven filter. `ALL` displays every catalog task that is not
excluded by the save snapshot or an exact runtime completion; an active
repeatable cycle remains visible. This changes only selection during the
existing fixed refreshes and adds no task query, SQL request, timer, scan, or
steady allocation.

The same Hub stores one independent `ASSAULT MODE` choice. `AVAILABLE` preserves
the normal Assault time, state, and 120-minute cooldown filters. `ALL` is a
presentation-only static catalog view: it displays all 40 Assault records even
when one is outside its active hours, defeated, cooling down, or save state is
not ready. Actual defeat/cooldown authority is not modified; switching back to
`AVAILABLE` immediately restores the live filters. Boss and area-quest
selection are unchanged. The mode reuses the existing fixed 49-entry selection
and open-only Hub service and adds no timer, SQL request, object scan, provider
query, or steady allocation.

A full-width accent divider separates these filtering modes from the RADAR/MAP
visibility rows above them. Each real choice is persisted and updates compact
selection immediately. Expanded-map changes made while the Hub is open are
coalesced into at most one atlas rebuild when the Hub closes; if the final
choices match the opening state, no rebuild runs. This avoids one large rebuild
per click without changing the final map state.

The compact pool keeps fixed Treasure and shared mini-game height groups and evaluates
Area Quest height inside the already bounded 80-slot marker pass. The nearest
Treasure and nearest visible Fly/Mole/Wave marker keep independent Z targets, while every
visible Area Quest may show its own height-band state without per-motion
allocation or UObject reads. Treasure keeps its unchanged six-piece shafted
pointer to the left of the selected chest, dark outline, and selected category
fill. Within any source band expanded by the
  inclusive +/-500 vertical-unit margin, an Area Quest keeps its normal black
  frame and displays three white dots. For the sole multi-band profile, authored
  marker Z first chooses the uniquely nearest existing source band. Below or
  above the selected band, those same pieces form a closed black triangle at the
  original task-marker center, pointing up or down respectively with no dots. It
  does not move beside the task or use Treasure guidance clearance. An exact-
  distance tie or missing profile keeps the normal frame with no dots and no
  false direction. Shape geometry changes only when its discrete state changes. The
nearest-mini-game channel instead uses one shaftless triangle centered directly
below the selected Fly, Mole, or Wave icon, colored from that marker's actual
  kind palette, and edged by a near-black contrast outline. The outline leaves
  its size, position, and projection unchanged. Target Z above the comparable player Z by more than 500 units shows
  an up triangle, target Z below by more than 500 shows a down triangle, and the
  inclusive +/-500 band hides the triangle. Treasure, Area Quest, and mini-game
  guidance all compare against `playerZ - 150`.

The Area Quest catalog keeps the original 147 MnMRadar marker coordinates and
adds ActorPositionData-derived height-band profiles. Exactly 144 rows have a
profile, one of those has two separated bands, and three have no source
  profile. Move_Check-only trigger bands are excluded when a real task-actor band
  exists. Authored marker Z only selects the uniquely nearest existing band for
  the multi-band row; it never replaces source height, and an exact-distance tie
  fails closed neutral. The shared `-150` comparison offset calibrates the
  player's root for every compact height channel and is not an Area Quest height
  source. The lower transparent clock remains a numeric world-time
display, not weather. Its four configured presentation bands start at 06:00,
12:00, 18:00, and 21:00 and use
sunrise, full-sun, sunset, and crescent-star glyphs. These names and thresholds
are presentation policy, not confirmed game-native phase semantics. The wider
clock group adds horizontal separation and sits six reference pixels lower.

The compact host reuses the existing one-hertz minimap-scale service to sample
the current viewport size and DPI. It changes UMG origin/render scale only on a
real geometry edge, so fullscreen, windowed, and DPI transitions no longer
  leave the radar and clock at stale off-screen coordinates. The layout consumes
  the game viewport rather than desktop monitor geometry. Deterministic unit
  tests cover numeric viewport/DPI inputs including 3440x1440, 3840x1600,
  2560x1080, ordinary windowed sizes, fullscreen-sized inputs, and DPI changes.
  Those numeric inputs do not prove a 3840x2160 game viewport with an internal
  21:9 content rect, native 21:9, windowed client geometry, or runtime Slate
  layout. The expanded map samples the live `PlayerIconWidget` alignment pivot
  at attach time. The player pivot and read-only native icon Canvas cached
  geometry are transformed through `LocalToAbsolute`, then through the game
  viewport geometry's `AbsoluteToLocal` to produce viewport-local host
  coordinates. `WorldMapUISize` is authored metadata, not the native Canvas or
  viewport extent.
  Initial attachment retains only numeric observations and uses its separate
  bounded three-attempt readiness service. A later attach, map-image edge, F7
  resume, or exact zoom event arms the five-deadline tail described above. Each
  tail observation re-resolves the witnessed native Canvas, player anchor,
  viewport geometry, and local width/height; no sampled UObject wrapper or
  `FGeometry` crosses passes. Each outer viewport host uses the accepted
  viewport-local atlas rectangle, and each `Panel_Point` Image remains local
  `{0,0,atlas_width,atlas_height}`. Later layout transitions update only the
  viewport-host transforms through the witnessed stable-geometry and bounded
  map/zoom strategy; they do not mutate or participate in the native Canvas.
  Missing, implausible, or final-unstable geometry fails closed;
  no authored `3000`/`8000` extent, centered fallback, desktop-resolution
  substitution, or new polling schedule is published. Real 21:9, internal
  black-bar, 16:10, and windowed behavior still require live gameplay evidence.

Bird eggs are intentionally absent from the expanded-map atlas. The UObject
creation listener accepts only the two exact egg classes and publishes weak
identities into a fixed 512-slot pool. The existing 250 ms control service
performs at most eight bounded candidate/position queries per tick. Availability
comes only from the Actor-owned `DInteractableComponent`: both
`InteractableValue` and `InteractTypeValue` must equal `2`. Both fields are
read through their actual one-byte scoped enum properties and validated integer
underlying properties. An unavailable
reflection result remains unknown and is retried by the same bounded service;
it is never guessed available or permanently discarded from one failed read.
A fixed nearest-16 active set shares the existing 250 ms discovery edge while
the compact category is enabled, and exact Bird Egg EndPlay retires that weak
identity immediately. The 400 ms unavailable debounce remains a bounded
fallback. No raw Actor is retained, and the feature adds no new polling,
global UObject enumeration, SQL query, dynamic queue, file polling, or
world-map work. F6 owns an independent `BIRD EGGS` RADAR toggle; its MAP cell is
unavailable.

R4 gameplay proved that the exact Boss and Assault death-event route removes
both marker types and applies their cooldowns without F8/F7. The remaining R4
failure was expanded-map composition after maximum-zoom close/reopen: the game
can add another native child later at the same maximum Z, after R4's successful
  one-shot restack. Historical R5 changed only that lifecycle edge. It added the
  exact `OnSliderValueChanged` event and its then-current bounded four-settle
  tail, with the native Canvas resolved again on every settle. It changed no coordinates,
atlas contents, marker selection, marker geometry, or state-decision logic, and
it adds no steady polling. The current installed R5 log then proved a separate
minimize/restore lifecycle: a replacement `DLayerMap` can be created with
`map_id` temporarily unavailable while the old layer remains attached. R6
returns `RetryLater` before same-layer payload validation on that ownership
mismatch and waits for the replacement layer's exact `SetWorldMapImage` edge.
R7 preserves the R6 renderer lifecycle and schedules, but repairs the numeric
handoff between an accepted encounter-death callback and the existing 250 ms
control service. The callback still publishes only one of 49 fixed bits after
all actor, catalog, lifecycle, availability, visibility, and proximity checks
pass. Ordinary consumption now waits for a valid runtime context and does not
recheck the time window that the accepted event already proved. Successful bits
are cleared individually; an apply exception retains the affected bit without
erasing any other pending bit for the next bounded service or authoritative
boundary. F7, disable, travel, and
activity-suppression boundaries may settle pending cooldown state numerically
without touching the renderer before reset. Only process shutdown and UObject
array shutdown discard the mask. This repair adds no poll, timer, scan, SQL
query, queue, or steady work. R7 also excludes only confirmed nonexistent
treasure save ID `11230106` at `(182813, 162051, 3150)`, the sole difference
between the 1,693 unique render IDs and 1,692 unique actor IDs, and hardens the
  release and installer boundaries. At that R7 checkpoint, source, native-build,
  installer-matrix, and release-package gates passed. Those older artifact
  results are historical; the then-current D4EE build boundary is stated above.
  Gameplay and performance remain `NOT_VALIDATED`, and publication remains
  `BLOCKED`.

R8 preserves every R7 renderer and lifecycle schedule. Boss and Assault gain a
second native receiver route through
`DsFieldCharacter.NetMulticastSetDeathProcess`; only the reflected `End` state
is accepted, and the receiver must pass all existing exact identity, class,
availability, visibility, and 100-metre checks before one fixed bit is
published. For area quests, an exact dynamic catalog event arms its ten-second
exact-ID witness without requiring a prior `PROGRESS` sample; the event itself
is never completion, and a blueprint end remains non-authoritative because it
can mean failure. Exact `End` completes the witnessed ID. If the witness expires
unresolved, its fixed catalog bit may enter the single below-normal worker for
an immediate exact-ID save confirmation and at most two 15-second retries, for
three attempts total. F7 first records an exact per-ID `COMPLETE_CNT` baseline:
an absent row means zero only after a valid single-owner query; otherwise that
ID remains unknown and queues no confirmation SQL. A confirmation is accepted
only when the same exact ID has strictly grown above its known baseline. Three
non-confirming attempts lock that task generation so repeated events cannot
restart SQL; only a new F7 or a proven `NONE`/`END` then later
`ACCEPTABLE`/`PROGRESS` generation unlocks it. Those requests skip treasure SQL.
There is no periodic SQL, dynamic queue, save-file poll, or new steady work.
Exact-artifact 2.1.0 gameplay and performance acceptance remain
`NOT_VALIDATED`.

R4 uses the optional
`/Script/DS.DsFieldCharacter:NetMulticastNotifyDeath` pre-hook as the primary
Boss/Assault completion signal. It accepts only an exact already-observed weak
receiver whose class exactly matches the catalog, is a `DsMonsterCharacter`,
belongs to the current activation and epoch, is currently available, was
visibly seen, and is no more than 100 metres from the current player. The
callback publishes only one bit in a fixed 49-bit atomic mask. Ordinary
consumption by the existing 250 ms control service requires a valid runtime
context but does not recheck the accepted event's time window. Each successful
bit is cleared independently after the 120-minute cooldown and render-state
application; an apply exception retains that bit. F7, disable, travel, and
activity-suppression boundaries can settle pending numeric cooldown state
without renderer mutation before reset, while only process or UObject-array
shutdown clears unconsumed bits. Missing or failed optional-hook registration does not
disable the rest of the radar. The dev65 forty-sample/ten-second disappearance
gate remains the fail-closed fallback, and a far pooled relocation cannot
overwrite the last trusted in-range encounter position. R3 moribund polling is
not used: the target `MonsterCharacterData` rows set `UseMoribund=0`, and the
diagnostic logs recorded no moribund hit. No Actor or other UObject is retained
by either route. R4 also restacked the expanded-map hosts once after map-image
post events or F7 resume; R5 retains those triggers and adds only the bounded
zoom-event settle tail described above. R6 adds only the exact replacement-layer
ownership and one-shot rearm rules described above; it adds no focus hook,
poll, recurring timer, enumeration, or steady work. The shipped default
override no longer suppresses treasure `11003`; exact class and 3D identity
handle that overlap. It suppresses only confirmed nonexistent save ID
`11230106`; static validation fails if that singleton catalog difference or its
exact identity changes.

## Install

1. Exit the game. Never install or update the mod while the game process is
   running.
2. The one-click installer accepts a structurally complete ExperimentalNested
   UE4SS layout without comparing its loader or proxy DLL to a version hash.
   If UE4SS is absent, Setup offers to install the embedded, integrity-verified
   Experimental build. A root, dual, malformed, or incomplete layout requires
   explicit confirmation before backed-up conversion.
3. For the recommended transactional installation, download and fully extract
   `DragonSwordNativeWorldRadarPostRender-v2.2.1-Installer.zip` after the
   candidate package has passed its release gates.
4. Verify the SHA-256 sidecar, then run
   `DragonSwordNativeWorldRadarPostRender-Setup-2.2.1.exe`. The Setup executable
   is unsigned and requests administrator access so it can apply one bounded
   transaction; it never launches or terminates the game.
5. Select the exact `DSClient-Win64-Shipping.exe` when prompted. Setup performs
   a read-only preflight, then shows the detected UE4SS layout, destination,
   and whether bootstrap or conversion is required. Review and confirm that
   plan before installation.
6. After confirmation, Setup revalidates the game executable, UE4SS state,
   embedded payload, and active load-control paths. Conversion first creates a
   complete, SHA-256-verified backup whose contents begin at the original
   `Win64` level, including the active UE4SS tree, Mods,
   settings, configuration, load-control files, logs, mapping, and header dump.
   Only after that snapshot succeeds does Setup migrate Mods and `mods.txt`,
   remove the old active layout, and install the pinned Experimental loader.
   The old loader, Mods, and configuration then remain only in the backup.
   Setup also uses short-name file-level transaction copies for automatic
   rollback without extending user Mod paths. Normal installation and
   Update / Repair remove that temporary journal after a successful commit;
   only a UE4SS conversion retains the complete original-layout backup.
7. When Setup reports success, select OK. Setup stays open and refreshes the
   detected state: the main action becomes `Repair`, and `Uninstall` is enabled
   only after strict ownership of the active installation is proven.
8. Start the game in an open-world area and press F7 once.

The 2.2.1 output contract also defines two manual ExperimentalNested channels:

- `DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-No-UE4SS.zip` contains
  only the Mod payload and one clean single-product `mods.txt` for an already
  compatible ExperimentalNested runtime.
- `DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` contains
  the same Mod payload plus the pinned ExperimentalNested runtime for a direct
  closed-game installation.

Both manual archives are script-free for Nexus compatibility and their roots
map directly to `DS/Binaries/Win64`. For No-UE4SS, copy only the Radar Mod
folder; copy its one-line `ue4ss/Mods/mods.txt` only when the target file is
missing, otherwise merge the exact Radar line without replacing unrelated
entries. With-UE4SS is only for a clean target with no UE4SS and can be
extracted directly into `Win64`. Use Setup for every existing Radar update so
the three user-owned configuration files are preserved.

Neither manual archive supports StableRoot. Manual copying does not detect or
convert another UE4SS version, merge load control, preserve settings
automatically, repair an existing Radar installation, or provide transactional
rollback. Use Setup whenever the existing layout or same-product ownership
needs discovery, conversion, update, or repair.

Before any backup or mutation, Setup rejects an active external renderer expressed as
`DragonSwordWorldRadar : 1` or as the exact path
`DragonSwordWorldRadar/enabled.txt` in any approved Mods root. A disabled
`DragonSwordWorldRadar : 0` entry is preserved byte-for-byte. Setup never
deletes or disables that external renderer; the user must resolve an active
conflict explicitly. See
[Installation](docs/INSTALL.md) for exact compatibility and recovery details.

## Update

1. Exit the game.
2. Fully extract the new binary archive and run its Setup executable again. The
   installer automatically selects Update / Repair for a recognized existing
   Radar in a structurally complete ExperimentalNested layout. It refreshes the
   bundled DLL and generated catalogs while preserving all three user-owned
   files: `config/visibility.ini`, `config/diagnostics.ini`, and
   `data/defaults/treasure_overrides.txt`. No persistent backup is retained for
   a successful normal update.
3. A recognized same-product legacy `enabled.txt` is backed up and removed during migration,
   and every valid `DragonSwordWorldRadarObjectState` line is removed from
   `mods.txt`; the final installed product is controlled only by its one
   normalized native entry. The distinct `DragonSwordWorldRadar` external
   renderer is never mutated: active load control rejects the upgrade with zero
   mutation, while an exact disabled `: 0` entry is preserved. Unknown
   same-name ownership fails closed.

Before recursively replacing an existing target, Setup requires matching
top-level schema-5 release metadata and schema-1 package metadata, 2-256 unique
 manifest entries, exact installed size/hash for every immutable owned file,
 strict syntax and size validation for the mutable treasure override, proof of both
`metadata/release.json` and the version/label-bearing `dlls/main.dll`, and an
exact recursive tree containing only manifest files plus the bounded live-state
allowlist. Ownership JSON is limited to 4 MiB and depth 16; each manifest entry
and all manifest-owned bytes together are limited to 256 MiB. Legacy marker and
example files are limited to 64 KiB each, each log to 2 MiB, each atlas cache to
16 MiB, and an install record to 1-64 KiB with matching version and runtime
label. Any unknown path, mismatch, duplicate, overflow, reparse point, or bound
violation rejects the upgrade before backup or mutation.

Do not delete or replace `DS/Saved`; radar updates do not require save-file
cleanup.

## Uninstall

Exit the game, run Setup, select `DSClient-Win64-Shipping.exe`, and click
`Uninstall`. The button remains disabled unless Setup first proves exact
same-product ownership in the active compatible layout, then displays
a confirmation dialog. The confirmed transaction removes only the exact Radar
directory and at most one valid Radar entry from the controlling `mods.txt`.
It preserves `mods.txt` encoding and line endings, UE4SS, unrelated Mods, and
`DS/Saved`; any failure rolls back all recorded mutations. Successful removal
does not retain a backup. If ownership cannot be proven, Setup stops without
changing files. See [Installation](docs/INSTALL.md) for the manual fallback.

## Controls and settings

- F7 activates the radar or performs the documented bounded explicit recovery.
- F8 disables radar features. F8 followed by F7 starts a fresh state
  reconciliation.
- F6 opens the native responsive settings page in Off, On, or Fault state.
  Bug Report and Close occupy the top bar. Read-only status text uses a thin
  state-colored strip; a separate Enable, Disable, or Retry action
  keeps the page open, while Bug Report opens the fixed Nexus Posts page.
  Language, compact `RADAR`,
  expanded `MAP`, the three compact-only height indicators, and filter modes
  persist in `config/visibility.ini`. The current file uses readable
  `[radar]`, `[map]`, `[modes]`, `[height_arrows]`, and `[interface]` sections with
  named `true|false` category keys and
  `available|all` mode values. It is bounded to 4 KiB, read once at startup,
  and never hot-polled. Strictly valid legacy schema 1-4 mask files remain
  accepted; the next real F6 change rewrites them atomically in the readable
  current format. All three height indicators default ON for a clean or safely
  migrated configuration; a valid existing file keeps the user's choices. Bird
  eggs have an independent RADAR toggle and no
  MAP toggle. `AREA QUEST MODE` defaults to `AVAILABLE`; selecting `ALL`
  displays every unfinished catalog task even when its prerequisite graph
  cannot yet be proven. Completed tasks remain hidden in both modes. `ASSAULT
  MODE` defaults to `AVAILABLE`; selecting `ALL` shows all 40 static Assault
  records regardless of active hours, defeat, cooldown, or save readiness.
  Switching back to `AVAILABLE` restores those live filters. Boss and area-
  quest rules do not change. The selector contains only the 11 explicit
  languages. A legacy AUTO value resolves once on the next actual F6 opening or
  F7 activation from `DGameUserSettings.LanguageText`, then Kismet and English,
  and migrates to the matching explicit persisted language. Close the page with
  `X` or F6.

## Compatibility, logs, and source builds

This is an ABI-specific UE4SS native mod with a game-update-tolerant executable
boundary. Setup accepts the exact selected DragonSword executable path only
after bounded AMD64 PE32+ executable-image validation; it does not reject a
game update merely because the executable SHA-256 changed. The observed hash
is retained only in the confirmed transaction identity and install record.
 The one-click installer uses the structural ExperimentalNested contract recorded in
 `metadata/installer-product-profile.json`. Existing loader and proxy DLL hashes
 are provenance only and never a compatibility allowlist. Setup bootstraps its
 embedded integrity-verified runtime when UE4SS is absent. When another,
 incomplete, malformed, conflicting-root, or dual
layout exists, Setup requires a separate conversion confirmation, backs up the
loader state, migrates the existing Mods tree and load-control state, and
installs the pinned Experimental runtime. Conflicting duplicate Mod files stop
the operation before mutation. Runtime reflected ABI validation remains an
 additional fail-closed feature boundary; Setup does not claim ABI identity from
 a DLL hash.
The packaged save-owner RVA remains the fast path. If a game update invalidates
it, the existing below-normal save worker performs at most one process-lifetime
scan of executable PE sections through `min(SizeOfRawData, VirtualSize)`, counts
only pattern targets inside the mapped image, and accepts exactly one target.
Packaged and structural routes share at most 24 active-`.db` key validations;
only an authenticated result may be cached. It adds no
watcher, update prompt, periodic scan, or steady-state work. Static catalog
 changes still require a separate data release; Update / Repair reinstalls the
 release's bundled tables, while F7 cannot regenerate them.
Release diagnostics are disabled by default. To
capture a test session, exit the game, create or edit `config/diagnostics.ini`
and set `debug_logging=true` in its `[diagnostics]` section, then restart the
game. The commented file uses the same small INI style as Auto Pickup: at most
2,048 bytes, one `[diagnostics]` section, and exactly one
`debug_logging=true|false` key. It is read once at native startup and is never
hot reloaded. Missing, malformed, duplicate, unknown, or oversized content
fails closed to disabled. Set
`debug_logging=false` and restart to disable diagnostics again. The exact
legacy one-line `event_log_enabled=true|false` form remains accepted when an
older installed user file is preserved during Update / Repair.

The 2.0.0 public package keeps the immutable diagnostic default disabled. A
gameplay-test installation may be explicitly enabled through its live
`config/diagnostics.ini`; that installed-copy-only choice must not be copied
back into the release archive.

The immutable `visibility.example.ini` and `diagnostics.example.ini` resources
are embedded inside Setup and copied into both manual payloads as live
clean-install defaults; duplicate example files are not installed. Setup or a
manual install creates the live `visibility.ini`, `diagnostics.ini`, and
`data/defaults/treasure_overrides.txt` files. Every public channel defaults
`debug_logging=false`.

When enabled, the log is
`runtime/logs/DragonSwordNativeWorldRadarPostRender.Native.log`; one bounded
previous file is kept beside it. When disabled, the event path returns before
message formatting, locking, directory creation, rotation, or file I/O.
Enabled sessions start with one `SESSION_BEGIN` record naming log schema 2.
Every record includes exact `seq`, `utc_ms`, and `elapsed_ms` fields, making
area-quest completion and world-map geometry events easy
to correlate with a screenshot or another Mod log without adding another
timer, polling pass, or per-frame event.
Enabled events enter a fixed 256-record queue with capture-time metadata. The
game thread uses a non-blocking `try_lock`; contention or a full queue increments
`logger_dropped` instead of waiting. A below-normal-priority writer formats the
numeric Tick records and performs every file write. It drains at most 16 records
per batch and flushes an idle partial batch after 250 ms; critical lifecycle and
fault records request an immediate worker-side flush. The current file is capped
at 1 MiB and rotates to exactly one previous file, so diagnostics cannot
accumulate an unbounded history. `ENGINE_TICK_SLOW` and
`ENGINE_TICK_PROFILE` are fixed numeric records on the game thread; their text
formatting and `logger_dropped` / `logger_truncated` expansion occur only in the
writer.

The native log is local and is never transmitted by the mod, but it can contain
player/target coordinates, world and class names, catalog IDs, task IDs, and
lifecycle timing. Review and redact it before posting it publicly. Logs are not
included in any release archive.

The installed mod contains no external runtime executable, Lua renderer,
transparent window, runtime catalog generator, retired canary source, or
PostRender/Present configuration. The final output directory contains the
installer archive, the two ExperimentalNested manual archives,
`release-manifest.json`, and `SHA256SUMS.txt`. The No-UE4SS archive excludes the
loader; the With-UE4SS archive embeds only the pinned ExperimentalNested
runtime. No source archive belongs to the final binary allowlist. If a separate
source archive is generated for builders, it is a project-source snapshot, not
a complete dependency or Corresponding Source bundle: it excludes the pinned
SDK, RE-UE4SS, `UEPseudo`, FetchContent checkouts, and third-party toolchains.
The project code is offered under `GPL-3.0-only`; see [`LICENSE`](LICENSE) and
the repository [`LICENSE_SCOPE.md`](../LICENSE_SCOPE.md). That license does not
grant rights to third-party or generated Unreal material.

Public binary publication is currently blocked pending three reviews: the
applicable Unreal/`UEPseudo` authorization and compatibility terms; exact
source/build provenance for the bundled `e_sqlcipher.dll`; and redistribution,
attribution, and provenance for the PAK-derived catalogs and MnMRadar-derived
coordinate table. License files and notices in the binary archive are necessary
records, but they do not resolve those questions. See [Dependency Sources and Release-Clearance Boundary](docs/DEPENDENCY_SOURCES.md)
before publishing any archive.
The historical source, build, package, installer, deployment, gameplay, and
publication states for 2.1.0 remain recorded independently in
[Release](docs/RELEASE.md) and [Acceptance Checklist](docs/ACCEPTANCE_CHECKLIST.md).
The public-report closeout and its unvalidated reporter-environment boundaries
are recorded in [Nexus Feedback Audit for 2.0.0](docs/NEXUS_FEEDBACK_AUDIT_2_0_0.md).
Post-2.0.0 corrective feedback and the owner matrix are recorded
in [Runtime Feedback Audit for 2.1.0](docs/RUNTIME_FEEDBACK_AUDIT_2_1_0.md).
The current 2.2.1 fixes-only scope and acceptance boundary are recorded in
[Runtime Feedback Audit for 2.2.1](docs/RUNTIME_FEEDBACK_AUDIT_2_2_1.md).
The [2.2.0 audit](docs/RUNTIME_FEEDBACK_AUDIT_2_2_0.md) remains historical
feature-release evidence.
The controller-menu and Area Quest height-arrow patch boundary remains
preserved in
[Runtime Feedback Audit for 2.1.1](docs/RUNTIME_FEEDBACK_AUDIT_2_1_1.md); it is
historical evidence and is not relabelled as 2.2.1.
The related [DragonSwordWorldRadar](https://github.com/xiaofei-dev/DragonSwordWorldRadar)
repository is now the control workspace for all maintained DragonSword Mods,
not only the historical external Radar. New public commits use a source-only
boundary: the local SQLCipher binary and generated/derived game catalogs remain
excluded pending provenance and redistribution review.

## Development history

The sections below preserve the implementation history that produced 2.0.0.
They are historical evidence and do not replace the current release status,
artifact manifest, or acceptance checklist above.

## Dev67 changes

Dev67 applies four repairs derived from the latest gameplay log and preserves
the native-only steady-state design:

- Exact area-quest task-actor and quest-event evidence now arms one fixed
  ten-second per-task verification window. The game is queried for that exact
  catalog ID at 750 ms intervals, with at most one exact query across all
  witnesses per engine tick. Final `END` proves completion even if the previous
  published state was `NONE`, `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` alone
  is not completion. Duplicate events neither extend the deadline nor add a
  second fixed-queue entry. After a completion, `NONE` or `END` remains only the
  inactive boundary before a later active sample may rearm a repeatable task.
- Treasure completion uses the exact
  `DsAnimationProp.NetMultiExecuteInteractProp` receiver Actor and accepts the
  event when its Actor parameter is the freshly resolved local Pawn. Version
  2.0.0 also accepts the exact callback-local `Rider` UObject owned by that
  Pawn when the Pawn is a water mount, the receiver is one of the mount-only
  treasure classes, Rider and Pawn share a non-null World, and the exact
  receiver remains within eight metres. The receiver's reported `ObjectID` is
  validated against exact class and 3D position before activation-matched weak
  identity or bounded unique-class 3D fallback. `SetDeathProcess` remains an
  exact-receiver fallback. Any other rejected non-Pawn event may queue only the
  exact receiver's immutable save ID when the receiver is within eight metres
  of the fresh local player. The existing below-normal worker checks only those
  exact treasure categories after 15 seconds and, if still negative, once more
  285 seconds later. Positive save bits alone hide markers; fixed arrays cap
  one request at 64 IDs, retain no UObject, and add no polling or idle SQL.
- A runtime state delta rebuilds the expanded atlas immediately only when the
  exact retained map layer is still visibly open. An attached but hidden layer
  is detached and the rebuild waits for the next real `SetWorldMapImage` edge.
  That edge may rearm the three-attempt readiness budget once for the matching
  candidate serial, including when the first three `map_id` probes exhausted
  before the renderer could publish a retryable failure. Repeated same-layer
  events cannot replenish it after that one rearm.
- A replacement `DLayerMap` created during minimize/restore cannot detach or
  restack an attached renderer until exact full weak ownership matches. Zoom
  and same-layer `SetWorldMapImage` paths return `RetryLater` on mismatch before
  validating the old payload. The exact replacement `SetWorldMapImage` may
  consume the existing one-shot rearm while the renderer is `Ready` or still
  `Attached` to a different old layer; the same bounded three-attempt service
  remains authoritative.
- Boss/Assault candidate discovery now resolves the current actor position
  before applying the 100-metre player bound. The existing 250 ms service uses
  the fixed 49 weak slots round-robin and performs at most eight actor-position
  queries per control tick. It performs no `FindAllOf`, dynamic queue, or work
  on the 16 ms motion path.

The release archive defaults diagnostics to disabled, while the installed
dev67 test copy enables them for runtime evidence. The owner reported completed
gameplay testing and acceptance on 2026-08-31; an independently retained exact
artifact hash receipt and external frame-time capture remain separate.

## Dev66 changes

Dev66 fixes the runtime-proven clustered-treasure loss without adding recurring
work. `Server_RunInteractV2` now resolves the exact receiver component owner
before the compatibility target fields. The exact weak Actor observation is
reused when available; otherwise exact-class catalog correlation ranks bounded
candidates by 3D distance so adjacent or XY-overlapping chests keep separate
save IDs. The catalog-only nearby fallback remains fail closed if no Actor can
be recovered. All new work occurs only on a real interaction event: there is no
new timer, poll, object enumeration, SQL query, queue, or motion/render work.

The dev64 diagnostic session proves the prior implementation missed two of nine
opens in one cluster sequence and two of three opens in another, while every
successfully captured open refreshed immediately. That evidence establishes the
defect and repair target; it does not accept dev66. A fresh clustered gameplay
test remains required.

## Dev65 changes

Dev65 fixes the encounter false-positive reproduced by repeatedly entering and
leaving a large Assault without defeating it:

- Boss and Assault observation now binds the actor's current position instead
  of the catalog coordinate. The conservative disappearance fallback arms only
  after the same actor was present for at least four 250 ms samples spanning at
  least one second, then requires forty consecutive missing samples spanning at
  least ten seconds while the current player remains within 100 metres of that
  last actor position in a stable open-world, cursor-hidden context.
- `RemovedFromWorld`, travel, menus, activity suppression, leaving the
  100-metre bound, and actor reappearance all reject or reset the encounter
  completion candidate. An exact current `Destroyed` event remains the fast
  path. Returning to an encounter can bind the same still-live weak actor again,
  and an encounter already hidden by a future cooldown cannot start redundant
  observation work. The final state write independently rechecks the current
  time condition and cooldown, and a rejected race clears the processed actor
  identity so it can rebind without F8/F7. Confirmed activity entry also clears
  armed encounter evidence and processed identities immediately, independent
  of whether the current Pawn position sample already failed.
- Duplicate treasure interaction evidence now returns before logging or
  presentation invalidation, removing repeated no-op event traffic.
- Native event diagnostics use one bounded startup-only configuration. Release
  packages default to `debug_logging=false`; disabled sessions perform no
  formatting, locking, directory, rotation, or file work. Development
  deployment can explicitly enable the same local file for gameplay evidence.
- Dev65 introduced a loose `Install.cmd` binary-package installer. The 2.0.0
  public package replaces that entry point with one embedded, hash-verifying
  C# Setup executable while retaining Steam discovery, three stopped-game
  guards, exact target and manifest validation, full backup/rollback,
  encoding-preserving `mods.txt` updates, and independent preservation of both
  user configuration files.

Dev65 source, native-core, static, build, and package results remain separate
from gameplay acceptance. The encounter fix is accepted only after the runtime
checks below prove both non-kill rejection and real-kill removal.

## Dev64 changes

Dev64 removes the two failure and hitch mechanisms found in the latest
historical dev60 logs without adding steady-state work:

- The UObject creation listener now recognizes an exact `DLayerMiniMap`, stores
  only its weak index/serial identity, and publishes it through one fixed
  mailbox. A newly created valid layer can rearm one bounded compact attachment
  transaction after dungeon return. The renderer receives that exact candidate
  instead of depending on the old three-attempt timing window. A distinct
  replacement identity rearms once even if the old renderer still reports
  attached or menu-suppressed; duplicate identity delivery never replenishes
  work. Travel defers the single weak mailbox until the new GameMode world can
  validate or reject it, and confirmed non-open-world suppression does not
  attach it.
- Boss and Assault discovery no longer calls `FindAllOf`. The same creation
  listener stores at most one weak identity in each of 49 fixed catalog slots.
  The existing 250 ms discovery service consumes only slots whose catalog
  position is within the 100-metre observation radius. Travel clears the slots;
  F8/F7 preserves valid same-world candidates. There is no dynamic queue,
  catalog-class scan, or encounter work on the 16 ms motion path.
- Strict exact-class `Destroyed` EndPlay recovery now applies to both Boss and
  Assault when the actor was not observed earlier. It remains bounded by the
  existing unique-class and 100-metre checks and uses the same disappearance
  confirmation before applying the two-hour cooldown.
- Required catalogs, layer classes, fixed encounter-name keys, UObject-create
  listener, and all five native lifecycle callbacks now share one immutable
  readiness latch. Initialization failure, F7, travel, and the one bounded
  fault recovery all remain disabled behind the same fail-closed gate.
- Every SQLCipher row callback is exception-contained and row-bounded: 4,096
  treasure rows and 65,536 encounter or dynamic-completion rows. Malformed,
  excessive, or allocation-failing results abort that one reconciliation
  attempt fail closed; they cannot unwind through the C callback boundary.
- The native logger now checks stream state after open, write, and flush. A
  failed stream disables logging only, instead of retaining a permanently bad
  stream and repeating failed writes.
- Production `main.dll` is native UMG only. It compiles and connects no
  PostRender/Present canary, external diagnostic shared mapping, executable,
  motion bridge, or Lua renderer. Dormant historical canary source and its
  disabled configuration remain source-only audited development records; they
  are not read by the production owner or included in the binary payload.

Historical dev60 evidence reproduced the compact `state=5` dungeon-return
failure and 22-32 ms encounter class-enumeration spikes that motivated dev64.
Later dev64 gameplay logs exposed the separate encounter-disappearance false
positive addressed by dev65. Historical evidence does not establish the current
installed artifact identity. The owner reported completed gameplay testing and
acceptance for the current version on 2026-08-31. Fine-grained checklist
evidence and exact installed hashes remain independently recorded states.

## Dev63 changes

Dev63 adds bounded recovery without adding a watchdog, timer, polling loop, or
steady retry path:

- An SEH fault in the engine-tick callback disables the radar immediately. The
  following tick performs guarded cleanup and may make one automatic recovery
  attempt for the entire process only when the radar was active. That budget is
  spent before cleanup; a second fault, a fault while inactive, or a failed
  recovery stops and is logged instead of looping.
- A world-map renderer that faulted only at runtime may return to `Ready` on a
  later explicit activation after guarded detach clears its weak handles, no new
  fault occurs in that call, and the reflected ABI remains valid. F7 treats this
  state and a faulted area-quest scanner as bounded activation work. Same-call
  faults and ABI failures remain terminal.
- A runtime-only visibility-Hub fault may recover only on a later explicit F6
  open after the same clean-detach and valid-ABI checks. The closed Hub still
  performs no UObject or file work.
- Native diagnostics now use a fixed 256-record non-blocking queue and one
  below-normal-priority process-session writer. The game thread performs no
  diagnostic file I/O; queue contention or saturation drops and counts the
  record. The writer drains at most 16 records per batch, flushes idle partial
  batches after 250 ms, and immediately flushes lifecycle/fault records after
  dequeuing them. The current file is capped at 1 MiB and rotates to exactly one
  previous file. Numeric Tick telemetry is formatted only by the writer; this
  reduces observer interference but is not a claim that logging is free.
- The compact/world-map UObject-create mailboxes use an atomic no-event fast
  path. Their mutexes are entered only while copying or clearing a published
  weak candidate; the game tick does not lock either mailbox when idle.
- The release stage now creates separate binary and source archives in `dist`,
  verifies both after extraction, and records exact file hashes. The binary
  package has an exact runtime allowlist and package manifest; the source
  package has `SOURCE_MANIFEST.sha256` and rejects scratch artifacts.

Dev63 has historical source/static/package evidence only. Its gameplay,
stability, and performance acceptance was never completed before dev64.

## Dev62 changes

Dev62 fixes dungeon-return lifecycle for the compact renderer. A confirmed
non-open-world edge detaches the compact host and clears world-local weak
handles; the open-world return edge rearms a fresh attachment through the
current Controller. Its recovery allowance applies only to a prior runtime-only
compact-renderer fault after clean detach. Same-call faults and ABI failures
remain terminal, and ordinary menu/cursor suppression remains a collapse rather
than a detach.

## Dev61 changes

Dev61 shortens the F6 column headings to `RADAR` and `MAP` and assigns explicit
role-specific text multipliers over the shared viewport/DPI scale. The title,
column labels, row labels, unavailable marker, and X therefore remain inside
their slots even when the game's default UMG font is unusually large.

## Dev60 changes

Dev60 fixes the high-resolution Hub defect where Canvas slots followed the
responsive panel scale but TextBlocks followed the game's UMG DPI independently.
Every Hub text widget now applies the same open-time viewport/DPI scale as its
geometry from a top-left pivot. The title bar and minimap/world subheader are
separate, rows are compacted, and linear-space colors are retuned darker.

Both reflected transform functions are ABI-gated. The additional calls occur
only while constructing an explicitly opened F6 panel; the 50 ms service and
closed path are unchanged.

## Dev59 changes

Dev59 refines the transient F6 Hub into a framed layered card with distinct
minimap/world header chips, a darker content surface, consistent alternating
rows, clearer toggle frames, and a more recognizable close control. It keeps
the same widgets, change-only auto-apply behavior, and open-only service cost.

The panel remains centered and resolution independent. Its 620-by-482 reference
layout scales by the smaller of the viewport width/2560 and height/1440, then
converts through the live UMG DPI scale. Wide displays therefore use height as
the constraint, narrower displays use width, and the panel never stretches.

## Dev58 changes

Dev58 places both expanded-map radar atlas hosts above game-native map icons at
adjacent deterministic Z orders. Radar-internal ordering remains unchanged:
treasure is last inside the lower task/mini-game atlas, and Boss/Assault use the
upper atlas. Map zoom can no longer expose a category behind native icons.

The compact host now collapses on the first failed current-Pawn position sample
during scene handoff. The already reacquired current Controller supplies cursor
state before Pawn resolution, so this uses the existing 16 ms scalar path and
adds no timer, lookup loop, global enumeration, or retained UObject.

## Dev57 changes

Dev57 redesigns the transient F6 Hub as a smaller, denser settings panel with
a distinct header, aligned minimap/world columns, alternating row bands, and a
single top-right `X`. Apply and Cancel are removed. A changed checkbox publishes
the compact and expanded-map selections immediately and atomically replaces
`config/visibility.ini` once in the named section format; unchanged 50 ms
service samples perform no write or renderer rebuild.

Input setup now calls `SetInputMode_GameAndUIEx` before explicitly showing the
cursor. While the Hub is open, the existing 50 ms service reads the cursor bit
and reasserts Game-and-UI input only when the game has overwritten it to hidden.
There is still no closed-state Controller lookup, UObject work, polling, or file
I/O. `X`, a second F6, F8, travel, and shutdown restore the previous cursor and
GameOnly input state through the panel's owning Controller.

## Dev56 changes

Dev56 adds a single native `ETSendQuestEventTrigger` hook for dynamic quest
events. The exact event task ID receives a ten-second completion witness only
when one exact live query proves `PROGRESS`; the last published `PROGRESS` is a
fallback only if that query is unavailable. Each task owns its own deadline and
single follow-up budget, repeated step events cannot replenish either, and the
generic treasure/workstation interaction path never arms area tasks. This lets
the observed cooking/delivery transaction hide the exact task when it reaches
`END` without accepting unwitnessed terminal states globally. No
Blueprint-script hook, 147-function hook set, idle task poll, SQL retry, or
UObject retention is added. The fixed numeric witness survives travel only
until its original deadline; a new F8/F7 activation clears it.

The loaded `MONSTER_ALIVE` rows legitimately use `value1=0`. Dev56 links such a
condition only when exactly one Assault exists within 150 metres of that task;
zero or multiple candidates remain fail closed. This restores global evaluation
of the time-gated Assault companion task without requiring the player to first
approach its trigger. The first valid game-hour sample after F7 also coalesces
one transactional task refresh, so activating inside the time window does not
wait for the next hour edge.

F6 opens a transient native UMG visibility Hub. Compact and expanded-map
columns independently control clock, treasure, Boss, Assault, mini-games, and
area quests; expanded-map clock is intentionally unavailable. Each changed
selection or display mode publishes both category selections and both persisted
modes in one game-thread transaction and writes the readable sectioned
`config/visibility.ini` once.
The Assault `ALL` mode is a presentation-only static catalog view; `AVAILABLE`
retains encounter-state, time-window, and cooldown checks. A closed Hub does no UObject work,
deployment preserves an existing user file, and a world-map selection can
rebuild an already-open expanded-map session through its existing bounded gate.
Panel cleanup restores input and cursor state through the panel's own owning
Controller, not a potentially replaced current Controller.

Compact and expanded-map UMG attachment now permit at most three delayed
soft-not-ready attempts in the current activation/session. There is no sleep or
steady retry loop; success or the third failure stops the transaction. Duplicate
callbacks for the same live map layer cannot replenish either a terminal
one-attempt result or an exhausted budget.

## Dev55 changes

Dev55 changes only compact area-task contrast. The existing rounded diamond now
uses a 55-percent translucent charcoal fill, its thick dark frame is unchanged,
and its three existing dots return to solid white. This prevents the player
arrow or bright map terrain from visually replacing the task symbol while still
letting the map show through. The same four preallocated pieces, 28-pixel
reference size, attachment, layering, and update cadence remain unchanged.

## Dev54 changes

Dev54 replaces ID-less completion inference with a direct reflected identity
map. After each F7 activation or completed travel, the game thread reads the
current game instance's `TaskActorClassContainer.DynamicQuestTaskList` once.
Only rows with the dynamic-quest task-use type are considered. Each row's
`CreateTaskClass` full name is joined through its single unambiguous
`UseQuestList` ID to the validated 147-entry area-quest catalog. Capture is
bounded to one post-stability attempt plus one delayed retry after failure;
success or the second failure stops all further work. Confirmed non-open-world
activities defer any pending capture until open-world rendering resumes. For
task-class mapping, an explicit F7 while the mod is already active may rearm
only a failed capture; that mapping action preserves exact-completion bits,
revisions, and repeatable-task fences. A fresh F8/F7 activation remains the
explicit full-state resynchronization path.

The map is accepted only when all 147 catalog IDs have exactly one class
binding, no class resolves to multiple quest IDs, and no catalog ID is covered
twice. Only class-name strings and catalog indices survive capture; the game
instance, container, reflected rows, task classes, and task actors are never
retained. Any missing, ambiguous, duplicate, or unmapped identity fails closed.

`ADETTaskBaseActor.OnRecvCompleteQuest` supplies the current task actor as its
callback context. The post-hook reads that actor's exact class full name only
inside the callback and publishes a fixed atomic catalog bit. The next game
tick consumes the bit, immediately hides that exact task from the compact map,
invalidates any retained expanded atlas, and makes
its ID available as activation-local `DYNAMIC_QUEST_COMPLETE`
prerequisite evidence. Every real completion also schedules the existing
one-second-debounced transactional state refresh for settled state. A per-task
completion revision and fixed reactivation latch prevent both an older
in-flight scan and a later still-active `ACCEPTABLE`/`PROGRESS` sample from
reviving the completed task. A current-revision scan must first observe a
proven inactive `NONE` or `END` boundary; only a later current-revision scan may
clear the completion when the genuinely repeatable task reports
`ACCEPTABLE` or `PROGRESS` again. `FAIL` never arms reactivation. Unmapped
callbacks receive only the generic refresh and cannot hide a guessed task.

The asynchronous one-shot F7 save result is merge-only for positive dynamic
quest completions. Applying an older snapshot never clears completion IDs that
arrived later from the exact callback or the generic runtime transition. The
activation lifecycle remains the only owner of full completion-state clearing;
this ordering fix adds no save retry or polling path. If that delayed result
arrives after an expanded atlas was already attached, the stale atlas is retired
immediately. A visibly open exact attachment may rebuild once in the current
session; a hidden retained layer defers until its next exact
`SetWorldMapImage` edge.

Boss/Assault cooldowns now use the same process-lifetime precedence rule as
native treasure deltas. F7 no longer clears the bounded 49-entry effective
cooldown map, and the save snapshot merges timestamps by maximum value. A fast
F8/F7 therefore cannot revive a just-defeated encounter before its save write.
One scalar is checked by the existing 250 ms control service and is rate-limited
to 1 Hz. Only the earliest cooldown expiry or a displayed world-hour change
recomputes the fixed 49-bit encounter visibility mask. A real visibility edge
performs one bounded rebuild when the exact attachment is visibly open, or
retires a hidden retained atlas until its next exact `SetWorldMapImage` edge;
there is no 49-entry scan on the 16 ms motion path.

Dev54 preserved dev53's removal of nearby treasure-class `FindAllOf` catch-up.
Treasure state is owned by the interaction hook plus BeginPlay/EndPlay
disappearance fallback, so a nearby catalog coordinate no longer causes a
28-38 ms game-thread class enumeration. Dev64 removes the remaining Boss and
Assault class enumeration and replaces it with the fixed 49-slot weak creation
cache described above.

## Runtime schedules

- Player/root translation: 16 ms while compact rendering is active.
- Activity and discovery service: 250 ms.
- Compact catalog rebind: on a real state/radius change, 1,000 Unreal units of
  movement, or five seconds.
- Game clock: one guarded numeric baseline two seconds after F7, followed by
  local 60x `steady_clock` extrapolation.
- Area quests: one reflected query per game frame only during F7, completed
  travel, a coalesced quest-state refresh, or a displayed game-hour edge. The
  task-class identity table uses at most two bounded attempts per F7/travel and
  stops after success or the second failure; exact mapped completion hides
  immediately and does not wait for the follow-up scan. Repeatable reactivation
  is a two-scan settled-state proof: current `NONE`/`END`, then later current
  `ACCEPTABLE`/`PROGRESS`. There is no idle task-state polling.
- Save state: one full below-normal SQLCipher worker attempt after the first
  valid F7 player sample. That valid single-owner query records each exact
  area-task ID's `COMPLETE_CNT`; a missing row is baseline zero, while an
  ambiguous or invalid owner/query leaves the baseline unknown. An inconclusive
  exact area-quest completion witness may trigger an immediate coalesced
  exact-ID confirmation only for a known baseline, followed by at most two
  15-second retries. Only strict count growth confirms completion. Three
  non-confirming attempts lock that task generation until a new F7 or the
  settled `NONE`/`END` then later `ACCEPTABLE`/`PROGRESS` reactivation sequence.
  Those requests skip treasure SQL and are never periodic.
  Native runtime deltas override older snapshots, and Boss/Assault cooldown
  timestamps merge by maximum and survive F8/F7.
- Encounter visibility edges: one scalar check in the existing 250 ms service,
  rate-limited to 1 Hz. The fixed 49-entry visibility mask is recomputed only at
  the earliest cooldown expiry or a displayed world-hour edge.
- Expanded map: atlas construction/import occurs only for an explicit map
  attachment transaction. There is no per-marker expanded-map tick.
- Bird eggs: exact creation events populate a fixed weak pool. The existing
  250 ms control service examines at most eight unresolved candidates per tick;
  availability requires exact Actor-owned `DInteractableComponent`
  `InteractableValue=2` and `InteractTypeValue=2`, while unknown reads remain
  retryable. The nearest fixed set of 16 receives exact-state samples on that
  same 250 ms edge while the compact category is enabled, and exact EndPlay
  removes one weak identity immediately. There is no second timer. They never
  enter the expanded atlas or any SQL path.

## Safety invariants

- No retained Pawn, Controller, Canvas, Actor, task actor, or data-table UObject
  across frames or worlds. Runtime identities are weak and scalar snapshots are
  fixed-capacity.
- Travel and confirmed non-open-world activity transitions detach world-local
  compact and expanded-map hosts fail closed. Returning to the open world
  rearms a fresh compact host through the current Controller; ordinary
  menu/cursor suppression alone remains a non-destructive collapse.
- The exact title map is a cross-save hard boundary, not ordinary menu
  suppression. It disables the radar, detaches both renderers, clears mutable
  save-owned runtime state, and requires an explicit F7 only after the next save
  reaches a loaded open world.
- The normal UE4SS module-unload callback unregisters hooks and listeners
  symmetrically; this is runtime cleanup, not a user-facing uninstall tool. During
  UObject-array shutdown, the create listener is removed and the pinned
  process-lifetime callbacks become inert through the shutdown gate; they are
  not unsafely unregistered after UObject teardown has started.
- Unknown task conditions, mixed save identities, unsupported weighted task
  selection, invalid reflected schemas, ambiguous task-class identities, and
  unmapped completion callbacks do not become visible or completed by
  guesswork.
- No periodic SQL, recurring global UObject enumeration, automatic retry loop,
  or dynamically growing work queue. Encounter discovery uses only the fixed
  49-slot weak creation cache, and bird eggs use only their fixed 512-slot weak
  creation pool with a fixed nearest-16 active set.

## Build and verification

```powershell
.\tools\Build-Release.ps1 `
  -SdkRoot ".\.sdk" `
  -UE4SSRoot ".\.sdk\RE-UE4SS" `
  -ImGuiColorTextEditRoot ".\.sdk\ImGuiColorTextEditPinned" `
  -IconFontCppHeadersRoot "C:\path\to\pinned\IconFontCppHeaders" `
  -InstallerTestGameExecutable "C:\path\to\DSClient-Win64-Shipping.exe" `
  -InstallerTestExperimentalUE4SSDll "C:\path\to\ue4ss\UE4SS.dll" `
  -InstallerTestExperimentalDwmapiDll "C:\path\to\dwmapi.dll" `
  -InstallerTestWorkingDirectory "G:\isolated-installer-tests"
```

`Build-Release.ps1` is the supported release entry point. It builds the
Experimental-only Setup artifact, runs the isolated 20-case bootstrap,
conversion, migration, conflict, confirmation-token, and rollback matrix,
requires the two-case manual-install matrix, and re-extracts all three ZIPs
before publication. The 2.2.1 output contract is `dist/final-2.2.1` with
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Installer.zip`,
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-No-UE4SS.zip`,
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`,
`release-manifest.json`, and `SHA256SUMS.txt`. Legacy
`Stage-Release.ps1`, `Install.cmd`, and
`installer/Install-DragonSwordNativeWorldRadar.ps1` remain retired.

All generated workspace state is contained by `dist`: native and installer
builds use `dist/work/build`, package staging uses `dist/work/staging`, and
developer deployment backups use `dist/work/deployment`. The project root must
not contain `build-*`, `staging`, or `runtime` output directories. The pinned
dependency cache remains separately under `.sdk` and is not a deployable output.

The historical exact 2.2.0 replacement passed Core `2/2`, the compact, world-map,
PostRender, and release-hygiene gates, and a clean native `/W4 /WX` build. Its
source-bound `main.dll` SHA-256 is
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`, with
compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
`Build-Release.ps1` package validation passed for that exact DLL: Setup reports
`20/20`, the manual-copy matrix reports `2/2`, payload equivalence, manual layout,
and clean-target policy validation pass, and all three public ZIPs re-extract
byte-identically. Local diagnostics-enabled deployment of exact DLL
`6AEFDACC...` passed with matching source, build, and installed hashes. Its
rollback backup is
`dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
Backup
`dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`
belongs to the superseded intermediate 59529B2A deployment and is not 2.2.0
candidate evidence. This is not Setup ownership. The earlier 634D283A local debug deployment and
rollback backup remain superseded historical evidence only.
That historical evidence is not 2.2.1 evidence. Current exact-artifact 2.2.1
source/static/build/package/installer checks and rollback-backed developer
deployment passed for DLL `C2182308...` and compiled source `DE0100B2...`.
Gameplay, native-icon stability, click-target alignment, controller behavior,
height visuals, responsive layout, localization glyphs, exit behavior, and
external performance remain `NOT_VALIDATED`. The available healthy log is bound
to the prior exact 84A360B0 DLL and cannot validate either the historical
6AEFDACC 2.2.0 artifact or the current 2.2.1 candidate bytes.
Historical 2.1.1 automated and deployment
evidence remains bound to `dist/final-2.1.1` and packaged DLL SHA-256
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
The earlier `D4EE...`, `4AFE...`, and `BDE21...` artifacts and accepted local
gameplay remain explicitly scoped to the historical 2.1.0 audits.

The native build requires `IconFontCppHeaders` commit
`210b5a399a64270674560d633638952d1e8d804d`; this explicit source override
removes the floating upstream `main` branch from the release build. The script
also validates canonical origins, exact commits, clean worktrees, and nested
gitlinks, allows only the deterministic UE4SS `fmt` macro patch, refuses partial
or modified caches, and performs a graph-owned clean rebuild. It emits a receipt
that binds `main.dll` to the compiled source, dependency lock, build scripts,
configuration, and toolchain; staging and deployment reject a stale receipt.
SDK/dependency checkouts are intentionally not bundled in the source archive.
Exact revisions, license evidence, nested dependencies, and the current
public-release blockers are recorded in
[`docs/DEPENDENCY_SOURCES.md`](docs/DEPENDENCY_SOURCES.md).

`Deploy-NativePrototype.ps1` refuses to run while the game is active, rejects a
stale DLL, receipt, or source/metadata/DLL version mismatch, runs core tests and
all four static gates, creates a rollback backup, normalizes only the owned
`DragonSwordNativeWorldRadarPostRender` entry and removes only the owned
`DragonSwordWorldRadarObjectState` predecessor entry in `mods.txt` while preserving
its encoding and line ending, and verifies the exact installed payload. It never
changes the external `DragonSwordWorldRadar` entry: active or malformed authority
is rejected without mutation, while an explicit disabled `: 0` entry is preserved.
It rechecks that the game is stopped
before preparation, before backup creation, and once more immediately before
the short mutation transaction; it does not perform a late check that could
force rollback writes after the game has started.
Build and deployment are static evidence only.

The historical corrected 2.1.0 installer gate required 20 passed, zero failed,
and zero skipped cases; the manual gate requires 2 passed, zero failed, and zero
skipped.
Setup and both manual packages carry immutable public defaults, and no artifact contains
`enabled.txt`, local logs, backups, or runtime state. Package checks do not
deploy or launch the game and do not establish gameplay or publication
acceptance.

## Runtime acceptance for 2.2.1

- Require `START version=2.2.1`, runtime label
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1`, and the final packaged DLL
  SHA-256 recorded in `dist/final-2.2.1/release-manifest.json`.
- Confirm F6 fits as one page at representative windowed/fullscreen, DPI,
  16:9, 16:10, and 21:9 layouts. Every section and close control must remain
  readable and reachable with the longest localized strings. Bug Report and
  Close must remain in the top bar; the status text must stay read-only with its
  thin state strip; the independent status action must keep the page open; and
  translucent cards, equal-width filter choices, text alignment, and hit regions
  must remain correct.
- Confirm the three compact-only height settings all default ON for a clean
  install. Each selection must persist independently, and an existing valid
  configuration must retain its previous choices.
- Exercise Treasure, Area Quest, and Fly/Mole/Wave targets above and below the player.
  Treasure must keep its unchanged category-colored six-piece full arrow to the
  left of the selected chest. Every visible Area Quest
  must use authored marker Z to select the uniquely nearest existing source band
  for a multi-band profile without treating it as height. It preserves the black
  frame with three white dots inside the selected band's inclusive +/-500
  margin, uses an up triangle below that band, a down triangle above it, and
  remains neutral on an exact-distance tie or missing profile. The nearest visible
  Fly/Mole/Wave marker must use its actual kind palette for a shaftless triangle
  centered directly below its icon with a near-black contrast outline and
  unchanged size, position, and projection: target high shows up, target low shows down,
  and the inclusive +/-500 band hides it. All three height channels compare
  against `playerZ - 150`; Treasure retains its existing dead-zone behavior.
- Confirm all 83 map-100 Fly/Mole/Wave rows use their exact trusted
  `NPC_Start` height. A missing or ambiguous height must hide only the shared
  mini-game triangle, not the mini-game marker; the persisted key remains `mole`.
- Inspect English, Japanese, Korean, Simplified Chinese, Traditional Chinese,
  French, German, Spanish (Spain), Russian, Thai, and Portuguese (Brazil) for
  correct terminology, glyph coverage, clipping, slot-safe line height, and
  persistence. Confirm exact-size records use the post-viewport/prepass
  Font.Size reapply/readback path, and force both preparations to fail once to
  confirm the bounded no-Font-mutation render-scale fallback still presents the
  complete page without a partially valid tree. Confirm Korean and Traditional
  Chinese 2x overlays use pinned DroidSansFallback at base size 32, a one-pixel
  translucent stroke, and role-specific optical baselines. Confirm all 30 fixed
  main-panel text slots in each status-specific overlay, only the ko/zh-Hant
  names in the shared popup, regenerated current-layout coordinates, the
  canonical `assets/ui/f6` payload, and native game-font rendering for the other
  nine languages. Static no-clipping checks do not replace in-game size, weight,
  and alignment acceptance.
- Seed a legacy AUTO preference, change game language, and confirm the next
  actual F6 opening or F7 activation reads `DGameUserSettings.LanguageText`,
  applies the Kismet and English fallbacks when required, and persists the
  matching explicit language. Confirm AUTO/Use Game Language is not displayed,
  explicit choices persist, and no recurring language/font work is added.
- Confirm F6 opens while Radar is Off, On, and Faulted, reports the matching
  state through read-only text and a thin strip, exposes a separate
  Enable/Disable/Retry action without closing the page, rejects Enable before a
  playable world is ready, and opens the fixed Nexus Posts page from the top-bar
  Bug Report control.
- Repeat the physical-controller world-map and pause-menu suppression test with
  no hardware cursor. No controller mapping read is required.
- At windowed/fullscreen 16:9, 16:10, 21:9, varied DPI, and multiple zoom
  levels, confirm the native icon Canvas remains read only: no Mod child, slot,
  desired-size, prepass, ordering, or hit-test mutation is permitted. Confirm
  both hit-test-invisible atlas hosts remain viewport owned and their accepted
  viewport-local transforms track the native Canvas/player geometry without
  moving native icons or click targets. Same-parent observations may update only
  host transforms; they must not rerasterize, rebuild or reproject marker data,
  remove/add, or reparent. Each `Panel_Point` Image remains local
  `{0,0,atlas_width,atlas_height}`. No fixed `3000`/`8000` extent may appear in
  the live projection path. Confirm capacity 4,096 covers the accepted 2,779-row
  maximum with 1,317 spare slots. Inspect revision 50 outlines, shadows, and
  internal details at 3072, capture the approximately 72 MiB raw two-atlas BGRA
  envelope, and confirm marker coordinates and projection remain unchanged.
- Capture diagnostics-disabled same-session frame-time evidence and confirm no
  new scan, SQL, filesystem access, PAK extraction, language poll, or unbounded
  retry in steady state.

All items above are `NOT_VALIDATED` until captured against the exact 2.2.1
artifact. Static and deterministic verification cannot replace this live visual
acceptance.

## Historical runtime acceptance for 2.1.1

- Require `START version=2.1.1`, runtime label
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_1`, provider/hook
  readiness including `AREA_QUEST_EVENT_HOOK_READY`, and
  `AREA_QUEST_TASK_CLASS_MAP ready=true bindings=147
  ambiguities=0` with success at `attempt=1/2` or `attempt=2/2`. If the first
  attempt fails, require exactly one delayed second attempt and no third or
  steady retry.
- With a real controller and hidden hardware cursor, require the compact host
  to collapse while the world map or pause menu is open and to restore after
  close, including after F7 and travel. Confirm edge-only diagnostics and no
  added timer, controller-mapping read, scan, or 16 ms reflected query.
- Treat the former 2.1.1 single-height examples as historical artifact tests,
  not current 2.2.1 acceptance. Current task-height acceptance is owned by the
  144-profile height-band matrix above.
- The installed gameplay-test copy must report
  `runtime_diagnostics=startup_config_once config_debug_logging=true
  log_schema=2`. Separately
  stage or install the release default, restart once with diagnostics disabled,
  and confirm no new log file, rotation, or appended line is produced.
- In a clean session require no `ENGINE_TICK_FAULT`, renderer `Faulted` state,
  or Hub fault. For an intentionally induced engine-tick fault, require one
  `ENGINE_TICK_FAULT` followed by at most one `ENGINE_TICK_RECOVERED`; a later
  fault must produce `ENGINE_TICK_RECOVERY_STOPPED` and never enter a loop.
- Exercise explicit world-map recovery with F7 and Hub recovery with F6 only
  from prior runtime-only faults after clean detach. Same-call faults and ABI
  failures must remain fail closed.
- Confirm startup reports exactly one ignored treasure, save ID `11230106`, and
  that `(182813, 162051, 3150)` has no compact or expanded marker while nearby
  real treasures remain visible. Any additional ignore or changed catalog set
  difference fails acceptance.
- With diagnostics explicitly enabled, confirm the current native log stays at
  or below 1 MiB, only one previous log
  is retained, ordinary event batching does not create per-frame logging, and
  critical fault/lifecycle lines survive an immediate exit.
- Open two or three chests whose catalog points are inside one eight-metre
  radius, including the known 2.80-metre pair or 1.815-metre same-level pair.
  Open them in immediate succession and repeat once with overlapping XY at
  different heights. Require one distinct `TREASURE_OPENED_NATIVE` ID and one
  immediate marker removal for every real chest, with
  `identity=exact_treasure_actor_receiver` and an expected
  `source=object_id_class_3d|observed_weak_identity|unique_class_3d_fallback`.
  Reject any non-local interactor as immediate land-completion proof,
  neighboring false removal, or need for F8/F7 reconciliation.
- Open one uncollected underwater mount-only chest. Require immediate marker
  removal with `TREASURE_OPENED_NATIVE` evidence
  `local_mounted_rider_net_multi_execute_interact_prop`. The callback Actor
  must be the exact current `Pawn.Rider`; the receiver must remain mount-only,
  same-World, and within eight metres. For a deliberately unverified non-Pawn
  event, require `TREASURE_EVENT_REJECTED` and at most two
  `TREASURE_SAVE_CONFIRMATION_REQUESTED` exact-only requests with no encounter
  or dynamic-quest query. A positive confirmation must still remove the marker
  without F8/F7. No third request, poll, unrelated chest removal, UObject
  retention, or game-thread SQL is allowed.
- Complete the cooking/delivery task that previously remained visible. Require
  `AREA_QUEST_EVENT_TRIGGER progress_precondition=not_required` and
  `AREA_QUEST_COMPLETION_WITNESS_ARMED` with
  `window_seconds=10 probe_ms=750 schedule=exact_id_only`. The event alone must
  not complete the task. Accept either
  `AREA_QUEST_COMPLETION_VERIFIED evidence=exact_task_id_end_state`, or an
  expired witness followed by `AREA_QUEST_SAVE_CONFIRMATION_QUEUED` and a
  baseline-relative exact-ID `SAVE_COMPLETION_CONFIRMATION_APPLIED`. Require a
  valid single-owner F7 baseline and strict `COMPLETE_CNT` growth; an absent row
  is zero only for that valid query, while an unknown baseline must queue no
  SQL. The latter may make
  one immediate request and at most two retries spaced 15 seconds apart; require
  no fourth request and no periodic SQL. After three non-confirming attempts,
  repeated same-generation events must remain locked until a new F7 or a
  current `NONE`/`END` followed by later `ACCEPTABLE`/`PROGRESS`. The final
  `END` may follow a previously
  published `NONE`, `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` alone must not
  complete the task. Across simultaneous witnesses, allow at most one reflected
  exact-ID query per engine tick and no duplicate fixed-queue entry or deadline
  extension. Both maps must hide the task without F8/F7.
- Cross the relevant 23:00 edge without approaching task 1110080. Assault 104
  and its linked task must appear from global numeric state; the definition
  snapshot must report a unique monster link and zero ambiguity for that row.
- Press F6 and require a usable mouse cursor without opening another game menu.
  Change at least one compact-only and one expanded-map category and verify each
  selection applies immediately and persists across F8/F7. Close with `X`, then
  reopen and close with F6. Travel once with the Hub open; no panel, cursor, or
  input ghost may remain in the destination world.
- With `BIRD EGGS` enabled under RADAR, approach both exact egg actor classes
  and require stable compact markers with no expanded-map marker. Disable only
  that row and require egg markers to stop without changing treasure or
  mini-game visibility; re-enable it and verify persistence across restart.
  Diagnostics must show availability only at owned-component values `2/2`, an
  unknown read remaining retryable, exact EndPlay removing its one marker, no
  candidate-pool overflow in ordinary gameplay, no more than eight unresolved
  candidate/position queries per 250 ms control tick, and no new polling,
  UObject enumeration, or SQL request attributable to the feature.
- Activate in one save, return to the exact title screen, and load another save.
  Require one hard-stop boundary, no stale marker/provider state, no automatic
  reactivation, and rejection of F7 until the new save reaches a loaded open
  world. One explicit F7 there must start a fresh reconciliation without a new
  title-screen poll or SQL request before activation.
- Open the expanded map at several zoom levels and overlapping native icons.
- At maximum zoom, repeatedly close and reopen the map and change zoom. Require
  the exact zoom hook and only the five-deadline tail at 100, 250, 500, 1,000,
  and 1,250 ms for the latest trigger. Each due game-thread pass must take one
  fresh observation; overdue deadlines must never collapse into multiple
  observations in one pass. The first four passes must be read-only. The final
  pass may otherwise mutate only under the existing exact retained-parent
  witness and stable parent-local geometry rules. The correction must introduce
  no new parent-size post-check, growth-rejection rule, or extent-change token;
  later layout transitions remain on the existing bounded map/zoom
  reconstruction strategy.
  Each outer native Canvas slot must remain the atlas parent-local rectangle,
  and each `Panel_Point` Image must remain local
  `{0,0,atlas_width,atlas_height}`. Afterward both hosts must remain above
  native children with no continuing restack or steady poll. The current
  replacement DLL still requires live alignment acceptance, and a
  dense-Treasure flicker pass must be diagnosed independently rather than inferred
  from this placement contract.
- With the expanded map attached and visible, minimize the game and restore it.
  If the game creates a replacement `DLayerMap`, require the old renderer to
  remain fail-closed without failure 103/state 5, then require the replacement
  layer's exact `SetWorldMapImage` edge to consume at most one rearm and attach
  through the existing three-attempt service. Radar content must return without
  F8/F7 or another map close/reopen. No focus hook, focus poll, or steady timer
  may appear.
- After attaching the expanded map, close it, apply one treasure, mini-game,
  encounter, or area-task state delta, and open it again. A hidden retained
  layer must log `WORLD_MAP_RUNTIME_DELTA
  action=deferred_until_set_world_map_image`; the next exact
  `SetWorldMapImage` edge may log one serial-matched
  `WORLD_MAP_ATLAS_SET_IMAGE_REARM` and must build the nonblank current atlas.
  While the same exact layer is visibly open, the delta may instead use
  `action=bounded_visible_session_rebuild`. Repeated same-layer events must not
  replenish a completed or exhausted readiness budget.
- Enter one dungeon from an active open-world compact radar, complete or leave
  it, and return. Require `RADAR_ACTIVITY_SUPPRESSION` to report compact
  `detached` on entry, then a distinct `COMPACT_LAYER_CAPTURE
  source=create_listener` serial, fresh `COMPACT_GEOMETRY`, and
  `COMPACT_POOL_STATE state=2` with no new compact fault. Require
  `attach_rearmed=true` when the event is consumed in an enabled,
  non-transition, non-activity-suppressed context; `false` is valid only when
  the return edge already performed the bounded rearm. Repeat the round trip
  twice without F7/F8 to prove the weak-layer event route is not a one-shot
  timing accident.
  Every radar category must remain above game-native icons while Boss/Assault
  remain above the lower radar atlas.
- Trigger travel or a scene handoff. Compact rendering must collapse on the
  first failed current-Pawn sample instead of remaining visible for one to two
  seconds, then resume only after a valid destination sample and lifecycle
  stability gate.
- Complete a short area task without F8/F7. Require
  `AREA_QUEST_EXACT_COMPLETION_OBSERVED` with the exact catalog `id`,
  `evidence=task_class_map`, and `action=immediate_numeric_completion`; the
  compact icon must disappear before the generic refresh completes, and the
  task must be absent when the expanded map is next opened.
- During the same test, complete a task while an older transactional scan is in
  progress. The exact completion must remain hidden after that scan publishes.
- Complete a task and immediately trigger travel before the next normal engine
  sample. The `InitGameState` pre-transition path must drain the pending exact
  bit into numeric completion state before clearing task-class mapping state;
  the task and its dynamic-prerequisite evidence must remain completed after
  travel. Require `AREA_QUEST_EXACT_COMPLETION_OBSERVED
  action=travel_boundary_numeric_preserve`. This path must perform no
  task-actor/UObject access or retention.
- Require one later `AREA_QUEST_STATE_SCAN_REQUESTED
  reason=quest_state_event_debounced` transaction. Neither an older in-flight
  scan nor a later current scan that still reports `ACCEPTABLE` or `PROGRESS`
  may revive the task before an inactive boundary.
- For a genuinely repeatable task, require one current scan to observe `NONE`
  or `END`, then a later current scan to report `ACCEPTABLE` or `PROGRESS`
  before the icon reappears. A `FAIL` sample must not arm this reactivation.
- Complete two tasks in close succession. Each exact ID must disappear without
  an `AREA_QUEST_COMPLETION_UNMAPPED` line or cross-task removal.
- Complete a task while the one-shot F7 save worker is still pending. Applying
  that older result must not revive the task or remove its runtime completion
  from prerequisite evidence. If the expanded map was opened before the result,
  a visibly open exact attachment may rebuild once in the current session; a
  hidden retained attachment must defer until its next exact
  `SetWorldMapImage` edge. Either path must show the reconciled snapshot.
- After forcing a failed task-class capture, press F7 while the mod remains
  active. Require only bounded capture rearm and verify that existing exact
  completion state is not cleared.
- Confirm F8 followed by F7 still performs a fresh resynchronization rather
  than preserving the prior activation's completion latch.
- Approach one large Assault, let it become observed for more than one second,
  then leave its 100-metre range several times without defeating it. Also open
  the expanded map or another cursor-visible menu nearby. None of these cases
  may produce `ENCOUNTER_DEFEATED` or a new cooldown, and returning must show
  the still-live encounter again without requiring F8/F7.
- If a time-conditioned Assault is visible near the end of its availability
  window, let the window close without defeating it. The disappearance must
  produce no cooldown or completion, and a later valid window must allow the
  same live actor identity to bind again without F8/F7.
- Prove separately that an intentionally unavailable optional hook reports a
  bounded disabled reason while the rest of the radar remains ready. For
  exact-artifact 2.1.0 encounter regression coverage, require both
  `ENCOUNTER_DEATH_HOOK_READY` and `ENCOUNTER_DEATH_PROCESS_HOOK_READY`. The R4
  gameplay pass is historical evidence and does not accept the final 2.1.0
  artifact. Without using F8/F7,
  defeat that Assault and one Boss while staying nearby. For each real defeat,
  require `ENCOUNTER_DEFEATED_NATIVE
  evidence=exact_observed_net_multicast_notify_death`, one
  `ENCOUNTER_RUNTIME_STATE_APPLIED`, the 120-minute cooldown, and immediate
  marker removal. Separately verify that the conservative fallback cannot
  complete before at least forty missing 250 ms samples spanning at least ten
  seconds. An observed `RemovedFromWorld` event may
  retain only numeric missing evidence and must still pass the full ten-second
  gate; it must never publish completion directly or retain the ended Actor.
- At the end of a time-conditioned Assault window, accept a real exact death
  event while its callback context is valid, then let the displayed window
  change before the next 250 ms service. The published bit must still apply;
  the consumer must not recheck the event's already-proved time window.
- Delay ordinary consumption with an invalid Pawn/context, transition, or
  suppression state and require the accepted bit to remain pending. Publish
  more than one encounter bit and inject one apply failure: every successful
  bit must clear independently, the failing bit must remain for bounded retry,
  and no unrelated bit may be lost.
- With accepted bits pending, exercise F7, F8 disable, travel, and activity
  suppression separately. Each authoritative boundary may settle cooldown and
  eligibility numerically before reset but must not call the renderer. Only
  safe live-GameThread process-lifetime cleanup or the TitleMap owner boundary
  may hard-clear an unconsumed mask. UObject-array shutdown does not.
  Confirm there is no new polling, timer, enumeration, SQL, dynamic queue, or
  steady work.
- During the encounter test, confirm there is no `NEARBY_CLASS_CATCHUP` line
  and no production `FindAllOf` timing event. The `READY` contract must report
  `discovery=event_driven_fixed_49_weak_slots_no_enumeration_8_position_queries_per_control_tick`;
  current-actor distance, not catalog-point distance, must control candidate
  binding, and no control tick may perform more than eight actor-position
  queries.
- After attaching the expanded map, return to gameplay and open a map-100
  treasure or complete a linked mini-game. Reopen the same live map layer and
  require the changed marker to be absent from a next-session atlas, not reused
  through stale `WORLD_MAP_ATLAS_REUSED reason=same_live_layer` content. Repeat
  the same completion event and require no additional atlas invalidation or
  rebuild.
- Defeat a Boss or Assault and first require
  `ENCOUNTER_RUNTIME_STATE_APPLIED`. Then use F8 followed by F7 before the game
  writes a new save and require the encounter to remain hidden. The later save
  result must merge by maximum timestamp and never roll the cooldown backward.
- Observe one cooldown expiry and one time-conditioned Assault hour boundary.
  Require a single `RUNTIME_VISIBILITY_EDGE` only when visibility really
  changes, immediate compact refresh, and a fresh next-session expanded atlas;
  there must be no 49-entry work on the 16 ms motion path.
- Enter and leave the expanded map, pan and zoom, use F8 then F7, travel, and
  repeat. Require zero compact/world renderer faults and no stale icons.
- Measure the one-shot task-class-map and task-definition `elapsed_us`, bounded
  world-map-layer catch-up, clock baseline, and expanded-atlas `attach_total_us`.
  These event transactions are known bounded hitch candidates, not accepted
  performance claims. Confirm a multi-minute session contains no repeated SQL,
  class enumeration, attach retry storm, or growing log/queue pattern.

See `PROJECT_CONTEXT.md`, `docs/ARCHITECTURE.md`, and
`docs/PERFORMANCE_BASELINE.md` for ownership, lifecycle, and evidence limits.
