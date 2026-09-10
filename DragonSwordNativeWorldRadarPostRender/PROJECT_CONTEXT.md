# Project Context

## September 10 source handoff

The owner has published Radar 3.0.0 on Nexus. Source synchronization is tracked
in `../docs/GITHUB_CLOSEOUT_2026_09_10.md`; dated package/deployment records below
retain their original evidence scope. Public download bytes were not rechecked.
Two support reports (previously opened chest markers and markers appearing
only after first fast travel) await logs; neither has a confirmed root cause
or a new fix in this handoff. See `docs/RELEASE_STATUS.md`.

## Current Guide asset revision

Candidate `radar-3.0.0-sg16-guide4-20260909`; version **3.0.0**.

Guide swaps the Marmot mini-game and Sudden mission entries as complete groups: the text and both Radar/Map icons move together. The left column now reads Flying mini-game, Wave mini-game, Marmot mini-game and Bird eggs; the right column reads Sudden mission, World boss and Area quest. All eleven languages use this order. The three height columns, 40 icon examples and 539 text cells remain unchanged.

This is a layout-only asset revision of `radar-3.0.0-sg16-guide3-20260909`. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. No native code or localization-header change is included, and no new native compilation is claimed.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T02:04:37.4453785Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T02:06:49.3883924Z`.

Manifest UTC: `2026-09-10T02:06:02.2485470Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3](dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3/).

Evidence: [deployment](dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/deployment-verification.json), [release manifest](dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/release-packages/release-manifest.json) and [promotion](dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/package-verification.json).

The previous Guide3 and earlier SG-16 sections below remain historical records. Their earlier activity order and ZIP hashes identify those sets. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Current Guide asset revision

Candidate `radar-3.0.0-sg16-guide3-20260909`; version **3.0.0**.

The owner requested a simpler height reference. Guide now displays only Above, Near level and Below, with 15 height examples, 40 total icon examples and 539 text cells across eleven languages. The Unknown column is no longer drawn. Actual unknown-height handling, height switches and all other gameplay behavior are unchanged. The existing 44-string localization schema remains intact.

This is an asset/documentation revision of `radar-3.0.0-sg16-20260909`, not a new native build. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. The parent's 446/446 native and Core 9/9 evidence retains its original identity.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T01:45:25.9749727Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T01:47:39.7016800Z`.

