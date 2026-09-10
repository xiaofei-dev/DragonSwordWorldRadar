# Native World Radar - 3.0.0 local release handoff

## Publication and support update - 2026-09-10

The owner reports Nexus publication of 3.0.0, supported by the supplied
file-page screenshots. The local release identity remains Guide4 below;
remote downloads were not independently hash-compared. Historical statements
that no Nexus upload was performed describe those earlier local build steps.

Pending user reports:

- Previously opened chests remain marked at some locations. Startup already
  reads saved completion bits; mid-playthrough installation alone is not an
  expected limitation. Exact locations/version and a debug log are requested.
- Settings and clock appear at first world entry but markers appear only
  after fast travel. Initialization timing or other failures remain hypotheses;
  Radar and UE4SS logs are requested. A Mod conflict is not established.

The existing `data/defaults/treasure_overrides.txt` accepts `ignore <save ID>`
and is loaded at startup. Current Native 3.0 has no visual chest-ID switch;
the archived external renderer's `diagnostic_verbose` setting is not a Native
configuration option. Easier manual hiding is a future consideration, not a
delivered feature. No gameplay code is changed by this documentation closeout.

## Current Guide revision — activity entry order

Candidate `radar-3.0.0-sg16-guide4-20260909`; version **3.0.0**.

Guide swaps the Marmot mini-game and Sudden mission entries as complete groups: the text and both Radar/Map icons move together. The left column now reads Flying mini-game, Wave mini-game, Marmot mini-game and Bird eggs; the right column reads Sudden mission, World boss and Area quest. All eleven languages use this order. The three height columns, 40 icon examples and 539 text cells remain unchanged.

This is a layout-only asset revision of `radar-3.0.0-sg16-guide3-20260909`. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. No native code or localization-header change is included, and no new native compilation is claimed.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T02:04:37.4453785Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T02:06:49.3883924Z`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Installer.zip` | 16,036,260 | `C7DE271638719DB38D7335774F374976CC572E7FBB12D1D37E14AA72901C47F8` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-No-UE4SS.zip` | 8,004,000 | `DAD1C941CE3C893762065F79917917CE55C40E91E5D3819B2BE935EE402183EF` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | 16,029,635 | `8F52CCE27530508E0624B717DB6954CE8EC38DE7F8A9C82A1668AB8DDE476AD3` |
| `DragonSwordNativeWorldRadarPostRender-Setup-3.0.0.exe` (inside Installer ZIP) | 27,262,976 | `C1F1C895769A7E52D79728A194AD5D852C0B278D39D89EAE279F8D4377BF1C94` |

Manifest UTC: `2026-09-10T02:06:02.2485470Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/package-verification.json).

The previous Guide3 and earlier SG-16 sections below remain historical records. Their earlier activity order and ZIP hashes identify those sets. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Current Guide revision — three height columns

Candidate `radar-3.0.0-sg16-guide3-20260909`; version **3.0.0**.

The owner requested a simpler height reference. Guide now displays only Above, Near level and Below, with 15 height examples, 40 total icon examples and 539 text cells across eleven languages. The Unknown column is no longer drawn. Actual unknown-height handling, height switches and all other gameplay behavior are unchanged. The existing 44-string localization schema remains intact.

This is an asset/documentation revision of `radar-3.0.0-sg16-20260909`, not a new native build. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. The parent's 446/446 native and Core 9/9 evidence retains its original identity.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T01:45:25.9749727Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T01:47:39.7016800Z`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Installer.zip` | 16,030,954 | `E847D34B463C5AB12B101CB36CD129E6A42D2E9CBE17D0B8D70B6540315336F4` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-No-UE4SS.zip` | 7,994,761 | `B4D62BE9CD257965ADE07C8F10FBEE154DB47A96D7985D22C45C8B2321009D07` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | 16,020,397 | `7E95FC97CDAB332303CF0D32C2168B94DFD11285A706EC362301944DA27ECC79` |
| `DragonSwordNativeWorldRadarPostRender-Setup-3.0.0.exe` (inside Installer ZIP) | 27,253,760 | `D7CADE457B51354E124F15A3852E8851E733337415D4E9BE561CF0028CBABFD0` |

