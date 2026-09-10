# DragonSword Native World Radar 3.0.0 Acceptance Checklist

## Current Guide activity-order checks

Candidate `radar-3.0.0-sg16-guide4-20260909`; version **3.0.0**.

- [x] All eleven Guide atlases move the Marmot/Sudden mission labels with both corresponding icons.
- [x] Left and right entry order matches the requested layout; other Guide cells are unchanged.
- [x] Three height columns, 40 icons and 539 text cells remain.
- [x] Parent DLL, compiled source and localization header are unchanged.
- [x] New deployment verifies the native receipt and 73 UI files and preserves settings.
- [x] Setup 20/20, Manual 2/2, payload equivalence, fresh extraction and final promotion pass.
- [x] The five parent Guide3 final files are backed up and hash-verified.
- [ ] Owner checks the revised activity order in-game across languages and display layouts.

Manifest UTC: `2026-09-10T02:06:02.2485470Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/previous-final-guide3/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide4-20260909/package-verification.json).

The previous Guide3 and earlier SG-16 sections below remain historical records. Their earlier activity order and ZIP hashes identify those sets. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

## Current Guide three-column checks

Candidate `radar-3.0.0-sg16-guide3-20260909`; version **3.0.0**.

- [x] Eleven Guide atlases show only Above, Near level and Below: 40 icons and 539 text cells.
- [x] Parent DLL and compiled source identity are unchanged; runtime unknown-height handling is retained.
- [x] New deployment verifies the native receipt and 73 UI files and preserves settings.
- [x] Setup 20/20, Manual 2/2, payload equivalence, fresh extraction and final promotion pass.
- [x] The five parent SG-16 final files are backed up and hash-verified.
- [ ] Owner checks the three-column Guide in-game across languages and display layouts.

Manifest UTC: `2026-09-10T01:46:51.6121910Z`. The five parent final files are backed up at [dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/previous-final-sg16/).

Evidence: [deployment](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/deployment-verification.json), [release manifest](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/release-packages/release-manifest.json) and [promotion](../dist/work/candidates/radar-3.0.0-sg16-guide3-20260909/package-verification.json).

The previous SG-16 sections below are preserved historical records. Their four-column Guide description and old ZIP hashes describe that earlier set. The new asset and package checks do not establish game visual/input or performance acceptance; these remain owner testing. No Nexus upload or post was performed. Finalized payload documents and metadata remain unchanged after the new package seal.

Mark an item only from evidence for the exact recorded bytes. Source, build,
package, installation, gameplay, performance, and publication are independent.

## Current SG-16 / 3.0.0 delivery (2026-09-09)

Candidate: `radar-3.0.0-sg16-20260909`. Current final packages are verified SG-16; exact identities
and receipts are in [Release status](RELEASE_STATUS.md). These checks separate
source/resource evidence from the owner's live game acceptance.

- [x] Canonical native 446/446, Core 9/9 and all four source/release gates pass.
- [x] Deployment verifies the same DLL/receipt and 73 UI files while preserving
      four settings/data files and recording AutoPickup unchanged.
- [x] Three packages pass Setup 20/20, Manual 2/2, payload equivalence and fresh
      extraction checks: 104 runtime files and 4/108/112 ZIP entries.
- [x] Final promotion backs up and verifies five preceding SG-15 final files.
- [x] Frozen payload docs remain unchanged; Nexus copy has balanced BBCode,
      valid local link targets/HTTPS syntax and three descriptions under 255 characters.
- [ ] Owner checks independent All controls and partial selection states in-game.
- [ ] Owner checks live Map edits, all-off and re-enabling while F6 remains open.
- [ ] Owner checks Auto focus defaults, saved settings, first Aim acquisition
      and stable Aim/Auto target changes during camera movement.
- [ ] Owner checks Guide symbols, height examples, Clock, scrolling and
      language/Scene-label layout on small, standard and ultrawide displays.
- [ ] Owner records visual, input, Guide usability and measured performance acceptance.

No Nexus upload, post or other external publication was performed.

## Historical SG-15 / 3.0.0 release candidate (2026-09-09)

Package candidate: `radar-3.0.0-sg15-release-20260909`. The owner has authorized
three local packages, now verified and promoted. Setup 20/20 and Manual 2/2
pass with 104 runtime files and 4/108/112 ZIP entries. The rebuilt release DLL
was not redeployed; exact identities are in Release status. Prior no-package
decisions below are historical.

- [x] SG-15 runtime baseline: Core 9/9, native 446/446 and four source gates;
      deployment verifies the DLL/receipt and all 73 UI files. Its receipt
      remains distinct from the new package candidate.
- [x] README and both installation guides updated and frozen for staging.
- [x] Fresh canonical native receipt and final gates for the release candidate.
- [x] Installer plus both manual ZIPs use the same source-bound runtime payload.
- [x] Setup matrix, manual-copy matrix and payload equivalence pass.
- [x] Freshly extracted ZIPs match staged files, checksums and exact topology.
- [x] Final directory promotion preserves the previous SG-10 package evidence.
- [x] Record archive sizes, SHA-256 hashes and final manifest in Release status.
- [ ] Owner checks Guide/Settings switching, scroll restoration, language change,
      confirmations, smaller/ultrawide layouts and Aim/Auto camera sweeps.
- [ ] Owner records visual, controller and performance acceptance separately.

No Nexus upload, post or other external publication is part of this request.

## Historical SG-13 / 3.0.0 distance focus (2026-09-09)

- [x] Core 9/9 and native clean build 446/446 match the final SG-13 source.
- [x] Scene 382 checks: Aim 350 ms / Auto 500 ms timing, brief camera sweeps,
      identity reordering, exit margins and target-loss behavior pass regressions.
- [x] Local deployment at 2026-09-09T21:33:45Z verifies DLL/receipt and 61 UI
      files, preserving all four settings/data files and AutoPickup.
- [ ] Owner checks slow/fast left-right sweeps and deliberate target selection
      in both modes; waiting must retain one valid old distance label.
- [x] No release package requested; existing SG-10 ZIPs retained.

## Historical SG-12-H2 / 3.0.0 hotfix (2026-09-09)

- [x] Live logs bind H1's F6 rejection to ABI bit 30; read-only reflection
      confirms that `SetScrollBarVisibility` is absent in this game.
- [x] Preserve core ScrollBox checks, skip unavailable visual setter safely.
- [x] Native build, final gates and installed-file verification.
- [x] Owner confirms F6 opens; two logged openings have zero font/text failures.
- [ ] Actual game Esc-close/reopen, scrolling and all-resolution interaction.

## Historical SG-12-H1 / 3.0.0 hotfix (2026-09-09)

- [x] Correct both UMG ScrollBox parameter names; native build 446/446 passes.
- [x] Compact gate rejects both old names, with eight viewport regressions total.
- [x] All 61 UI files remain identical to SG-12.
- [x] Core 9/9 and all four final gates pass; deployment verified at
      2026-09-09T20:40:45Z, with 61 matching UI files and preserved user settings.
- [ ] Owner confirms F6 opens in game; SG-12's F6 failure is not accepted.

## Historical SG-12 / 3.0.0 local revision (2026-09-09)

All eleven languages use regular-font resources and concise wording. The fixed
Reset / Vote / Feedback footer and header surround a scrolling settings body.
Scene uses frame-local calibrated projection, native fallback, subpixel motion
and bounded visibility hysteresis. See [Release Status](RELEASE_STATUS.md).

- [x] Core 9/9, native clean build 446/446 and all four source/release gates pass.
- [x] Scene 319, projection 39, viewport 83, clock 28, Escape 58 and confirmation
      55 checks pass. Projection tests include a mismatched-camera counterexample.
- [x] Eleven-language resources validate 423 main / 750 help / 250 confirmation
      glyphs. All 43 font/resource and six viewport-source regressions are rejected.
- [x] 53 F6 TGAs, six Scene TGAs and two manifests are verified; 61 UI files total.
- [x] Source previews cover regular font weight, fixed footer, clipped body and
      low-resolution layout. Numeric bounds cover 640x360 through 4K and 21:9.
- [x] Local deployment verified at 2026-09-09T20:18:13Z: DLL, receipt and all
      61 UI files match; four settings/data files and AutoPickup remain unchanged.
- [ ] Owner verifies font appearance, scroll/resize, confirmation input and
      camera-motion/performance in game.

Release packages are deferred at the owner's request. Existing final ZIPs remain
SG-10. SG-11 below was built only and superseded before deployment or packaging.

## Historical SG-11 / 3.0.0 build-only revision (2026-09-09)

This revision shortens all eleven languages' hover and confirmation text and
moves Reset to Defaults, Endorse and Feedback into a separate footer. Confirmation
semantics and runtime display behavior remain unchanged. Exact build, asset,
deployment and package verification are tracked in [Release Status](RELEASE_STATUS.md).

- [x] Core 7/7, canonical native build (446 targets), all four source/release
      gates and the F6 resource gate pass for SG-11. The 27 retained resource
      counterexamples are rejected.
- [x] Source-bound previews verify the 760-by-852 panel, unchanged four settings
      cards, footer at Y 794..844 and popup/modal coverage through the footer.
      Main/popup images are 1520-by-1704; the 206-by-24 Endorse crop keeps the full
      26-pixel font in all 11 languages. Glyph coverage is 180 main / 760 tooltip /
      245 confirmation, with short help and one-question action confirmations.
      Preview source and manifest identities match; static visual review passed.
SG-11 deployment, packaging and game acceptance were superseded by SG-12.

## Historical SG-10 / 3.0.0 local revision (2026-09-09)

SG-10 adds confirmation for Endorse, Feedback and Global Restore Preset.
SG-10 is installed and its verified three-package set occupies
`dist/final-3.0.0`. SG-09 candidate packages were verified but never promoted;
the previous SG-03 final files are backed up in the SG-10 candidate's
`previous-final-sg03` directory. Gameplay and publication remain separate.

- [x] Final Windows PowerShell 5.1 F6 resource/source gate passes. Main layout
      retains 44 overlay slots and 48 scaled language-popup bounds. The main
      record has 43 labels plus 32 help strings per language; the separate
      confirmation record adds seven, totaling 82 x 11 = 902 strings.
- [x] Each language atlas is 640 x 5616: 32 complete hover tiles plus seven
      transparent confirmation-text tiles. All 352 hover and 77 confirmation
      tiles pass source-text, glyph and exact crop checks. Required codepoints:
      180 fixed labels, 1,087 tooltip and 391 confirmation. File inventory
      remains 26 F6 TGAs plus manifest and six Scene TGAs plus manifest, 34 UI files.
- [x] Actual text/control material checks pass 118 states with minimum
      linear-white small-text contrast 4.522:1 and a separate 3:1 title
      threshold. The native confirmation reading surface also passes 4.5:1.
- [x] Twenty-seven SG-10 source/manifest/pixel mutants are rejected, including
      wrong body/crop, missing No, enabled Yes without text, old 31/32-tile
      native atlas acceptance, missing alpha and altered real text pixels.
      Evidence: root `out/handoff/F6_SG10_NEGATIVE_VERIFICATION.json`.
- [x] Nine source-bound previews include English/Chinese full pages and all
      three confirmations, plus an 11-language contact sheet. Visual review
      finds no missing text or clipping. Source hash and atlas identity match
      `out/handoff/F6_SG10_SOURCE_PREVIEW.json`. These are static previews.
- [x] Core passes 7/7, including Scene 313, Clock 28, Escape 58 and
      confirmation 55 checks. Clean native compilation passes 446 targets;
      Compact, WorldMap, PostRender and ReleaseHygiene/resource gates pass.
      Official native receipt UTC: `2026-09-09T16:43:53.6047126Z`.
      DLL: 1,310,208 bytes, SHA-256
      `F1203366DA7488FBFC9E8BF101598A4FC4D2E840F850D66CEC487452FEC7EB65`.
      Compiled-source SHA-256:
      `C999BB82AD356F1C38421868E2A242B7BA0223A2ACD0595ED658829A9A5C380D`;
      release-tools SHA-256:
      `CE5E1B2F5CFE20633EAF0A5B9DB0497A90EEB905495B7A3BB791E97BF9EAD85B`.
      Candidate `validation-summary.json` is the explicit pre-deployment
      build snapshot and retains its original phase.
- [x] Authorized local deployment is independently verified at
      `2026-09-09T16:51:20.6801835Z` by candidate
      `deployment-verification.json`: DLL/build receipt and 34 UI files match,
      four settings files are byte-preserved, two checked AutoPickup files
      are unchanged and one native Radar entry remains enabled. Backup:
      `dist/work/deployment/deploy-backups/20260909-095118-801-native-only-deploy`.
- [x] Release manifest UTC `2026-09-09T16:53:04.9764235Z` records Setup 20/20
      and Manual 2/2 with zero failures/skips, 65 equivalent runtime files,
      4/69/73 archive entries and three byte-identical fresh re-extractions.
      `package-verification.json` verifies all 64 payload files per manual ZIP,
      60 installed static files and final promotion at
      `2026-09-09T16:55:11.2153219Z`. Exact ZIP/Setup identities are in
      `assets/nexus/NEXUS_FILES.txt`; previous SG-03 files are backed up in
      `dist/work/candidates/radar-3.0.0-sg10-20260909/previous-final-sg03`.
      Subsequent deployment/package receipts supersede the frozen metadata
      snapshot's earlier deployment/package status without changing that snapshot.
- [ ] Owner gameplay confirms Yes performs only the selected action; No/Esc
      cancel only, background controls do not receive the dismissal click,
      and missing text retains a readable cancel-only fallback.
- [ ] Owner gameplay/visual/performance acceptance and Nexus publication
      recorded separately; none is inferred from static or package checks.

## Retained SG-09 / 3.0.0 build, deployment and unpromoted packages (2026-09-09)

The owner authorized all six fixes, local deployment and three replacement
packages. SG-09 was independently verified as the installed build and a
candidate package set. Promotion was deferred for SG-10; the final directory
still held SG-03 at this checkpoint. Owner gameplay acceptance and Nexus
publication are separate.

- [x] PS5.1 F6 resource/source verification passes: 26 TGAs, 44 main slots,
      48 popup bounds, 74 strings per language and 341 complete tooltip tiles.
      No layout, language text, resource count or square-check change.
- [x] Soft blue-gray material passes 3,384 card probes and 116 text-slot states:
      15.294% card transmission, 22.353-23.922% gaps, 96-99% tooltip opacity.
      Linear-white small text measures at least 4.522:1; title threshold is 3:1.
      The native status fallback separately measures 4.613:1.
- [x] PS5.1 Scene resource verification passes 4,058 assertions. Only the Area
      Quest interior gains translucent gray-blue fill; its footprint, solid
      rails/dots and lower v are preserved. All five chest/flag textures retain
      their prior bytes. Six Scene TGAs remain.
- [x] Twenty-one targeted pixel mutants are rejected (14 material, seven Scene),
      plus two native status-color regressions. Evidence: root
      `out/handoff/SG09_VISUAL_NEGATIVE_CHECKS.json` and
      `out/handoff/SG09_NATIVE_STATUS_REGRESSION.json`.
- [x] Fourteen source-bound EN/ZH synthetic-background previews and the Scene
      comparison sheet are generated and visually reviewed. Their geometry and
      native palette match source; they do not establish in-game composition,
      target alignment or FPS. Evidence: root `out/handoff/F6_SG09_ALPHA_PREVIEW.json`
      and `out/handoff/SCENE_SG09_SOURCE_PREVIEW.png`.
- [x] Final Core passes 6/6, including Scene 313, Clock 28 and Escape 50 checks;
      the clean native build passes 446 targets. Compact, WorldMap, PostRender
      and ReleaseHygiene gates pass for the frozen SG-09 source.
      Canonical `dist/work/build/native/native-build-receipt.json`, UTC
      `2026-09-09T15:43:06.5903373Z`, records DLL size 1,299,456 bytes and SHA-256
      `EC82CA04927ABCB3148409AEC16D6574641984E4A9F94790515D2B94F800F1BF`.
      Compiled-source SHA-256:
      `9AE2769E0D255A2AF1400123031D395E4993866264DEE919CFD4B3773458CD3B`;
      release-tools SHA-256:
      `83317A019F65844ADCA377E930B9F205B1A1608F0E7C09E203876875ADEB80BA`.
- [x] SG-09 Setup matrix passes 20/20 and Manual matrix 2/2, with zero failures
      and zero skips. Setup/Manual payloads are byte-equivalent across 65 files.
- [x] Clock source review and 28 standalone MSVC helper checks pass: existing
      1 Hz geometry service, RetainerBox/DLayerQuest gap, visual center 15,
      minimum gap 30, screen/host bounds, owner depth eight with exact minimap
      back-reference, paint depth 24, top-offset fallback 178, resize deferral
      and accumulated 0.25-reference-unit position threshold. Actual game
      placement remains unchecked.
- [x] Authorized SG-09 deployment is independently verified at
      `2026-09-09T15:52:57.0256520Z` in candidate `deployment-verification.json`.
      The DLL above, build receipt and 34 UI assets match. Four settings files
      retain their bytes; the two checked AutoPickup files remain unchanged.
      Backup: `dist/work/deployment/deploy-backups/20260909-085255-045-native-only-deploy`.
      One native entry remains enabled. Gameplay stays `PENDING_OWNER_TEST`.
- [x] Three SG-09 packages pass re-extraction byte identity and the canonical
      release pipeline: manifest UTC `2026-09-09T15:56:21.9516109Z`, candidate
      `dist/work/candidates/radar-3.0.0-sg09-20260909/release-packages`.
      Installer/Manual-No-UE4SS/Manual-With-UE4SS contain 4/69/73 entries;
      exact ZIP sizes/hashes are retained in that candidate manifest.
      Setup is 26,894,336 bytes, SHA-256
      `7CC8FDEF406F693955742D6402D42D4C4D481152E22AF147A3150A11DE38480B`.
      Packaged metadata preserves the explicit pre-deployment snapshot; the
      subsequent installation receipt is authoritative over `NOT_PERFORMED`.
- [ ] Owner gameplay validates the +160 cm Treasure UI lift, own-F6 live Scene
      preview, frame-cadence projection, render translation, revised Clock
      placement and blue-gray/quest-backing appearance. No measured FPS benefit
      or game acceptance is claimed.
- [ ] Nexus upload/publication is separately performed and recorded.

## Retained SG-08 / 3.0.0 build and deployment checkpoint (2026-09-09)

SG-08 changes Settings materials and native fallback/slider colors only:
neutral dark gray, restrained transparency, fine edges and calmer active cyan.
Layout, all 11 languages/31 explanations, preset behavior, catalog/height/focus
logic and file counts stay unchanged. Treasure height remains unresolved.
SG-08 is installed following the owner's explicit deployment request;
SG-05/SG-06/SG-07 retain their historical evidence. The three final SG-03
archives retain their original bytes. SG-08 has not been packaged.

- [x] SG-08 deployment independently verified at `2026-09-09T14:50:01.0367076Z`:
      candidate DLL, receipt and 34 UI assets match; four user files preserve
      their exact bytes. AutoPickup's two DLL/config files are unchanged.
      See `dist/work/candidates/radar-3.0.0-sg08-20260909/deployment-verification.json`
      relative to the project root. This supersedes the earlier build-only
      `NOT_PERFORMED` snapshot. The previous SG-05 installation and mods.txt are
      backed up in `dist/work/deployment/deploy-backups/20260909-074959-027-native-only-deploy`.
- [ ] Owner gameplay/visual acceptance of this installed SG-08 build.

- [x] Final F6 resource/source verification passes in Windows PowerShell 5.1:
      26 TGAs, 44 main slots, 48 popup bounds, 74 strings per language,
      177 main and 1,076 tooltip codepoints across 341 complete tiles.
- [x] Independent final-pixel checks cover 3,384 ordinary card-body samples,
      all at alpha 216/255 (15.294% scene transmission). Gaps transmit
      22.353-23.922%; tooltip reading surfaces remain 96-99% opaque.
      Neutral tint and at least 4.5:1 text contrast under linear-light white
      are required; card-body RGB (247,253,255) measures at least 4.662:1.
- [x] All 116 text-slot states (44 main, two numeric values and twelve popup
      choices, each inactive/active) pass actual layered-pixel checks. Small
      text measures at least 4.535:1 on linear white; the large title is checked
      against 3:1. Utility buttons/language/status reading plates prevent the
      formerly uncovered small text from blending into a bright scene.
- [x] All 62 in-memory regressions are rejected: the inherited 42 plus excess
      opacity/transparency, a byte-space-readable but linear-light-unreadable
      surface, blue tints, opaque gaps, weakened tooltip surfaces, duplicate native backdrops and
      missing utility/text reading protection or unreadable control labels. Evidence: root `out/handoff/F6_SG08_NEGATIVE_CHECKS.json`.
- [x] Fourteen source/asset previews cover EN/ZH, three original procedural
      backgrounds and sRGB/linear composition, plus popup/tooltip examples.
      DAY/DARK linear style has been reviewed. The preview JSON at root
      `out/handoff/F6_SG08_ALPHA_PREVIEW.json` binds final hub SHA-256
      `91EA65DB24169AAD0135B6333A030042DC69E28A75370BFB25D5B5CE1E658E01`.
      Slider/status authored colors are parsed from final native constants and
      actual API bindings. No user screenshot is altered or reused as a backdrop.
      These are static readability bounds; actual game composition, tonemapping,
      font metrics, slider brush and tooltip placement remain unverified.
- [x] Final SG-08 Core passes 5/5; the clean native build passes 446 targets.
      The official `dist/work/build/native/native-build-receipt.json` dated
      `2026-09-09T14:31:08.5695045Z` records a 1,288,192-byte DLL, SHA-256
      `5E4045E3160E00A5FA234167926E72E318EDF7F92F8A059E5AB8A832D70DD381`.
      Compiled-source SHA-256 is
      `2FC8A4B44415A9B5C42ED444802F1A231387A5D5DBAF9574EA9CC41A0A56D13F`;
      release-tools SHA-256 is
      `67B37DF590B7118308F70BA56B8D39745721F72C5F02C4ED4159C650769CE394`.
      Compact, WorldMap, PostRender and ReleaseHygiene pass; final logs are in
      `dist/work/candidates/radar-3.0.0-sg08-20260909`. This is local build and
      static verification: installed SG-05 and final SG-03 archives were unchanged
      at that pre-deployment checkpoint; the subsequent SG-08 deployment above
      supersedes its installation status.
- [ ] Owner gameplay, real transparency/readability across lighting and DPI,
      input and performance are accepted after a separately authorized deployment.

Future runtime/manual counts remain 65 files (64 plus manifest, whose 61 members
exclude three examples) and 69/73 entries. No new final package or Nexus upload
has been created; this style pass is not a Treasure-height fix.

## Retained SG-07 / 3.0.0 local build checkpoint (2026-09-09)

SG-07 expands setting-specific hover help; it changes no Scene coordinates,
height thresholds, UI geometry, preset semantics or package file counts.
The Treasure height report is still under investigation, not a verified fix.
SG-05 remains installed, with the retained deployment identity below. SG-06 is
a completed local build checkpoint. The three final SG-03 ZIPs retain their
original 4/43/47 entries, sizes and hashes. SG-07 is not deployed or repackaged.

- [x] Windows PowerShell 5.1 strict F6 source/resource verification passes for
      31 tooltip topics, 74 strings per language (43 main plus 31 help), 814
      wide literals, 44 main slots and 48 scaled popup bounds. All 26 RLE TGAs
      retain the existing file inventory; eleven atlases are now 640-by-4464.
- [x] Main glyph coverage remains 177 codepoints with minimum fit 1.000 and
      maximum optical-center error 0.5 raster px. All 341 tooltip tiles cover
      1,076 codepoints without source-text loss, clipping, split ASCII tokens
      or detached Thai combining marks; each displays at 320 by 72 units.
- [x] Seven marker topics bind each actual Radar/Map column's category.
      Seven independent row-name hover targets remain available behind the
      packaged main-text route and end before the checkbox columns. Three
      Scene, five height and four filter choices bind their own explanations;
      twelve common controls retain separate help. Fifty-five owners fit the
      unchanged 64-record pool with bounded shared-atlas import/fallback.
- [x] Help explains Treasure colors/opening conditions, 9 Bosses, 40 Assaults,
      147 Area Quests and 83 Mini-games, completion/filter limits, loaded-only
      Bird Eggs and the game Clock. The unchanged Mole height label explicitly
      explains its shared 33 Fly / 40 Mole / 10 Wave coverage. It does not
      promise an actor is loaded, a reward chest is the same activity point,
      or that changing a filter unlocks a quest or starts an event.
- [x] Forty-two in-memory F6 regressions are rejected, preserving all SG-06
      failures and adding wrong category/Scene/height/filter bindings, old
      18-topic state, disabled row hover and hover interception of checkboxes.
      Evidence: root `out/handoff/F6_SG07_NEGATIVE_CHECKS.json`.
- [x] Source-derived main/popup/all-language previews and complete EN/ZH
      31-topic sheets are retained under root `out/handoff/F6_SG07_*`.
      All 438 reference-font runs fit; the complete Chinese sheet has been
      visually reviewed. These are source/raster checks, not in-game captures.
- [x] Final SG-07 clean native build passes all 446 targets; Core passes 5/5.
      The official `dist/work/build/native/native-build-receipt.json` at
      `2026-09-09T13:59:17.4623002Z` records a 1,288,192-byte DLL, SHA-256
      `050897A864CB2FAD699BF7A351B58949C60072C9BC648CBF72EDBEB1386637F6`.
      Compiled-source SHA-256 is
      `4A7689300D57F4F065AEF480CB078B420EDA752A36FF4447E17F062CC64C0481`;
      release-tools SHA-256 is
      `67B37DF590B7118308F70BA56B8D39745721F72C5F02C4ED4159C650769CE394`.
- [x] Compact, WorldMap, PostRender and ReleaseHygiene gates plus the complete
      F6 gate pass. Final Core/native/gate logs are retained under
      `dist/work/candidates/radar-3.0.0-sg07-20260909`. These verify the current
      local source/build; they do not change the installed SG-05 or SG-03 ZIPs.
- [ ] Owner gameplay validates every actual hover target, language fallback,
      viewport-edge placement, repeated opening/travel and live input/font
      behavior after a separately authorized deployment. No measured FPS,
      Treasure height correction or Nexus publication is claimed.

The unchanged future contract is 65 runtime files: 64 payload files plus the
manifest, whose 61 members exclude three example files. Future manual
No-UE4SS/With-UE4SS counts remain 69/73. No new archives were generated.

## Retained SG-06 / 3.0.0 local build checkpoint (2026-09-09)

SG-05 is the installed baseline, verified at
`2026-09-09T12:30:52.1411039Z` by
`dist/work/candidates/radar-3.0.0-sg05-20260909/deployment-verification.json`.
Its DLL SHA-256 is
`C04C6E29D1113B183D8ED511C00BBE7482E46428B6EE53FFDA889319CC252B97`.
SG-06 has not been deployed or repackaged. All three final ZIPs still contain
SG-03 and retain their original 4/43/47 entries, sizes and hashes.

- [x] Windows PowerShell 5.1 strict F6 resource/source gate passes for 26 RLE
      TGAs plus manifest: nine fixed-label images, 11 tooltip atlases and six
      procedural glass/check/chip skins. All 11 language records contain 61
      strings; 44 main slots and 48 scaled popup bounds remain valid.
- [x] Main glyph coverage is 177 codepoints, minimum fit 1.000 and optical-center
      error at most 0.5 raster px. Tooltip coverage is 878 codepoints across
      198 complete tiles; every tile fits its 320-by-72 reference rectangle
      without truncation, source-text loss or detached Thai combining marks.
      Font hashes and the unmodified Thai OFL license are pinned.
- [x] Thirty-two in-memory F6 regression cases are rejected, covering the
      previous accent/AUTO/highlight faults and new square-hit overlap,
      Clock order, tooltip clipping/index/cleanup, failed-language fallback,
      nine-slice brush readback, sRGB decoding and altered image pixels.
      Evidence: root `out/handoff/F6_SG06_NEGATIVE_CHECKS.json`.
- [x] EN/ZH, language popup and hover-help source previews were visually
      reviewed; the all-language sheet and complex hover explanations are
      retained under root `out/handoff/F6_SG06_*`. All 438 reference-font
      runs fit. Preview fonts/slider appearance and sample hover position
      remain substitutes for live game rendering.
- [x] Source/resource review confirms 760-by-792 four-card layout, a top-level
      Restore Preset, 22-by-22 check visuals in non-overlapping 24-by-24 hits,
      Clock after Bird Eggs, six shared skins and reflected Box nine-slice.
      All 18 setting explanations have an independent native owner, with one
      shared language atlas, bounded import/fallback and no new hover polling.
- [x] Final SG-06 clean native build passed all 446 targets. The receipt at
      `dist/work/build/native/native-build-receipt.json` is dated
      `2026-09-09T13:25:19.6896094Z`; DLL size is 1,251,328 bytes, SHA-256
      `D1A521ABD1FDE80C1C7108FB06B980A7525E35412A54217AC21CDD25196C1AAC`.
      Compiled-source SHA-256 is
      `E24EEE9A882E1BA8E65619799A9EB0CF445B58125DD83B83A22616BCC6DB5908`;
      release-tools SHA-256 is
      `EA24111BE1836ACC1EB9E753D703117E0273A8012AE9F0E7D273F457457F8BA5`.
- [x] Core suites pass 5/5. Compact, WorldMap, PostRender and ReleaseHygiene
      gates all pass; final evidence is retained under
      `dist/work/candidates/radar-3.0.0-sg06-20260909` in
      `core-clean-build.log`, `native-clean-build.log` and the four gate logs.
      The separately named first-build failure is historical. Source-preview
      identity was refreshed after the SDK pointer-type compilation repair;
      its hub SHA-256 is
      `65619F746CBA0F2AA8AED9E683884F43BA37971FFBDF6328460373AD3D03EAA8`.
- [ ] Owner gameplay, actual font/tooltip/input behavior, resolution and
      frame-time acceptance are recorded after an authorized SG-06 deployment.

The future SG-06 payload contract is 65 runtime files and 69/73 manual
No-UE4SS/With-UE4SS entries. Those are planned allowlist counts, not freshly
generated archives or successful installer-matrix evidence. There is no
measured FPS claim. Nexus text remains a local draft.

## Retained SG-05 / 3.0.0 build and subsequent deployment (2026-09-09)

The initial SG-05 source/build checkpoint below skipped deployment; the owner
later authorized deployment. Its verification receipt above supersedes that
earlier skip status: four user-owned files were byte-preserved and 17 UI files
matched. Backup: `dist/work/deployment/deploy-backups/20260909-052906-388-native-only-deploy`.
SG-05 did not replace the three SG-03 final archives. The following source/build
results retain their original scope; Scene-only reset is historical SG-05 behavior.

- [x] F6 source/asset verification covers the 760-by-792 panel, 44 disjoint main
      text slots and 43 strings in each of 11 languages. Nine TGAs plus manifest
      contain 177 required codepoints: seven 1520-by-1584 full-panel images and
      two 440-by-52 French/Spanish selected-name images.
- [x] Windows PowerShell 5.1 strict F6 verification passes: minimum fit 1.000,
      maximum optical-center error 0.5 px, no clipping, exact source/font pins,
      11 x 3 x 44 language/status inventory and 48 scaled popup-cell bounds.
      All sixteen in-memory negative regressions are rejected. English,
      Simplified Chinese, popup and all-language source previews are recorded
      under root `out/handoff/F6_SG05_*`; 396 native reference-font runs fit.
      Preview fonts and native slider appearance are substitutes, not game QA.
- [x] Source review records the two-column Radar/Map table and independent Scene
      chips, three-plus-two Height chips and side-by-side Filter groups.
      Restore Preset affects only Scene: categories Off / 600 m / 24 / Aim Focus;
      other visibility, height, filter, language and hotkey preferences persist.
- [x] Scene source uses six shared textures, 50 glyph Images and six collapsed
      keeper Images instead of 600 Border pieces. Menu sampling preserves the
      renderer tree. SetPosition skips displacement of at most 0.25 physical px
      measured from the last submission, allowing small movements to accumulate.
- [x] Eight additional in-memory Scene regression cases are rejected. These
      static negatives supplement resource and source checks; they do not
      establish live texture lifetime, visual or performance acceptance.
- [x] Aim source uses normalized elliptical distance with radii 16% horizontally
      and 34% vertically of the shorter viewport side and a 120 ms dwell. Auto
      retains Euclidean center distance and its independent 15% switch buffer.
      Raw XYZ, projection lifts, displayed-distance corrections and filtering
      remain on their existing paths.
- [x] Final SG-05 clean native build passed all 446 targets. The receipt at
      `dist/work/candidates/radar-3.0.0-sg05-20260909/native-build-receipt.json` is dated
      `2026-09-09T12:10:21.3873261Z`. Its DLL is 1,200,128 bytes, SHA-256
      `C04C6E29D1113B183D8ED511C00BBE7482E46428B6EE53FFDA889319CC252B97`.
      Compiled-source SHA-256 is
      `16A389B6F543F0A54496FFD90ECC07DE0D7C1AE920CF5116F19F4C3E4DC9B207`;
      release-tools SHA-256 is
      `C65867EDC916C3727A312DC4370ED0DF39F55F461036637F11D076C590878A05`.
- [x] Final Core suites passed 5/5, including Scene 308 checks / zero failures
      and Esc 50 checks. Compact, WorldMap, PostRender and ReleaseHygiene gates
      all passed. Final logs are retained under
      `dist/work/candidates/radar-3.0.0-sg05-20260909`; `core-build.log` contains
      the successful test run. Renamed first-attempt failure logs are historical.
- [x] Eleven additional in-memory Compact shutdown regressions are rejected,
      retaining the atomic-only process-shutdown and bounded Scene handle-
      abandonment contracts. These are static checks, not gameplay acceptance.
- [x] Subsequent SG-05 deployment is recorded by the exact verification above.
- [ ] Owner gameplay, input, visual, resolution and performance acceptance is
      recorded separately for those installed bytes.

At the SG-05 checkpoint, the planned package contract was 48 runtime files and 52/56 manual
No-UE4SS/With-UE4SS archive entries. These are not newly generated packages or
successful installation-matrix results. There is no measured SG-05 FPS result.
The retained SG-03 archive sizes, hashes and counts below remain unchanged.

## Retained SG-04 / 3.0.0 source-only checkpoint (2026-09-09)

The owner closed the game and explicitly requested skipping deployment because
more changes are planned. This round covers code, native compilation and gates;
SG-04 deployment and replacement public packages are outside this round's scope.
At that SG-04 checkpoint, the installed DLL and three final archives remained SG-03. Their
recorded hashes below must not be presented as SG-04 artifacts.

- [x] F6 preserves all 11 languages / 42 strings and 41 main-overlay slots.
      Nine TGA assets plus manifest cover 170 required codepoints. Popup Korean,
      Traditional Chinese, French and Spanish names are independent of page
      language, including English; two 560-by-54 images cover only the selected
      French/Spanish LanguageValue, including AUTO resolution.
- [x] PS5.1 strict asset verification passes: minimum fit 1.000, maximum optical
      center error 0.5 px, no clipping, both pinned fonts and correct accented
      endonym pixels. No native language name was simplified to remove accents.
- [x] The actual selected Border is bound to a 186-by-28 rectangle inside the
      190-by-32 cell with inset two. All 12 cells at four scales pass 48 bounds.
      Ten negative regressions are rejected, covering the old 353-unit width,
      missing/English-gated popup fallback, broken AUTO, premature native-text
      hiding, missing retry reset and corrupted French/Spanish pixels in both
      popup and selected-name assets.
- [x] Source review records approximately 18-19-unit Scene glyph widths, an
      open gray quest diamond with three white dots, and the small shared `v`.
      The fixed marker pool is 12 pieces (eight glyph plus four chevron pieces).
      Catalog positions, UI lifts, distance corrections and focus rules remain
      unchanged; these source checks do not establish live visual acceptance.
- [x] Final SG-04 clean native build passed all 446 targets; receipt UTC is
      `2026-09-09T11:08:33.1000443Z`. The DLL is 1,196,032 bytes, SHA-256
      `96485EC81471D8D4BD568177A1D18F673CA2553DA60F3047ABAC978E24F881C3`.
      Core tests passed 5/5 and Compact, WorldMap, PostRender and ReleaseHygiene
      gates passed, including F6 resource verification. Evidence is recorded in
      `SCENE_GUIDANCE_ATTEMPT_LEDGER.md` and
      `dist/work/candidates/radar-3.0.0-sg04-20260909/validation-summary.json`
      (verified UTC `2026-09-09T11:15:33.2586503Z`). This summary also confirms
      skipped deployment, the unchanged installed SG-03 DLL and three archives,
      and that the game was not running; SG-04 gameplay remains unvalidated.
- [ ] After a later authorized deployment, owner gameplay/visual/input/
      resolution/performance results are recorded for its exact bytes.

Nexus Description/FAQ and the pending changelog are local drafts for a later
merged release. `NEXUS_FILES.txt` continues to identify the unchanged SG-03
archives. No SG-04 deployment, package replacement or Nexus upload is claimed.

## Retained SG-03 / 3.0.0 package/deployment checkpoint (2026-09-08)

SG-03 supersedes the earlier developer candidate. Exact new DLL, source,
installation and package identities belong to `SCENE_GUIDANCE_ATTEMPT_LEDGER.md`
and `RELEASE_STATUS.md`. Three local release archives are ready and
rollback-backed deployment is independently verified. Preparing release copy
does not establish publication or owner gameplay acceptance.

The SG-03 manifest was generated at `2026-09-09T05:00:35.6146355Z` and is now
retained at `dist/work/candidates/radar-3.0.0-sg10-20260909/previous-final-sg03/release-manifest.json`.
Its native DLL is **1,194,496 bytes**,
SHA-256 `4C0B9E788E35A6E46F45B4B5AB6EFCF925B3CDB6CF9E3F3D70E86EB13A853E32`.
`SHA256SUMS.txt` identifies the three archives; their sizes and hashes were
independently read back during this documentation closeout.

- [x] All 11 runtime languages use the four revised distance labels. English
      is OFF / AIM FOCUS / AUTO FOCUS / ALL; Simplified Chinese is 不显示 /
      瞄准显示 / 自动聚焦 / 全部显示. Persisted enum/string tokens are unchanged.
- [x] Seven F6 overlay TGAs and their manifest were regenerated and verified
      under Windows PowerShell 5.1: 41 main slots, 156 required codepoints,
      minimum fit 1.000, maximum optical-center error 0.5 px, no clipping,
      and unchanged 680-by-896 layout and 2x raster dimensions.
- [x] SG-03 Core 5/5 suites, including 289 Scene model and 50 hidden-window
      Escape checks, pass. The clean native build and all four static gates
      pass for the finalized source and DLL; offline input checks are not game
      input acceptance.
- [x] Setup 20/20, manual installation 2/2, hotkey parser 177 assertions and
      visibility parser 414 assertions pass. The three archives contain 4/43/47
      entries respectively; all 39 runtime payload files match across packages.
- [x] The 3.0.0 Setup/ZIP set is generated and independently validated against
      its own release manifest and hashes, including byte-identical re-extraction.
- [x] Rollback-backed SG-03 local deployment is independently verified at
      `2026-09-09T05:08:44.4698483Z`: `game_running=false`, all four user-owned
      files preserved byte-for-byte, all other 35 payload files matching the
      package, and exactly one native enabled entry. Evidence is
      `dist/work/candidates/radar-3.0.0-sg03-20260908/installed-verification.json`;
      backup is `dist/work/deployment/deploy-backups/20260908-220537-090-native-only-deploy`.
- [ ] Owner gameplay, visual, input, resolution and performance acceptance is
      recorded for those exact bytes.
- [ ] A publication/upload is explicitly performed and recorded.

## Historical SG-02 / 3.0.0 technical evidence (2026-09-08)

Exact source/build/install evidence is recorded in `SCENE_GUIDANCE_ATTEMPT_LEDGER.md`.
This was the preceding locally deployed developer baseline. The
recorded DLL is **1,188,352 bytes**, SHA-256
`177E0401718F677EB9DD4C1314A94C0AE7DD13A0208B15BBAF35475934EC094B`.
The evidence directory is `dist/work/candidates/radar-3.0.0-sg02-20260908`.

- [x] Four core CTest suites passed; the final clean native build passed and
      produced the source-bound receipt. `core-tests.log` and
      `native-clean-build-final.log` identify these checks.
- [x] Compact, world-map, runtime-safety and release-hygiene gates passed for
      the deployed source; `local-deployment-final.log` records all four passes.
- [x] Scene source reproduction retains 147 IDs, 143 available / 4 unavailable,
      with the original map/height catalog unchanged. F6 static validation
      covers 11 languages / 42 strings, 41-slot Korean/Traditional Chinese
      overlays, 154 codepoints and seven pinned TGA assets.
- [x] Current-source installer visibility parsing passed 414 assertions as
      recorded in the SG-02 ledger; this did not build a new Setup.
- [x] Rollback-backed local deployment and `installed-verification.json`
      confirm the DLL, Scene anchor hash and release metadata. User visibility,
      hotkeys, diagnostics and treasure overrides were preserved byte-for-byte;
      diagnostics remained off at that deployment checkpoint.
- [ ] Owner gameplay, visual, controller, resolution and performance acceptance
      is recorded for these exact bytes.
- [ ] A 3.0.0 public Setup/ZIP set is generated and independently validated.
- [ ] A 3.0.0 publication/upload is performed and recorded.

The final two items were not performed at that SG-02 checkpoint.
Static/core/build/install success does not mark any gameplay check below as
accepted or establish identity for the successor SG-03 candidate.

## SG-09 owner checks for the authorized deployment

- [ ] Scene Treasure/Area Quests/Mini-games each work while compact/map
      counterparts are off. Clean defaults and absent legacy Scene keys are
      on; explicit existing choices, including false, survive upgrades.
- [ ] All four Treasure colors and purple Mini-game flags remain legible at
      approximately 18-19-unit widths, alongside the SG-06 Area Quest diamond
      body enlarged by 18% to approximately 22 units with a 1.8-unit outline.
      Their small `v` and distances stay clear across supported resolutions/DPI
      without looking like exact ground anchors.
- [ ] Validate the 143 authored Area Quest Scene origins, including the corrected
      `1110033`, reviewed `1110038` and two-band `1103061`. The four unproven IDs
      (`1101301`, `1103108`, `1104104`, `1104203`) show no fabricated Scene point
      while their compact/map presentation remains available.
- [ ] The 83 Fly/Mole/Wave activity locations remain distinct from reward-chest
      locations and obey their existing completion/eligibility policy.
- [ ] Default 600 m / 24 and configurable limits through 1000 m / 50 behave
      correctly; either zero setting stops Scene work. Dense clusters remain
      bounded and do not create mirrored, stale or off-screen markers.
- [ ] Distance Off, Aim Focus, Auto Focus and All select the expected labels.
      Aim requires the same nearest normalized-distance target strictly inside
      the 16% horizontal / 34% vertical ellipse for 120 ms; both radii use the
      shorter viewport side. Leaving it immediately hides the label.
      Auto retains a valid visible focus until another point is more than 15%
      closer to center. Check dwell resets, mode changes and viewport edges.
      Compare cold text creation (at most four TextBlocks/eight FText
      values per update) and warm 100 ms changed-value refresh. Pending/faulted
      labels must not leave stale text or suppress otherwise valid icons.
- [ ] Projection-only lifts (+160 cm Treasure, +180 cm Area Quest, +150 cm
      Mini-game) improve visibility without changing any catalog XYZ, raw-anchor
      distance, range eligibility or compact/map height result. Scene distances
      round `max(0, d - 1)` for Treasure/Area Quest, `max(0, d - 2)` for Mole,
      and unchanged `d` for Fly/Wave. Nearby values never become negative;
      displayed zero is not a guarantee that interaction is available.
- [ ] Both Scene sliders update immediately, zero range/count hides Scene, and
      values persist after 300 ms or panel close. Verify the complete 760-by-792
      panel, all cards/chips and language modal at supported resolutions/DPI.
- [ ] Global Restore Preset enables every supported Radar/Map category and all
      five heights, restores Scene all On / 600 m / 24 / Aim Focus, filters
      Available and language AUTO. Runtime Mod state and startup hotkeys stay
      unchanged. Verify pending edits persist even if controls already match.
- [ ] A new/missing Scene configuration defaults to all three categories On;
      existing explicit false choices survive upgrades until the owner resets.
- [ ] Square check visuals and hits do not overlap adjacent rows; Clock is
      last. Glass chips keep circular corners at narrow/wide widths and DPI.
      Soft blue-gray surfaces visibly transmit the scene while labels remain
      readable in bright/dark gameplay; validate actual tone mapping and final
      composition. Missing skins retain readable native blue-gray controls.
      Source sRGB/linear previews do not establish these in-game results.
- [ ] Hover every setting and all seven category row names in all 11 languages:
      each column/category, Scene switch, height and filter choice shows its
      specific topic among all 31; row-label targets never block checkboxes.
      Verify complete text, correct tiles and stable viewport-edge placement.
      Range means 0..1000 m, not a 1000 m projection lift. Missing atlas,
      language switching, repeated open/close and travel must not show an old
      language, corrupt brush, blank retained custom tooltip or leaked owner.
- [ ] Escape closes the entire F6 page with language popup open, slider dragging
      or control focus, retains the last values, and does not open the game's
      menu with the same press/repeat/release. A subsequent independent Escape
      press works normally. Verify loss of focus, travel and reopening; blocked
      hook setup must refuse the panel instead of permitting passthrough.
- [ ] AUTO remains the first language option and stays persisted; F6/F7 refresh
      the displayed game language. Manual choices remain fixed. Settings,
      Enable and Disable respect the configured hotkeys without F6 rewriting
      the separate `hotkeys.ini` file.
- [ ] Every language highlight remains within its own cell. In an English page,
      Français and Español (España) retain ç/ñ in the popup; selecting either
      language and AUTO resolving to it retain the complete main-card name.
      Verify repeated open/close, language/status changes and missing-resource
      fallback without duplicate labels, stale names or missing native text.
- [ ] Opened treasures/completed quests disappear; camera turns and close
      clusters do not produce stale or mirrored markers.
- [ ] Own F6 Settings previews Scene changes over active gameplay. Real game
      menus, pause, expanded map, controller menus, activities and HUDHidden
      retain suppression; travel, F8/F7 and title/save transitions still hide
      or rebuild the host correctly without stealing input.
- [ ] Routine menu-state samples do not detach or rebuild Scene; hidden-menu
      transitions reuse its tree. Texture kinds remain valid across category
      changes, garbage collection and reopening; six imports belong to attach
      only. Genuine world/disable/fault cleanup still releases the tree.
- [ ] Slow camera motion accumulates from the last submitted group position
      and updates once displacement exceeds 0.25 physical pixels; unchanged
      positions skip render-translation writes without leaving stale focus or
      distance labels. Projection follows each EngineTickPost frame; catalog
      selection remains at 250 ms and no per-frame candidate sort is added.
- [ ] The Clock glyph visually centers between the minimap and quest tracker
      with a usable gap. Resize, hidden/stale geometry, insufficient gap and
      owner changes keep the bounded fallback and do not overlap other HUD.
- [ ] Boss/Assault/Area Quest compact sizes 35/30/25 and shared 4-unit visible
      triangle/frame stroke remain consistent after display/DPI scaling, with
      category colors and Encounter dark-green outlines preserved.
- [ ] Boss/Assault above/below triangles and aligned original glyphs use the
      authored spawn Z, `player.z - 150` and the shared inclusive +/-500 margin.
      Test both boundary crossings, unknown source and independent height
      switches; no added monster-center offset or unrelated height change.
- [ ] Dense-scene frame-time comparison records disabled and default-enabled
      Scene costs without assuming source work reductions equal higher FPS.

These are owner gameplay checks, not claims established by compilation.
SG-09 deployment and packaging are independently verified as recorded above.
That success and the historical SG-08 installation/older builds do not establish
SG-09 gameplay, visual, resolution, controller or performance acceptance.
Within the named historical sections,
"current" refers to that section's recorded candidate, not today's baseline.

## Historical 2.2.1 acceptance state

- [x] Source identity is `2.2.1` and runtime label is
      `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_1` in every compiled,
      installer, metadata, and package authority.
- [x] Source review, Core `2/2`, static gates, release hygiene, and the clean
      native build pass for the exact full-stretch-host/inner-atlas candidate
      bytes.
- [x] Rollback-backed diagnostics-enabled developer deployment installs the
      exact current DLL while the game is stopped, verifies the installed hash
      and size, retains exactly one Radar entry in `mods.txt`, preserves
      `debug_logging=true`, and leaves the game stopped after deployment.
- [x] Installer `20/20`, manual `2/2`, payload-equivalence, clean-target,
      archive-integrity, and final-package gates pass against the exact 2.2.1
      DLL and `dist/final-2.2.1` manifest.
- [ ] The exact 2.2.1 package is installed and gameplay-tested with native icons
      and click targets stable through pan, zoom, reopen, travel, 4K, 21:9,
      16:10, DPI, windowed, dense-Treasure, controller, and clean-exit cases.
- [ ] Both Mod atlas hosts remain hit-test-invisible children of the directly
      resolved current `DLayerMap.FogAbovePanel`. `ArrayIconInfo` supplies only
      a creation-time instantiable icon class and is not scanned by retained-host
      validation/refresh. Each outer slot is full stretch with zero offsets,
      `AutoSize=false`, zero alignment, and maximum Z. Each cloned inner
      `Panel_Point` slot is independently reasserted as full stretch with zero
      offsets. Only the Image Canvas slot owns
      `{atlas_left,atlas_top,atlas_width,atlas_height}`; Image render translation
      remains `(0,0)`. No Mod-owned negative outer offset may feed back into the
      native parent's desired extent, and no host transform or forced prepass is
      used.

Status: `SOURCE_VALIDATED = PASSED_FOR_FULL_STRETCH_HOST_INNER_ATLAS_CANDIDATE_WITH_RELEASE_HYGIENE`

Status: `BUILT = PASSED_FOR_WM_06_IMMUTABLE_SLOT_CANDIDATE`

Built DLL SHA-256: `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`

Compiled-source SHA-256: `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`

Built DLL size: `1,107,968` bytes

Status: `INSTALLER_TESTED = PASSED_CURRENT_WM06_SETUP_20_OF_20_MANUAL_2_OF_2_AND_PAYLOAD_GATES`

Status: `PACKAGED = PASSED_CURRENT_WM06_THREE_ARCHIVE_BYTE_IDENTICAL_REEXTRACTION`

Installer ZIP SHA-256: `A7F4065F52C4032A26B0D93FA7074FA478C1F985B98315C498DFEA23F44FC025`

Manual No-UE4SS ZIP SHA-256: `5ED7B0736C27B521CD11381EB912AE32068FD360B043DB9D35FBCA3CC97FEF14`

Manual With-UE4SS ZIP SHA-256: `D54AF55078C972DE4044403BBA29E2785821C73B8747C9CF306587A9B21595F2`

Status: `DEPLOYED = PASSED_ROLLBACK_BACKED_DIAGNOSTICS_ENABLED_CURRENT_CANDIDATE`

Status: `GAMEPLAY_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `VISUAL_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `RESOLUTION_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `CONTROLLER_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `RESPONSIVE_UI_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `LOCALIZATION_GLYPHS_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `PERFORMANCE_ACCEPTED = NOT_VALIDATED_FOR_2_2_1`

Status: `BINARY_AND_DERIVED_DATA_PUBLICATION = BLOCKED`

Current developer-deployment backup:
`dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`

The deployed and installed DLL both match SHA-256
`6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`
and size 1,107,968 bytes. The controlling `mods.txt` contains exactly one Radar
entry, `debug_logging=true`, and the game was stopped after deployment. This
developer deployment proves only rollback-backed local installation identity;
it is not Setup ownership, gameplay acceptance, visual acceptance, resolution
acceptance, or performance acceptance. Package and installer evidence is
recorded separately against the same exact DLL in `dist/final-2.2.1`.

The prior independent-viewport/extreme-Z candidate is runtime rejected. It made
markers visible, but live testing reported severe lag, wrong placement, and
delayed updates. The later first-valid-parent candidate is also rejected:
temporary topology logs show zoom-driven native icon reconstruction alternating
the selected parent between `FogAbovePanel` and `FogUnderPanel`, with four Mod-
host reattachments in one sequence, fog occlusion, hitching, and flashing.
The later full-stretch-outer/Image-translation candidate, DLL
`CCC6B1170...6AE00` from compiled source `B650B5FB...74EA`, is runtime rejected
for its measured zoom-pivot misalignment. The subsequent outer-atlas-rectangle
candidate, DLL `CD41F0E1...6FBB2` from compiled source `433710E0...E62C`, is
also runtime rejected. Its diagnostics-enabled deployment is preserved only as
rejected evidence at
`dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`.
That run attached 1,632 markers with no data, texture, or ABI fault, but the Mod's
negative outer atlas offset changed the native parent extent from `3000` to
`3191.521`, triggering `WORLD_MAP_LAYERING_REBUILD_REQUIRED` oscillation: six
attaches and five detaches. These historical logs do not accept the current
candidate.

The current matrix is maintained in
`docs/RUNTIME_FEEDBACK_AUDIT_2_2_1.md`.

## Historical 2.2.0 acceptance state - preserved

- [x] Source identity is `2.2.0` and runtime label is
      `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_0` in every compiled,
      installer, metadata, and package authority.
- [x] Current 6AEFDACC replacement source review, core `2/2`, and all static
      source gates pass.
- [x] Clean native `/W4 /WX` build for the current replacement passes and records its source-bound
      `main.dll` SHA-256.
- [x] The packaged game-1.0.11 owner RVA and member offset `0x128` remain fast
      paths only. One FullActivation shares a total budget of at most 24 active-
      `.db` key validations across packaged and structural owner routes. If the
      packaged fast-path candidates fail, FullActivation scans the current
      executable at most once and accepts exactly one retained structural
      signature. That scan counts only targets inside the mapped image and scans
      executable sections through `min(SizeOfRawData, VirtualSize)`.
      Structurally incompatible updates fail
      closed; the source does not promise compatibility with every future
      version.
- [x] Replacement Setup reports exactly 20 passed, 0 failed, and 0 skipped for
      DLL `6AEFDACC...`.
- [x] Replacement manual-copy tests report exactly 2 passed, 0 failed, and 0
      skipped for DLL `6AEFDACC...`.
- [x] Replacement payload equivalence, copy validation, clean-target validation,
      and fresh re-extraction of all three archives pass for DLL `6AEFDACC...`.
- [x] A newly generated `dist/final-2.2.0` for DLL `6AEFDACC...` contains exactly the
      three expected ZIPs, `release-manifest.json`, and `SHA256SUMS.txt`.
- [x] Replacement DLL, Setup, and all three ZIP hashes are copied from the new
      `6AEFDACC...` manifest rather than the superseded `5210...` candidate.
- [x] The exact source-built replacement DLL is deployed to the owned game Mod
      directory and source/build/installed hashes match `6AEFDACC...`.
- [x] The replacement deployed test copy has `debug_logging=true`, exactly one enabled Radar
      entry in `mods.txt`, and a verified rollback backup.
- [ ] Capture live evidence for the current 6AEFDACC F6 presentation, localized-
      overlay, and compact-outline bytes. The healthy 84A360B0 log validates
      only the prior exact bytes and must not accept this current build.
- [ ] A live Setup-owned Install or Update produces and verifies
      `INSTALL-RECORD.txt`. The exact-DLL developer deployment used for gameplay
      testing is not Setup ownership evidence.
- [x] Public diagnostics default to `debug_logging=false`; no local debug file,
      log, backup, or runtime state enters an archive.
- [ ] F6 presents one responsive page with top-bar Bug Report and Close controls,
      read-only Mod Status text plus a thin state-colored strip, a separate
      action, centered Language, Marker Visibility, Height Indicators (Radar
      Only), and Filter Modes at fullscreen/windowed, DPI, 16:9, 16:10, and 21:9
      layouts. Translucent cards, equal-width filter choices, text alignment,
      and bounded non-overlapping hit regions remain correct.
- [x] Current source schedules a measured desired-size text-layout pass for exact
      font-layout TextBlocks after first F6 open, language changes, status
      changes, and language-popup display. `GetDesiredSize` must return the
      known `Vector2D` structure identity. Unavailable, faulting, invalid, or
      oversized results and the render-scale fallback preserve authored text
      geometry; no button hit box, compact-radar or expanded-map geometry, or
      per-frame path is modified.
- [ ] Exact current-candidate gameplay confirms every F6 text role is vertically centered
      after those four rebuild/display paths; static checks are not visual
      acceptance.
- [ ] F6 opens while Radar is Off, On, and Faulted; shows the matching read-only
      state; offers a separate Enable, Disable, or Retry action that keeps the
      page open; rejects Enable until a playable world is ready; and opens the
      fixed Nexus Posts URL from the top-bar Bug Report control.
- [ ] Treasure, Area Quest, and shared mini-game height default ON on a clean install; all
      three persist independently and a valid existing file keeps its choices.
- [ ] Treasure uses its unchanged category-colored six-piece full arrow to the
       left of the selected chest. Every visible Area Quest
       keeps the black frame with three white dots inside the selected source
       band's inclusive +/-500 margin. The marker becomes an up triangle below
       that band and a down triangle above it. For the multi-band row, authored
       marker Z must select the uniquely nearest existing source band; an exact-
       distance tie or missing profile stays neutral. Fly, Mole, and Wave share
       one nearest-mini-game channel. Its shaftless triangle remains centered
       directly below the selected icon, uses that marker's actual kind palette,
       adds a near-black contrast outline without changing size, position, or projection,
       points up when the target is more than 500 vertical units above the
       comparable player Z, points down when it is more than 500 below, and hides
       in the inclusive +/-500 band. All three compact height channels compare
       against `playerZ - 150`; Treasure retains its existing dead-zone behavior.
- [ ] All 83 map-100 Fly/Mole/Wave records use their exact trusted
      `MiniGame_<kind>_<id>_NPC_Start` height. Missing or ambiguous height hides
      only the shared mini-game triangle, not the marker.
- [ ] English, Japanese, Korean, Simplified Chinese, Traditional Chinese,
      French, German, Spanish (Spain), Russian, Thai, and Portuguese (Brazil)
      pass terminology, glyph, clipping, and persistence review.
- [ ] The selector shows only the 11 explicit languages. Seed a legacy AUTO
      preference and prove the next actual F6 opening or F7 activation resolves
      `DGameUserSettings.LanguageText`, Kismet, and English, persists one
      explicit language, and never displays AUTO/Use Game Language.
- [ ] F6 selects already-loaded Common/TC/JP/TH game Font objects by script;
      missing evidence falls back without changing language, guessing an asset
      path, replacing FontMaterial, or adding recurring work. Exact-size
      game/base paths are bounded and verified. If both fail, a fresh DTextBlock
      then base TextBlock leaves Font untouched and uses the bounded viewport/DPI
      render scale with a justification-aware pivot; `target_size=0` records skip
      both post-prepass Font.Size loops. Core UMG creation, `SetText`, tree
       insertion, and viewport attachment remain fail closed, and language does
       not change because of font evidence. Disable or replace the external
       `DS_HYFont_P.pak` for glyph QA; do not route its raw FontFace as a UFont.
       Verify the canonical `assets/ui/f6` payload contains the six expected
       ko/zh-Hant status-specific main overlays, shared `language-popup.tga`, and
       `manifest.json`. Each main overlay must replace all 30 fixed main-panel
       text slots and the popup must replace only the two affected language
       names; the other nine languages must continue through native game fonts.
       The overlays must use pinned DroidSansFallback at base size 32, a one-
       pixel translucent stroke, role-specific optical baselines, and the final
       top-bar/status/filter coordinates. Live review must still confirm size,
       weight, and alignment.
- [x] The localized-overlay gate audits all 11 runtime blocks and `123/123`
      overlay codepoints; reports minimum fit `1.000`, maximum optical-center
      error `0.5` raster pixel, minimum ink height `0.500`, and no edge alpha or
      slot overflow; and reproduces all seven 1360-by-1320 RLE TGAs plus the
      manifest byte-for-byte (`8/8`). Manifest SHA-256:
      `F8458AADFBFE2EB250222B3427122047C4745F619B9B47452C88508ECC9D3044`.
      This is static asset evidence only.
- [ ] Historical 2.2.0 expanded-map hosts use the restored atlas-local layout: each outer native
      Canvas slot is `{atlas_left,atlas_top,atlas_width,atlas_height}`, and each
      `Panel_Point` Image is local `{0,0,atlas_width,atlas_height}`. The rejected
      full-parent outer-host layout is absent. The correction adds no parent-
      size post-check, growth rejection, extent-change token, or other new
      parent-size assumption; later layout transitions use the existing
      witnessed stable-geometry and bounded map/zoom reconstruction strategy.
      A same weak parent identity must return `Unchanged` without Remove/Add,
      reparenting, or an extra atlas render; only a real weak parent replacement
       may move the hosts. Capacity is 4,096 against an accepted maximum of 2,500
       Treasures plus 279 fixed non-Treasure rows (2,779 total), leaving 1,317
       spare slots. The observed 1,632 total/1,501 Treasure snapshot was below
       the old 1,785 limit, so capacity is not accepted as the flicker cause.
       The current log must retain its single-attach/no-repeated-detach-rebuild
       behavior. Live acceptance must prove alignment and dense-Treasure
       stability without introducing a `3000`/`8000` geometry substitute.
       Confirm the current F6/localization/compact-indicator closeout changes no
       expanded-map source, atlas, capacity, coordinate, projection, ownership,
       or style-revision contract.
- [ ] World-map glyph style revision 50 uses two 3072 atlases, 50 percent more
       linear raster density, and about 72 MiB raw decoded BGRA memory versus
       about 32 MiB at 2048. Marker coordinates, projection, zoom handling,
       native parent ownership, and outer/inner container geometry remain
       unchanged. Exact-artifact visual quality remains `NOT_VALIDATED` until
       inspected in game.
- [ ] Physical-controller world-map and pause-menu suppression works without a
      hardware cursor or controller-mapping read.
- [ ] Diagnostics-disabled same-session frame-time evidence shows no new scan,
      SQL, filesystem access, PAK extraction, timer, or unbounded retry.

Static and deterministic verification cannot replace live visual acceptance of
expanded-map alignment, revision 50 world-map glyph styling, or every-language
F6 font and glyph layout.

Status: `SOURCE_VALIDATED = PASSED_FOR_REPLACEMENT_2_2_0`

Status: `BUILT = PASSED_6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`

Status: `INSTALLER_TESTED = PASSED_6AEFDACC_SETUP_20_OF_20_MANUAL_2_OF_2`

Status: `PACKAGED = PASSED_6AEFDACC_THREE_ARCHIVES_BYTE_IDENTICAL`

Status: `DEPLOYED = PASSED_LOCAL_DEBUG_6AEFDACC_NOT_SETUP_OWNED`

Status: `GAMEPLAY_ACCEPTED = NOT_VALIDATED_FOR_2_2_0`

Status: `CONTROLLER_ACCEPTED = NOT_VALIDATED_FOR_2_2_0`

Status: `RESPONSIVE_UI_ACCEPTED = NOT_VALIDATED_FOR_2_2_0`

Status: `LOCALIZATION_GLYPHS_ACCEPTED = NOT_VALIDATED_FOR_2_2_0`

Status: `PERFORMANCE_ACCEPTED = NOT_VALIDATED_FOR_2_2_0`

The build, package, installer-test, and local developer-deployment statuses above
describe exact DLL `6AEFDACC...`. `Build-Release.ps1` reports package validation
passed, Setup `20/20`, Manual `2/2`, payload equivalence, manual layout, and
clean-target policy validation passed, and three public ZIPs re-extracted
byte-identically. Source, build, and installed hashes match, and diagnostics
remain enabled in the local developer deployment. Its rollback backup is
`dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
Rollback backup
`dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`
belongs to the superseded intermediate 59529B2A deployment and is not current
candidate evidence. This local deployment is not Setup ownership; the
earlier 634D283A deployment is historical only.
No status above describes gameplay, F6 visual presentation, localized-overlay,
controller, exit, or performance behavior as runtime accepted.

