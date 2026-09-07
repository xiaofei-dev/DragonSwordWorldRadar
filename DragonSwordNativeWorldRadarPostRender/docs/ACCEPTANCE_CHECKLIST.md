# DragonSword Native World Radar 2.2.1 Acceptance Checklist

Mark an item only from evidence for the exact recorded bytes. Source, build,
package, installation, gameplay, performance, and publication are independent.

## Current 2.2.1 acceptance state

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

- [ ] Unreal Engine/UEPseudo authorization and license compatibility cleared.
- [ ] Exact `e_sqlcipher.dll` source/build provenance cleared.
- [ ] Generated catalog and derived coordinate redistribution rights cleared.
- [x] User explicitly authorizes publication/upload of the workspace source.

Status: `SOURCE_PUBLICATION_AUTHORIZED = PASSED`

Status: `BINARY_AND_DERIVED_DATA_PUBLICATION = BLOCKED`
