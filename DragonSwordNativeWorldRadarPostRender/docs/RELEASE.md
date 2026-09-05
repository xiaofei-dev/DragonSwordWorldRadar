# DragonSword Native World Radar 2.2.1 Release Contract

## Release identity

| Field | Value |
| --- | --- |
| Product | `DragonSwordNativeWorldRadarPostRender` |
| Version | `2.2.1` |
| Runtime label | `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1` |
| Public diagnostics | disabled |
| Installer | installer-first, unsigned |
| Supported runtime layout | ExperimentalNested |
| Game compatibility | bounded executable AMD64 PE32+ image at the exact path |
| UE4SS compatibility | bounded AMD64 PE32+ loader/proxy plus complete nested structure |
| Load authority | controlling `mods.txt` only |

The version label is an identity, not acceptance evidence. Source, build,
package, installation, gameplay, performance, and publication are separate
states.

The current feature scope and evidence boundary are recorded in
[Runtime Feedback Audit for 2.2.1](RUNTIME_FEEDBACK_AUDIT_2_2_1.md). The
[2.2.0 audit](RUNTIME_FEEDBACK_AUDIT_2_2_0.md),
[2.1.1 audit](RUNTIME_FEEDBACK_AUDIT_2_1_1.md), and
[2.1.0 audit](RUNTIME_FEEDBACK_AUDIT_2_1_0.md) remain historical evidence.

## Current 2.2.1 evidence state

Version 2.2.1 is a fixes-only release. The native world-map icon Canvas is a
read-only geometry witness and the Mod-owned atlas hosts are independent,
hit-test-invisible viewport widgets. Same-parent geometry changes update only
viewport-host transforms; they do not reraster, rebuild, reproject marker data,
re-add, or reparent an atlas. Exact-artifact 2.2.1 source validation, native
build, package validation, Setup `20/20`, Manual `2/2`, payload equivalence,
manual layout, clean-target policy, and three-archive byte-identical
re-extraction passed. The accepted native DLL SHA-256 is
`C21823088E38D2BD1635651981187AB4C01C2FFD0DCD4804CB9FFDB1899FABB9`,
and the compiled-source SHA-256 is
`DE0100B2D4DE894FA94C6911AD328F7699C55D50EC21193C688B44F7F2588BA2`.
Developer-local deployment of the same DLL passed with diagnostics enabled and
rollback backup
`dist/work/deployment/deploy-backups/20260903-194004-398-native-only-deploy`.
Gameplay, F6 visual behavior,
controller behavior, responsive-layout behavior, localization, clean exit,
and external performance remain `NOT_VALIDATED`.

## Historical exact 2.2.0 technical evidence

Version `1.2.0` was never published; its candidate changes are superseded by
and included in `2.1.0`.

The corrected 2.2.0 source passed Core `2/2`, every then-current static gate,
release hygiene, and a clean native `/W4 /WX` build. The source-bound DLL
SHA-256 is
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`,
and the compiled-source SHA-256 is
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
Historical build status was `PASSED`. `Build-Release.ps1` package validation passed
for that exact DLL: Setup reports `20/20`, the manual-copy matrix reports `2/2`,
payload equivalence, manual layout, and clean-target policy validation pass, and
all three public ZIPs re-extract byte-identically. Local
diagnostics-enabled deployment of exact DLL `6AEFDACC...` passed with matching
source, build, and installed hashes. Its rollback backup is
`dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
Backup
`dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`
belongs to the superseded intermediate 59529B2A deployment and is not current
candidate evidence. This is not Setup ownership and creates no live
`INSTALL-RECORD.txt`. The earlier
634D283A deployment and backup are historical only.

