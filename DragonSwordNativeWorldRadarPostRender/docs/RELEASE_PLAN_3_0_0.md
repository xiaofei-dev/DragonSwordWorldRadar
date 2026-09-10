# Radar 3.0.0 release plan and preserved candidate evidence

## Current Guide activity-order revision

Candidate `radar-3.0.0-sg16-guide4-20260909`; version **3.0.0**.

Guide swaps the Marmot mini-game and Sudden mission entries as complete groups: the text and both Radar/Map icons move together. The left column now reads Flying mini-game, Wave mini-game, Marmot mini-game and Bird eggs; the right column reads Sudden mission, World boss and Area quest. All eleven languages use this order. The three height columns, 40 icon examples and 539 text cells remain unchanged.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T02:04:37.4453785Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T02:06:49.3883924Z`.

Manifest UTC: `2026-09-10T02:06:02.2485470Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/package-verification.json).

The previous Guide3 and earlier SG-16 sections below remain historical records. Their earlier activity order and ZIP hashes identify those sets. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Current Guide three-column revision

Candidate `radar-3.0.0-sg16-guide3-20260909`; version **3.0.0**.

The owner requested a simpler height reference. Guide now displays only Above, Near level and Below, with 15 height examples, 40 total icon examples and 539 text cells across eleven languages. The Unknown column is no longer drawn. Actual unknown-height handling, height switches and all other gameplay behavior are unchanged. The existing 44-string localization schema remains intact.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T01:45:25.9749727Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T01:47:39.7016800Z`.

Manifest UTC: `2026-09-10T01:46:51.6121910Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/package-verification.json).

The previous SG-16 sections below are preserved historical records. Their four-column Guide description and old ZIP hashes describe that earlier set. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Current SG-16 verified local delivery

Candidate: `radar-3.0.0-sg16-20260909`. Live Map changes, independent Radar/Map All controls,
default Auto focus and the revised compact Guide are implemented. Green means
mini-game reward chests; Mini-games groups flying, marmot and wave types. Guide
covers height states, Clock, aligned Scene examples and four distance rows.
First Aim acquisition is 100 ms; Aim/Auto replacement remains 350/500 ms.

Native 446/446, Core 9/9 and all four source/release gates pass. Setup passes 20/20 and Manual 2/2, with no failures or skips. The three channels have 104 equivalent runtime files, including 73 UI files and twelve Guide files; ZIP entry counts are 4/108/112.

Deployment UTC: `2026-09-10T00:22:37.0293741Z`. The packaged DLL matches that
installed artifact. Promotion UTC: `2026-09-10T00:23:01.4155094Z`. The five
preceding final files are preserved at `dist/work/candidates/radar-3.0.0-sg16-20260909/previous-final-sg15`; exact archive and native
identities are recorded in [Release status](RELEASE_STATUS.md).

README and both installation guides were finalized and frozen before the final package build. They describe the product and its controls; these external receipts record exact build, deployment and delivery results. The payload documents and metadata were not rewritten after sealing.

Build, source/resource checks, installation and package checks are verified; they do not establish game visual, input, Guide usability, camera-motion or measured performance acceptance. Those remain owner testing. No Nexus upload or post was performed.

## Preserved SG-15 runtime and release handoff

The preceding runtime is **SG-15**, with localized Guide, natural typography and
stable Aim/Auto switching. The package candidate is
**`radar-3.0.0-sg15-release-20260909`**. The owner has authorized the Installer
and both manual packages. Build tooling changes require a fresh canonical
receipt while preserving SG-15 runtime behavior and its prior deployment record.

Scene markers use current-frame calibrated projection with native fallback,
subpixel render translations and bounded edge/overlap hysteresis. All world
positions, height rules and distance corrections remain unchanged.
[Release status](RELEASE_STATUS.md) owns current build, deployment, package and
promotion receipts. Previous candidate sections below are historical snapshots;
their old packaging holds do not limit the current authorization. Nexus text
is prepared locally for the owner, with no external upload or post.

Current verification checklist:

- Freeze README.md, docs/INSTALL.md and docs/MANUAL_INSTALL.md before staging;
  these are the documents embedded in the three packages.
- Verify the canonical native receipt, Core suites and four source/release
  gates against the release candidate; keep the 73-file UI inventory intact.
- Build exactly three packages from the same payload, run Setup and manual
  installation matrices, verify payload equivalence and fresh ZIP extraction,
  and record exact sizes/hashes before promoting the final directory.
- Keep game appearance, guide scrolling, resizing, camera motion and measured
  performance acceptance separate. Do not infer these from package tests.

The UI inventory is 73 files: the unchanged 61-file SG-14 inventory plus eleven
Guide atlases and one manifest. Guide adds 27 strings per language, with a
1120-reference-unit scrolling body. Existing settings keep four cards, 32 help
topics and seven confirmation strings per language. The title bar has Guide
and Close; Reset to defaults, Vote for this mod and Feedback stay in the footer.