Manifest UTC: `2026-09-10T01:46:51.6121910Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/package-verification.json).

The previous SG-16 sections below are preserved historical records. Their four-column Guide description and old ZIP hashes describe that earlier set. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## SG-16 build, deployment and release packages - September 9, 2026

**Three local 3.0.0 packages are generated, verified and promoted.** Candidate:
`radar-3.0.0-sg16-20260909`. SG-16 adds live open-map category changes, independent Radar/Map All
controls, default Auto focus and the compact, expanded icon Guide.

Native 446/446, Core 9/9 and all four source/release gates pass. Setup passes 20/20 and Manual 2/2, with no failures or skips. The three channels have 104 equivalent runtime files, including 73 UI files and twelve Guide files; ZIP entry counts are 4/108/112.

Local deployment was verified at `2026-09-10T00:22:37.0293741Z`. The installed and packaged DLL is the same 1,325,056-byte file, SHA-256 `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`. The deployment receipt verifies the native receipt and all 73 UI files, preserves four settings/data files and records 2 unchanged AutoPickup files.

Native receipt UTC: `2026-09-10T00:08:45.8205296Z`.
Compiled source SHA-256: `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`.
Release-tools SHA-256: `AC44E446C30BF0400D3E8B40BFFB7036A711F00618CFFD1737145209B05AFF7F`.
Manifest UTC: `2026-09-10T00:22:33.1489987Z`.
Final promotion UTC: `2026-09-10T00:23:01.4155094Z`.
Final directory: `dist/final-3.0.0/`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Installer.zip` | 16,070,060 | `4718586DE1902CB617908A1307C23F3153BD34C43307BEC227B4AD742C9762E6` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-No-UE4SS.zip` | 8,038,485 | `86F650972C441032BE439C29ECB2CE9BFEA5712920D2461DF6FCB9CB2D16C3D5` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | 16,064,122 | `F7E9B6AB21C734D170A42D4DBACC38A115F7032E8EA1AC19DB36D23AF4A55597` |
| `DragonSwordNativeWorldRadarPostRender-Setup-3.0.0.exe` (inside Installer ZIP) | 27,297,280 | `388FF2CDE9DE88BF85C778A3FFE4239DA72F07D7F5B98D0A418DD98A70C456CE` |

Fresh extraction is byte-identical. Independent promotion verification compares
the embedded Setup payload with both manual payloads. The five previous SG-15
final files are backed up and hash-verified at `dist/work/candidates/radar-3.0.0-sg16-20260909/previous-final-sg15`. Historical SG-15
receipts below retain their original artifact identities.

First Aim acquisition is 100 ms; Aim/Auto target replacement remains 350/500 ms.
The separate open-map coalescing interval is 100 ms, and disabling every Map
category hides the overlay immediately. If another event captures a marker
snapshot during that interval, the old comparison baseline is invalidated so
returning a checkbox to its previous value cannot leave an intermediate atlas.

README and both installation guides were finalized and frozen before the final package build. They describe the product and its controls; these external receipts record exact build, deployment and delivery results. The payload documents and metadata were not rewritten after sealing.

Evidence: [validation summary](../dist/work/candidates/radar-3.0.0-sg16-20260909/validation-summary.json),
[deployment verification](../dist/work/candidates/radar-3.0.0-sg16-20260909/deployment-verification.json),
[release manifest](../dist/work/candidates/radar-3.0.0-sg16-20260909/release-packages/release-manifest.json)
and [promotion verification](../dist/work/candidates/radar-3.0.0-sg16-20260909/package-verification.json).

Build, source/resource checks, installation and package checks are verified; they do not establish game visual, input, Guide usability, camera-motion or measured performance acceptance. Those remain owner testing. No Nexus upload or post was performed.

## SG-15 release packages - September 9, 2026

**Three local 3.0.0 packages are generated, tested and promoted.** Candidate:
`radar-3.0.0-sg15-release-20260909`. Manifest UTC: `2026-09-09T23:22:15.6900550Z`.
Final promotion UTC: `2026-09-09T23:23:40.0349295Z`. Final directory:
`dist/final-3.0.0/`.

Canonical native build 446/446 and source/build/package validation pass.
Setup passes 20/20 and Manual passes 2/2, with no failures or skips. All three
channels contain the same 104 runtime files, including 73 UI files and the
twelve Guide files. Installer/No-UE4SS/With-UE4SS ZIPs have 4/108/112 entries.
Fresh extraction is byte-identical, and the promotion check independently
hashes the embedded Setup payload and both manual payloads.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Installer.zip` | 15,276,092 | `3FB7ABA94C6FB67817EC5C768C2666337545C8A813E4A71F4DEB031D43894FD0` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-No-UE4SS.zip` | 7,243,929 | `5C8F09178A2C7BDA1696DBE0EF92F9DD87A16F917CCE3123DD24BF0D2469E0F8` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | 15,269,571 | `E45902756E737C2D2B5709BEEE600586FCACE6A6839DA58AD9BB9A5D0CA09BB5` |
| `DragonSwordNativeWorldRadarPostRender-Setup-3.0.0.exe` (inside Installer ZIP) | 26,503,168 | `7A5DDF70E50BAE8B4DCC60CA078152D4A34E711E2F8E10BC5B7CAE00974777FB` |

Release DLL: 1,316,352 bytes,
SHA-256 `F2C1E703D85926C96740CFF0E49DD183C2A4F43D3D206611163117C2C6334652`.
Compiled source SHA-256: `CCC2DDF329763986C4AD9284DC76EB39E31CF7B98644AB3FA1BCA1126D877E73`.
Release-tools SHA-256: `8C08E036655ED69B833BA4787F909BD32FCC8353C3F53B7302D3C0DB63A2D176`.
Native receipt UTC: `2026-09-09T23:15:23.1918294Z`.

The release build uses the same compiled source as the already deployed
SG-15 runtime but has a different rebuilt DLL hash. It was not redeployed into
the game. The existing installed DLL remains
`FFD4703F7A147C9D57C7132C7342CA881BC7CD48CC8682F282276E44B16C851B`, verified at
`2026-09-09T22:40:04.2927240Z`. Its original deployment receipt and section
below retain their identity; they do not establish installation of the new DLL.

The previous three SG-10 ZIPs, release manifest and SHA256SUMS.txt are backed up
and hash-verified at `dist/work/candidates/radar-3.0.0-sg15-release-20260909/previous-final-sg10`. The first package attempt was superseded after
correcting stale current-state metadata; only the final manifest above defines
this delivered set. Packaged metadata and the three payload documents were
frozen before package tests. Their pre-package snapshot does not replace these
later test and promotion receipts, and has not been rewritten after sealing.

Evidence: [release manifest](../dist/work/candidates/radar-3.0.0-sg15-release-20260909/release-packages/release-manifest.json)
and [independent promotion verification](../dist/work/candidates/radar-3.0.0-sg15-release-20260909/package-verification.json).
Guide, Settings, controller, camera-motion and performance acceptance for these
exact release bytes remain owner testing. Nothing was uploaded or posted to
Nexus, and this local delivery does not establish external publication clearance.

## SG-15 localized icon guide - September 9, 2026

Candidate `radar-3.0.0-sg15-20260909` adds Guide beside Close in F6. The static guide
explains treasure colors, Radar/Map activity symbols, height arrows, Scene
markers and distance modes in all eleven languages. The 1120-reference-unit
body uses eleven 1520×2368 atlases, with a separate Settings return label.
Both pages keep their own scroll position while the menu remains open and
reuse the fixed header/footer. Missing guide artwork disables only Guide or
returns to Settings. No additional gameplay polling service is introduced.
SG-14 typography, SG-13 focus switching and H2 menu compatibility are retained.

Core 9/9 and native clean build 446/446 pass. Compact, WorldMap, PostRender and
Release Hygiene gates pass; the final candidate logs record each result.
All 73 UI files match the validation inventory, including eleven guide atlases
and their manifest. The 61 SG-14 UI files remain byte-identical. Source review
checks page state, clipping and confirmation input ownership; it does not
establish game scrolling, controller or visual acceptance.

Deployed and read-back verified at `2026-09-09T22:40:04.2927240Z`. The
1,316,352-byte DLL SHA-256 is
`FFD4703F7A147C9D57C7132C7342CA881BC7CD48CC8682F282276E44B16C851B`.
Compiled source SHA-256: `CCC2DDF329763986C4AD9284DC76EB39E31CF7B98644AB3FA1BCA1126D877E73`.
The installed DLL, build receipt and all 73 UI files match. Four settings/data
files and 2 checked AutoPickup files remain byte-identical.
Backup: `dist/work/deployment/deploy-backups/20260909-154001-516-native-only-deploy`.
The [deployment receipt](../dist/work/candidates/radar-3.0.0-sg15-20260909/deployment-verification.json)
and [validation summary](../dist/work/candidates/radar-3.0.0-sg15-20260909/validation-summary.json)
record the completed checks. Game visual, input, scrolling and camera-motion
acceptance remain pending owner testing. No release package was rebuilt;
all three final SG-10 ZIP names and SHA-256 hashes remain unchanged. Nexus text
remains a local draft, with no upload or publication.

## SG-14 settings typography and compact confirmations - September 9, 2026

Candidate `radar-3.0.0-sg14-20260909` uses natural label capitalization, shared font
baselines and fixed 22/16/14/13/12 reference-pixel text roles. Blue-gray surfaces
reduce scene interference. The 380×184 rounded confirmation removes the old
1.6-times body enlargement; text and hit regions match, with No left and Yes
right. SG-13 distance switching, H2 optional-ABI compatibility, all settings
behavior and six Scene sprites are retained.

Core 9/9 and native clean build 446/446 pass. Compact, WorldMap, PostRender and
Release Hygiene gates pass. The UI verifier checks all 11 languages, 1464
main/popup slots, exact glyph pixels, common baselines, safe crops and fixed
font sizes. Main/help/confirmation glyph coverage is 443/748/227. Material
checks cover 118 text states and keep small text above 4.5:1 on a worst-white
linear-light backdrop. Source-layout checks and actual-resource previews cover
640×360 through 4K, including 2560×1080 and 3440×1440; these are not game captures.
Four old font-size assertions initially rejected the revised layout; source
checks were synchronized and the final native build was made afterward.

Deployed and read-back verified at `2026-09-09T22:07:41.4623531Z`. The
1,296,384-byte DLL SHA-256 is
`E3350809E50ABA8C3A821785F9E5FE0E8C93BE4A544D653A061BE02A1801255A`.
Compiled source SHA-256: `E56789C4BE9FA113AF1C3BDB165AA73E2AC5C959F5AA914EC61F0D907F41F168`.
All 61 installed UI files and the build receipt match. Four user settings/data
files and 2 checked AutoPickup files remain byte-identical.
Backup: `G:/my_projects/game_mods/DragonSword/DragonSwordNativeWorldRadarPostRender/dist/work/deployment/deploy-backups/20260909-150739-013-native-only-deploy`.
The [deployment receipt](../dist/work/candidates/radar-3.0.0-sg14-20260909/deployment-verification.json)
records installation. Visual, input and camera-motion acceptance remain pending
owner testing. No package was rebuilt; all three final SG-10 ZIP hashes remain
unchanged. Nexus text remains a local draft.

## SG-13 distance focus stabilization - September 9, 2026

Candidate `radar-3.0.0-sg13-20260909` keeps the acquired distance label visible
while a clearly better challenger settles for 350 ms in Aim / 500 ms in Auto.
Initial Aim acquisition stays 120 ms. An acquired Aim target receives a small
exit margin. Projection, assets, settings and the verified H2 menu fix remain.
Core 9/9, Scene 382 checks, native clean build 446/446 and all four deployment
gates pass. The Scene regressions include 30/60/144 FPS camera sweeps, exact
replacement timing, lost advantage, reordering, mode changes and target loss.
The initial test run exposed three fixture issues (an unintended third
challenger and a floating-point threshold fixture); corrected tests pass.

Deployed and read-back verified at `2026-09-09T21:33:45.4114616Z`. The 1,295,872-byte
DLL has SHA-256 `F06B232275CCF4715991EA6C66135F8B35745B1154545D3D27C95BD7A82D8C84`.
All 61 UI files and the native build receipt match; four user settings/data
files and AutoPickup remain unchanged. H2 is backed up under
`dist/work/deployment/deploy-backups/20260909-143342-728-native-only-deploy`.
The [deployment receipt](../dist/work/candidates/radar-3.0.0-sg13-20260909/deployment-verification.json)
records the installation. Game feel remains pending owner testing. No release
package was rebuilt, and Nexus changes remain local drafts.

## SG-12-H2 current hotfix - September 9, 2026

H1 remains rejected: actual F6 diagnostics report ABI bit 30. A live read-only
reflection probe shows that the shipped game has no UMG `SetScrollBarVisibility`
function. H2 preserves the default scrollbar appearance when that optional
setter is unavailable or incompatible, while retaining required core ScrollBox
ABI checks. Its sole call is guarded. The ten-case viewport gate includes
regressions for making the setter mandatory or calling it without checking.
UI files and preferences are unchanged. Build/deployment/game evidence follows;
no release package is requested.

| SG-12-H2 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg12-h2-20260909` |
| DLL bytes | 1,295,360 |
| DLL SHA-256 | `7B017E4339D149AE86F6871D8339411CE55CCD29790C7331B2AA95B9FE7F2284` |
| Compiled source SHA-256 | `87015B737377D94D9C24AFCD79F14063636EE6539BCBC6C3D268ACF69AFCEB23` |
| Release tools SHA-256 | `491BDD68A4601CA11CC37455E11523B8E046E88B4E94C1A6BE7E602B48A05803` |
| Canonical receipt UTC | `2026-09-09T21:01:02.2364431Z` |

