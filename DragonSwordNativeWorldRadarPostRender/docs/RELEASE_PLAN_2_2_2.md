# 2.2.2 Development Plan

Historical unpublished candidate: on 2026-09-07 the owner skipped 2.2.2 and
assigned this work plus configurable hotkeys to 2.3.0. Continue with
`RELEASE_PLAN_2_3_0.md`. The receipts below remain bound to the original bytes.

## Version and artifact boundary

On 2026-09-06 the owner assigned all unpublished follow-up changes to 2.2.2.
This is an unpublished development target. The previous WM-07 test DLL identifies
itself as 2.2.1, with SHA-256
`B1952BA6A5C80A3127498346452E42B0F74EB273FE3B4732A204650AA2D3037C`.
Published `dist/final-2.2.1` packages are immutable historical artifacts.

Runtime, metadata, installer, package-tool, and verifier version owners now target
2.2.2 together. New source/build/package receipts belong to the new identities.
Do not silently promote old installer or gameplay results to 2.2.2.

## 1. Flying initial map attachment - preliminary owner pass

WM-07 is implemented, built, and installed. The owner reported that it seems
to work correctly and requested continuation. The local log independently
records successful attachment on attempt 2/3: player motion 329.912770 world
units, anchor delta 1.683838, unchanged parent extent, and compensated-origin
delta 0.052545. See `WORLD_MAP_ATTEMPT_LEDGER.md` for exact evidence.

Keep WM-06 immutable retained placement, two-sample validation, and all finite
budgets. Controller, complete resolution, travel, and broader performance
acceptance remain separate from this preliminary result.

## 2. Compact Radar over start menu/submenus - investigating, not fixed

The supplied support log proves cursor/pause/world-map observations, but does
not record a start-menu transition. Controller use remains a plausible cause,
not a proven reproduction. Existing compact suppression can miss a menu if
cursor visibility, game pause, and map visibility all remain false.

Read-only inspection of the current local USMAP confirms:

- `DsClientLocalPlayer.UIManager` is an object property.
- `DUIManager` has `CurrentRootPanelInstance`, `PanelUI`, `PanelStack`,
  `PanelUIDefault`, and `PopupUI` properties.
- `PanelUI` and `PanelStack` are arrays of `DENM_PanelType`.
- The current enum includes Main=3, Main_PC=4, Inventory=6,
  CharacterManager=11, Quest=15, WorldMap=27, AdventureBook=28, Option=31,
  and KeySettings=64. These values are evidence for this schema only, not
  permission to hardcode an unchecked future enum contract.

Schema source: local UE4SS `DS-5.3.2-0+UE5-1c1a1497.usmap`, SHA-256
`0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47`.
The uncompressed version-4 format was read using the locally pinned UE4SS
generator as the format authority. No game memory, settings, or files changed.

Next: obtain bounded live state samples for gameplay, mouse-opened main menu,
submenu, back navigation, and closed menu. Establish whether each array means
active, stacked, cached, or default panels before using it for suppression.
The owner can currently test only mouse/keyboard and will obtain a controller
later. Mouse results must not be labelled controller acceptance.

Required guardrails: use the current local player's manager and validated
reflected types; do not enumerate all widgets, latch on controller keys, or
treat every nonempty UI array as a blocking menu. Reuse the existing bounded
control schedule and preserve world-map session and layout logic. This turn
does not implement a new suppression provider or claim a menu fix.

## 3. Follow game language - implemented, runtime acceptance pending

Removed the one-time AUTO-to-explicit migration. The popup now adds `AUTO (Game
Language)` in its previously vacant twelfth cell. All eleven explicit-language
indices, Korean/Traditional Chinese overlay placement, and popup geometry remain
unchanged. The selector face shows the resolved language; the selected popup cell
indicates manual versus follow mode. No new texture or font family is introduced.

Implemented behavior:

- Provide a distinct persistent Follow Game Language mode plus manual choices.
- In follow mode, read the game text language once when F6 actually opens;
  resolve the language/font before constructing the panel, with no frame poll.
