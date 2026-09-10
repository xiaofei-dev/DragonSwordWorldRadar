# Scene guidance attempt ledger

## Guide activity-order revision — owner display request

Candidate `radar-3.0.0-sg16-guide4-20260909`; version **3.0.0**.

Guide swaps the Marmot mini-game and Sudden mission entries as complete groups: the text and both Radar/Map icons move together. The left column now reads Flying mini-game, Wave mini-game, Marmot mini-game and Bird eggs; the right column reads Sudden mission, World boss and Area quest. All eleven languages use this order. The three height columns, 40 icon examples and 539 text cells remain unchanged.

This is a layout-only asset revision of `radar-3.0.0-sg16-guide3-20260909`. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. No native code or localization-header change is included, and no new native compilation is claimed.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T02:04:37.4453785Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T02:06:49.3883924Z`.

Manifest UTC: `2026-09-10T02:06:02.2485470Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/package-verification.json).

The previous Guide3 and earlier SG-16 sections below remain historical records. Their earlier activity order and ZIP hashes identify those sets. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Guide three-column revision — owner display request

Candidate `radar-3.0.0-sg16-guide3-20260909`; version **3.0.0**.

The owner requested a simpler height reference. Guide now displays only Above, Near level and Below, with 15 height examples, 40 total icon examples and 539 text cells across eleven languages. The Unknown column is no longer drawn. Actual unknown-height handling, height switches and all other gameplay behavior are unchanged. The existing 44-string localization schema remains intact.

This is an asset/documentation revision of `radar-3.0.0-sg16-20260909`, not a new native build. Its 1,325,056-byte DLL remains `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`, with compiled source `8B5E73BEF70C4960C322CCDC1EE0172155B0D101D09B7D27F5D8ED63634D6793`. The parent's 446/446 native and Core 9/9 evidence retains its original identity.

The canonical release pipeline reran Core 9/9 and all four source/release gates successfully while reusing the verified native binary; no new native compilation is claimed. New deployment was verified at `2026-09-10T01:45:25.9749727Z`; the same DLL, native receipt and all 73 UI files match the new candidate. Four settings/data files were preserved, with 2 AutoPickup files recorded unchanged. Setup 20/20 and Manual 2/2 pass with no failures/skips, 104 equivalent runtime files and 4/108/112 ZIP entries. The final set was promoted at `2026-09-10T01:47:39.7016800Z`.