The packaged game-1.0.11 owner RVA and key-member offset `0x128` are fast paths
only. One FullActivation shares a total budget of at most 24 active-`.db` key
validations across packaged and structural owner routes. If the packaged fast-
path candidates fail authentication, FullActivation scans the current
executable at most once for exactly one retained structural signature. The scan
counts only targets inside the mapped image and scans each executable section
through `min(SizeOfRawData, VirtualSize)`. Structurally incompatible updates
fail closed; compatibility with every future game version is not promised.
Current F6 source applies measured desired-size text reflow to exact font-layout
TextBlocks after first open,
language changes, status changes, and language-popup display. Unavailable or
invalid desired-size evidence, a return structure that does not match the known
`Vector2D` identity, and the render-scale fallback retain authored text
geometry. That pass changes
no button hit box, compact-radar or expanded-map geometry, or per-frame path.
Gameplay, F6 visual alignment, UI, physical-controller behavior, localization,
exit behavior, and external performance remain `NOT_VALIDATED`.

An earlier package set sealed superseded DLL `5210E27D...`; that package set is
historical evidence only. The historical `dist/final-2.2.0/release-manifest.json`
is the machine-readable authority for the package set bound to exact DLL
`6AEFDACC...`. Setup and ZIP identities remain in that generated manifest and
`SHA256SUMS.txt` rather than being duplicated in this document. The local
developer deployment remains separate from public Setup ownership.

### Historical 2.1.1 sealed evidence

The 2.1.1 core tests, four source/static gates, clean native `/WX` build,
isolated Setup `20/20`, manual-copy `2/2`, payload equivalence, clean-target
validation, and archive re-extraction gates passed. Its packaged `main.dll`
SHA-256 is
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
That DLL is sealed in `dist/final-2.1.1` and was installed by the local
diagnostics-enabled deployment with exact hash identity. The existing treasure
override retained SHA-256
`CD52EE5C006B99BBF32ACDFE3ADD301507FDFD6C3DA7249BA6C3B6E6DB00BA43`.
This evidence is historical and must not be relabelled as 2.2.1.

The authoritative 2026-08-31 2.1.1 reseal records:

