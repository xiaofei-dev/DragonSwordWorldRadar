# Release Routing and Historical 2.2.1 Contract

## Current Guide asset and package revision

Candidate `radar-3.0.0-sg16-guide4-20260909`; version **3.0.0**.

This is a layout-only asset revision of `radar-3.0.0-sg16-guide3-20260909`. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. No native code or localization-header change is included, and no new native compilation is claimed.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T02:04:37.4453785Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T02:06:49.3883924Z`.

Manifest UTC: `2026-09-10T02:06:02.2485470Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/package-verification.json).

The previous Guide3 and earlier SG-16 sections below remain historical records. Their earlier activity order and ZIP hashes identify those sets. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Current Guide asset and package revision

Candidate `radar-3.0.0-sg16-guide3-20260909`; version **3.0.0**.

This is an asset/documentation revision of `radar-3.0.0-sg16-20260909`, not a new native build. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. The parent's 446/446 native and Core 9/9 evidence retains its original identity.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T01:45:25.9749727Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T01:47:39.7016800Z`.

Manifest UTC: `2026-09-10T01:46:51.6121910Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/package-verification.json).

The previous SG-16 sections below are preserved historical records. Their four-column Guide description and old ZIP hashes describe that earlier set. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

Current local delivery is **3.0.0 / SG-16**, candidate `radar-3.0.0-sg16-20260909`.
Build, local deployment and all three packages are verified and promoted. The
installed and packaged DLL is byte-identical. Native 446/446, Core 9/9 and all four source/release gates pass. Setup passes 20/20 and Manual 2/2, with no failures or skips. The three channels have 104 equivalent runtime files, including 73 UI files and twelve Guide files; ZIP entry counts are 4/108/112.
The preceding five SG-15 final files are backed up at `dist/work/candidates/radar-3.0.0-sg16-20260909/previous-final-sg15`.
Use current receipts in [Release status](RELEASE_STATUS.md); do not reuse
historical hashes. Payload documents remain unchanged after the final package seal.
Game acceptance and external publication remain separate.

The preceding runtime source is **3.0.0 / SG-15**, including localized Guide,
stable Aim/Auto distance switching and the revised Settings UI. SG-15 was
locally deployed and verified. The owner has now authorized three local
release packages; the package candidate is `radar-3.0.0-sg15-release-20260909`.
The three packages are generated and promoted, with Setup 20/20, Manual 2/2,
104 equivalent runtime files and 4/108/112 ZIP entries. Release status records
their exact identities. The rebuilt DLL was not redeployed; the earlier SG-15
installation and backed-up SG-10 packages retain their own identities.

Current installed 3.0.0 candidate, package status and exact checks:
[Release status](RELEASE_STATUS.md).
Current validation plan: [3.0.0 Development and Validation](RELEASE_PLAN_3_0_0.md).
All 2.2.1 contracts, commands, hashes, and acceptance rows below are historical;
do not use them to build or validate 3.0.0. The local 3.0.0 package set is in
`dist/final-3.0.0`; owner gameplay acceptance and external publication remain separate.
This request does not authorize Nexus uploading, posting or external publication.

This document's version-specific receipts are historical. For the historical
2.3.0 CM-04 three-package delivery and remaining acceptance boundaries, use
`RELEASE_PLAN_2_3_0.md` and `dist/final-2.3.0/release-manifest.json` (the latter
relative to the project root). Do not relabel the 2.2.1 evidence below.

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

Version 2.2.1 is a fixes-only full-stretch-host/inner-atlas candidate. Both Mod-
owned, hit-test-invisible atlas hosts attach to the current
`DLayerMap.FogAbovePanel`, resolved directly from the current layer.
`ArrayIconInfo` is creation-only evidence for one instantiable icon class; it
does not choose the parent, and retained-host validation/refresh does not scan
it. Each outer slot is full stretch with zero offsets, `AutoSize=false`, zero
alignment, and maximum Canvas Z. Each cloned inner `Panel_Point` slot is also
full stretch with zero offsets. Only each Image Canvas slot owns
`{atlas_left,atlas_top,atlas_width,atlas_height}`; Image render translation stays
`(0,0)`. This prevents negative atlas offsets from feeding back into the native
parent's desired extent. No host render transform or forced layout prepass is
used.

Earlier 2.2.1 technical evidence remains bound to its exact candidate bytes.
The independent-viewport/extreme-Z deployed build is runtime rejected: it made
markers visible, but live testing found severe lag, wrong placement, and delayed
updates. The later first-valid-icon-parent candidate is also rejected. Temporary
zoom-topology diagnostics show native icon reconstruction changing the first
valid parent between `FogAbovePanel` and `FogUnderPanel`; the Mod reattached its
hosts four times in one zoom sequence, crossing fog layers and producing the
observed occlusion, hitching, and flashing. DLL `FE811E81...B82A21`, compiled
source `45EF8C60...39CA3`, and its rollback-backed deployment are exact-byte
evidence for that superseded candidate only. The topology diagnostics are
development evidence, not corrected gameplay acceptance. The subsequent direct-
`FogAbovePanel` full-stretch-outer/Image-translation DLL `CCC6B117...AE00` from
compiled source `B650B5FB...74EA` is also runtime rejected. Quantitative
2026-09-05 screenshots show base-map/Radar scale pairs 1.214/1.218 and
0.760/0.758 but relative translations about `(+113,-190)` and `(+67,+200)` px;
mean fitted residuals are only 0.28 px over 32 points and 0.82 px over 35
points. Near-equal scale with sign-reversing vertical offset establishes a
different local origin/zoom pivot rather than scale-formula or cumulative-frame
drift. Its deployment backup
`dist/work/deployment/deploy-backups/20260905-092836-495-native-only-deploy`
remains rejected-candidate evidence only. The subsequent outer-atlas-rectangle
DLL `CD41F0E1...6FBB2` from compiled source `433710E0...E62C` is also runtime
rejected. Although it attached 1,632 markers with no data, texture, or ABI fault,
its negative outer offset changed the same parent extent from `3000` to
`3191.521`, emitted `WORLD_MAP_LAYERING_REBUILD_REQUIRED`, and oscillated through
six attaches and five detaches. Its diagnostics-enabled deployment backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`
remains rejected evidence only. Current WM-06 source review, static gates, Core
`2/2`, release hygiene, and local native build pass at DLL
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1` from
compiled source
`0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`, size
1,107,968 bytes. Rollback-backed diagnostics-enabled developer deployment
passes for those exact bytes with backup
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
The installed DLL hash and size match, the controlling `mods.txt` contains
exactly one Radar entry, `debug_logging=true`, and the game was stopped after
deployment. This proves local deployment identity only; it is not Setup
ownership or gameplay acceptance. Package and installer identities remain
pending. Gameplay, F6 and expanded-map visual behavior, controller behavior,
responsive layout and resolutions, localization, clean exit, attach time,
memory, and external performance remain `NOT_VALIDATED`. Public binary and
derived-data publication remains blocked.

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

The 2.2.1 expanded-map correction resolves the live `PlayerIconWidget` pivot
only during fresh attachment and resolves the current
`DLayerMap.FogAbovePanel`, then inserts only two Mod-owned hosts.
`ArrayIconInfo` supplies only a missing-host creation class and is not scanned
by retained-host validation/refresh. Each outer slot is full stretch with zero
offsets, `AutoSize=false`, zero alignment, and maximum Z. After every fresh
attachment, including a scheduler-accepted rebuild, each cloned inner
`Panel_Point` slot is also full stretch with zero
offsets. Only each Image Canvas slot holds
`{atlas_left,atlas_top,atlas_width,atlas_height}`; Image render translation stays
`(0,0)`. The native parent
therefore supplies pan, zoom, clipping, visibility, and RetainerBox composition
directly. Every pass in the bounded 100/250/500/1,000/1,250 ms tail reads live
parent extent. A fully unchanged same-parent pass, including the final pass,
performs no layout/transform write, `ArrayIconInfo` scan, viewport-transform
replay, restack, Remove/Add, visibility write, or `RequestRender`.
Retained refresh reads no `PlayerIconWidget`; anchor-only changes are ignored
and the attach-time Image Canvas-slot atlas position stays immutable. A real
`FogAbovePanel` replacement reports `RebuildRequired`; only a scheduler-owned
fresh attachment restores the full-stretch outer host, inner fill, Image atlas
rectangle, and zero Image translation. A changed
extent must be observed as two matching samples before `RebuildRequired` is
reported. Reporting is non-mutating: it does not collapse, hide, detach, or mark
the last valid payload transform-unready; only the scheduler owns an accepted
rebuild. A successful attach arms the tail from a fresh post-attach clock sample
and does not issue an empty-host `RequestRender` before visibility is applied.
One open-map session may rebuild at most once while preserving the marker
snapshot. Its rebuild has its own hard-capped three-attempt attach/geometry
budget, bounding the session to initial 3 plus rebuild 3. Unchanged and anchor-
only passes do not rerasterize or rebuild marker data, and no forced layout
prepass is used. Runtime acceptance requires one attach, zero detaches, no
native-parent extent feedback, and no `WORLD_MAP_LAYERING_REBUILD_REQUIRED` in a
stable map session.

The two decoded BGRA atlases occupy approximately 32 MiB. Cache envelope
`DSNWRA52` quantizes the normalized fingerprint at 1/4096 UMG logical unit. A
hit requires exact dimensions/header/magic/fingerprint/visible count, full RLE
decode to exactly 2048-by-2048 pixels, encoded-payload checksum validation, and
payload termination exactly at EOF. Revision-51, corrupt, truncated, or
trailing-byte files miss. Writes use a same-directory temporary file and
atomically publish via `MoveFileExW` with replace-existing and write-through
flags; failures remove the temporary file. File validation, optional write, and
texture import remain bounded attachment work. These are source properties, not
gameplay or performance acceptance.

## 2.2.1 fixes-only runtime delta

The sole release delta is the expanded-map native ownership and attachment-
cost correction. Both Mod hosts are hit-test-invisible children
of the directly resolved current `DLayerMap.FogAbovePanel`. `ArrayIconInfo` is
used only when constructing a missing host to obtain an instantiable icon class;
steady validation/refresh does not scan it. Each outer slot is full stretch with
zero offsets, non-auto-sized, zero aligned, and maximum Z. Each inner
`Panel_Point` is independently forced to full stretch; only the Image Canvas
slot owns the atlas rectangle, and Image render translation is zero. No host
transform or forced prepass is used. Two 2048 atlases and persistent TGA
`DSNWRA52` fingerprint reuse remain unchanged. This placement replaces the
runtime-rejected outer-atlas-rectangle candidate, the earlier full-stretch-
outer/Image-translation candidate, and the 3072 independent-viewport candidate.
The 2.2.0 feature set, catalogs, 4,096 capacity, marker coordinates, F6 behavior,
and non-expanded-map schedules remain unchanged. Exact-artifact gameplay and
performance acceptance is pending.

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

The previous 2.2.1 candidate passed the following matrix for its exact bytes:

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

Its isolated manual-install matrix reported exactly 2 passed, 0 failed, and 0
skipped. For those superseded bytes it proved payload equivalence with Setup, the exact
ExperimentalNested load-control line, public diagnostics disabled, absence of
`enabled.txt`, and inclusion/exclusion of the pinned runtime in the correct
manual archive.

The current full-stretch-host/inner-atlas candidate must repeat every package,
installer, and archive gate. Previous static and installer evidence does not prove the candidate's
runtime gameplay or performance.

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
  minimize/restore, with both Mod-owned atlas hosts remaining hit-test-invisible,
  children of the current `DLayerMap.FogAbovePanel`; their outer slots must be
  full stretch with zero offsets, `AutoSize=false`, zero alignment, and maximum
  Z. After each fresh attachment, including a scheduler-accepted rebuild, the
  inner `Panel_Point` slots must also be full stretch with zero offsets. Only
  the Image Canvas slots may hold
  `{atlas_left,atlas_top,atlas_width,atlas_height}`, while each Image keeps zero
  render translation; verify
  `ArrayIconInfo` is creation-only class evidence and absent from steady validation;
  verify direct pan/zoom/clipping/RetainerBox inheritance, no same-parent
  viewport-transform write, no forced prepass or native desired-size change,
  and no native click-target displacement; verify every tail reads live extent,
  unchanged same-parent passes including the final pass make no layout/transform
  write and never restack, Remove/Add, or `RequestRender`; verify attachment
  itself does not request an empty-host render and arms the tail from a fresh
  post-attach clock sample; retained tails read only the direct
  `FogAbovePanel` extent, never `PlayerIconWidget`, and anchor-only changes must
  leave the attach-time Image Canvas-slot atlas positions immutable. A real
  parent replacement must only report `RebuildRequired`; the scheduler alone
  may perform the fresh attachment that restores the complete layout contract;
  force extent drift and verify that two matching
  stable samples are required before report-only `RebuildRequired`; reporting
  must not hide, collapse, detach, or mark the last valid payload transform-
  unready, because the scheduler alone owns the accepted mutation; verify at
  most one snapshot-preserving rebuild per open session with its own three-
  attempt budget (initial 3 plus rebuild 3 maximum); require one attach, zero
  detaches, unchanged parent extent, and no
  `WORLD_MAP_LAYERING_REBUILD_REQUIRED` in a stable open-map session;
  visually review the 2048 atlas output while marker coordinates remain
  unchanged; capture cold and fully validated `DSNWRA52` cache-hit attach time
  and the approximately 32 MiB raw two-atlas BGRA envelope; verify exact metadata,
  1/4096 fingerprint quantization, full RLE pixel count, encoded checksum, exact
  EOF/trailing rejection, and same-directory atomic temp publication;
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
acceptance of expanded-map alignment, 2048 glyph presentation, or every-
language F6 font/glyph layout.

## Evidence states

| State | Current status | Meaning |
| --- | --- | --- |
| `SOURCE_VALIDATED` | `PASSED_CURRENT_FULL_STRETCH_HOST_INNER_ATLAS_CANDIDATE` | source review, static gates, release hygiene, and Core `2/2` pass for the exact current source |
| `BUILT` | `PASSED_CURRENT_WM_06_IMMUTABLE_SLOT_LOCAL_NATIVE_BUILD` | DLL `6435E100...C723A1` from compiled source `0A1A4CE3...B5E5BC5`, size 1,107,968 bytes; package is a separate gate |
| `PACKAGED` | `PASSED_CURRENT_WM_06_THREE_ARCHIVE_BYTE_IDENTICAL_REEXTRACTION` | all three 2.2.1 ZIPs bind DLL `6435E100...C723A1`; final SHA-256 values are recorded in `dist/final-2.2.1/release-manifest.json` and `SHA256SUMS.txt` |
| `INSTALLER_TESTED` | `PASSED_CURRENT_WM_06_SETUP_20_OF_20_MANUAL_2_OF_2_AND_PAYLOAD_GATES` | Setup `20/20`, Manual `2/2`, payload equivalence, manual layout, clean-target policy, source immutability, and fixture cleanup pass |
| `DEPLOYED` | `PASSED_CURRENT_CANDIDATE_ROLLBACK_BACKED_DIAGNOSTICS_ENABLED` | exact installed DLL `6435E100...C723A1`, size 1,107,968 bytes, matches the build; backup `20260905-202742-614-native-only-deploy`, `mods.txt` count 1, `debug_logging=true`, and game stopped after deploy; this is not Setup ownership or gameplay acceptance |
| `GAMEPLAY_ACCEPTED` | `NOT_VALIDATED` | native-icon stability, click-target alignment, dense Treasure, F6, controller behavior, exit behavior, and gameplay require exact-artifact testing |
| `VISUAL_ACCEPTED` | `NOT_VALIDATED` | expanded-map alignment, flashing, disappearance, and F6 presentation require live visual testing |
| `RESOLUTION_ACCEPTED` | `NOT_VALIDATED` | pan/zoom/DPI/aspect, 4K, 21:9, 16:10, fullscreen, borderless, and windowed cases require live testing |
| `PERFORMANCE_ACCEPTED` | `NOT_VALIDATED` | attach time, memory, and external frame time require live testing |
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