Package generation and final promotion are verified for the release candidate.
Setup passes 20/20 and Manual 2/2 with zero failures/skips; all channels have
104 equivalent runtime files. Freshly extracted ZIPs are byte-identical and
have 4/108/112 entries. Promotion UTC: `2026-09-09T23:23:40.0349295Z`.
The previous five SG-10 final files are hash-verified in `dist/work/candidates/radar-3.0.0-sg15-release-20260909/previous-final-sg10`.
Exact archive and Setup identities are in Release status. The rebuilt release
DLL was not redeployed; owner game acceptance remains separate.

## Preserved SG-10 build and delivery

Previous source candidate: **`radar-3.0.0-sg10-20260909`**. The owner added
confirmation before opening Nexus and before restoring the global display
preset, with separate Endorse and Feedback entries. **The final native build,
local deployment and all three final local packages are complete and verified.**
SG-10 is installed, and `dist/final-3.0.0` now contains its replacement archives.
[Release status](RELEASE_STATUS.md) owns the exact receipts, hashes and current
transaction state. Game visual, input and performance acceptance remain
pending owner testing. Nexus material is a local draft only; no external
publication or upload was performed.

The canonical native receipt is dated `2026-09-09T16:43:53.6047126Z`; its
1,310,208-byte DLL SHA-256 is
`F1203366DA7488FBFC9E8BF101598A4FC4D2E840F850D66CEC487452FEC7EB65`.
Core **7/7**, the **446-target** clean native build, Compact, World Map,
PostRender and Release Hygiene/resource gates pass. The final installer and
manual matrices pass **20/20** and **2/2**, respectively.

Deployment was verified at `2026-09-09T16:51:20.6801835Z`, with a matching
DLL/receipt and all 34 UI files. Visibility/language, diagnostics, hotkeys and
Treasure overrides preserve their previous bytes. The previous SG-09
installation is backed up under
`dist/work/deployment/deploy-backups/20260909-095118-801-native-only-deploy`.

Package promotion was verified at `2026-09-09T16:55:11.2153219Z`. The three
final archives contain **4/69/73 entries**; Setup/manual payload equivalence
passes, all **64 payload files per manual archive** match, and **60 installed
static files** match the packaged payload. The runtime inventory remains 65
files. The previous SG-03 final ZIPs, manifest and checksums are retained in
`dist/work/candidates/radar-3.0.0-sg10-20260909/previous-final-sg03`.

## Preserved SG-10 scope and acceptance boundaries

1. **Separate website entries:** Endorse asks before opening this mod's Nexus
   page; Feedback asks before opening its suggestions/posts page. Only Yes
   emits `OpenEndorsement` or `OpenBugReport`. Opening either page does not
   submit an endorsement, post or report.
2. **Confirmed preset:** Restore Preset first opens the same native UMG modal.
   Only Yes executes the existing page-wide preset: display/height choices on,
   filters Available, language AUTO and Scene 600 m / 24 / Aim. Runtime power
   and startup hotkeys are preserved. No and Esc dismiss only the modal,
   retain F6 and leave all settings unchanged.
3. **Lifecycle and input:** consume confirmation before producing a command
   or preset mutation. Require a neutral sample before Yes and reject repeated
   confirmation; No wins simultaneous input. Disable background controls and
   retain the input shield for one dismissal service turn. Focus loss closes
   the entire F6 panel and clears intent; F6 exit and travel also clear intent.
   Esc can cancel the modal through its verified owner even if the current
   controller probe is temporarily unavailable. No Windows MessageBox is used.
4. **Localized material:** preserve the blue-gray 760x792 settings page and
   four cards, with a 560x280 confirmation panel. All eleven languages use
   seven confirmation text tiles in the existing atlases, now 32 tooltip plus
   seven confirmation tiles each (640x5616 pixels). Main text stays at 44 slots
   and the UI payload stays at 34 files. Format/dimension validation rejects
   obsolete atlases before import; unavailable text disables Yes and leaves a
   cancel-only fallback.

The standalone pure confirmation model passes **55 checks / 0 failures**.
Independent source review covers command/reset gating, weak-handle cleanup,
background control ownership and the null-controller Esc cancellation edge.
English and Chinese source-derived previews show the actual atlas crops and
native panel geometry without clipping. Resource checks include all eleven
languages and rejection of obsolete atlas layouts. The completed native build
and delivery checks do not establish actual Slate click capture, held input,
focus loss, travel, game composition or FPS; those remain runtime acceptance
items for the exact installed artifact.

Evidence is retained under `dist/work/candidates/radar-3.0.0-sg10-20260909`:
`validation-summary.json`, `deployment-verification.json`,
`package-verification.json` and `release-packages/release-manifest.json`.
The validation summary is a predeployment snapshot, and the deployment record
precedes packaging; their earlier pending fields are superseded by the later
deployment/package receipts. SG-09 receipts remain historical identities.