Native clean build 446/446 and the ten-case viewport source gate pass. All 61 UI
files match SG-12/H1. H2 was deployed at `2026-09-09T21:04:10.3891696Z`.
The owner subsequently confirmed menu opening; two OPENED log records report
zero font/text failures. This does not establish all-resolution interaction or
performance acceptance. Diagnostics were restored to their original disabled
configuration; the change takes effect on the next game start.

## Historical SG-12-H1 hotfix - September 9, 2026

SG-12 failed owner acceptance because F6 would not open. The new ScrollBox
initialization checked two Slate parameter names instead of the UMG names,
causing the required ABI check to disable the Hub. H1 corrects `NewOrientation`
and `NewScrollBarVisibility` and adds a negative regression for each old name.
All size/type/offset checks remain. The eight viewport source regressions pass;
UI assets, preferences and Scene rendering are unchanged. Native clean build
446/446 passes. Game-open acceptance remains pending; deployment is recorded below.

| SG-12-H1 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg12-h1-20260909` |
| DLL bytes | 1,295,360 |
| DLL SHA-256 | `B578566C881B2C5E5FDE0CE4FE88975366E404EB5F39DAF7BB460906B64A82A3` |
| Compiled source SHA-256 | `ACE81EC10EDB134B8E1AA23B1A664320AB70E0BF643CB20EC0E349E06D93419F` |
| Release tools SHA-256 | `04F651ED796C9EEF11E3CABEBEDA4613F4ABDD801EE440A98614885E7F83781E` |
| Canonical receipt UTC | `2026-09-09T20:33:25.4406627Z` |

All 61 UI resource hashes match the frozen SG-12 snapshot. The original SG-12
font previews retain their original source identity and are not H1 previews.
No release package has been rebuilt.

H1 was deployed and read-back verified at **2026-09-09T20:40:45.7209511Z**.
Core 9/9 and all four deployment gates passed, including the corrected eight-case
viewport regression gate. Installed DLL, build receipt and 61 UI files match;
four settings/data files and both AutoPickup DLL/INI files are unchanged.
The prior installation is backed up under
`dist/work/deployment/deploy-backups/20260909-134043-078-native-only-deploy`.
See the [H1 deployment receipt](../dist/work/candidates/radar-3.0.0-sg12-h1-20260909/deployment-verification.json).
All three final ZIP hashes remain the SG-10 baseline. Owner F6 retest is pending.

## Historical SG-12 candidate - September 9, 2026

SG-12 standardizes regular-weight fonts across all eleven languages, including
all page states, popup labels, hover help, confirmations and dynamic values.
The footer actions share the same reference font size. Vote for This Mod means
Nexus Mod of the Month voting; confirmation opens mod 254's page, leaving the
actual vote on Nexus to the player. Feedback still opens Posts.

Header and footer remain fixed while the body scrolls on short displays. The
retained widget tree adapts to viewport/DPI changes within 250 ms, preserving
settings and scroll position. Numeric layout checks cover 640x360 through 4K,
including 2560x1080 and 3440x1440. Static previews do not establish game input
or compositor behavior.

Scene markers keep per-frame camera updates with a calibrated batch path and
native fallback. Both projection depth axes and a retained marker are checked;
orthographic views retain native projection. Changed subpixel translations,
volatile marker groups and modest edge/overlap hysteresis remove avoidable
motion stepping without interpolation from an older camera pose. Actual
game-frame timing and camera-motion acceptance remain pending.

| SG-12 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg12-20260909` |
| DLL bytes | 1,295,360 |
| DLL SHA-256 | `56ADC902BED3C468D5E933F2D03A81850A9FEB7226A27D6702A04153716A1961` |
| Compiled source SHA-256 | `37E0263E4C68CF1A2E5DE331B336B0E9D8B7EB41A150C6DDE9AA807DFB028C43` |
| Release tools SHA-256 | `CFC6A45D50FD3D46236988091D3668F23B16D280C2A3DF7DE3BB22CD0C7EE5E1` |
| Canonical receipt UTC | `2026-09-09T19:58:58.1322624Z` |
| F6 manifest SHA-256 | `07E4385D2FF74A92932D804625652C17A4F6C639EBAD50530BAC1ABD1C9043F6` |

