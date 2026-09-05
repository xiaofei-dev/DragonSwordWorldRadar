# Runtime Feedback Audit for 2.2.0

Version 2.2.0 is the feature release for shared mini-game height guidance, independent
height controls, localization, and the responsive F6 settings page. This audit
does not relabel the 2.1.1 source/build/package/deployment evidence or its
`B89F8584274850ADC35D0703725A14F00A14F0F9093574DB09F022E0B68F2F3B`
DLL as 2.2.0 evidence.

## Intended 2.2.0 behavior

| Area | 2.2.0 contract | Runtime evidence required |
| --- | --- | --- |
| Treasure height | Existing category-colored six-piece full arrow remains unchanged to the left of the selected chest; default `ON`; compact radar only. | Confirm above/below direction, category color, unchanged six-piece geometry/left-side placement, default, persistence, and simultaneous display with the other channels. |
| Area Quest height | Default `ON`; compact radar only; every visible task uses its generated one- or two-band profile. A multi-band profile uses authored marker Z to select the uniquely nearest existing source band; marker Z is selection evidence only and never synthetic height. Inside the selected band's inclusive +/-500 margin the original black frame shows three white dots; below it points up and above it points down. An exact-distance tie or missing profile remains neutral. There is no separate arrow or Treasure-style horizontal offset. The 147-row catalog has 144 profiles, one genuine two-band profile, and three no-source rows; Move_Check-only trigger bands are excluded when a real task-actor band exists. | Confirm default, persistence, every simultaneous visible task, exact +/-500 boundaries against comparable `playerZ - 150`, stable marker-center placement, selected-band up/down/aligned states, and tie/no-source neutral behavior. |
| Mini-game height | Fly, Mole, and Wave share one nearest-mini-game channel; default `ON`; compact radar only. A shaftless triangle stays centered directly below the selected icon, uses that marker's actual kind palette, and adds a near-black contrast outline without changing size, position, or projection. Target more than 500 vertical units above comparable `playerZ - 150` shows up, target more than 500 below shows down, and the inclusive +/-500 band hides the triangle. All 83 map-100 Fly/Mole/Wave records (33/40/10) use exact trusted `NPC_Start` heights; the persisted key remains `mole`. | Exercise representative Fly, Mole, and Wave targets above, below, and inside the +/-500 band. Confirm below-icon placement, unchanged geometry/projection, actual kind color, outline, direction, dead zone, and that missing or ambiguous height hides only the shared mini-game triangle. |
| F6 layout and control | One responsive page with top-bar Bug Report and Close controls, read-only Mod Status text plus a thin state-colored strip, a separate Enable/Disable/Retry action that keeps the page open, centered Language, Marker Visibility, Height Indicators (Radar Only), and Filter Modes. Enable still requires a playable world. Translucent section cards, equal-width filter choices, aligned text, and bounded non-overlapping hit regions are presentation-only. On first open and after language changes, status changes, or language-popup display, an event-driven measured desired-size pass recenters exact font-layout TextBlocks. `GetDesiredSize` must return the known `Vector2D` structure identity; unavailable or invalid evidence and the render-scale fallback retain authored text geometry. It changes no button hit box, map geometry, or per-frame path. | Verify all three status/action states, menu persistence after the action, unavailable-world rejection, full visibility, vertical centering after all four presentation paths, click targets, popup dismissal, close behavior, configuration persistence, and the fixed Nexus Posts URL at supported windowed/fullscreen/DPI/aspect-ratio combinations. |
| Localization | English, Japanese, Korean, Simplified Chinese, Traditional Chinese, French, German, Spanish (Spain), Russian, Thai, and Portuguese (Brazil). F6 prefers a compatible game `DTextBlock`; `ForceApplyLanguageFont` and a compatible CDO composite font are best-effort. Exact-size game/base paths remain bounded and verified. The canonical `assets/ui/f6` payload contains six status-specific ko/zh-Hant main overlays, one shared language-popup overlay, and its manifest. Each main overlay replaces all 30 fixed main-panel text slots; the popup replaces only the two affected language names. These assets are regenerated for the current top-bar, status, and filter-row coordinates from pinned DroidSansFallback at base size 32, with a one-pixel translucent stroke and role-specific optical baselines; the other nine languages remain on native game fonts. If both text paths fail, a fresh DTextBlock then base TextBlock fallback leaves Font untouched and uses the same bounded viewport/DPI render scale with a justification-aware pivot; `target_size=0` excludes those records from post-prepass Font.Size loops. Core UMG creation, `SetText`, tree insertion, and viewport attachment remain fail closed. | Review terminology, truncation, exact-size and render-scale paths, font/glyph coverage, all 30 main-panel overlay slots, the two popup names, overlay size/weight/alignment, layout, and persistence in every language. Static generation and no-clipping checks cannot replace this live visual review. |
| Game language | F6 shows only the 11 explicit languages. A legacy AUTO value migrates on the next actual F6 opening or F7 activation. Four exact weak game composite UFonts retry only on a real F6 open when missing/expired. `DS_HYFont_P.pak` overrides Common/TC without complete glyph coverage; disable/replace it for QA. Raw Pretendard FontFace is not routed as UFont. | Seed legacy AUTO, prove migration and persistence, disable the external PAK for all-script QA, and prove no displayed AUTO or recurring language/font work. |
| Expanded world map | Two 3072 atlases at style revision 50 provide 50 percent more linear raster density and use about 72 MiB raw decoded BGRA memory versus about 32 MiB at 2048. Capacity is 4,096 against the accepted maximum of 2,500 Treasures plus 279 fixed non-Treasure rows (2,779 total), leaving 1,317 spare slots. Marker coordinates, projection, zoom handling, native parent ownership, and outer/inner container geometry remain unchanged. Same weak parent identity still returns `Unchanged` with no Remove/Add, reparent, or extra render; only a real weak parent identity change reparents. No `8000` geometry expansion is used. The final F6/localization/compact-indicator closeout changes no expanded-map source or contract. | Prove alignment, zoom, dense-Treasure stability, revision 50 visual quality, attach time, and memory against the exact artifact at windowed, 16:10, 16:9, 21:9, and varied DPI. Runtime visual acceptance remains `NOT_VALIDATED`; static gates do not establish it. |
| Controller menus | Preserve the 2.1.1 controller-independent map/pause suppression contract. Only `SetWorldMapImage` may create the map-open latch; a constructed layer's `IsVisible` result cannot suppress the first F7 attachment. | Test F7 before ever opening the map, then test a physical controller with no hardware cursor; confirm compact markers attach immediately, hide while the map is open, and restore without reading controller mappings. |
| Performance | Reuse fixed-capacity channels and existing bounded services; add no steady scan, SQL, filesystem access, PAK extraction, or language poll. | Capture same-session frame-time and diagnostics-off evidence with all three height channels and the F6 page exercised. |

