# Changelog

## 2.2.1 (Unreleased)

Version 2.2.1 is a fixes-only world-map stability candidate. It adds no marker,
catalog, capacity, F6, localization, height, save, or installer feature.

- Keep one explicit native owner: both Mod-owned, hit-test-invisible outer hosts
  attach to the current `DLayerMap.FogAbovePanel`, resolved directly from the
  current layer. `ArrayIconInfo` is consulted only while creating a missing
  host, solely to obtain one currently instantiable native icon class; it does
  not select the parent, and retained-host validation and refresh do not scan
  the array.
- Keep each outer `FogAbovePanel` Canvas slot full stretch, with anchors `(0,0)`
  to `(1,1)`, zero offsets, `AutoSize=false`, alignment `(0,0)`, and maximum
  Canvas Z. The game-owned parent therefore sees no negative atlas-bound child
  capable of expanding its desired extent.
- At each fresh attachment, including a scheduler-accepted bounded rebuild,
  force the cloned host's inner `Panel_Point` Canvas slot to full stretch with
  zero offsets, `AutoSize=false`, and alignment `(0,0)`. Only that fresh
  attachment reads `PlayerIconWidget` and places each atlas Image inside that
  Mod-owned Canvas at `{atlas_left,atlas_top,atlas_width,atlas_height}` with
  render translation `(0,0)`. The complete hierarchy is `FogAbovePanel -> full-
  stretch outer host -> full-stretch Panel_Point -> atlas-rectangle Image`; no
  host render transform or forced layout prepass is part of the candidate.
- Inherit pan, zoom, clipping, map visibility, and RetainerBox composition
  directly from `FogAbovePanel`. Same-parent pan and zoom require no viewport-
  transform synchronization. Every retained event-tail pass re-resolves the
  exact `FogAbovePanel` and reads only its live local extent. While parent
  identity and extent remain unchanged, the attach-time Image placement is
  immutable and refresh performs no layout, transform, widget-tree, or
  RetainerBox write. A parent replacement reports `RebuildRequired`; refresh
  never reparents or delta-rebases retained content. A stable same-parent extent
  change reports the same result only after two matching successful samples,
  and only the scheduler may perform one fresh bounded attachment.
- Record the topology evidence for the rejected heuristic. Native zoom rebuilt
  icon widgets and changed the first valid `ArrayIconInfo` parent between
  `FogAbovePanel` and `FogUnderPanel`. The Mod followed those transient icon
  parents, producing fog-layer occlusion plus four host reattachments in one
  zoom sequence, which explains the observed disappearance, hitching, and
  flashing. The temporary topology diagnostics are development evidence only;
  they are not gameplay acceptance for the corrected bytes.
- Require two matching successful changed-extent samples before reporting
  `RebuildRequired` for a same-parent Canvas extent change. A rebuild report is
  observation-only: refresh does not pre-collapse a valid payload or clear its
  transform readiness, and the bounded main scheduler owns an accepted detach
  and rebuild. Arm the event tail from a fresh post-attach clock, and remove the
  empty-state end-of-attach `RequestRender`; guarded visibility owns the first
  repaint. Retained-RetainerBox or Mod-owned-payload replacement still reports
  `RebuildRequired`. Main may service it at most once per open-map session while
  preserving the marker snapshot and giving the rebuild its own hard-capped
  three-attempt attach/geometry budget. One open session remains bounded to at
  most three initial attempts plus three rebuild attempts.
- Reduce the two event-built atlases from 3072 to 2048, approximately 32 MiB
  total when decoded as BGRA. Cache envelope `DSNWRA52` fingerprints UMG values
  at 1/4096 logical-unit precision and stores the visible count plus encoded-
  payload checksum. A hit must decode exactly the full atlas pixel count, match
  the checksum, and end exactly at EOF. Old or corrupt files are misses. Misses
  write a same-directory temporary file and publish it atomically with the
  Win32 replace-existing and write-through flags; cache I/O and texture import
  remain bounded attach work, not steady-state work.
- Record the independent viewport plus extreme-Z deployed build as runtime
  rejected: it made markers visible, but live testing found severe lag, wrong
  placement, and delayed updates. The later direct-`FogAbovePanel` full-stretch-
  outer/Image-translation candidate is also runtime rejected. Quantitative
  2026-09-05 screenshots show that a zoom-in scaled the base map by about 1.214
  and Radar by about 1.218 while leaving a relative translation of about
  `(+113,-190)` pixels (32-point mean residual 0.28 px); the reverse zoom scaled
  them by about 0.760 and 0.758 but left about `(+67,+200)` pixels (35-point mean
  residual 0.82 px). The near-equal scale and sign-reversing vertical offset
  identify inconsistent local origins/zoom pivots, not a scale-formula error or
  cumulative frame drift. Its exact DLL
  `CCC6B1170BAD1BF94AE5149DE52B48E2E11553747016E10299423BFD0C06AE00`,
  compiled source
  `B650B5FBD731EC0BC24D43F2256A1826E403D164353DFC5B684AC943FAD174EA`,
  and deployment backup
  `dist/work/deployment/deploy-backups/20260905-092836-495-native-only-deploy`
  remain rejected-candidate evidence only. The subsequent outer-atlas-rectangle
  DLL
  `CD41F0E1FD04AE3E06AA3EA0163EAE0019A19E6B3B7B0B9A110A907B06E6FBB2`,
  from compiled source
  `433710E06412A5BEB4F225CB7B3658024C5974AC5B26CDD94BDB09D55ED2E62C`,
  is also runtime rejected. It attached 1,632 markers successfully and recorded
  no marker-data, texture, or ABI fault, but its negative atlas-left outer slot
  expanded the same native parent from `3000x3000` to `3191.521x3000`. That
  self-authored geometry change produced six attaches and five detaches, causing
  the flashing/blank-map regression. Its rollback backup remains
  `dist/work/deployment/deploy-backups/20260905-182946-652-native-only-deploy`.
  The next dynamically rebased inner-atlas DLL
  `5C632820CC44065AB9FEDDA260A392B5C72655D3815F445C73CF4A50A72DD606`, from
  compiled source
  `798297BBF7F9791A9E2FFBFF4EC5894A62CC4B16B825CB6BAEBC19A398CB2585`, is also
  runtime rejected. Six same-parent `PlayerIconWidget` anchor observations
  rewrote its Image placement and account for approximately `(+92,-915)` pixels
  of screen displacement. Its deployment backup
  `dist/work/deployment/deploy-backups/20260905-191717-084-native-only-deploy`
  remains rejected-byte evidence only. The immutable candidate matrix and
  prohibition list are in `docs/WORLD_MAP_ATTEMPT_LEDGER.md`. All 2.2.0 hashes,
  tests, packages, and deployment records likewise remain evidence only for
  their exact bytes.
- Keep exact-artifact gameplay, native-icon stability, click-target alignment,
  pan/zoom, dense-Treasure, 4K/21:9/16:10/DPI/windowed behavior, clean exit,
  memory, attach-time, and frame-time acceptance `NOT_VALIDATED` until the
  immutable-placement candidate is gameplay-tested. The first immutable-
  placement revision passed source review, static gates, release hygiene, Core
  `2/2`, and a local native build at DLL
  `A5CEBAECAD75E8AE5C8BC2435625CD52BEE878AA656C1D8EA1C58C5C24542EDB`
  from compiled source
  `72BD98D3AA612D4F902D901B4174C4340BE325A54A2015E8B88DEACE738862A1`,
  size 1,107,968 bytes. It was superseded before deployment by a follow-up source
  change that removed visibility reconciliation from retained refresh. A5CE is
  build evidence only, not the final candidate. Current WM-06 source review,
  static gates, release hygiene, Core `2/2`, native build, and rollback-backed
  diagnostics-enabled developer deployment pass for DLL
  `6435E10031D90840BF0499664CF57347D7991C9C192BD3B2239ADE2324C723A1`
  from compiled source
  `0A1A4CE3EE9F3A04E4B258976CFD830654BCB778B1F9BF5715FB66242B5E5BC5`,
  size 1,107,968 bytes. Installed identity matches exactly, `mods.txt` contains
  one Radar entry, `debug_logging=true`, and rollback backup
  `dist/work/deployment/deploy-backups/20260905-202742-614-native-only-deploy`
  is retained. Package and installer validation pass for those exact bytes:
  Setup `20/20`, Manual `2/2`, payload equivalence, layout, clean-target, and
  byte-identical re-extraction of all three ZIPs. Gameplay, visual, performance,
  and resolution evidence remain `NOT_VALIDATED`; public binary and derived-data
  publication remains blocked. Evidence for every superseded candidate remains
  bound to those exact bytes.

## 2.2.0 (Historical release)

Version 2.2.0 is a feature release for compact-radar height guidance,
localization, and the F6 settings experience. It preserves the complete 2.1.1
history below; the 2.1.1 `B89F...` DLL and package hashes are historical only
and are not 2.2.0 evidence.

- Restore save-backed Radar functionality for the September 2026 game update
  identified locally by Steam build `25076183`, executable length
  `162606488`, and SHA-256
  `B3E0B8CAB6752ACB981E104CA95A0105F76FCDD42EE622A8063AB8DE44FCA94C`.
  The executable owner pointer
  remains uniquely resolvable at RVA `0x94F4FA8`, while the live SQLCipher-key
  `FString` moved from owner offset `0x120` to `0x128`. The packaged owner RVA
  and verified current `0x128` member are zero-scan fast paths, not fixed
  compatibility requirements. Reconciliation tries the cached field, legacy
  `0x120`, verified current `0x128`, and only then a bounded aligned fallback.
  Every candidate must authenticate the active `.db` by reading its schema
  before it may be cached; an older `.bak` cannot select a stale key. If all
  candidates from the packaged owner fail that active-database check, one full
  activation may scan the current executable once for exactly one retained
  structural signature. The scan counts only targets inside the mapped image
  and scans executable sections through `min(SizeOfRawData, VirtualSize)`.
  Packaged and structural owner routes share one total budget of at most 24
  active-`.db` key validations. Zero or multiple signature matches, or a second
  failure to authenticate, fail closed. This supports only structurally
  compatible game updates and is not a promise of compatibility with every
  future version. The work remains F7-triggered on the existing below-normal
  worker, logs only route/offset/count/timing metadata, and adds no steady-state
  or per-frame work. A failed full reconciliation is exposed as Fault so F6
  Retry or an explicit F7 performs one fresh bounded activation.
- Add a shared compact-radar mini-game height indicator. Exact trusted
  `NPC_Start` heights cover all 83 map-100 rows: 33 Fly, 40 Mole, and 10 Wave.
  A missing or ambiguous height hides only the mini-game triangle and never hides
  the marker. The persisted configuration key remains `mole` for compatibility.
- Keep the three height controls visually and operationally independent and
  enable all three on a clean install. Treasure uses the existing
  category-colored full arrow. Every visible Area Quest evaluates a generated
  height profile. A single-band profile is used directly. For a multi-band
  profile, the authored marker Z selects the uniquely nearest existing source
  band; marker Z is selection evidence only and never becomes a synthetic
  height. An exact-distance tie or missing profile remains neutral. The normal
  black frame keeps three white dots inside the selected source band's inclusive
  +/-500 margin; below it points up and above it points down. There is no
  separate Area Quest arrow. The nearest
  Fly, Mole, or Wave uses a shaftless triangle centered below its own icon and
  colored from its actual kind, with a near-black outline for contrast. This
  outline changes neither size nor position nor projection. Target Z above the comparable player Z points
  up, target Z below points down, and the inclusive +/-500 band hides it. Every
  compact height channel compares against the same calibrated
  `playerZ - 150`; Treasure retains its existing category-specific geometry and
  dead-zone behavior. All three controls are compact-radar only, and valid
  existing choices are preserved.
- Replace the superseded single-height Area Quest catalog contract with bounded
  height profiles: 147 marker rows contain 144 profiles, one genuine two-band
  profile, and three no-source rows. Move_Check-only trigger bands are excluded
  when a real task-actor band exists. The runtime uses authored marker Z only to
  choose the uniquely nearest source band; it never falls back to marker Z as a
  height source, and a missing profile or exact-distance tie remains neutral.
- Extend the persisted F6 preferences with independent Treasure, Area Quest,
  and shared mini-game height-indicator switches. The compatibility storage key
  remains `mole`; existing visibility and mode settings remain independent.
- Add UI text for all 11 currently supported game languages: English,
  Japanese, Korean, Simplified Chinese, Traditional Chinese, French, German,
  Spanish (Spain), Russian, Thai, and Portuguese (Brazil).
- Add a centered language dropdown containing only the 11 explicit languages;
  AUTO/Use Game Language is no longer displayed. A legacy AUTO preference is
  migrated on the next actual F6 opening or F7 activation by resolving
  `DGameUserSettings.LanguageText`, then Kismet and English, and persisting the
  matching explicit language. An explicit persisted selection remains
  authoritative.
- Select already-loaded game Font objects by script: Common for Korean and
  Latin/Cyrillic UI languages, TC for both Chinese choices, JP for Japanese,
  and TH for Thai. Missing loaded-font evidence falls back safely without
  changing the language, guessing an asset path, or replacing FontMaterial;
  exact-artifact glyph acceptance remains pending. Missing or expired weak
  identities are retried only on a real F6 open. The external `DS_HYFont_P.pak`
  overrides Common/TC without complete glyph coverage and must be disabled or
  replaced for localization QA; raw Pretendard FontFace assets are not routed
  as `UFont` objects.
- Regenerate the fixed Korean and Traditional Chinese 2x text overlays from the
  pinned DroidSansFallback source at base size 32, with a one-pixel translucent
  foreground stroke and role-specific optical baselines. The canonical
  `assets/ui/f6` payload contains six status-specific main overlays, one shared
  language-popup overlay, and its manifest. Each ko/zh-Hant main overlay replaces
  all 30 fixed main-panel text slots; the popup replaces only those two language
  names. The other nine languages remain on native game fonts. Regenerate the
  assets against the finalized top-bar, status, and filter-row coordinates:
  Bug Report `(411,17,126,26)` at role scale `0.40`, Close `(559,17,94,26)`,
  status label/value/action `(32,142,98,24)` / `(158,142,154,24)` /
  `(435,142,208,24)`, and filter text X `334` / `496`, width `146`, Y `583` /
  `615`. Tight-alpha placement centers Bug Report on both axes. Static
  verification audits all 11 runtime blocks, covers `123/123` overlay
  codepoints, reports minimum fit `1.000`, maximum optical-center error `0.5`
  raster pixel, no edge alpha or slot overflow, and deterministic `8/8`
  regeneration including the manifest. In-game
  size, weight, and alignment remain pending live visual acceptance.
- Redesign F6 as one responsive settings page with Mod Status, Language,
  Marker Visibility, Height Indicators (Radar Only), Filter Modes, and Bug
  Report controls. The panel scales and clamps to the available viewport rather
  than relying on one fixed desktop resolution. It opens in Off, On, or Fault
  state; the action becomes Enable, Disable, or Retry. Enable still requires a
  playable world, and Bug Report opens the fixed Nexus Posts page.
- Finalize the F6 interaction hierarchy: Bug Report and Close are separate top-
  bar controls; Mod Status is read-only text beside a thin state-colored strip;
  and Enable, Disable, or Retry remains a separate action that does not close
  the page. Use translucent section cards, equal-width filter choices, corrected
  text alignment, and bounded non-overlapping click regions without adding any
  closed-panel or steady-state work.
- Fix the F6 initialization regression caused by resolving text justification
  from `TextBlock` instead of its declaring `TextLayoutWidget` class. Text
  centering is now optional visual behavior and can no longer disable the
  complete settings panel.
- Fix the exact-artifact F6 construction rejection recorded as `failure=8`
  with `font_abi_details=128`. The runtime log proves the F6 request, selected
  language, grouped header rejection, and unavailable optional CDO font path;
  the source audit separately identified exact-zero `Font.Size` handling as a
  source-level path capable of producing the grouped rejection. The corrected path seeds one bounded
  reference size only for `Font.Size == 0`. If game-widget construction,
  target-size calculation, or font commit still fails, the same F6 transaction
  retries that text once as base UMG `TextBlock`. If both exact-size paths fail,
  one fresh game `DTextBlock` and then one base `TextBlock` fallback leave
  `Font` untouched and use only bounded `SetRenderScale`/pivot presentation.
  Their `target_size=0` sentinel skips both post-prepass exact-size loops. Core
  UMG widget creation, `SetText`, tree insertion, and viewport attachment remain
  fail-closed, and this work runs only during an explicit F6 construction.
- Vertically center native F6 text during event-driven construction and explicit
  presentation changes. After the second layout prepass and exact font-size
  readback, a measured `GetDesiredSize` pass repositions exact font-layout
  TextBlocks around their authored vertical center. Its return structure must
  match the known `Vector2D` identity. An unavailable, faulting, invalid, or
  oversized desired size leaves that text's original slot geometry unchanged
  and does not reject F6. Render-scale fallback text preserves its authored
  slot geometry. This
  presentation-only pass does not change button hit boxes, compact-radar or
  expanded-map geometry, and adds no closed-panel or per-frame work.
- Record the second diagnostics-enabled F6 rejection from the deployed
  `D6CF...` DLL. Every F6 press reached `VISIBILITY_HUB_OPEN_PENDING`, resolved
  the selected and active language to `zh-hans`, and then rejected the page as
  `failure=8`, `font_abi_details=128`, `font_source=2`,
  `font_fallback_reason=2`, and `text_runtime_failure=6`. This places both the
  game `DTextBlock` attempt and its base UMG `TextBlock` retry at failed target-
  size preparation; the log alone does not prove one unique internal cause.
  Source review found that both paths performed an instance-level `Font`
  property lookup that differed from the successful initialization-time class-
  chain lookup. The replacement reuses the initialization-validated
  `TextBlock.Font` class property and verifies owner, inheritance, offset, and
  container access before reading it. This remains explicit-F6-only work. The
  D6CF package set and deployment are superseded, and live F6 opening remains
  `NOT_VALIDATED` until the owner tests the replacement DLL.
- Record the owner retest of exact deployed DLL `5210E27D...`. F6 again reached
  `VISIBILITY_HUB_OPEN_PENDING`, selected `zh-hans`, and rejected with
  `failure=8`, `abi_failures=0`, `font_abi_details=128`, `font_source=2`,
  `font_fallback_reason=2`, and `text_runtime_failure=6`. This proves cached
  class-property reuse alone was insufficient: optional `Font.Size`
  preparation was still incorrectly coupled to required page labels. The
  replacement therefore treats the reflected Font/SetFont layout as optional
  diagnostic detail and retains the bounded no-Font-mutation render-scale path.
- Record the owner visual rejection of exact deployed DLL `64BFEB26...`. F6
  opened in Simplified Chinese, but the log reported `font_source=3` and
  `font_fallback_reason=6`; labels were clipped and compressed because
  `SetRenderScale` changed paint output without changing Slate line boxes.
  The root cause was the integer-only compatibility check for
  `FSlateFontInfo.Size`, which is reflected as floating point. The replacement
  accepts floating-point and integer numeric metrics through their matching
  UE4SS APIs, writes the target only into a copied `SetFont` parameter, and
  verifies the committed widget size after the call and again after prepass.
  The render-scale path remains an emergency availability fallback only and is
  not visual acceptance. This work occurs only during explicit F6 construction
  and adds no frame work.
- Make `SetWorldMapImage` the only positive world-map-open signal. F7 and
  travel catch-up may service or clear an existing latch but cannot suppress
  the compact radar merely because a constructed `DLayerMap` reports visible;
  the compact radar now attaches without first opening the expanded map.
- Apply Area Quest height state to every visible Area Quest marker itself.
  A multi-band profile first selects the uniquely nearest existing source band
  from the authored marker Z. Inside that selected band's inclusive +/-500
  margin it keeps the normal black frame and shows three white dots. Below the
  selected band it becomes an upward black triangle; above it becomes a downward
  black triangle. An exact-distance tie or missing profile keeps the frame with
  no dots and no direction. Directional triangles remain centered on the original task
  marker; no Treasure-style horizontal clearance or separate left-side Area
  Quest pointer is used.
- Document the confirmed third-party font boundary: a font PAK that replaces
  Common/TC assets with a face missing Hangul or extended Latin also removes
  those glyphs from F6. Renaming the overridden asset cannot restore absent
  glyph data.
- Prefer the game's compatible `DTextBlock` for transient F6 text.
  `ForceApplyLanguageFont` and a compatible class-default-object composite font
  are best-effort. An exact zero `Font.Size` is seeded once; a game-widget
  construction, target-size, or font-commit failure retries that text once as
  base UMG `TextBlock` during the same F6 open. The real reflected `Font.Size`
  still participates in layout, is constrained by each slot's safe line height, and
  is reapplied and read back after `AddToViewport` and layout prepass. If both
  exact-size attempts fail, a fresh DTextBlock/base TextBlock fallback leaves
  Font untouched and applies the already bounded viewport/DPI scale through
  render scale and a justification-aware pivot. Missing optional font evidence
  never changes the selected UI language or disables the complete page.
- Restore expanded-map atlas-local placement after live screenshots rejected
  the full-parent outer-host experiment. Each outer native Canvas slot now
  occupies `{atlas_left,atlas_top,atlas_width,atlas_height}`, and its
  `Panel_Point` Image occupies local `{0,0,atlas_width,atlas_height}`. This
  correction explicitly withdraws the added post-attach parent-size check,
  parent-growth rejection, and extent-change token: it introduces no new
  parent-size assumption and continues the existing witnessed stable-geometry
  and bounded map/zoom reconstruction strategy. A same weak parent observation
  returns `Unchanged` with no Remove/Add, reparent, or extra atlas render; only
  a real weak parent identity change reparents. Marker projection, cached Slate transforms,
  DPI, zoom, player-anchor, independent X/Y scaling, and aspect-ratio handling
  are unchanged, and no `3000`/`8000` geometry constant is used. Expanded-map
  capacity is 4,096. The accepted maximum is 2,500 Treasure rows plus 279 fixed
  non-Treasure rows, or 2,779 total, leaving 1,317 spare slots. Expanded-map
  alignment remains pending exact-build live acceptance.
- Refine expanded-map glyph raster styling at revision 50: Treasure uses a
  cleaner symmetric lid/body/lock silhouette, Fly/Mole/Wave render complete
  shadow-outline-fill layers, Mole gains separate handle/head highlights, and
  Boss, Assault, and Area Quest internals remain readable at small sizes. Both
  atlases are 3072-by-3072, a 50-percent increase in linear raster density. Two
  decoded BGRA atlases occupy about 72 MiB raw versus about 32 MiB at 2048. This
  changes event-built raster density only: marker coordinates, projection, zoom,
  parent ownership, and all outer/inner container geometry are unchanged.
  Runtime visual acceptance remains pending.
- Keep all expanded-map source and contracts completely unchanged during the
  final F6/localization/compact-indicator closeout. Atlas geometry, 4,096-entry
  capacity, coordinates, projection, zoom, parent ownership, and style revision
  50 are outside this source change.
- Record the bounded dense-map evidence separately from capacity headroom. The
  observed snapshot contained 1,632 total markers, including 1,501 Treasures,
  below the old 1,785 limit; capacity was therefore not the flicker root. The
  current log shows one attach and no repeated detach/rebuild sequence. The
  larger 4,096 limit protects the accepted catalog envelope and future growth,
  but dense-Treasure flicker still requires exact-artifact live acceptance.
- Pin the mini-game height generator to the reviewed mini-game and actor-position
  inputs. The 83 trusted rows are a generated release input, not a runtime PAK
  extraction or scan.
- Close out the current F6 presentation, localized-overlay, and compact-
  indicator replacement with Core `2/2`, compact, world-map, PostRender,
  release-hygiene, and clean native `/W4 /WX` gates. Its source-bound
  `main.dll` SHA-256 is
  `6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`, from
  compiled-source SHA-256
  `A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
  Current build status is `PASSED`. Local diagnostics-enabled deployment of
  exact DLL `6AEFDACC...` passed with matching source, build, and installed
  hashes. Its rollback backup is
  `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
  Backup
  `dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`
  belongs to the superseded intermediate 59529B2A deployment and is not current
  candidate evidence. Setup ownership and gameplay evidence remain pending. The earlier
  634D283A local debug deployment and backup remain superseded historical
  evidence only.
  `Build-Release.ps1` package validation passed for exact DLL `6AEFDACC...`:
  Setup reports `20/20`, the manual-copy matrix reports `2/2`, payload
  equivalence, manual layout, and clean-target policy validation pass, and all
  three public ZIPs re-extract byte-identically. The earlier package set bound
  to `5210E27D...` is superseded historical evidence only.
  Live gameplay, F6 visual alignment, controller, exit behavior, performance,
  responsive-layout, every-language font/glyph, and expanded-map visual
  acceptance remain separate live runtime checks; static verification cannot
  substitute for them.