The historical 2.2.0 matrix is maintained in
`docs/RUNTIME_FEEDBACK_AUDIT_2_2_0.md`.

## Historical 2.1.1 evidence - preserved

The current 2.1.1 source passed the core gate, all four static source gates, the
clean native `/WX` build, Setup `20/20`, manual-copy `2/2`, payload equivalence,
and three-archive re-extraction. The packaged `main.dll` SHA-256 is
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
It is sealed in the authoritative `dist/final-2.1.1` package and deployed as an
exact diagnostics-enabled local test copy, but has not been gameplay-validated.
The 2.1.0 `D4EE...`, `4AFE...`, and
`BDE21...` artifacts are historical and non-authoritative for 2.1.1.
Installation, gameplay, performance, and publication remain independent.

The 2.1.1 runtime feedback matrix is maintained in
`docs/RUNTIME_FEEDBACK_AUDIT_2_1_1.md`. Its single-height Area Quest cases are
historical artifact evidence and are superseded by the 2.2.0 height-band gate;
controller-opened map/pause suppression, arrow presentation, wheel zoom, and
windowed/21:9/16:10 alignment remain owner-runtime acceptance items even after
their automated gates pass.

### 2.1.1 release identity

- [x] Version is `2.1.1` in source, DLL marker, metadata, Setup, and all three
      archives.