## Preserved SG-09 scope and verified deployment

SG-09 candidate: **`radar-3.0.0-sg09-20260909`**. Core 6/6 and the clean native
build passed, and deployment completed on September 9 with a matching DLL,
receipt and 34 UI files while preserving the four user configuration files.
Its exact identity and deployment evidence are retained in Release status.
Its three generated package candidates had not been promoted when SG-10 began.
The six changes below remain inherited behavior; they do not by themselves
establish gameplay, visual or performance acceptance.

1. **Blue-gray settings:** soften SG-08 charcoal into restrained translucent
   blue-gray surfaces and gradients, with readable text and status plates.
   Preserve the approved layout, square checkboxes, languages, 31 explanations
   and resource inventory. Static composition checks do not prove the game's
   HDR/display pipeline; no real-time blur or refraction is added.
2. **Area Quest contrast:** add a subtle translucent backing inside the open
   gray diamond. Its enlarged footprint, rails, three white dots and lower
   direction tip remain fixed; chest and purple-flag textures stay unchanged.
3. **Chest display clearance:** change Treasure projection-only lift from
   +100 to **+160 cm**; Area Quest/Mini-game remain +180/+150 cm. Source XYZ,
   range, raw distance, minimap/map height and focus rules remain unchanged.
   This uniform UI adjustment does not inspect terrain or mesh bounds and
   requires owner checks against differently sized chests.
4. **Clock gap midpoint:** the existing **1 Hz** layout service reads the exact
   minimap RetainerBox and `DLayerQuest` completed geometries. Its 15-unit visual
   center is placed midway between minimap bottom and task top. A valid gap
   needs at least 30 reference units and must fit the retained host and viewport.
   Exact owner/paint-chain and reflected ABI checks reject unavailable, hidden
   or unsuitable geometry; fallback remains **178 reference units** from the
   minimap center. Resize defers the stale sample. No widget scan or clock-time
   sampling change is added. The 28 standalone MSVC geometry checks pass;
   actual HUD placement remains an owner acceptance item.
5. **Settings preview:** Compact Radar and Scene remain eligible while this
   F6 panel owns a cursor opened over visibly active gameplay. Native menus,
   pause, world map, hidden HUD and activity guards remain effective. Ordinary
   menu sampling and preference edits preserve the retained renderer tree.
6. **Scene motion:** project every engine frame independently of the 16 ms
   position sampler. Retain catalog/eligibility selection at 250 ms, update
   scalar distances for at most 50 selected markers per frame, and replace
   Canvas position writes with render translation after cumulative 0.25-pixel
   movement. Focus, distance text cadence and provider authority are preserved.
   These source changes do not establish an FPS or frame-time improvement.

Existing user visibility, language, hotkeys, diagnostics and treasure overrides
must retain their bytes during the authorized deployment. The approved page
preset remains all display/height choices on, filters Available, language AUTO
and Scene 600 m / 24 / Aim; explicit saved choices survive upgrades. The three
local replacement archives require their own package/installer verification
and exact identity checks. This work does not include Nexus upload, automatic
game control or owner gameplay acceptance.

## Historical SG-08 candidate and deployment

Candidate at that checkpoint: **`radar-3.0.0-sg08-20260909`**. The owner's dark
translucent menu reference produced neutral charcoal, restrained edges,
transparent module gaps and readable small-text surfaces. Static sRGB and
linear-light previews bounded readability over generated backgrounds; they
did not establish game composition or visual acceptance. Layout, controls,
tooltips, resource count, Scene anchors and height parameters were unchanged.
The translucent native fallback added no live blur, refraction, texture import
or per-frame work.

SG-08 was subsequently deployed at the owner's explicit request, preserving
user settings and backing up the previous SG-05 installation. Its evidence is
`dist/work/candidates/radar-3.0.0-sg08-20260909/deployment-verification.json`
relative to the project root; [Release status](RELEASE_STATUS.md) retains the
exact build/deployment identity. Final packages were not replaced in SG-08;
the retained ZIPs identify SG-03. These records do not validate SG-09 bytes.

## Historical SG-07 candidate

Candidate at that checkpoint: **`radar-3.0.0-sg07-20260909`**. Hover text expanded
from 18 shared descriptions to 31 setting-specific topics, covering seven marker
categories, three Scene switches, five height switches, four separate filter
options and twelve utility/range/distance controls. Seven marker names also
accept hover; 55 owners fit the existing 64-slot pool. The layout, native hover
behavior, file inventory and runtime power rules are unchanged. Each of eleven
languages contains 74 strings, with 341 tooltip tiles in the same eleven atlas
files. Build and verification identities are in [Release status](RELEASE_STATUS.md).