Manifest UTC: `2026-09-10T01:46:51.6121910Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/package-verification.json).

The previous SG-16 sections below are preserved historical records. Their four-column Guide description and old ZIP hashes describe that earlier set. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## SG-16 - 2026-09-09 - map settings, defaults and Guide clarity

Candidate `radar-3.0.0-sg16-20260909` implements live Map category edits from F6, independent All
controls for seven Radar/five Map categories, Auto focus defaults and clearer
mini-game terminology. Green marks mini-game reward chests; Mini-games groups
flying, marmot and wave activities.

Guide removes redundant color-name labels, uses two-column activity entries
with Radar/Map symbols, illustrates above/near/below/unknown height states, adds
Clock, aligns Scene examples and gives each distance mode its own row. Turning
height indicators off keeps ordinary category icons and is explained separately
from unknown height. Only the nearest chest and mini-game get height hints;
other supported categories show them on each visible marker. Mini-game arrows
retain their activity color. The clock
description identifies in-game time and its sun/moon time-of-day symbol.

The owner described a 150-to-100 ms delay without naming the timer. Root stated
the conservative interpretation: first Aim acquisition changes from 120 to
100 ms, retaining the 350/500 ms replacement buffers. The separate live-map
merge is 100 ms with immediate all-off hiding. Read-only review caught an
interleaving where an external event could collect an intermediate snapshot
before a user returned a toggle to its old baseline. The snapshot collector
now invalidates that comparison baseline only when F6 is open and its map
refresh is pending; it preserves the original pending deadline.

Native 446/446, Core 9/9 and all four source/release gates pass. Setup passes 20/20 and Manual 2/2, with no failures or skips. The three channels have 104 equivalent runtime files, including 73 UI files and twelve Guide files; ZIP entry counts are 4/108/112.

Local deployment was verified at `2026-09-10T00:22:37.0293741Z`. The installed and packaged DLL is the same 1,325,056-byte file, SHA-256 `92F0D10860E991BE090565EAD247D3C8731FF9A5C0E08CB65056C3E65BE0BE30`. The deployment receipt verifies the native receipt and all 73 UI files, preserves four settings/data files and records 2 unchanged AutoPickup files.

Promotion verified at `2026-09-10T00:23:01.4155094Z`. The five preceding SG-15
final files are preserved at `dist/work/candidates/radar-3.0.0-sg16-20260909/previous-final-sg15`. Exact native/archive hashes and all
current receipts are in [Release status](RELEASE_STATUS.md).

README and both installation guides were finalized and frozen before the final package build. They describe the product and its controls; these external receipts record exact build, deployment and delivery results. The payload documents and metadata were not rewritten after sealing.

Build, source/resource checks, installation and package checks are verified; they do not establish game visual, input, Guide usability, camera-motion or measured performance acceptance. Those remain owner testing. No Nexus upload or post was performed.

## SG-15 release packaging - 2026-09-09 - authorized local package handoff

The owner now requests release packages and final documentation. This supersedes
the earlier packaging hold without authorizing a Nexus upload or post. Package
candidate: `radar-3.0.0-sg15-release-20260909`; the SG-15 runtime feature set and
its verified deployment snapshot remain unchanged.

Packaging tools must use the current payload inventory rather than SG-10's old
fixed ZIP counts. A fresh canonical build binds the revised tooling to its own
receipt. README and both installation guides were frozen before staging.
The final set passes Setup 20/20 and Manual 2/2 with no failures/skips, 104
equivalent runtime files, 73 UI files, 4/108/112 ZIP entries and byte-identical
fresh extractions. Promotion was verified at `2026-09-09T23:23:40.0349295Z`;
the previous five SG-10 files are backed up at `dist/work/candidates/radar-3.0.0-sg15-release-20260909/previous-final-sg10`.
Stale current-state metadata found after the first package attempt was corrected
before the final seal. The rebuilt release DLL matches deployed SG-15 source,
but has a different hash and was not redeployed. Final hashes and receipts are
in Release status. Game visual/input/performance acceptance remains separate.

## SG-15 - 2026-09-09 - localized icon guide inside Settings

Add Guide beside Close in F6, with Settings as the return action. Candidate:
`radar-3.0.0-sg15-20260909`. The new page explains the four treasure colors,
Radar and Map activity icons side by side, Radar height arrows, Scene symbols
and the four distance modes in all eleven languages. Activity labels describe
flying, marmot and wave mini-games instead of exposing internal enum names.
Keep activity/quest reward and treasure-map/legendary meanings complete;
green does not promise that a reward chest can be opened immediately.

Use a 1120-reference-unit scrolling guide body with a 1184-unit atlas,
including its two header-button labels. The static atlas is 1520×2368 pixels.
It reuses the fixed header/footer and saves separate Guide and Settings scroll
positions while F6 is open. Closing F6 clears page state for the next opening.
Language changes replace the localized atlas; missing guide artwork disables
Guide or returns an open guide to Settings without blocking F6. Resizing clamps
the active page's scroll offset and retains the existing confirmation guard.
Reset still uses the existing confirmed settings transaction.

Read-only native review found no blocking defect in page state, clipping or
input layering. The guide's non-interactive artwork does not disable its parent
ScrollBox; actual mouse-wheel and controller behavior still require game testing.
Guide adds static reference artwork, not a gameplay polling service. Retain
SG-14 typography, SG-13 distance switching and SG-12-H2 menu compatibility.
Native build 446/446, Core 9/9 and all four deployment gates pass. Local
installation verifies the DLL, build receipt and all 73 UI files, including
the unchanged 61-file SG-14 inventory. Four user settings/data files remain
byte-identical. Exact receipts and backup identity are in Release status.
Game visual/input acceptance remains owner testing. No release package or
Nexus upload was made; the SG-14 evidence below remains historical.

## SG-14 - 2026-09-09 - settings typography and confirmation appearance

Owner reports that the latest text feels inconsistent, the all-caps English
page is visually heavy, and the large rectangular confirmation looks unlike
the surrounding settings. Candidate: `radar-3.0.0-sg14-20260909`.

Use natural capitalization in English, French, German, Spanish, Russian and
Portuguese, retaining German noun capitalization. Remove obsolete AUTO wording
from language help in all eleven locales. Keep the 75 main/help and seven
confirmation strings per language in their existing order, with explicit Nexus
opening and voting intent and unchanged Yes/No choices.

Set reference-pixel typography to 22 for the title, 16 for section headings,
14 for body text and slider values, 13 for buttons and all three footer actions,
and 12 for small annotations. Align glyph roles
to shared font baselines. Reduce scene interference with stronger blue-gray
reading surfaces, and use a compact 380×184 rounded confirmation card instead
of the oversized rectangular dialog. Retain the four settings sections, fixed
header/footer, responsive scrolling, category choices and confirmation input
guards. No live scene blur is introduced.

Retain SG-13 Aim/Auto switching and SG-12-H2's optional scrollbar-appearance
setter. This revision changes presentation, without changing Scene positions,
distance corrections, height rules or presets. Native build 446/446, Core 9/9,
all UI/resource checks and four deployment gates pass. Local installation is
verified against the DLL, receipt and 61 UI files, preserving settings. Exact
evidence is in Release status; game appearance and input acceptance remain
owner testing. No release package or Nexus upload is requested at this stage.

## SG-13 - 2026-09-09 - distance labels switch too rapidly

Owner reports distracting Aim label changes during left/right camera movement
and suspects Auto as well. Source confirms Aim immediately blanked the old
label and restarted a 120 ms acquisition; Auto switched on a single frame with
15% improvement. Keep initial Aim acquisition at 120 ms, but separate the
acquired target from its challenger. Require continuous 350 ms (Aim) / 500 ms
(Auto) advantage before replacing a still-visible target. Require over 20%
center-distance improvement and a minimum gap (0.08 normalized Aim distance,
1.2% of viewport short side for Auto). Keep the old label during that interval.

Only acquired Aim targets use a 1.2-times exit ellipse. Pending targets still
need the original ellipse. Hidden or removed targets lose labels immediately;
mode changes, backward time, challenger identity changes or lost advantage reset
their respective timers. This changes distance-label selection only, without
delaying camera projection or changing glyphs, anchors or settings. Validation
and installation are recorded in Release status. Core 9/9, Scene 382 checks,
native 446/446 and four deployment gates pass. Deployed at
2026-09-09T21:33:45Z with all 61 UI files, four saved settings/data files and
AutoPickup verified. Actual camera-sweep comfort remains owner acceptance.
No release package was created.

## SG-12-H2 - 2026-09-09 - runtime ScrollBox capability failure

H1 is rejected by owner testing. A diagnostic session with its exact installed
DLL logged F6 receipt followed by `VISIBILITY_HUB_REJECTED`, failure/ABI mask
`1073741824` (bit 30), before tree construction. A bounded, read-only Lua probe
then observed `/Script/UMG.ScrollBox:SetOrientation` with `NewOrientation`, but
`/Script/UMG.ScrollBox:SetScrollBarVisibility` was absent in the shipped game.
Evidence: `dist/work/candidates/radar-3.0.0-sg12-h2-20260909/h1-f6-failure.log`
and `live-umg-probe.log`. Missing native log evidence in the first attempt and
using newer engine documentation were insufficient to establish H1 compatibility.

H2 makes only the scrollbar-appearance setter optional. If present, it still
requires the correct enum/byte parameter, offset and size. If unavailable or
incompatible, the pointer is cleared and its call skipped, preserving the
game's ScrollBox default. Core scrolling, content, size and transform ABI
remain required. Two additional negative cases enforce this behavior (ten
viewport regressions total). The temporary probe is disabled after capture.
Native build, deployment and actual F6 opening are tracked separately.

## SG-12-H1 - 2026-09-09 - F6 cannot open

Owner reported that the deployed SG-12 settings menu does not open. The installed
DLL is `56ADC902BED3C468D5E933F2D03A81850A9FEB7226A27D6702A04153716A1961`;
diagnostics were disabled, so there is no runtime failure-code trace.

Source review found two deterministic ABI-check errors: UMG ScrollBox setters
use `NewOrientation` and `NewScrollBarVisibility`; SG-12 checked the Slate names
`InOrientation` and `InVisibility`. Either missing parameter sets bit 30 and
disables the Hub before F6 can create its tree. The
[Epic UMG interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/UScrollBox)
confirms the names. Independent review found no further newly added parameter-name mismatch.

Correct only these names, retain type/offset/size validation, and add two negative
cases to the Compact gate (eight viewport regressions total). Existing graphics,
fonts, settings and Scene motion behavior are unchanged. Build/deployment evidence
is recorded in [Release status](RELEASE_STATUS.md). H1 was deployed and verified
at 2026-09-09T20:40:45Z after Core 9/9, native 446/446 and all four gates passed.
Installed DLL SHA-256 is `B578566C881B2C5E5FDE0CE4FE88975366E404EB5F39DAF7BB460906B64A82A3`.
Settings and AutoPickup remain unchanged; no release package was made.
Owner F6 reopening acceptance remains pending.

## SG-12 - 2026-09-09 - fonts, viewport layout and camera motion

Owner requested a complete eleven-language font and wording check, an explicit
Vote for This Mod action, smoother camera-follow markers and usable settings on
small and ultrawide displays. Local deployment is authorized; release packages
are expressly deferred until testing is accepted.

Use fixed regular-weight glyph resources for all page roles, help, confirmations
and numeric values. All three footer actions use 13 reference units. Keep the
header/footer fixed and scroll the four cards on short displays; resize the
retained tree in place, preserving state and cancelling pending confirmations.

Use the current frame's camera basis and engine-projected calibration points,
including witnesses on both depth axes and a real retained marker. Unsuitable
calibration uses native projection; orthographic views retain native projection.
Remove the subpixel translation deadband and use volatile marker groups with
pixel snapping disabled where available. Add small edge/overlap hysteresis;
world coordinates, distance corrections and camera update scheduling stay intact.
No frame-time or game-motion improvement is claimed from source tests alone.

Core 9/9, native 446/446 and all four gates passed. Deployed and verified at
2026-09-09T20:18:13Z with DLL SHA-256
`56ADC902BED3C468D5E933F2D03A81850A9FEB7226A27D6702A04153716A1961`.
All 61 UI files match; settings and AutoPickup are preserved. Existing final
ZIPs are unchanged. Owner game testing remains pending; no SG-12 package was made.

SG-11 remains a verified build-only snapshot. Its deployment and packages were
superseded by this request. Final SG-12 evidence belongs in
[Release status](RELEASE_STATUS.md).

## SG-11 - 2026-09-09 - concise localization and footer actions

Owner requested shorter natural wording in every language, conventional reset
terminology and a separate bottom row for Reset, Endorse and Feedback. Rewrite
the 32 help and seven confirmation strings per language without changing their
IDs or behaviors. English uses Reset to Defaults. The shared activity height
switch is labeled Mini-games to reflect its existing Fly/Mole/Wave scope.

Keep the four settings cards at their existing coordinates and add a footer at
Y=794..844. The panel is 760 x 852; top controls are title and Close only.
Move all corresponding text, click regions and the Endorse atlas crop together.
Extend the language popup's dismissal shield to the footer and move the existing
confirmation panel down to remain centered. No runtime rendering, provider or
confirmation transaction logic changes. Final validation and delivery evidence
will be recorded in [Release status](RELEASE_STATUS.md).

## SG-10 - 2026-09-09 - confirmed actions built, deployed and packaged

Owner requested separate Endorse and Feedback actions, confirmation before
opening Nexus, and confirmation before restoring defaults. No must dismiss
without a side effect. Nexus mod 254 was verified as this Radar mod: Endorse
opens the homepage and Feedback opens Posts, with no vote or post submission.

The native preallocated modal blocks settings underneath it. Entry stores only
an intent; Yes requires neutral input arming, consumes that intent and then
executes the selected action. No wins over simultaneous Yes, clears intent and
returns unchanged settings. Esc cancels only the modal and retains the same
press/repeat guard; loss of focus closes all of F6. Lifecycle reset clears intent.
Missing confirmation glyph resources produce a cancel-only fallback.

The full SG-09 display and frame-projection fixes are preserved. The 11-language
atlas now has 32 help and seven confirmation tiles per language, without adding
texture files or per-frame scans. Reviewed source previews remain distinct from
runtime Slate input, layout and game acceptance.

Clean native build passed 446 targets; Core passed 7/7 with 55 confirmation and
58 Escape checks. PostRender rejects twelve transaction regressions in addition
to ten Scene/live-settings regressions; F6 verification rejects 27 variants.
Deployment passed at `2026-09-09T16:51:20.6801835Z`, preserving four user config
files and AutoPickup. All three 3.0.0 packages were built and promoted at
`2026-09-09T16:55:11.2153219Z`, with Setup 20/20 and Manual 2/2 checks passing.
The manual default config filenames map to their installed `.ini` names;
all 64 payload bytes were checked per manual ZIP, and 60 immutable installed
files match. The previous five SG-03 final files were backed up before replacement.
Exact DLL/source/tools and transaction receipts are recorded in
[Release status](RELEASE_STATUS.md). Game acceptance remains with the owner.

## SG-09 - 2026-09-09 - six owner-tested follow-up corrections

The owner reported weak task contrast, low chest guidance on some bodies, an
off-center clock, Radar flashing when moving with F6 open, coarse camera-follow
motion and an overly black menu. They explicitly authorized implementing all six,
deploying, producing all three local 3.0.0 packages and closing local Nexus docs.

- Preserve the task diamond's geometry and add restrained translucent blue-gray
  backing. Keep other Scene sprites byte-identical to SG-08.
- Change the uniform Treasure display lift from 100 to 160 cm. Keep raw XYZ,
  distance offsets/clamping and minimap height classification unchanged.
- Place the clock's visible Y=15 center midway between owned RetainerBox and
  DLayerQuest geometries. Use the existing 1 Hz service, strict visible ancestry
  and fitting bounds, resize deferral and a 178-reference fallback.
- Exempt only F6's actually owned gameplay cursor while native HUD paint is
  positively visible. All actual native-menu, map, pause, activity and lifecycle
  suppression remains. Settings apply to the retained pool immediately.
- Move Scene service outside the 16 ms player-position gate. Keep catalog
  selection at 250 ms; update current distances only for at most 50 preselected
  targets, without re-sorting. Use Widget:SetRenderTranslation instead of Canvas
  slot position writes, retaining cumulative 0.25-pixel caching.
- Recolor approved rounded translucent surfaces to soft blue-gray, including
  reading surfaces and native fallback. Add no live blur or texture inventory.

Canonical Core 6/6 and native 446-target build pass. Scene has 313 checks,
Clock 28, Escape 50; four source/release gates pass. Ten new Scene-frame/preview
regressions are rejected. Source/pixel visual checks and numeric layout tests do
not establish game alignment or actual FPS. Exact DLL/source/tool hashes and
subsequent deployment/package receipts are tracked in `docs/RELEASE_STATUS.md`
and `dist/work/candidates/radar-3.0.0-sg09-20260909`.

SG-09 deployment subsequently passed at `2026-09-09T15:52:57.0256520Z`.
The installed DLL, build receipt and all 34 UI resources match the candidate.
Four user configuration/override files are byte-preserved; AutoPickup's two
DLL/config files are unchanged. The full SG-08 installation and mods.txt are
backed up at `dist/work/deployment/deploy-backups/20260909-085255-045-native-only-deploy`.
The deployment receipt supersedes build-time metadata's predeployment snapshot;
it does not establish gameplay, visual alignment or performance acceptance.

## SG-08 deployment - 2026-09-09

After reviewing the completed style candidate, the owner explicitly requested
deployment. The canonical native deployment tool installed SG-08 with
Diagnostics Preserve after Core/static preflight passed. Independent checks
confirm the candidate DLL, receipt and 34 UI resource hashes, plus byte-exact
preservation of visibility, diagnostics, hotkeys and treasure overrides.
AutoPickup DLL/config hashes are unchanged. The previous SG-05 installation
and mods.txt are backed up under
`dist/work/deployment/deploy-backups/20260909-074959-027-native-only-deploy`.
Current evidence is candidate `deployment-verification.json`, UTC
`2026-09-09T14:50:01.0367076Z`. This supersedes the earlier build-only deployment
status; it is not game/visual acceptance. Existing SG-03 ZIPs remain unchanged.

## SG-08 - 2026-09-09 - neutral translucent settings

The owner supplied a dark rounded desktop-menu reference and requested subtle
transparency. The previous skin composed card and outer layers to about 98%
opacity, so reducing a single layer was insufficient. SG-08 budgets final
composited alpha, uses neutral charcoal and reduces colored highlights. Small
utility text and tooltip surfaces receive a deeper reading background.
The native fallback palette is softened as well; successful texture rendering
already bypasses the full native board and does not add an opaque cover.

Readability checks use actual final TGA pixels, source text-slot geometry and
linear-light compositing against white. Generated light/dark/high-contrast
preview backgrounds do not modify the user's screenshots or establish game
acceptance. No live blur, refraction, extra images or per-frame work is added.
Layout, 31 help topics, language resources, tooltip ownership, Scene assets,
height classification and projection offsets stay unchanged. SG-07 chest
height investigation remains unresolved; this styling change makes no height
fix claim. Canonical build evidence is recorded in `docs/RELEASE_STATUS.md`.

## SG-07 - 2026-09-09 - individual explanations and chest-height investigation

The owner requested explanations for every setting and reported mixed chest
height alignment, with a screenshot of a levelled chained chest. Existing SG-06
tooltips covered controls but reused broad column, Scene, height and filter
descriptions. SG-07 supplies 31 distinct topics across all eleven languages:
seven marker categories, three Scene switches, five height choices, four
filter options and twelve utility/range/distance controls. The seven marker
labels gain separate hover targets. There are 55 owners within the existing
64-slot weak bookkeeping pool; layout, skins and native hover behavior stay
unchanged. Eleven same-name tooltip atlases contain 341 tiles, preserving the
26-TGA F6 file set and the 65-file future runtime specification.

Chest investigation is read-only. All 1693 render rows match the retained
SectionTreasureBoxData XML XYZ, and all 1692 actor-class rows agree with their
render positions. The remaining render row already has an explicit ignore
entry. No catalog-copy Z mismatch was found. The white Other visual category
contains ten actual actor classes; it is not a body-height classification.
Scene uses catalog Z +100 cm for every chest, without mesh-top bounds. In the
32x36 UI group, the visible direction tip is 16 logical pixels below the
projected icon center (geometric lower edge 16.1163). These are separate
world-space and screen-space effects and are not evidence of the correct
height for the chest in the screenshot.

An older Probe document describes one 52.6765 cm static/live actor-root Z
difference; its original runtime TSV is not retained, so it cannot justify a
current global or per-class correction. The screenshot does not identify a
unique chest ID or actual mesh bounds. No coordinate, height threshold or
projection-lift value is changed in SG-07. Clarification is pending on whether
the reported problem concerns Scene placement or minimap height classification.
Evidence: repository-root `out/handoff/SG07_TREASURE_HEIGHT_AUDIT.md` and `.json`.

Current build and validation evidence is recorded in `docs/RELEASE_STATUS.md`.
SG-06 remains a retained build, SG-05 remains installed, and the final ZIPs
remain SG-03. No deployment, package replacement or game acceptance is implied.

## SG-06 - 2026-09-09 - settings polish, global preset and contextual help

The owner requested visual refinement of the existing popup, a slightly
larger Area Quest symbol, a global preset, hover explanations, and a lower
clock. The owner clarified that square boxes mean the settings Radar/Map
checkboxes only. The previous SG-05 candidate was subsequently deployed;
its deployment verification supersedes the earlier skip instruction below.

- Preserve the 760x792 four-card arrangement. Six static skins supply rounded
  surfaces and highlights; native brush colors decode sRGB once, fixing the
  brighter gray seen in game. Radar/Map checkboxes are 22x22 inside nonoverlapping
  24x24 hit bounds. Chips use reflected nine-slice brushes with verified margin,
  resource and mode fields. Unsupported optional style ABI uses the plain fallback.
- Top Restore Preset restores this page's visibility and height flags, filters
  to Available, language AUTO, and Scene all on / 600 m / 24 / Aim. Preserve
  runtime power and startup hotkeys. Existing explicitly disabled Scene choices
  stay disabled until the owner changes or resets them. Pending slider writes
  still flush on reset even when the values already equal the preset.
- Eighteen explanations in all eleven languages describe every setting.
  Native Slate tooltip ownership supplies hover behavior without frame polling.
  Each owner has its own clipped widget content; the current-language atlas is
  shared. Weak bookkeeping is bounded at 64. Language/import failures clear old
  contents and fall back to native text instead of displaying stale translations.
- Only the Area Quest open diamond body grows 18%, to about 22 reference units,
  with a 1.8-unit gray outline. Its visible alpha area grows from about 95 to
  141 square units; chest is about 265, explaining why equal outer dimensions
  had looked smaller. The other five textures and the small direction tip stay
  unchanged. True coordinates, display-only lifts and distance clamp stay intact.
- Clock follows Bird Eggs in the settings table. Its HUD top moves down 16
  reference units; the 42-unit clock panel stays inside the existing host bounds.
  Sixty-three viewport/DPI combinations pass the static fit check.
- All-on clean defaults are consistent across native parsing, shipped INI and
  installer validation. Legacy explicit choices remain supported. Core tests
  include default reset and explicit all-off round trips; installer configuration
  tests pass 420 assertions.
- Assets comprise 26 F6 TGAs plus manifest and six Scene TGAs plus manifest.
  Tooltips add 198 verified tiles and 878 covered codepoints; the existing main
  overlays cover 177. There are 61 strings per language, 44 main slots and 48
  scaled popup-cell checks. F6 rejects 32 mutants, Hub 10, Compact 22; Scene
  verification passes 86 positive and seven negative checks.
- Source-derived previews are under repository-root `out/handoff/F6_SG06_*`
  and `SCENE_SG06_SOURCE_ONLY_PREVIEW.png`. Native fonts, tooltip placement,
  game appearance/input and frame times still require owner testing.

Current build identities and handoff evidence are in `docs/RELEASE_STATUS.md`
and `dist/work/candidates/radar-3.0.0-sg06-20260909`. Current installed baseline
is SG-05; existing final ZIPs are SG-03. SG-06 does not imply new installed or
published bytes. Future packaging targets are 65 runtime files and 69/73
manual archive entries; those counts do not describe the existing ZIPs.

## SG-05 - 2026-09-09 - settings rebuild, tolerant Aim and Scene rendering cost

The owner requested wider Aim selection (especially Y), a complete settings
popup redesign and Scene performance improvements. The earlier instruction to
skip deployment remains in force. This candidate changes local source,
resources and validation only; installed SG-03 and existing final ZIPs stay intact.

- Aim uses radii X16% / Y34% of the shorter viewport side, versus the old 10%
  circle. Candidates rank by normalized ellipse distance; Auto still uses
  Euclidean screen distance with 15% hysteresis. Same-identity dwell stays 120 ms.
  Boundary inclusion is consistent despite decimal floating-point error;
  existing hidden/overlap/mode/suppression retirement remains immediate.
- F6 becomes a 760x792 four-card popup: Marker visibility (Radar/Map only),
  Scene guidance, Height indicators, Filter mode. Three independent Scene
  switches form a horizontal row. Titles are stronger, utility controls compact,
  and nested table/frame decoration is reduced. Reset changes only Scene to
  off/600/24/Aim and flushes any pending slider edit even if values already match.
- F6 inventory is 11 languages x 43 strings, 44 main slots and nine TGAs
  (seven 1520x1584 overlays and two 440x52 selected-language labels).
  Coverage is 177 codepoints; 48 scaled selected-cell bounds pass, 16 source/asset
  mutants and seven separate Hub binding/reset mutants are rejected. Existing
  French/Spanish fallback, AUTO, Esc and final-slider sampling contracts remain.
- Six generated 128x144 glyph textures preserve the exact SG-04 32x36 canvas,
  small glyph shapes, category colors and direction tips. Assets pass 78 checks
  and nine malformed/mutated cases. No new coordinate or object source is added.
- Installed SG-03 used 400 Border pieces; the undeployed SG-04 source used 600.
  SG-05 uses 50 marker Images plus six collapsed keeper Images that own all six
  textures through reflected Brushes. Imports happen only at attachment and
  kind changes need one verified texture bind. No per-frame file import occurs.
- Position changes accumulate against the last submitted point and commit only
  beyond 0.25 physical screen pixels. Projection still runs each frame. Repeated
  suppression is an early return; configuration sampling invalidates selection
  while retaining the collapsed renderer tree instead of rebuilding it.
- Source-derived EN/ZH/full-language/popup previews are in repository-root
  `out/handoff/F6_SG05_*`; glyph previews are in
  `out/handoff/SCENE_SG05_SOURCE_ONLY_PREVIEW.png`. These are offline geometry
  evidence, with reference font rendering where noted, not game screenshots.
- During validation, a new exact-Y-boundary test exposed one-ULP inclusion;
  the model now consistently excludes the edge. The first native compile also
  caught a missing complete FStrProperty include, which was added. The Compact
  gate's old Scene/Hub contracts were updated to the new exact layout and a
  strict 35-field teardown assignment whitelist. Eleven shutdown mutants are
  rejected, including unknown assignments and UObject operations. Eight
  additional Scene source-gate mutants are also rejected. A full native rebuild
  followed the final tool change; no old receipt was edited or re-stamped.

Final SG-05 evidence:

- Core 5/5; Scene 308/308, Escape 50/50. Pinned clean native build: 446 targets.
- Compact, World Map, PostRender/runtime and release hygiene/resource gates pass.
- DLL: 1,200,128 bytes, SHA-256
  `C04C6E29D1113B183D8ED511C00BBE7482E46428B6EE53FFDA889319CC252B97`.
- Compiled source SHA-256:
  `16A389B6F543F0A54496FFD90ECC07DE0D7C1AE920CF5116F19F4C3E4DC9B207`.
- Release-tool SHA-256:
  `C65867EDC916C3727A312DC4370ED0DF39F55F461036637F11D076C590878A05`.
- Receipt UTC: `2026-09-09T12:10:21.3873261Z`.
- Candidate DLL, receipt, three metadata files, both asset folders, final logs
  and `validation-summary.json` are retained in
  `dist/work/candidates/radar-3.0.0-sg05-20260909`. The installed SG-03 DLL and
  three old final archives retain their recorded hashes, sizes and entry counts.
- No SG-05 Setup or manual-install matrix was run, because final packaging is
  deferred. Future 52/56 manual entries and 48 runtime files are planned input
  contracts only; the existing final archives still contain SG-03.

Gameplay visibility, language font appearance, controller input and measured
frame-time impact remain pending owner testing. No final package was rebuilt,
no game file was written and no external publication was performed.

## SG-04 - 2026-09-09 - language picker and small Scene glyph corrections

User feedback rejects SG-03's enlarged glyphs/exclamation task badge and reports
language-selector overflow. The owner subsequently requested skipping deployment
because more changes are coming. This round is source/core/native/resource
validation only; installed SG-03 and existing final packages remain unchanged.

- The selected language background incorrectly used `kMarkerHeaderY - 1.0`,
  producing 353 units of width in a 190-wide cell. The background now derives
  186x28 from that same 190x32 cell with a 2-unit inset. Actual drawing geometry
  is verified for 12 choices at four scales; the old overflow is rejected.
- Correct UTF-16 French/Spanish endonyms are present in both source and the
  installed SG-03 binary. Their existing Common font route does not establish
  accent coverage. Verified raster labels now cover KO/TC/FR/ES popup names and
  the FR/ES selected-name card. The native text is hidden only after successful
  image/texture binding and readback. AUTO uses the resolved language; missing
  assets fall back to native text. One name Image and two weak texture slots
  refresh only on open/language/status edges and clear on close/travel.
- F6 assets: nine TGAs (six full main overlays, one popup, two 560x54 name-only
  images), 170 verified codepoints, unchanged 41 fixed main slots and 11-language
  / 42-string inventory. Pinned build-only Liberation Sans covers Latin accents
  and includes its license under `tools/f6-fonts`; no runtime font binary is added.
- Scene glyph bounding boxes: Treasure 18x14.9, Area Quest 18.675x18.675,
  Mini-game 18.576x16.726 reference units, plus the common approximately 8x5
  downward tip. Area Quest becomes a gray open diamond with three dots; the
  other two keep the lid/clasp chest and purple crossed flags. Twelve fixed
  pieces per marker use the existing 50-slot capacity and 32x36 group bounds.
- Direction tips follow the UI group. Catalog XYZ, raw range/ranking, the
  +100/+180/+150 cm projection-only lifts, distance corrections with zero clamp,
  and Aim/Auto behavior are unchanged. Text slots move inward symmetrically;
  existing projection/candidate frequencies and clustering remain bounded.
- Source-derived normal-scale and enlarged preview reviewed at repository-root
  `out/handoff/SCENE_SG04_SOURCE_ONLY_PREVIEW.png` (matching JSON geometry).
  This is an offline geometry preview, not in-game visual acceptance.
- Core 5/5 and PostRender/runtime gates pass. Strict overlay verification passes
  all 48 scaled-cell bounds plus ten negative mutation checks, including missing
  accents, old-width overflow and overlay visibility/lifecycle regressions.
- Candidate directory: `dist/work/candidates/radar-3.0.0-sg04-20260909`.
  Core 5/5, compact, world-map, PostRender/runtime and release-hygiene/resource
  gates pass. The pinned clean native build passed all 446 targets.
  Superseded SG-03 packages and exact DLL/receipt/metadata are retained in
  `dist/work/candidates/radar-3.0.0-sg03-20260908/superseded-by-sg04`.

SG-04 native identity:

- DLL: 1,196,032 bytes; SHA-256
  `96485EC81471D8D4BD568177A1D18F673CA2553DA60F3047ABAC978E24F881C3`.
- Compiled source SHA-256:
  `CBE6649DB164F5AACF4E6250E6FDCB7C52489A82D48C059F042CF46568C67546`.
- Release-tool SHA-256:
  `337E8940017C4BD9AE1C6021B07357EF0A5DF15CE6C06270826E3B48A286B593`.
- Receipt UTC: `2026-09-09T11:08:33.1000443Z`.
- Candidate retains the DLL, receipt, release metadata, F6 assets and logs.
  `validation-summary.json` records all checks and confirms the installed
  SG-03 DLL and three existing final archives are unchanged. No SG-04 Setup
  or manual-install matrix was run because final packaging is deferred.

No deployment, game control, final package overwrite or external publication
is included in this round. Source and compiler evidence do not establish
runtime appearance, input or performance acceptance.

## SG-03 - 2026-09-08 - display and Esc follow-up; local release handoff

Owner authorized implementation of the final display/input plan, local
deployment, documentation/Nexus closeout and three local 3.0.0 release packages
for their review. The owner will decide publication after testing; no external
upload, Git publication, automatic game launch or game control is included.

Implemented behavior:

- Scene symbols now use a gray exclamation task badge, a wider chest with a
  separate lid/clasp and larger purple crossed flags. Distance text is smaller.
  The raised glyph replaces the ground-tip chevron. The eight-piece fixed glyph
  pool is retained. A source-derived preview was reviewed under
  `out/handoff/SCENE_SG03_SOURCE_ONLY_PREVIEW.png` at the repository root; the
  preview uses a substitute font and is not a gameplay screenshot.
- A copy of the navigation point is raised only for UI projection: Treasure
  +100 cm, Area Quest +180 cm, Mini-game +150 cm. Catalog XYZ, actual distance,
  candidate ranking/range and compact height calculations retain their original
  coordinates. Focus is measured at the raised glyph's visible center.
- Treasure and Area Quest distance labels subtract 1 m; Mole subtracts 2 m in
  total; Fly/Wave remain unadjusted. The value is clamped to >=0 before integer
  rounding. Negative or zero results display `0 m`, never `-1 m`. Explicit
  Mini-game subtype accompanies the marker; the label rule does not change
  shared selection or gameplay interaction reach.
- AIM FOCUS uses the nearest actually shown marker strictly inside a radius of
  10% of the shorter viewport side. One identity must remain selected for 120 ms
  before its label appears. Losing/changing the target hides the label and
  restarts the dwell. AUTO FOCUS considers all shown markers and alone retains
  the 15% replacement buffer. OFF/ALL remain independent label modes. Saved
  `central_radius`/`nearest_center` tokens are unchanged for upgrade compatibility.
- F6 Esc uses two hooks restricted to the current process's verified foreground
  UnrealWindow thread. WH_GETMESSAGE consumes Esc key/character messages before
  dispatch by converting them to WM_NULL; WH_CALLWNDPROC observes synchronous
  focus/lifecycle notifications only. No global or low-level keyboard hook,
  input injection, Unreal query or allocation is added to the callback.
- Esc closes the complete Hub, including its language popup and focused/dragged
  slider. Final controls are sampled before the existing close/persist path,
  including a retained-owner fallback when the current controller probe is absent.
  The same press's repeats and key-up remain consumed after the panel closes;
  a new press after release returns to the game. Focus loss, owner replacement,
  travel and shutdown have explicit native-state cleanup. Hook installation
  failure rejects the opening instead of presenting a page with leaking input.
- All 11 languages use the revised Aim/Auto names. The seven F6 overlays retain
  their 680x896 layout and 41 fixed slots; the required glyph set is now 156.

Technical evidence:

- Candidate: `dist/work/candidates/radar-3.0.0-sg03-20260908`.
- Core build: 5/5 CTest suites pass, including 289 Scene model checks and 50 Esc
  checks against an isolated hidden window's real hook/message-dispatch route.
  No SendInput, foreground switch or game interaction was used by these tests.
- Compact, world-map, PostRender/runtime and release-hygiene gates are part of
  the canonical release pipeline. Scene and Esc gates were updated to verify
  their new behavior while preserving closed-state, lifecycle and pool bounds.
- Clean native build: 446 targets, Windows PowerShell 5.1, pinned CMake 3.29.6,
  Ninja 1.12.1 and MSVC 19.44.35228.0; passed.
- Native DLL: **1,194,496 bytes**; SHA-256
  `4C0B9E788E35A6E46F45B4B5AB6EFCF925B3CDB6CF9E3F3D70E86EB13A853E32`.
- Compiled source SHA-256:
  `F4039912D2A97908B5AAF7083E2254B932EFC0EA41BDA6077FAB99FA5C69C0D5`.
- Release-tool SHA-256:
  `E164FCF7AF93F036B26DC7E0BE12AC60B885A866689A9254A19D8B3D7B8D8138`.
- Receipt UTC: `2026-09-09T04:53:44.6368020Z` (September 8 Arizona time).
- Evidence files: `installed-before.json`, `core-tests.log`,
  `native-clean-build.log` and `release-build.log` in this candidate directory.
  The final release manifest owns package hashes and exact Setup/manual results;
  deployment and independent installed/archive verification are appended here.

Completed package/deployment transaction:

- The first package attempt caught an exact-file-count guard still set to
  42/46. Comparing path sets proved the only addition was the verified Scene
  anchor TSV. Build and manual-test expectations are now 43/47; strict payload
  allowlists remain enforced. The superseded native receipt and failed package
  log are retained in `superseded-package-count/`. The canonical clean build
  above was rerun with the final release-tool identity before packaging.
- The full release pipeline passes Core 5/5, Setup 20/20, Manual 2/2, 177 hotkey
  and 414 visibility assertions, plus all source/resource/release gates.
  `dist/final-3.0.0/release-manifest.json` records all three exact archive hashes,
  4/43/47 file counts, re-extraction and 39-file Setup/manual equivalence.
- Independent archive audit: 1,548 checks, zero failures. The JSON report is
  `out/handoff/SG03_INDEPENDENT_ARCHIVE_AUDIT.json` at the repository root.
- Deployed with the game closed through `Deploy-NativePrototype.ps1
  -Diagnostics Preserve`. Backup:
  `dist/work/deployment/deploy-backups/20260908-220537-090-native-only-deploy`.
  Installed DLL matches the native artifact above. All 35 non-user package
  files match; the four existing user files are byte-for-byte unchanged.
  Exactly one native Radar enable entry remains, with no predecessor entry.
- Verification UTC: `2026-09-09T05:08:44.4698483Z`. `local-deployment.log`,
  `installed-before.json` and `installed-verification.json` capture the local
  transaction. `release-build.log` captures the final successful pipeline.
- Nexus Description, support FAQ, Changelog and three Files entries are ready
  locally. No upload, Git publication or AutoPickup modification was performed.

The preceding SG-02 bytes and all old 2.3.0 archives remain historical artifacts.
Game input routing, visual positioning, first-open/frame-time cost and physical
controller behavior remain exact-SG-03 owner acceptance, not inferred from these
source, compiler or isolated-window checks.

## SG-02 - 2026-09-08 - 3.0.0 built and locally deployed; gameplay pending

Owner authorized the combined scene-guidance and settings changes as 3.0,
including related display/performance/UI improvements and the established local
backup-capable deployment workflow. External publication remains separate.

- Correct Area Quest scene anchors using complete, verified entity XYZ tuples;
  retain existing map coordinates and compact height profiles. Do not invent
  anchors when source evidence is absent.
- Add all catalogued mini-games to the independent Scene column with a purple
  crossed-flags symbol. Use gray Area Quest diamonds and existing chest colors.
- Share configurable 0-1000 m range and 0-50 visible-object budget among enabled
  Scene categories only. Zero disables scene rendering; defaults remain 600/24.
- Add Off / Central radius / Nearest center / All distance modes. The two
  single-target modes select from actually visible markers with switch buffering.
- Set compact Boss/Assault/Area sizes to 35/30/25, with a shared outline stroke
  and the existing player-origin correction and inclusive aligned height band.
- Refactor the F6 layout, add real range/count sliders and translated controls,
  and correct frame/overlay bounds. Preserve user selections on upgrade.

Implementation and source validation:

- All 83 Fly/Mole/Wave mini-games reuse existing save/reward eligibility and
  have a separate identity namespace. Scene category masks remain independent
  of Radar/Map visibility, with one shared bounded candidate selection.
- 143 Area Quests have verified whole entity XYZ anchors. IDs 1101301,
  1103108, 1104104, and 1104203 lack confirmed scene sources and are omitted
  only from Scene. The original 147-row map/height catalog remains byte-identical.
  The reviewed Spirit entity for 1110038 is pinned by exact source ID and an
  11 m planar bound, independently of its old Empty-derived height band.
- Scene anchor TSV SHA-256:
  `6652AC507719F55F03A0C11463838F20AAC875EAA9D6EA66660B7F6F66D945D6`.
- Central-radius and nearest-center modes retain a visible incumbent until
  another target is over 15% closer to center. Invalid/offscreen/cluster-hidden
  incumbents have no grace period. Off hides only labels; zero range/count
  avoids the entire scene service. All remains bounded by the selected pool.
- Text updates are limited to 100 ms and changed integers/identities. Cold
  creation is capped at four TextBlocks/eight FText values per update; cached
  integer values are bounded to 0-1000 and explicitly released on normal detach.
  Text faults preserve icons and have separate diagnostic state. No first-open
  timing or frame-rate guarantee is inferred from these source bounds.
- F6 reference layout is 680x896, with shared bounds for sections, inner frame,
  language dimmer and dismiss target. Two real reflected USliders apply values
  immediately; disk saves debounce for 300 ms and flush on panel close.
- Core clean build: four CTest suites passed, including 1,453 native-state
  assertions, 83 scene checks, event-log queue and encounter-height tests.
- F6 validation: 11 languages / 42 strings each; Korean/Traditional Chinese
  41-slot overlays, 154 required codepoints, seven RLE TGA assets, source/font
  pins and full panel geometry passed. A source/TGA composite preview at
  1080p was visually inspected; it is not a gameplay screenshot.
- Installer visibility parser: 414 assertions passed using the current C#
  source in memory. No new Setup or public archive was built.
- Compact, world-map, runtime-safety and release-hygiene gates passed. The
  first core run exposed a misplaced test-only localization loop; it was
  corrected before the successful run. New slider diagnostic codes avoid
  reusing the retired fatal-font diagnostic path.
- The first 3.0 clean DLL (`26118A1DC625D316CBAF6551D08E8A31E5460A5C9F08DFED00287DFF493EE06C`)
  was rejected by deployment preflight because startup/readiness strings still
  named 2.3.0. It was not installed. Its DLL/receipt/preflight log are preserved
  under the candidate's `superseded-version-log` directory. Those strings and
  their regression gate were corrected for the final clean build.

Final build/deployment identity:

- Candidate: `dist/work/candidates/radar-3.0.0-sg02-20260908`.
- Final native build: Windows PowerShell 5.1, pinned CMake 3.29.6 / Ninja 1.12.1 /
  MSVC 19.44.35228.0 / ExperimentalNested UE4SS; clean 445-target build passed.
- Receipt created UTC: `2026-09-09T01:33:48.9741419Z` (September 8 local time).
- DLL: **1,188,352 bytes**;
  `177E0401718F677EB9DD4C1314A94C0AE7DD13A0208B15BBAF35475934EC094B`.
- Compiled source SHA-256:
  `6B3A61703177267096664F83EEFB3DEB9E343D75C2491165264DF8877753D30E`.
- Release-tool SHA-256:
  `31AB7A9608D3E85CED05F60DDB797CA9D38ED4144FFCCDA7B96356329BDE1F55`.
- Deployment: `Deploy-NativePrototype.ps1 -Diagnostics Preserve`, exit 0.
  All required deployment gates, receipt validation and payload checks passed.
- Rollback backup:
  `dist/work/deployment/deploy-backups/20260908-183551-727-native-only-deploy`.
- Independent installed verification: final DLL hash/size, Scene anchor hash,
  and release metadata match source. Visibility, hotkeys, diagnostics and
  treasure overrides match their pre-deployment hashes byte-for-byte.
  `debug_logging=false` remains unchanged. Evidence:
  `installed-verification.json` and `local-deployment-final.log` in the candidate.
- Final DLL/receipt retained in the candidate's `final-native` directory; the
  prior SG-01 DLL/receipt are in `previous-native` as well as the deployment backup.

The game was closed during installation and was not launched or controlled by
this task. No 3.0.0 public package, Git publication, or AutoPickup change was
performed. Prior dirty AutoPickup paths/diff counts remain unchanged. Actual
gameplay, visual styling, slider interaction, scene distance labels, first-open
cost, frame timing, controller/photo menus and travel remain owner acceptance.

### SG-02 documentation closeout - 2026-09-08

The owner requested independent validation and documentation/Nexus-copy
closeout while testing the installed candidate. This pass left the game
installation, configuration, native source and release scripts unchanged.

- Windows PowerShell 5.1 source-bound receipt verification passed. Build,
  retained candidate and installed DLL still match the final SG-02 identity.
  All 11 installed generated files match source. PowerShell 7 produces a
  different release-tool file-order digest; the canonical 5.1 host matches the
  original receipt. No receipt was regenerated to mask that host difference.
- Existing core binaries rerun: 4/4 CTest. Current C# visibility parser rerun
  in memory: 414 assertions. Compact, world-map, PostRender/runtime and
  release-hygiene gates rerun and pass; the last gate includes F6 asset/geometry
  validation. No duplicate native or Setup build was required.
- README, project context, architecture, provider contracts, acceptance,
  installation guides and release routing now distinguish 3.0.0 state from
  named historical receipts. Descriptive source metadata/defaults are brought
  up to date without replacing the installed metadata snapshot. Compiled source,
  DLL identity and release-acceptance records remain unchanged.
- Local Nexus Description/Quick Support are evergreen, with separate 3.0.0
  Changelog/Files drafts. BBCode and 255-character file-description limits are
  checked. Three planned archive names are explicit drafts, not existing files.

Evidence is retained in the candidate's `documentation-closeout` directory:
`artifact-identity.json`, `installed-metadata-catalog-snapshot.json`,
`verification-results.json`, `ctest-existing-binaries.log`, four verifier logs,
and `VisibilityConfiguration.Tests.log`. Final documentation verification is
recorded in `documentation-validation.json`, separately from those unchanged
runtime/build snapshots: three JSON files, 29 INI/default-profile fields,
43 local link targets across 13 documents, balanced Nexus BBCode and all three
255-character file-description limits pass. Release hygiene and the installed
receipt also pass after the metadata edits; installed metadata remains unchanged.

No 3.0.0 packages, Nexus posts/uploads, Git publication or AutoPickup changes
were made. Owner gameplay/visual/performance results remain pending.

## SG-01 - 2026-09-08 - built and locally deployed; gameplay pending

Owner authorization: implement independent scene guidance for treasure and the
147 Area Quests, with new graphics, plus the existing compact up/down height
triangles for Boss and Assault. This is an unpublished development candidate;
the existing 2.3.0 delivery archives remain historical artifacts.

Constraints:

- Scene Treasure and Area Quests each default off and remain independent from
  compact/map visibility. Reuse numeric catalogs and established eligibility;
  no new global UObject scan, gameplay hook, input capture, or save write.
- Use an independent native UMG host, bounded marker/projection budget, current
  controller projection, and explicit travel/menu/off cleanup. A scene failure
  must leave the existing Radar renderers available.
- Boss/Assault triangles use authored spawn Z with the existing player-origin
  correction and inclusive 500-unit aligned band. Missing height stays unknown.
- Preserve the unrelated AutoPickup worktree. Build and static evidence do not
  establish gameplay performance, visual quality, or owner acceptance.

Implemented display: small chest glyphs preserve the four existing treasure
colors; 147 Area Quests use a white diamond. A downward anchor chevron sits at
the projected navigation point. Only the central focused marker shows distance.
The pool is capped at 24 and 600 m; off-screen/behind-camera points are hidden,
and close screen clusters retain one marker. Full numeric selection runs every
250 ms; the 16 ms pass only reprojects the selected points.

Validation for these exact bytes:

- Windows PowerShell 5.1 clean native build passed with pinned MSVC 19.44,
  CMake 3.29.6 and UE4SS ExperimentalNested tooling.
- Four core CTest suites passed, including 1,106 native state assertions,
  61 scene model checks, encounter height boundaries and event log timing.
- Compact, world-map, retired-canary/runtime safety and release hygiene gates
  passed. F6 verification passed for 11 languages, 7 TGA assets and source/font
  pins. Installer visibility parsing passed 220 assertions without a Setup build.
- Incremental native preflight and final clean build corrected the initial
  weak-pointer assignment ambiguity and SEH/logging unwind separation errors.
  Only final successful bytes were deployed.
- Final verifier-only changes occurred just after the clean build receipt.
  The standard receipt validator confirmed that only `release-tools` differed;
  the original receipt was retained, then the existing helper rebound the same
  DLL/source/toolchain to the final verifier scripts. Full receipt and hygiene
  checks passed afterwards.

Identity:

- Candidate: `dist/work/candidates/scene-guidance-20260908`.
- Runtime version remains `2.3.0`; this does not identify the older CM-04 DLL.
- DLL size: **1,161,728 bytes**.
- DLL SHA-256:
  `CD2BCBA5159C9637C163C183ED1F36634175E4D3DF75B047BC22016C552778B7`.
- Compiled source SHA-256:
  `CD88022E260BC027942B14E60D11B1EF394DEF6F9A8AD24FFB8AE31C489518DF`.
- Clean build completed at `2026-09-08T21:26:25Z`.
- Local deployment used `Deploy-NativePrototype.ps1 -Diagnostics Preserve`.
- Rollback backup:
  `dist/work/deployment/deploy-backups/20260908-142958-918-native-only-deploy`.
- Installed DLL was independently rehashed and matches the clean build.
  Visibility, hotkeys, diagnostics and treasure overrides each retain their
  pre-deployment SHA-256 exactly. Diagnostics remain false.

The game was closed for deployment and was not launched or controlled by this
task. Gameplay appearance, frame-time cost, camera transitions, controller menus,
photo mode, resolution/DPI behavior and travel remain owner acceptance checks.
Photo suppression currently uses the existing minimap-paint/cursor/pause signals.
The 147 task positions are navigation anchors, not inferred live quest actors.
Public archives, Git publication and the preexisting AutoPickup work are outside
this candidate; no public package was generated.