- [x] Runtime label is
      `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_1_1` everywhere.
- [x] Public diagnostics default to `debug_logging=false`; exact legacy
      `event_log_enabled=true|false` files remain accepted during updates.
- [x] The only public runtime layout is ExperimentalNested.

### 2.1.1 compatibility and installer behavior

- [x] The selected game path is the exact DragonSword shipping executable and
      passes bounded executable AMD64 PE32+ validation.
- [x] A structurally valid changed game hash is accepted; the observed hash is
      used only for transaction identity and install-record provenance.
- [x] Existing ExperimentalNested `UE4SS.dll` and root `dwmapi.dll` pass
      bounded AMD64 PE32+ DLL validation and the required settings, Mods, and
      controlling `mods.txt` structure exists.
- [x] Structurally valid changed loader/proxy hashes are accepted and those
      existing bytes are not overwritten during Update / Repair.
- [x] Embedded bootstrap/conversion resources and the Radar payload retain
      exact SHA-256 integrity validation.
- [x] Missing UE4SS selects Bootstrap Install.
- [x] Root, dual, incomplete, or malformed UE4SS selects confirmed Conversion
      Install with a complete verified retained backup.
- [x] Compatible UE4SS plus no owned Radar selects Install.
- [x] Compatible UE4SS plus a strictly owned Radar, including an older owned
      version, selects Update / Repair.