The reported chest height discrepancy was an investigation, not a claimed
fix. Matching static catalogs did not establish actual mesh-top height. At
this checkpoint all chests received the same 100 cm display lift; the visible
direction tip ended 16 logical pixels below the projected center. The screenshot
cannot establish a unique chest ID or whether the requested correction targets
Scene placement or minimap height classification. No height parameter changes
were included in SG-07. The +160 cm SG-09 UI lift supersedes that value without
claiming mesh-height detection. Read repository-root
`out/handoff/SG07_TREASURE_HEIGHT_AUDIT.md`.

At the SG-07 checkpoint, SG-06 build/assets were retained separately, SG-05
was installed and the final ZIPs identified SG-03. SG-07 did not deploy or
replace packages. SG-08 later superseded that installed baseline.

## Historical SG-06 candidate

Candidate at that checkpoint: **`radar-3.0.0-sg06-20260909`**. SG-05 had been
installed after the owner's separate deployment request; final release
archives still identified SG-03. The sections below retain their earlier
acceptance scopes and are not current installation claims.

## Preserved SG-06 scope

Keep the current settings flow and four-card layout. Replace flat gray native
surfaces with static rounded skins and corrected linear brush colors. Square
Radar/Map checkboxes are confined to settings; no minimap/world-map glyph is
redesigned. The new native nine-slice path verifies reflected fields and has a
plain fallback. No real-time blur or hover polling is introduced.

Restore Preset moves to the top and resets all display preferences on this
page, including all three Scene categories on, 600 m, 24 markers, Aim Focus,
height flags on, filters Available and language AUTO. Current module power and
startup hotkeys are preserved. Existing saved Scene choices are not overwritten.

Every control receives one of 18 explanations, fully translated into eleven
languages. The 320x72-reference tooltip tiles use shared language textures and
separate native widget ownership; old-language failures retire stale contents.
Range/count explanations distinguish display range from anchor height and
describe shared limits, performance cost and Aim/Auto selection behavior.

Area Quest's open gray diamond grows 18% with a stronger outline to balance
its smaller filled area. Chest, Mini-game and the small direction tips keep
their geometry. Clock moves below Bird Eggs in settings and 16 reference units
lower in the HUD. True positions, projection-only lifts and distance rules stay
unchanged. Asset/source previews are geometry evidence, not game acceptance.

The runtime specification includes 26 F6 TGAs and six Scene TGAs with two
manifests. Future targets are 65 runtime files and 69/73 manual archive entries;
existing final archives remain 4/43/47 entries. Current source, build, installed
and archive identities are recorded separately in [Release status](RELEASE_STATUS.md).

## Historical SG-05 scope and acceptance snapshot

Candidate at that checkpoint: **`radar-3.0.0-sg05-20260909`**. This round covers the F6
redesign, bounded Scene rendering changes, core/native compilation and source
verification. Final Core tests pass 5/5, the pinned native clean build passed
all 446 targets, and all four source gates pass. The final SG-05 receipt below
identifies this source; SG-04 and SG-03 hashes remain historical evidence.
At this build-only checkpoint, deployment was **skipped by owner request** and
packaging was deferred; installation and all three final ZIPs still identified
**SG-03**. A later separate request installed SG-05, and SG-08 subsequently
superseded that installation. Preserve this original build checkpoint.

## Preserved SG-05 scope

F6 uses a **760×792** dark panel at a 1920×1080 reference, with the existing
viewport/DPI fit clamp. A larger title, enlarged section headings, short accent
lines and separated cards replace the dense framed table. The compact language
and Mod-status controls share the utility row below the title.

1. **Marker visibility:** seven rows with Radar and Map columns only. Map
   continues to omit Clock and Bird Eggs.
2. **Scene guidance:** immediately below marker visibility, with independent
   Treasure, Area Quest and Mini-game chips, two range/count sliders and four
   distance choices. Restore Preset changes only Scene to all three switches
   off, 600 m, 24 markers and Aim. Normal final-control sampling publishes one
   atomic result and a reset flag; other modules and hotkeys are not reset.
   An explicit reset also makes an already-pending Scene preference write due
   immediately when the controls already equal the preset, without inventing
   a new change or touching other settings.
3. **Height indicators:** five choices arranged as three plus two chips.
4. **Filters:** Area Quest and Assault choices sit in two bottom groups.

The 44 main text slots comprise 32 localized slots, seven marker labels and
five height labels. Scene labels have their own objects while reusing existing
translated category names. All eleven languages and the existing Esc/F6/Close
input ownership, final-slider sampling and persistence remain supported.

F6 retains nine TGA files: six Korean/Traditional Chinese main overlays,
one popup overlay for Korean/Traditional Chinese/French/Spanish names, and two
French/Spanish LanguageValue textures. The new inventory contains **177
verified codepoints**. Main overlays are 1520×1584; value-only textures are
440×52, shown at 126 / 58 / 220 / 26 reference geometry. Successful brush
readback still precedes native-text hiding; AUTO, resource-failure fallback,
bounded weak caches and edge-only loading are retained.