Core **9/9** and native **446/446** pass, including Scene **319**, projection
**39**, viewport **83**, clock **28**, Escape **58** and confirmation **55**
checks. Compact, World Map, PostRender and Release Hygiene gates pass. Resource
checks reject **43** F6 regressions; Compact rejects six layout regressions;
PostRender retains ten Scene and twelve confirmation regression cases. The
font inventory covers **423/750/250** main/help/confirmation codepoints. There
are **53 F6 TGAs**, six Scene TGAs and two manifests: **61 UI files**, 91 payload
entries and 92 runtime files. Twenty-one source-bound font previews were
regenerated against the final Hub source and manifest. Preview and build
verification remain separate from game appearance and input acceptance.

SG-12 was deployed and read-back verified at **2026-09-09T20:18:13.3037580Z**.
The installed DLL matches the build above; all 61 UI files and the build receipt
match the candidate. Four existing settings/data files and both AutoPickup
DLL/INI files were preserved byte-for-byte. The previous SG-10 installation was
backed up under `dist/work/deployment/deploy-backups/20260909-131810-275-native-only-deploy`.
The machine-readable receipt is
[`deployment-verification.json`](../dist/work/candidates/radar-3.0.0-sg12-20260909/deployment-verification.json).

SG-11 below was built only and superseded before deployment. SG-12 release
packages are explicitly deferred until owner testing; all three existing final
ZIP hashes were checked unchanged and remain SG-10. Gameplay acceptance is pending.

## Historical SG-11 build-only candidate - September 9, 2026

All eleven languages now use short hover help and single-question confirmations.
Reset to Defaults, Endorse and Feedback move to a separate footer beneath the
four unchanged settings cards. The header contains only the title and Close.
The reference panel grows to 760 x 852, retaining viewport/DPI fitting. The
language popup's dismissal shield covers the footer as well. The shared activity
height label now says Mini-games; its existing scope and stored key are unchanged.

| SG-11 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg11-20260909` |
| DLL bytes | 1,272,320 |
| DLL SHA-256 | `455E401DD327B430D699638D73DA7F8E4FEE1B80E11330DCFC04997FDE7B8CB4` |
| Compiled source SHA-256 | `851038127E39E109BCC20A2CF1F43A6C18C8F03AF976F3F0AC8B3E15FF225CB1` |
| Release tools SHA-256 | `37054861E53BE865188F027C800B9219E924874FC02441B077827B9DBA42958F` |
| Canonical receipt UTC | `2026-09-09T17:21:07.0477550Z` |

Core 7/7 and the clean 446-target native build pass. Compact, World Map,
PostRender and Release Hygiene gates pass, including the existing ten Scene
and twelve confirmation regression checks. F6 validation retains 27 asset
regression cases; required glyphs are 180 labels, 760 tooltip and 245 confirmation.
The 118 text/control states meet the existing contrast thresholds. Eleven
source-bound previews cover full pages, popups and confirmations; the reviewed
English page and Chinese confirmation match the frozen native source and F6
manifest. The 26 F6 TGAs, six Scene TGAs and two manifests remain 34 UI files.

SG-11 was superseded by SG-12 before deployment or packaging. Its DLL, receipt
and three metadata files are retained in the candidate's `build-snapshot` folder;
its old UI resources were not copied after the SG-12 resources changed. SG-10
remained installed at that handoff and its final packages remain unchanged.
There are no SG-11 deployment or package receipts.

## Historical SG-10 candidate - September 9, 2026

All six SG-09 corrections are retained. Restore Preset, Endorse and Feedback
now ask for confirmation. Only Yes resets the preset or opens the fixed Nexus
destination; No or Esc cancels the pending action without changing settings or
launching a browser. Endorse opens mod 254's homepage; Feedback opens its Posts
page. No vote or post is submitted. Background settings are disabled while a
confirmation is visible; Escape repeats do not also close F6. Losing application
focus closes F6 and clears the pending action.

| SG-10 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg10-20260909` |
| DLL bytes | 1,310,208 |
| DLL SHA-256 | `F1203366DA7488FBFC9E8BF101598A4FC4D2E840F850D66CEC487452FEC7EB65` |
| Compiled source SHA-256 | `C999BB82AD356F1C38421868E2A242B7BA0223A2ACD0595ED658829A9A5C380D` |
| Release tools SHA-256 | `CE5E1B2F5CFE20633EAF0A5B9DB0497A90EEB905495B7A3BB791E97BF9EAD85B` |
| Canonical receipt UTC | `2026-09-09T16:43:53.6047126Z` |

Core **7/7** passes, including Scene **313**, Clock **28**, Confirmation **55**
and Escape **58** checks. Clean native compilation completes **446 targets**.
Compact, World Map, PostRender and Release Hygiene gates pass; PostRender rejects ten deliberate
Scene regressions and twelve confirmation transaction regressions. F6 assets
reject 27 deliberate variants. Each language atlas contains 32 help and seven
confirmation tiles at 640 x 5616; 26 F6 TGAs and six Scene TGAs still produce
34 UI payload files with their two manifests. Source previews were reviewed
separately from game input and visual acceptance.

SG-10 deployment was verified at `2026-09-09T16:51:20.6801835Z`. Installed DLL,
build receipt and all 34 UI files match the candidate. Visibility, diagnostics,
hotkeys and Treasure overrides preserve their exact previous bytes; AutoPickup's
two DLL/config files are unchanged. The SG-09 installation is backed up at
`dist/work/deployment/deploy-backups/20260909-095118-801-native-only-deploy`.