### Static localized-overlay evidence

The regenerated 1360-by-1320 RLE-TGA payload uses Bug Report slot
`(411,17,126,26)` at role scale `0.40`, Close `(559,17,94,26)`, status
label/value/action `(32,142,98,24)` / `(158,142,154,24)` /
`(435,142,208,24)`, and filter text at X `334` / `496`, width `146`, Y `583`
/ `615`. Tight-alpha placement centers Bug Report on both axes. The strict
verifier audited all 11 runtime text blocks, covered `123/123` overlay
codepoints, reported minimum fit `1.000`, maximum optical-center error `0.5`
raster pixel, minimum ink height `0.500`, no edge alpha or slot overflow, and a
clean deterministic `8/8` regeneration including `manifest.json`. The manifest
SHA-256 is
`F8458AADFBFE2EB250222B3427122047C4745F619B9B47452C88508ECC9D3044`.
This is static asset evidence, not live F6 typography acceptance.

## 2.1 public-report rationale

The Nexus Posts page was reviewed on 2026-09-01. Multiple 2.1 users reported
that enabling Treasure MAP markers made the expanded map flash or shift while
other categories did not, and that the issue was most visible with dense
uncollected-Treasure saves. The initial full-parent outer-host hypothesis was
implemented, but live screenshots then disproved that placement as an alignment
fix: it moved the atlas offset into the inner Image and displaced the complete
marker layer. The current replacement restores the outer native Canvas slot to
the atlas parent-local rectangle and the `Panel_Point` Image to local zero.
That correction still needs exact-DLL live alignment acceptance. It does not
establish the cause or resolution of dense-Treasure flicker, which is reopened
for separate runtime diagnosis.

The latest bounded diagnostic snapshot contained 1,632 total markers, including
1,501 Treasures, below the old 1,785 capacity. Capacity therefore was not the
flicker root. The same current log records one attach and no repeated detach/
rebuild sequence. The 4,096 capacity is future headroom, not a flicker-fix claim;
dense-map behavior still requires exact-artifact live acceptance.