| Artifact | SHA-256 |
| --- | --- |
| `DragonSwordNativeWorldRadarPostRender-Setup-2.1.1.exe` | `2A5E1B7FBB08569E2E61D81EA9542A5CD12F7D8DE3D4A6FFBBCA529DE84F98A7` |
| `DragonSwordNativeWorldRadarPostRender-v2.1.1-Installer.zip` | `0BA492517DBD550DD6DF024B729F9083F51BEF979265D2BD0847B2BC41948A4B` |
| `DragonSwordNativeWorldRadarPostRender-v2.1.1-Manual-No-UE4SS.zip` | `D747CB865F1DF6F24449B166CEF64FFD5A00FAE8DE3AA628C4597AF3CBAD31EB` |
| `DragonSwordNativeWorldRadarPostRender-v2.1.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | `C9F4609AF1C02F03685A8F30308DA63168E33E4B4721157673BB31E007336BCF` |

`dist/final-2.1.1/release-manifest.json` is the machine-readable authority for
the native DLL identity, Setup `20/20`, manual-copy `2/2`, payload equivalence,
clean-target validation, and archive re-extraction evidence.
Embedded package metadata intentionally freezes the pre-deployment seal state;
the later local debug-deployment evidence is recorded in this contract and the
2.1.1 runtime feedback audit instead of mutating already-hashed public ZIPs.

The 2.2.1 expanded-map correction observes the live `PlayerIconWidget` pivot
and native icon Canvas without adding any Mod child to that Canvas. Both
geometries pass through `LocalToAbsolute` and then through the game viewport's
`AbsoluteToLocal` to update independent viewport-host transforms. It uses a
five-deadline 100/250/500/1,000/1,250 ms tail with one observation per due pass,
no overdue multi-observation collapse, and read-only sampling of game-owned
geometry at every deadline. A due pass may update only Mod-owned viewport-host
position, desired size, or visibility; no pass rebuilds marker data, rerasterizes
an atlas, or mutates the native widget tree. Before the first verified transform
or after an exact layer identity change, an unavailable Canvas or geometry
sample remains a hidden bounded retry. On the same layer after a valid transform,
the same transient gap retains the last verified position, size, and gated
visibility. Mod-owned payload, ABI, and guarded runtime failures still fail
closed.
No native Canvas desired-size, prepass, layout, child list, or hit-test state is
mutated. No new parent-size post-check, growth rejection, or extent-change token
is part of the current correction. These are
source properties, not runtime acceptance.

## 2.2.1 fixes-only runtime delta

The sole release delta is world-map ownership isolation. Native game-map
widgets remain authoritative and read-only. Mod atlas hosts are viewport-owned
and hit-test invisible. A stable host identity is retained across same-parent
geometry changes; only its transform changes. A transient same-layer native-
Canvas or geometry observation gap retains the last verified transform rather
than being misclassified as host corruption. The 2.2.0 feature set, catalogs,
capacity, projection math, zoom behavior, marker styling, F6 behavior, and
performance schedules remain unchanged. Exact-artifact gameplay acceptance is
pending.

## Historical 2.2.0 runtime delta

F6 is one responsive page containing Language, Marker Visibility, Height
Indicators (Radar Only), and Filter Modes. Its top bar contains Bug Report and
Close. A read-only Mod Status text with a thin state-colored strip is followed
by a separate Enable, Disable, or Retry action; using that action keeps the page
open, and enabling still requires a loaded playable world. The page fits and
clamps to the available viewport and can open while Radar is Off, On, or
Faulted. Bug Report opens the fixed Nexus Posts URL. The three
compact-only height controls are independent and all default ON on a clean
install; valid existing choices remain unchanged. Treasure keeps its unchanged
category-colored six-piece full arrow to the left of the selected chest. Every visible Area Quest evaluates a generated
  one- or two-band profile. For a multi-band profile, authored marker Z selects
  the uniquely nearest existing source band; it is selection evidence only and
  never synthetic height. Its normal black frame keeps three white dots inside
  the selected source band's inclusive +/-500 margin. A player below that band
  gets an up triangle and a player above it gets a down triangle. An exact-
  distance tie or missing source profile remains neutral with no dots or
  direction. There is no separate task arrow. Fly, Mole, and Wave share one
nearest-mini-game channel. Its shaftless triangle is centered directly below
the selected icon, uses the selected marker's actual kind palette, and adds a
near-black contrast outline without changing size, position, or projection. A target
  more than 500 vertical units above the comparable player Z shows an up triangle;
  a target more than 500 below shows a down triangle; the inclusive +/-500 band
  hides it. Treasure, Area Quest, and mini-game guidance all compare against
  `playerZ - 150`; Treasure retains its existing dead-zone behavior.

All 83 map-100 mini-game records use trusted exact heights from
`MiniGame_<kind>_<id>_NPC_Start`: 33 Fly, 40 Mole, and 10 Wave. An unavailable
or ambiguous height hides only the shared mini-game triangle and does not remove
its ordinary marker. The persisted key remains `mole`. The runtime consumes a
generated immutable table and does not parse XML, extract PAKs, scan objects,
or read files for mini-game heights.

The UI supports English, Japanese, Korean, Simplified Chinese, Traditional
Chinese, French, German, Spanish (Spain), Russian, Thai, and Portuguese
(Brazil). The centered selector contains only those 11 explicit choices; AUTO
and Use Game Language are not displayed. A legacy AUTO value migrates on the
next actual F6 opening or F7 activation through `DGameUserSettings.LanguageText`,
Kismet, and English to one persisted explicit language. F6 selects the already-
loaded Common/TC/JP/TH game Font objects by script. Missing font evidence falls
back safely without changing language, guessing an asset path, or replacing
FontMaterial. If the
constructed text widget reports exactly `Font.Size == 0`, one bounded reference
size is seeded. A game-widget construction, target-size, or font-commit failure
retries that text once as base UMG `TextBlock` during the same F6 transaction.
The real reflected `Font.Size`
participates in layout under each slot's safe line-height limit. After
`AddToViewport` and layout prepass, exact-size records are reapplied and read
back. If both exact-size paths fail, one fresh DTextBlock and then one base
TextBlock fallback leave Font untouched and use only the bounded viewport/DPI
render scale with a justification-aware pivot. Their `target_size=0` sentinel
skips both post-prepass exact-size loops. Core UMG creation, `SetText`, tree
 insertion, and viewport attachment remain fail closed. Font evidence never
 changes the selected language, and there is no per-frame or recurring
 language/font work. The canonical `assets/ui/f6` payload contains six status-
 specific Korean/Traditional-Chinese main overlays, one shared
 `language-popup.tga`, and its manifest. Each main overlay replaces all 30 fixed
 main-panel text slots; the popup replaces only the two affected language
 names. The overlays are generated at 2x from pinned DroidSansFallback at base
 size 32 with a one-pixel translucent stroke and role-specific optical
 baselines. The other nine languages remain on native game fonts. Static
 generation verifies that no overlay slot clips; live in-game size, weight, and
 alignment remain pending.

The historical 2.2.0 expanded-map host placement restored atlas-local projection ownership after
live screenshots rejected the full-parent outer-host experiment. Each outer
native Canvas slot occupies
`{atlas_left,atlas_top,atlas_width,atlas_height}`, and its `Panel_Point` Image
occupies local `{0,0,atlas_width,atlas_height}`. This correction adds no parent-
size post-attach check, parent-growth rejection, extent-change token, or other
new parent-size assumption. Later layout transitions continue through the
existing witnessed stable-geometry and bounded map/zoom reconstruction
strategy. Missing, mismatched, or final-unstable geometry follows that
established fail-closed/defer path. The cached-Slate,
player-anchor, independent-X/Y, DPI, zoom, and aspect-ratio chain is unchanged,
  and no `3000`/`8000` geometry constant is used. World-map glyph style revision
  50 uses two 3072 atlases and 50 percent more linear raster density. Two decoded
  BGRA atlases occupy about 72 MiB raw versus about 32 MiB at 2048. Marker
  coordinates, projection, zoom handling, native parent ownership, and outer/
  inner container geometry remain unchanged. Capacity is 4,096 against an
  accepted maximum of 2,500 Treasures plus 279 fixed non-Treasure rows, or 2,779
  total, leaving 1,317 spare slots. Runtime visual acceptance of revision 50
  remains `NOT_VALIDATED`.

This placement correction is pending live expanded-map alignment acceptance of
the current replacement DLL. The observed runtime snapshot contained 1,632
markers, including 1,501 Treasures, below the old 1,785 capacity; capacity was
  therefore not the flicker root. The historical exact 84A360B0 log shows one attach and no repeated
detach/rebuild sequence. Dense-Treasure flicker remains unresolved pending live
acceptance; release documentation must not describe it as fixed.

These were intended 2.2.0 source contracts, not current 2.2.1 build, package, gameplay,
responsive-layout, localization-glyph, controller, or performance acceptance.

## 2.1.1 runtime delta

Area Quest marker placement continues to use the validated 147-row MnMRadar
render table. Version 2.1.1 introduced the first independent reproducible
ActorPositionData task-height correction. That historical single-height
contract is superseded by the 2.2.0 height-band profile and is not a current
release gate. The authored marker Z may select the uniquely nearest existing
source band, but it never becomes a height source.

Treasure keeps the full colored shafted pointer. Area Quest reuses its own
fixed marker pieces in place and draws a closed black triangle, making the two
channels visibly distinct without adding
motion-path allocation or UObject work.

Controller-opened map and pause paths no longer depend on a visible mouse
cursor. Exact `SetWorldMapImage` latches compact suppression, optional validated
`IsGamePaused` sampling reuses the existing 250 ms edge, and layer `IsVisible`
is read only during bounded catch-up or while latched. No controller mapping,
new timer, scan, allocation, recurring log, or reflected 16 ms query is added;
the motion path consumes Booleans only. These are source and package properties,
not real-controller or frame-time acceptance.

## 2.1.0 runtime delta

The immediate treasure path requires the exact interaction receiver and either
the fresh local Pawn or, for mount-only underwater treasures, that Pawn's exact
callback-local `Rider` UObject. The mounted route additionally requires a
non-null same-World relationship and the existing eight-metre receiver bound.
Any other rejected non-Pawn event is not completion. It may arm only one
uniquely resolved treasure ID within eight metres of the current local player.
Fixed arrays wait 15 seconds before an exact-category, positive-only save
request and permit one final request 285 seconds later. Each request is capped
at 64 IDs, runs on the existing below-normal worker, skips encounter and
dynamic-quest SQL, and retains no UObject. This structural bound does not
establish exact 2.1.0 underwater gameplay or frame-time acceptance.

The same final version adds compact-only bird eggs. Exact `Bird_Egg01_C` and
`Bird_Egg02_C` creation events enter a fixed 512-slot weak pool; the existing
250 ms service examines at most eight unresolved candidates per tick. A Bird
Egg is available only when its Actor-owned `DInteractableComponent` reports
both `InteractableValue=2` and `InteractTypeValue=2`; Actor hidden state is not
used. Unknown reflection or ownership reads stay pending for the same bounded
service, and exact Bird Egg EndPlay removes the matching weak identity
immediately. A fixed nearest-16 set receives bounded checks on the same 250 ms
service edge while enabled. F6 controls this RADAR-only category independently; MAP is unavailable.
There is no new polling schedule, enumeration, SQL, dynamic queue, filesystem
polling, or expanded-map work.

The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is the
cross-save owner boundary. Entering it disables the radar, detaches both
renderers, clears weak candidates and mutable save-owned runtime state, and
latches activation off. Loading a save does not reactivate the runtime. F7 is
rejected until a fully loaded open-world identity exists; one explicit F7 there
starts a fresh activation and reconciliation. This edge reuses existing
transition/world identity signals and adds no new poll, enumeration, or SQL.

## Install versus Update / Repair

Setup selects exactly one of these modes after read-only preflight:

1. **Update / Repair**: a structurally complete ExperimentalNested UE4SS
   layout and a strictly owned existing Radar target are present. An older
   owned Radar release is eligible.
2. **Install**: the ExperimentalNested UE4SS layout is structurally complete
   and no owned Radar target exists.
3. **Bootstrap Install**: no UE4SS layout exists. Setup installs its embedded,
   integrity-verified ExperimentalNested runtime and Radar.
4. **Conversion Install**: a root, dual, incomplete, or malformed UE4SS layout
   exists. Setup requires explicit confirmation, creates and verifies a
   complete retained backup, migrates compatible Mod state, and installs the
   ExperimentalNested runtime.

Existing game, `UE4SS.dll`, and `dwmapi.dll` SHA-256 values are transaction
provenance only. They are not version allowlists and do not reject a
structurally compatible update. Embedded loader, proxy, mapping, settings, and
Radar payload hashes remain mandatory Setup-integrity checks.

Update / Repair keeps compatible existing loader, proxy, and settings bytes.
It refreshes the bundled Radar DLL, immutable generated catalogs, release
metadata, and mapping file. It does not extract or regenerate PAK catalogs on
the player's machine.

## User-owned state

Setup preserves these files byte-for-byte after strict validation:

- `config/visibility.ini`
- `config/diagnostics.ini`
- `data/defaults/treasure_overrides.txt`

A clean install creates those live files from embedded defaults. `.example.ini`
files are installer resources only and are not installed. Unknown paths,
malformed configuration, invalid treasure-ignore syntax, unsafe ownership, or
an unrecognized same-name target fails before mutation.

## Backup and rollback

Normal Install and Update / Repair use a bounded local transaction journal.
The journal is removed after a verified commit and is restored on failure. No
persistent copy of the replaced Radar is retained.

Bootstrap/Conversion retains a complete verified original UE4SS-layout backup
because it changes loader topology. The backup is created before active layout
removal and is never required for a normal compatible update.

## Runtime and load-control boundary

The installed product contains native UE4SS payload, declared catalogs,
configuration, metadata, licenses, and notices. It contains no external
renderer executable, Lua renderer, transparent overlay, watcher, or
`enabled.txt`.

The selected ExperimentalNested `mods.txt` is the sole load authority. Setup:

- normalizes exactly one
  `DragonSwordNativeWorldRadarPostRender : 1` entry;
- removes valid obsolete `DragonSwordWorldRadarObjectState` entries instead of
  adding a redundant disabled entry;
- preserves unrelated `mods.txt` bytes, encoding, BOM, and line endings;
- rejects active or malformed `DragonSwordWorldRadar` load control; and
- preserves an exact disabled `DragonSwordWorldRadar : 0` entry byte-for-byte.

## Public artifacts

The recommended channel is the installer-first archive:

`DragonSwordNativeWorldRadarPostRender-v2.2.1-Installer.zip`

Its exact root allowlist is:

1. `DragonSwordNativeWorldRadarPostRender-Setup-2.2.1.exe`
2. `DragonSwordNativeWorldRadarPostRender-Setup-2.2.1.exe.sha256`
3. `INSTALL.md`
4. `THIRD_PARTY_NOTICES.txt`

Two additional expected archives use the same ExperimentalNested Mod payload:

- `DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-No-UE4SS.zip` for an
  already compatible ExperimentalNested runtime;
- `DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` with the
  pinned integrity-verified ExperimentalNested runtime.

Both manual archives are script-free and map their roots directly to
`DS/Binaries/Win64`. No-UE4SS omits loader, proxy, and settings but includes a
clean one-line `ue4ss/Mods/mods.txt`; it is copied only when the target file is
missing and otherwise its Radar line is merged manually. With-UE4SS contains
the pinned runtime and clean load control and is valid only for a target with
no existing UE4SS. Neither manual archive contains a StableRoot payload. The
target `dist/final-2.2.1` allowlist is exactly those three ZIPs,
`release-manifest.json`, and `SHA256SUMS.txt`.

## Static acceptance gate

The following matrix passed against the source-bound 2.2.1 DLL and proves:

- structurally valid changed game and UE4SS DLL hashes do not block;
- malformed binaries and unsafe layouts still fail closed;
- older strictly owned Radar versions enter Update / Repair;
- all three user-owned files remain byte-identical;
- immutable catalogs are refreshed;
- successful normal/update paths retain no persistent backup;
- confirmed uninstall removes only strictly owned Radar state;
- stale uninstall confirmation performs no mutation; and
- injected uninstall failure restores Radar and `mods.txt` exactly;
- conversion retains a verified full backup; and
- injected failures roll back byte-exactly.

The isolated manual-install matrix reports exactly 2 passed, 0 failed, and 0
skipped. It proves payload equivalence with Setup, the exact
ExperimentalNested load-control line, public diagnostics disabled, absence of
`enabled.txt`, and inclusion/exclusion of the pinned runtime in the correct
manual archive.

Static and installer evidence does not prove runtime gameplay or performance.

## Gameplay acceptance

The future exact resealed and installed hashes require owner testing of:

- cold start, F7 activation, F8 disable, and F8/F7 resynchronization;
- exact title-screen hard stop, no automatic activation while another save
  loads, rejection of pre-open-world F7, and one fresh explicit F7 activation
  after the loaded open world is ready;
- F6 responsive one-page layout in Off/On/Fault states, top-bar Bug Report and
  Close, read-only status text plus a thin state-colored strip, a separate
  Enable/Disable/Retry action that keeps the page open, playable-world guard,
  fixed Nexus Posts Bug Report, language selection, three
  independent default-on height controls, and persistence, including independent compact-only bird eggs with
  an unavailable MAP cell, area-quest `AVAILABLE` / `ALL`, and
  Assault `AVAILABLE` / `ALL`; ALL must show the static 40-record Assault
  catalog, and switching back to AVAILABLE must immediately restore state,
  time-window, and cooldown filters;
- compact and expanded rendering, pan, zoom, maximum zoom, close/reopen, and
  minimize/restore, with the native icon Canvas remaining read-only and both
  Mod-owned atlas hosts remaining independent, hit-test-invisible viewport
  widgets; same-parent geometry changes must update transforms only, with no
  reraster, rebuild, marker-data reprojection, re-add, reparent, native
  desired-size change, or native click-target displacement;
  revision 50 outlines, shadows, and internal details at 3072 must be visually
reviewed while marker coordinates, projection, zoom handling, game-native widget
ownership, and native container geometry remain unchanged; capture attach time and
  the approximately 72 MiB raw two-atlas BGRA envelope;
  reproduce the reported dense-Treasure drift/flicker and prove it absent against
  the exact 2.2.1 artifact before making a runtime-fixed claim; the observed
  1,632-marker snapshot was below the old capacity and the
  historical exact 84A360B0 log's single attach/no repeated detach-rebuild
  sequence is not live
  visual acceptance;
- treasure, clustered/overlapping treasure, Boss, Assault, mini-game, every
  visible Area Quest's inclusive +/-500 aligned/up/down/neutral height-band state,
  including below/above the selected source band and exact-tie/no-source cases,
  compact-only bird egg, clock, the unchanged left-side Treasure six-piece
  arrow, and the nearest-mini-game below-icon triangle with actual Fly/Mole/Wave
  coloring, target-high/up, target-low/down, and an inclusive +/-500 hidden
  band, including shared `playerZ - 150` comparison for every height channel,
  uniquely nearest source-band selection for multi-band Area Quest profiles,
  all 83 trusted mini-game heights, and
  Bird Egg availability from the exact owned component values, retry of unknown
  reads, and immediate exact EndPlay removal;
- Boss/Assault defeat and area-task completion without F8/F7 dependency;
- dungeon/travel/open-world return and long-session stability; and
- diagnostic-off startup plus real frame-time comparison.

Static, deterministic, build, and package gates cannot replace live visual
acceptance of expanded-map alignment, revision 50 glyph styling, or every-
language F6 font/glyph layout.

## Evidence states

| State | Current status | Meaning |
| --- | --- | --- |
| `SOURCE_VALIDATED` | `PASSED` | exact 2.2.1 source and all current static/core gates passed |
| `BUILT` | `PASSED` | source-bound DLL `C2182308...` and compiled source `DE0100B2...` accepted |
| `PACKAGED` | `PASSED` | all three 2.2.1 ZIPs built and re-extracted byte-identically |
| `INSTALLER_TESTED` | `PASSED` | Setup `20/20`, Manual `2/2`, payload equivalence, layout, and clean-target validation passed against exact 2.2.1 bytes |
| `DEPLOYED` | `PASSED_DEVELOPER_LOCAL` | exact DLL `C2182308...` is installed with diagnostics enabled; this is not public Setup ownership or gameplay acceptance |
| `GAMEPLAY_ACCEPTED` | `NOT_VALIDATED` | native-icon stability, click-target alignment, pan/zoom/DPI/aspect behavior, dense Treasure, F6, controller behavior, exit behavior, gameplay, and external frame time require exact-artifact testing |
| `SOURCE_PUBLICATION_AUTHORIZED` | `PASSED` | owner authorized upload of the workspace source |
| `BINARY_AND_DERIVED_DATA_PUBLICATION` | `BLOCKED` | rights/provenance review remains incomplete |

No state implies a later state.

## Publication blockers

Public binary and derived-data publication remains blocked independently of
technical readiness until:

1. Unreal Engine/UEPseudo authorization, redistribution, and license
   compatibility are confirmed.
2. Exact `e_sqlcipher.dll` source/build provenance is recorded and reviewed.
3. Redistribution rights and attribution for generated catalogs and derived
   coordinate data are confirmed.

Do not publish the blocked binary or derived inputs, a binary archive, or a
release tag until all three reviews are cleared. Source-only workspace upload
is separately authorized and excludes those local inputs.