- Treat the healthy 84A360B0 runtime log as evidence only for those exact prior
  bytes. It records no renderer, ABI, F6, or UE4SS fatal error and reaches normal
  shutdown, but it cannot validate the newly built 6AEFDACC compatibility
  replacement, F6 presentation, localized-overlay, or compact-outline bytes.
  Those bytes still
  require a fresh live test.
- Bound shutdown against the reported 09:33 exit crash. That crash came from the
  old E64 artifact and does not prove causality. Current shutdown closes ingress
  atomically; only a known live GameThread performs UE cleanup, true process
  teardown performs no I/O/log/join/close, and one finalizer owns the final
  flush. A repeated-exit runtime matrix remains pending.

## 2.1.1

Version 2.1.1 is a corrective patch for controller-opened menu suppression and
Area Quest height-arrow accuracy and presentation. It does not relabel or
replace the accepted 2.1.0 history.

- Suppress the independent compact-radar host when the world map is visibly
  open or the game is paused, including controller paths that do not expose a
  hardware cursor. The exact `SetWorldMapImage` edge latches map suppression;
  bounded `IsVisible` catch-up and optional `IsGamePaused` sampling share the
  existing 250 ms activity service. The 16 ms path consumes Booleans only. No
  controller mapping is read, and no input poll, focus hook, timer, scan,
  allocation, or recurring log is added.
- Introduce the first separately sourced Area Quest height correction while
  retaining all 147 original MnMRadar marker coordinates. This historical
  single-height contract is superseded by the 2.2.0 one- or two-band profile
  model and is not a current release gate.
- Give Area Quest height indicators a distinct shaftless chevron with black
  outline and white fill. It collapses the two shaft pieces in the existing
  fixed six-piece group; treasure keeps its complete colored shafted pointer.
  No widget, allocation, or UObject read is added to the motion path.
- Preserve an existing installed `treasure_overrides.txt` across developer
  deployment, detect a staging-time change, restore the preserved bytes, and
  verify their SHA-256 identity. The shipped default remains the single
  `ignore 11230106` rule.
- The core tests, all four current static source gates, a clean native `/WX`
  build, Setup `20/20`, manual-copy `2/2`, payload equivalence, clean-target
  validation, and three-archive re-extraction pass. The packaged `main.dll`
  SHA-256 is
  `B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`.
  A local diagnostics-enabled deployment installed that exact DLL, retained
  the existing treasure override byte-for-byte, enabled exactly one Radar load
  line, and left no predecessor load line. Live controller behavior, Area Quest
  visuals, gameplay, and external performance remain `NOT_VALIDATED`.

## 2.1.0

Version 2.1.0 is the post-2.0.0 corrective release. It keeps the published
2.0.0 feature set and incorporates the first runtime feedback fixes.

- Give the nearest visible area quest its own fixed six-piece height pointer.
  Treasure and area-quest channels retain independent numeric Z targets and
  may display simultaneously without allocation or UObject reads on the
  16 ms motion path. Treasure uses its selected category color, while the area-
  quest channel uses the official cyan accent; both retain the dark outline.
- Lower the transparent time/phase group by six reference pixels and widen it
  so the digits and phase glyph have a clearer horizontal gap. Preserve the
  four configured presentation bands beginning at 06:00, 12:00, 18:00, and
  21:00, with distinct sunrise, full-sun, sunset, and crescent-star geometry
  rather than color-only variants. These names and thresholds are presentation
  policy, not proven game-native phase semantics. Weather remains unavailable
  and unqueried.
- Pin compact-host reflow tests to deterministic numeric viewport/DPI inputs
  including 3440x1440, 3840x1600, 2560x1080, ordinary windowed sizes,
  fullscreen-sized inputs, and DPI changes. The runtime source remains
  `WidgetLayoutLibrary.GetViewportSize/GetViewportScale`, not desktop monitor
  geometry. These unit inputs do not validate internal 21:9 black bars, native
  21:9, windowed client geometry, or runtime Slate layout.
- Thread a nondefault native build directory and its exact receipt through
  release hygiene, Setup, final packages, and manual-copy verification. This
  fixes stale default-receipt packaging while preserving project-root and
  byte-identity gates.
- Keep expanded-map zoom handling on the established witnessed parent-local
  geometry and bounded reconstruction path. Projection scales world-space X/Y
  deltas by the witnessed native parent's live local width/height;
  `WorldMapUISize` remains authored metadata, not a substitute parent extent.
  The atlas-local placement correction adds no parent-size post-check,
  parent-growth rejection, or new extent-change allowance.
- Replace the older four-stage tail with five deadlines at
  100/250/500/1,000/1,250 ms. Service takes at most one fresh observation per
  due game-thread pass; overdue deadlines remain due and still advance only one
  observation per later pass. The first four passes
  are read-only; only the final pass may mutate the tree under the existing
  retained-parent witness and stable-geometry rules. The established bounded
  map/zoom reconstruction path performs no steady marker recollection, file
  polling, or unbounded widget work.
- Include the post-2.0.0 parent-local geometry correction for windowed, 21:9,
  16:10, and other finite positive expanded-map parent extents. There is no
  centered fallback and no per-frame aspect-ratio work.
- Include the post-2.0.0 area-quest completion correction for tasks such as
  cooking and delivery that may complete without an object disappearance.
  Exact `COMPLETE_CNT` growth remains authoritative, with one immediate and at
  most two delayed positive-only exact-ID save confirmations on the existing
  below-normal worker; treasure SQL is skipped and the schedule is not
  periodic.
- Add explicit static gates for exact live parent-width/height projection,
  retained-parent witness, one observation per due pass, no overdue
  multi-observation collapse, read-only first four passes, existing stable-
  geometry handling, final-pass-only mutation, and absence of immediate
  same-layer atlas rebuilding.
- Reduce Bird Egg steady UObject work without changing discovery or storage:
  the nearest-16 availability service now shares the existing 250 ms discovery
  edge instead of owning a separate 100 ms timer. Exact EndPlay still retires
  immediately, and the 400 ms unavailable debounce remains the bounded
  fallback. The maximum active-state query rate falls from 160 to 64 reads per
  second with no new timer, scan, allocation, or SQL path.
- Make F6 `ASSAULT MODE: ALL` a literal static catalog view. It displays all 40
  Assault records regardless of save readiness, active hours, defeat state, or
  cooldown. `AVAILABLE` retains every authored time, state, and cooldown gate;
  actual defeat/cooldown authority and Boss/area-quest selection are unchanged.
- Current-source core/static gates, a clean native `/WX` build, Setup `20/20`,
  manual-copy `2/2`, payload equivalence, and archive re-extraction pass with
  `main.dll` SHA-256
  `D4EE700178244096D3095BB55F5F88F94396E46D1A5C926FD2963FFEFEDA3775`.
  That DLL is sealed in the authoritative `dist/final-2.1.0` package set. It is
  not deployed or gameplay-validated. The earlier `4AFE...` and `BDE21...`
  package sets are historical and non-authoritative for this source.

## 2.0.0

Version 1.2.0 was never published. Its unreleased candidate changes are
superseded by and included in 2.0.0.

- Replace the numeric visibility schema with an Auto Pickup-style, bounded
  `visibility.ini`: readable `[radar]`, `[map]`, and `[modes]` sections use
  named Boolean category keys and `available|all` mode values. The startup-only
  parser accepts at most 4 KiB, rejects incomplete, duplicate, mixed, unknown,
  or malformed input to safe defaults, and retains strict legacy schema 1-4
  compatibility. F6 changes still apply immediately and now atomically rewrite
  the readable current format only on a real edge; no file polling is added.
- Make the startup-only diagnostics file as readable as Auto Pickup's config:
  one strict, bounded `[diagnostics]` section with exactly one
  `debug_logging=true|false` key and `debug_logging=false` by default. The parser
  accepts at most 2,048 bytes.
  Exact legacy `event_log_enabled=true|false` files remain accepted during
  Update / Repair. Enabled logs now add one bounded session header plus
  allocation-free sequence, UTC Unix-millisecond, and elapsed-millisecond
  fields under log schema 2 without changing event cadence or adding polling.
- Replace executable-script manual installation with two Nexus-compatible,
  script-free archives whose roots map directly to `DS/Binaries/Win64`.
  No-UE4SS carries the Radar payload and a clean one-line `mods.txt` that must
  never overwrite an existing load-control file; With-UE4SS is explicitly
  clean-target only and carries the pinned nested runtime plus clean load control. Existing
  Radar updates remain a Setup responsibility so user configuration is not
  silently overwritten by a generic directory merge.
- Add a full-width accent divider between the F6 RADAR/MAP visibility rows and
  the display-mode controls. It reuses the existing Bird Egg row divider widget and
  changes no control, service, allocation count, or closed-panel work.
- Render compact Bird Egg markers as native vertical ovals matching the game's
  egg silhouette while reusing
  the existing four preallocated pieces, marker pool, and update schedule.
- Persist F6 choices and update compact selection on every real edge, but
  coalesce expanded-map changes made while the Hub is open into at most one
  atlas rebuild when the Hub closes; returning to the opening state performs
  no rebuild. The observed session previously rebuilt the
  atlas 20 times at 81-100 ms each during repeated mode changes; the new path
  preserves the final state while removing those intermediate rebuilds.
- Add a confirmed transactional `Uninstall` action to Setup. It requires strict
  same-product ownership, removes only the exact Radar tree and its valid
  `mods.txt` entry, preserves UE4SS, unrelated Mods, saves, encoding, and line
  endings, and rolls back on failure.
- Make Setup derive its actions from the inspected installation state. It shows
  `Install`, `Update`, or `Repair` as appropriate, enables `Uninstall` only for a
  strictly owned active installation, and refreshes the state after each
  successful action instead of closing.
- Reflow the retained compact UMG host when the existing one-hertz minimap
  scale service observes a real viewport-size or DPI change. Windowed and
  fullscreen transitions now update the host origin and render scale without
  F8/F7; the 16 ms motion path, clock source, marker pool, and service cadence
  are unchanged.
- Project expanded-map markers from the live `PlayerIconWidget` alignment pivot
  by converting its current cached Slate geometry through `LocalToAbsolute`,
  then into the selected native icon Canvas through `AbsoluteToLocal`, only
  during the bounded attach transaction. The first valid numeric result is a
  seed; a stable sample at least 150 ms later is accepted. If that result
  changed, one third sample may establish stability, for three samples maximum.
  Only numeric state crosses samples--no sampled UObject wrapper or `FGeometry`
  is retained. This supports 21:9, 16:10, and arbitrary finite positive parent
  extents. Missing, implausible, or still-unstable geometry uses the existing
  bounded retry path; no centered fallback or new polling is published.
- Arm an exact catalog area-quest witness from a schema-valid dynamic event
  without requiring a prior `PROGRESS` sample. The event remains non-completion;
  exact-ID `END` inside the existing ten-second witness is authoritative.
  F7 records an exact per-ID `COMPLETE_CNT` baseline; a missing ID is zero only
  after a valid single-owner query, while an invalid or ambiguous owner/query
  leaves it unknown and disables this fallback. Unresolved expiry may queue one
  immediate exact-ID save confirmation plus at most two 15-second retries on
  the existing single below-normal worker. Only strict count growth above the
  known baseline confirms completion. Three non-confirming attempts lock the
  same task generation until a new F7 or a proven `NONE`/`END` then later
  `ACCEPTABLE`/`PROGRESS` generation. The path skips treasure SQL and adds no
  periodic SQL, polling, dynamically growing queue, or retained UObject.
- Fix Bird Egg availability reflection by reading `InteractableValue` and
  `InteractTypeValue` as their actual one-byte scoped enum properties and then
  reading the validated integer underlying properties. The accepted state
  remains exactly `2/2`, with no new scan, timer, SQL, or allocation.
- Keep manual copying bounded to the documented direct `Win64` mapping. The
  release gate proves both script-free layouts, safe existing/missing
  `mods.txt` instructions, and Setup/manual payload byte equivalence; Setup
  remains the only supported automatic update, repair, settings-preservation,
  and uninstall route.
- Add a persisted F6 `AREA QUEST MODE` with mutually exclusive `AVAILABLE` and
  `ALL` choices. Available mode preserves strict prerequisite proof. All mode
  displays every unfinished catalog task while still excluding saved and exact
  runtime completions. The choice reuses existing fixed selection passes and
  adds no query, timer, SQL, UObject scan, or steady-state allocation.
- Add an independent persisted F6 `ASSAULT MODE` with mutually exclusive
  `AVAILABLE` and `ALL` choices. Available mode preserves the authored Assault time
  window. All mode bypasses only that display-time condition, while encounter
  state and the existing 120-minute cooldown remain authoritative. Boss and
  area-quest selection do not change. The choice reuses the fixed 49-entry
  selection and open-only Hub service and adds no timer, SQL, provider query,
  object scan, or steady-state allocation.

- Accept mounted underwater treasure interaction immediately only when the
  callback Actor is the exact callback-local `Rider` UObject owned by the
  freshly resolved current Pawn, Rider and Pawn share a non-null World, the
  receiver is one of the three mount-only treasure classes, and the receiver
  is within eight metres of the current player sample.
- Preserve the exact-ID, positive-only save confirmation as the fallback
  for every rejected non-Pawn interaction. The immediate Rider route adds no
  scan, timer, worker, retained UObject, recurring SQL, or steady-state work.
- Add pure policy tests and source gates for the exact Rider, same-World,
  mount-only receiver, proximity, and fallback boundaries. Exact 2.0.0
  underwater gameplay and frame-time acceptance remain pending fresh owner
  testing.
- Add compact-only bird-egg markers for exact `Bird_Egg01_C` and
  `Bird_Egg02_C` runtime actors. One UObject creation listener publishes only
  weak identities into a fixed 512-slot pool; the existing 250 ms control
  service performs at most eight one-time position queries per tick, and the
  100 ms active probe tracks only the nearest fixed set of 16 candidates.
- Replace Actor-level hidden-state inference with the exact owned interaction
  state. A Bird Egg is available only when its Actor-owned
  `DInteractableComponent` reports `InteractableValue=2` and
  `InteractTypeValue=2`. An unknown schema/value read stays pending for the
  existing bounded service, while exact Bird Egg EndPlay removes the matching
  weak candidate immediately.
- Add an independent `BIRD EGGS` compact toggle to F6. The expanded-map column
  is intentionally unavailable, the public compact default is enabled, and no
  bird-egg bit is admitted to the world-map mask.
- Keep bird-egg discovery free of global UObject enumeration, SQL, retained raw
  Actor pointers, dynamic queues, filesystem polling, or expanded-map work.
  The state correction adds no new poll, schedule, enumeration, or SQL path.
- Treat the exact `TitleMap` identity as a cross-save hard boundary instead of
  ordinary presentation suppression. Entering it disables the radar, detaches
  both renderers, clears weak candidates and mutable save-owned runtime state,
  and latches activation off. A subsequently loaded save stays disabled until
  a loaded open world receives an explicit F7; title-screen and incomplete-load
  F7 requests fail closed. The boundary reuses existing transition/world
  identity signals and adds no new polling, enumeration, or SQL work.
- Merge these previously unpublished changes into the final 2.0.0 identity;
  there is no public 1.2.1 release.
- Publish only ExperimentalNested channels: the recommended Setup archive, a
  `Manual-No-UE4SS` archive for an existing compatible ExperimentalNested
  runtime, and a pinned Experimental `Manual-With-UE4SS` archive. Do not restore
  a StableRoot payload. Public diagnostics remain disabled by default.
- Treat the previously recorded source/static, native-build, Setup, manual,
  archive, and native test-deployment results as evidence for the prior artifact
  set. The current final-repair source must repeat those gates before release;
  public-Setup deployment, gameplay, external frame-time, and publication
  acceptance remain separate.

- Fix underwater treasure completion without weakening the immediate local-Pawn
  authority gate. A rejected non-Pawn interaction may arm confirmation only
  when the hook receiver is an exact treasure Actor, its immutable catalog ID
  resolves uniquely, and it is within eight metres of the current local player.
- Store only catalog bits, attempt counters, and deadlines in fixed-capacity
  process-local arrays. No Pawn, proxy, treasure Actor, or other UObject is
  retained across the callback, frames, travel, or worker execution.
- Add an exact-category, positive-only SQLCipher confirmation scope on the
  existing below-normal worker. The first request is due after 15 seconds; a
  negative or failed result receives one final bounded attempt 285 seconds
  later. Each request contains at most 64 exact treasure IDs, skips encounter
  and dynamic-quest SQL, and never becomes a poll or recurring full snapshot.
- Log the rejected interactor and fresh Pawn class/name once at the event edge,
  plus queued, requested, confirmed, retried, and exhausted confirmation
  outcomes. Exact 2.0.0 underwater gameplay and frame-time acceptance remain
  pending fresh owner testing.

## 1.1.0 installer install-or-repair update

- Classify a structurally complete ExperimentalNested UE4SS layout plus a
  strictly owned existing Radar directory as `Update / Repair`. Older owned
  Radar releases are eligible; a game or UE4SS DLL byte change alone is not a
  compatibility failure.
- Replace fixed installed game, UE4SS loader, and proxy hash allowlists with
  bounded AMD64 PE32+ structural validation. Embedded bootstrap/conversion
  resources remain SHA-256 verified as Setup payload-integrity evidence only.
- Keep compatible existing `UE4SS.dll`, `dwmapi.dll`, and
  `UE4SS-settings.ini` bytes unchanged during Update / Repair. Refresh the
  verified Radar DLL, immutable bundled catalogs, and mapping file.
- Preserve `config/visibility.ini`, `config/diagnostics.ini`, and
  `data/defaults/treasure_overrides.txt` byte-for-byte after strict validation.
  Update / Repair uses a temporary rollback journal and removes it after a
  verified commit; only a real UE4SS bootstrap/conversion retains a complete
  original-layout backup.
- Extend the isolated Experimental installer matrix from 14 to 17 cases. The
  added cases prove changed-but-structural loader/proxy acceptance, byte-exact
  user-state preservation with catalog refresh and no persistent backup, and
  Update / Repair from an older strictly owned Radar release.

## 1.1.0 game-update compatibility repair

- Remove the obsolete fixed game-executable SHA-256 allowlist from Setup. The
  selected path must still be the exact DragonSword shipping executable and a
  bounded executable AMD64 PE32+ image; malformed images, DLLs, active games,
  unapproved UE4SS ABI files, and payload-integrity failures remain fail closed.
- Retain the observed game SHA-256 only as confirmed transaction identity and
  install-record provenance. A game update no longer requests a reinstall,
  rebuild, or separate updater merely because those bytes changed.
- Keep the packaged save-owner RVA as the zero-scan fast path. When that binding
  is stale, the existing below-normal save worker may scan executable PE
  sections once per process, accept exactly one bounded owner-pointer pattern,
  cache only its numeric RVA, and validate the live save key. Failure remains
  feature-local and fail closed, with no watcher, timer, recurring scan, or
  game-thread work.
- Clarify that F7 re-synchronizes runtime and save-backed state but does not
  reinstall the Mod or regenerate immutable catalogs. Static data changes still
  require a new catalog release.
- Extend the isolated Experimental installer matrix from 12 to 14 cases with a
  structurally valid changed game image and a malformed-image rejection case.

## 1.1.0 installer conversion update

- Show an explicit `Installation successful` confirmation and close Setup after
  the user dismisses it.
- Before conversion, copy the complete active UE4SS layout, Mods, settings, Mod
  configuration, load-control files, logs, mapping, and header-dump directory
  into `Win64/UE4SS-<layout>-<date>-Backup`, preserving paths relative to the
  original `Win64` directory without recreating the parent `DS` tree.
- Store transaction rollback files under short numbered names so long Mod paths
  do not fail solely because the backup path is longer than the active path.
- Only after every backup file passes size and SHA-256 verification, migrate
  Mods and configuration, remove the old active UE4SS layout, and install the
  pinned ExperimentalNested runtime. The old layout remains only in backup.
- Require the pinned ExperimentalNested UE4SS runtime for the native Radar.
- When UE4SS is absent, install the embedded hash-verified Experimental runtime
  after confirmation.
- When another, incomplete, or dual UE4SS layout is detected, show a dedicated
  conversion confirmation before changing files.
- Reject conflicting duplicate Mod files before mutation, verify the installed
  loader and Radar, and roll back recorded changes on failure.
- Replace the obsolete StableRoot/dual-ABI installer test gate with a 12-case
  Experimental conversion, migration, and rollback gate.

## 1.1.0

- Add a separately compiled native Radar payload for the official UE4SS v3.0.1
  StableRoot ABI while retaining the pinned ExperimentalNested payload.
- Make the one-click installer select the matching payload for an existing
  approved layout. Arbitrary UE4SS builds remain fail-closed because native
  plugin ABI compatibility cannot be inferred from a version label alone.
- When UE4SS is absent, transactionally install the embedded, SHA-256-verified
  official UE4SS v3.0.1 StableRoot release before installing Radar.
- Preserve unrelated compatible Mods, user settings, `mods.txt` encoding and
  line endings, and roll back every recorded mutation after an injected or
  real installation failure.
- Reject incomplete, unknown, dual-loader, competing Mods-root, and reparse
  target layouts without mutation.
- Preserve the 1.0.0 R8 renderer, data providers, polling schedules, marker
  behavior, and public diagnostics defaults. Exact 1.1.0 gameplay and
  performance acceptance remains `NOT_VALIDATED` pending owner testing.

## 1.0.0-runtime-repair8

- Preserve the R7 renderer, projection, marker geometry, map lifecycle, and
  control schedules unchanged. Register the exact native
  `DsFieldCharacter.NetMulticastSetDeathProcess` receiver as a second encounter
  completion route. Only reflected `DENM_ProcessState::End` is accepted, and
  the receiver must still pass the existing exact observed weak identity,
  immutable catalog class, `DsMonsterCharacter`, activation/epoch, current
  availability, visible-seen, and 100-metre player-distance gates. The callback
  publishes one bit in a second fixed 49-bit mask and performs no SQL,
  enumeration, renderer work, allocation, or polling.
- Keep area-quest blueprint end fail-closed because the same event can represent
  failure or abandonment. When its existing ten-second exact-ID witness expires
  without a verified `End` state, queue that exact catalog index in a fixed
  147-bit set. Multiple witnesses coalesce into at most one below-normal worker
  request. The confirmation skips `tb_treasure_box`, queries encounter respawn
  and dynamic-quest completion state, and accepts only an unambiguous positive
  row for an exact requested quest ID. A failed or negative confirmation does
  not hide anything and is not retried until a later exact completion event.
- Retain one full F7 reconciliation, but allow the same single worker to accept
  bounded event-driven completion confirmations. At most one request may be
  pending, running, or awaiting collection. There is no 30-second SQL loop,
  recurring timer, dynamic queue, save-file poll, or new motion-path work.