- Keep the follow preference as AUTO; never save the detected explicit language
  back over it. Preserve existing manual selections.
- On an unavailable/invalid read, retain the last valid detected language;
  use English only when no valid sample exists. Do not persist a failure fallback.
- A language change made while F6 is already open takes effect on its next
  opening. F7 may retain the existing once-per-activation refresh if needed.
- Verify popup layout, all 11 explicit choices, fallback overlays, and both
  explicit/follow persistence paths before delivery.

The public default remains `auto`; installed manual preferences must be preserved.
Tests cover all twelve choice/config roundtrips, follow/manual independence,
successive game-language changes, unsupported samples, and last-valid fallback.
Actual F6 rendering, in-game language changes, and persistence after restart still
require owner validation on the exact new DLL. Mouse tests are not controller tests.

## 4. 2026-09-06 candidate build and local deployment receipt

Status: implemented, built, and rollback-backed developer-local deployment passed.
This is not a public package, gameplay acceptance, or a controller-menu fix.

- Version/runtime: `2.2.2` / `DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_2_2`.
- Native build UTC: `2026-09-07T04:03:44.1494616Z`.
- DLL SHA-256: `DF65072DB2E0D05179CC140D3FBE5BAEEDCA4CF5AD7E60F1C1CA4CE20B9E8688`.
- DLL size: 1,112,576 bytes; installed DLL hash matches exactly.
- Compiled source SHA-256: `9044932DAD58A7B75F55D942CABA6D0685BA9F4E3E6EC2F655A63E37F12D3A89`.
- Release tools SHA-256: `0E35CE87F6471BF6F71862DD66D1871CC24E43E31456F23A5A111825338B69F7`.
- Build receipt: `dist/work/build/native/native-build-receipt.json`.
- Passed Core 2/2, 904 state assertions, localized-overlay, compact, world-map,
  PostRender safety, release-hygiene, and pinned-SDK native build checks.
  Deployment independently reran Core and source gates before changing game files.
- An intermediate 2.2.2 build `1F6165CC759E6F330910BF1C0DB9E7761EA4B404E27D31FD642DFEEFF8CD11C3`
  was not deployed; it was rebuilt after the PostRender verifier was updated for
  persistent AUTO, so the final receipt binds the completed verifier set.
- Rollback backup: `dist/work/deployment/deploy-backups/20260906-210610-149-native-only-deploy`.
  It retains the previous WM-07 DLL `B1952BA6A5C80A3127498346452E42B0F74EB273FE3B4732A204650AA2D3037C`
  and prior installed state. The old 2.2.1 identifier is not relabelled.
- Existing visibility config remained byte-identical, SHA-256
  `89B856ED295F72E1EA104141112A905FEF2BDCFA12FBC9B41CDDD4CBD4B17A40`:
  `language=zh-hans` remains the owner's manual selection.
- Existing diagnostics remained byte-identical, SHA-256
  `E888884C590B6F97EE061DF9EDB85CD83738BD1BED171D8713DCEF1A55DAF10D`:
  local `debug_logging=true`; public default remains false.
- `mods.txt` contains one enabled Native Radar entry and no predecessor entry.
- All three `dist/final-2.2.1` ZIP hashes still match the previous release receipts.
  No 2.2.2 final package, installer acceptance, Git commit/push, or publication
  was performed. Other Mods' working changes were not modified.

Owner validation next:

1. Open F6 and choose the last language cell, `AUTO (Game Language)`. The
   selector face shows the detected language rather than the word AUTO.
2. Close F6, change the game's text language, and open F6 again. Verify the
   panel follows it and the AUTO popup cell remains selected. Check the saved
   preference stays `auto` after restart.
3. Select an explicit language, change game language, and reopen F6: the manual
   selection must remain unchanged. Check Korean/Traditional Chinese readability.
4. Recheck flight attachment, ordinary ground attachment, pan/zoom, and reopening
   on this exact DLL. The preceding WM-07 preliminary pass is not acceptance for
   these new bytes.
5. Controller start-menu/submenu reproduction remains pending the owner's device;
   do not change menu suppression based only on cached UI arrays or mouse results.