- [x] A compatible game or UE4SS update never prompts for a rebuild based only
      on a changed hash.

Status: `HISTORICAL_2_1_1_INSTALLER_COMPATIBILITY = PASSED_20_CASE_MATRIX`

### 2.1.1 ownership and user state

- [x] Existing-target ownership requires top-level schema-5 release metadata,
      schema-1 package metadata, a bounded unique manifest, exact immutable
      path/size/hash identity, a runtime-label-bearing `dlls/main.dll`, and an
      exact recursive tree limited to the declared live-state allowlist.
- [x] Unknown same-name ownership, unsafe paths, reparse points, duplicate
      entries, overflows, or immutable mismatches fail before mutation.
- [x] `config/visibility.ini` is preserved byte-for-byte after validation.
- [x] Current-source gate: the 4 KiB startup-only visibility parser accepts the
      complete named `[radar]`, `[map]`, and `[modes]` format plus strict legacy
      schema 1-4 documents, fails malformed input to safe defaults, performs no
      hot polling, and atomically upgrades format only on a real F6 change.
- [x] `config/diagnostics.ini` is preserved byte-for-byte after validation.
- [x] `data/defaults/treasure_overrides.txt` is preserved byte-for-byte after
      bounded UTF-8 and unique positive `ignore <save-id>` validation.