- Change the runtime label to
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_0_0_R8`. Exact-artifact R8
  gameplay and performance acceptance remain `NOT_VALIDATED` pending real
  Boss, Assault, and mixed area-quest completion tests.

## 1.0.0-runtime-repair7

- Improve only the compact-map Fly mini-game glyph: use a deeper blue fill and
  a 1.25-unit dark-blue native Slate outline on the existing four pieces. The
  expanded-map glyph, marker geometry, widget count, render layers, polling,
  scans, timers, and update schedules are unchanged.
- Quarantine exactly one confirmed nonexistent treasure marker: save ID
  `11230106` at `(182813, 162051, 3150)`. The immutable render catalog retains
  1,693 unique records, the actor catalog retains 1,692 unique records, and
  this ID is their sole render-only difference. The startup-only default
  override is exactly `ignore 11230106`; no coordinate range, class family, or
  neighboring treasure is suppressed.
- Add a fail-closed catalog gate for that exception. It verifies the exact
  identity, section `2142120000100`, coordinates, generated UID
  `DT_Unlock_G3_11206`, 1,693/1,692 unique-count contract, and
  singleton set difference, so a future regenerated actor record cannot remain
  silently ignored without review.
- Retire the legacy loose `Stage-Release.ps1`, `Install.cmd`, and
  `installer/Install-DragonSwordNativeWorldRadar.ps1` distribution paths.
  `Build-Release.ps1` is the only release builder. It emits the exact four-file
  Setup-first artifact plus a separate ExperimentalNested manual archive built
  from the same source-bound payload and clean public configuration defaults.
  Both archives are reopened, freshly re-extracted, and byte-compared with
  staging; the Setup checksum sidecar is independently revalidated before the
  manifest records success. The manual archive's root `SHA256SUMS.txt` is also
  verified against every other file before and after extraction. The manual
  archive contains no executable install path and requires explicit `mods.txt`
  editing documented in its README. An independent one-case manual-install
  matrix installs the exact archive beside an isolated Setup install and
  requires the shared installed file set, sizes, and SHA-256 hashes to match.
- Harden Setup ownership validation against nested or unrelated product-name
  text and bind replacement to strict top-level product metadata plus manifest
  and payload hashes. A recognized target requires schema-5 release and schema-1
  package metadata, 2-256 unique manifest files, exact size/hash identity,
  proof of `metadata/release.json` plus a version/label-bearing `dlls/main.dll`,
  and an exact recursive tree containing only manifest files and bounded live
  state. Ownership JSON is limited to 4 MiB/depth 16, each and total manifest
  data to 256 MiB, legacy marker/example files to 64 KiB, logs to 2 MiB, atlas
  caches to 16 MiB, and the install record to 1-64 KiB. Unknown, oversized,
  duplicate, extra, or spoofed ownership must make zero mutation.
- Reject an active external `DragonSwordWorldRadar : 1`, malformed same-name
  entry, or exact `DragonSwordWorldRadar/enabled.txt` path in any approved Mods
  root before backup or mutation. Preserve an exact disabled
  `DragonSwordWorldRadar : 0` entry and never delete or disable that external
  mod. Continue removing valid `DragonSwordWorldRadarObjectState` predecessor
  entries as the separate owned migration.
- Keep `visibility.example.ini` and `diagnostics.example.ini` as immutable
  Setup-embedded defaults only. A clean installed target contains the two live
  user files `visibility.ini` and `diagnostics.ini`, not duplicate example
  files; recognized upgrades preserve both live files independently.
- Exclude build, installer, archive-verification, staging, runtime, and
  distribution roots from source control so generated release artifacts cannot
  be mistaken for source files.
- Preserve the R6 renderer schedules, layer ownership, marker projection, and
  map lifecycle while repairing the fixed 49-bit encounter-death handoff.
  Ordinary consumption waits for a valid context and does not recheck an
  accepted event's time window. Applied bits clear individually; an apply
  exception retains its bit. F7, disable, travel, and activity-suppression
  boundaries may settle pending cooldown state numerically without renderer
  mutation before reset. Only process shutdown and UObject-array shutdown
  hard-clear the mask. The repair reuses the existing 250 ms service and adds no
  poll, timer, scan, SQL, queue, or steady work. A visible exact expanded-map
  attachment may rebuild once for an ordinary real numeric delta; a hidden
  retained layer defers until its next exact `SetWorldMapImage` edge.
- Change the runtime label to
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_0_0_R7`. Exact-artifact R7
  gameplay and performance acceptance remain `NOT_VALIDATED`. Public
  publication remains `BLOCKED` by the three recorded rights and provenance
  reviews.

## 1.0.0-runtime-repair6

- Preserve the R4 Boss/Assault gameplay result and the R5 maximum-zoom
  four-settle repair unchanged. The current installed R5 diagnostic session
  attached 328 markers to `DLayerMap` serial 3, then proved a separate
  minimize/restore replacement: serial 4 arrived with `map_id` temporarily
  unavailable while the zoom settle still targeted the old attached renderer,
  leading to failure 103, state 5, detach, and no automatic recovery.
- Gate zoom and same-layer `SetWorldMapImage` restacks on exact full weak layer
  ownership. A candidate-layer mismatch returns `RetryLater` before any
  same-layer payload validation, so an incomplete replacement cannot fault or
  detach the valid-but-old renderer graph.
- Let the exact replacement layer's `SetWorldMapImage` event consume at most
  one existing rearm bit when the renderer is `Ready` or is still `Attached`
  to a different old layer. The existing three-attempt readiness service
  remains the only attach budget and the consumed bit is never replenished by
  retry or focus state.
- Treat a raw create-listener candidate as evidence only. An incomplete or
  spurious candidate cannot detach the attached renderer before its exact
  `SetWorldMapImage` readiness edge.
- Add no focus hook, focus poll, recurring timer, UObject enumeration, steady
  work, coordinate change, projection change, atlas-content change,
  marker-selection change, or Boss/Assault state change.
- Change the runtime label to
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_0_0_R6`. Exact-artifact R6
  gameplay and performance acceptance remain `NOT_VALIDATED`.

## 1.0.0-runtime-repair5

- Record the R4 gameplay result separately from this repair: one real Boss and
  one real Assault defeat completed through the exact observed
  `NetMulticastNotifyDeath` route without F8/F7, removed their markers, and
  applied the runtime cooldown.
- Fix only the remaining expanded-map composition lifecycle. R4 could complete
  a successful one-shot background-then-foreground restack, then lose visual
  priority when maximum-zoom close/reopen inserted a game-native child later at
  the same maximum Canvas Z.
- Register the exact
  `/Script/DSClient.DPanelWorldMap:OnSliderValueChanged` event as an additional
  bounded layering trigger. Attach, `SetWorldMapImage`, F7 resume, and zoom
  events each start or restart exactly four settles at 100, 250, 500, and
  1,000 ms.
- Re-resolve the current game-native map-icon Canvas on every settle before
  removing and reinserting the background and foreground radar hosts in that
  order at maximum Z. A changed Canvas fails closed for a full attach instead
  of reusing projection geometry across coordinate spaces.
- Change no coordinates, projection, atlas contents, marker selection, marker
  geometry, category ordering, or state-decision logic. The settle tail is
  event-local and finite; it adds no steady polling, retry queue, or per-frame
  layering work.
- Change the runtime label to
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_0_0_R5`. R4 encounter evidence
  remains historical evidence only; exact-artifact R5 gameplay and performance
  acceptance remain `NOT_VALIDATED`.

## 1.0.0-runtime-repair4

- Add the optional
  `/Script/DS.DsFieldCharacter:NetMulticastNotifyDeath` pre-hook as the primary
  Boss/Assault completion boundary. It accepts only the exact already-observed
  weak receiver, exact immutable catalog class, `DsMonsterCharacter`, current
  activation and epoch, current availability, `visible_seen`, and a current
  player-to-Actor distance no greater than 100 metres.
- Keep the death callback event-only and constant-bounded. It sets one bit in a
  fixed 49-bit atomic mask; the existing 250 ms control service consumes the
  mask, rechecks numeric state, applies the 120-minute cooldown, and refreshes
  renderer state. The callback performs no UObject enumeration, SQL query,
  dynamic queue, recurring timer, continuous polling, or atlas work.
- Treat the death hook as optional capability rather than required-runtime
  readiness. Missing, non-native, or failed registration reports a bounded
  disabled result without disabling the rest of the radar or starting a retry
  loop.
- Preserve the strict dev65 fallback: four present samples over one second,
  then forty missing 250 ms samples over ten seconds while every lifecycle,
  availability, visibility, and 100-metre gate remains valid. A far pooled
  Actor position may cause range eviction but no longer overwrites the last
  trusted in-range fallback position.
- Remove the R3 moribund assumption from encounter completion. The target
  `MonsterCharacterData` records set `UseMoribund=0`, and the R3 runtime log
  contains no moribund completion hit, so it is not reliable primary evidence.