Scene keeps the exact SG-04 small glyphs, colors, outlined `v`, 32×36 group and
16 / 16 glyph/focus center. Six shared **128×144** sprites replace 600 individual
Border pieces with **50 marker Images and six hidden texture-holder Images**.
Textures load on attachment; category edges bind the corresponding brush.
Position commits are skipped until cumulative movement exceeds **0.25 screen
pixels**, and hidden paths return early. Existing candidate/projection cadence,
distance formatting/update cadence, authored anchors, category masks, offsets,
completion and save authority remain unchanged. These are bounded-work
changes; an FPS or latency improvement has not been measured in the game.

Aim now uses an ellipse with horizontal and vertical radii of **16% and 34% of
the viewport short side**, respectively. It ranks visible targets by normalized
ellipse distance and requires the same best target for **120 ms**. The boundary
is excluded; leaving the ellipse, view or retained set removes the label.
Auto continues to rank by Euclidean screen-center distance and replaces a valid
target only when a challenger is more than **15%** closer. The saved
`central_radius` and `nearest_center` tokens remain compatible.

## SG-05 verification and next handoff

- F6 source/resource checks pass for 44 main slots, 11×43 localized strings,
  177 codepoints, nine TGA files and 48 scaled selected-background bounds.
  Sixteen deliberately invalid source/asset variants are rejected.
- Independent Hub checks pass for the real control/reset baseline and reject
  seven wrong-binding, extra-column, wrong-reset, missing-result and missing-
  cleanup variants. Independent code review found no confirmed binding/reset
  defect. English and Simplified Chinese source previews were visually reviewed;
  all 396 native-reference text runs in the eleven-language preview fit.
- Scene resources pass 78 checks and reject nine deliberate invalid variants.
  Their glyph geometry is tied to the retained SG-04 source snapshot.
  Eight deliberate runtime-source variants were also rejected by the real gate.
- Compact, World Map, PostRender and Release Hygiene gates all pass against
  the frozen source/tools. The Compact gate also rejected eleven deliberate
  shutdown-path variants, retaining its Unreal-access and cleanup restrictions.

The final SG-05 Core run passed **5/5**, including **308 Scene checks** and
**50 Escape checks**. The pinned native clean build passed all **446 targets**.
The final receipt is `dist/work/build/native/native-build-receipt.json`:

| SG-05 final build field | Value |
| --- | --- |
| Receipt UTC | `2026-09-09T12:10:21.3873261Z` |
| Native DLL size | 1,200,128 bytes |
| Native DLL SHA-256 | `C04C6E29D1113B183D8ED511C00BBE7482E46428B6EE53FFDA889319CC252B97` |
| Compiled source SHA-256 | `16A389B6F543F0A54496FFD90ECC07DE0D7C1AE920CF5116F19F4C3E4DC9B207` |
| Release tools SHA-256 | `C65867EDC916C3727A312DC4370ED0DF39F55F461036637F11D076C590878A05` |

These are build and source-validation results. Source previews use substitute
native fonts and approximate slider styling; they are not game screenshots.

Future packaging targets are **52 / 56** entries in No-UE4SS / With-UE4SS manual
archives and **48** runtime manifest files, including nine F6 and six Scene TGA
files plus their two asset manifests. These targets are not completed packages.
The existing three ZIPs, release manifest and installed baseline remain SG-03.
[Release status](RELEASE_STATUS.md) owns the current transaction state.

After a later authorized deployment, owner acceptance must check the four-card
layout at actual viewport/DPI settings, all languages and AUTO, Scene-only
Restore Preset plus reopening/persistence, Esc during popup/slider interaction,
and sprite/distance stability across camera movement, Off/On and Travel.
Measure dense-scene frame cost and memory before making performance claims.

## Preserved SG-04 source candidate (not deployed or packaged)

SG-04 passed its pinned clean native build of all 446 targets and its source
gates. The owner skipped deployment and final packaging. Its exact acceptance
object remains in `historical_sg04_release_acceptance` in metadata/release.json;
the following scope and receipt identify SG-04 only.

## SG-04 source scope

- The F6 language popup selected Border is now **186×28** inside its 190×32
  cell, with a two-unit inset. The former width of 353 came from an unrelated
  vertical layout coordinate and extended into the next language column.
  The corrected dimensions constrain the actual background geometry.
- Popup names for French and Spanish now join Korean and Traditional Chinese
  in the packaged text layer. Native labels are hidden only when the popup
  resource loads and its brush binding is verified. Other language choices
  retain their existing native text path.
- The main-card French and Spanish names use two separate **560×54** textures
  displayed only in the existing **200 / 92 / 280 / 27** reference rectangle.
  One noninteractive Image and two weak texture slots cover this one name.
  AUTO uses the resolved language. Only a verified texture binding hides the
  native LanguageValue; failed resources retain it. Other languages return to
  native text or the existing Korean/Traditional Chinese main overlay.
  Loading occurs on open/language/status edges, with failed attempts remembered
  until the next open; close and Travel clear the weak handles and failure state.