A separate uploaded 2.1 log showed `WORLD_MAP_ATLAS_ATTACH_FAILED` after map
data/class readiness remained unavailable. That empty-expanded-map failure is
not evidence for the dense-layout offset root cause and remains a separate
attachment-readiness diagnostic case.

The first locally deployed 2.2.0 candidate with DLL SHA-256
`F7C3E6177A87202B707EDC39F6A4C592071E54DB6B284A13896A496784A30668`
received F6 and resolved `zh-hans`, but rejected the initial header with
`failure=8`, `abi_failures=0`, and `font_abi_details=128`. That proved the
F6 request, language selection, grouped header rejection, and unavailable
optional DTextBlock CDO font path. It did not by itself prove which header
operation failed. A separate source audit identified exact-zero `Font.Size`
handling as a remaining possible construction reject. The candidate is
superseded and is not F6 acceptance evidence. The corrected source seeds a
bounded reference size only for `Font.Size == 0` and retries a failed game
widget once as base TextBlock in the same open; exact-artifact F6 behavior
remains `NOT_VALIDATED` until the owner tests the replacement DLL.

The next diagnostics-enabled deployed candidate, DLL SHA-256 `D6CF...`, also
did not open F6. Every press reached `VISIBILITY_HUB_OPEN_PENDING`, resolved
the preference, detected language, and active language to `zh-hans`, then
rejected with `failure=8`, `font_abi_details=128`, `font_source=2`,
`font_fallback_reason=2`, and `text_runtime_failure=6`. This shows that both
the game `DTextBlock` path and the base UMG `TextBlock` retry failed during
target-size preparation; the log cannot by itself prove one unique internal
sub-cause. Source review found that the shared instance-level `Font` lookup did
not match the class-chain lookup that had already succeeded during
initialization. The replacement reuses that initialization-validated
`TextBlock.Font` class property and retains owner, inheritance, offset, and
container checks. The D6CF packages and deployment are superseded. Live F6
opening remains `NOT_VALIDATED` until owner testing of the replacement DLL.

The owner then retested exact deployed DLL
`5210E27DB48952737BE49AD05E7D70C86DAB671CE0F3DD2D19AF7B30D7A1BF4B`.
It again logged `VISIBILITY_HUB_OPEN_PENDING` and selected `zh-hans`, then
rejected with `failure=8`, `abi_failures=0`, `font_abi_details=128`,
`font_source=2`, `font_fallback_reason=2`, and `text_runtime_failure=6`. Cached
class-property reuse therefore did not solve the runtime: both preferred
Font.Size preparations could still fail before required page labels existed.
The next replacement made reflected Font/SetFont layout optional and provided
the bounded no-Font-mutation render-scale fallback described above. Its exact
DLL was `64BFEB26237B794D8CD1D81BE7E64788BEAF516B71C00049AD00B49EB64A4436`.
The owner then rejected that exact DLL visually: F6 opened in Simplified
Chinese, but the log recorded `font_source=3` and `font_fallback_reason=6`, and
the render-only scaling clipped text inside unchanged Slate line boxes. Source
review found the integer-only compatibility test had rejected reflected
floating-point `FSlateFontInfo.Size`. The prior 84A360B0 replacement uses
matching floating-point/integer access, writes the target only into a copied
`SetFont` parameter, and verifies the committed widget size. Exact-DLL live F6
visual acceptance remains `NOT_VALIDATED`.

## Mini-game height provenance

The generated 83-row Fly/Mole/Wave height table is limited to map 100 and exact
trusted `NPC_Start` actors (33/40/10). It is derived from the pinned
mini-game and actor-position inputs recorded in `DEPENDENCY_SOURCES.md`. The
runtime consumes immutable numeric rows only; it does not parse game XML,
extract PAKs, enumerate actors globally, or perform filesystem work.

The source-input hashes recorded for the generator review are:

- `017_ActorPositionData.xml`:
  `11CA916050AFA25F856DF0AFF46CAE0D23F928E8490617FBD3B524DC183E3ACF`
- `001_MiniGameData.xml`:
  `EDA335750F30C31D1A3E003C96B54024F671F600E18FCC02759970F7236CE9E3`

These source-input hashes are not DLL or public-package hashes.

## Evidence state