The three packages were generated at `2026-09-09T16:53:04.9764235Z` and promoted
to `dist/final-3.0.0` at `2026-09-09T16:55:11.2153219Z`. Setup passed **20/20**
cases and Manual passed **2/2**, with no failures or skipped cases. All three
ZIPs were re-extracted byte-identically; both manual archives match all 64
payload files. Their three default configs use installed `.ini` names rather
than Setup's `.example.ini` names, with identical bytes. Setup/manual payload
equivalence passes. Each runtime has 65 files and 61 manifest members; 60
immutable installed files also match the packaged candidate. The older five
SG-03 final files were copied and hash-verified at candidate `previous-final-sg03`
before replacement.

| Final archive | Bytes | Entries | SHA-256 |
| --- | ---: | ---: | --- |
| Installer | 16,192,958 | 4 | `45B116E7A9743A6D82D94B83D801A3A7BFF85D889B892534C47718CB747DC18E` |
| Manual-No-UE4SS | 8,137,292 | 69 | `0192A756520240044FB38CA9DAA2F6D6153F7F360F8AF212439CDDA88AE89D52` |
| Manual-With-UE4SS | 16,162,931 | 73 | `2447F070DACA466E77633A45CBA766F00C60D576F5A3C6EA44D17F742C5D5AD9` |

Setup is 27,395,072 bytes, SHA-256
`E8C53FA6F95BC1804571EBEFFDA86E625815BB36A09227CA1D09A4BC7220A77D`.
Candidate `deployment-verification.json`, `package-verification.json` and
`release-packages/release-manifest.json` retain the exact evidence. Packaged
metadata deliberately remains a predeployment build snapshot; the subsequent
independent receipts record installation and archive identity. Build-Release's
`deployment=NOT_PERFORMED` describes that packaging command only, not the
separate successful deployment. Local Nexus drafts are synchronized. Owner game,
visual and performance acceptance remains pending; no external publication occurred.

## Historical SG-09 candidate - September 9, 2026

The owner authorized the six follow-up fixes, local deployment, all three
3.0.0 release packages and local Nexus documentation. SG-09 adds a translucent
backing to the Area Quest diamond, raises Treasure UI projection to 160 cm,
centers the clock in the measured minimap/quest gap, keeps live Radar previews
while F6 owns gameplay input, projects Scene markers each engine frame using
render translation, and changes the settings palette to soft blue-gray.

| SG-09 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg09-20260909` |
| DLL bytes | 1,299,456 |
| DLL SHA-256 | `EC82CA04927ABCB3148409AEC16D6574641984E4A9F94790515D2B94F800F1BF` |
| Compiled source SHA-256 | `9AE2769E0D255A2AF1400123031D395E4993866264DEE919CFD4B3773458CD3B` |
| Release tools SHA-256 | `83317A019F65844ADCA377E930B9F205B1A1608F0E7C09E203876875ADEB80BA` |
| Canonical receipt UTC | `2026-09-09T15:43:06.5903373Z` |

Core **6/6** passes, including Scene **313 checks**, Clock **28 checks** and
Escape **50 checks**. The clean native build completes **446 targets**.
Compact, World Map, PostRender and Release Hygiene gates pass. PostRender
also rejects ten deliberate regressions of per-frame Scene work and the
owned-settings preview boundary. F6/Scene asset verification and source previews
remain separate from gameplay acceptance.

The Area Quest sprite keeps its outline, dots, direction tip and footprint;
the other five Scene textures are unchanged from SG-08. The six shared Scene
and 26 F6 TGAs retain the same runtime inventory. New visual checks reject 21
cases (14 F6 materials, seven task backings); two separate native fallback
palette regressions are also rejected. Linear-light small-text contrast is
at least 4.522:1 for texture reading states, and 4.613:1/5.266:1 for native
utility/content fallback. These calculations do not validate HDR or the game
compositor. No live blur is added.

Scene catalog selection stays at 250 ms. At most 50 selected distances are
refreshed in a linear pass each frame, and render translation replaces Canvas
slot movement with cumulative 0.25-pixel caching. The clock uses the existing
1 Hz service, exact owned HUD widgets and completed Slate geometry; its visible
center is Y=15 in the 42-unit container. A fitting gap of at least 30 reference
units is required, with a 178-reference fallback if evidence is unavailable.
Raw world coordinates, height classification, distance correction/clamping and
native menu/HUD suppression remain intact.

Deployment completed at `2026-09-09T15:52:57.0256520Z`. Installed DLL and build
receipt match the SG-09 candidate; all 34 UI files were verified. Visibility,
diagnostics, hotkeys and Treasure overrides preserve their exact predeployment
bytes. AutoPickup's two DLL/config files remain unchanged; Native Radar is enabled.
The previous SG-08 installation and mods.txt are backed up at
`dist/work/deployment/deploy-backups/20260909-085255-045-native-only-deploy`.
See candidate `deployment-verification.json` and `deployment.log`.

Three SG-09 packages were generated and verified in the candidate directory,
but were superseded by the owner's SG-10 request before final promotion. Build-time metadata is a predeployment
snapshot; the subsequent deployment/package receipts are authoritative for
installation and archive identity. Owner game/visual/performance acceptance
and any external publication remain separate.

## Historical SG-08 candidate - September 9, 2026

Current source and local build are **SG-08**, refining settings to neutral dark
translucent surfaces using the owner's menu reference. Module gaps transmit
more of the background while small-text and tooltip surfaces remain deeper.
The existing layout, square checkboxes, 31 help topics, Scene assets, anchors
and height values are unchanged. There is no live blur or refraction.

| SG-08 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg08-20260909` |
| DLL bytes | 1,288,192 |
| DLL SHA-256 | `5E4045E3160E00A5FA234167926E72E318EDF7F92F8A059E5AB8A832D70DD381` |
| Compiled source SHA-256 | `2FC8A4B44415A9B5C42ED444802F1A231387A5D5DBAF9574EA9CC41A0A56D13F` |
| Release tools SHA-256 | `67B37DF590B7118308F70BA56B8D39745721F72C5F02C4ED4159C650769CE394` |
| Canonical receipt UTC | `2026-09-09T14:31:08.5695045Z` |

Core 5/5, clean native 446 targets and the Compact, World Map, PostRender and
Release Hygiene gates pass. The latter also validates the final F6 and Scene
resources. F6 rejects 62 deliberate source/resource/material variants. Pixel
checks cover 116 text/control states across the main page, slider values and
language popup. Small-text contrast is at least 4.535:1 against white in
linear light; the large title is checked separately at 3:1. Card-only samples
remain at 4.662:1 or higher, with 15.294% transmission; gaps transmit
22.353-23.922%. These values account for the actual composited layers.
The fallback palette has 84.6% utility opacity and 87.68% content opacity;
against white, linear-light composition yields 4.643:1 and 5.336:1 contrast
respectively for RGB (247,253,255) text. These are source calculations, not
in-game, HDR or display-pipeline acceptance.

Previews and material review are retained at repository-root `out/handoff/`
(`F6_SG08_*` and `SG08_NATIVE_PALETTE_REVIEW.*`). Candidate files and logs are
in `dist/work/candidates/radar-3.0.0-sg08-20260909`. The owner subsequently
requested deployment. SG-08 is now installed; the existing final ZIPs remain
SG-03. The SG-07 chest-height investigation remains open; no height fix is
claimed. Local Nexus drafts are synchronized; no external publication occurs.