- [x] Clean install creates the three live user files from embedded defaults.
- [x] No `.example.ini` or `enabled.txt` file is installed.
- [x] Update / Repair refreshes the Radar DLL, immutable generated catalogs,
      release metadata, and mapping file without regenerating PAK data locally.

Status: `HISTORICAL_2_1_1_USER_STATE = PASSED_SETUP_AND_MANUAL_MATRICES`

### 2.1.1 load control and transaction safety

- [x] Setup performs all required stopped-game checks and never launches or
      terminates the game.
- [x] The selected ExperimentalNested `mods.txt` is the sole load authority.
- [x] Exactly one `DragonSwordNativeWorldRadarPostRender : 1` entry remains.
- [x] Every valid `DragonSwordWorldRadarObjectState` entry is removed without
      writing a redundant disabled entry.
- [x] Unrelated `mods.txt` bytes, encoding, BOM, comments, and line endings are
      preserved.
- [x] Active or malformed `DragonSwordWorldRadar` load control rejects with
      zero mutation; exact disabled `DragonSwordWorldRadar : 0` is preserved.
- [x] Normal Install and Update / Repair use a temporary rollback journal and
      leave no persistent backup after a verified commit.
- [x] Bootstrap/Conversion retains one complete verified original-layout
      backup.
- [x] Every injected failure restores all recorded mutations byte-exactly.