- Change the runtime label to
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_0_0_R4`. This repair does not
  change map coordinates, compact or expanded projection, marker positions,
  icon geometry, icon sizes, colors, atlas composition, or category layering.
- Keep evidence states separate. Static checks, unit tests, compilation,
  packaging, and deployment do not prove gameplay completion. Acceptance
  requires one Boss and one Assault defeat without F8/F7, each producing
  `ENCOUNTER_DEFEATED_NATIVE
  evidence=exact_observed_net_multicast_notify_death` and the corresponding
  runtime state application.

## 1.0.0

- Arm the existing ten-second numeric disappearance fallback when a visible,
  already observed Boss or Assault reaches `RemovedFromWorld`, even if the
  250 ms observer did not accumulate the ordinary one-second presence history
  before a fast kill. The EndPlay event still cannot complete the encounter;
  forty missing samples, ten real seconds, current availability, lifecycle,
  and 100-metre gates remain mandatory, and a returned Actor cancels the
  sequence. Opening the ordinary cursor-owning map no longer erases an exact
  ended numeric observation; live weak-pointer disappearance still resets in
  that context, and travel/activity suppression still clears all evidence.
- Fix expanded-map layering at its actual lifecycle edge. A same-layer
  `SetWorldMapImage` event previously returned before the retained atlas could
  refresh, and maximum-Z alone could tie a newly rebuilt native icon. The post
  event now removes and reinserts the background and foreground hosts, in that
  order, after native children at the shared maximum Z. F7 resume uses the
  same bounded restack; no frame polling or atlas rebuild is added.
- Preserve an already observed Boss or Assault as numeric-only disappearance
  evidence when its Actor reaches `RemovedFromWorld`. The ended weak UObject is
  cleared immediately; the event itself never completes the encounter. Only
  the existing stable-presence, nearby, forty-sample, ten-second missing gate
  may apply the two-hour cooldown. This restores Assault completion without a
  polling loop or F8/F7 save reconciliation.
- Keep both expanded-map atlas hosts at maximum Canvas Z and rely on explicit
  background-then-foreground reinsertion for deterministic equal-Z ordering.
- Remove the accidentally restored stock `ignore 11003` default. The exact
  class and full 3D identity path now owns overlapping treasure resolution;
  user-authored live overrides remain untouched.

- Freeze the dev67 native UMG runtime as the behavioral basis of the 1.0.0
  technical release candidate. R4 now reports the runtime identity
  `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_1_0_0_R4`; exact-artifact gameplay
  and performance acceptance remain pending and are not inferred from the
  version change.
- Define ExperimentalNested as the only supported UE4SS layout for 1.0.0. The
  installer contract accepts only the pinned game executable, nested UE4SS
  loader, and root proxy hashes. StableRoot, missing, incomplete, unknown, and
  dual-loader layouts fail closed without mutation.
- Make the controlling ExperimentalNested `mods.txt` the sole load authority.
  The public payload contains no `enabled.txt`; a recognized legacy marker may
  be backed up and removed only through the transactional upgrade path.
- Remove every valid `DragonSwordWorldRadarObjectState` entry from the
  controlling `mods.txt` during Setup migration instead of retaining a
  redundant `DragonSwordWorldRadarObjectState : 0` line.
- Replace the development `Install.cmd` package contract with an unsigned,
  installer-first four-file archive: Setup executable, Setup SHA-256 sidecar,
  `INSTALL.md`, and `THIRD_PARTY_NOTICES.txt`. The release gate requires an
  exact isolated-installer result of 19 passed, 0 failed, and 0 skipped.
- Keep public diagnostics disabled. A diagnostics-enabled gameplay deployment
  is an installed-copy-only override and must never be used to rebuild the
  public archive.
- Keep source, build, package, installer, deployment, gameplay, and publication
  evidence independent. Public distribution remains blocked by the recorded
  Unreal/UEPseudo, SQLCipher source-build provenance, and derived-catalog or
  coordinate-table rights reviews.

## 0.6.0-dev67-release-candidate4

- Replace the generic area-quest settle fallback for exact task-actor and exact
  quest-event evidence with a fixed per-task exact-ID verification window. An
  exact witness lasts ten seconds, probes at 750 ms intervals, and permits at
  most one reflected state query across all witnessed tasks per engine tick.
  Final `END` proves the witnessed task complete even when the preceding
  published state was `NONE`, `FAIL`, `ACCEPTABLE`, or `PROGRESS`; `NONE` by
  itself is not completion evidence. Repeated events neither extend the window
  nor duplicate the fixed 147-entry queue. After completion, `NONE` or `END`
  remains only the inactive boundary for repeatable-task reactivation.
- Replace the ambiguous component-target treasure path with the exact
  `DsAnimationProp.NetMultiExecuteInteractProp` receiver Actor and require its
  Actor parameter to equal the freshly resolved local Pawn. Resolve the
  receiver's reported `ObjectID` through exact class and 3D validation first,
  then use activation-matched weak identity or bounded unique-class 3D fallback.
  `SetDeathProcess` remains an exact-receiver fallback. All work is event-only;
  no timer, polling, enumeration, SQL, queue growth, or frame work is added.
- Fix the expanded-map first-open blank after a runtime state delta. Rebuild in
  the current session only when the exact retained layer/attachment is visibly
  open. A retained but hidden layer is detached and deferred until the next
  real `SetWorldMapImage` event. That event may rearm the bounded three-attempt
  readiness budget once for the matching candidate serial, including a prior
  three-attempt `map_id` probe that exhausted its budget before the renderer
  could publish a retryable failure. Duplicate same-layer delivery cannot
  replenish an exhausted or completed budget after that one event rearm.
- Resolve Boss/Assault candidates from the current live actor position before
  applying the 100-metre player bound, instead of prefiltering by the immutable
  catalog coordinate. The existing 250 ms service walks the fixed 49 weak
  slots round-robin with a hard limit of eight actor-position queries per
  control tick. Defeat confirmation continues to use the last live actor
  position and adds no enumeration, dynamic queue, or 16 ms motion-path work.
- Keep the binary release default at `event_log_enabled=false` while the
  installed dev67 gameplay-test copy is explicitly enabled. Source, tests,
  static gates, build, package, installer, deployment, gameplay, and performance
  evidence remain separate; fresh dev67 gameplay and frametime acceptance are
  pending.

## 0.6.0-dev66-release-candidate3

- Resolve `DInteractableComponent.Server_RunInteractV2` from the exact receiver
  component owner before consulting the intermittently populated
  `ExecuteTargetObject` and `ExecuteTargetComponent` compatibility fields. This
  preserves the interacted Actor identity when two or three treasures are
  inside the same eight-metre correlation radius instead of dropping the event
  as an ambiguous catalog-only match.
- Reuse an existing activation/epoch-matched weak Actor observation before
  rematching the catalog. When a new match is required, retain the six-metre XY
  and Z bounds but rank exact-class candidates by squared 3D distance. Same-class
  adjacent and XY-overlapping treasures therefore retain distinct save IDs.
- Keep the catalog-only unique-nearby path fail closed when no exact component
  Actor can be recovered. The repair runs only on a real interaction event and
  adds no timer, polling, UObject enumeration, SQL work, dynamic queue, or
  per-frame rendering cost.
- Add regression coverage for receiver-owner-first routing, bounded event-only
  work, adjacent same-class treasure IDs, and XY-overlapping 3D catalog matches.
  Gameplay acceptance still requires opening a real clustered set without an
  F8/F7 reconciliation.

## 0.6.0-dev65-release-candidate2

- Split Boss/Assault disappearance from the treasure two-sample fallback.
  An encounter must now remain continuously visible for at least one second,
  then remain absent for at least ten seconds and forty 250 ms discovery
  samples while the player stays within 100 metres of its last live actor
  position. Reappearance resets missing evidence. Travel, activity suppression,
  a visible mouse cursor, or leaving the radius clears the entire confirmation
  gate instead of writing a two-hour cooldown.
- Bind and refresh encounter observations with the actor's live position rather
  than the static catalog coordinate. Treat `RemovedFromWorld` as streaming
  removal, never as a defeat. Only exact `Destroyed` EndPlay may use the strict
  immediate path. Eviction clears the fixed-slot processed identity so the same
  still-live actor can be observed again after the player returns. Completion
  rechecks the current time condition and cooldown immediately before writing
  state, and a rejected race also clears the processed identity. Entering a
  confirmed suppressed activity clears armed encounter evidence and every
  processed identity immediately at the activity edge, even if Pawn position
  sampling has already failed; this does not wait for the 250 ms probe.
- Gate encounter candidate resolution on current numeric availability before
  reflected actor work. This prevents cooling-down placeholders from producing
  repeated observations and rejection logs without adding a scan or timer.
- Make treasure completion return an explicit idempotence result. Duplicate
  interaction and disappearance reports now erase stale observation state
  silently instead of formatting and writing repeated completion diagnostics.
- Add a strict startup-only `config/diagnostics.ini` switch. The release default
  is `event_log_enabled=false`; missing, oversized, duplicate, unknown, or
  malformed input also disables logging. The disabled path avoids event
  formatting, mutex acquisition, directory creation, rollover, and file I/O.
  Developer deployment can explicitly preserve, enable, or disable the switch.
- Add a standalone end-user `Install.cmd` and fail-closed PowerShell installer
  to the binary archive. It locates the Steam App 4570720 installation, refuses
  to run while the game is active, verifies the exact payload manifest and
  hashes, preserves live visibility and diagnostics settings, updates only the
  two radar `mods.txt` entries without normalizing unrelated content, and rolls
  back the exact prior native target and enablement state on failure.
- Keep gameplay acceptance separate from core tests, static gates, the pinned
  native build, archive verification, and deployment. In particular, the new
  conservative encounter fallback and strict Destroyed path require fresh
  runtime evidence before public release acceptance.

## 0.6.0-dev64-release-candidate1

- Replace the compact dungeon-return timing window with an exact
  `DLayerMiniMap` UObject-create weak mailbox. A newly created valid layer may
  rearm one bounded attachment budget and is passed directly into the renderer;
  a distinct replacement identity rearms once even if the old renderer still
  reports attached or menu-suppressed, while duplicate identity delivery cannot
  form a retry loop. Travel defers the weak mailbox until exact new-world
  validation instead of clearing a candidate created just before transition
  pre.
- Add one immutable required-runtime latch for validated catalogs, compact and
  world-map layer classes, fixed encounter-name keys, the UObject-create
  listener, and all five required native lifecycle callbacks. Initialization
  failure cannot be bypassed by F7, travel, or automatic fault recovery.
- Remove every production Boss/Assault `FindAllOf` path. The UObject creation
  listener now owns one weak slot per immutable encounter catalog entry, and the
  existing 250 ms service resolves only slots inside the 100-metre catalog
  radius. Travel clears the fixed 49-slot cache while F8/F7 preserves valid
  same-world candidates. No dynamic queue or 16 ms encounter work is added.
- Generalize strict exact-class `Destroyed` EndPlay recovery from Assault-only
  handling to both encounter types while preserving unique-class, 100-metre,
  disappearance-confirmation, and two-hour cooldown gates.
- Contain every SQLite C callback behind a C++ exception boundary and fail the
  one-shot reconciliation closed above 4,096 treasure rows or 65,536
  encounter/dynamic-completion rows.
- Detect native log stream failure after open, write, and flush. Disable only
  logging on failure instead of repeatedly using a permanently failed stream.
- Keep compact/world-map state diagnostics in the existing bounded 16-line
  batch instead of synchronously flushing them from the game thread. Critical
  lifecycle and fault-class evidence still flushes immediately.
- Bypass both layer-mailbox mutexes with an acquire-only pending fast path when
  no UObject-create event exists. The locked exchange remains authoritative,
  removing two no-event game-tick lock/unlock pairs without weakening delivery.
- Retire the external diagnostic shared mapping and disconnect PostRender/late
  Present canaries from production. Their historical source remains uncompiled
  for audit purposes, but no canary source or configuration enters the binary
  runtime payload. The shipped renderer is native UMG only.
- Harden clean builds, staging, deployment, source-bound receipts, source-file
  snapshots, path/reparse-point validation, and `mods.txt` encoding preservation
  without adding runtime work. Deployment performs three stopped-game checks
  before one mutation transaction, enters no late process check, and permits
  backup/install-stage writes only under exact non-reparse project runtime
  directories.
- Snapshot and restore the native build process environment through an ordinal
  dictionary and the process environment API, so case-distinct Windows entries
  such as `PATH` and `Path` cannot abort a clean release build.
- Preserve compiler/linker error lines separately from the bounded native-build
  output tail, so verbose include tracing cannot hide the actionable failure.
- Isolate compact candidate UObject validation in the same destructor-free SEH
  wrapper pattern as expanded-map capture. The mailbox owner retains ordinary
  C++ lock/log lifetimes without mixing C++ unwinding and `__try` in one frame.
- Record dev60 as the latest historical diagnostic evidence. It reproduced the
  old compact `state=5` failure and 22-32 ms encounter enumeration; dev64
  gameplay and performance acceptance remain pending.
- Keep public publication blocked pending Unreal/`UEPseudo` rights and
  compatibility review, exact SQLCipher source/build provenance, and
  redistribution/provenance review for derived catalogs and coordinates.

## 0.6.0-dev63-release-audit1

- Add one process-lifetime automatic recovery budget for SEH faults in the
  engine-tick callback. The fault path disables immediately; the next tick
  performs guarded cleanup and may reactivate only when the radar was active.
  The budget is consumed before cleanup, and a second fault, inactive fault, or
  failed recovery stops with `ENGINE_TICK_RECOVERY_STOPPED` instead of entering
  a retry loop.
- Permit a prior runtime-only expanded-map renderer fault to recover on a later
  explicit activation after clean detach, no same-call fault, and valid ABI.
  Treat that state and a faulted area-quest scanner as bounded F7
  activation work. Same-call faults and ABI failures remain terminal.
- Permit the same runtime-only recovery for the visibility Hub only on a later
  explicit F6 open after clean detach and valid ABI. The closed Hub still adds
  no UObject, file, or input-service work.
- Replace per-event log open/close with one persistent process-session stream.
  Batch ordinary events up to 16 lines, immediately flush critical lifecycle,
  state, and fault-class events, cap the current log at 1 MiB, and retain exactly
  one previous log. This removes repeated file-open overhead but is not a claim
  that diagnostics are free.
- Add release hygiene and deterministic staging for separate binary and source
  archives. Re-extract and validate exact manifests/hashes, require the exact
  ten generated catalogs and license/notices, reject stale versions and scratch
  artifacts, and keep runtime generation out of the binary package.
- Bind every packaged runtime file directly to its allowlisted source hash in
  addition to validating the package manifest. Reject duplicate, escaped,
  missing, extra, or jointly modified payload/manifest entries.
- Make the pinned native build tolerate non-fatal upstream stderr while still
  enforcing exit codes, reuse a complete cache without network updates, and
  validate every reused FetchContent origin, commit, and worktree. The only
  allowed dirty state is the exact UE4SS `fmt` check-macro patch. Require an
  explicit `IconFontCppHeaders` source fixed at commit `210b5a3` instead of the
  upstream floating `main` branch.
- If a bounded native-log rollover is blocked by another Windows file handle,
  truncate the current file or disable logging instead of allowing growth past
  the configured bound. If guarded engine-fault cleanup itself faults, record
  a terminal cleanup failure on the next tick without repeating cleanup.
- Add end-user install, update, uninstall, control, compatibility, log, license,
  and source-build guidance. Static/build/package gates pass independently of
  gameplay; dev48-dev63 runtime and performance acceptance remain pending.
- Validate top-level dependency origins plus every nested PolyHook/Zydis
  gitlink, use `--clean-first` for a complete graph-owned rebuild, and emit a
  build receipt that binds the DLL to compiled-source, dependency-lock, build
  script, helper, configuration, and toolchain hashes. Staging and deployment
  reject a missing or stale receipt.
- Preserve the existing `mods.txt` encoding, BOM, line ending, and untouched
  lines while changing only the two radar entries. Commit the update with a
  same-directory atomic file replacement, and fail closed on mixed line endings
  instead of normalizing unrelated user content.
- Record the exact native dependency graph and current public-release clearance
  boundary. The binary candidate is technically testable, but public release
  remains blocked pending UEPseudo/Unreal licensing compatibility and exact
  SQLCipher native-source/build provenance review.

## 0.6.0-dev62-activity-compact-lifecycle-fix1

- Fix the compact radar becoming permanently `Faulted` after returning from a
  dungeon. A confirmed non-open-world activity edge now detaches the compact
  host and clears its world-local weak handles instead of retaining the old
  widget tree as merely `Collapsed`.
- Rearm a fresh compact activation when the open-world identity returns. The
  next normal valid-position update attaches through the current Controller;
  no old-world host receives an uncollapse call.
- Permit one bounded activation to recover a prior runtime-only compact-renderer
  fault after guarded detach has cleared all weak handles. A new detach fault in
  the same call and every ABI validation failure remain terminal.
- Keep ordinary menu/cursor suppression as `Collapsed`. Dungeon enter/exit work
  remains edge-only and adds no recurring lookup, polling, allocation, or
  per-frame cost.

## 0.6.0-dev61-hub-textscale1

- Replace the long F6 column headings with `RADAR` and `MAP`.
- Add explicit role-specific text multipliers on top of the shared viewport/DPI
  scale: title, column labels, row labels, unavailable marker, and close glyph
  no longer inherit one oversized default-font proportion. The top-left pivot
  remains fixed, preventing displacement while scaling.
- Keep panel geometry, masks, auto-apply, input ownership, and the open-only
  service unchanged. The fix adds no recurring work.

## 0.6.0-dev60-hub-dpifix1

- Fix the F6 Hub DPI mismatch exposed at high resolution: every TextBlock now
  uses the same open-time viewport/DPI scale as its slot geometry and a fixed
  top-left render pivot. Title, column labels, row labels, the unavailable
  marker, and `X` therefore remain proportional to the panel.
- Separate the title bar from the minimap/world subheader, compact the six row
  grid, and retune linear-space colors darker so the frame, surfaces, toggles,
  and text no longer wash into one oversized grey block.
- ABI-gate both reflected text-transform functions. The fix remains bounded to
  explicit F6 construction and adds no open-service or closed-state work.

## 0.6.0-dev59-hub-style2

- Refine the F6 Hub into a framed layered card with a restrained cyan accent,
  separate minimap/world header chips, a darker content surface, consistent
  alternating rows, clearer toggle frames, and a distinct close control.
- Preserve the 620-by-390 reference layout, aspect-ratio-aware viewport scale,
  UMG DPI conversion, and exact centering. The style creates widgets only when
  F6 opens and adds no closed-state, renderer, SQL, or discovery work.
- Keep dev58 expanded-map layering and first-invalid-position scene-handoff
  suppression unchanged. Runtime visual acceptance remains pending.

## 0.6.0-dev58-layer-transition1

- Place both expanded-map radar atlas hosts above game-native map icons at
  adjacent deterministic Z orders. The lower radar atlas still draws treasure
  after area quests and mini-games, while the upper radar atlas remains Boss
  and Assault. Zoom invalidation can no longer change which radar categories
  appear behind native icons.
- Collapse the independent compact UMG host on the first failed current-Pawn
  position sample. Cursor state is read from the already reacquired current
  Controller before Pawn resolution, so menu suppression remains current even
  during scene handoff. This reuses the existing 16 ms scalar sample and adds
  no timer, lookup loop, enumeration, or retained UObject.
- Keep dev57 Hub, quest, encounter, atlas construction, and attachment cadence
  unchanged. Static, build, deployment, and gameplay acceptance remain
  separate; dev58 runtime evidence is pending.

## 0.6.0-dev57-hub-input-style1

- Replace the large Apply/Cancel Hub layout with a compact layered panel,
  aligned minimap/world columns, alternating row bands, and one top-right `X`.
  Each real checkbox transition publishes both masks and atomically persists
  once without closing; unchanged 50 ms samples do not write or rebuild.
- Establish Game-and-UI input before showing the cursor. During the existing
  open-only service, read the cursor bit and repeat the input/cursor transaction
  only if gameplay hid it. The closed path still returns before Controller or
  UObject work, and `X`, F6, F8, travel, and shutdown restore input through the
  panel-owning Controller.
- Keep dev56 quest, encounter, renderer, and lifecycle behavior unchanged.
  Static, build, deployment, and gameplay acceptance remain separate; dev57
  runtime evidence is pending.

## 0.6.0-dev56-quest-event-hub1

- Register one schema-validated native `ETSendQuestEventTrigger` hook. A
  dynamic catalog task receives a ten-second numeric witness only when an exact
  live query at its own event proves `PROGRESS`, with the last published
  `PROGRESS` used only if that exact query is unavailable. Witness deadlines and
  one-follow-up budgets are per task, repeated step events cannot replenish
  them, and generic treasure/workstation interaction never arms area tasks.
  This recognizes delivery/workstation completion that reaches `END` after a
  stale intermediate snapshot while keeping every unwitnessed `END` fail closed.
  The witness is pure numeric state and now survives travel until its original
  deadline; F8/F7 still starts a clean activation and clears it.
- Accept the current data's `MONSTER_ALIVE value1=0` shape only through a
  one-time positional join with exactly one Assault inside 150 metres. Zero,
  multiple, or future nonzero unknown candidates remain hidden. This restores
  the time-gated Assault companion task before local trigger discovery. The
  first valid clock sample after F7 now coalesces the same bounded task refresh
  as a later displayed-hour edge.
- Add an F6 native UMG visibility Hub with independent compact and expanded-map
  switches for clock, treasure, Boss, Assault, mini-games, and area quests.
  Apply publishes both masks together and atomically persists them once;
  Cancel changes nothing. The closed Hub performs no UObject work, deployment
  preserves user settings, temporary `FText` inputs are explicitly destroyed,
  key repeat is debounced, and travel removes the panel before old-world UMG
  can become stale. Input and cursor restoration always target the Controller
  that owns the panel, even if the game's current Controller changed silently.
- Permit only soft-not-ready compact/world UMG attachment failures to retry,
  at most three times with a fixed delay. Success or the third failure stops;
  there is no sleep or steady retry loop. Repeated delivery of the same live
  world-map layer cannot reset any completed or exhausted service budget.
- Preserve dev55's compact area-task contrast. Core, static, build, deployment,
  and gameplay acceptance remain separate; dev56 runtime evidence is pending.

## 0.6.0-dev55-task-icon-clarity1

- Replace the compact area-task marker's transparent center and black dots with
  a 55-percent translucent charcoal RoundedBox fill and three solid-white dots.
  Keep the existing thick dark frame, 28-pixel reference size, and layer order.
- Reuse the same four preallocated UMG pieces and ABI-gated Brush. Add no widget,
  allocation, query, scan, timer, atlas work, layout mutation, or update cadence.
  All dev54 state, lifecycle, completion, and performance behavior is unchanged;
  dev55 gameplay appearance remains pending.

## 0.6.0-dev54-task-id-completion1

- Reflect the current game instance's
  `TaskActorClassContainer.DynamicQuestTaskList` once after F7 activation or
  completed travel. Join dynamic `CreateTaskClass` full names through their
  unique `UseQuestList` IDs to the validated 147-entry area-quest catalog.
- Enable direct completion identity only when all 147 catalog IDs have exact
  one-to-one class coverage with zero ambiguous classes or duplicate catalog
  bindings. Ignore task classes used only by rows outside the 147-entry catalog;
  treat a class shared by catalog and non-catalog IDs as ambiguous. Retain only
  copied strings and catalog indices, and fail closed on missing, invalid,
  ambiguous, incomplete, duplicate, or unmapped identities.
- Resolve `ADETTaskBaseActor.OnRecvCompleteQuest` from its current task-actor
  class, publish one fixed atomic catalog bit, and hide that exact task on the
  next game tick. Update activation-local dynamic-prerequisite evidence at the
  same boundary.
- Fence each transactional scan with fixed per-task completion revisions. A scan
  that began before an exact completion can no longer publish stale
  `ACCEPTABLE`/`PROGRESS` over that completion. Keep the completion latched even
  for later current-revision active samples until a current scan first observes
  `NONE` or `END`; only a subsequent current-revision `ACCEPTABLE`/`PROGRESS`
  sample may rearm a genuinely repeatable task. `FAIL` does not arm reactivation.
- Bound task-class mapping to one post-stability attempt and one delayed retry
  after failure. Stop after success or the second failure. An already-active
  explicit F7 rearms only failed mapping capture and preserves completion bits,
  revisions, and reactivation latches; completed travel may start a fresh
  bounded capture. No steady-state retry or polling path is added.
- Preserve one generic debounced transactional refresh after every real task
  completion for settled-state evidence and the two-scan repeatable reactivation
  sequence. Generic teardown continues to accept only `PROGRESS -> END`; `FAIL`
  remains non-proof for both completion and reactivation.
- Make asynchronous F7 save-result publication merge-only for positive dynamic
  quest completions. An older one-shot snapshot can no longer clear exact or
  generic native completion IDs that arrived after the snapshot was taken;
  activation lifecycle reset remains the only full clear. If the result arrives
  after expanded-map attachment, retire that pre-reconcile atlas and defer its
  replacement until the next real map session. Add no retry, query, timer, or
  polling work.
- Drain pending exact-completion bits at `InitGameState` pre-transition before
  clearing task-class mapping and atomic publication state. The drain converts
  only an already resolved catalog bit into activation-local numeric completion
  and prerequisite evidence, so a same-frame completion/travel edge cannot lose
  the task. It performs no UObject access or retention; F8 followed by F7 still
  creates a fresh resynchronization.
- Invalidate a retained expanded atlas after an exact or settled task-state
  change, but defer its rebuild to the next real map session instead of the
  next 16 ms gameplay sample. This explicitly includes eligibility changes from
  the displayed world-hour refresh path.
- Make treasure and linked mini-game runtime deltas idempotent. A treasure-open
  event dirties compact selection only when an eligibility entry actually moves
  from visible to hidden. A changed map-100 treasure or mini-game entry retires
  an attached atlas, clears pending attachment, and rebuilds only in the next
  real expanded-map session, preventing same-live-layer reuse of stale pixels.
  Duplicate events trigger no additional dirty state, atlas invalidation, or
  rebuild, and return before scanning either generated catalog.
- Preserve the bounded 49-entry Boss/Assault cooldown map across F8/F7 and merge
  one-shot save timestamps by maximum. A stale save snapshot can no longer
  revive a just-defeated encounter before the game's next save write.
- Add one rate-limited 1 Hz scalar edge check to the existing 250 ms control
  service. Only the earliest two-hour cooldown expiry or a displayed world-hour
  change recomputes the fixed 49-bit encounter visibility mask. A real direct
  encounter or linked-task visibility change dirties compact selection and
  retires an attached atlas for the next real map session; no 49-entry work or
  atlas rebuild runs on the 16 ms motion path.
- Correct lifecycle documentation: normal uninstall unregisters hooks and
  listeners, while UObject-array shutdown removes the create listener and makes
  process-pinned callbacks inert instead of invoking unsafe late hook removal.
- Preserve dev53's removal of treasure-class `FindAllOf` catch-up. Add no
  recurring SQL, timer, UObject enumeration, retained UObject, growing queue,
  widget, or steady-state render work. Runtime evidence remains capped at dev47
  and dev54 gameplay acceptance is pending.

## 0.6.0-dev53-task-completion-reliability1

- Preserve the published state snapshot at an ID-less completion event and
  accept the later confirmation only when exactly one new `END` candidate is
  found. Cap callback coalescing at two seconds and reject zero or multiple
  candidates.
- Remove treasure-class game-thread `FindAllOf` catch-up; treasure interaction
  and lifecycle evidence remain authoritative. Boss/Assault exact-class
  activation catch-up remains a measured hitch candidate.
- Keep fixed staging, lifecycle resets, one-ID-per-frame scanning, and no
  retained task UObject. This inference path is superseded by dev54's direct
  task-class-to-catalog-ID map.

## 0.6.0-dev52-exact-task-completion1

- Hook the game-owned `ADETTaskBaseActor.OnRecvCompleteQuest` completion
  boundary and merge it into the existing one-second quest-event debounce.
- Allow only that exact completion-confirmation scan to latch a newly observed
  `END` when a short task completed before the previous snapshot saw
  `PROGRESS`. Generic teardown still cannot treat `FAIL` or `NONE` as
  completion.
- Keep the existing one-ID-per-frame transactional scan, fixed staging arrays,
  lifecycle resets, and F7 save reconciliation. Add no recurring timer, SQL
  query, UObject enumeration, retained task object, queue, or steady-state
  render work.

## 0.6.0-dev51-clock-clarity1

- Thicken the four existing seven-segment digits and colon, use full-opacity
  white, and move the complete transparent clock group eight reference pixels
  farther below the compact map for clearer separation.
- Replace the night phase's nested diamonds with a recognizable pale-blue open
  crescent and warm two-stroke star. Reuse the existing ten preallocated phase
  pieces; no widget, timer, scan, query, allocation, or steady-state update is
  added.
- Preserve dev50's hour-edge transactional task refresh, every marker path,
  coordinate, lifecycle rule, and render cadence unchanged.

## 0.6.0-dev50-time-task-refresh1

- Queue one transactional dynamic-task state scan after each displayed game-hour
  edge. This lets time-activated area tasks adopt the game's authoritative
  `ACCEPTABLE` or `PROGRESS` state when their paired Assault becomes available.
- Coalesce the hour-edge request in one activation-local boolean while another
  scan is running. The existing schedule remains one reflected task read per
  game frame, with no recurring SQL, UObject enumeration, timer, allocation,
  retry loop, or accumulating queue.
- Preserve the dev49 `PROGRESS`-to-`END` completion latch, all visual geometry,
  encounter state, and render cadences. The unproven 150-metre static
  `MONSTER_ALIVE` proximity link remains fail closed instead of being widened
  into a potentially incorrect association.

## 0.6.0-dev49-task-fail-latch1

- Stop treating `PROGRESS` to `FAIL` as an area-task completion event. Current
  runtime evidence shows the generic quest-blueprint end callback can expose
  that transition immediately after opening an unrelated treasure chest.
- Keep `PROGRESS` to `END` as the only activation-local completion proof. The
  existing F7 completed-save snapshot remains authoritative, and current
  `ACCEPTABLE` or `PROGRESS` still revives repeatable tasks.
- Add a production-used regression helper and tests proving `FAIL` cannot set
  the completion latch. Preserve dev48 visuals and every scan/render cadence;
  no recurring query, timer, allocation, or per-frame work is added.

## 0.6.0-dev48-icon-clarity1

- Increase only the expanded-map Boss reference size from 42 to 44 and advance
  the one-shot atlas style revision so an older cached foreground atlas cannot
  retain the previous geometry.
- Keep the compact area-task center transparent, increase its ABI-gated rounded
  black outline from 2.25 to 2.75 reference pixels, and change the three ivory
  0.13-square dots to black 0.16-square dots for clearer compact-map contrast.
- Preserve dev47 task-refresh state, every marker coordinate and layer, the
  fixed widget/atlas counts, and all update cadences. The change adds no scan,
  query, widget, timer, allocation, or steady-state render work.

## 0.6.0-dev47-transactional-task-refresh1

- Coalesce bursts from the generic quest-blueprint end callbacks for one
  second, then read all 147 dynamic task states and deduplicated prerequisites
  into private numeric staging arrays. Publish only the complete transaction,
  so neither compact nor expanded task markers blank during a refresh.
- Stop treating an uncorrelated terminal sample as completion proof. The game
  emits the same callback while adjacent mini-games tear down; only the F7 save
  snapshot or an already `PROGRESS` task transitioning to terminal may hide an
  otherwise proven unfinished task. A refresh fault preserves the previous
  complete snapshot and still fails closed when no snapshot exists.
- Avoid compact rebinds and expanded-atlas invalidation when a completed
  refresh produces the same task mask. All work remains event-driven with one
  reflected query per game frame and no idle polling.
- Increase the six-piece nearest-treasure height pointer by about eight percent
  and reduce its chest clearance from four to 2.5 reference pixels. Its 16 ms
  path remains one group translation and one rotation.

## 0.6.0-dev46-layered-monster-tasks1

- Support the 22 current `MONSTER_ALIVE` area-task conditions by linking each
  numeric task coordinate once to the unique nearest Assault within 150 metres.
  Reuse the existing Assault cooldown and world-time availability state;
  defeat changes refresh immediately and timed availability refreshes only on
  a game-hour edge. No recurring UObject scan, SQL query, or new timer is added.
- Split the one-shot expanded-map composition into two co-located 2048 atlases.
  Boss and Assault use a foreground host above game-native icons. Treasure,
  area tasks, Fly, Mole, and Wave use a background host below native icons,
  while treasure remains above other background radar glyphs.
- Extend exact graph validation, F8 collapse, F7 resume, rollback, travel
  removal, and weak-handle cleanup to both atlas hosts as one fail-closed
  transaction. Cache fingerprints remain process-local and layer-specific.
- Give compact area tasks a true black frame, transparent center, and three
  existing dots through one ABI-gated rounded-box `FSlateBrush`. No additional
  widget or recurring render work is introduced.

## 0.6.0-dev45-worldmap-edge-smoothing1

- Route every production expanded-map rectangle, diamond, and polygon through
  the existing bounded 4-by-4 coverage rasterizer. This removes mixed hard-pixel
  and antialiased edges from the single atlas without increasing its sample
  rate or adding any recurring work.
- Increase only the expanded-map Boss reference size from 40 to 42 and reduce
  Fly, Mole, and Wave uniformly from 34 to 32. Compact-map artwork and every
  visibility, lifecycle, coordinate, and layering rule remain unchanged.
- Advance the atlas style revision so a dev44 cache cannot preserve the old
  hard-edged geometry. The change remains event-only during an explicit atlas
  build and adds no timer, scan, SQL query, retained UObject, or per-frame UMG
  mutation.

## 0.6.0-dev44-area-quest-eligibility1

- Replace global visibility for every unfinished dynamic task with a
  fail-closed qualification snapshot. F7 reaches the already loaded
  `DDynamicQuestDataTable` through
  `DGameSingleton.GameDBTableManager.GameDBArray`, copies numeric Main/Group
  fields for exactly 147 catalog IDs under SEH, and retains no table UObject.
- Preserve current runtime `ACCEPTABLE` and `PROGRESS` as authoritative. A
  `NONE` record now requires positively proven supported conditions: `NONE`,
  ordinary `QUEST_CLEAR`, or saved `DYNAMIC_QUEST_COMPLETE`. Unsupported
  conditions, missing rows, query faults, and unresolved weighted selections
  remain hidden.
- Keep the event-driven performance contract. The 147 current-state calls and
  deduplicated ordinary prerequisites run at one reflected query per game
  frame only after F7, travel, or quest-end; there is no idle timer, recurring
  object search, additional SQL query, atlas polling, or retained UObject.
- Retain dev43's accepted one-shot Fly/Mole/Wave atlas geometry unchanged.
  Runtime acceptance must confirm the definition coverage, proof counts, and
  expected visible task set from the new diagnostic events.

## 0.6.0-dev43-minigame-atlas-visuals1

- Replace only the expanded-map mini-game block approximations with the
  accepted external renderer's 34-reference-pixel geometry. Fly restores the
  complete winged arrow, Mole restores the broad tapered hammer, and Wave
  restores four curved strands sampled from the original cubic paths.
- Restore the accepted dark outline and drop shadow, and use bounded 4-by-4
  edge coverage while the one-shot 2048 atlas is rasterized. This improves
  scaled edge clarity without adding a timer, scan, SQL query, widget, or
  steady-state render mutation.
- Preserve compact-map artwork and sizing, all save/runtime visibility rules,
  the fixed 1,785-entry snapshot, single-image atlas ownership, marker layering,
  F7/F8 lifecycle, travel handling, and cache fingerprint invalidation.

## 0.6.0-dev42-minigame-worldmap1

- Restore all 83 generated open-world mini-game records to the native expanded
  atlas: 33 Fly, 40 Mole, and 10 Wave. The existing one-shot F7 save snapshot
  remains the eligibility authority, so completed reward SaveIds stay hidden.
- Increase only the fixed numeric expanded-map snapshot from 1,702 to 1,785
  entries. Mini-game rows are appended during an explicit atlas build and add
  no recurring timer, UObject lookup, SQL query, widget pool, or per-frame UMG
  mutation.
- Add code-native Fly, Mole, and Wave raster glyphs using the same blue-wing,
  brown-hammer, and blue-wave palette as the compact renderer. They remain
  below treasure chests in the single atlas composition.
- Preserve the dev41 single compact Blueprint host unchanged. Compact Boss is
  intentionally still a bounded four-Border approximation of the expanded
  raster silhouette; exact pixel identity would require a different texture or
  additional widget path and is not introduced without runtime evidence.
- Keep area-quest claims fail-closed: runtime `ACCEPTABLE` and `PROGRESS` are
  authoritative, but unfinished `NONE` records remain candidates because the
  catalog still has no main-story prerequisite graph.

## 0.6.0-dev41-single-host-visuals1

- Reject the dev40 split compact encounter tree after runtime evidence showed
  the renderer entering `Faulted` on the first selection containing two
  Assault markers. Restore the dev35-dev37 single Blueprint host and single
  moving Canvas for all compact marker kinds.
- Preserve the dev39 exact-class create queue, EndPlay/disappearance evidence,
  idempotent two-hour cooldown, and compact/expanded invalidation behavior.
  This change affects presentation ownership only.
- Return the accepted 16 ms path from two root translations to one and remove
  the second Canvas, 64 encounter Borders and slots, their weak handles, and
  their suppression and cleanup calls.
- Change the compact area-quest backing from opaque black to 52-percent dark
  charcoal. Change the expanded-map inner dialogue diamond to the matching
  translucent treatment and increment the atlas style fingerprint so a cached
  opaque atlas cannot survive the update.
- Runtime gameplay acceptance remains required for compact Boss/Assault
  visibility, Assault removal, and both area-quest surfaces.

## 0.6.0-dev40-compact-coordinate-repair1

- Replace the dev39 compact encounter attachment to the moving external
  `PlayerIconWidget` parent with one fixed Canvas centered inside the live
  player widget's own root Canvas. The pool remains below the authored arrow at
  child Z `-1`, follows the player widget's final placement automatically, and
  retains the same fixed eight-marker allocation and second scalar translation.
- Reuse the one-shot unfinished-task mask for nearby compact area quests. This
  restores small-map task markers without recurring SQL or UObject work. The
  catalog still lacks prerequisite data, so unfinished candidates are not
  claimed to be currently triggerable; terminal/runtime completion continues
  to hide them.
- Tighten the transparent clock group from 138 to 104 reference pixels and move
  the fixed day-phase dial from X 118 to X 86. Digits and phase icon remain
  centered as one group and retain minute/phase-edge-only mutation.
- Preserve the dev39 streamed encounter create queue and confirmed two-missing-
  sample completion path. Runtime dev39 evidence recorded automatic removal for
  Boss 9000012 and Assault 114 and 156; this version changes only compact
  presentation coordinates and task selection.

## 0.6.0-dev39-world-quest-layering1

- Replace the incorrect `tb_dynamic_quest_group` world-coverage assumption
  with one optional single-user
  `tb_dynamic_quest_complete(QUEST_ID, COMPLETE_CNT)` query inside the existing
  F7 snapshot. The expanded map starts from all 147 catalog tasks and hides
  positive completed rows. Current `ACCEPTABLE` or `PROGRESS` overrides
  history for repeatable availability; `END`, `FAIL`, or a completion
  observed in this activation hides the task. Compact eligibility remains
  strictly local.
- Split compact composition around the real player arrow. Boss and Assault use
  a fixed eight-marker, eight-piece Canvas attached to the
  `PlayerIconWidget` Canvas parent at player Z minus one. Treasure, area
  quests, mini-games, height, and clock remain in the existing high-Z
  foreground. Both roots receive the same scalar translation and no marker
  child layout is mutated per sample.
- Change compact Boss and Assault to dedicated 30/27-pixel variants of the
  expanded-map white, deep-green, and cyan glyph language.
- Extend the existing UObject create listener to recognize only newly created
  Actors whose exact class is one of the immutable 49 encounter classes. It
  sends only weak identities through a fixed 64-entry queue to the normal
  observation path, closing the Assault stream-in gap after a zero-result F7
  catch-up without repeated `FindAllOf`, polling, or dynamic queue growth.
- Add truth-table, SQL-source, fixed-pool, split-layer, ABI, two-translation,
  listener, no-enumeration, and lifecycle gates. Core tests pass; native build
  and all gameplay acceptance remain pending.

## 0.6.0-dev38-world-quest-visuals1

- Preserve compact area-quest filtering as strict runtime state:
  `ACCEPTABLE` and `PROGRESS` render, while `NONE`, `END`, `FAIL`,
  unavailable calls, and structured faults remain hidden.
- Add global expanded-map eligibility from each single-user save snapshot's
  current `tb_dynamic_quest_group.QUEST_ID`. The query is optional,
  fail-isolated from treasure and encounter reconciliation, uses only the
  newest successful source, and rejects a source containing more than one
  `USER_DBID`. A mixed-user newest readable source is terminal for optional
  task augmentation rather than allowing an older backup to select another
  character; the selected identity and ambiguity state are logged.
- Do not query or interpret `tb_dynamic_quest_complete.COMPLETE_CNT`.
  Dynamic groups may reset or repeat, so a historical count is not a proven
  permanent-completion boolean. Current `END` or `FAIL` hides a stale saved
  group; a later current `ACCEPTABLE` or `PROGRESS` restores a repeatable
  task.
- Make each save source transactional before merging its treasure and
  encounter rows. A source that fails a mandatory query can no longer leave
  partial rows in a result accepted through another source.
- Match compact Boss and Assault reference sizes to the accepted expanded-map
  sizes of 40 and 34 pixels. Replace the compact Boss center cross with the
  separated white wing treatment while retaining the fixed four-piece pool.
- Replace the gold area-quest exclamation on both maps with a dialogue
  treatment. Compact mode reuses one dark diamond and three ivory dots;
  expanded mode rasterizes one ivory/dark speech bubble with three dots into
  the existing one-shot atlas.
- Align the bounded clock phase icon to the seven-segment digit center by
  moving only its fixed Y center from 21 to 15 reference pixels. No clock
  cadence, widget count, or motion path changes.
- Add production-used visibility truth-table tests and static gates for the
  current group source, single-user rejection, no completion-count query,
  compact/world policy split, exact sizes, dialogue styling, and clock
  alignment. No recurring SQL, object scan, timer, widget, or steady-state
  renderer work is added. Runtime gameplay acceptance remains pending.
- On a structured task-state fault, clear and dirty compact task output and
  invalidate an attached expanded atlas through the existing one-shot path so
  stale task pixels cannot survive the fail-closed state.

## 0.6.0-dev37-assault-destroyed-recovery1

- Correct the unobserved EndPlay recovery introduced in dev36 before its first
  gameplay run. Recovery now applies only to Assault and only to
  `EEndPlayReason::Destroyed`; `RemovedFromWorld` fails closed so World
  Partition or level streaming cannot be treated as a defeat.
- Read the ending actor's actual position while the EndPlay callback is active.
  Require exact class identity, actor-to-catalog, player-to-actor, and
  player-to-catalog 100-metre bounds, a valid weak identity, the current
  activation and epoch, no activity suppression, no duplicate observation, and
  a currently available encounter.
- Make encounter cooldown writes idempotent. A stale or duplicate event cannot
  extend an existing future two-hour cooldown and cannot publish a false
  completion event. Ended objects are no longer resolved or queried from the
  normal probe, and activity suppression clears pending observations.
- Add production-used pure decision gates and native behavior tests for
  streaming removal, wrong position, distance, lifecycle, duplicate
  observation, invalid identity, and future-cooldown rejection. The recovery
  remains event driven and adds no timer, recurring lookup, enumeration, SQL
  work, or combat-time atlas rebuild.
- Retain the unified official-reference compact and expanded Boss/Assault
  palette. Active compact source contains no legacy red/pink encounter color
  path. Runtime gameplay acceptance remains pending.

## 0.6.0-dev36-assault-end-visual-unification1

- Recover an eligible Destroyed or RemovedFromWorld encounter EndPlay when the
  actor was missed by the one-shot catch-up. Recovery requires a load-time
  unique exact encounter class, the current open-world lifecycle token, and the
  existing 100-metre player bound. It contributes only the first missing
  sample; the existing discovery edge must provide the second confirmation
  before the 120-minute cooldown is applied.
- Add no timer, periodic `FindAllOf`, UObject enumeration, retained Actor
  pointer, SQL refresh, or combat-time atlas rebuild. Replace the encounter
  class linear hot-path lookup with one immutable class-to-index table built at
  startup and fail closed if any of the 49 class names is duplicated.
- Add `encounter_type=boss|assault` and an explicit recovered-EndPlay evidence
  label to lifecycle logs. Runtime dev34 evidence now distinguishes the proven
  Boss path from the Assault that was hidden only by the next F7 save snapshot.
- Remove the unused legacy red/pink compact Boss and Assault color constants.
  Require the active compact and expanded renderers to keep the same
  official-reference white, pale, deep-green, and cyan palette. Dev35 had been
  deployed but had no runtime `START`, so dev36 gameplay appearance and Assault
  recovery remain pending.

## 0.6.0-dev35-area-quest-clock-visuals1

- Correct the 147-entry area-quest scan to pass `IsDynamic=true` to
  `GetQuestInfoInStandAlone`. Runtime dev34 evidence showed 147 failed
  ordinary-store queries and zero visible tasks; cadence remains one ID per
  game frame only during an explicit event-driven refresh.
- Remove the full opaque clock backing widget. Restore a transparent
  138-by-42 lower-map status group and add one ten-piece preallocated
  morning/afternoon/evening/night icon. Digits change only on minute edges and
  the icon changes only on phase boundaries.
- Replace the compact red Boss/Assault approximations with white, deep-green,
  pale-green, and cyan official-reference geometry. Simplify the expanded-map
  Boss silhouette and Assault diamond layers while retaining the single atlas,
  fixed marker sizes, existing layering, and zero steady-state map updates.
- Add static regression gates for the dynamic quest store, transparent clock,
  bounded phase icon, and simplified encounter contrast. Runtime gameplay
  acceptance remains pending.

## 0.6.0-dev34-encounter-disappearance1

- Replace the stale six-metre encounter catalog match with exact unique class
  identity inside the already approved 100-metre observation bound. This lets
  one-shot F7 catch-up register a Boss or Assault actor that has moved from its
  catalog origin without adding a scan or increasing discovery cadence.
- On a confirmed encounter disappearance, dirty the compact snapshot
  immediately and invalidate any attached expanded atlas. Defer expanded atlas
  selection, rasterization, file access, texture import, and attachment until
  the next real map session so the combat event adds no atlas-build hitch.
- Add static gates prohibiting the rejected six-metre match and requiring both
  compact refresh and bounded next-session expanded-map invalidation. Runtime
  Boss and Assault disappearance acceptance remains pending.

## 0.6.0-dev33-area-quest-clock-icons1

- Add a strict generated catalog of 147 unique area quests. Query reflected
  standalone quest state only during F7, travel, or quest-end rescans, at one ID
  per game frame. Show only `ACCEPTABLE` and `PROGRESS`; hide completed, failed,
  locked, unavailable, and failed-query records.
- Isolate a structured quest-query fault to the area-quest feature until the
  next explicit lifecycle trigger. The accepted treasure, Boss, Assault, and
  minigame paths remain available, and no retry loop is introduced.
- Restore the native four-digit seven-segment clock as a fixed sibling of the
  moving compact marker Canvas. Reuse the existing scalar game-time baseline
  and mutate segment visibility only when the displayed minute changes.
- Extend compact selection with bounded area-quest candidates and the one-shot
  expanded-map snapshot to 1,702 fixed entries. Area quests use a distinct gold
  exclamation diamond and remain below overlapping treasures.
- Replace oversized expanded-map encounter glyphs with official-reference
  designs: a 40-pixel white Boss wing/crown diamond and a 34-pixel pale-green
  Assault diamond with a cyan exclamation. Keep treasure glyphs at 12 pixels
  and area quests at 26 pixels.
- Add catalog, reflection, scheduling, widget ownership, capacity, layering,
  and deployment regression gates. Runtime gameplay acceptance remains pending.

## 0.6.0-dev32-encounter-visual-parity1

- Scale compact Boss and Assault markers from 46/27.6 to 64.4/38.64
  reference pixels because native compact treasures are 14 rather than the
  stable overlay's 10. Scale expanded-map Boss and Assault markers from the
  temporary 18/15 to 64.8/38.88 because native expanded treasures are 12.
  Both conversions preserve the stable Boss/Assault-to-normal-treasure ratios.
- Replace the temporary expanded-map Boss exclamation and simplified Assault
  mark with the stable visual language: shadowed double-outline warm-red
  diamonds, the field-boss wing/crown silhouette, and gold crossed swords with
  an ivory highlight.
- Rasterize encounters first and treasure chests second inside the same
  one-shot atlas, so treasures deterministically remain above overlapping Boss
  or Assault markers. Compact selection already keeps encounters before
  treasures in the fixed pool and now has an explicit regression gate.
- Add no UObject, host, texture, timer, recurring scan, lookup, file access, or
  steady-state render work.

## 0.6.0-dev31-native-feature-completion1

- Increase only the expanded-map treasure glyph from 10 to 12 reference pixels.
  Compact-map treasure sizing and the accepted 16 ms motion path are unchanged.
- Extend the one-shot expanded-map numeric snapshot from 1,506 treasure slots
  to 1,555 fixed slots: all eligible map-100 treasures plus the already loaded,
  cooldown-filtered 9 Boss and 40 Assault records. Boss and Assault use distinct
  bounded atlas glyphs; no marker UObjects or recurring atlas work are added.
- Make activity suppression own both renderers. Entering a non-open-world
  activity collapses compact drawing and detaches the expanded-map host on the
  existing context-change edge. F7 cannot resume a suspended host while
  suppressed; leaving the activity permits one event-only map-layer catch-up.
- Preserve open-world interiors, transition fail-closed cleanup, exact weak
  runtime identities, fixed three-attempt map readiness, and the single
  2048-by-2048 RLE-TGA atlas.

## 0.6.0-dev30-world-map-brush-abi1

- Record the first dev29 runtime result: the compact renderer and sharp height
  pointer worked, and the expanded-map session selected 305 eligible map-100
  treasures, but the expanded-map renderer started Disabled with ABI mask 512.
  No atlas was built, written, imported, or attached.
- Correct only the `UImage.SetBrushFromTexture` reflected parameter-span gate
  from 16 bytes to 9 bytes. The installed game header exposes exactly a texture
  pointer and one bool; Unreal excludes the local C++ tail padding from
  `UFunction::GetParmsSize()`. Keep the naturally aligned 16-byte call buffer.
- Preserve every dev29 selection, atlas, native-parent, F8/F7, travel, weak
  identity, and no-steady-work invariant. Runtime expanded-map acceptance is
  still required.

## 0.6.0-dev29-full-world-map-atlas1

- Replace the two-host expanded-map canary with one fixed-capacity map-100
  snapshot that linearly visits the loaded 1,693-entry render catalog and
  copies every currently eligible map-100 treasure into the 1,506-entry output
  array. Selection is an explicit map-session transaction with no radius,
  nearest ranking, heap allocation, or steady-state scan.
- Rasterize that snapshot once into a transparent 2048-by-2048 atlas, encode it
  as an RLE true-color TGA cache artifact, import it once as a transient texture,
  and display it through exactly one `UImage` inside one game-native map-icon
  host. The host remains under the exact native Canvas parent, so pan, zoom,
  clipping, and map visibility continue to come from the game.
- Keep atlas construction, cache writing, texture import, and UMG mutation on
  explicit map-session events only. The attached atlas owns no recurring tick,
  timer, polling, lookup, marker-object pool, or per-marker UWidget.
- Change F8 expanded-map handling from removal to `Collapsed` suspension. F7
  restores the same host without rebuilding or importing only after exact
  layer, slot, parent, content, image, and texture identity validation. A failed
  identity check detaches fail closed. Travel, map replacement, and shutdown
  still perform complete removal.
- Strengthen attached-host reuse, F8, and F7 validation with one event-only,
  bounded reflected pass over the current layer's `ArrayIconInfo`. The retained
  native parent is accepted only when a live game map icon still has that parent
  and the same owning player. No array element, icon widget, raw pointer, or
  property offset is retained, and no steady polling is added.
- Add `attach_total_us` for the complete `attach_once` transaction, beginning
  before guarded validation and ending after atlas rasterization, cache reuse or
  writing, texture import, and UMG attachment. Attach-result logs publish the
  measured value; retained-host reuse and F7-resume logs repeat the last complete
  attach value for correlation. Runtime hitch and memory acceptance remain
  pending.
- Preserve the six preallocated compact height-pointer pieces and its 16 ms
  group-only transform path. The two outer square-ended head strokes now begin
  at tangent-derived endpoints so their outer corners meet at one exact tip;
  the tail is inset by three reference pixels. No widget or steady update call
  is added.
- Runtime acceptance remains pending for full eligible-marker coverage, atlas
  appearance and coordinates, same-open-map F8/F7 restoration, attach-time hitch
  and memory envelope, travel, and clean shutdown. Static gates and a clean build
  are not gameplay acceptance.

## 0.6.0-dev28-smooth-height-pointer1

- Place the existing six preallocated nearest-height pieces under one
  preallocated Canvas group. Their connected, capped local geometry is created
  once; scalar updates move and rotate the group instead of rewriting six child
  slots.
- Calculate the height angle on every existing accepted position sample at the
  16 ms gate and remove the additional 33 ms height gate, avoiding the effective
  40-50 ms cadence created by two unsynchronized intervals. Skip both group
  transform calls unless the clamped angle changed by at least 0.15 degrees.
- Make the 100-Unreal-unit height dead zone continuous by feeding only the signed
  excess `abs(delta) - 100` into the angle curve. This removes the roughly
  2.3-degree discontinuity between deltas 100 and 101.
- Worst-case height work per accepted position sample is one group translation
  plus one group rotation with zero scan, allocation, logging, child geometry
  mutation, or layout work. Flat ground and micro-angle changes are epsilon-skipped.
- Stabilize nearest-treasure ownership with a 100-Unreal-unit (100 cm) advantage
  threshold. A retained target that disappears or is opened is immediately
  ineligible, so the next enabled game-thread service tick selects the best
  remaining target without hysteresis.
- Preserve the fixed 80-marker selection pool and its existing 1000-unit
  movement, five-second, radius-change, and real-state-change rebind gates. The
  height-only path does not increase full marker rebinding frequency.
- No dev27 runtime log session was captured. Dev27 behavior remains unaccepted,
  and dev28 height smoothing, target switching, expanded-map rearm, travel, and
  clean shutdown all require runtime gameplay evidence.

## 0.6.0-dev27-arrow-caps-map-rearm1

- Record the dev26 runtime acceptance boundary: the exact native map-icon
  Canvas parent, visible projected markers, inherited pan/zoom, and F8 removal
  are accepted. F7 restoration while the same map remains open is not accepted;
  its bounded attempts stopped at temporary failure-8 map-data absence and a
  close/reopen created the next successful map session.
- Connect the six-piece height pointer's outer shaft to its tip and inset every
  inner endpoint by two reference pixels so the dark shaft and arrowhead caps
  are not overwritten. Keep the same six preallocated pieces and update path.
- Reduce each expanded-map treasure canary host from 24 to 10 reference pixels.
- Preserve only a validated numeric map ID/dimensions/UI-size triple across
  detach. Use it only for the exact current map ID with finite valid scalars,
  and retain no map-data UObject.
- Permit one explicit F7 to rearm a stopped world-map session against the
  retained live layer without another full activation or SQL snapshot. This is
  event-only and adds no timer, polling, scheduled retry, recurring lookup, or
  steady-state work.
- Runtime acceptance remains pending for the arrow caps, reduced marker size,
  same-open-map F7 restoration, travel, and clean shutdown.

## 0.6.0-dev26-native-map-icon-path-height2

- Record the decisive dev25 runtime split: map 100, the canonical classless
  data paths, two real marker records, numeric projection inputs, the native
  `DLayerMap` listener, bounded readiness attempts, and F8/F7 reattachment all
  succeeded. The same attachment reported `paint_owner=missing`, and the user
  observed no expanded-map pixels, so the old composition host is rejected
  without changing the proven data or lifecycle paths.
- Replace only the expanded-map composition host. During a successful bounded
  attachment transaction, inspect the current `DLayerMap.ArrayIconInfo`, accept
  a live `UDMapPointIconUserWidget` whose `CanvasPanelSlot` resolves the exact
  native Canvas parent, and create exactly two fixed native-icon-class hosts
  under that parent. Invalid reflection or partial attachment fails closed.
- Preserve the canonical map-data paths, two-marker selection, numeric
  projection, create listener, at-most-three adjacent readiness attempts, weak
  retention, and F8/travel/shutdown cleanup. Add no global enumeration, timer,
  polling, scheduled retry, recurring lookup, or steady expanded-map work.
- Repair the fixed six-piece nearest-height pointer with actual-angle clearance,
  a shaft ending at the arrowhead base, and outline-before-inner composition.
  No widget, update cadence, allocation, lookup, or steady motion work is added.
- Runtime compact-arrow and expanded-map visual acceptance remain pending; this
  release does not claim visible pixels, correct map coordinates, or gameplay
  acceptance from static source changes.

## 0.6.0-dev25-world-map-data-path-height1

- Record the decisive dev24 runtime stage: the native `DLayerMap` listener
  captured map 100 and selected two real treasures, but every bounded attempt
  ended at failure 8 with zero map dimensions, zero UI size, and zero
  attachments. Discovery, selection, projection, and composition are therefore
  separated explicitly.
- Replace the rejected export-text
  `DWorldMapData'/Game/...WorldMapData_100'` and map-200 strings with the
  canonical classless `/Game/...Asset.Asset` paths required by UE4SS
  `StaticFindObject`. Add a regression gate that prohibits the old form.
  No enumeration, timer, retry schedule, or steady-state update is added.
- Restore the user-accepted dev23 single-fill four-piece treasure silhouette and
  remove the dev24 two-tone body regression without changing the 14/22 sizes,
  category palette, marker count, or update cadence.
- Pin the complete outlined height arrow outside the nearest chest using
  chest-width, arrowhead, and outline-aware clearance. Reduce only the inner
  stroke width so the existing dark outline remains visible; no widget is added.
- Retain the dev24 Boss and Assault contrast geometry and all native lifecycle,
  weak-retention, bounded-session, and performance invariants. Runtime gameplay
  evidence remains required.

## 0.6.0-dev24-world-map-discovery-icons1

- Record the runtime-proven dev23 failure stage: the exact
  `DLayerMap:SetWorldMapImage` hook registered successfully, but opening the
  expanded map produced zero sessions, attempts, attachments, or map-data
  lookups. Projection and attachment therefore never had an opportunity to run.
- Register one native exact-class `DLayerMap` UObject-create listener. Its hot
  callback performs only a class filter, one fixed mutex-protected weak-identity
  slot update, and atomic publication; it performs no logging, allocation,
  `ProcessEvent`, file access, or global enumeration.
- Keep `SetWorldMapImage` as an auxiliary trigger and permit at most one exact
  `FindFirstOf(DLayerMap)` catch-up after each F7 activation or completed travel
  while enabled, and only when no live candidate is retained. No timer or
  recurring lookup is added.
- Unregister the create listener during UObject-array shutdown and normal
  process cleanup before weak renderer state is cleared.
- Preserve the fixed 80-by-four compact marker pieces and six height-pointer
  pieces while improving only existing geometry and colors: two-tone treasure
  bodies and lids, a wider outlined pointer gap, a higher-contrast Boss glyph,
  and a distinct Assault backing.
- Preserve the accepted 16 ms single-root translation, 14/22 treasure sizing,
  selection bounds, one-shot save reconciliation, and zero steady-state
  expanded-map update path. Runtime composition, projection, reopen, F8/F7,
  travel, and clean-shutdown evidence remain required.

## 0.6.0-dev23-world-map-overlay-canary1

- Strengthen the existing fixed four-piece treasure silhouette and six-piece
  nearest-height pointer without increasing the compact widget count. Normal
  and nearest treasure sizes become 14 and 22 reference pixels.
- Raise the compact viewport host to a high hit-test-invisible Z order so native
  radar pixels compose above ordinary game task indicators.
- Register one post-`SetWorldMapImage` owner hook that publishes only a weak
  DLayerMap identity and a numeric session serial. F8 and travel detach the
  world-map host and invalidate the session fail closed.
- Add an event-driven MapOverlay canary with two fixed four-piece treasure
  glyphs. A ready session resolves the exact WorldMapData_100/200 asset and
  attaches once; a momentarily incomplete layer or asset receives at most three
  adjacent readiness attempts before stopping. It accepts only map-100 treasure
  selection and inherits the game's pan, zoom, clipping, and RetainerBox
  composition.
- Reuse an already-attached identical live layer instead of detaching and
  rebuilding its host after duplicate `SetWorldMapImage` events.
- Make detach faults fail closed in `Faulted` state so an unreachable old host
  cannot be followed by a second same-generation attachment.
- Consume a pending canary session when one-shot save reconciliation has
  definitively failed instead of retaining a permanent 16 ms scalar gate.
- Keep the expanded-map path free of timers, recurring update functions,
  ambiguous FindFirstOf calls, UObject enumeration, file/database polling, and
  OnPaint hooks. Full expanded-map parity remains gated on runtime composition
  and projection evidence.


## 0.6.0-dev22-compact-feature-parity

- Preserve the user-accepted dev21 UMG DPI-logical coordinate conversion and
  exactly one root-Canvas translation in the 16 ms motion path.
- Replace the small single-color proof chest with a larger four-piece chest
  glyph using the stable white, green, orange, and blue treasure-category
  palette. Normal treasure uses 12 reference pixels and exactly one nearest
  treasure uses 19.
- Restore the stable nearest-treasure Z-axis height pointer with six
  preallocated line pieces. It is not the removed fixed yellow test arrow and
  performs no per-frame child update.
- Restore 9 Boss, 40 Assault, 33 Fly, 40 Mole, and 10 Wave compact markers in
  the same fixed 80-slot pool. Mixed selection adds only bounded scalar work at
  the existing movement/five-second rebind boundary.
- Extend the one F7 below-normal SQLCipher transaction to read
  `tb_actor_respawn` through the same database connection. Boss and Assault
  records use an elapsed 120-minute cooldown; native defeat events update the
  in-process numeric state immediately.
- Reuse reward treasure opened bits for all mini-game visibility, preserve
  process-lifetime native treasure deltas, and keep travel state numeric and
  fail closed. No runtime SQL polling or additional UObject scan is added.
- Extend deployment and source gates to require exactly 9 Boss, 40 Assault,
  33 Fly, 40 Mole, and 10 Wave catalog records and all fixed-pool invariants.

## 0.6.0-dev21-umg-dpi-space-fix

- Read `WidgetLayoutLibrary.GetViewportScale` exactly once during the guarded
  attachment transaction and reject non-finite or implausible values.
- Keep the host origin in raw viewport pixels for
  `SetPositionInViewport(..., RemoveDPIScale=true)`, while converting desired
  size, Canvas slot positions, icon sizes, and root translation through
  `display_scale / viewport_dpi_scale`. This removes the double UMG DPI
  application that let logically bounded treasure markers overflow visually.
- Remove the temporary dev20 yellow player-arrow witness, its second Blueprint
  host, and its per-sample viewport layout mutation. The 16 ms motion path now
  performs exactly one numeric root-Canvas `SetRenderTranslation` call.
- Add one `COMPACT_GEOMETRY` record after a successful attachment so runtime
  acceptance can prove the viewport size, DPI scale, display scale, and final
  UMG logical-unit conversion without logging in the motion path.
- Preserve the stable 16/10 nearest/normal chest sizing, 80-marker bound,
  125/225-metre live radius, one-shot save reconciliation, and existing F7/F8,
  menu, travel, and weak-lifetime handling.
## 0.6.0-dev20-projection-parity-arrow

- Restore the stable external radar's 16/10 reference-pixel nearest/normal
  treasure sizing instead of the insufficient 17/14 experimental contrast.
- Add one temporary 24-pixel yellow arrow for the nearest treasure through the
  independently proven direct viewport route. The fixed cloned center arrow
  remains removed; this arrow exists only when a nearest treasure is bound and
  shares menu, F8, travel, and shutdown lifecycle handling.
- Log a changed compact selection's count, nearest ID, player position, target
  position, normalized coordinates, and radius at the low-frequency rebind
  boundary. No logging enters the 16 ms motion path.
- Preserve the 80-chest pool and live 125/225-metre radius. Dev19 runtime showed
  only 14-18 active entries and zero renderer faults, proving the reported
  disappearance was not capacity overflow.

## 0.6.0-dev19-live-minimap-projection

- Replaced the incorrect fixed 225-metre compact projection with a guarded
  once-per-second read of the game's live minimap scale. Scale 2.7 or above
  selects the accepted 125-metre town radius; lower values select the accepted
  225-metre field radius. Nested minimap UObjects are never retained, and the
  steady 16 ms motion path remains one numeric root-Canvas translation.
- Restored the accepted fixed capacity of 80 treasure glyphs. The pool is
  still allocated only once per F7 activation or valid travel epoch, so nearby
  candidates no longer churn through the temporary eight-slot experiment and
  the per-frame call count does not increase.
- Clear the complete authored visual subtree of the cloned player-icon
  Blueprint root before adding chest glyphs, removing the fixed yellow arrow
  without replacing the runtime-proven WidgetTree or root Canvas.
- Fail closed by collapsing the marker pool when the live minimap scale cannot
  be read, and retry only on the one-second scalar schedule.

## 0.6.0-dev18-world-catalog-root-motion

- Replace the proof-time 1,692-row actor render source and its forced map-100
  assignment with the 1,693-entry section-aware render catalog. Compact mode
  now accepts only the validated 1,506 map-100 points, so dungeon and interior
  local coordinates cannot appear as open-world treasure markers.
- Permanently detach the cloned Blueprint's stock player-arrow child after its
  viewport construction lifecycle. The game-owned player arrow is untouched;
  the native radar renders only chest glyphs, with the nearest chest modestly
  enlarged from 14 to 17 reference pixels.
- Move only the initialized root Canvas during the 16 ms motion path instead
  of moving the Blueprint host that can receive its own updates. Unchanged
  bindings now refresh every five seconds instead of every second, while
  meaningful movement and eligibility changes still refresh immediately.

## 0.6.0-dev17-native-save-chest-smooth

- Reconcile treasure save state exactly once per F7 activation on a
  below-normal native worker. The worker receives no Unreal object, performs
  no retry, and publishes only numeric opened-bit fields tagged with the
  activation token.
- Start compact eligibility fail-closed, preserve the static `ignore 11003`
  duplicate rule, and prevent a late save snapshot from resurrecting any
  treasure hidden by a native interaction or disappearance event.
- Replace temporary copied player-arrow art with eight fixed four-piece
  `UBorder` chest glyphs. The nearest glyph is 17 reference pixels and the
  remaining glyphs are 14 pixels.
- Increase player sampling from 33 ms to 16 ms and replace the high-frequency
  `SetPositionInViewport` layout mutation with exactly one numeric
  `SetRenderTranslation` call on the existing Blueprint host.
- Ship SQLCipher and its licenses for the one-shot native reconciliation while
  retaining a native-only package with no host executable, Lua scripts, or
  recurring file/database polling.

## 0.6.0-dev16-minimap-local-canvas

- Correct the dev15 coordinate-space mismatch. Marker slots now use positions
  local to a bounded compact-map host instead of writing full-screen positions
  into the Blueprint Canvas.
- Require the Blueprint-created `WidgetTree.RootWidget` to be the existing
  `CanvasPanel`, place that host at the accepted compact-map viewport anchor,
  and retain exactly one host-position update in the 33 ms path.
- Collapse the Blueprint stock player arrow after `AddToViewport` so its
  construction lifecycle cannot restore the single diagnostic arrow.
- Preserve the fixed eight-image pool, weak UObject retention, single-attempt
  activation, F7 idempotence, F8 cleanup, and menu suppression behavior.

## 0.6.0-dev15-blueprint-tree-pool

- Remove the runtime-invisible base `UUserWidget` and private replacement
  `WidgetTree`. Dev14 proved that public `UImage.SetBrush` succeeded while that
  replacement tree still emitted no Slate pixels.
- Create exactly one instance of the game player-icon Blueprint, preserve its
  engine-initialized `WidgetTree`, and locate an existing `CanvasPanel` through
  a bounded parent walk from `PlayerIcon_MiniMap`.
- Collapse the cloned stock player arrow and add the fixed eight-image pool to
  that existing Canvas. The temporary dev14 Blueprint witness is removed.
- Keep the 33 ms path to one numeric `SetPositionInViewport` call on the single
  host. Child creation, brush synchronization, Canvas discovery, and slot setup
  remain one-time attachment work with no retry loop.

## 0.6.0-dev14-umg-visual-gate

- Add one temporary Blueprint-initialized player-arrow witness through the
  dev11 viewport route while retaining the dev13 eight-image compact pool.
  Both are created at most once per activation and share F7, F8, travel, menu,
  and shutdown lifecycle handling.
- Apply the live player-arrow brush to every hand-built `UImage` through the
  reflected public `UImage.SetBrush` function instead of relying on direct
  property-memory copy. The reflected struct parameter is shape-validated and
  bounded before any call.
- Keep the production pool's 33 ms path at exactly one numeric root-canvas
  translation. The temporary witness adds one numeric viewport-position call
  only while this visual gate is under runtime validation.
- Remove the dev11 attachment retry window for this gate: the Blueprint witness
  gets exactly one attachment attempt per activation or travel epoch.

## 0.6.0-dev13-root-canvas-f7-idempotent

- Attach the eight fixed marker images directly to `WidgetTree.RootWidget` and
  translate that root as one motion layer, matching HUDControl's proven
  in-game topology instead of the dev12 nested canvas that attached without
  visible pixels.
- Coalesce repeated F7 input while the native radar is already active so key
  repetition cannot detach the host and restart the 750 ms stability delay.
  Explicit F8 followed by F7 remains the normal restart path.
- Raise the low-frequency rebind threshold from 20 to 1000 Unreal units while
  preserving the one-second deadline. Dev12 runtime evidence showed 1223
  rebinds in 1678 translations, so the old threshold was not low frequency.
- Increase the temporary copied-arrow witnesses to 20 pixels and 28 pixels for
  the nearest entry so brush transparency cannot reduce the proof to only a
  few colored pixels.

## 0.6.0-dev12-real-treasure-pool

- Replace the single cloned player-widget witness with one base `UUserWidget`
  that owns a private `WidgetTree`, root canvas, translatable marker canvas,
  and eight fixed `UImage` children.
- Bind the pool to the nearest actual coordinates from the 1,692-entry native
  treasure actor catalog. The nearest marker alone is enlarged; all markers
  temporarily reuse the live player-arrow brush for an unambiguous mechanism
  test.
- Recompute bindings only after meaningful movement or a bounded one-second
  deadline. Between rebinds, the 33 ms path performs exactly one cached
  `SetRenderTranslation` call on the marker canvas.
- Permit one attachment transaction per F7 activation or travel epoch and
  expose a numeric failure stage instead of retrying a global UObject lookup.
- Keep all catalog entries eligible for this route proof. Save filtering and
  production marker art remain subsequent acceptance gates.

## 0.6.0-dev11-widget-owner-fix

- Resolve `GetOwningPlayer` from its reflected owner, `UMG.Widget`, instead of
  the derived `UMG.UserWidget` path. Dev10's ABI mask isolated this as the only
  remaining initialization failure and showed that no attachment was attempted.

## 0.6.0-dev10-viewport-abi-fix

- Fix the dev9 startup gate that compared Unreal's unpadded 17-byte
  `SetPositionInViewport` parameter span with the 24-byte aligned local C++
  buffer and therefore disabled UMG before the first attachment attempt.
- Keep the local aligned buffer, validate the correct reflected parameter
  span, and publish a startup ABI failure bitmask so future metadata drift is
  distinguishable from an invisible or failed attachment.

## 0.6.0-dev9-viewport-widget-canary

- Reject the dev8 `MapOverlayOutSide` route after runtime proved successful
  attachment and numeric updates with zero faults but no visible pixels.
- Create the game's existing `PlayerIconWidget` Blueprint through
  `WidgetBlueprintLibrary.Create` and add it directly to the game viewport at
  a high UMG Z order, outside the compact-map clipping and composition stack.
- Copy the live minimap player brush into the cloned widget, size and position
  it once from the current viewport, then keep the 33 ms steady state to one
  `SetPositionInViewport` call with numeric parameters only.
- Correct native-only packaging: the installed prototype must not contain the
  stable Radar's Lua scripts, host executable launcher, installer, source, or
  exact `enabled.txt` marker.

## 0.6.0-dev8-game-brush-minimap-canary

- Replaced the invisible cloned-player-widget experiment with one native
  `UImage` whose complete `Brush` and `ColorAndOpacity` properties are copied
  from the currently visible `PlayerIcon_MiniMap` before Slate attachment.
- Retained the game-owned `MapOverlayOutSide` composition layer, forced a
  34-pixel desired size and visible opacity, and kept the steady-state path to
  one numeric translation at the existing 33 ms cadence.
- Replaced the one-shot attachment attempt with a bounded 250 ms retry window
  lasting at most eight seconds after F7 or travel. Lookup stops immediately
  after success and cannot become a session-long periodic hitch.
- Restart the same bounded attachment window when compact rendering resumes
  after a menu or other temporary suppression. Pausing still removes the child
  once, but reopening no longer leaves the marker permanently detached.
- Added numeric attachment failure stages to runtime state logging so a missing
  layer, widget, brush, or slot can be distinguished without repeated F7 input.

## 0.6.0-dev7-game-widget-clone-canary

- Stop treating successful `AddChild` calls for hand-constructed blank UMG
  primitives as visual proof.
- Clone the game's current, already-visible `PlayerIconWidget` class through
  `WidgetBlueprintLibrary.Create` so its Blueprint tree, brush, texture,
  material, and Slate initialization follow the same engine-owned route as a
  stock minimap icon.
- Attach the clone to `MapOverlayOutSide`, force visibility, opacity, scale,
  and one layout prepass only at creation, then retain weak identities and
  update only one translation at the existing 33 ms cadence.
- Dev7 still requires visible in-game evidence before any renderer claim.

## 0.6.0-dev6-umg-minimap-canary

- Replaced the runtime-rejected D3D12 command-queue injection experiment with
  a game-thread-only UMG child attached to `DLayerMiniMap.LayerMap.MapOverlay`.
- Removed the late-Present implementation from the native target and removed
  its D3D12 link dependency; the rejected source remains only as historical
  evidence and cannot execute in dev6.
- Construct a transient 18-by-18 `USizeBox` and colored `UBorder` under the
  owning `WidgetTree`, retain only weak object identities, and update one
  numeric render translation at the existing 33 ms coordinate cadence. A bare
  `UImage` was rejected after runtime proved that it attached and updated but
  had no Brush resource and therefore emitted no visible pixels.
- Keep the canary visible at a fixed quarter-radius offset when no catalog
  target is available; switch to the selected target displacement when one is
  available so attachment can be validated in any open-world location.
- Remove the child on F8, travel, suppression, and shutdown. Attachment is
  attempted at most once per activation, so the steady state performs no
  UObject search, allocation, file access, or logging.
- Runtime rendering, movement, F8, travel, and exit safety remain unaccepted
  until the in-game canary sequence passes.
- Runtime proved that both the bare Image and the sized Border attached and
  updated without faults inside `MapOverlay`, but remained invisible behind
  the retained map/fog composition. Move the canary to the game-provided
  `MapOverlayOutSide` layer and invalidate its layout once after attachment;
  the steady-state translation path is unchanged.
- Replace hand-constructed primitive widgets with a `WidgetBlueprintLibrary`
  clone of the game's already-visible `PlayerIconWidget` class. This uses the
  game's initialized Blueprint widget tree, texture, material, and Slate
  resources instead of treating successful `AddChild` as proof that a blank
  native primitive can paint. Force visibility, opacity, scale, and one layout
  prepass only at attachment; steady-state work remains one translation.

## 0.5.0-dev14-bounded-dispatch-recovery

- Replaced permanent session isolation after a single 3-second ProcessEvent callback timeout with one bounded F8-to-F7 recovery attempt.
- Invalidates the expired dispatch token before recovery so a late callback cannot mutate current state. Any successful owned callback resets the consecutive-timeout strike.
- A second consecutive callback timeout still isolates the dispatcher for the rest of the game session. The normal path adds no polling, filesystem work, queue, or growing state.

## 0.5.0-dev13-baseline-lock-fix1

- Locked the open-world identity baseline for the lifetime of the game process. F8 disables active work without erasing it, so F7 inside a dungeon cannot redefine that dungeon as the visible-radar baseline.
- Added a confirmed open-world path guard (`/Maps/World/`). A first activation inside a dungeon remains paint-suppressed until the native probe observes the open world and establishes its baseline.
- Retained the dev13 fixed-size 250 ms identity probe and menu cursor paint gate with no UObject enumeration, history, retry queue, or growing state.

## 0.5.0-dev13-world-identity-activity-gate

- Added a bounded native World identity probe to the existing 250 ms discovery cadence so seamless challenge and instance travel can stop compact radar painting even when `InitGameState` is not called.
- The activity gate retains only World and GameMode/GameState strings, performs no UObject enumeration or filesystem I/O, emits logs only when identity changes, and fails open when a current identity cannot be read.
- Preserved the event-driven GameMode/GameState comparison as an early auxiliary signal and recomputes suppression from either reliable mismatch without accumulating transition history.

## 0.5.0-dev12-slow-refresh-activity-gate

- Added threshold-only `OVERLAY_SLOW_TICK` and `OVERLAY_SLOW_REFRESH` phase timings for refresh stalls of at least 50 ms. Normal ticks add only monotonic timestamp reads and produce no extra log traffic.
- Captured the current GameMode/GameState class signature at F7 and after `InitGameState`. A different non-empty signature stops compact-radar painting until the open-world signature returns; missing context fails open.
- Published activity suppression through an unused native shared-memory flag without changing the mapping size or retaining GameMode, GameState, World, Controller, or Pawn objects.

## 0.5.0-dev11-single-arrow-native-clock

- Restored a single nearest-treasure height arrow; the second-nearest treasure now follows the normal marker path without enlargement or an arrow.
- Moved the one-shot `DGameSingleton.TimeOfDay` baseline and 60x monotonic extrapolation into the native adapter. Lua no longer loads or calls the clock provider and publishes no clock changes.
- Added world-clock availability and seconds to unused fields in the existing native shared-memory frame without changing its protocol size.
- Corrected menu cursor sampling to use `FBoolProperty::GetPropertyValueInContainer`, preserving UE bitfield semantics.

## 0.5.0-dev9-menu-paint-gate

- Added native `bShowMouseCursor` state to the existing shared-memory status flags.
- Stopped compact-radar window presentation, invalidation, and immediate motion wakeups while the cursor is visible. World-map rendering remains unaffected.
- Added a 250 ms resume debounce while menu entry stops compact painting immediately.

## 0.5.0-dev8-hidden-state-fallback

- Extended registered treasure presence from UObject allocation lifetime to visible Actor lifetime through `Actor:IsHidden`. A chest that becomes hidden after being seen now completes even when UE retains its UObject after the player departs.
- Required a visible observation before hidden state can count as disappearance, preventing initially hidden actors from being treated as opened.
- Kept the check bounded to already registered objects at the existing 250 ms probe; no additional object enumeration, SQL, or filesystem polling was added.

## 0.5.0-dev7-interaction-correlation

- Correlated `Server_RunInteractV2` with the unique treasure catalog point within eight metres when the RPC receiver does not expose its target. This closes the open-and-immediately-depart race before the 250 ms nearby observer can bind, while ambiguous or missing matches fail closed.
- Retained exact target resolution as the primary interaction path and the 10-second, two-sample disappearance path as a fallback.
- Added native regression coverage for unique, ambiguous, and absent interaction correlations.

## 0.5.0-dev6-interaction-hook

- Hook the game-owned `DInteractableComponent.Server_RunInteractV2` interaction boundary and resolve its exact Treasure actor through `ExecuteTargetObject` or `ExecuteTargetComponent` ownership.
- Publish the matched install-generated Treasure `save_id` immediately when the current player interacts within 8 meters, before the multi-second disappearance animation finishes.
- Retain the type-specific 10-second disappearance grace as a fallback and keep transition handling fail closed.

## 0.5.0-dev5-departure-grace

- Arm treasure observations within 8 meters and Boss/Assault observations within 100 meters.
- Preserve an observed candidate for 10 seconds after leaving its radius so delayed destruction can still publish completion.
- Cancel departure expiry when the player re-enters the radius and evict live candidates after the grace period to prevent accumulation.
- Keep LoadMap transitions fail closed and retain no gameplay UObject across worlds.

## 0.5.0-dev4-nearby-catchup

- Delay the one-shot exact-class catch-up scan from 35 meters to 10 meters so a class is not permanently consumed before its nearby actor is streamed in.
- Add bounded observation and eligible-EndPlay evidence logs without adding polling, SQL, or extra enumeration.

## 0.5.0-dev3-native-endplay

- Preserve an eligible EndPlay callback as a logical lifetime boundary even while Unreal keeps the weak UObject allocation reachable until garbage collection.
- Prevent later weak-pointer probes from incorrectly cancelling an already confirmed `Destroyed` or `RemovedFromWorld` transition.

## 0.5.0-dev2-native-presence

- Confirm opened treasure and defeated encounters after a positively observed nearby actor is absent for two consecutive 250 ms probes.
- Accept `Destroyed` and `RemovedFromWorld` only as first-stage disappearance evidence; transition, stale epoch, and distant observations still fail closed.
- Generate exact native blueprint-class catalogs for all 9 world bosses and 40 Assault targets from the installed game PAK.
- Retain only `FWeakObjectPtr` identities between callbacks and add no periodic SQL or recurring UObject enumeration.

## 0.5.0-dev1-native-object-state

- Forked the frozen production radar into the independent
  `DragonSwordWorldRadarObjectState` Mod identity.
- Added a native UE4SS provider for 33 ms fresh player coordinates, F7/F8
  activation, epoch-safe actor lifecycle tracking, and numeric shared memory.
- Added proximity-gated, once-per-class-and-activation catch-up for actors that
  already exist when F7 is pressed.
- Removed periodic save fingerprint/copy/SQL scheduling. Every F7 now consumes
  exactly one reconciliation attempt, with no retry until the next activation.
- Added native treasure-opened deltas and three evidence-backed encounter
  defeated deltas; returning SQL snapshots merge current-activation deltas.
- Changed encounter availability to the user-validated elapsed 120-minute
  cooldown.
- Removed compact player-motion bridge-file writes while retaining control,
  status, heartbeat, and UMG world-map geometry transport.
- Added install-time generation of the exact treasure actor catalog from the
  matching section and blueprint PAK tables.
- Added strict C++ core, UE4SS ABI, shared-memory protocol, generator, managed,
  performance-policy, and package gates.

## 0.4.0-dev74-processdispatchguard1-localcapfix1

- Fixes the release-blocking UE4SS load error `too many local variables (limit is 200)` introduced by the serialized dispatcher candidate. Dispatcher constants, ownership state, and stable handlers now share one table without changing scheduling behavior.
- Adds a source gate capped at 190 top-level Lua locals, preserving ten slots of headroom below the VM limit so a syntactically valid but unloadable `main.lua` cannot pass the normal build again.

## 0.4.0-dev74-processdispatchguard1

- Serializes activation, compact control, one-shot clock, and expanded-map UObject work through one stable ProcessEvent callback and one shared in-flight slot; task producers coalesce behind that slot instead of submitting independent UE4SS actions.
- Preserves task-owned scalar tokens and the world epoch at both dispatcher and task layers, so F8, travel, or runtime recovery makes queued work stale before any UObject access.
- Replaces the ineffective activation-only retry with a session-scoped ProcessEvent poison state. One accepted callback that fails to enter within three seconds disables WorldRadar and permanently rejects further F7 submissions for the current game session without retrying the dead route.
- Keeps EngineTick fallback intentionally absent. Normal sampling and presentation intervals, UObject read counts, bridge traffic, save work, and LoopAsync registration count are unchanged.

## 0.4.0-dev74-savecooldown1

- Starts a fresh 19-second eligibility cooldown after every completed or failed SQLCipher worker, preventing an expired change-check deadline from immediately queuing a catch-up snapshot after a slow query.
- Retains a changed active-slot fingerprint while a worker is busy or cooling and revalidates the current source before the next eligible queue; unchanged saves still exit before copy or SQLCipher.
- Replaces the moving-coordinate two-second treasure detail log with save/catalog/filter version gates. Acceptance debug remains enabled by default without repeatedly sorting and formatting nearby treasure details.

## 0.4.0-dev74-startuponlyio1

- Removed runtime `mods.txt` polling from Lua and the Overlay. The existing watcher host performs the single startup enablement check; changing `mods.txt` during a running game now requires a game restart.
- Loads `data/treasure_overrides.txt` and its generated name catalog once during Overlay construction. Runtime edits require a game restart and no longer cause two filesystem metadata reads per second.
- Defaults `high_resolution_timer` and `debug_logging` to `true` for final runtime acceptance while keeping `diagnostic_verbose` false.

## 0.4.0-dev74-savechangegate1

- Replaced the two-second metadata poll, four-second debounce, and 45-second periodic snapshot window with one non-harmonic 19-second change-check interval.
- Exits before snapshot copy or SQLCipher whenever the active slot, exact source fingerprint, and key match the last successful publication; changed input queues one complete background snapshot immediately.
- Retains fail-closed before/after copy fingerprint validation, so a concurrent game save is discarded instead of publishing a mixed database/WAL snapshot.

## 0.4.0-dev74-processeventfix1

- Removed the expanded-map `LoopInGameThreadAfterFrames` action after runtime evidence showed UE4SS `EngineTick.LuaModImpl` failing with `Ref was not function`, followed by stalled map production and F7 activation callbacks.
- Routes every WorldRadar game-thread handoff explicitly through `EGameThreadMethod.ProcessEvent` and fails closed when that route is unavailable, avoiding the installed UE4SS EngineTick deferred-action queue whose overlapping-action registry failure matches the observed log.
- Restores the 8 ms expanded-map request path with one reusable callback object and one strict pending request across F8/travel epochs. A stale queued callback cannot read UObjects and must drain before another lifecycle can queue work; compact sampling, drawing, bridge, save, and presentation cadences are unchanged.

## 0.4.0-dev74-gameupdate1

- Rebound the exact Assault inference policy to game fingerprint `f7c6734b99d4` after a fresh current-PAK audit reproduced all 1693 Treasure, 9 Boss, 83 mini-game, and 40 Assault records without semantic changes.
- Retains the map-100 correction for eastern Boss IDs `9000022`, `9000023`, and `9000025` and the bounded hidden-candidate replacement fix from `0.4.0-dev74-worldmapcandidatefix1`.

## 0.4.0-dev74-worldmapcandidatefix1

- Corrected eastern field Boss IDs `9000022`, `9000023`, and `9000025` to map group 100. Their authoritative `SectionUID` values end in `100`; genuine map-200 treasure records remain untouched.
- Fixed expanded-map reopen starvation: a newly created current-epoch map widget can replace one retained hidden candidate when the two-entry cap is full, while active visible candidates remain protected. This adds no recurring scan or polling work.
- Retains the persistent game-frame world-map sampler introduced by `0.4.0-dev74-worldmapframeloopfix1`.

## 0.4.0-dev74-worldmapframeloopfix1

- Replaced the visible world's repeated 8 ms `LoopAsync -> ExecuteInGameThread` submissions with one UE4SS `LoopInGameThreadAfterFrames(1, ...)` action that is created once, paused while inactive, and resumed for later map sessions.
- Removed the continuous world-map `luaL_ref`/`luaL_unref` cycle confirmed in the exact installed UE4SS implementation; expanded-map UMG reads now occur once per game frame, while scalar bridge presentation remains at 8 ms.
- Preserved fail-closed world epoch/loop-token ownership and added bounded handle validation plus pause/resume failure reporting. F8, travel, map close, and generation invalidation revoke numeric ownership before any native pause attempt, so an indeterminate pause can perform only a scalar no-op and cannot admit a duplicate frame action.
- Retained the dev74 compact radar, top-level minimap cache correction, activation watchdog, catalogs, and rendering behavior unchanged.

## 0.4.0-dev74-worldmapcallbackfix1

- Fixed the confirmed lifecycle overlap after F8 or travel invalidated an outstanding visible-world-map game-thread task: its scalar pending ownership now remains closed until the stale callback drains instead of allowing a second lifecycle to queue over it.
- Replaced the visible 8 ms world's per-sample anonymous `ExecuteInGameThread` closure with one stable function object, removing continuous native Lua-registry callback-reference churn without adding a timer, thread, UObject read, bridge operation, or render pass.
- Preserved the dev74 8 ms expanded-map cadence and all existing epoch/token/UObject fail-closed gates, and retained the preceding top-level `DLayerMiniMap` cache correction.

## 0.4.0-dev74-minimapfindfix1

- Fixed the confirmed post-travel one-hertz `DLayerMiniMap` global lookup hitch by separating top-level UObject lifetime from temporary nested-property availability.
- Retains only the freshly validated top-level `DLayerMiniMap` root when `LayerMap -> MapOverlay -> RenderTransform.Scale.X` is temporarily unavailable; the nested wrappers remain callback-local and protected by `pcall`.
- Revalidates the root before every scalar sample and still purges it on every world/F8 transition. Stable cache-hit samples perform no `FindFirstOf`, while protected property reads continue so minimap scale can recover without a new timer, hook, enumeration, or retry loop.

## 0.4.0-dev74-activationwatchdog1

- Fixed the confirmed F8-to-F7 permanent-disabled state where `ExecuteInGameThread` accepted an activation probe but never invoked its callback.
- Added a scalar-only three-second activation callback deadline that invalidates the stale token and queues exactly one replacement probe.
- Fails closed after the single replacement also times out; automatic retries stop, while a later manual F7 remains available. All dev74 sampling, rendering, catalog, and save-query behavior is unchanged.

## 0.4.0-dev74-minigamecatalog2

- Kept the established 250 ms control loop alive during bounded runtime recovery instead of replacing its active UE4SS Lua callback.
- Expanded the install-generated reward mini-game catalog to 83 records: 33 ordinary-world Fly, 40 Mole, and 10 Wave.
- Added allocation-free code-native hammer and wave markers for Mole and Wave activities.
- Bound Wave 13008 to the current PAK's exact mislabelled reward tuple `save_id=13008`, `DT_MiniGame_G5_13009` without introducing a general offset rule.

## 0.4.0-dev72-runtimewatchdog1

- Added bounded automatic F8-to-F7 lifecycle recovery for confirmed asynchronous Lua failures and a confirmed stalled 250 ms control loop. The existing compact or expanded-map presentation loop observes only scalar control-heartbeat progress once per second and requires five consecutive stale observations before recovery.
- Kept temporary expanded-map read loss on the dev71 candidate-only recovery path; missing map state does not arm whole-runtime recovery. The watchdog performs no UObject, bridge, drawing, game-thread queue, or file work and adds no fourth `LoopAsync` registration.
- Automatic recovery invalidates all pending tokens and UObject wrappers, publishes disabled state, registers one replacement control loop, and reuses the four-sample F7 stability gate. Three consecutive automatic failures fail closed instead of creating an unbounded restart loop.

## 0.4.0-dev71-worldmaprecovery1

- Replaced the two-second expanded-map rediscovery gap after an active read failure with one next-control-sample bounded rescan. The failed candidate wrappers are still dropped immediately, the candidate token is advanced, and no UObject survives the failure.
- Added a 250 ms visibility probe for at most two already-bounded hidden current-epoch candidates, allowing the first map opening and later reopenings to be detected without recurring UObject enumeration.
- Preserved the visible 8 ms expanded-map path, retired-identity protection, epoch/token travel invalidation, F8 suspension, and compact scheduling. Added structural gates that reject direct enumeration or wrapper retention in the recovery block.

## 0.4.0-dev70-clocklogfix1

- Corrected the debug-only `WORLD_TIME_TASK_PERF.failure` field so a successful one-shot clock capture records `none` instead of the misleading string `true`.
- Preserved thrown `pcall` errors and normal capture-failure reasons without changing clock capture, scheduling, publication, or normal-play execution.
- Added source and scheduling gates that reject the ambiguous Lua truthiness expression. All dev69 marker, control, cadence, bridge, and lifecycle behavior remains unchanged.

## 0.4.0-dev69-heightpairdebug1

- Extended the existing nearest-treasure height indicator to the two closest visible treasures in one allocation-free selection pass. The closest marker remains enlarged and labeled; the second-closest marker keeps the normal diameter and receives only its height indicator.
- Applied the same two-marker rule to the expanded map while preserving projected-pixel deduplication; an exactly overlapping second marker remains suppressed instead of producing indistinguishable duplicate geometry.
- Restricted F5 no-paint and F6 frozen-motion A/B hotkey registration to `debug_logging = true`. F7 and F8 remain the only registered normal-play controls, while the protocol and diagnostic implementations remain available for bounded debugging.
- Added deterministic nearest-pair and debug-hotkey guard tests without adding runtime queries, catalogs, timers, bridge traffic, or allocations.

## 0.4.0-dev68-moleoverridefix1

- Removed the obsolete stock `ignore 11003` treasure override after gameplay identification confirmed that the apparent overlap belongs to the valid MiniGame/Fly reward path.
- Added an exact installer migration that removes only the old `11003` ignore and its two stock explanation lines while preserving unrelated ignores and user-authored aliases.
- Added source and scheduling gates that prohibit the obsolete default rule. All dev67 performance scheduling and runtime behavior remain unchanged.

## 0.4.0-dev67-compactingest1

- Changed only compact Overlay prediction/presentation from 50 ms to 33 ms while retaining 250 ms fresh current-Pawn sampling, the 50 ms scalar-only Lua flush loop, the 1000 ms heartbeat, and the visible expanded-map 8 ms path.
- Replaced unchanged compact bridge polling with one slot-dirty `FileSystemWatcher` gate. Callbacks only coalesce a dirty bit; fixed-buffer parsing remains serialized on the UI thread, healthy notification loss is covered by a 250 ms dual-slot scan, and unavailable notifications fall back to 50 ms polling.
- Added coalesced slow-mode UI wakes for F7/F8/mode/epoch frames, disabled compact notifications during world-map presentation, added epoch-only visual invalidation, bounded partial-record retry, deterministic reader/gate tests, independent package byte/path/manifest auditing, and source-manifest build-output exclusions.
- Retired the dev30 cadence comparison as a performance acceptance gate. Runtime acceptance is now based on same-session F5/F6/F7/F8 and frame-time evidence; static/build/package gates do not claim a gameplay result.

## 0.4.0-dev66-inlineab1

- Added one-session compact-radar attribution controls: F5 suppresses custom Overlay painting while retaining the active producer/bridge/control/maintenance chain; F6 freezes the rendered scalar snapshot, skips prediction and motion-triggered repaint, and reduces control-only bridge reads to 250 ms; F7 restores the existing normal path; F8 remains complete-off.
- Upgraded the sole scalar Motion Bridge to protocol v6 with one strict three-value diagnostic enum so mixed source/Overlay packages fail closed.
- Added protocol, hotkey, no-paint, frozen-snapshot, timer, source-verifier, and package gates. Runtime smoothness attribution remains pending owner A/B testing.

## 0.4.0-dev65-hotpathopt1

- Removed normal-mode Lua F7 trace activation and eager trace-field table allocation while retaining the complete bounded trace when debug logging is explicitly enabled.
- Removed disabled Overlay performance bookkeeping, repeated per-marker UTC reads, interface-enumerator allocation in paint/query loops, unchanged encounter-availability rebuilds, and sub-half-pixel prediction invalidations without changing any feature, cadence, or marker precision threshold.
- Kept complete save snapshots and moved their worker into balanced Windows background mode; zero-only treasure categories are no longer materialized, and the diagnostic-only encounter-task table is queried only in debug mode.
- Preserved 250 ms fresh current-Pawn sampling, 50 ms compact presentation, 8 ms visible expanded-map presentation, direct vector rendering, all configured layers, and every fail-closed travel invariant.

## 0.4.0-dev64-debugoffab1

- Disabled detailed file diagnostics by default for a no-feature-loss runtime performance A/B. Normal-use lifecycle logging remains enabled.
- Preserved every dev63 radar layer, 50 ms compact presentation, 250 ms current-Pawn sampling, 8 ms visible world-map path, 45-second complete save-snapshot window, and fail-closed lifecycle boundary.
- Kept `diagnostic_verbose=false`; detailed diagnostics can still be enabled temporarily through the single authoritative `scripts/config.lua` followed by a game restart.

## 0.4.0-dev63-coldreadcoalesce1

- Coalesced continuous save fingerprints into one latest stable complete snapshot per 45-second window. The dev62 log opened a database on all 69 refreshes because `Slot1.bak` changed roughly every 20-30 seconds; the same 24-minute span is now structurally bounded to about 33 cold refreshes while two-second metadata observation remains enabled.
- Changed each permitted cache miss to build one Treasure-rich strict-superset snapshot for Treasure, Boss, Assault, and task state together. An unchanged window is served entirely from the independent `.db`/`.bak` caches.
- Retained immediate first-load, four-second stable-copy, below-normal worker, F8 invalidation, and request-scoped fail-closed publication boundaries. Added explicit requested-versus-executed Treasure-query diagnostics plus executable and structural scheduling gates.

## 0.4.0-dev62-treasuredeadlinefix1

- Kept treasure-rich cache entries compatible with encounter-only reads while making snapshot publication request-scoped: a narrow read reuses Boss/Assault and task data but never publishes cached treasure bits as a fresh treasure result.
- Prevented save-triggered encounter refreshes from indefinitely extending the 45-second treasure deadline. Only a request that explicitly includes Treasure and returns opened data may advance the next Treasure query time.
- Added executable regression coverage for narrow rich-cache reuse, retained encounter publication, and full Treasure publication.

## 0.4.0-dev61-savecacheconfig1

- Replaced the path-only save snapshot cache with query-shape-aware entries. Treasure-rich snapshots satisfy encounter-only reads, while the 45-second treasure pass and save-triggered encounter pass no longer evict each other for an unchanged `.db` or `.bak` sibling.
- Preserved exact database fingerprints, independent `.db`/`.bak` merging, the four-second change debounce, the 45-second treasure cadence, BelowNormal workers, and fail-closed publication; the optimization removes redundant SQLCipher work without weakening freshness or consistency.
- Restored visible expanded-map Lua transform production and Overlay presentation from the diagnostic 4 ms cadence to 8 ms. Compact presentation and player UObject sampling remain 50 ms and 250 ms respectively.
- Replaced the installed `config.default.lua` plus generated `config.lua` pair with one authoritative `scripts/config.lua`; runtime, Overlay, installer validation, diagnostics, and local deployment preservation now share that file.

## 0.4.0-dev60-minimapcachefix1

- Fixed the one-hertz `DLayerMiniMap` cache miss loop: only the retained top-level widget is checked with UObject `IsValid()`, while `LayerMap`, `MapOverlay`, and scale are read inside the existing protected access block.
- Missing or throwing nested-property access still fails closed and clears the top-level cache, but a usable nested UE4SS property wrapper is no longer rejected solely because its incompatible `IsValid()` result is false.
- Added regression gates that prohibit nested `LayerMap`/`MapOverlay` UObject validation while preserving one cache-miss-only `FindFirstOf("DLayerMiniMap")` site and phase timing diagnostics.

## 0.4.0-dev59-ue4ssroot1

- Standardized the runtime root on `DS/Binaries/Win64/ue4ss` and the Mod root on `DS/Binaries/Win64/ue4ss/Mods/DragonSwordWorldRadar`.
- Updated the Lua fallback resolver, installer Oodle fallback, local deployment helper, and documentation without changing F7 runtime scheduling or rendering behavior.

## 0.4.0-dev58-taskstateperf1

- Corrected the shared daily Boss/Assault recovery boundary from the player's local 09:00 to 09:00 Korea Standard Time (00:00 UTC), so availability is independent of the host time zone and daylight-saving rules.
- Added bounded, change-only diagnostics for the current save's `tb_unexpected_switch_week` rows, preserving `.db` and `.bak` provenance so the single-player encounter-task selection semantics can be established before production filtering changes.
- Changed inactive world-map detection from a 250 ms UObject validation path to notification-driven wake-up with a 2 second fallback probe. Visible expanded-map presentation remains at 4 ms.
- Split save work so the 49-record Boss/Assault query and task-table diagnostic remain on the fast save-change path while the full treasure query runs at most once per 45 seconds. No new game thread or UObject access was added.

## 0.4.0-dev57-stableactivation1

- Added a four-sample, 250 ms F7 stability gate that reads only the current player location before releasing map, clock, encounter, save, and fast-presentation work.
- Added per-encounter respawn diagnostics with actor ID, respawn type, destroy time, next availability, and hidden state.

## 0.4.0-dev56-assault60pct1

- Reduced Assault marker diameter to 60% of the corresponding world-boss marker in both minimap and world-map rendering.

## 0.4.0-dev55-worldmap4ms1

- Changed only the visible expanded-map Lua transform loop and Overlay presentation timer from 8 ms to a diagnostic 4 ms cadence for runtime A/B telemetry. Compact mode remains 50 ms and player UObject sampling remains 250 ms.

## 0.4.0-dev54-worldmapreopen1

- Added an explicit normal world-map close boundary that drops only retained widget candidates, increments their token, and preserves scalar map dimensions. Hidden prior-session widgets can no longer fill the bounded two-candidate cap and block later map opens.
- Reduced Assault marker diameter independently to roughly 78-80 percent of Boss marker size in compact and expanded-map modes.

## 0.4.0-dev53-stablecallback1

- Replaced request-specific compact-control game-thread closures with one stable Lua callback function and scalar request/epoch fields. The 250 ms sampling cadence and all UObject reads are unchanged.
- Retained the bounded F7 trace and enabled Assault reproduction path. Existing encounter lifecycle, save-state, condition, and draw diagnostics provide the staged attribution boundary without adding UObject work.

## 0.4.0-dev52-f7crashtrace1

- Added a six-second, 220-record, file-only F7 crash trace around existing asynchronous and UObject boundaries; it records scalar stage names only and performs no additional object query.
- Enabled Assault in this diagnostic deployment so the known F7 crash can be reproduced and attributed from the last durable trace record. No runtime behavior fix is claimed.

## 0.4.0-dev51-assaultfailsafe1

- Made `scripts/config.lua` the authoritative runtime Assault switch for both Lua and the Overlay. `config.default.lua` is only the installation template.
- Defaulted and upgrade-migrated `show_assaults=false` after the repeatable F7 native-crash A/B result; the generated 40-record catalog remains installed for further diagnosis.

## 0.4.0-dev50-unifiedencounter1

- Replaced the independent Boss and Assault runtime paths with one immutable 49-record world-encounter catalog, one `tb_actor_respawn` target set, one availability tracker, and one render traversal per surface.
- Moved all Boss/Assault catalog loading and save-filter configuration to Overlay startup. F7 no longer loads Assault data, changes the SQL target signature, resets the snapshot cache, or emits Assault-specific Lua lifecycle work.
- Preserved install-time current-PAK validation for nine Bosses and forty Assault targets. Optional encounter conditions remain record-owned and fail closed; the current build generates one fingerprint-pinned `world_time_window` condition, while unsupported weather semantics remain disabled.

## 0.4.0-dev49-assaultrestore1

- Re-enabled the install-generated 40-target Assault layer by default while preserving the dev48 one-shot clock, protocol-v5 scalar bridge, transition safety, and direct retained-vector rendering.
- Replaced the compiled Assault CID list in save SQL and diagnostics with the exact validated current-PAK catalog; the normalized target signature is part of the in-memory snapshot-cache identity and changes fail closed.
- Added rate-limited catalog, save-filter, availability-cache, respawn/time-gate, selection/draw-cost, F7/F8, epoch, and travel diagnostics. Time gates consume only the cached protocol-v5 local-clock scalar and perform no additional game-time UObject read.

## 0.4.0-dev48-clockrestore1

- Restored the compact game clock through the isolated one-shot provider: F7 waits for eight valid 250 ms player-context samples, reads `DGameSingleton.TimeOfDay` once, discards the wrapper, and advances the scalar locally at 60 game seconds per real second.
- Kept clock access outside the normal Pawn/map control task and added no polling loop, retry, recalibration, weather query, retained UObject, or transition-time lookup.
- F8 cancels clock work with all other Mod work. World travel hides the bridge temporarily but preserves a successfully captured numeric baseline.
- Preserved the dev47 minimap diagnostics and all current transition/performance behavior unchanged.

## 0.4.0-dev47-minimapdiagnostics1

- Added debug-only, phase-separated timing for the unchanged one-hertz
  `DLayerMiniMap` scale path. Logs now distinguish cache validation,
  `FindFirstOf`, child-object validation, scale access, result, and total time.

## 0.4.0-dev46-debugseparation1

- Separated file-based performance logging from in-map marker labels.
  `debug_logging=true` now records diagnostics without drawing coordinate or
  identity text; only `diagnostic_verbose=true` enables those visual labels.

## 0.4.0-dev45-debugbaseline1

- Enabled file-based debug diagnostics by default for the remaining
  pre-release performance tests. In-map verbose marker labels remain disabled
  by default and debug logging must be disabled again for the final release.

## 0.4.0-dev44-worldmapepochbound120hz1

- Retained dev43's epoch-bound two-candidate world-map ownership and tokenized loop/callback lifecycle.
- Restored visible expanded-map transform and Overlay presentation to 8 ms without changing the 250 ms player full-chain sampler or 50 ms compact cadence.
- Restored idempotent world-map-only `timeBeginPeriod(1)` acquisition and balanced release on map exit, suppression, F8, or form close; the startup-wide user option is never double-acquired.

## 0.4.0-dev43-worldmapepochbound1

- Replaced the unbounded retained world-map widget list with a two-candidate lifecycle-epoch/token-owned set. Reset drops all wrappers, retires bounded scalar identities, and prevents later scans from promoting previously retained old-world widgets merely because UE GC still reports them valid.
- Added unique compact/world LoopAsync serial ownership plus request tokens for queued control/world callbacks. Old instances exit on token mismatch and cannot clear or mutate a newer pending gate; logical compact/world ownership transitions directly with no overlap.
- Returned visible world-map production/presentation from 8 ms to 16 ms and removed dev42's transient mode-scoped timer-resolution request. The existing user-config startup A/B remains unchanged.
- Preserved dev42's fingerprint-bound install-generated save-owner RVA and one-shot initial snapshot optimization. Structural accumulation is removed, but attribution of the reported progressive slowdown remains a runtime hypothesis pending repeated-travel acceptance.

## 0.4.0-dev42-installrva-worldmap120hz1

- Added install-time exact-EXE owner-pointer pattern resolution, bound only to the locked pre/post game fingerprint and persisted as RVA/provenance without the SQLCipher key. Runtime validates the current EXE+PAK fingerprint before trying the generated RVA, then retains known RVAs and one delayed pattern fallback.
- Allowed only the first save snapshot to bypass the four-second change debounce because the worker still requires a stable source fingerprint before/after its consistent copy; later save changes retain the debounce.
- Raised only the visible expanded-map transform and Overlay presentation from 16 ms to 8 ms. The same WinForms timer is reused, compact mode remains 50 ms, and player UObject sampling remains one fresh 250 ms scalar read.
- Added idempotent, mode-scoped `timeBeginPeriod(1)` acquisition and balanced release on world-map exit/disable/close, without double-acquiring when the existing startup-wide option is enabled.

## 0.4.0-dev41-worldmap60hz1

- Raised only the visible expanded-world-map transform producer and Overlay presentation timer from 50 ms to 16 ms for lower dragging latency.
- Kept the dev40 safety boundary: one fresh Engine-to-current-Pawn numeric sample per 250 ms, no player reread from the world-map loop, no retained Pawn or Controller wrapper, bounded scalar prediction, forced Assault isolation, and zero clock work.
- Added bounded debug aggregates for world-map producer rate, world-map presentation rate, and deterministic mode-transition count; the existing single timers change cadence in place and stop/restore on map exit.

## 0.4.0-dev40-scalarmotion1

- Based on user-tested dev38: Assault remains force-isolated and production clock access remains disconnected.
- Removed the compact 50 ms full player-root traversal. One fresh, non-retained player sample now occurs in the 250 ms control callback; compact and large-map loops reuse the numeric sample.
- Protocol v5 adds world epoch and sample timestamp. The 50 ms Overlay predicts from two scalar samples, resets on lifecycle/outliers, clamps at 250 ms, and freezes after 500 ms stale age.
- Added full-chain sample-rate and prediction age/clamp/reset/stale diagnostics. In-game acceptance remains pending.

## 0.4.0-dev39-assaultgateclockwall1

- Based exactly on the separately packaged dev38 forced Assault-isolation stage.
- Added one serialized clock-baseline attempt per F7 after eight valid 250 ms context samples. F7 only arms state and the capture task replaces one normal control sample for isolation.
- Added DataProbe-style protected singleton validity and scalar field reads, immediate wrapper release, no retry/recalibration, and stale-token rejection before native access.
- Changed local advancement to nonnegative `os.time()` wall elapsed at 60x, so delayed/skipped callbacks do not slow the clock. Static tests do not prove native crash safety.

## 0.4.0-dev38-assaultisolation2

- Made packaged dev30 the explicit owner-accepted performance/smoothness baseline while retaining the later fresh-current-Pawn, no-cross-frame-wrapper travel-safety correction.
- Added a missing-default-true Assault isolation gate in both Lua and Overlay, so an older preserved `show_assaults=true` cannot reactivate catalog/availability maintenance, traversal, drawing, or Assault-only save diagnostics.
- Retained dev37's zero-production-clock behavior to isolate the owner’s first performance test from clock stability.
- Added debug-only split timing for current-root traversal versus `K2_GetActorLocation`. Static validation does not claim dev30 performance parity.

## 0.4.0-dev37-clockrollback1

- Removed the production `world_environment` import and every arm, refresh, cancellation, and travel callback after the delayed F7 `DGameSingleton.TimeOfDay` read reproduced an uncatchable native UE4SS crash.
- Forced world status fail-closed even when an older preserved user configuration still requests it. Marker, save-state, debounce, snapshot-cache, worker-priority, and F8 suspension optimizations from dev36 remain intact.
- Added source and scheduling gates that fail the build if production reconnects the unsafe provider.

## 0.4.0-dev36-saveclock1

- Suspended save metadata and SQLCipher work behind the F7/F8 master gate; F8 invalidates queued publications and performs no new save refresh.
- Added a four-second stable-change debounce and two-second metadata cadence before save snapshot work.
- Added per-database fingerprint caching so an unchanged `.db` or `.bak` reuses its parsed snapshot instead of being copied and decrypted again.
- Applied the SQLCipher key once per isolated temporary snapshot connection and moved refresh workers to best-effort below-normal priority.
- Added debug-only copy, key, treasure-query, Boss/Assault-query, cache-hit, and total save-refresh timings.
- Restored the clock with one delayed `DGameSingleton.TimeOfDay` baseline read per F7 activation. No UObject is retained and no retry or recalibration read occurs; local wall time advances the cached value at the confirmed 60x rate.
- Routed the conditioned Assault visibility rule through the same cached/extrapolated protocol-v4 time value.

## 0.4.0-dev35-clockfailclosed1

- Removed the production `DGameSingleton.TimeOfDay` provider after an F7 startup sample reproduced an uncatchable native UE4SS C++ exception immediately after the otherwise successful player/minimap reads.
- World-status protocol fields remain wire-compatible but fail closed as unavailable; the clock is hidden until a non-UObject provider exists.
- Preserves the dev34 unified Assault marker design and the dev33 reference-style Pawn-loss cooldown unchanged.

## 0.4.0-dev34-assaultstyle1

- Unified Assault markers with the Boss marker visual system: equal map and minimap sizing, retained diamond shadow/backing/inner border, and allocation-free gold crossed swords for distinct Assault semantics.
- Retains the validated dev33 world-transition recovery behavior unchanged.

## 0.4.0-dev33-recoveryrollback1

- Removed the dev26-dev32 dedicated world-identity recovery callback after an F8 A/B isolated the repeated dungeon crash to the active Radar lifecycle.
- Restored the proven 1.6.1 cooldown policy: three seconds with zero UObject work, followed by one normal player-location sample; every failed sample rearms the complete cooldown.
- Removed recovery-time `Pawn:GetWorld()` / `World:GetFullName()` reads and the two-sample identity state while preserving world epochs, stale-callback rejection, fail-closed bridge output, and current-Pawn sampling.
- Kept cross-frame Pawn caching disabled because its performance benefit is not independently measured and it would add a separate stale-wrapper risk.

## 0.4.0-dev32-assault1

- Added 40 install-generated Assault markers on both map surfaces using shared `tb_actor_respawn` state.
- Added generic generated time-window conditions from a versioned fingerprint-pinned inference-policy input, with exact fresh target/cycle resolution, confirmed-cycle/inferred-binding provenance, and fail-closed unavailable-time behavior; weather remains excluded.
- Bound every generated dataset and Assault catalog record to the pre-generation game fingerprint and reject installation if the post-generation identity changes.
- Added a retained warm-orange crossed-swords diamond vector layer with independent diagnostics and no bitmap cache.
- Preserved protocol v4, one bridge/save reader, existing cadence, and the then-current dev31 travel lifecycle.

## 0.4.0-dev25-secondary1

- Reduced foreground, visibility, and minimized-window API sampling to 250 ms while keeping cached visibility decisions on every Overlay tick; F7/F8 and radar/world transitions force an immediate fresh sample.
- Added the startup-only `high_resolution_timer` A/B setting. It defaults to `false`; `true` requests `timeBeginPeriod(1)` and retains balanced `timeEndPeriod(1)` cleanup.
- Added debug-only per-layer draw calls, rendered marker counts, average time, and maximum time for treasures, Bosses, and Mole/Fly markers alongside the existing world-status and total-paint evidence.
- Added debug evidence for the actual window-visibility sampling rate and corrected world-map Mole timing to record once per layer pass instead of once per marker.
- Preserved dev24's 50 ms motion/paint cadence, half-pixel compact-motion threshold, one-second maintenance/save checks, reward-bit Mole/Fly completion, and all dev21 map/index correctness behavior.

## 0.4.0-dev24-motion20hz1

- Capped the Lua radar/world-map producers and active Overlay consumer at a coherent 50 ms (20 Hz) cadence to reduce moving bridge, paint, and layered-window composition work.
- Suppressed compact player motion smaller than half a projected radar pixel, using a radius-derived threshold with the accepted 20-unit safety floor; no world-map transform or treasure-index cache was added.
- Reduced save-slot metadata checks and Overlay maintenance to one hertz, and stopped filesystem metadata polling for installer-generated treasure/Boss catalogs after their first validated load.
- Added debug-only visual, invalidate, paint, refresh-duty, paint-duty, and composed-pixel-rate evidence for runtime A/B evaluation.
- Preserved the dev21/dev23 map ownership, save-filtered treasure index, direct vector rendering, 34 reward-bit Mole/Fly filter, F7/F8 master gate, and one-second nearest-treasure selection.

## 0.4.0-dev23-molereward1

- Mapped all 34 Fly activities `11001-11034` to their exact same-ID `DT_MiniGame_G5` reward treasure records.
- Reused the existing external `tb_treasure_box.OPENED_BIT_FIELD` snapshot to hide a Fly marker after its reward is claimed, without any game UObject query.
- Added a raw save-bit path isolated from treasure ignore/alias overrides so the treasure-specific `11003` ignore rule cannot hide Fly `11003` incorrectly.
- Added fail-closed startup, full 34-bit visibility, first/last claimed reward, and `11003` override-bypass regression coverage.

## 0.4.0-dev22-molequerysafe1

- Disabled `DETUtil.CIsClearMiniGameInStandAlone` resolution and invocation after three reproducible native UE4SS crashes immediately following F7 activation across quest/world lifecycle transitions.
- Kept the validated 34-record Mole/Fly catalog and publishes all markers while completion state is unavailable; treasure, Boss, map, clock, protocol, and rendering behavior are unchanged.
- Added negative structural validation preventing the unsafe UObject query path from returning unnoticed.

## 0.4.0-dev21-dev15rollback1

- Restored the packaged dev15 runtime behavior for treasure visibility indexing, compact map ownership, nearest-query buffering, Overlay diagnostics, native window interop, and Boss/Mole vector rendering.
- Removed all dev16 composition-region behavior and all dev18-dev19 retained-map, map-key, and radar-map debug behavior.
- Retained only the approved 33 ms Lua/Overlay active cadence, 1000 ms nearest selection, and whole-mod F7/F8 clock lifecycle.
- Moved the transparent 24/15-reference-font time/phase group into a six-pixel-gapped 52-pixel strip below the 360-pixel compact minimap square.
- Preserved the accepted data rules across the rollback: fixed chest 10220122 is not ignored and its obsolete ignore is removed on upgrade, while duplicate/offset record 11003 remains ignored in favor of authoritative 14016.

## 0.4.0-dev20-vectorrenderrollback1

- Removed the dev17 Boss/Mole bitmap marker caches, cache blit paths, runtime vector toggle, and cache-specific diagnostics completely.
- Restored direct retained-geometry vector rendering on every Boss and Mole marker paint, matching the packaged dev16 behavior.
- Added executable vector-renderer smoke coverage and negative structural gates preventing the removed cache symbols from returning.

## 0.4.0-dev19-treasurecorrectness1

- Restored fail-closed treasure visibility until the first valid save snapshot, removing the temporary full-catalog layer and its already-open markers.
- Restored the supported compact radar to map 100 and removed generation-retained world-map state that could survive teleports or region changes; expanded world-map rendering remains keyed to the current frame map ID.
- Added map ID to the nearest-treasure query-cache key so a source-map change always rebuilds before the normal one-second interval.
- Added executable regressions for unknown-save hiding, ready-save filtering, map-key invalidation/reset, fixed compact ownership, and unchanged world-map map ownership.

## 0.4.0-dev18-masterab5-startupfailopen1

- Changed startup treasure visibility from an empty unknown-save index to the complete catalog grouped by map, preventing blank compact/world-map treasure views while the first save snapshot is pending.
- Rebuilds atomically as soon as the first save snapshot becomes available or its version advances, filtering confirmed-open IDs. Already-open boxes may briefly appear during startup.
- Added executable unknown-save/full-catalog and loaded-save/opened-filter regression coverage while preserving generation-scoped compact map retention.

## 0.4.0-dev18-masterab4-mapretention1

- Fixed compact treasure selection to use the most recently observed valid world-map map ID instead of hard-coded map 100, with a safe pre-observation default and generation-scoped reset that preserves F8/F7 state.
- Added bounded debug-only radar map evidence for treasure candidates/selection and Mole catalog/mask/range counts.
- Confirmed no Mole map-split defect in current data: all 34 generated records are map 100, world-map rendering already filters by frame map ID, and compact rendering correctly evaluates all records by mask and distance.

## 0.4.0-dev18-masterab3

- Corrected installer completion guidance and default configuration comments to state that F7 enables the complete configured mod including world status and F8 disables every mod feature for FPS comparison.
- Added source gates that reject the obsolete marker-only F7/F8 installer claim.

## 0.4.0-dev18-masterab2

- Reduced hidden master-disabled Overlay bridge polling from 125 ms to 500 ms while retaining the one-second game-lifetime shutdown check.
- Skipped periodic window geometry placement while disabled; the first transition back to radar/world mode still forces immediate placement.

## 0.4.0-dev18-masterab1

- Restored F7/F8 as a whole-mod runtime A/B gate: F7 enables configured markers and world status; F8 hides everything and stops radar/world-map producers, Mole/Fly queries, and world-time work after one disabled lifecycle publication.
- Preserved configured layer choices without rewriting `config.lua`; context recovery cannot bypass the F8 master-disabled state.
- Removed the active status-after-F8 control and Overlay presentation paths while retaining one process, one window, and protocol-v4 bridge.
- Closed cross-restart Mole/Fly persistence by design. Each launch performs the bounded fail-closed initial scan, and the existing session permanently stops completion calls after all 34 are complete.

## 0.4.0-dev17-moving33-markercache1

- Changed Lua radar/world-map motion producers and active Overlay polling from 24 ms to 33 ms without adding adaptive logic.
- Changed nearest unopened radar treasure selection from 250 ms to 1000 ms; selection continues to consume only the save-filtered active index and keeps the 80-point cap.
- Added retained DPI/scale-keyed Boss and Mole bitmap caches with allocation-free blits, automatic vector fallback, and `EVENTRADAR_VECTOR_MARKERS=1` rollback.
- Added cache blit/vector/rebuild/time diagnostics. Runtime FPS benefit remains an in-game A/B gate.
- Removed the obsolete stock `ignore 11003` rule and its comments. User override files remain owned and preserved.
- Did not add unsafe cross-restart Mole completion persistence: the Lua completion producer has no stable save/account identity, while the Overlay-only database stem is not a proven account identity and is not available through protocol v4.
- Recorded the future Boss/Assault permanence contract: persist next-due time only when keyed to a proven save identity; do not query before due after restart.

## 0.4.0-dev16-compositionregion1

- Limited compact Win32 composition exposure to the union of the complete right-hand radar square and a padded left status rectangle; status-only mode excludes the radar region and world-map mode restores the full rectangle.
- Applied region changes only on presented-mode or geometry/scale transitions, with correct transferred HRGN ownership and rectangular fail-open behavior.
- Added change-only debug region mode, approximate visible-pixel area, and ratio evidence explicitly labeled as not equivalent to DWM GPU savings.
- Preserved all dev15 producer cadence, adaptive behavior, marker caching, treasure selection, Mole scheduling, world-time scheduling, bridge, process, and window counts. Runtime FPS effect remains unvalidated.

- Preserved the bounded five-per-callback initial Mole/Fly snapshot and replaced four-per-second steady polling with a paced full sweep of unfinished IDs across one minute, never exceeding one UObject query per 250 ms callback.
- Added gameplay-context suspension, permanent all-complete shutdown, and bounded 10/30/60/120/300-second failure backoff while preserving the fail-closed mask.
- Replaced one-hertz `TimeOfDay` reads with local 60x extrapolation accumulated from the established 250 ms control callback and source recalibration once per in-game hour. This avoids portable-Lua `os.clock`/wall-clock assumptions; the 60x rate remains runtime-gated.
- Suppressed exact-seconds-only bridge writes, repaired Mole debug diagnostics, added aggregate layer timing, cached the debug Fly font, and moved research-only Assault detail to debug.
- Made world time automatic and independent of F7/F8 using the existing 250 ms control path; status-only mode never starts 24 ms motion production.
- Extended the compact transparent overlay 170 reference pixels left, kept the original 360-pixel minimap square fixed on the right, and placed a clamped 150-pixel horizontal status group 12 pixels left of the minimap circle.
- Increased reference time/phase fonts to 24/15 pixels and retained full-screen world-map omission.
- Fixed context-loss and `mods.txt=0` lifecycle frames to clear published time/weather availability before serialization, preventing stale status display.
- Centralized the F7 marker predicate as treasures, bosses, or Moles; world status and treasure-only height/type decorations cannot start 24 ms motion production.

## 0.4.0-dev14-timephase1

- Removed user-facing `W/BT` values and stopped querying the invariant weather manager scalars.
- Enlarged the transparent clock and circular icon while keeping the group against the extreme top-right edge.
- Added retained phase-specific visuals and labels for `MORNING` (06:00), `AFTERNOON` (12:00), `EVENING` (18:00), and `NIGHT` (21:00), with night continuing until 06:00.
- Reduced world-status sampling from three scalar properties to one cached `DGameSingleton.TimeOfDay` read per second.
- Added exact boundary tests for all four time phases.

## 0.4.0-dev13-molecatalogfix1

- Fixed the production Overlay catalog parser to recognize Lua Boolean and `nil` literals, matching the owner-tested standalone Mole parser.
- Restored parsing of generated `has_z = true` fields so all 34 validated Fly records can load instead of being skipped at runtime.
- Added a 34-record parser-contract regression test covering unique IDs, contiguous mask bits, and Boolean `has_z` values.
- Added source verification markers that reject removal of the shared Boolean-literal parsing contract.

## 0.4.0-dev12-worldstatus-layout1

- Removed the opaque rectangular world-status background and border so only the dial, weather glyph, and outlined text remain visible.
- Moved the circular day/night dial to the extreme top-right and placed the time/weather text to its left, reducing overlap with the circular radar map.
- Kept the same cached one-hertz sampler, protocol-v4 record, minute-level redraw filtering, and retained rendering resources.

## 0.4.0-dev11-worldstatus-singlebridge1

- Added a compact time and weather-status panel to the upper-right of the radar window; it is not drawn over the expanded world map.
- Added read-only, cached scalar sampling of `DGameSingleton.TimeOfDay`, `DsEnvironmentManager.CurrentWeatherState`, and `CurrentWeatherBTState` at one sample per second inside the existing 250 ms control callback.
- Kept startup time fail-closed until a nonzero clock value has been observed, after which midnight zero is accepted.
- Displayed weather as neutral raw `W` and `BT` values because the current evidence does not yet prove semantic labels such as sunny or rainy.
- Extended the sole Motion/control bridge to protocol v4 with 35 fields; legacy and malformed records remain rejected.
- Added retained drawing resources and visual-delta filtering so unchanged status values do not create extra redraw work.

## 0.4.0-dev10-molefly-singlebridge1

- Integrated the owner-tested 34-record `MiniGame_Fly` layer into the production Radar without adding a Host, watcher, Overlay, scheduler, or bridge.
- Added install-pinned Fly catalog extraction with exact classification, stable IDs, contiguous mask bits, section/map indexing, and fail-closed shape validation.
- Added bounded read-only completion checks through `DETUtil.CIsClearMiniGameInStandAlone`: five initial reads and one unfinished read per existing 250 ms callback.
- Extended the single Motion/control bridge to protocol v3 with 29 fields for `showMoles` and the 34-bit visibility mask; older records are rejected.
- Added a retained, code-native winged upward-arrow renderer for minimap and world-map Fly markers.
- Preserved treasure, Boss, save-key startup waiting/recovery, F7/F8 lifecycle, and accepted 24 ms active cadence.

- Rebased the performance work on the accepted 1.7 Stable Core cadence: 24 ms minimap/world-map motion, 250 ms control sampling, 24 ms active Overlay polling, and 20/10 XY/Z thresholds.
- Removed the active Static JSON Bridge, its one-second Lua heartbeat write, the Overlay's 200 ms Static file poll, `JavaScriptSerializer`, and static/motion fallback merging.
- Introduced explicit Motion protocol v2 with 27 strictly validated fields, alternating slots, generation/sequence ordering, and leading/trailing sequence equality.
- Moved fixed treasure and nine-Boss catalog ownership to the Overlay; Lua publishes only live UObject/control state.
- Added a 2500 ms Motion stale timeout that hides obsolete output and automatically restores on the next valid frame.
- Kept the effective immediate world-map transition: hidden no-copy resize, one synchronous prepaint, then immediate reveal; closing the map immediately restores the small radar.
- Installer upgrades remove only the three obsolete 1.7 source files that folder overwrite cannot delete.
- Preserved the save-startup visibility gate, buffered snapshot flush, delayed full-EXE key scan, and exact process-bound watcher lifecycle.

## 0.4.0-dev9-performance1.4-30hz1

- Synchronized the Lua minimap/world-map motion producers and the active Overlay consumer at 33 ms (~30 Hz); the prior source-only 33 ms Overlay edit had left the runtime producer at 24 ms.
- Kept static state at 250 ms and retained 50/75/125/500 ms idle, disabled, and background Overlay intervals.
- Removed the obsolete default `ignore 10220122` rule because the current game data exposes that chest again.
- Installer upgrades now remove only the exact deprecated `ignore 10220122` rule from a preserved `data\treasure_overrides.txt`; other user overrides are preserved.
- Session-overlap audit confirmed one resident hidden WScript watcher, one game-bound host, mutex-protected duplicate host startup, and clean host/Overlay teardown when the game exits. No extra runtime watcher process is created by F7/F8 toggles.
- No bridge schema, Boss rule, marker style, save schema, or world-map lifecycle change.

## 0.4.0-dev9-performance1.3-mapinstant-hiddenhost1

- Removed the 1000 ms world-map reveal delay and all associated warm-up state; map entry and exit now restore the Overlay in the same timer cycle.
- Kept hide-before-resize, added `SWP_NOCOPYBITS`, and synchronously repaints the newly sized hidden surface before `SW_SHOWNOACTIVATE`. A paint-sequence check skips the second full-screen invalidate only when hidden prepaint actually completed. Debug mode records `MAP_SURFACE_PREPARED` timing. This is a zero-timer attempt to suppress stale layered-window pixels without retaining a full-screen surface outside map mode.
- Restored the proven resident hidden WScript launcher used by the stable baseline. Game-time Lua now writes only `runtime\launch.request` and no longer invokes `cmd.exe`, `wscript.exe`, or PowerShell, eliminating the transient console-window path.
- The installer recreates and validates the hidden user Startup watcher; the game-bound PowerShell host remains hidden and exits with the exact game process.
- Classified the expected pre-login save-key-not-ready state as Debug-only instead of writing a normal-use stack trace.
- The diagnostic game executable is 162,551,704 bytes; normal startup no longer reads it before the known RVAs have had 30 seconds to resolve.
- Launch request stamps are digits-only and newline-free; WScript validates them before command construction, fixing the observed broken WatcherHost log lines and reducing parameter-injection surface.
- Changed save-key discovery to try cached/current/legacy RVAs first. A full game-EXE signature scan is now a one-time worker-thread fallback after 30 seconds; transient owner/key-not-ready states neither trigger it early nor disable it, and the Overlay UI thread is never blocked by that scan.
- No bridge fields, treasure/Boss semantics, save schema, marker style, F7/F8 behavior, or active map-producer intervals changed.

## 0.4.0-dev9-performance1.2-mapfix-debug1-cleanup1

- Removed Overlay runtime polling for `debug_logging`/`diagnostic_verbose`; the setting is parsed once at process startup, so normal Timer ticks and debug-log guards perform no config-file stat/read work.
- Removed runtime Boss RespawnCycle PAK enumeration, binary-to-text decoding, regex parsing, the 30-second debug rescan path, and the associated locks/state.
- Preserved Boss availability semantics with the verified fixed rule-106 schedule: daily reset at 09:00 local time.
- Removed the save-worker call that existed only to refresh the optional Boss rule scanner.
- Added source-verifier assertions that reject reintroduction of Debug hot-reload polling and runtime Boss PAK scanning.
- No bridge schema, save-table interpretation, marker behavior, F7/F8 behavior, or map lifecycle logic changed.

## 0.4.0-dev9-performance1.2-mapfix-debug1

- Removed the always-full-client radar window introduced after `performance1.1`; radar mode again uses an actual small top-right layered window.
- Changed world-map lifecycle handling to hide the Overlay before resizing, keep it hidden for 1000 ms after every map entry, and restore the small radar immediately on the first confirmed map-close read.
- Removed the 96 ms always-on map UObject detector, three-miss exit confirmation, and 250 ms stale re-entry block from the 24 ms radar motion path.
- Restricted 24 ms world-map UObject sampling to the period in which the world map is actually active, and reused the active transform in the 250 ms static publisher.
- Made Lua game-thread queue/callback gates recover after scheduling or callback exceptions instead of remaining permanently pending.
- Split Lua and Overlay output into low-volume `Use` logs and opt-in `Debug` logs. Normal mode no longer computes or writes periodic performance diagnostics.
- Expanded debug diagnostics with producer queue/update latency, 50/100/250 ms stall counts, motion sample/write rates, Overlay timer/paint gaps, normalized process CPU, working set, visibility, and geometry. `overlayPaintFps` is explicitly labeled as an Overlay metric rather than game Present FPS.
- Reduced optional Boss respawn-rule PAK discovery to one scan per Overlay process in normal mode; periodic rescans remain available only in debug mode. PAK enumeration failures now preserve the daily 09:00 fallback and cannot abort save-state refresh.
- Added transient game-process and game-window failure recovery, log-session rotation, and exact source-verifier rules for the repaired map lifecycle.
- Hardened Overlay launch retry and serialized, before/after-consistent debug-config reads across UI/save worker threads; the normal timer checks for debug-mode changes only once per second.
- Removed the no-op Boss tracker call from the 250 ms static-state build.
- Excluded generated `runtime`/patch-deployment backups from the source-only unexpected-EXE scan while retaining SHA-256 enforcement for the single bundled `ooz.exe`.
- Static validation was completed in Linux. Windows PowerShell 5.1 `Add-Type` compilation and in-game FPS/transition validation remain mandatory deployment checks; `Install.cmd` must print `OVERLAY_COMPILE_OK`.

## 0.4.0-dev9-performance1.1

- Reduced compact-motion bridge writes through cumulative visual-delta filtering while retaining a one-second liveness heartbeat.
- Changed active minimap and world-map sampling to 24 ms without changing the fixed 21-field bridge protocol.
- Added adaptive Overlay polling at 24/50/75/125 ms for active, world-idle, radar-idle, and disabled states.
- Prevented unchanged bridge slots, heartbeat-only motion frames, and visually equivalent static frames from causing repeated reads, parsing, invalidation, or paint.
- Batched non-nearest treasure markers into retained type-specific `GraphicsPath` instances.
- Reused a validated game process ID across geometry, lifetime, and save polling.
- Added scheduler/write-suppression counters to Lua and Overlay diagnostics.
- Fixed the Windows PowerShell 5.1 CodeDOM local-variable shadowing error in `MotionVisualSnapshot.Update`.
- In-game diagnostics confirmed normal operation, adaptive timer transitions, zero bridge/write failures, and materially lower idle/active work.

## 0.4.0-dev8-refactor2

- Fixed the refactor1 Windows PowerShell 5.1 `Add-Type` compiler blocker.
- Added mandatory real-compiler and regression-harness gates to release builds.
- Added SHA-256 package integrity validation and exact immutable-tree checks before installation.
- Made datasets, preserved config, overrides, metadata, `mods.txt`, and startup shortcut one staged, rollback-protected transaction; watcher readiness is the commit point.
- Isolated treasure and boss save-state failures, retained last-known-good snapshots, and added bounded partial-module retry.
- Added bounded bridge/catalog reads, PAK/decoder guards, canonical ZIP validation, and per-record rendering isolation.
- No intended feature, marker-style, bridge-protocol, schema, hotkey, or sampling-interval change.

## 0.4.0-dev7-stable6

- Frozen dev7 stability baseline after in-game validation of F7/F8, minimap and world-map double buffering.
- Restored the complete original dev7 rendering implementation before applying narrowly scoped visibility and marker changes.
- Overlay remains active after F7 until F8; foreground, overlap, and minimized-window visibility polling are removed.
- Host and watcher processes use a temporary working directory rather than holding the Mod folder as their current directory.
- Main Radar contains no experimental Boss, treasure, or sudden-mission collection hooks.
- Treasure and Boss marker colors share `RadarMarkerStyle`; Boss uses the accepted warm-red visual.
- Treasure outlines use the shared dark outline. World-map paths use `FillMode.Winding` and projected-pixel deduplication.
- Installer validates/repairs `config.lua` and compiles the exact complete Overlay source set before installation succeeds.

## 0.4.0-dev5

- World-map refresh now runs at 8 ms while dragging or zooming and returns to 16 ms after 250 ms of stable input.

- Reduced radar motion sampling from 16 ms to 32 ms while retaining the 16 ms world-map producer.
- Producer loops now terminate completely while disabled and restart only on F7.
- World-boss Character discovery runs at most every 5 seconds and only near a known spawn.
- Installer now changes only the DragonSwordWorldRadar entry in mods.txt.

- Added a local-PAK `BossDataProvider` that generates exactly nine world-boss records.
- Added independent UE4SS world-boss runtime tracking and bridge payloads.
- Added a larger unified marker modeled on the game's field-boss map icon to the minimap and world map.
- Added death/disappearance hiding and actor-respawn restoration for previously observed bosses.
- Added independent `show_bosses` and `show_treasures` rendering controls.
- Changed the treasure player-height reference offset from `-120` to `-150`.
- Fixed release packaging to include the validated bundled `tools/ooz.exe`.
- Updated source verification to allow only the declared `ooz.exe` tool binary.

## 0.3.2c

- Established the standalone DragonSwordWorldRadar repository and release layout.
- Removed the installation dependency on DragonSwordTreasureMap 1.6.1.
- Added one-click local treasure-data extraction and generation.
- Added a reusable `IDataProvider` pipeline for future radar layers.
- Added game-version fingerprinting and launch-time reinstall prompts.
- Preserved the completed treasure layer.

## 0.4.0-dev31-freshpawn1

- Removed the cross-frame Pawn cache after a dungeon transition crashed in UE4SS at the exact `player_access_failed` boundary.
- Matched the supplied 1.6.1 player path: cache Engine only, then resolve the current Controller and Pawn for every position sample.
- Added regression gates that reject any retained `player_pawn` wrapper while preserving zero `FindFirstOf`/`FindAllOf` calls in the stable position path.
- Preserved delayed two-sample World recovery, direct rendering, and existing 50/250/1000 ms scheduling.

## 0.4.0-dev30-referencecache1

- Replaced repeated stable-path root acquisition with the supplied 1.6.1 cached Engine -> Controller -> Pawn policy; Engine lookup now occurs only on cache miss/recovery.
- Restored the supplied 1.6.1 cached `DLayerMiniMap` scale reader; the widget is re-found only after absence/invalidity.
- Completely removed the custom task/cutscene HUD visibility scan and `surface_hidden` state machine, including `IsVisible`, `DPanelMain`, and `Overlay_Minimap` access.
- Kept world-epoch fail-closed recovery, F7/F8 master control, expanded-map handling, and 50/250/1000 ms scheduling without adding object enumeration.

## 0.4.0-dev29-pawnvisibility1

- Removed the dev26-dev28 UE4SS `LoadMap` pre/post hooks after two identical startup crashes and restored the stable 1.6.1 Pawn-loss plus delayed-widget-rescan lifecycle.
- Retained world epochs and queued-callback rejection, but recovery now begins after root/Pawn loss, waits three seconds, and requires two matching fresh World identities.
- Replaced the 250 ms `DPanelMain`/`Overlay_Minimap`/`DLayerMiniMap` visibility chain with one freshly acquired and immediately validated `DLayerMiniMap` sample per second.
- Kept hidden-HUD bridge shutdown and expanded-world-map independence without retaining a compact HUD widget wrapper or adding a scheduler.

## 0.4.0-dev28-hudvisibility1

- Added a fail-closed compact-surface gate using the existing 250 ms control callback and current `DPanelMain`, `Overlay_Minimap`, and `DLayerMiniMap` visibility.
- Hidden cutscene/menu HUD now publishes one `surface_hidden` bridge frame, hides the Overlay, context-disables world-time/Mole work, and stops the compact 50 ms producer until the HUD returns.
- Kept expanded world-map detection and rendering independent from compact minimap visibility, including a fail-closed map-close handoff.
- Added executable/static HUD-surface scheduling contracts without adding a fourth Lua scheduler or changing protocol v4.

## 0.4.0-dev27-transition2

- Independently reviewed the dev26 transition lifecycle against the repeated UE4SS access-violation evidence and the installed UE4SS LoadMap API.
- Removed unused retained Engine/PlayerController wrappers and stopped retaining the one-hertz minimap widget and low-frequency world-time singleton across UI/world rebuilds.
- Replaced the large-map per-property shared-generation lookup with a Lua-only lifecycle-epoch guard while preserving one generation check at each queued callback boundary.
- Strengthened scheduling tests to reject reintroduced low-frequency UObject retention and high-frequency shared-generation lookups.

## 0.4.0-dev26-transition1

- Added a fail-closed LoadMap pre/post lifecycle that purges every retained UE4SS UObject reference before world destruction and resumes only after two stable fresh-root samples from the new world.
- Added per-world epochs to reject stale queued control, radar-motion, world-map-motion, recovery, provider, bridge, and diagnostic work after transitions.
- Made F7 defer safely during loading/incomplete world roots and made F8 cancel deferred activation while invalidating all queued work immediately.
- Added concise transition enter, purge, post-load, and resume diagnostics plus executable/static lifecycle regression contracts.
## 0.6.0-dev5-d3d12-late-present-canary

- Runtime rejected: the first F7 D3D12 submission triggered a confirmed GPU
  crash. The installed acceptance configuration is fail-closed and the direct
  queue injection path must not be re-enabled without exact presentation-queue
  ownership from the engine render lifecycle.
- Added a D3D12 late-Present canary path after dev4 runtime evidence proved
  that the active game swap chain was not D3D11.
- Captured the matching game-owned direct command queue through a chained
  `ExecuteCommandLists` detour and retained only native COM resources.
- Added cached per-back-buffer RTVs and command allocators, explicit
  `PRESENT`/`RENDER_TARGET` transitions, and small `ClearRenderTargetView`
  rectangles for the cyan and relative-marker witnesses.
- Skip a canary frame when its command allocator is still in flight instead
  of waiting inside `Present`; source defaults remain disabled.