Manifest UTC: `2026-09-10T01:46:51.6121910Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16](dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16/).

Evidence: [deployment](dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/deployment-verification.json), [release manifest](dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/release-packages/release-manifest.json) and [promotion](dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/package-verification.json).

The previous SG-16 sections below are preserved historical records. Their four-column Guide description and old ZIP hashes describe that earlier set. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

Current source and local delivery: **3.0.0 / SG-16**, candidate
`radar-3.0.0-sg16-20260909`. The revision adds independent Radar/Map All controls, live category
changes on an open world map and default Auto focus. Guide clarifies mini-game
reward chests and flying/marmot/wave types, uses compact two-column entries,
illustrates height states and Clock, aligns Scene examples and gives each
distance mode its own row. Aim first acquisition is 100 ms; replacement buffers
remain 350/500 ms. Updates preserve explicit saved settings.

Native 446/446, Core 9/9 and all four source/release gates pass. Setup passes 20/20 and Manual 2/2, with no failures or skips. The three channels have 104 equivalent runtime files, including 73 UI files and twelve Guide files; ZIP entry counts are 4/108/112.

The same native artifact is locally deployed and packaged. Deployment UTC:
`2026-09-10T00:22:37.0293741Z`; promotion UTC: `2026-09-10T00:23:01.4155094Z`.
The final directory is SG-16. Five earlier SG-15 files are preserved in
`dist/work/candidates/radar-3.0.0-sg16-20260909/previous-final-sg15`. [Release status](docs/RELEASE_STATUS.md) owns exact identities and
receipts. Payload documents were finalized and frozen before the final package build.

Build, source/resource checks, installation and package checks are verified; they do not establish game visual, input, Guide usability, camera-motion or measured performance acceptance. Those remain owner testing. No Nexus upload or post was performed.

Previous source, 2026-09-09: **3.0.0 / SG-15**, candidate
`radar-3.0.0-sg15-20260909`. Guide beside Close opens an eleven-language icon
reference with Radar/Map comparisons, treasure colors, height arrows, Scene
symbols and distance modes. Guide and Settings retain separate scroll positions
while F6 is open. Missing guide artwork leaves Settings available. The existing
61 UI files remain unchanged; eleven guide atlases and one manifest bring the
installed UI inventory to 73 files. Core 9/9, native 446/446, all four gates and
local deployment are verified in [Release status](docs/RELEASE_STATUS.md).
SG-14 typography, SG-13 Aim/Auto switching and H2 optional scrollbar compatibility
are retained. Game appearance, guide scrolling and input remain owner testing.
All eleven languages have concise help and
confirmation text, pinned regular fonts and consistent numeric glyphs.
**Reset to Defaults / Vote for This Mod / Feedback** share a fixed bottom row.
Vote refers to Nexus Mod of the Month voting and opens the mod page after
confirmation. The player completes the vote there.

The reference panel is **760×852**. Its four cards scroll on short displays,
with header/footer always accessible. Window and DPI changes update the
existing tree. Scene rendering uses current-frame calibrated batch projection,
native fallback, changed subpixel translations and stable visibility margins.
Raw catalog positions, height rules, distance corrections and presets remain.

Core **9/9**, native **446/446**, font checks and numeric projection/layout
checks pass. [Release status](docs/RELEASE_STATUS.md) tracks the final gates and
deployment receipt. SG-11 was built only and superseded before delivery.
Three local 3.0.0 release packages are complete and verified. Candidate
`radar-3.0.0-sg15-release-20260909` passes Setup 20/20, Manual 2/2, payload
equivalence and fresh extraction checks: 104 runtime files, 73 UI files and
4/108/112 ZIP entries. Release status records exact hashes and verified final
promotion, with the previous five SG-10 files backed up. The rebuilt DLL has
the same compiled source as deployed SG-15 but a different hash; it was not
redeployed. The original SG-15 installation remains in place. Package metadata
and payload documents remain the frozen pre-package snapshot. Nexus edits are
local copy only; game acceptance and external publication remain separate.

## Preserved SG-10 delivery

Verified previous source, 2026-09-09: **3.0.0 / SG-10**, candidate
`radar-3.0.0-sg10-20260909`. **Build, local deployment and all three final local
packages are complete and verified.** Core 7/7, the 446-target clean native
build and all four source gates pass; Setup 20/20 and Manual 2/2 pass.
SG-10 was installed at `2026-09-09T16:51:20.6801835Z`, preserving all four user
configuration files and verifying the DLL/receipt and 34 UI files. Package
verification also matched 60 installed static files and 64 payload files in
each manual archive.

The three SG-10 archives replaced `dist/final-3.0.0` at
`2026-09-09T16:55:11.2153219Z`; their entry counts are 4/69/73. The previous
SG-03 final set is preserved in the SG-10 candidate's `previous-final-sg03`
directory. [Release status](docs/RELEASE_STATUS.md) records the exact build,
deployment and package receipts. Source and delivery verification do not
establish game visual, input or performance acceptance; those remain pending
owner testing. Nexus material is a local draft only; nothing was uploaded.

## Preserved SG-10 confirmation scope

- **Endorse and Feedback are separate entries.** Each opens a blue-gray UMG
  confirmation panel. Only Yes emits the corresponding command to open this
  mod's Nexus page or its feedback/posts page. Neither entry submits an
  endorsement, post or report on the owner's behalf.
- **Restore Preset also requires Yes.** Confirmation uses the existing global
  display preset; runtime power and startup hotkeys remain unchanged. No or
  Esc dismisses only the confirmation and does not change settings or open a
  website. F6 remains open. Focus loss closes the entire F6 panel and clears
  pending confirmation; F6 exit and travel also clear the intent.
- **Confirmation is consumed once before effects.** A fresh neutral input
  sample must precede Yes. While the panel is active, its input shield and
  disabled background controls prevent settings interaction; a dismissal
  guard skips background sampling on the following service turn. The modal
  uses native UMG and does not block the game thread with a Windows dialog.
- **All eleven languages have confirmation text.** The same eleven tooltip
  atlases now contain 32 help topics plus seven confirmation tiles each;
  the 34-file UI inventory and 44 main text slots remain unchanged. Missing
  or obsolete atlas data leaves a cancel-only fallback with Yes disabled.

The pure confirmation model passes **55 checks**, including held/repeated Yes,
No priority, all three intents and lifecycle clearing. Independent source
review and English/Chinese source-derived panel previews are complete. These
checks do not prove actual Slate click capture, focus transitions or in-game
composition; those remain owner acceptance items for the installed SG-10
artifact. The final delivery receipts are retained under
`dist/work/candidates/radar-3.0.0-sg10-20260909` and indexed in Release status.

## Inherited SG-09 implementation

1. **Settings material:** soft blue-gray translucent surfaces, restrained
   gradients and readable text/status plates replace SG-08 charcoal. Preserve
   the approved four-card layout, square settings checkboxes, eleven languages,
   existing help topics and file inventory. No live blur or refraction is added.
2. **Area Quest Scene icon:** retain the enlarged open gray diamond, three
   white dots and small direction tip; add a subtle translucent gray-blue
   interior backing. Chest and flag textures keep their existing appearance.
3. **Treasure Scene height:** increase only the projected Treasure point from
   +100 to **+160 cm**. Area Quest/Mini-game lifts remain +180/+150 cm. Raw XYZ,
   distance, range and minimap/map height logic do not change. This uniform UI
   clearance adjustment does not measure terrain or mesh bounds and cannot
   establish correct alignment for every chest.
4. **Clock placement:** use the existing **1 Hz** layout service and the exact
   native minimap RetainerBox/`DLayerQuest` geometry. Center the visible glyph
   between minimap bottom and task top, accounting for its 15-unit visual center
   inside the 42-unit container. Invalid/hidden geometry, insufficient gap or
   screen/host bounds use the **178-reference-unit** fallback from minimap
   center. No widget scan or world-time provider change is introduced.
5. **Live F6 preview:** Compact Radar and Scene may remain visible while this
   settings panel owns a cursor opened over visibly active gameplay. Real menus,
   pause, world map, hidden HUD and activity guards still suppress them. Routine
   menu sampling and settings edits retain the renderer tree.
6. **Scene motion and bounded work:** project the retained selection each engine
   frame, independently of the 16 ms player-position sample. Catalog selection
   stays at 250 ms; each frame updates at most 50 selected scalar distances.
   Move markers with render translation and cumulative 0.25-physical-pixel
   caching instead of repeated Canvas layout changes. No measured FPS or
   frame-time improvement is claimed.

The inherited Scene choices remain independent Treasure/Area/Mini-game switches,
shared 0-1000 m range and 0-50 maximum marker count, with defaults all on,
600 m / 24 and Aim Focus. Explicit saved choices survive upgrades. Zero range
or count hides Scene. Aim keeps the 16%/34% short-side ellipse and 120 ms initial
dwell. Aim/Auto replacements require a continuous 350/500 ms advantage while
the old visible label remains. Both require over 20% improvement and a small
absolute advantage; acquired Aim targets receive a 1.2-times exit margin.
After confirmation, Reset to Defaults in the bottom action row resets display
preferences, filters and language; runtime power and startup hotkeys remain
unchanged. Scene uses six shared glyph
textures and a fixed pool of 50 marker Images plus six hidden texture keepers.

143 Area tasks have verified whole-XYZ Scene anchors; four unconfirmed anchors
remain omitted from Scene only. Compact Boss/Assault/Area sizes remain 35/30/25
with a common stroke, corrected player Z and inclusive same-level thresholds.
The original distance offsets and zero-before-rounding rule remain intact.
Details and historical experiments are in the
[Scene guidance ledger](docs/SCENE_GUIDANCE_ATTEMPT_LEDGER.md),
[3.0 release plan](docs/RELEASE_PLAN_3_0_0.md) and
[acceptance checklist](docs/ACCEPTANCE_CHECKLIST.md).
Nexus copy is indexed in `assets/nexus/README.md`.

## Preserved SG-09 deployment and earlier 3.0 checkpoints

SG-09 was built and installed on September 9. Release status records Core
6/6, the clean native build, the matching installed DLL/receipt and all 34 UI
resources, with four user configuration files preserved byte for byte. Its
deployment supersedes the SG-08 installation below. SG-09's generated package
candidate is separate from promotion of the final archives, and neither
transaction identifies the SG-10 source or establishes owner acceptance.

SG-08 was installed on September 9 after the owner's explicit request. Its DLL,
receipt and 34 UI resource files matched the retained candidate; visibility,
hotkeys, diagnostics and treasure overrides were preserved byte for byte.
`dist/work/candidates/radar-3.0.0-sg08-20260909/deployment-verification.json`
records that transaction and supersedes SG-08's earlier undeployed snapshot.
The complete preceding SG-05 installation was backed up. That receipt verifies
SG-08 installation only; it is not an SG-09 receipt or game/visual acceptance.

SG-08's neutral-charcoal material, SG-07's 31 setting-specific tooltips,
SG-06's page-wide preset/enlarged task diamond and SG-05's retained sprite pool
remain dated history in the release plan. SG-06 moved Clock down 16 reference
units; SG-09 replaces that fixed correction with measured gap placement.
The SG-07 audit found matching static chest catalogs and a direction tip 16
logical pixels below its projected center. Its then-current +100 cm lift and
unconfirmed mesh-top alignment are historical findings, not today's lift.
The audit remains at repository-root `out/handoff/SG07_TREASURE_HEIGHT_AUDIT.md`.

Historical SG-03 local handoff: `dist/final-3.0.0` contains three verified
archives and their manifest/checksums. Core 5/5, Setup 20/20 and Manual 2/2
passed; independent archive checks passed 1,548/1,548. At that checkpoint the
same DLL and 35 non-user payload files were installed, preserving all four
existing user files byte for byte. Its deployment backup remains
`dist/work/deployment/deploy-backups/20260908-220537-090-native-only-deploy`.
SG-03's exclamation task icon, 10% circular Aim and default-off Scene controls
are historical behavior, superseded by later source changes. No external
publication or owner gameplay acceptance follows from those archive checks.

## Ownership

`DragonSwordNativeWorldRadarPostRender` is the active native-only successor to
DragonSword World Radar. Its source root is this directory. Its installed root
is:

`DS/Binaries/Win64/ue4ss/Mods/DragonSwordNativeWorldRadarPostRender`

`DragonSwordWorldRadarObjectState` is a superseded external-renderer baseline
available through Git history, not a current working-tree project. Do not
modify or deploy other Mods from this project.

## Historical 2.3.0 package work

2.2.2 was not published. On 2026-09-07 the owner promoted its flight/language
work plus configurable hotkeys to 2.3.0. `config/hotkeys.ini` is a standalone
startup-only file for settings/enable/disable; F6 saves must never rewrite it.
Setup reads existing keys and allows an explicitly confirmed new set on
Install / Update / Repair. Preserve unchanged key-file bytes and all other
user settings; public defaults remain F6/F7/F8. The 2026-09-07 installer-only
follow-up does not change the accepted/native-test candidate DLL.

At that time, the owner assigned all unpublished follow-up work to 2.3.0.
Runtime, installer, metadata, package tools, and source gates were aligned to
2.3.0. The published 2.2.1
package set is unchanged; do not relabel historical artifacts or receipts.
Those historical build/deployment identities, scope, and acceptance boundaries
are tracked in `docs/RELEASE_PLAN_2_3_0.md`.

On 2026-09-07 the owner requested the three local 2.3.0 packages. The preceding
CM-04 set is now in `dist/final-2.3.0`, with Setup 20/20, Manual 2/2 and
byte-identical archive/payload checks passed. The installer-hotkey follow-up
supersedes the initial set (manifest UTC `2026-09-07T11:00:51.0231580Z`): Setup
now offers three editable key selectors for Install / Update / Repair; parser
and editor checks pass 177 assertions. The previous five files are preserved
in `dist/work/candidates/cm04-before-installer-hotkeys-20260907`.
It contains the same CM-04 DLL
as the controller-tested installation, with public debug off and AUTO/F6/F7/F8
defaults. Older `dist/work/candidates/hotkeys-2.3.0` files are superseded for
delivery. This is local packaging, not upload, full runtime acceptance, or
third-party publication-rights clearance; see the 2.3.0 plan for exact hashes.

### Owner-authorized local deployment workflow (2026-09-07)

The owner requested installation of CM-04 and automatic local installation
after subsequent Radar fixes in this development workflow. After a successful
build, matching receipt and relevant regression checks, deploy through the
product's backup/rollback-capable script with `-Diagnostics Preserve` without
asking for installation approval each time. Preserve current language,
visibility, hotkeys, diagnostics and treasure overrides byte-for-byte.
Never overwrite a running game's mod or terminate the game automatically;
if it is running, wait for the owner to close it before installation. This
authorization does not cover game launch, save changes, other mods, public
release/packaging, Git commits/pushes, or acceptance claims.

## Inherited WM-07 and CM-04 behavior (historical 2026-09-06/07 evidence)

The 3.0.0 source retains the fix to 2.2.1's initial world-map
attachment gate. Raw PlayerIcon motion previously exhausted all three attempts
while flying. The new attach-only sampler compares the motion-compensated
world-to-Canvas origin and parent extent using the same attempt's player
coordinates. It requires two valid samples with matching layer, FogAbove,
PlayerIcon, and owning-controller weak identities and matching map metadata.
The generic extent sampler and WM-06 immutable retained placement are unchanged.
`WORLD_MAP_ATTACH_PROJECTION` records inputs and separate deltas only during
bounded attachment attempts. The earlier local 2.3.0 packages include this fix;
the version label alone does not identify a developer candidate. Exact historical build,
deployment, and runtime status are appended to `docs/WORLD_MAP_ATTEMPT_LEDGER.md`.
The owner reported that preliminary testing found no issue. The current local
log independently confirms moving-player attachment on attempt 2/3 with a
compensated-origin delta of 0.052545. This does not fill the entire runtime
matrix. The next issue is compact Radar remaining visible in the controller
start menu/submenus. On 2026-09-07 a virtual Xbox controller reproduced leakage
in both the main menu and Hero information page, confirmed by the owner.
CM-03 proved hidden native paint ancestry while the local minimap stays Visible.
CM-04 implements this additional compact-only guard at the existing 250 ms edge;
two virtual-controller Start/Hero return cycles, including Hero Skill, passed
without settled-menu residue on 2026-09-07. Full runtime acceptance remains
pending; exact evidence and untested routes are in
`docs/CONTROLLER_MENU_ATTEMPT_LEDGER.md`. CM-04 was installed for owner testing
at that time; the preceding CM-03 diagnostic and logs remain in its deployment backup.
WM-09 records Radar-on/F8-disabled map-close comparisons: similar 0.7-0.8 s
transitions, with no additional close stall reproduced in the tested scene.
Persistent follow-game language is implemented in 2.3.0;
its runtime acceptance is tracked separately in the release plan.

## Historical 2.2.1 baseline (not 3.0.0 acceptance)

- Version: current fixes-only release candidate `2.2.1`.
- Runtime label: `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1`.
- Version `1.2.0` was never published; its candidate work is included in
  `2.1.0` and has no separate public artifact line.
- Package shape: UE4SS native `main.dll`, immutable generated catalogs,
  SQLCipher runtime library, configuration, notices, and metadata only.
- Version 2.2.1 is a regression-fix-only inner-atlas-layout candidate. Both Mod-owned, hit-
  test-invisible hosts attach to the current `DLayerMap.FogAbovePanel`, resolved
  directly from the current layer. `ArrayIconInfo` supplies an instantiable
  native icon class only when a host must be created; it never selects the
  parent, and steady retained-host validation/refresh does not scan it. Each
  outer host slot is full stretch with zero offsets, `AutoSize=false`, alignment
  `(0,0)`, and maximum Canvas Z. At each fresh attachment, including a scheduler-
  accepted bounded rebuild, the cloned host's inner `Panel_Point` slot is forced
  to full stretch with zero offsets, `AutoSize=false`, and alignment `(0,0)`.
  Only that fresh attachment reads `PlayerIconWidget` and assigns
  `{atlas_left,atlas_top,atlas_width,atlas_height}` to each Image Canvas slot;
  retained refresh never rewrites it. Image render translation remains `(0,0)`.
  No host render transform or forced layout prepass is used.
- Zoom-topology diagnostics explain the superseded failure: native zoom rebuilt
  icon widgets, the first valid icon parent alternated between `FogAbovePanel`
  and `FogUnderPanel`, and the heuristic caused four Mod-host reattachments in
  one zoom sequence. That produced fog occlusion, hitching, and flashing. These
  temporary diagnostics support the source correction but do not validate its
  gameplay behavior.
- The direct-`FogAbovePanel` full-stretch-outer/Image-translation candidate was
  built and deployed at DLL
  `CCC6B1170BAD1BF94AE5149DE52B48E2E11553747016E10299423BFD0C06AE00`
  from compiled source
  `B650B5FBD731EC0BC24D43F2256A1826E403D164353DFC5B684AC943FAD174EA`.
  Live 2026-09-05 screenshots reject it: zoom-in measured base-map/Radar scales
  near 1.214/1.218 with relative translation about `(+113,-190)` px, while the
  reverse zoom measured 0.760/0.758 with about `(+67,+200)` px. Mean fitted
  residuals were only 0.28 px across 32 points and 0.82 px across 35 points.
  Equal scale but sign-reversing vertical translation proves a local-origin/
  zoom-pivot mismatch rather than scale-formula error or cumulative frame drift.
  The previous DLL
  `FE811E81B741E2F9FA849DB3FF208CDC567187BD3E77F5A6C440E80BB7B82A21`
  from compiled source
  `45EF8C60E46427DC66FC13C16FFCA1BBF11FC0A78E5EC68753ECDD46A6339CA3`
  and backup
  `dist/work/deployment/deploy-backups/20260905-053728-726-native-only-deploy`
  belong to the superseded first-valid-parent candidate and are not gameplay
  evidence for this correction. The `CCC6B117...AE00` build's complete
  predeployment and rollback-backed deployment records, including backup
  `dist/work/deployment/deploy-backups/20260905-092836-495-native-only-deploy`,
  likewise remain rejected-candidate evidence only. The later outer-atlas-
  rectangle build, exact DLL
  `CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2` from
  compiled source
  `433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`, is
  also runtime rejected. Its negative atlas-left outer slot changed the native
  parent extent from `3000x3000` to `3191.520996x3000`, producing a self-induced
  6-attach/5-detach rebuild loop, flashing, and an empty map. The same run had
  1,632 populated markers and no data, texture, or ABI fault. Its rollback backup
  `dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`
  remains exact-byte historical evidence only. The later dynamically rebased
  inner-atlas DLL
  `5C632820CC44065AB9FEDDA260A392B5C72655D3815F445C73CF4A50A72DD606`, from
  compiled source
  `798297BBF7F9791A9E2FFBFF4EC5894A62CC4B16B825CB6BAEBC19A398CB2585`, is also
  runtime rejected. Six same-parent `PlayerIconWidget` anchor observations
  rewrote its Image placement and account for approximately `(+92,-915)` pixels
  of screen displacement. Its deployment backup
  `dist/work/deployment/deploy-backups/20260905-191717-084-native-only-deploy`
  remains rejected-byte evidence only. See
  `docs/WORLD_MAP_ATTEMPT_LEDGER.md`.
  The first immutable-placement source revision passed source review, static
  gates, Core `2/2`, F6 overlay, compact, native world-map, PostRender, release
  hygiene, and a local native build at DLL
  `A5CEBAECAD75E8AE5C8BC2435625CD52BEE878AA656C1D8EA1C58C5C24542EDB`, from
  compiled source
  `72BD98D3AA612D4F902D901B4174C4340BE325A54A2015E8B88DEACE738862A1`, size
  1,107,968 bytes. It was superseded before deployment by a follow-up source
  change that removed visibility reconciliation from retained refresh, so it is
  build evidence only and not the final candidate. Current WM-06 source review,
  static gates, Core `2/2`, release hygiene, native build, and rollback-backed
  diagnostics-enabled developer deployment pass for DLL
  `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`, from
  compiled source
  `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`, size
  1,107,968 bytes. The installed identity matches exactly, the controlling
  `mods.txt` has one Radar entry, and `debug_logging=true`. Rollback is retained
  at `dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`.
  Package and installer validation pass for the exact WM-06 bytes: Setup
  `20/20`, Manual `2/2`, payload equivalence, layout, clean-target, and all three
  archive re-extractions. Gameplay, visual behavior,
  resolution, controller, responsive-layout, localization-glyph, clean-exit,
  and performance acceptance remain `PENDING` or `NOT_VALIDATED` as recorded
  below. Public publication remains blocked by the recorded provenance and
  rights reviews.
  No 2.2.0 hash or result may be relabelled as 2.2.1 evidence. The current matrix is
  `docs/RUNTIME_FEEDBACK_AUDIT_2_2_1.md`.
- The exact 2.2.0 release remains historical evidence. Its source-bound
  `main.dll` SHA-256 was
  `6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`, its
  compiled-source SHA-256 was
  `A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`, and its
  Core `2/2`, static gates, `/W4 /WX` build, Setup `20/20`, manual `2/2`, package,
  archive-integrity, and local diagnostics-enabled deployment results remain
  bound only to those exact 2.2.0 bytes. Its rollback backup is
  `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
  Those results do not validate the 2.2.1 world-map ownership correction.
  The prior 2.1.1
  package set remains
  historical evidence under `dist/final-2.1.1`; its packaged `main.dll`
  SHA-256 is
  `B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
  That hash must never be relabelled as 2.2.1. Detailed scenario evidence and
  publication rights remain independent.
- No custom executable, PowerShell host, watcher, C# Overlay, or Lua script is
  part of the installed native mod.
- F6 opens the native responsive settings page while Radar is Off, On, or
  Faulted. The top bar contains separate Bug Report and Close controls. Mod
  Status is a read-only long text display with a thin state-colored strip;
  Enable, Disable, or Retry is a separate action, and using it keeps the page
  open. Enable still requires a loaded playable world. Bug Report opens the
  fixed Nexus Posts URL. F7 activates. F8 disables, and F8 followed by F7
  explicitly resynchronizes. The one-page layout contains Mod Status, Language, Marker
  Visibility, Height Indicators (Radar Only), Filter Modes, and Bug Report. It
  scales and clamps to the live viewport. Translucent section cards, equal-width
  filter choices, corrected text alignment, and bounded non-overlapping hit
  regions are presentation-only. All three height indicators default
  ON on a clean install, while a valid existing config keeps its choices. F6 also owns
  one persisted `AVAILABLE` / `ALL` area-quest display mode.
  `AVAILABLE` preserves prerequisite-proven filtering; `ALL` removes only that
  display filter and still excludes save- or runtime-confirmed completions.
  A separate persisted Assault `AVAILABLE` / `ALL` display mode defaults to
  `AVAILABLE`. `ALL` is a presentation-only static catalog view of all 40
  Assault records and bypasses save readiness, time, defeat, and cooldown only
  for drawing. `AVAILABLE` retains every live gate; actual completion/cooldown
  authority, Boss selection, and area-quest logic are unchanged. Both modes reuse the existing fixed
  selection passes and open-only Hub service, with no added timer, SQL request,
  object scan, or steady-state work.
  F7 re-reads bounded runtime and save-backed state; it does not run Setup,
  replace files, extract PAKs, or regenerate immutable catalogs.
  In 2.3.0 the centered language selector exposes `AUTO (Game Language)` first,
  followed by 11 explicit UI languages. AUTO persists;
  each actual F6 opening or F7 activation samples `DGameUserSettings.LanguageText`
  with a bounded Kismet fallback. Invalid samples retain the last valid detection,
  with English only before the first valid sample. Explicit persisted languages
  remain authoritative and detection never overwrites the preference. The 11 UI languages are English,
  Japanese, Korean, Simplified Chinese, Traditional Chinese, French, German,
  Spanish (Spain), Russian, Thai, and Portuguese (Brazil). No per-frame or
  recurring language/font work is added. F6 selects already-loaded game Font
  objects by script: `DsCompositFont_CommonSystem` for Korean and Latin/Cyrillic
  UI languages, `DsCompositFont_TCSystem` for both Chinese choices,
  `DsCompositFont_JPSystem` for Japanese, and `DsCompositFont_THSystem` for Thai.
  Missing/expired weak identities are retried only on a real F6 open. The
  external `DS_HYFont_P.pak` overrides Common/TC without complete glyph
  coverage, so localization QA requires disabling or replacing it. Raw
  Pretendard FontFace assets are not bundled or routed as `UFont` substitutes.
  Missing font evidence falls back safely without changing the explicit
  language or guessing an asset path or FontMaterial. An exact zero `Font.Size` receives one
  bounded reference size. Floating-point `Size` and integer `LetterSpacing`
  are handled through their matching reflected numeric APIs. A game-widget
  construction, target-size, or
  font-commit failure retries that text once as base UMG `TextBlock` during the
  same F6 transaction. If both exact-size paths fail, a fresh DTextBlock then
  base TextBlock leaves Font untouched and uses the bounded viewport/DPI render
  scale with a justification-aware pivot; that emergency path is not accepted
  visual output. Exact sizes are written only into the `SetFont` parameter copy
  and verified from the widget after the call. Core UMG creation, `SetText`,
  tree insertion, and viewport attachment remain fail-closed.
  Korean and Traditional Chinese fixed labels additionally use generated 2x
  overlays from pinned DroidSansFallback. Their canonical payload is
  `assets/ui/f6/{ko,zh-hant}-{off,on,fault}.tga`,
  `assets/ui/f6/language-popup.tga`, and `assets/ui/f6/manifest.json`. The six
  status-specific main overlays replace all 30 fixed main-panel text slots for
  ko/zh-Hant; the shared popup overlay replaces only those two language names.
  The other nine languages continue through the native game-font path. The
  overlays are regenerated against the current top-bar, status, and filter-row
  coordinates: Bug Report `(411,17,126,26)`, Close `(559,17,94,26)`, status
  label/value/action `(32,142,98,24)` / `(158,142,154,24)` /
  `(435,142,208,24)`, and filter text at X `334` / `496`, width `146`, Y `583`
  / `615`. Bug Report uses role scale `0.40` and tight-alpha two-axis centering.
  The generator uses base size 32, a one-pixel translucent stroke,
  and role-specific optical baselines. Static verification confirms the overlay
  slots do not clip. The strict verifier audits all 11 runtime text blocks,
  covers `123/123` overlay codepoints, reports minimum fit `1.000`, maximum
  optical-center error `0.5` raster pixel, no edge alpha or slot overflow, and
  deterministic `8/8` regeneration including the manifest. Live in-game size, weight, and alignment remain
  `NOT_VALIDATED`.
  Developer
  deployment and recognized Setup
  Update / Repair preserve all three validated user-owned files byte-for-byte:
  `config/visibility.ini`,
  `config/diagnostics.ini`, and `data/defaults/treasure_overrides.txt`.
  Non-open-world activities suppress both renderers; open-world interiors
  remain eligible. For task-class mapping, an already-active explicit F7 may
  rearm a failed capture, but that mapping-only action preserves
  exact-completion bits and revision/reactivation fences.
  The exact `World /Game/Title/TitleMap/DS_Title.DS_Title` identity is not
  ordinary activity suppression. It is the save-owner hard boundary: the
  runtime disables, detaches both renderers, clears weak candidates and every
  mutable save-derived/process-local delta, and latches activation off. A later
  save load remains disabled until the player reaches an open world and presses
  F7 explicitly; F7 on the title screen or during an incomplete load is
  rejected.
- The compact host samples viewport size and DPI on the existing one-hertz
  minimap-scale service. Only a real geometry edge updates its top-left origin
  and render scale, so window/fullscreen changes move the radar and clock
  together without touching the 16 ms motion path. Deterministic tests cover
  numeric viewport/DPI inputs including 3440x1440, 3840x1600, 2560x1080,
  ordinary windowed sizes, and DPI changes. They do not prove a 3840x2160
  viewport with an internal 21:9 content rect, native 21:9, windowed client
  geometry, or the game's runtime Slate layout. Each fresh expanded-map
  attachment, whether initial or a scheduler-accepted bounded rebuild, samples
  the live `PlayerIconWidget` alignment pivot through its current Slate cached
  geometry and `LocalToAbsolute`, then through the exact `FogAbovePanel` cached
  geometry and `AbsoluteToLocal`. `WorldMapUISize` is authored metadata, not the
  parent extent. Initial attachment has a separate bounded three-attempt
  readiness service and retains only numeric observations.
  Missing, implausible, or unstable geometry fails closed; no centered fallback,
  desktop-resolution substitution, or new polling schedule is published.
- The 2.2.1 candidate uses the directly resolved current
  `DLayerMap.FogAbovePanel` as transform, clipping,
  visibility, and RetainerBox owner. The two Mod hosts are hit-test-invisible
  children whose outer Canvas slots are full stretch with zero offsets,
  `AutoSize=false`, zero alignment, and maximum Z. Their cloned inner
  `Panel_Point` slots are also forced to full stretch with zero offsets at each
  fresh attachment, including a scheduler-accepted bounded rebuild.
  `ArrayIconInfo` supplies only a creation-time
  icon class and is not scanned during retained-host validation/refresh. Only
  their atlas Image Canvas slots hold the fixed
  `{atlas_left,atlas_top,atlas_width,atlas_height}` rectangle; Image render
  translation remains `(0,0)`. No host render transform or forced layout
  prepass is used. Same-parent pan and zoom therefore require no transform-sync
  write, atlas rebuild, rerasterization, or marker reprojection. Every retained
  event-tail pass, including the final pass, re-resolves `FogAbovePanel` and
  reads only that exact owner's live local extent. While parent identity and
  extent remain unchanged, attach-time Image placement is immutable and refresh
  performs no layout, transform, widget-tree, or RetainerBox write. A parent
  replacement reports `RebuildRequired`; refresh never reparents or delta-
  rebases retained content. Parent-extent drift must produce two matching
  successful observations before it can return `RebuildRequired`; retained-
  RetainerBox replacement or owned-
  payload replacement/invalidity may report the same result directly. The
  request itself is report-only and does not pre-collapse a valid payload before
  the scheduler accepts it. Capacity remains
  4,096. The accepted ceiling is 2,500 Treasure rows plus
  279 fixed non-Treasure rows, or 2,779 total, leaving 1,317 spare slots. The
  observed 1,632-marker/1,501-Treasure snapshot was below the old 1,785 limit,
  so capacity was not the dense-map flicker root. The rejected CD41 log instead
  records 6 attaches and 5 detaches caused by its own outer-slot extent change;
  rejected 5C632820 then records six same-parent Image-slot rewrites caused only
  by changing player anchors. A5CEBAEC immutable-placement bytes passed a local
  build but were superseded before deployment by the post-A5CE refresh cleanup.
  Current 6435E100/0A1A4CE3 bytes pass build and exact-identity rollback-backed
  developer deployment; gameplay remains `NOT_VALIDATED`. Two 2048-by-2048
  atlases use
  about 32 MiB raw when decoded as BGRA. Cache envelope `DSNWRA52` quantizes the
  normalized fingerprint at 1/4096 UMG logical unit. A hit requires exact
  dimensions, header, magic, fingerprint, and visible count; complete RLE decode
  to exactly 2048-by-2048 pixels; a valid encoded-payload checksum; and payload
  termination exactly at EOF. Revision-51, corrupt, truncated, or trailing-byte
  files miss automatically. Writes use a same-directory temporary file and
  atomically publish with `MoveFileExW` replace-existing and write-through flags;
  failures remove the temporary file. Cache reads/writes and texture import
  remain bounded attachment work. Marker coordinates and capacity remain
  unchanged. Expanded-map alignment, dense-Treasure behavior, 2048 visual
  quality, memory, and attach time remain pending exact-build live acceptance
  (`NOT_VALIDATED`).
  The current F6/localization/compact-indicator closeout does not modify any
  expanded-map source, atlas geometry, capacity, coordinates, projection,
  ownership, or style-revision path.
- Attach, `SetWorldMapImage`, F7 resume, and exact zoom events arm a finite
  five-deadline tail at 100, 250, 500, 1,000, and 1,250 ms. Each due game-thread
  pass revalidates the exact native parent and reads only its live extent.
  Attach uses a fresh completion timestamp for this tail and does not submit an empty
  Retainer `RequestRender` before visibility is applied. A fully
  unchanged same-parent and same-extent pass, including the final pass, keeps
  Image placement immutable and performs no layout, transform, tree, or
  RetainerBox write. Parent replacement reports `RebuildRequired` without live reparenting
  or delta-rebasing. Two matching successful extent observations are required
  before extent drift returns `RebuildRequired`; a rebuild request
  leaves valid payload visible until scheduling succeeds. At most one rebuild is
  accepted per open-map
  session. It preserves the marker snapshot and receives its own hard-capped
  three-attempt attach/geometry budget, so the whole session is bounded to the
  initial three attempts plus the rebuild's three attempts. Missing or invalid
  ownership remains bounded and fail closed. Mod-owned payload, ABI,
  and guarded runtime failures still detach. This correction introduces no new
  marker recollection cadence, texture-import cadence, or steady poll; bounded
  persistent-cache file access occurs only at explicit atlas attachment.
- The compact pool owns fixed Treasure and shared mini-game height groups and evaluates Area
  Quest height in the already bounded 80-slot marker pass. The nearest treasure
  retains its unchanged category-colored six-piece full shafted pointer to the
  left of the selected chest. Every
  visible Area Quest evaluates a generated one- or two-band height profile. A
  multi-band profile uses authored marker Z to choose the uniquely nearest
  existing source band; marker Z is not a synthetic height source. The normal
  black frame keeps three white dots while the comparable player Z is inside the
  selected band expanded by the inclusive +/-500 vertical-unit margin. A player
  below that band gets an upward triangle and a player above it gets a downward
  triangle. An exact-distance tie or missing profile keeps the frame with no
  dots or false direction. There is no separate task arrow. The 147 rows contain 144 height
  profiles, including one genuine two-band profile, and three no-source rows.
  Move_Check-only trigger bands are filtered when a real task-actor band exists.
  Fly, Mole, and Wave share one nearest-mini-game channel. A shaftless triangle
  is centered directly below the selected icon, uses that marker's actual kind
  palette, and adds a near-black outline for contrast. Its size, position, and
  projection are unchanged. A target more than 500 vertical units above the comparable
  player Z shows an up triangle; a target more than 500 below shows a down
  triangle; the inclusive +/-500 band hides it. Treasure, Area Quest, and
  mini-game guidance all compare against `playerZ - 150`. The channel uses the exact
  trusted height from each of the 83 map-100 Fly/Mole/Wave `NPC_Start` actors
  (33/40/10). An unavailable height hides only the mini-game triangle. The
  persisted key remains `mole` for
  compatibility. All three channels may display simultaneously with
  no per-motion allocation or UObject read. The
  numeric clock uses configured presentation bands beginning
  at 06:00, 12:00, 18:00, and 21:00. These are presentation policy, not proven
  game-native phase semantics; weather remains unavailable and unqueried.
- Controller-safe compact suppression does not read controller bindings or
  input-mapping settings. Exact `SetWorldMapImage` delivery latches world-map
  suppression immediately; validated map-layer visibility and optional
  `GameplayStatics.IsGamePaused` state are sampled on the existing shared
  250 ms control edge with edge-only logging. No new timer is added, and the
  16 ms motion path consumes only the published Boolean suppression state.
- Final 2.1.0 adds compact-only bird eggs for exact `Bird_Egg01_C` and
  `Bird_Egg02_C` actors. The creation listener publishes only weak identities
  into a fixed 512-slot pool. The existing 250 ms service performs at most
  eight bounded candidate/position queries per control tick. A candidate is
  available only when its Actor-owned `DInteractableComponent` reports both
  `InteractableValue=2` and `InteractTypeValue=2`. Unknown reflection reads stay
  pending for the same bounded service instead of becoming a false negative.
  Both values are one-byte scoped enums and are read only through their
  validated integer underlying properties;
  exact Bird Egg EndPlay retires the matching weak identity immediately. A
  fixed nearest-16 set receives bounded presence probes on the existing 250 ms
  discovery edge while the independent F6 RADAR category is enabled. Its four preallocated pieces use a
  native rounded-box brush and narrow geometry to render a vertical oval; no
  widget or schedule is added.
  The MAP cell is unavailable. There
  is no new polling, UObject enumeration, SQL, retained raw Actor, dynamic
  queue, filesystem polling, or expanded-map work.
- Dev56 adds one native dynamic-quest event hook, exact live-`PROGRESS`
  validation, per-task ten-second numeric completion witnesses with one
  non-replenishing follow-up budget, first-valid-hour task refresh, unique
  positional linking for `MONSTER_ALIVE value1=0`, bounded non-replenishing UMG
  soft-attach retries, and a transient native F6 category Hub. Generic
  interaction never arms task completion, and the closed Hub performs no
  UObject work. Pure numeric task witnesses retain their original ten-second
  deadline across travel, while F8/F7 clears them. Hub cleanup restores input
  through the panel's owning Controller.
- Dev57 makes Hub selections auto-apply only on a real packed-mask change,
  replaces Apply/Cancel with a top-right `X`, compacts the visual hierarchy,
  orders Game-and-UI mode before the cursor write, and conditionally restores
  the cursor only when the game hides it during the existing open-only 50 ms
  service. The closed path remains free of Controller/UObject/file work.
- Dev58 places both expanded-map radar atlas hosts above game-native icons with
  adjacent deterministic radar-internal Z orders. It also collapses the compact
  host on the first failed current-Pawn sample while reading cursor state from
  the same current Controller before Pawn resolution. It adds no polling,
  retry, enumeration, or UObject retention.
- Dev59 changes only the F6 Hub presentation: framed panel, column header chips,
  content surface, alternating rows, toggle contrast, and close-control styling.
  The existing aspect-ratio-aware viewport scale, live UMG DPI conversion,
  centered placement, auto-apply logic, and open-only 50 ms service remain.
- Dev60 fixes the observed high-resolution TextBlock/panel DPI mismatch by
  applying the same open-time unit scale to every text render transform from a
  top-left pivot. It also separates title and column headers and darkens the
  linear-space palette. Both new reflected functions are ABI-gated and run only
  during explicit F6 construction.
- Dev61 shortens Hub column headings to RADAR/MAP and layers explicit
  role-specific text multipliers over the shared viewport/DPI scale. It changes
  no masks, input behavior, persistence, renderer path, or service cadence.
- Dev62 changes confirmed non-open-world compact lifecycle from collapse to
  guarded detach and rearms one fresh compact attachment on the open-world
  return edge. Its fault recovery is limited to a prior runtime-only compact
  renderer fault after clean detach; same-call faults and ABI failures remain
  terminal.
- Dev63 adds one process-lifetime automatic engine-tick recovery budget. An SEH
  fault disables immediately; the next tick performs guarded cleanup and may
  reactivate only when the radar was previously active. A second fault, an
  inactive fault, or failed recovery stops with explicit diagnostics and never
  retries steadily.
- Dev63 also permits explicit F7 recovery of a prior runtime-only world-map
  renderer fault and a faulted area-quest scanner, and explicit F6 recovery
  of a prior runtime-only Hub fault. Every path requires clean detach, no new
  same-call fault, and valid reflected ABI.
- Dev64 keeps the process-session stream and 16-line ordinary batching, moves
  compact/world-map state changes out of synchronous game-thread flushes, and
  reserves immediate flushes for critical lifecycle/fault events. The 1 MiB
  current file plus one previous-file rollover remains bounded. The 2.1.0
  corrected release flow builds one exact unsigned ExperimentalNested Setup
  artifact from the source-bound payload; the four-file installer archive and
  any source archive remain separate publication decisions.
- Dev63 release verification binds every installed file to both the package
  manifest and its allowlisted source hash. The native build validates exact
  FetchContent origins/commits, accepts only the pinned UE4SS `fmt` patch, and
  requires `IconFontCppHeaders` commit `210b5a3` through a source override so
  the upstream floating branch cannot enter a release build.
- Dev64 closes the dungeon-return timing hole with the existing UObject-create
  listener. An exact `DLayerMiniMap` is copied into a fixed weak mailbox and can
  rearm one bounded compact attachment when the newly created layer becomes
  available. A distinct replacement identity can rearm once even if the old
  renderer still reports attached or menu-suppressed; duplicate identity
  delivery cannot create a retry loop. Travel clears the active renderer
  candidate but preserves the one weak mailbox only until transition end can
  validate it against the exact new GameMode world.
- Dev64 removes every production Boss/Assault `FindAllOf` path. The listener
  keeps one weak identity per immutable encounter catalog entry in a fixed
  49-slot array. The 250 ms discovery service considers only catalog positions
  inside 100 metres, while strict exact-class `Destroyed` EndPlay recovery is
  available to both Boss and Assault. No dynamic queue, catalog enumeration, or
  16 ms encounter work is added.
- Dev64 contains all SQLCipher row callbacks behind exception boundaries and
  fixed row ceilings. Any malformed, excessive, or allocation-failing result
  aborts only that one F7 snapshot fail closed. Native logging also disables
  itself after a failed stream open/write/flush instead of repeatedly operating
  on a failed stream.
- Dev65 separates encounter disappearance from treasure disappearance. Boss and
  Assault observation stores the actual live actor position, arms only after at
  least four present samples spanning one second, and accepts a non-exact
  completion only after forty missing 250 ms samples spanning ten seconds while
  the current player remains within 100 metres in a stable open-world,
  cursor-hidden context. An already observed `RemovedFromWorld` Actor is
  released immediately but its numeric observation may continue through that
  same ten-second gate; the EndPlay reason itself is never completion evidence.
  Travel, activity suppression, menus, leaving range, and reappearance cannot
  complete an encounter. Exact
  current `Destroyed` remains the fast path. Range eviction also clears the
  processed weak identity so the same still-live actor can bind again on return.
  Candidate consumption and the final completion write both recheck current
  availability on this conservative disappearance route; any rejected
  completion race clears the identity as well. This recheck does not apply to
  an exact death event already accepted into the R7 handoff.
  Confirmed activity entry clears encounter observations and processed
  identities at the suppression edge without depending on a valid Pawn sample.
- R4 makes the optional
  `/Script/DS.DsFieldCharacter:NetMulticastNotifyDeath` pre-hook the primary
  encounter-completion signal. The receiver must be the exact weak Actor already
  observed for the current activation and epoch, exactly match its immutable
  catalog class, be a `DsMonsterCharacter`, remain currently available and
  visibly seen, and be within 100 metres of the current player. The callback
  only sets its catalog bit in a fixed 49-bit atomic mask. In R7, ordinary
  consumption on the existing 250 ms control service requires a valid context
  but does not recheck the event's already-proved time window. Successful bits
  clear individually after the 120-minute cooldown and render-state application;
  an apply exception retains the affected bit. F7, disable, travel, and
  activity-suppression boundaries may settle pending bits numerically without
  renderer mutation before reset. Only process shutdown and UObject-array
  shutdown hard-clear the mask. This introduces no scan, SQL query, dynamic
  queue, recurring timer, or continuous polling path. Hook lookup or
  registration failure is explicitly optional and cannot make the radar's
  required-runtime latch fail.
- R4 preserves the full dev65 forty-sample/ten-second disappearance fallback.
  A last trusted in-range Actor position is not replaced by a later far pooled
  position, so pool relocation cannot poison fallback proximity evidence.
  Reappearance, range exit, suppression, travel, and unavailable/cooling state
  still reject completion. The R3 moribund approach is not authoritative: the
  target `MonsterCharacterData` records use `UseMoribund=0`, and existing
  diagnostics recorded no moribund completion hit.
- R4 gameplay proved exact Boss and Assault kill completion without F8/F7.
  Maximum-zoom close/reopen nevertheless proved that a game-native child can be
  inserted later at the same `INT_MAX` Z after R4's successful one-shot
  restack, leaving radar content underneath it.
- Historical R5 changed only that expanded-map lifecycle edge. The exact
  `/Script/DSClient.DPanelWorldMap:OnSliderValueChanged` event joins attach,
  `SetWorldMapImage`, and F7 resume as a bounded layering trigger. Each trigger
  started or restarted exactly four settles at 100, 250, 500, and 1,000 ms. Every
  settle re-resolves the current native icon Canvas, then removes and reinserts
  the background and foreground radar hosts in that order at maximum Z. R5
  changes no coordinates, projection, atlas content, marker selection, marker
  geometry, or state-decision logic and added no steady poll. Exact-artifact R5
  acceptance was not completed before the next repair.
- R6 addresses the distinct minimize/restore replacement-layer failure proved
  by the installed R5 log. R5 attached 328 markers to `DLayerMap` serial 3;
  after restore, serial 4 appeared with `map_id` temporarily unavailable while
  a zoom settle still targeted the old retained renderer, producing failure
  103, state 5, detach, and no recovery. R6 requires exact full weak layer
  ownership before zoom or same-layer `SetWorldMapImage` restack work. A layer
  mismatch returns `RetryLater` before same-layer payload validation. The exact
  replacement layer's `SetWorldMapImage` event may consume the existing one-shot
  rearm when the renderer is `Ready` or is still `Attached` to a different old
  layer, retaining the consumed bit and existing bounded three-attempt service.
  A raw incomplete or spurious create-listener candidate cannot detach the old
  renderer. R6 adds no focus hook, poll, timer, enumeration, steady work,
  coordinate, projection, marker, or encounter-state change. Exact-artifact R6
  gameplay and performance acceptance remain `NOT_VALIDATED`.
- R7 preserves the R6 renderer lifecycle and schedules while repairing the
  exact encounter-death handoff described above. It also excludes only
  confirmed nonexistent treasure save ID `11230106` at
  `(182813, 162051, 3150)`. The 1,693-entry render catalog and 1,692-entry actor
  catalog have that ID as their sole set difference; a fail-closed gate binds
  the exception to the exact record and rejects future catalog drift. R7 also
  hardens release and installer boundaries. The handoff repair reuses the
  existing 250 ms service and fixed 49-bit mask and adds no poll, timer, scan,
  SQL, queue, renderer schedule, or steady work. At the historical R7
  checkpoint, source, native-build, installer-matrix, and package gates passed.
  Those older artifact results are not current 2.1.1 `B89F...` evidence;
  exact-artifact
  gameplay and performance remain `NOT_VALIDATED`, and publication remains
  `BLOCKED`.
- R8 preserves the R7 renderer and lifecycle schedules. It adds the exact
  native `DsFieldCharacter.NetMulticastSetDeathProcess` receiver as a second
  Boss/Assault completion route, accepting only reflected process state `End`
  after the same exact observed weak identity, catalog class,
  `DsMonsterCharacter`, activation/epoch, availability, visibility, and
  100-metre gates. An exact dynamic catalog quest event arms its ten-second
  exact-ID witness without a prior `PROGRESS` requirement, but never completes
  the task by itself. Exact `END` completes the witnessed ID. An unresolved
  expiry may queue its fixed bit on the single below-normal save worker for one
  immediate exact-ID confirmation and at most two retries at 15-second
  intervals, but only when F7 established a known per-ID `COMPLETE_CNT`
  baseline. An absent row is zero only for a valid single-owner query;
  otherwise the baseline is unknown and no confirmation SQL is queued. Only
  strict count growth confirms completion. Three non-confirming attempts lock
  the same task generation until a new F7 or a current `NONE`/`END` followed by
  later `ACCEPTABLE`/`PROGRESS`. The bounded path adds no periodic SQL, dynamic queue,
  save-file poll, object enumeration, or motion-path work; exact-artifact
  gameplay and performance acceptance remain `NOT_VALIDATED`.
- Dev65 makes diagnostics startup-only and fail closed. Setup embeds
  `config/diagnostics.example.ini` with `debug_logging=false` as an
  immutable clean-install default but does not write the example file into the
  target; live `config/diagnostics.ini` is user state. Disabled logging returns before event
  formatting, locking, directory creation, rotation, or file I/O. The local
  gameplay-evidence deployment may explicitly enable it without changing the
  release default.
- The current diagnostics document accepts at most 2,048 bytes, uses one
  commented `[diagnostics]` section with exactly one
  `debug_logging=true|false` key like Auto Pickup, and preserves exact legacy
  one-line `event_log_enabled=true|false` files. Enabled records carry one
  schema-2 session header plus exact capture-time `seq`, `utc_ms`, and
  `elapsed_ms` fields. Enabled events use a fixed 256-record queue with
  non-blocking `try_lock`; contention or saturation drops and counts a record
  instead of waiting on gameplay. A below-normal-priority writer drains at most
  16 records per batch, flushes idle partial batches after 250 ms, performs
  critical flushes after dequeue, and owns all file I/O. Numeric Tick telemetry
  is formatted only on that writer and includes `logger_dropped` and
  `logger_truncated`. A 1 MiB current file rotates to exactly one previous file.
  This adds no game query or event cadence. Public defaults remain off.
- The current startup-only visibility document accepts at most 4 KiB and uses
  readable `[radar]`, `[map]`, `[scene]`, `[modes]`, `[height_arrows]`, and `[interface]`
  sections. Every category is a
  named `true|false` key; `area_quests` and `assault` modes accept
  `available|all`; five height keys accept `true|false`, and `language`
  accepts one of the 11 explicit languages or persistent `auto`. Scene adds
  independent Treasure/Area/Mini-game switches, range 0-1000 m, count 0-50 and
  `off|central_radius|nearest_center|all` distance modes. Old readable three- and
  five-section forms, prior Scene controls, and strict packed formats migrate
  with existing choices preserved; missing new Scene settings default to
  600 m / 24 / Aim Focus with new Scene switches on. Explicit saved false
  values remain false. The persisted tokens remain
  `central_radius` / `nearest_center` for backward compatibility.
  AUTO follows the game on actual F6 opening or F7 activation without converting
  to an explicit persisted language. Strictly valid legacy schema 1-4 files remain
  readable for upgrades. F6 applies changes immediately and replaces the file
  atomically in current format only on a real change. Slider writes debounce for
  300 ms and flush on panel close; there is no hot polling.
- Dev65 added a binary-package `Install.cmd` one-click installer. The 2.0.0
  public package retires that entry point in favor of one embedded C# Setup;
  `Build-Release.ps1` is the only release builder. Setup discovers
  Steam app 4570720 or accepts an exact Win64 directory, verifies the complete
  manifest and source-bound payload, performs three stopped-game checks, backs
  up the exact prior target and enablement state, normalizes the native radar
  `mods.txt` entry, removes the obsolete predecessor entry while preserving
  encoding and line endings, and preserves both
  live user configuration files independently, and fully rolls back on failure.
  It never launches, terminates, or automatically elevates the game or itself.
  An active external `DragonSwordWorldRadar : 1` entry or exact
  `DragonSwordWorldRadar/enabled.txt` path in any approved Mods root is a
  zero-mutation conflict. An exact disabled `DragonSwordWorldRadar : 0` entry is
  preserved, and Setup never deletes or disables that external mod.
- Dev66 fixes clustered and XY-overlapping treasure completion at the exact
  `Server_RunInteractV2` event. The receiver `DInteractableComponent` owner is
  resolved before optional `ExecuteTarget*` fields, an existing weak Actor
  observation is reused when available, and bounded exact-class fallback
  matching ranks candidates in 3D. Catalog-only ambiguity still fails closed.
  This path is interaction-only and adds no recurring query, enumeration,
  queue, timer, or frame work.
- Dev67 replaces that compatibility path with the exact
  `DsAnimationProp.NetMultiExecuteInteractProp` receiver Actor, accepts the
  event only when its Actor parameter equals the freshly resolved local Pawn,
  and resolves the receiver's reported `ObjectID` through exact class and 3D
  validation before weak-identity or bounded unique-class 3D fallback.
  `SetDeathProcess` remains an exact-receiver fallback. The work remains
  event-only.
- Version 2.0.0 recognizes the game's mounted underwater identity split: the
  fresh local Pawn is the water mount while the interaction Actor is that
  Pawn's exact callback-local `Rider` UObject. Only a mount-only treasure
  receiver within eight metres, exact Rider pointer equality, and a non-null
  same-World relationship can complete immediately. No Rider, Pawn, receiver,
  or other UObject is retained. Any other rejected non-Pawn event keeps the
  2.0.0 delayed positive-only fallback: it may arm only an exact nearby
  treasure ID. Fixed arrays retain its catalog index, attempt count, and due
  edge only. The existing below-normal worker checks at most 64 exact treasure
  categories after 15 seconds and once more 285 seconds later if needed; it
  skips encounter and dynamic-quest queries and never becomes an idle poll or
  full treasure scan.
- Dev67 gives exact task-actor and exact quest-event evidence a ten-second
  exact-ID verification window. One query may run every 750 ms for a witnessed
  task, with a global limit of one such query per engine tick. Final `END`
  proves the exact task complete even after a previously published `NONE`,
  `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` itself is not completion. The
  fixed 147-entry queue cannot duplicate a live task or grow. Post-completion
  `NONE` or `END` remains only the repeatable-reactivation boundary.
- Dev67 makes expanded-map runtime deltas visibility-aware. A visibly open
  exact attachment may rebuild in the same session; an attached but hidden
  layer is retired and deferred until the next exact `SetWorldMapImage` event.
  That event can rearm the three-attempt readiness budget once for the matching
  candidate serial, including an exhausted three-probe `map_id` transaction
  that never reached a renderer failure code. Duplicate same-layer delivery
  never replenishes work after that one rearm.
- Dev67 removes the static-catalog-distance prefilter from Boss/Assault weak
  candidate resolution. The 250 ms service reads the current actor position
  first, applies the 100-metre player-to-actor bound, and performs at most eight
  actor-position queries per control tick while walking the fixed 49 slots
  round-robin. No enumeration, dynamic queue, or 16 ms work is added.
- The production owner is native UMG only. It connects no PostRender/Present
  hook, external shared mapping, executable, bridge, or Lua runtime. Historical
  canary sources/configuration remain dormant, uncompiled, unread by `main.dll`,
  and absent from the binary runtime payload.
- Public binary publication is blocked independently of technical acceptance.
  The pinned `UEPseudo`/generated Unreal interface has no repository-level
  license in the audited checkout, and the bundled `e_sqlcipher.dll` lacks an
  exact reproducible source/build chain. Redistribution rights, attribution,
  and provenance for the PAK-derived catalogs and MnMRadar-derived coordinate
  table also require review. See `docs/DEPENDENCY_SOURCES.md`.

## 2.0.0 release contract

- The one-click installer targets the ExperimentalNested layout. It requires
  the exact selected DragonSword shipping-executable path
  plus a bounded executable AMD64 PE32+ application image, but does not use a
  fixed game SHA-256 allowlist. The observed game hash is only confirmed
  transaction identity and install-record provenance. Existing nested loader
  and root proxy DLLs are accepted by bounded x64 PE32+ and directory-structure
  validation, not a fixed hash allowlist. Embedded bootstrap/conversion hashes
  remain package-integrity evidence only.
- The packaged save-owner RVA remains the zero-scan runtime fast path. If a
  game update invalidates it, the existing below-normal save worker may scan
  executable PE sections once per process through `min(SizeOfRawData,
  VirtualSize)`, counts only targets inside the mapped image, and requires
  exactly one owner-pointer target. Packaged and structural routes share at
  most 24 active-`.db` key validations; only an authenticated result may be
  cached. No watcher, updater, timer, recurring scan, or game-thread work is
  added. Static catalog changes still require a separate data release.
- Loader absence triggers a transactional, integrity-verified bootstrap of the
  embedded ExperimentalNested runtime. Root, dual, malformed, or incomplete
  layouts enter the explicitly confirmed, fully backed-up conversion path.
  A structurally complete ExperimentalNested layout keeps its loader, proxy,
  settings, and configured Mods paths unchanged.
- The controlling `mods.txt` for the selected layout is the sole load authority.
  The public payload contains no `enabled.txt`. A recognized legacy marker is
  upgrade input to back up and remove, and the obsolete
  `DragonSwordWorldRadarObjectState` line is removed rather than retained as a
  disabled entry. An active `DragonSwordWorldRadar : 1` entry, malformed
  external-radar entry, or exact `DragonSwordWorldRadar/enabled.txt` path in any
  approved Mods root rejects Setup before backup or mutation. An exact disabled
  `DragonSwordWorldRadar : 0` entry is preserved byte-for-byte; Setup never
  deletes or disables that external renderer.
- The recommended package is installer-first and contains exactly the unsigned
  Setup executable, its SHA-256 sidecar, `INSTALL.md`, and
  `THIRD_PARTY_NOTICES.txt`. The isolated installer gate is exactly 20 passed,
  0 failed, and 0 skipped.
- Two additional manual archives target only ExperimentalNested:
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-No-UE4SS.zip` for an
  existing compatible nested runtime and
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` with the
  pinned nested runtime. Their matrix is exactly 2 passed, 0 failed, and 0
  skipped. Both are Nexus-compatible script-free packages whose roots map
  directly to `DS/Binaries/Win64`. No-UE4SS includes the Radar Mod and a clean
  one-line `ue4ss/Mods/mods.txt`; users copy that file only when the target is
  missing and otherwise merge the Radar line. With-UE4SS is clean-target only
  and includes the pinned nested runtime plus clean load control. StableRoot is
  not restored as a supported payload.
- The final directory is `dist/final-2.0.0` and contains exactly
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Installer.zip`,
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-No-UE4SS.zip`,
  `DragonSwordNativeWorldRadarPostRender-v2.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`,
  `release-manifest.json`, and `SHA256SUMS.txt`.
- `Build-Release.ps1` is the supported release entry point and delegates to the
  corrected `Build-Installer.ps1`, the exact 20-case installer matrix, the
  exact two-case manual matrix, and the three-channel archive builder. The retired
  `Stage-Release.ps1` and `Install.cmd` paths create no distributable artifact.
  Setup also exposes one confirmed, token-bound `Uninstall` transaction after
  strict same-product ownership proof. It removes only the exact product tree
  and valid load-control entry, preserves UE4SS, unrelated Mods, `DS/Saved`,
  encoding, and line endings, and rolls back on failure. Setup-embedded
  `.example.ini` resources supply immutable clean-install
  defaults but are not written into the target; live `visibility.ini`,
  `diagnostics.ini`, and `data/defaults/treasure_overrides.txt` are installed or
  preserved. Recognized Update / Repair replaces the bundled DLL and generated
  catalogs, accepts older owned releases, and retains no persistent backup
  after a successful transaction.
- Existing-target replacement requires strict top-level product ownership
  consistent with the package manifest and source-bound payload hashes. Nested
  or unrelated product-name text is not ownership proof and must make zero
  mutation. The proof requires schema-5 release and schema-1 package metadata,
  2-256 unique manifest files, exact size/hash identity for immutable files including
  `metadata/release.json` and a version/label-bearing `dlls/main.dll`, and an
  exact recursive tree containing only manifest files plus the bounded live
  allowlist. Each ownership JSON is at most 4 MiB with depth 16; each owned file
  and the total owned payload are at most 256 MiB. Legacy marker/example files
  are at most 64 KiB, each log 2 MiB, each atlas cache 16 MiB, and an optional
  install record 1-64 KiB with matching version/label. Any extra path or bound
  failure rejects before backup or mutation. The treasure override is the sole
  mutable manifest path and must pass its strict 64 KiB syntax contract.
- Public diagnostics are exactly disabled. A diagnostics-enabled local test is
  an installed-copy-only override whose hash must be recorded separately; no
  installed file or log is a release staging source.
- `SOURCE_VALIDATED`, `BUILT`, `PACKAGED`, `INSTALLER_TESTED`, `DEPLOYED`, and
  `GAMEPLAY_ACCEPTED` are independent. Assigning version 2.0.0 proves none of
  them. Publication is separately `BLOCKED` until all three provenance and
  rights reviews are resolved.

## Implemented foundation

- Dev21-dev29 established compact-map logical coordinates, one preallocated
  moving Canvas root, bounded marker pools, and the nearest-treasure height
  pointer. The 2.1.0 correction added a second fixed height group for the
  nearest area quest, allowing both categories to indicate Z at once. Version
  2.1.1 introduced an independently sourced task-height precursor and a
  distinct black-outline, white-fill shaftless task chevron. Version 2.2.0
  replaces that single-height precursor with the bounded one- or two-band
  profile used by every visible Area Quest marker.
- Dev26-dev30 established the exact expanded-map native parent, map projection,
  texture-brush ABI, inherited pan/zoom/clipping, and F8 removal.
- Dev33-dev44 added the 147 dynamic-area-task catalog, dynamic query source,
  transactional refresh, save completion proof, and fail-closed Main/Group
  prerequisite capture.
- Dev34-dev39 established native treasure/encounter observation and two-hour
  Boss/Assault cooldown. Runtime evidence includes automatic Boss and Assault
  removal. Dev64 replaces the later activation catch-up enumeration with one
  fixed weak slot per encounter catalog entry.
- Dev41 restored one compact host/root after the split encounter tree faulted.
- Dev42-dev48 restored mini-games and refined both-map glyph geometry, layering,
  task styling, height pointer, and clock presentation.
- Dev49 rejected `PROGRESS -> FAIL` as completion evidence.
- Dev50 added one bounded transactional task refresh at each displayed
  game-hour edge.
- Dev52 registered the game-owned task completion event. Dev53 added a bounded
  ID-less confirmation fallback and removed treasure-class game-thread
  `FindAllOf` catch-up.
- Dev54 replaces ID-less inference with one F7/travel reflection of
  `TaskActorClassContainer.DynamicQuestTaskList`. It requires exact one-to-one
  class-full-name coverage for all 147 catalog IDs, hides the mapped ID
  immediately at `OnRecvCompleteQuest`, and retains the generic debounced scan
  only for settled state and repeatable-task reactivation. A fixed per-task
  revision fence prevents an older in-flight scan from reverting a newer exact
  completion. Completion also stays latched through later still-active samples
  until a current scan observes `NONE` or `END`; only a subsequent current scan
  may restore a repeatable task on `ACCEPTABLE` or `PROGRESS`. `FAIL` does not
  arm restoration. Mapping capture has one post-stability attempt and only one
  delayed retry after failure; success or the second failure stops work.
- The asynchronous F7 save snapshot only merges positive dynamic-task
  completion IDs. It cannot clear a newer exact or generic runtime completion;
  only activation lifecycle reset owns full completion-state clearing. A result
  arriving after expanded-map attachment rebuilds once when the exact layer is
  visibly open, or retires a hidden retained atlas until its next exact
  `SetWorldMapImage` edge, rather than preserving pre-reconcile pixels.
- `InitGameState` pre-transition drains any already published exact-completion
  bits into activation-local numeric completion latches and completed quest IDs
  before task-class mapping/bits are cleared. This closes the same-frame
  completion/travel race without accessing or retaining a task actor or any
  other UObject. F8 followed by F7 remains the fresh resynchronization boundary.
- The bounded 49-entry Boss/Assault cooldown map is process-local and is not
  cleared by F8/F7. Save timestamps merge by maximum, so a stale one-shot
  snapshot cannot revive a just-defeated encounter. The existing 250 ms control
  service performs one rate-limited 1 Hz scalar edge check; only a cooldown
  expiry or displayed world-hour change recomputes the fixed 49-bit visibility
  mask and invalidates an atlas when visibility really changes.
- The R7 exact-death handoff owns a second fixed 49-bit pending mask. Ordinary
  consumption requires a valid runtime context and does not recheck an accepted
  event's time window. Applied bits clear independently; an apply exception
  retains its bit. F7, disable, travel, and activity-suppression boundaries may
  settle pending numeric state without renderer mutation. Only safe live-
  GameThread process-lifetime cleanup or the TitleMap owner boundary hard-clears
  the mask; UObject-array shutdown does not. This reuses the existing
  250 ms service and adds no new scheduling or steady work.

## Current data contracts

- Treasures: 1,693 unique render records and 1,692 unique actor records;
  confirmed nonexistent save ID `11230106` is their sole set difference and
  the only default exclusion. There are 1,506 map-100 render records before
  that exact eligibility filter.
- Encounters: 9 Boss and 40 Assault records. Compact Boss/Assault height
  controls use authored spawn Z, the existing `playerZ - 150` correction and
  inclusive +/-500 alignment band; their reference sizes are 35/30, with
  Area Quest at 25 and consistent outline weight. These are icon sizes, not
  height thresholds. No new Boss/Assault Scene category is added.
- Mini-games: 33 Fly, 40 Mole, and 10 Wave records. All 83 map-100 rows carry
  exact trusted `NPC_Start` heights for the shared compact mini-game triangle;
  it is centered below the selected icon, uses the actual kind palette, points
  up for target high and down for target low, and hides inside the inclusive
  +/-500 band after comparison with `playerZ - 150`. The
  persisted control key remains `mole`.
- Area quests: 147 records with marker `X/Y/Z` retained independently from
  the generated `Height1MinZ/Height1MaxZ/Height2MinZ/Height2MaxZ/HeightBandCount`
  profile. There are 144 profiles, one with two genuine bands, and three no-source
  rows. Authored marker Z selects the uniquely nearest existing source band for
  the multi-band row but never supplies height; an exact-distance tie or missing
  profile remains neutral. Area Quest comparison also uses `playerZ - 150`.
- Area Quest Scene anchors are a separate full-XYZ catalog: 143 verified entity
  positions and four unconfirmed rows (1101301, 1103108, 1104104, 1104203).
  Those four are omitted only from Scene; the 147 original map records and
  144 height profiles above remain unchanged.
- Scene capacity: 50, with configurable 0-1000 m range and 0-50 shared limit.
  Only enabled Treasure/Area/Mini-game Scene categories consume the candidate
  budget. Defaults are 600 m / 24, Aim Focus labels and Scene switches on.
  Candidate refresh is 250 ms, selected projection 16 ms, changed distance text
  at most 100 ms. The shared cap does not promise that offscreen or clustered
  candidates will be replaced to fill every visible slot. Distance selection
  uses actually shown, raised icons: Aim Focus requires a same-identity 120 ms
  dwell inside the X16% / Y34% short-side ellipse, ranked by normalized ellipse
  distance; Auto Focus uses Euclidean distance and a 15% switch buffer.
  Projection-only offsets are Treasure +100, Area +180 and Mini-game +150 cm.
  Display distances subtract 1 m for Treasure/Area or 2 m for Mole only, clamp
  <=0 to zero, then round; Fly/Wave and authoritative distances stay unchanged.
- Expanded-map fixed capacity: 4,096 markers. The accepted maximum is 2,500
  Treasures plus 279 fixed non-Treasure rows, or 2,779 total, leaving 1,317 spare
  slots. Observed 1,632/1,501 snapshots were below the old 1,785 limit, so
  capacity was not the flicker root. No 8,000-unit geometry expansion is used.
- Compact marker capacity: 80, with the single nearest chest enlarged, fixed
  height groups for the nearest chest and nearest visible Fly/Mole/Wave marker, and independent
  height-band state for every retained visible Area Quest.

## Safety invariants

- Never retain Pawn, Controller, Canvas, Actor, task actor, table, or reflected
  array-element UObjects across frames or worlds.
- Use current game-thread Controller/Pawn reads and scalar/fixed-capacity
  snapshots; retained runtime identities must be weak.
- Never erase accepted encounter-death bits merely because ordinary context is
  temporarily invalid. Clear only a successfully applied bit; retain an
  exception bit; use numeric-only authoritative boundary settlement; reserve a
  whole-mask clear for process or UObject-array shutdown.
- Travel, F8, shutdown, and activity suppression must invalidate or detach state
  fail closed. The UE4SS module-unload callback unregisters hooks/listeners;
  this is runtime cleanup, not a user-facing uninstall tool. Once UObject-array
  shutdown starts, the create listener is removed and pinned callbacks become
  inert instead of calling unsafe late unregistration APIs.
- No recurring global UObject enumeration, periodic SQL worker, accumulating
  queue, or unbounded/steady retry loop.
- Area-task ambiguity must remain hidden or unconfirmed. `FAIL` alone is never
  completion evidence. Missing, ambiguous, duplicate, or unmapped task-class
  identity must never hide a guessed catalog ID.
- Static/build/deployment success is not gameplay acceptance.

## Verified evidence boundary

The latest historical runtime evidence available to this release audit includes
the R4 encounter and expanded-map layering sessions. Earlier dev60 evidence reproduced the dungeon-return compact failure
(`state=5` after the old short attachment window) and 22-32 ms Boss/Assault
class-enumeration spikes. Dev64 removes both historical mechanisms
structurally. Dev65 additionally
closes the runtime-proven encounter false-positive where streaming disappearance
after leaving a live large Assault was accepted as defeat. Dev66 addressed the
separate runtime-proven clustered-treasure event loss. The latest dev66 logs
then exposed the exact task-completion, treasure receiver, first-open world-map,
and moving-encounter candidate defects repaired in dev67. The R3 log then showed
Boss and Assault observations ending as departure eviction, with no direct
completion evidence; F8/F7 worked only through one-shot save reconciliation.
R4 subsequently proved one real Boss and one real Assault completion through
the exact death-notification route without F8/F7. The same R4 gameplay also
proved that a successful one-shot world-map restack is insufficient at maximum
zoom after close/reopen because a native child can be inserted later at the
same Z. R5 addresses only that late insertion with the exact zoom event and
bounded four-settle tail. The installed R5 log then attached 328 markers to
layer serial 3 before minimize/restore published replacement layer serial 4
with `map_id` temporarily unavailable. A settle wrongly refreshed the old
renderer, leading to failure 103, state 5, detach, and no recovery. R6 confines
same-layer work to exact weak ownership and waits for the replacement layer's
exact map-image readiness event. No R6 gameplay session has yet proved the
current correction set. R7 preserves that renderer correction and adds the
fixed 49-bit exact-death handoff repair, exact ghost-treasure eligibility, and
release/installer hardening. Its gameplay and performance matrix is independently
`NOT_VALIDATED`; publication remains `BLOCKED`. Earlier dev47 task scans were approximately 0.6-1.0 ms
total across 147 frames. Keep those historical measurements separate from the
current acceptance claim.

Expanded-map attach has previously measured about 94-97 ms when building and
importing the former atlases and about 27-37 ms on the former process-local
atlas-file cache hit. Those measurements do not describe the 2048 hybrid.
The current candidate can reuse an exact `DSNWRA52` persistent TGA fingerprint
across process restarts after strict full-payload validation, but cache read/
import work remains an explicit attachment transaction and a possible hitch.
Do not describe it as accepted performance.

## Build and deployment boundary

The historical exact 2.2.0 replacement passed Core `2/2`, the compact, world-map,
PostRender, and release-hygiene gates, and the clean native `/W4 /WX` build at
source-bound `main.dll` SHA-256
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`,
with compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
`Build-Release.ps1` package validation passed for that exact DLL: Setup reports
  `20/20`, the manual-copy matrix reports `2/2`, payload equivalence, manual
  layout, and clean-target policy validation pass, and all three public ZIPs
  re-extract byte-identically. Local diagnostics-enabled deployment of exact
  DLL `6AEFDACC...` passed with matching source, build, and installed hashes.
  Its rollback backup is
  `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
  The prior backup is
  `dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`.
  belongs to the superseded intermediate 59529B2A deployment and is not current
  candidate evidence. This remains non-Setup-owned. The prior 634D283A local deployment and its
  recorded rollback backup remain
superseded historical evidence only. The available healthy runtime log is bound
only to the prior exact 84A360B0 DLL; it does not validate the historical 6AEFDACC
structurally compatible update path, F6 presentation,
overlay, or compact-outline bytes. Controller, gameplay, height
visuals, responsive F6 layout and measured desired-size alignment, localization glyphs,
exit behavior, and performance acceptance remained independently `NOT_VALIDATED`.
This historical subsection does not validate 2.2.1. Earlier 2.2.1 technical
checks remain bound to superseded candidate bytes, and the independent-viewport/
extreme-Z, first-valid-parent, full-stretch-outer/Image-translation, and outer-
atlas-rectangle deployments are runtime rejected. The final rejected deployment
was DLL `CD41F0E1...6FBB2` from compiled source `433710E0...E62C`, with backup
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`.
The later dynamically rebased inner-atlas DLL `5C632820...D606` from compiled
source `798297BB...2585` is runtime rejected by its six same-parent Image-slot
writes and approximately `(+92,-915)` pixel displacement. Its deployment backup
`dist/work/deployment/deploy-backups/20260905-191717-084-native-only-deploy`
remains rejected-byte evidence only; see `docs/WORLD_MAP_ATTEMPT_LEDGER.md`.
The first immutable-placement revision passed its local native build at DLL
`A5CEBAEC...542EDB` from compiled source `72BD98D3...862A1`, size 1,107,968
bytes. It was superseded before deployment when the follow-up source removed
visibility reconciliation from retained refresh, so A5CE is build evidence only
and not the final candidate. Current WM-06 source review, static gates, Core
`2/2`, native build, and rollback-backed diagnostics-enabled developer
deployment pass for DLL `6435E100...C723A1` from compiled source
`0A1A4CE3...B5E5BC5`, size 1,107,968 bytes. Installed identity is exact,
`mods.txt` has one Radar entry, `debug_logging=true`, and backup
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy` is
retained. Package and installer validation pass for the exact WM-06 bytes:
Setup `20/20`, Manual `2/2`, payload equivalence, layout, clean-target, and all
three archive re-extractions. Gameplay, visual behavior,
native-icon stability, click-target alignment, controller behavior, responsive
layout, localization glyphs, resolution, attach time, memory, exit, and external
performance remain `NOT_VALIDATED`; public publication remains blocked.
Historical 2.1.1 automated and local
deployment evidence remains bound to `dist/final-2.1.1` and packaged DLL
SHA-256
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
The prior 2.0/2.1.0 evidence is likewise non-authoritative for current source.

The reported 09:33 exit crash came from the old E64 artifact and does not prove
shutdown causality. Current shutdown closes ingress atomically; only a known
live GameThread performs UE cleanup, true process teardown performs no
I/O/log/join/close, and one finalizer owns the final flush. Repeated-exit runtime
testing remains pending.

Run core tests, all four static gates, and the pinned clean native build before
deployment. `Build-Native.ps1` validates top-level and nested dependency
identity and emits a source-bound receipt. `Deploy-NativePrototype.ps1` must
refuse a running game, stale DLL or receipt, or source/metadata/DLL version
mismatch; it creates a rollback backup, atomically normalizes only the native
entry and removes valid `DragonSwordWorldRadarObjectState` predecessor entries
from `mods.txt` without normalizing unrelated content, and verifies the
exact installed payload. Do not launch or terminate the game from the deployment
workflow. The script checks before deployment preparation, before backup
creation, and once more immediately before the short mutation transaction.
It intentionally performs no later process check after mutation begins,
because a late failure would otherwise trigger rollback writes while the game
is already running.

Before a public release, build the ExperimentalNested payload and unsigned
single-file Setup only after the native DLL and dependency receipt are current.
`Build-Release.ps1` is the only complete release builder; `Build-Installer.ps1`
is its Setup-only internal stage. Release staging must
begin empty, re-extract and audit the installer archive, and admit exactly four
files: Setup, Setup SHA-256, `INSTALL.md`, and
`THIRD_PARTY_NOTICES.txt`. The runtime contains no `enabled.txt`; Setup migrates
a recognized legacy marker, removes the obsolete
predecessor line, and leaves one authoritative native `mods.txt` entry. It
zero-mutation rejects an active `DragonSwordWorldRadar` external renderer,
preserves its exact disabled `: 0` entry, and never deletes or disables it.
Existing-target replacement also requires the exact bounded metadata,
manifest, hash, recursive-tree, and live-file size contract recorded in the
release and installation documents before backup or mutation. The
release gate must require exactly 20
isolated installer tests passed, 0 failed, and 0 skipped. Packaging and
installer testing remain static/filesystem evidence, not gameplay acceptance.
Legacy `Stage-Release.ps1`, `Install.cmd`, and
`installer/Install-DragonSwordNativeWorldRadar.ps1` packaging paths are
retired.

The source archive is only the allowlisted project source; it excludes all SDK,
RE-UE4SS, `UEPseudo`, FetchContent, Rust-crate, and toolchain sources. Do not call
it a complete dependency-source or Corresponding Source bundle. Technical gates
do not authorize public publication: `UEPseudo`/Unreal redistribution and
GPL-compatibility review plus exact `e_sqlcipher.dll` source/build provenance
and derived-catalog/coordinate redistribution provenance remain unresolved
release blockers.

`Build-Native.ps1` requires a clean `IconFontCppHeaders` checkout at
`210b5a399a64270674560d633638952d1e8d804d` via
`-IconFontCppHeadersRoot`. Every supplied or reused checkout is validated by
canonical origin, exact commit, allowed worktree state, and parent gitlink where
applicable before disconnected reuse and again after a graph-owned clean build.
`metadata/native-build-lock.json` declares the dependency graph and
`dist/work/build/native/native-build-receipt.json` binds the output DLL to the current
compiled inputs and build tools.

## Historical 2.2.1 runtime gate

Require `START version=2.2.1`, runtime label
`DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1`, and exact identity with the
current `dist/final-2.2.1/release-manifest.json` DLL hash. Then require:

- F6 full-page fit at representative fullscreen/windowed, DPI, 16:9, 16:10,
  and 21:9 layouts, including the longest strings in all 11 languages; top-bar
  Bug Report and Close controls; the read-only state text with thin state strip;
  the separate Enable/Disable/Retry action remaining open after activation;
  translucent cards, equal-width filter choices, aligned text, and bounded hit
  regions;
- independent persisted height switches with all three defaults ON and valid
  existing choices preserved;
- the unchanged left-side Treasure six-piece arrow, the below-icon nearest-mini-game
  triangle with actual Fly/Mole/Wave color, near-black outline, unchanged size/
  position/projection, and +/-500 dead zone, and every visible
  Area Quest's aligned, up, down, or neutral shape, including simultaneous
  overlap, authored-marker-Z selection of the uniquely nearest source band,
  inclusive +/-500 comparison around that band, exact-tie/no-source neutrality,
  shared `playerZ - 150`, and mini-game fail-closed triangle hide;
- exact use of all 83 map-100 Fly/Mole/Wave trusted `NPC_Start` heights;
- resolved-language selector face; `DGameUserSettings.LanguageText` reads on
  every F7 and actual F6 opening; Kismet then English fallback; persisted manual
  override; preferred compatible game `DTextBlock`, best-effort CDO font support,
  exact-zero `Font.Size` seeding, same-open base `TextBlock` retry after
  game-widget construction/size/commit failure; every-language glyph coverage;
  Korean/Traditional generated 2x overlay size, weight, and alignment from the
  pinned base-32 DroidSansFallback source with one-pixel translucent stroke and
  role-specific optical baselines; all 30 fixed main-panel slots in each
  status-specific overlay; only the ko/zh-Hant names in the shared popup; the
  canonical `assets/ui/f6` payload and regenerated current-layout coordinates;
  and native game-font rendering for the other nine languages;
  and no recurring
  language/font work;
- expanded-map capacity 4,096 against the accepted 2,779-row maximum, two 2048
  atlases at approximately 32 MiB raw decoded BGRA, unchanged marker
  coordinates, and live dense-Treasure behavior; confirm both atlas hosts remain
  hit-test-invisible `DLayerMap.FogAbovePanel` children whose outer and inner `Panel_Point`
  slots are both full stretch with zero offsets after each fresh attachment,
  including a scheduler-accepted bounded rebuild. Only that fresh attachment
  may read `PlayerIconWidget` and set the Image Canvas slots to the exact parent-
  local atlas rectangle; the Images must retain zero render translation.
  Confirm native desired size and click targets remain unchanged, same-parent
  pan/zoom is inherited without transform-
  sync writes and every retained tail reads only the exact `FogAbovePanel` live
  local extent. Same-parent and same-extent passes, including the final pass,
  must keep Image placement immutable and perform no layout, transform, widget-
  tree, or RetainerBox write. Parent replacement must report `RebuildRequired`
  without live reparenting or delta-rebasing, and two matching successful extent
  observations must precede any extent-driven `RebuildRequired`. A rebuild
  request must not pre-collapse valid payload; only a successful schedule may
  enter at most one snapshot-preserving rebuild with its own three-attempt budget
  (session maximum initial 3 plus rebuild 3). Confirm the tail uses a fresh post-
  attach clock and attach issues no empty Retainer `RequestRender`. Confirm only
  a strictly validated
  `DSNWRA52` fingerprint hit skips rerasterization;
- F6 Off/On/Fault read-only status text with a thin state-colored strip,
  separate Enable/Disable/Retry action that keeps the menu open, playable-world
  activation guard, and fixed top-bar Nexus Posts Bug Report action;
- physical-controller map/pause suppression with no hardware cursor; and
- diagnostics-disabled same-session performance evidence showing no new scan,
  SQL, filesystem access, PAK extraction, timer, or unbounded retry.

All 2.2.1 runtime rows remain `NOT_VALIDATED` until captured against the exact
final artifact. The detailed matrix is
`docs/RUNTIME_FEEDBACK_AUDIT_2_2_1.md`.

## Historical 2.1.1 runtime gate

Require `START version=2.1.1` and runtime label
`DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_1`, provider/hook
readiness, a successful 147-item F7 snapshot, and no renderer or top-level
fault. The activation must also report `AREA_QUEST_TASK_CLASS_MAP ready=true
bindings=147 ambiguities=0` at `attempt=1/2` or `attempt=2/2`, with no third or
  steady retry. The diagnostic test deployment must report
`runtime_diagnostics=startup_config_once config_debug_logging=true log_schema=2`
in the
installed gameplay-test copy; a separate restart with the release default must
prove disabled logging produces no new
formatting-visible output, file creation, append, or rotation. Complete a short
currently visible area task without F8/F7 and require:

- open and close the world map by controller with no visible mouse cursor and
  require immediate compact collapse plus bounded restoration on the existing
  250 ms edge; pause/unpause must follow the same fail-closed suppression path
  without controller-mapping reads or a new timer;
- treat the archived 2.1.1 task-height behavior as historical evidence only.
  Its single-height acceptance cases are superseded by the 2.2.0 one- or
  two-band profile contract and must not be reused as current release gates;
- `AREA_QUEST_EVENT_HOOK_READY` for the one native dynamic-event hook;
- the cooking/delivery task to emit
  `AREA_QUEST_EVENT_TRIGGER progress_precondition=not_required`, arm
  `AREA_QUEST_COMPLETION_WITNESS_ARMED window_seconds=10 probe_ms=750
  schedule=exact_id_only`, and hide without F8/F7 through either exact
  `AREA_QUEST_COMPLETION_VERIFIED evidence=exact_task_id_end_state` or a
  baseline-relative exact-ID save confirmation; the fallback requires a known
  valid single-owner F7 `COMPLETE_CNT` baseline and strict count growth. A
  missing ID is zero only for that valid query; an unknown baseline queues no
  SQL. It permits one immediate attempt and at most two 15-second retries, with
  no fourth request or periodic SQL. Three non-confirming attempts lock the
  task generation until new F7 or settled repeatable reactivation. Final
  `END` is completion even after stale `NONE`, `FAIL`, `ACCEPTABLE`, or
  `PROGRESS`, but the event or `NONE` alone is not completion, and all
  simultaneous witnesses share the one-exact-query-per-engine-tick limit;
- task 1110080 and its unique linked Assault to become globally eligible at the
  applicable displayed-hour edge without local trigger discovery;
- F6 immediate selection changes, compact/world category independence, config
  persistence, direct cursor availability, `X`/F6 close, and travel cleanup
  with no cursor or panel ghost;
- the independent compact-only `BIRD EGGS` category for both exact egg classes,
  with no MAP toggle, no expanded marker, availability only at exact owned-
  component values `InteractableValue=2` and `InteractTypeValue=2`, unknown
  reads remaining retryable, exact EndPlay removing one identity immediately,
  at most eight unresolved candidate/position reads per 250 ms control tick, a
  nearest-16 bound, and no new polling, enumeration, or SQL;
- one exact `TitleMap` hard stop followed by a different save load: no stale
  mutable runtime state, no automatic reactivation, no accepted pre-open-world
  F7, and one fresh explicit F7 only after the loaded open world is ready;
- `AREA_QUEST_EXACT_COMPLETION_OBSERVED` with the expected catalog `id`,
  `evidence=task_class_map`, and `action=immediate_numeric_completion`;
- immediate compact removal and absence on the next expanded-map session;
- no revival when an older in-flight scan finishes after the exact completion;
- no revival from any later still-active sample before a current `NONE` or
  `END` inactive boundary;
- one later `AREA_QUEST_STATE_SCAN_REQUESTED
  reason=quest_state_event_debounced` transaction;
- no `AREA_QUEST_COMPLETION_UNMAPPED` for the mapped task;
- for a genuinely repeatable task, one current scan that observes `NONE` or
  `END`, followed by a later current scan that reports `ACCEPTABLE` or
  `PROGRESS`; `FAIL` must not arm reactivation;
- an already-active F7 mapping retry that does not clear exact completion bits,
  revisions, or the reactivation fence;
- an older in-flight F7 save result that cannot erase a newer native task
  completion or its dynamic-prerequisite evidence;
- an exact completion followed immediately by travel before the next normal
  engine sample; post-travel state must preserve the completion and prerequisite
  evidence through the numeric-only pre-transition drain and log
  `action=travel_boundary_numeric_preserve`;
- F8 followed by F7 must still discard the prior activation latch and perform a
  fresh resynchronization;
- a clean session with no `ENGINE_TICK_FAULT`, renderer `Faulted` state, or Hub
  fault. A controlled engine-tick fault may produce at most one
  `ENGINE_TICK_RECOVERED` in the process; a later fault must log
  `ENGINE_TICK_RECOVERY_STOPPED` and remain disabled without retrying;
- explicit F7 recovery only for a prior runtime-only world-map fault after clean
  detach, and explicit F6 recovery only for the equivalent Hub state. Same-call
  faults and ABI failures must remain terminal;
- a bounded native log: current file no larger than 1 MiB, at most one previous
  file, no per-frame event stream, and immediate persistence of critical
  lifecycle/fault lines.

For encounter acceptance, require the `READY` contract to report the fixed
49-slot discovery route with no enumeration, an eight-position-query limit
per 250 ms control tick, and `ENCOUNTER_DEATH_HOOK_READY`. Test the fail-open
capability boundary separately: if lookup or registration is deliberately made
unavailable, require a bounded disabled reason while the rest of the radar
still reaches ready state, but do not count that session as exact-artifact 2.1.1
encounter regression coverage. The R4 gameplay pass is historical evidence and
does not accept the final 2.1.1 bytes. Approach the same large Assault for more than one
second, then leave the 100-metre range several times without defeating it. Open
the expanded map or another cursor-visible menu nearby as a separate negative
case. None may create `ENCOUNTER_DEFEATED` or a cooldown, and returning must
rebind and show the still-live actor without F8/F7. Then actually defeat one
Assault and one Boss while staying nearby, without pressing F8/F7. Require one
`ENCOUNTER_DEFEATED_NATIVE
evidence=exact_observed_net_multicast_notify_death` and one
`ENCOUNTER_RUNTIME_STATE_APPLIED` for each defeat. Separately prove the fallback
cannot complete before at least forty missing 250 ms samples spanning ten
seconds while all context gates remain valid. An observed
  `RemovedFromWorld` must release its weak UObject and may complete only after
  that full numeric gate. Travel, activity suppression, leaving range, and
  reappearance must reset or reject the candidate.

For the R7 handoff specifically, accept a real exact event at the end of a
time-conditioned window and let that window change before ordinary consumption;
the already accepted bit must still apply. Invalid ordinary context must defer,
not erase, a bit. With multiple bits pending, each successful application must
clear only itself, and an injected apply exception must retain the failing and
all still-pending bits. Exercise F7, disable, travel, and activity suppression with pending bits:
each boundary may settle numeric cooldown/eligibility before reset but must not
mutate the renderer. Whole-mask clearing is permitted only at process or
UObject-array shutdown. Require no new poll, timer, scan, SQL, queue, or steady
work.

Also verify that treasure interactions and Boss/Assault discovery produce no
`NEARBY_CLASS_CATCHUP` and no production `FindAllOf` timing event. Record the
bounded world-map-layer catch-up, clock baseline, task-definition capture, and
expanded-map `attach_total_us` separately. Test F8/F7, travel, map reopen, task completion,
treasure opening, Boss/Assault defeat, mini-game visibility, expanded-map
layering across zoom levels, immediate compact scene-handoff suppression, and
two complete dungeon enter/return cycles without F7/F8. Each return must log a
distinct `COMPACT_LAYER_CAPTURE source=create_listener` serial, a fresh
`COMPACT_GEOMETRY`, and `COMPACT_POOL_STATE state=2` without another fault.
`attach_rearmed=true` is required when the event is consumed outside transition
and activity suppression; `false` is valid only when transition return already
performed the bounded rearm. At maximum zoom, repeatedly close/reopen the map
  and change zoom. Require only the 100/250/500/1,000/1,250 ms five-deadline tail
  for the latest event. Every due game-thread pass must take one fresh numeric
  observation and overdue deadlines must not collapse into multiple
  observations in one pass. The first four passes are read-only; only the final
  pass may mutate under the existing exact retained-parent witness and stable
  parent-local geometry rules. Layout transitions continue through the existing
  bounded map/zoom reconstruction strategy; this correction adds no parent-size
  post-check, parent-growth rejection, or new extent-change allowance.
  Require radar ordering above late native same-Z children after the final pass
  and no continuing restack or steady poll. The latest historical evidence is
  the R4 Boss/Assault
pass, the R4 maximum-zoom layering failure, and the R5 minimize/restore
replacement-layer failure. With the map visibly attached, minimize and restore
the game. A new incomplete layer must not fault or detach the old renderer;
its exact `SetWorldMapImage` edge may consume at most one existing rearm and
must attach through the bounded three-attempt readiness service without F8/F7
or another map close/reopen. Require no focus hook, focus poll, recurring timer,
or continuing work. Final 2.1.1 runtime acceptance remains `NOT_VALIDATED` until this
exact-artifact session is captured.

For clustered-treasure acceptance, open the known 2.80-metre pair and the
1.815-metre same-level pair in immediate succession, then exercise one
XY-overlapping different-height set. Every physical chest must produce its own
new `TREASURE_OPENED_NATIVE` ID with
`identity=exact_treasure_actor_receiver` and an expected receiver identity
source, then disappear from compact selection on the next normal refresh
without F8/F7. Non-local interaction must be rejected and no unopened neighbor
may be hidden. The interaction
session must contain no treasure enumeration, SQL retry, recurring correlation,
or growing pending state.

After a Boss or Assault defeat, first require
`ENCOUNTER_RUNTIME_STATE_APPLIED`, then use F8 followed by F7 before the save
write and require the process-local cooldown to remain in force after the
one-shot save result.
Also cross one natural cooldown expiry and one time-conditioned Assault hour
edge. A real change must emit `RUNTIME_VISIBILITY_EDGE`, refresh compact state,
and either rebuild one visibly open exact attachment or retire a hidden retained
atlas until its next exact `SetWorldMapImage` edge, without adding 49-entry work
to the 16 ms motion path.

For expanded-map delta acceptance, first attach map 100, return to gameplay,
then open a treasure or finish a linked mini-game and reopen the same live map
layer. The next real session must omit the changed marker instead of reusing a
stale attached atlas. A hidden retained layer must defer to the next exact
`SetWorldMapImage` event; a visibly open exact layer may rebuild in the current
session. A matching serial may rearm the bounded readiness budget only once,
and repeating the same event must not dirty compact selection, replenish the
budget, or trigger another atlas invalidation/rebuild.