Status: `HISTORICAL_2_1_1_TRANSACTION_SAFETY = PASSED_20_CASE_MATRIX`

### 2.1.1 static and installer gates

- [x] Native and installer sources compile through their checked-in toolchains.
- [x] The clean native `/WX` build passes and records `main.dll` SHA-256
      `B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
- [x] Native lifecycle, UObject retention, bounded-work, fail-closed, renderer,
      and performance static gates pass.
- [x] The historical 2.1.1 Setup is sealed as an unsigned .NET Framework 4.8 x64
      WinForms executable with an
      administrator manifest.
- [x] The historical 2.1.1 isolated installer runner reports exactly:

```text
expected=20
passed=20
failed=0
skipped=0
release_gate=PASSED
sources_unchanged=true
fixtures_cleaned=true
```

- [x] The recorded evidence identifies the exact historical 2.1.1 Setup SHA-256 as
      `2A5E1B7FBB08569E2E61D81EA9542A5CD12F7D8DE3D4A6FFBBCA529DE84F98A7`
      in `dist/final-2.1.1/release-manifest.json`.
- [x] The historical 2.1.1 isolated manual runner reports exactly 2 passed, 0 failed, and 0
      skipped for the No-UE4SS and With-UE4SS ExperimentalNested archives.

Status: `HISTORICAL_2_1_1_SOURCE_BUILD_INSTALLER_MANUAL = PASSED_CORE_PLUS_4_STATIC_NATIVE_WX_PLUS_20_PLUS_2`.

### 2.1.1 package

- [x] Staging begins from an empty project-owned non-reparse directory.
- [x] The installer archive is exactly
      `DragonSwordNativeWorldRadarPostRender-v2.1.1-Installer.zip`.
- [x] It contains exactly:
  - [x] `DragonSwordNativeWorldRadarPostRender-Setup-2.1.1.exe`
  - [x] `DragonSwordNativeWorldRadarPostRender-Setup-2.1.1.exe.sha256`
  - [x] `INSTALL.md`
  - [x] `THIRD_PARTY_NOTICES.txt`
- [x] The sidecar matches the exact Setup bytes.
- [x] The two manual archives are exactly
      `DragonSwordNativeWorldRadarPostRender-v2.1.1-Manual-No-UE4SS.zip` and
      `DragonSwordNativeWorldRadarPostRender-v2.1.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`.
- [x] Both manual archives contain the same source-bound Mod payload and one
      clean single-product `ue4ss/Mods/mods.txt`; only With-UE4SS contains the
      pinned verified ExperimentalNested runtime. Both are script-free and map
      directly to `DS/Binaries/Win64`. The No-UE4SS instructions prove both the
      missing-file copy route and the existing-file manual-merge route without
      overwriting unrelated entries. Neither contains StableRoot.
- [x] Fresh extraction of all three ZIPs matches staging byte-for-byte.
- [x] `dist/final-2.1.1` contains exactly those three ZIPs,
      `release-manifest.json`, and `SHA256SUMS.txt`.
- [x] No source, log, backup, runtime state, unrelated Mod, StableRoot payload,
      or local debug override is included.

Status: `HISTORICAL_2_1_1_PACKAGED = PASSED_B89F_THREE_ARCHIVES`; existing D4EE/4AFE/BDE21
outputs are historical and non-authoritative.

### 2.1.1 deployment

- [x] Deployment is explicitly authorized and the game is closed.
- [x] The selected deployment path and transaction are recorded.
- [x] The post-install release identity, native DLL, and load-control state
      verify.
- [x] Public diagnostics differ only through a separately recorded installed
      testing override.
- [x] The existing installed `treasure_overrides.txt` remained byte-identical
      at SHA-256 `CD52EE5C006B99BBF32ACDFE3ADD301507FDFD6C3DA7249BA6C3B6E6DB00BA43`.
- [x] Installed and built `main.dll` both hash to
      `B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`;
      exactly one native load-control line is enabled and no predecessor line
      remains.

Status: `HISTORICAL_2_1_1_DEPLOYED = PASSED_LOCAL_DEBUG_EXACT_B89F`

### 2.1.1 gameplay and performance

- [ ] Cold launch, F7 activation, F8 disable, and F8/F7 resynchronization work.
- [ ] F6 visibility settings apply and persist independently for radar/map;
      bird eggs have one independent RADAR toggle and an unavailable MAP cell.
- [ ] F6 `AREA QUEST MODE` persists independently: `AVAILABLE` retains strict
      prerequisite filtering, while `ALL` shows unfinished catalog tasks and
      still hides saved or exact runtime completions on both maps.
- [ ] F6 `ASSAULT MODE` persists independently: `AVAILABLE` retains the authored
      Assault state, time, and future 120-minute cooldown filters, while `ALL`
      shows all 40 static Assault records regardless of defeat/cooldown state.
      Switching back to `AVAILABLE` restores the live filters immediately.
      Boss and area-quest visibility remain unchanged in both modes.
- [ ] Compact treasures, Bosses, Assaults, mini-games, area tasks, bird eggs,
      clock, and height pointers render and move correctly. Treasure and area-
      quest height channels must remain independent and simultaneously visible
      when both targets qualify. Treasure uses its colored full shafted pointer;
      Area Quest reshapes its original marker in place as a closed black
      shaftless triangle and never moves to a Treasure-style side position.
- [ ] Treat the historical 2.1.1 single-height task checks as superseded. The
       retained 2.2.x gate uses 144 height profiles, one genuine two-band profile,
       and three no-source rows. Authored marker Z selects only the uniquely
       nearest existing source band; it never becomes height fallback data, and
       an exact-distance tie or missing profile remains neutral.
- [ ] Open and close the world map and pause menu with a controller while no
      mouse cursor is visible. Compact icons hide immediately and restore after
      gameplay resumes, without duplicated hosts or requiring an input mapping.
- [ ] During one F7 activation, switch fullscreen to windowed and back (including
      a DPI-changing resolution) and verify compact markers and clock reflow on
      the next one-hertz sample without F8/F7, duplication, or accumulating
      layout work.
- [ ] At native 21:9, a 4K viewport with internal 21:9 black bars, 16:10, and
      windowed 16:9, open, pan, zoom, close, and reopen the expanded map. Every
      category must stay aligned through direct native-parent pan, zoom,
      clipping, visibility, and RetainerBox inheritance. A deterministic
      3840x1600 unit input is not runtime acceptance. Initial attach may use only
      its bounded three-attempt readiness service. A later trigger must produce
      only the five deadlines at 100/250/500/1,000/1,250 ms, one observation per
      due pass, and no overdue multi-observation collapse. Both hosts must remain
      hit-test-invisible native-Canvas children whose outer slots are full
      stretch with zero offsets, `AutoSize=false`, zero alignment, and maximum
      Z. Each inner `Panel_Point` must also remain full stretch with zero offsets.
      Only each Image Canvas slot may hold
      `{atlas_left,atlas_top,atlas_width,atlas_height}`, while Image render
      translation stays `(0,0)`. Same-parent pan/zoom must perform no viewport-
      transform write. Every tail pass must read live parent extent, and the
      outer/inner full-stretch layout must not change that extent. A fully
      unchanged same-parent pass, including the final pass, must perform no
      layout, transform, visibility, restack, Remove/Add, or `RequestRender`.
      Post-attach tail deadlines must be
      armed from a fresh clock sample taken after attachment completes, and
      attachment itself must not submit an empty-host `RequestRender` before
      visibility is applied. Retained refresh must not read
      `PlayerIconWidget`; anchor-only changes must leave the attach-time Image
      Canvas-slot atlas position immutable. A real parent replacement must only
      report `RebuildRequired`; the scheduler alone may perform a fresh
      attachment. A changed extent must be observed as
      two matching stable samples before `RebuildRequired` is reported. Merely
      reporting that result must not collapse, hide, detach, or mark the valid
      payload transform-unready; the scheduler alone owns the accepted rebuild
      mutation. Retained-RetainerBox or owned-payload replacement/invalidity
      follows the same report-only scheduling boundary. At most one rebuild may
      run in one open-map session; it must preserve the marker snapshot and
      receive its own hard-capped three-attempt attach/geometry budget, for a
      whole-session maximum of initial 3 plus rebuild 3. Require exactly one
      attach and zero detaches in a stable open-map session, with no
      `WORLD_MAP_LAYERING_REBUILD_REQUIRED`. Confirm no forced layout prepass,
      reraster, marker-data rebuild or reprojection, native desired-size growth,
      or native click-target displacement. No sampled UObject wrapper or
      `FGeometry` may cross passes; no centered/desktop fallback, `3000`/`8000`
      geometry constant, or steady poll is allowed.
- [ ] Confirm the two 2048 atlases consume approximately 32 MiB raw decoded
      BGRA. A cache hit must use envelope `DSNWRA52` and pass exact dimensions,
      header, magic, 1/4096-UMG-unit fingerprint, visible-count, full RLE decode
      to exactly 2048-by-2048 pixels, encoded-payload checksum, and exact-EOF
      checks. Revision-51, corrupt, truncated, and trailing-byte files must miss.
      Confirm writes use a same-directory temporary file, atomically publish via
      `MoveFileExW` with replace-existing and write-through flags, and remove the
      temporary file on failure. Record cold and cache-hit attach time; bounded
      cache I/O and texture import are not steady-state work and are not accepted
      merely because the cache reports a hit.
- [ ] Both exact bird-egg classes use only the fixed 512-slot weak pool, at most
      eight unknown-position queries per 250 ms control tick, and a nearest-16
      active bound. They never render on the expanded map or invoke enumeration
      or SQL.
- [ ] Expanded map categories render through first open, close/reopen, pan,
      zoom, maximum zoom, minimize/restore, and state-delta rebuild.
- [ ] Clustered and vertically overlapping treasures all clear correctly.
- [ ] An uncollected underwater mount-only chest emits one exact current-Rider
      interaction and clears immediately without F8/F7. The log identifies
      `local_mounted_rider_net_multi_execute_interact_prop`; an unrelated or
      unverified non-Pawn event remains visible and may use at most two exact-
      category positive-only requests. There is no third request, unrelated
      removal, poll, or full treasure query.
- [ ] Boss and Assault live departure does not clear them; real defeat does,
      without requiring F8/F7.
- [ ] Immediate, cooking/delivery, time-gated, and repeatable area tasks update
      without requiring F8/F7. The cooking/delivery path emits
      `progress_precondition=not_required`, treats the dynamic event as arming
      rather than completion, and hides through exact `END` or a positive
      baseline-relative exact-ID save result. F7 must establish the exact
      per-ID `COMPLETE_CNT` baseline; missing means zero only for a valid
      single-owner query, while an unknown baseline queues no SQL. The fallback
      accepts only strict count growth, runs immediately, and may retry twice at
      15-second intervals; no fourth attempt or periodic SQL is allowed. After
      exhaustion, repeated same-generation events remain locked until new F7
      or current `NONE`/`END` followed by later `ACCEPTABLE`/`PROGRESS`.
- [ ] Dungeon/travel/open-world return restores the radar without stale
      objects, repeated recovery, or a crash.
- [ ] Long-session owner observation and frame-time capture show no cumulative
      work, unbounded retry, unexpected auto-disable, or new stutter source.
- [ ] A diagnostic-enabled restart reports `config_debug_logging=true` and
      `log_schema=2` plus monotonic `seq`, `utc_ms`, and `elapsed_ms`. Ordinary
      records enter the fixed 256-record non-blocking queue, writer batches
      contain no more than 16 records, idle partial batches flush within 250 ms,
      and `ENGINE_TICK_PROFILE` reports zero or explained `logger_dropped` and
      `logger_truncated` counts. The current file stays at or below 1 MiB, and
      exactly one previous file is retained. A public-default
      `debug_logging=false` restart must return before formatting, locking,
      directory creation, rotation, or file I/O.

On 2026-08-31 the owner accepted the prior 2.1.0 gameplay build. That historical
decision does not accept the 2.1.1 `B89F...` artifact or its new controller and
Area Quest paths.

Status: `GAMEPLAY_ACCEPTED = NOT_VALIDATED_FOR_2_1_1`

Status: `INSTALLED_ARTIFACT_HASH = NOT_RECORDED_FOR_2_1_1`

## Publication

The retained SG-03 3.0.0 checkpoint has three generated, verified local packages
and a verified historical local deployment. SG-05 and SG-08 were subsequently
deployed; SG-06/SG-07 retain separate local build evidence. SG-09 local
deployment and three replacement packages are independently verified.
Owner gameplay approval and
Nexus publication remain separate; no upload/publication record has been created. The historical
workspace-source authorization below does not clear the distinct unresolved
binary/derived-data publication boundaries; the current release manifest
retains `BLOCKED_PENDING_RIGHTS_AND_SOURCE_PROVENANCE_REVIEW`.

- [ ] Unreal Engine/UEPseudo authorization and license compatibility cleared.
- [ ] Exact `e_sqlcipher.dll` source/build provenance cleared.
- [ ] Generated catalog and derived coordinate redistribution rights cleared.
- [x] User explicitly authorizes publication/upload of the workspace source.

Status: `SOURCE_PUBLICATION_AUTHORIZED = PASSED`

Status: `BINARY_AND_DERIVED_DATA_PUBLICATION = BLOCKED`