- F6 resources now comprise **nine TGA files**, retaining the 41 main text
  slots and expanding the verified inventory to **170 codepoints**. This change
  repairs language names without extending image coverage to French/Spanish
  settings body text.
- Scene symbols are reduced to approximately 18–19 reference units across.
  Tasks use an open gray diamond with three horizontal dots; chests retain
  their horizontal lid/clasp and four colors; mini-games retain purple crossed
  flags. A small outlined `v` sits below each glyph. Each marker uses a fixed
  pool of **12 pieces**: eight glyph pieces and four chevron pieces.
  The chevron follows the lifted UI group; it does not relocate the catalog
  anchor, alter world coordinates, range selection or distance calculations.

The SG-03 distance modes, displayed-height rules, eligibility, saved settings,
shared Scene cap and Escape lifecycle remain the baseline. No new gameplay
discovery or input route is introduced by these presentation changes.

## Historical SG-04 verification

SG-04 source/resource checks include Windows PowerShell 5.1 validation of
all nine TGA files, pinned font/source inputs, 41 main slots, 170 codepoints and
48 selected-background bounds. Ten deliberately invalid source/asset variants
are rejected, including missing French/Spanish glyphs, invalid AUTO routing,
premature native hiding and missing cache cleanup. These are static checks,
not an in-game visual result.

Candidate **`radar-3.0.0-sg04-20260909`** passed Core CTest **5/5**, including
289 Scene model checks and 50 isolated Windows Escape checks. The PostRender
source gate also passed. The pinned Windows PowerShell 5.1 clean native build
completed all **446 targets** at **2026-09-09T11:08:33.1000443Z**.
The retained receipt and result belong to the
`dist/work/candidates/radar-3.0.0-sg04-20260909/` evidence directory.

| SG-04 identity | Value |
| --- | --- |
| Native DLL size | 1,196,032 bytes |
| Native DLL SHA-256 | 96485EC81471D8D4BD568177A1D18F673CA2553DA60F3047ABAC978E24F881C3 |
| Compiled-source SHA-256 | CBE6649DB164F5AACF4E6250E6FDCB7C52489A82D48C059F042CF46568C67546 |
| Release-tools SHA-256 | 337E8940017C4BD9AE1C6021B07357EF0A5DF15CE6C06270826E3B48A286B593 |

Compact, world-map, PostRender/runtime and release-hygiene/resource gates all
pass; the exact results and unchanged SG-03 installation/archive checks are in
the candidate's `validation-summary.json`. This receipt identifies build output only; it is not an
installed-state or final-package receipt. Deployment and final packaging are
**deferred by owner request**; [Release status](RELEASE_STATUS.md) owns the
current transaction state. A later combined candidate must receive its own
receipts before any package, installed-state or gameplay claim is made.

After a later authorized deployment, owner checks must cover popup selections
in all columns, complete French/Spanish names in both popup and main card,
AUTO resolving to each language, switching away and back, viewport/DPI scaling,
and the smaller Scene symbols/chevrons against bright and dark backgrounds.
Recheck Esc with the popup and slider drag. Source previews and compilation do
not complete those visual or input acceptance checks.

## Preserved SG-03 baseline

Historical candidate: **`radar-3.0.0-sg03-20260908`**. Its pinned clean native
build, local package validation and backup-capable deployment passed. The
SG-03 DLL remains the installed baseline and the three existing final ZIPs
contain SG-03. Exact historical results are recorded in
[Release status](RELEASE_STATUS.md) and the final release manifest. Gameplay,
visual, input-device and performance acceptance remain owner checks for the
specific candidate being tested.

SG-03 retains SG-02 Scene settings and corrected task anchors, then changes
Scene presentation, distance focus and F6 Escape handling. The
[Scene guidance ledger](SCENE_GUIDANCE_ATTEMPT_LEDGER.md) retains SG-01 and SG-02
history. Earlier DLLs and test results are not acceptance for SG-03.

## SG-03 scope (historical installed baseline)

Scene Treasure, Area Quests and all 83 Mini-games remain independently
default-off. Enabled Scene categories share a range and marker budget:
600 m / 24 by default, configurable to 0–1000 m / 0–50. Either zero numeric
setting hides Scene. Disabled categories consume no slots. Offscreen and
overlapping selected targets can leave fewer visible markers than the cap.
Numeric selection uses the existing 250 ms refresh; the frame path projects
only the retained capped set through the engine widget-space projection.

Scene uses gray-backed exclamation task badges, horizontal lid-and-lock chest
glyphs in the four existing treasure colors, and purple crossed flags for
Fly, Mole and Wave. Distance text is smaller. Projection alone raises the
displayed position: Area Quest +180 cm, Treasure +100 cm, Mini-game +150 cm.
Authored XYZ, range selection, actual anchor distance, compact/map coordinates
and save/completion authority are unchanged by this display lift.