| State | 2.2.0 status | Required closeout |
| --- | --- | --- |
| Source review | `PASSED` | Core `2/2`, compact, world-map, PostRender, and release-hygiene gates passed. |
| Build | `PASSED` | Clean native `/W4 /WX` replacement DLL SHA-256: `6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`; compiled-source SHA-256: `A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`. |
| Installer tests | `PASSED` | Setup `20/20`, Manual `2/2`, payload equivalence, manual layout, and clean-target policy validation passed for exact DLL `6AEFDACC...`. |
| Package | `PASSED` | `Build-Release.ps1` package validation passed for exact DLL `6AEFDACC...`; all three public ZIPs re-extracted byte-identically. |
| Deploy | `PASSED_LOCAL_DEBUG_6AEFDACC_NOT_SETUP_OWNED` | Source/build/installed hashes match and Radar/Pickup diagnostics remain enabled. Rollback backup: `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`. The recorded `20260902-234105-640-native-only-deploy` backup belongs to superseded intermediate DLL `59529B2A...`, not this deployment. This is not Setup ownership. |
| Gameplay | `NOT_VALIDATED` | F6/F7/F8, F6 measured desired-size visual alignment after all rebuild/display paths, all categories, all visible Area Quest selected-band height states, unchanged Treasure arrow, shared `playerZ - 150`, nearest-mini-game below-icon triangles and +/-500 dead zone, languages, travel, save transitions, expanded-map alignment, revision 50 glyph styling, exit behavior, and long-session behavior. Static verification cannot replace live visual acceptance. |
| Controller | `NOT_VALIDATED` | Physical-controller map/pause suppression and restoration. |
| Responsive UI | `NOT_VALIDATED` | Windowed/fullscreen/DPI/16:9/16:10/21:9 plus long localized strings. |
| Localization glyphs | `NOT_VALIDATED` | Visual inspection of all 11 language tables with the shipped game font path. |
| Performance | `NOT_VALIDATED` | Diagnostics-disabled comparison plus bounded diagnostic evidence. |

The available healthy runtime log is bound only to the exact prior 84A360B0
DLL. It records no renderer, ABI, F6, or UE4SS fatal error and reaches normal
shutdown. It does not validate the current clean-build candidate's F6 presentation,
localized-overlay, or compact-outline bytes. Those exact bytes still require a
fresh live test; the `BUILT` and `DEPLOYED` rows establish artifact and installed-
file identity only, not runtime acceptance.

## Replacement artifact identity

The current development replacement DLL SHA-256 is
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`.
The current `dist/final-2.2.0/release-manifest.json` seals that exact DLL.
`Build-Release.ps1` package validation passed, Setup reports `20/20`, Manual
reports `2/2`, payload equivalence, manual layout, and clean-target policy
validation pass, and all three public ZIPs re-extract byte-identically. The
earlier package set bound to superseded DLL `5210E27D...` is historical evidence
only. The exact DLL's local diagnostics-enabled deployment passed with matching
source, build, and installed hashes. Its rollback backup is
`dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
Backup
`dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`
belongs to the superseded intermediate 59529B2A deployment and is not current
candidate evidence. This local deployment provides installed-file evidence but
not Setup ownership. The earlier
634D283A deployment is historical only. Do not reuse the historical
failed-candidate, D6CF, 5210, 64BF, or 2.1.1 hashes as current replacement
identity.

The packaged game-1.0.11 owner RVA and key-member offset `0x128` are fast paths
only. One FullActivation shares a total budget of at most 24 active-`.db` key
validations across packaged and structural owner routes. Packaged-owner
authentication failure permits one FullActivation-only scan of the current
executable for exactly one retained structural signature. That scan counts only
targets inside the mapped image and scans each executable section through
`min(SizeOfRawData, VirtualSize)`. Structurally incompatible
updates fail closed; this does not promise compatibility with every future
version.

## Remaining runtime evidence

- Final `START version=2.2.0` and runtime-label evidence from a game launch.
- Owner gameplay/controller/responsive-layout/localization/performance result.
- Repeated-exit runtime matrix. The 09:33 crash came from the old E64 artifact
  and does not prove shutdown causality. Current shutdown closes ingress
  atomically; only a known live GameThread performs UE cleanup; true process
  teardown performs no I/O, logging, join, or close, and one finalizer owns the
  final flush.

Refreshed source/build evidence and local diagnostics-enabled developer
deployment passed for exact DLL
`6AEFDACC...`; its rollback backup is
`dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
The F6 presentation,
localized-overlay, and compact-outline bytes still require a fresh live test;
installer/package evidence must also be regenerated for that exact artifact,
and the remaining runtime evidence must still be captured; until then,
2.2.0 must not be described as gameplay accepted, controller accepted,
visually accepted, localization-glyph accepted, performance accepted, or as a
Setup-owned live installation.