Deployment was verified at `2026-09-09T14:50:01.0367076Z`: installed DLL and
build receipt match the candidate above, and all 34 UI assets match their
recorded hashes. Visibility, diagnostics, hotkeys and treasure overrides retain
their exact previous bytes. AutoPickup's two DLL/config files are unchanged.
Native Radar is enabled. The complete previous SG-05 installation and mods.txt
are backed up at
`dist/work/deployment/deploy-backups/20260909-074959-027-native-only-deploy`.
The canonical deployment script passed its Core and static gates before writes.
See candidate `deployment.log` and `deployment-verification.json`; these
supersede the build-time `NOT_PERFORMED_FOR_SG08` snapshot for installation
status. Game/visual acceptance remains pending; the game was not launched.

## Historical SG-07 candidate - September 9, 2026

Current source and local build are **SG-07**. Every settings control now has a
specific description, and all seven marker names can also be hovered. There
are 31 topics in eleven languages, 55 tooltip owners in the existing 64-slot
pool and 341 tiles in the same eleven atlas files. The main layout, 26 F6
TGAs, Scene glyphs, height values and 65-file future runtime inventory stay
unchanged. The owner-reported chest height issue is under investigation;
**no chest-height fix is claimed or included in SG-07**.

| SG-07 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg07-20260909` |
| DLL bytes | 1,288,192 |
| DLL SHA-256 | `050897A864CB2FAD699BF7A351B58949C60072C9BC648CBF72EDBEB1386637F6` |
| Compiled source SHA-256 | `4A7689300D57F4F065AEF480CB078B420EDA752A36FF4447E17F062CC64C0481` |
| Release tools SHA-256 | `67B37DF590B7118308F70BA56B8D39745721F72C5F02C4ED4159C650769CE394` |
| Canonical receipt UTC | `2026-09-09T13:59:17.4623002Z` |

Core 5/5, clean native 446 targets, Compact, World Map, PostRender, release
hygiene and F6 resource gates pass. F6 covers 1,076 tooltip codepoints and 177
main-overlay codepoints with 74 strings per language; all 42 resource/source
mutants are rejected. Both Compact and PostRender gates additionally reject
ten incorrect setting/topic/hover mappings. The previous native hover, Esc,
language and cleanup contracts remain checked.

Retained candidate files and logs are in
`dist/work/candidates/radar-3.0.0-sg07-20260909`. Full English/Chinese tooltip
previews are repository-root `out/handoff/F6_SG07_COMPLETE_TOOLTIPS_*.png`;
these are source-derived images, not in-game acceptance.
The chest data and UI-anchor audit is
`out/handoff/SG07_TREASURE_HEIGHT_AUDIT.md` at repository root. It found no
static-catalog XYZ mismatch, but the current uniform +100 cm lift does not
measure box-top height and the visible tip sits 16 logical pixels below its
projected icon center. Further correction needs the reported display and
object identity clarified; neither global height changes nor new object scans
were introduced.

SG-07 is not deployed or repackaged. Installed DLL remains SG-05 and the final
three ZIPs remain SG-03. SG-06 is retained as a separate prior candidate.
All current user settings remain untouched. Local documentation/Nexus text is
synchronized; no upload or external publication was performed.

## Historical SG-06 candidate - September 9, 2026

Current source and local build are **SG-06**. This follow-up adds rounded dark
settings surfaces, square Radar/Map checkboxes, a top page-wide preset and
18 explanations in all eleven languages. Clean defaults enable all three
Scene categories; saved explicit choices remain intact. The Area Quest body
is 18% larger with a stronger gray outline, while other glyphs and the small
direction tips keep their geometry. Clock follows Bird Eggs in settings and
sits 16 reference units lower in the HUD. No live blur is added.

| SG-06 verified build | Value |
| --- | --- |
| Candidate | `radar-3.0.0-sg06-20260909` |
| DLL bytes | 1,251,328 |
| DLL SHA-256 | `D1A521ABD1FDE80C1C7108FB06B980A7525E35412A54217AC21CDD25196C1AAC` |
| Compiled source SHA-256 | `E24EEE9A882E1BA8E65619799A9EB0CF445B58125DD83B83A22616BCC6DB5908` |
| Release tools SHA-256 | `EA24111BE1836ACC1EB9E753D703117E0273A8012AE9F0E7D273F457457F8BA5` |
| Canonical receipt UTC | `2026-09-09T13:25:19.6896094Z` |

Core **5/5**, clean native **446 targets**, all four source/release gates and
both UI resource gates pass. Configuration parsing passes 420 assertions.
F6 covers 26 TGAs, 11x61 strings, 198 tooltip tiles and 177/878 main/tooltip
codepoints; 32 F6, 10 Hub and 22 Compact negative cases are rejected. Scene
assets pass 86 positive and seven negative checks. The first native attempt
caught an SDK TObjectPtr unpacking error; the final clean build uses explicit
Get() access while retaining reflected structure identity and bounds checks.

Retained DLL, receipt, metadata, 34 UI files and validation logs are under
`dist/work/candidates/radar-3.0.0-sg06-20260909`. The summary verifies the
compiled source and tools against the original canonical build receipt.
Offline EN/ZH and eleven-language previews are in repository-root
`out/handoff/F6_SG06_*`. Native fonts, Slate tooltip placement, actual game
appearance/input and frame-time performance remain pending owner testing.

**SG-06 has not been deployed or packaged.** Installed runtime remains SG-05,
whose DLL, receipt and 17 UI files match the retained candidate. All four user
files are untouched by this round. The installed visibility file has been
saved since the SG-05 deployment; preserve its current content, not its old
deployment-time hash. The read-only snapshot is repository-root
`out/handoff/SG06_BASELINE_IDENTITY.json`.

The existing three `dist/final-3.0.0` ZIPs are still SG-03, verified unchanged
at 4/43/47 entries. Future SG-06 targets are 64 payload inputs plus one manifest
(65 runtime files), 61 integrity-manifest members and 69/73 manual entries.
Those are specification targets, not newly generated release packages. Local
Nexus text is synchronized to SG-06; nothing has been uploaded or published.

## SG-05 deployment follow-up - September 9, 2026

The owner subsequently requested deployment. SG-05 is now installed, superseding
the earlier skip-deployment status below. The deployed DLL SHA-256 is
`C04C6E29D1113B183D8ED511C00BBE7482E46428B6EE53FFDA889319CC252B97`.
Pre-deployment Core 5/5 and all four gates passed again. All 17 UI asset files
match source; visibility, hotkeys, diagnostics and treasure overrides remain
byte-identical to their pre-deployment backup. Backup:
`dist/work/deployment/deploy-backups/20260909-052906-388-native-only-deploy`.
Exact verification: `dist/work/candidates/radar-3.0.0-sg05-20260909/deployment-verification.json`.
The three final ZIPs remain SG-03; no packaging or external publication occurred.
In-game appearance and performance remain pending owner testing.

## Historical SG-05 settings and Scene snapshot - September 9, 2026

Current source is **SG-05**. The popup has a 760x792 reference layout with
separate Marker, Scene, Height and Filter cards. Scene switches move to one
horizontal row below Marker visibility; Restore preset resets only Scene to
off / 600 m / 24 / Aim. The larger headings and compact language/status strip
replace the previous nested table layout. All eleven languages are retained.

Aim uses an ellipse with X16% / Y34% short-side radii and normalized ranking,
with the existing 120 ms dwell. Auto retains Euclidean ranking and its 15%
switch buffer. SG-04 small glyphs, direction tips, projection-only lifts,
distance corrections and the nonnegative clamp remain unchanged.

Six shared glyph textures and a 50-Image pool plus six collapsed texture keepers
replace the per-marker Border pieces. Positions submit only after cumulative
movement exceeds 0.25 physical pixels; candidate/projection frequencies remain
unchanged. Settings changes retain the hidden pool. This is structural evidence
of reduced UI work; game frame-time improvement is not yet measured.

The clean native build passed all 446 targets. Core 5/5 (308 Scene checks and
50 Escape checks), Compact, World Map, PostRender and release hygiene/resource
gates pass. The 1,200,128-byte DLL is
`C04C6E29D1113B183D8ED511C00BBE7482E46428B6EE53FFDA889319CC252B97`;
receipt UTC is `2026-09-09T12:10:21.3873261Z`. Exact source/tool identities,
retained assets and unchanged installed/archive checks are recorded in
`dist/work/candidates/radar-3.0.0-sg05-20260909/validation-summary.json`.
Owner direction remains
**skip deployment**. Existing installed files and all three final ZIPs remain
SG-03; SG-05 final packaging is deferred. Future manual targets are 52/56 entries
with 48 runtime files, not completed SG-05 archives. No game visual, input or
performance acceptance is claimed.

## Historical SG-04 source fixes - September 9, 2026

The language popup's selected background used a vertical coordinate as its
width (353), overflowing its 190-wide cell. It now derives 186x28 from the
cell with a 2-unit inset. The verifier checks actual highlight geometry over
12 cells and four scales, including rejection of the old overflow. French
and Spanish names retain their accents through verified popup and selected-name
images; the installed SG-03 DLL already contained the correct Unicode text.

All three Scene glyphs return to about 18-19 reference units with a common
small direction tip. Area Quest uses a gray open diamond and three dots;
Treasure retains the lid/clasp shape and Mini-games retain purple flags.
The fixed pool uses twelve pieces per marker. Existing projection-only lifts,
real positions, distance correction/clamp and Aim/Auto selection remain intact.

Owner direction: **skip deployment; more changes are pending**. This round
validates source, core tests, native compilation and assets only. No installed
file or final release archive is replaced. The existing three ZIPs and installed
DLL still identify SG-03; their exact historical checks remain below.
SG-04 evidence is in `dist/work/candidates/radar-3.0.0-sg04-20260909` and the
[attempt ledger](SCENE_GUIDANCE_ATTEMPT_LEDGER.md). The clean native build passed
all 446 targets; Core 5/5, compact, world-map, PostRender/runtime and release
hygiene/resource gates pass. The 1,196,032-byte SG-04 DLL is
`96485EC81471D8D4BD568177A1D18F673CA2553DA60F3047ABAC978E24F881C3`.
Its receipt UTC is `2026-09-09T11:08:33.1000443Z`; exact source/tools and
unchanged installed/archive checks are in `validation-summary.json` in that
candidate directory. Game display and input acceptance remain pending.

## Historical SG-03 handoff and retained installation

Current candidate: **3.0.0 / SG-03**, September 8, 2026. The three local release
packages are ready and the same build is deployed. Core 5/5, Setup 20/20 and
Manual 2/2 pass. No external publication or gameplay acceptance has been claimed.

- Native DLL: 1,194,496 bytes, SHA-256
  `4C0B9E788E35A6E46F45B4B5AB6EFCF925B3CDB6CF9E3F3D70E86EB13A853E32`.
- Compiled source SHA-256:
  `F4039912D2A97908B5AAF7083E2254B932EFC0EA41BDA6077FAB99FA5C69C0D5`.
- Current evidence: `dist/work/candidates/radar-3.0.0-sg03-20260908` and
  [SG-03 ledger](SCENE_GUIDANCE_ATTEMPT_LEDGER.md).
- Scope: distinct raised Scene symbols, nonnegative distance offsets, Aim/Auto
  focus, localized settings and page-owned Esc input; existing SG-02 features remain.
- Tests include 289 Scene checks and 50 hidden-window Esc checks, plus five
  complete CTest suites. F6 assets validate 156 glyphs across the existing
  11-language / 42-string / 41-overlay-slot contract.
- Owner-test focus: actual in-game Esc routing and popup/slider close, raised
  task/chest/Mole positions, zero-distance boundaries, Aim dwell/Auto buffering,
  travel/menu transitions and frame timing.

## Retained SG-03 package and deployment evidence

Canonical directory: `dist/final-3.0.0/`. The manifest was generated at
`2026-09-09T05:00:35.6146355Z` (September 8 Arizona time).

| Archive | Bytes | Files | SHA-256 |
| --- | ---: | ---: | --- |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Installer.zip` | 10,320,419 | 4 | `6F12CF2659AACCEC9908284FF18F5DC045EB16F0845AA9199EC5D4028263C743` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-No-UE4SS.zip` | 2,254,692 | 43 | `49AAB9BF0CCEF60BA08E9044C52CA87AE39201D010AEBC7780998FD8F773F6FD` |
| `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` | 10,280,329 | 47 | `130658C88EC3D4EB5D8337DC5D3DB95C65474DC5733306F61B71D28C04EE6BDA` |

Setup SHA-256: `33A91F1701B9F139978B8B0693F6FA11878F7C64538EA2173DE16B92D9827FF8`.
The installer/manual payload is byte-identical across 39 files. Full Setup
20/20 and Manual 2/2 matrices pass, alongside 177 hotkey and 414 visibility
assertions. Independent archive verification passes 1,548 checks with no failures;
its record is `out/handoff/SG03_INDEPENDENT_ARCHIVE_AUDIT.json` at the repository
root. CRC, safe paths, exact file lists, embedded resources, native receipt,
public defaults and checksums were checked independently.

Local deployment completed with the game closed. Verification at
`2026-09-09T05:08:44.4698483Z` confirms all 35 non-user payload files match the
package and four existing files retain their pre-deployment bytes:
`visibility.ini`, `hotkeys.ini`, `diagnostics.ini`, and `treasure_overrides.txt`.
The controlling `mods.txt` has exactly one enabled native Radar entry and no
predecessor entry. The full backup is
`dist/work/deployment/deploy-backups/20260908-220537-090-native-only-deploy`.
Exact records are `local-deployment.log` and `installed-verification.json`
under the SG-03 candidate directory. Nexus copy is synchronized locally;
no website upload or Git publication occurred. Existing third-party clearance
state is unchanged. Actual gameplay, visual, controller and performance
acceptance remain pending owner testing.

## Historical SG-02 documentation checkpoint

The following identity and table describe the earlier **SG-02** checkpoint only.
At that point SG-02 was locally installed and owner testing was pending. Its
verification/documentation pass
did not rebuild, redeploy, write game configuration, or control the game.

### SG-02 candidate identity and evidence

- Native DLL: 1,188,352 bytes, SHA-256
  `177E0401718F677EB9DD4C1314A94C0AE7DD13A0208B15BBAF35475934EC094B`.
- Compiled source SHA-256:
  `6B3A61703177267096664F83EEFB3DEB9E343D75C2491165264DF8877753D30E`.
- Runtime identity:
  `START version=3.0.0 runtime_label=DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_3_0_0`.
- Exact build, deployment and rollback details:
  [SG-02 ledger](SCENE_GUIDANCE_ATTEMPT_LEDGER.md).
- Evidence directory:
  `dist/work/candidates/radar-3.0.0-sg02-20260908/documentation-closeout/`.

| Check | Current evidence |
| --- | --- |
| Artifact identity | Build, retained candidate and installed DLL match; source-bound receipt passes under Windows PowerShell 5.1. |
| Installed generated data | All 11 files match the current catalogs, including the corrected Scene anchors. |
| Core regression | 4/4 existing CTest suites rerun and pass. The preceding clean build recorded 1,453 native-state assertions and 83 Scene checks. |
| Source safety | Compact, world-map, PostRender/runtime and release-hygiene gates rerun and pass. |
| F6 assets/layout | 11 languages / 42 strings, 41-slot Korean/Traditional Chinese overlays, seven TGA assets and geometry validation pass. |
| Installer visibility configuration | Current C# parser rerun in memory: 414 assertions pass. No Setup build is implied. |
| Gameplay / visuals / performance | Owner testing; results pending. Static geometry and bounded work do not prove runtime appearance or frame timing. |
| 3.0.0 Setup / manual archives | Not generated; package and full installation matrices remain pending. |
| Nexus | Local Description, Quick Support, Changelog and Files drafts updated; no upload or post. |

The source documentation and descriptive metadata have been synchronized with
the six-section configuration, Scene settings and five height controls. Their
documentation revision is later than the installed metadata snapshot; the
installed DLL, catalogs and configuration remain the owner's test baseline.
Use the saved snapshot when comparing installed metadata, and use current
source documentation when preparing a later package.

Scene has 143 verified Area Quest anchors; four unconfirmed anchors are omitted
from Scene only, while their minimap/world-map eligibility remains unchanged.
See [the 3.0.0 plan](RELEASE_PLAN_3_0_0.md) for the focused owner-test checklist.

## Historical 2.3.0 local packages

Date: 2026-09-07. The following package identities and checks apply only to the
three CM-04 archives. They are unchanged and contain neither SG-01 nor SG-02.
2.2.2 was never published; its follow-up work was included in 2.3.0.

### Deliverables

Canonical directory: `dist/final-2.3.0/`.

- `DragonSwordNativeWorldRadarPostRender-v2.3.0-Installer.zip`
- `DragonSwordNativeWorldRadarPostRender-v2.3.0-Manual-No-UE4SS.zip`
- `DragonSwordNativeWorldRadarPostRender-v2.3.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`
- Identity authority: `release-manifest.json` and `SHA256SUMS.txt`.

Choose one package. Setup is recommended for existing installations and
configuration changes. Install / Update / Repair load existing keys and apply
explicitly confirmed changes while preserving other user settings.

### Features and evidence boundary

- Configurable Settings / Enable / Disable keys; defaults F6/F7/F8.
- AUTO is first, stays AUTO, and refreshes on Settings open or Enable.
  Manual language choices remain fixed until AUTO is selected again.
- Mounted-flight map attachment and controller-menu hiding corrections.
- Retains the CM-04 native DLL; documentation repackaging does not add native fixes.
- Public diagnostics off, language AUTO, default hotkeys.
- Source-bound native receipt, core tests, compact/world-map/runtime source
  checks, and release hygiene passed under Windows PowerShell 5.1.
- Hotkey parser/editor: 177 assertions; Setup matrix: 20/20; manual-copy matrix:
  2/2. No failed or skipped fixtures. Setup/manual payload equivalence passed.
- Independent final ZIP reopen: 3/3 archive hashes and entry counts passed;
  both manual packages contain the exact retained DLL and the three validated
  public configuration files. Packaged Setup matches the manifest.

Native SHA-256: `D6C79578F3C19C033BB0A1F75EDB8382719CFDC87BB145D0D39ECB12FBDB7046`.
Setup SHA-256: `9ED38737B3E0914A784ACDC325706EA8F7D6D9F70099DAD2011B1EF84EA2E0AE`.

| Archive | Bytes | SHA-256 |
| --- | ---: | --- |
| Installer | 10151201 | `A37DD48C631358C5ABD708FCE7E5172DEF492D0E87277AEC58B1B11677941CA6` |
| Manual-No-UE4SS | 2084316 | `2611593B0E2148C59DC2099EA3B419E57C3A03F7BB8E345D2AA0E978F67B1583` |
| Manual-With-UE4SS | 10109953 | `226709595DF0C1D97E66D2637133BC86F6111D3AD70E9D18735DDBFD8D715A0C` |

Prior virtual-controller testing covered two Start / Hero / HeroSkill / return
cycles with diagnostics on. It does not prove all physical controllers, menus,
resolutions, debug-off sessions, city cold activation, or long-session gameplay.
See [2.3.0 validation plan](RELEASE_PLAN_2_3_0.md) and the controller/world-map
attempt ledgers. No universal runtime-fixed claim is made.

## Documentation routing

- Setup guide: [INSTALL.md](INSTALL.md).
- Manual guide: [MANUAL_INSTALL.md](MANUAL_INSTALL.md).
- Nexus publishing copy: [local draft index](../assets/nexus/README.md), excluded from GitHub.
- Current development and testing: [RELEASE_PLAN_3_0_0.md](RELEASE_PLAN_3_0_0.md).
- Historical package development: [RELEASE_PLAN_2_3_0.md](RELEASE_PLAN_2_3_0.md).
- `RELEASE.md` contains historical 2.2.1 contracts, not current build commands.

The historical 2.3.0 documentation repack was a local package handoff, not a game
deployment, Git push or Nexus upload. Preserve its exact-byte receipts and use
its own manifest rather than older ZIP hashes for that package set.

The previous five release files were copied and hash-verified at
`dist/work/candidates/before-doc-closeout-20260907/` before replacement.
Public publication status remains `BLOCKED_PENDING_RIGHTS_AND_SOURCE_PROVENANCE_REVIEW`,
as recorded in the release manifest and `metadata/release.json`. Local package
verification does not resolve that existing blocker.