Area Quest Scene coverage remains 143 verified whole-XYZ entity anchors.
IDs 1101301, 1103108, 1104104 and 1104203 remain omitted from Scene only because
their anchors are unconfirmed. These are static navigation anchors, not a claim
that each icon identifies an exact live interaction point.

| Distance choice | SG-03 behavior |
| --- | --- |
| Off (off) | Keep icons; hide distance labels. |
| Aim (central_radius) | Label at most one visible marker: the closest to screen center inside a circle whose radius is 10% of the viewport short side. The same target must remain selected for 120 ms. Leaving the circle, view or retained set removes its label immediately. |
| Auto (nearest_center) | Label one actual visible marker. Keep a valid current target until a challenger is more than 15% closer to screen center. Invalid or disappeared targets lose ownership immediately. |
| All (all) | Label all actual visible retained markers, within the shared cap. |

Display distance uses the original anchor distance, subtracts the amount below,
clamps any result at or below zero to **0 m**, then rounds to whole meters.
The display-only height lift never enters this calculation.

| Category | Display-distance subtraction |
| --- | ---: |
| Treasure, including mini-game reward chests | 1 m |
| Area Quest | 1 m |
| Mole mini-game | 2 m |
| Fly / Wave mini-game | 0 m |

Scene markers carry explicit Fly/Mole/Wave subtype information; a reward chest
does not receive the Mole correction.

F6 Escape handling is separate from Scene rendering. While F6 is open, Esc
closes the entire page, including an expanded language popup or a focused or
dragged slider. Esc, F6 and the Close control sample final controls before
closing, publish a changed final sample and flush pending preferences.

The input guard verifies the current-process foreground UnrealWindow and its
nonzero thread before the panel enters the viewport. A thread-local
WH_GETMESSAGE hook consumes Escape key/character messages before dispatch;
WH_CALLWNDPROC observes only owner focus, activation and destruction.
The same consumed press stays owned through repeat and key-up after UMG closes;
the next independent press restores ordinary routing. Other keys and other
windows remain outside the filter. Failed hook installation rejects opening.
Focus loss requests full closure; Travel, owner loss and teardown clear state.
Scene itself captures no input and adds no object discovery.

Compact Boss / Assault / Area reference sizes remain 35 / 30 / 25, with shared
visible stroke weight, the existing player-root correction and inclusive ±5 m
aligned band. Boss/Assault use authored spawn height. F6 retains its responsive
680×896 reference layout, three visibility columns, range/count sliders, five
compact height controls and eleven languages. SG-03 canonical Korean and
Traditional Chinese overlays cover 156 verified codepoints.

## Verified SG-03 build

The Windows PowerShell 5.1 pinned clean build completed all **446 targets** at
2026-09-09T04:53:44.6368020Z.

| Identity | Value |
| --- | --- |
| Version / runtime label | 3.0.0 / DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_3_0_0 |
| Native DLL size | 1,194,496 bytes |
| Native DLL SHA-256 | 4C0B9E788E35A6E46F45B4B5AB6EFCF925B3CDB6CF9E3F3D70E86EB13A853E32 |
| Compiled-source SHA-256 | F4039912D2A97908B5AAF7083E2254B932EFC0EA41BDA6077FAB99FA5C69C0D5 |
| Release-tools SHA-256 | E164FCF7AF93F036B26DC7E0BE12AC60B885A866689A9254A19D8B3D7B8D8138 |

- Core CTest: **5/5**, including **289 Scene checks** and **50 Escape checks**.
- Compact, world-map, PostRender/runtime and release-hygiene gates pass.
- F6 strings, geometry, pinned font/source inputs and canonical overlay assets
  pass, including the **156-codepoint** inventory.
- Escape tests use hidden windows owned by the test process and a real Windows
  message loop. They verify consumption before the target window procedure,
  child-window handling, other-window/key passthrough, peeks, repeat/release,
  synchronous focus notification, owner destruction and teardown. They do not
  change the foreground window, inject physical input or control the game.

These checks establish source/build and isolated Windows routing behavior.
They do **not** establish live gameplay behavior. Installer/manual matrices,
archive hashes, payload equivalence and installed identity belong to their own
transaction receipts in Release status and
`dist/final-3.0.0/release-manifest.json`.

## SG-03 owner acceptance checklist

Record the DLL identity, relevant settings, reproduction steps and visible
result with each finding. Include the target and F6 settings in a screenshot or
short clip when useful, and note whether an issue persists with Scene off.

1. **Independent visibility and limits.** Enable each Scene category separately
   and together. Exercise range 0 / 600 / 1000 m and count 0 / 24 / 50. Confirm
   either zero hides Scene and disabled categories take no slots. Behind-camera,
   offscreen and crowded targets must not leave orphan distance labels.
2. **Glyphs and displayed height.** Check task badges, four chest colors,
   purple flags and smaller labels against bright/dark backgrounds. Inspect
   previously floating/buried quest points. The display lift must not move
   minimap/map coordinates or claim an unconfirmed quest anchor.
3. **Distance near targets.** Approach a chest, Area Quest, Mole, Fly and Wave.
   Confirm 1 / 1 / 2 / 0 / 0 m subtraction, zero rather than negative results
   nearby, and whole-meter rounding. Mini-game reward chests use chest rules.
4. **Aim timing.** Hold the same visible target inside the smaller central
   circle for 120 ms. Briefly cross a target, change the nearest in-circle
   target, or move it outside the circle/view. Check the dwell resets on a
   target change and the previous label disappears immediately when invalid.
5. **Auto switching.** Move the camera slightly between nearby targets. Retain
   the current valid target until a challenger is more than 15% closer to
   center. Remove that target from view/eligibility and check stale focus is
   released. Aim uses dwell; Auto uses this switching buffer.
6. **Off and All.** Off leaves icons without labels. All labels only actual
   visible retained markers. At the 50-marker limit, inspect text overlap,
   integer changes, screen edges and rotation for stale labels or flicker.
7. **Esc closes only F6.** Test a normal page, expanded language popup, focused
   slider and active slider drag. First Esc must close the whole F6 page without
   also opening/closing a native menu or triggering a game action. Repeat above
   an already-open game map/menu. Hold Esc through close, then release: repeat
   and release must not leak. The next independent Esc restores game behavior.
8. **Final values and focus loss.** Move each slider and immediately close with
   Esc, F6 and Close. Reopen to check the last value, then restart normally to
   check persistence. Switch away and back while holding Esc: F6 should close
   safely without stuck input or a delayed escaped press.
9. **Eligibility and completion.** Complete ordinary treasure/task/mini-game
   objectives through normal play. Their eligible markers and labels must
   disappear. Exercise AVAILABLE/ALL task views and a repeatable active cycle
   without changing established completion/cooldown semantics.
10. **Lifecycle and menus.** Check first enable, F8/F7, F6 while Radar is Off,
    travel, return to title/load another save, native map open/close, controller
    menus and photo mode. Look for stale hosts, labels, input ownership, flicker
    or unexpected changes to compact/world-map behavior.
11. **F6 layout and languages.** Switch manual languages and AUTO, reopen the
    language popup and test smaller/larger viewports and DPI. Check controls
    remain reachable, text stays inside the frame, Korean/Traditional Chinese
    overlays match settings/status, and input remains usable.
12. **Compact height and longer play.** Compare Boss/Assault/Area glyphs and
    triangles on both sides of the ±5 m boundary, including unknown heights.
    During longer play, compare Scene Off, Aim and All in dense areas for frame
    timing, memory growth, stale labels and failures affecting another renderer.

Each item remains pending until the owner records its result for these SG-03
bytes. Static UI previews, model tests and hidden-window Escape tests cannot
substitute for gameplay, controller, visual or performance acceptance.

## Historical SG-03 package and deployment handoff

**SG-03 local delivery and deployment were verified.** The final manifest was
generated at 2026-09-09T05:00:35.6146355Z; installed verification completed at
2026-09-09T05:08:44.4698483Z. Both identify the final DLL listed above.

- The three ZIPs contain 4 / 43 / 47 entries for Installer / Manual-No-UE4SS /
  Manual-With-UE4SS. Re-extraction is byte-identical; Setup and manual payloads
  are equivalent, with 35 non-user payload files and four configuration/default
  files in the 39-file runtime manifest.
- Core 5/5, Setup 20/20, Manual 2/2, 177 hotkey assertions and 414 visibility
  assertions pass. The independent archive audit records **1,548 checks and
  zero failures** in repository-root
  `out/handoff/SG03_INDEPENDENT_ARCHIVE_AUDIT.json`.
- Installed verification confirms all 35 non-user payload files match the
  package. All four existing user files are byte-preserved, exactly one Radar
  load entry remains, and the game was not running during verification.
- The complete deployment backup is
  `dist/work/deployment/deploy-backups/20260908-220537-090-native-only-deploy`.
  The candidate evidence directory
  `dist/work/candidates/radar-3.0.0-sg03-20260908/` retains
  `local-deployment.log` and `installed-verification.json`.

Use [Release status](RELEASE_STATUS.md), the
[Scene guidance ledger](SCENE_GUIDANCE_ATTEMPT_LEDGER.md), and
`dist/final-3.0.0/release-manifest.json` / `SHA256SUMS.txt` for exact archive,
payload and transaction identities. Package/deployment success does not complete
the owner gameplay, visual, controller or performance checklist above.

Nexus changes are **local copy only**: no upload or post was performed. The
publication-clearance record in metadata/release.json is unchanged. Historical
2.x packages and SG-02 evidence retain their identities. The local Description,
Quick Support, Changelog and Files text is available through the
[local publishing index](../assets/nexus/README.md).
