# 2.3.0 Development and Validation

For the latest complete local release, use [Release status](RELEASE_STATUS.md).
The dated receipts below describe development and installer checkpoints; they
are not the hash authority for later documentation-only repackaging.

## Scope and ownership

On 2026-09-07 the owner skipped the unpublished 2.2.2 version and assigned
configurable Radar keys plus its existing flight and follow-language work to
2.3.0. See `RELEASE_PLAN_2_2_2.md` for historical exact-byte receipts. Never
relabel those receipts as 2.3.0 evidence. Published 2.2.1 ZIPs remain unchanged.
No AutoPickup source, game save, loader, or other Mod is part of this change.

## Hotkey contract

- File: `config/hotkeys.ini`, UTF-8 (optional BOM), at most 4096 bytes.
- One `[hotkeys]` section; exactly `settings_hotkey`, `enable_hotkey`, and
  `disable_hotkey`, with three different resolved key codes.
- Same families as Pickup: F1-F24, A-Z, 0-9, NUM0-NUM9, HOME, END, PAGEUP,
  PAGEDOWN, INSERT, DELETE, SPACE. Values are ASCII and case-insensitive.
  Modifier chords, mouse/controller buttons, and abbreviated aliases are unsupported.
- Defaults are F6/F7/F8. Missing, unreadable, oversized, or invalid files fall
  back transactionally to all three defaults; no partial rebinding.
- Load once during construction, register the existing generation-guarded
  UE4SS keydown callbacks, and retain the original atomic control requests.
  No extra polling, hooks, input consumption, menu state, or map layout changes.
- Keep this file separate from F6-written visibility/language preferences.
  A restart is required after editing. Diagnostics record the selected codes
  and parser status only when existing opt-in debug logging is enabled.
- Setup payload has `hotkeys.example.ini`; clean installation creates the live
  file with default or explicitly selected keys. The GUI reads existing bindings
  and lets Install / Update / Repair confirm a new set. Unchanged selections
  preserve bytes; changed values preserve comments, BOM, whitespace, and line
  endings. Invalid config is rejected before mutation. Original and selected
  bytes are bound to the confirmation token; transaction failure restores the
  old file. Other user settings are preserved. Manual packages carry defaults
  as `hotkeys.ini`; manual copying
  can overwrite user preferences, so document backup/Setup as the update path.
- Developer deployment preserves existing bytes or adds defaults. The runtime
  validation/fallback remains authoritative for a developer-preserved file.

## Installer hotkey follow-up - 2026-09-07

Owner requested the missing Setup key controls after the initial three-package
delivery. This is installer-only: retain the CM-04 native DLL and native receipt.

- Add editable Settings / Enable / Disable selectors. Read the owned existing
  installation or show defaults when absent. Same-path refresh and cancelled
  or rejected plans retain the user's uncommitted selection.
- Bind original and selected hotkey bytes to plan confirmation. Write selected
  values through the existing rollback transaction; do not silently preserve
  old keys when the owner explicitly changes them in Update / Repair.
- Invalid or duplicate selection rejects before writes. Invalid installed keys
  disable Update / Repair without disabling strictly owned Uninstall.
- Parser/editor checks: 177 assertions passed, including BOM/comments/spacing,
  line endings, single-key edits, no final newline, and output size overflow.
- Initial integration run: 19/20; failure was the success-backup assertion seeing
  a deliberately retained failed-transaction journal in the same fixture. Key
  rollback itself restored all checked bytes. The failure fixture is now isolated;
  the success no-backup check remains unchanged. Corrected isolated rerun:
  20/20 passed, zero failed/skipped, source inputs unchanged and fixtures cleaned.
  Includes clean custom installation, same-version Repair changes, older-version
  Update changes, unchanged-byte preservation, stale original/selected rejection,
  invalid-config uninstall availability, and injected-failure restoration.
- Setup visually opened at 150% scaling, displaying three unclipped key fields,
  installed F6/F7/F8 and Repair. Automated input did not change the elevated
  window, so this is visual/readback evidence only, not a completed GUI install.
- No game deployment, real user-key change, or new gameplay acceptance claimed.
  Previous package receipts below remain historical and are superseded by
  the final installer-hotkey package receipt immediately below.

### Final installer-hotkey package receipt

Full `Build-Release.ps1 -SkipNativeBuild` succeeded under Windows PowerShell 5.1.
Manifest generated UTC `2026-09-07T11:00:51.0231580Z` in `dist/final-2.3.0`.
Core and all required source/hygiene gates passed; final Setup parser/editor
177 assertions, Setup matrix 20/20 and manual matrix 2/2, zero failed/skipped.
Setup/manual payload equivalence, exact layouts, clean-target rules and fixture
cleanup passed. Independently reopened all three ZIPs and verified entry counts,
outer hashes, embedded Setup/native hashes and public defaults: AUTO, F6/F7/F8,
debug off. Installed native DLL still matches the unchanged CM-04 receipt.

| Artifact | Bytes | SHA-256 |
| --- | --- | --- |
| Installer ZIP | 10,150,945 | `52F877B81D7FD14A77FC4F0FB505AA30264CC9F7DEBFD8B753A9C99504B76B54` |
| Manual-No-UE4SS ZIP | 2,084,067 | `AD5852639A9C9B15CB52859F83AE4EC7B69CFEB149FF9ED19057687A1661F7DC` |
| Manual-With-UE4SS ZIP | 10,109,708 | `3C1EE6E12E95287DABBB681D85B00F6F6F8E5B5C36D27EA7C66065FE80815577` |
| Setup EXE | 21,339,136 | `6AB72C206E52AFACF8A430C34E20E19B30D35BC0B74CE6DE3428B488A3D319C4` |
| Embedded runtime ZIP (38 files) | See manifest | `07BC483954A6C2BA84B628D321D780842FCC29AF958D760CFA21A97416EB5CD4` |

The previous five-file package set was copied and hash-verified at
`dist/work/candidates/cm04-before-installer-hotkeys-20260907` before replacement.
Only generated build/staging output was cleaned; old delivered packages remain
recoverable there. One build attempt stopped on the preview EXE's file lock;
the owner closed that window, then the supported full release path passed.
No real-game configuration was changed, no new runtime acceptance asserted,
and no publication/Git action performed. Existing rights-review boundaries remain.

## Included previous work and exclusions

WM-07 motion-compensated initial flight attachment and persistent AUTO language
remain intact. Settings-open and enable action edges sample game language;
the effective language follows it only when AUTO is selected. Manual choices
and the last valid detection are preserved.
WM-06 immutable retained atlas placement, budgets, clipping, and scheduler are
unchanged by hotkey work. F6/F7/F8 remain logical action names in diagnostics.

Controller start-menu/submenu overlay leakage was reproduced with a virtual
Xbox controller on 2026-09-07 and confirmed by the owner. CM-04 now implements
a bounded native paint-ancestry guard. Two virtual-controller Start/Hero return
cycles, including Hero Skill, passed without settled-menu residue on
2026-09-07. Full runtime acceptance is still pending; CM-04 is installed for
testing, not accepted for public release. Follow
`CONTROLLER_MENU_ATTEMPT_LEDGER.md` for baseline, hypotheses, and exact receipts.

## Current CM-04 native-only candidate - 2026-09-07

- AUTO is now the first persistent language option; explicit choices stay
  fixed until AUTO is selected again. Updated popup raster cells agree with
  hit testing and selected highlights.
- CM-04 controller suppression uses native paint ancestry, not input-route
  detection. See `CONTROLLER_MENU_ATTEMPT_LEDGER.md` for scope and safety policy.
- Native DLL SHA-256:
  `D6C79578F3C19C033BB0A1F75EDB8382719CFDC87BB145D0D39ECB12FBDB7046`;
  size 1,129,984 bytes. Core 2/2, 1067 assertions and all five relevant static
  gates passed. Its initial native-only deployment preceded the three-package
  refresh recorded below.
- Installed with owner approval on 2026-09-07, exact DLL hash verified above;
  limited controller runtime routes passed as recorded above, with the full
  matrix still pending. Previous CM-03 DLL/logs are recoverable
  in `dist/work/deployment/deploy-backups/20260907-025731-623-native-only-deploy`.
  Language/visibility, diagnostics, hotkeys and overrides are byte-preserved;
  local language is AUTO, debug true, keys F6/F7/F8. Game was stopped before
  and after deployment. Standing authorization now covers subsequent checked
  Radar development deployments; see `PROJECT_CONTEXT.md` for boundaries.
- City/world-map activation issue: read-only diagnosis recorded under WM-08
  in `WORLD_MAP_ATTEMPT_LEDGER.md`; no world-map code change in this candidate.
- Map-close delay: WM-09 records two Radar-on and two F8-disabled closes;
  comparable 0.7-0.8 s transitions, no added close stall reproduced at the
  tested stationary location. This is not a clean no-mod or frame-exact test.

## Historical CM-04 three-package delivery - superseded by installer-hotkey follow-up

- **Request/status:** Owner requested the three 2.3.0 delivery packages after
  the controller test. `LOCAL_PACKAGED_INSTALLER_TESTED_PARTIAL_RUNTIME_ONLY`.
  This does not close WM-08 city-first-activation diagnosis, the remaining
  runtime matrix, or third-party distribution/source-provenance review.
  Nothing was uploaded, committed, pushed, launched or installed this turn.
- **Native identity:** Reused the exact CM-04 DLL and native build receipt
  above through `Build-Release.ps1 -SkipNativeBuild`. Independently verified
  that build and installed DLL hashes both equal `D6C79578...FBDB7046`.
  No runtime source, native DLL, loader, real game state or configuration was
  changed. The new package metadata describes CM-04; installed metadata was
  not rewritten or claimed to match the refreshed package byte-for-byte.
- **Packaging corrections:** The first invocation ran under PowerShell 7
  despite the requested shell setting and correctly failed the release-tools
  receipt gate before building. Explicit Windows PowerShell 5.1 invocation
  preserves the original receipt; no receipt relaxation or regeneration.
  The installer test working directory had to be created before its runner.
  A subsequent 20/20 run exposed four null success-stream entries emitted by
  reflection in `HotkeyConfiguration.Tests.ps1`, breaking the aggregate result
  contract. Discarded only that void Invoke return; verified zero parser
  pipeline objects with all 164 assertions passing. No production code change.
- **Final checks:** Full supported release entry point rerun succeeded.
  Core 2/2 plus F6 overlays, compact, world-map, PostRender and release-hygiene
  gates passed. Setup matrix 20/20 and manual matrix 2/2; zero failed/skipped,
  payload equivalence, manual layouts, clean-target rules, unchanged source
  inputs and fixture cleanup all passed. Test root:
  `dist/work/tests/cm04-release-20260907` (empty after completion).
- **Final directory:** `dist/final-2.3.0`, exactly three ZIPs plus
  `release-manifest.json` and `SHA256SUMS.txt`. All three ZIPs were re-extracted
  byte-identically by the release builder. Independent ZIP inspection also
  verified outer hashes/counts, Setup hash, exact CM-04 DLL in both manual
  channels, source-matching popup/metadata/configuration, and absence of logs
  or executable/script installers in manual packages. Inspection normalizes
  Windows ZIP entry separators; no archive-content change was needed.
- **Defaults:** `debug_logging=false`, `language=auto`, keys F6/F7/F8 in all
  channels. The live `debug_logging=true` override remains local. Installer
  is unsigned. Existing 2.2.1 and older hotkey-only candidate ZIPs are retained;
  use this new final directory for the current CM-04 payload.

| Artifact | Bytes | SHA-256 |
| --- | --- | --- |
| Installer ZIP | 10,150,452 | `7A1DE30398352BF007AEE9AFC56B59DFC875F9330992F27514BAB6DBAAF0FE69` |
| Manual-No-UE4SS ZIP | 2,083,686 | `2A5599436BF344F247952CEB1F2D35CAD91AA879C92EE71813DD49FB1E85C3DD` |
| Manual-With-UE4SS ZIP | 10,109,327 | `A353C23DE41167FF30ACACBFEB7C266679BEE46067D4654EF74B9163F184AA4A` |
| Embedded Setup EXE | 21,332,480 | `D880CF1A37BE9834EB66C71B528922BAEB5A03F3CFCF44E20D7A17511B9135C4` |
| Embedded runtime ZIP (38 files) | See manifest | `E8893802990C21315A033315A6AF455F0C54F425687B5E207F1CA8323395C2C0` |

The final manifest generated at `2026-09-07T10:29:56.9891075Z` is authoritative
for exact package and matrix results. Its conservative exact-artifact gameplay
status does not erase the narrower CM-04 live controller evidence, nor does
that live evidence accept the entire freshly packaged/default-debug-off matrix.
Local artifact delivery does not clear the recorded publication-rights blockers.

## Historical hotkey verification ledger (not CM-04 package acceptance)

- Core parser and existing state/event-log tests: 2/2 passed, 1052 state
  assertions, including distinct keys, all supported families, malformed input,
  BOM/CRLF, exact size boundary, transactional fallback, and allocation checks.
- F6 localized overlays, compact renderer, world map, PostRender/control safety,
  release hygiene, and source-bound native receipt: passed.
- Pinned local native build passed: DLL 1116160 bytes, created UTC
  `2026-09-07T07:18:47.3947940Z`.
  DLL SHA-256 `503957BC3D92F8E8CDBC010836ACDC6E6104F8174D88F9FAA296ED5EDCDA11EA`.
  Compiled-source SHA-256 `62D1AB33A3D29FDA217318264781A8D39B998A109F28B920E992417D488C84A8`.
  Release-tools SHA-256 `41D2556A79997132D5709A2460FEF8986359ECB156A1827A51F8A942A46BE116`.
- Final Setup parser: 164 assertions. Final Setup matrix: 20/20, no failures or
  skips, unchanged source files and cleaned fixtures. Includes byte-preserved
  custom Insert/Home/PageUp, no-write rejection of duplicate/invalid/missing
  fields/oversized config, older-version missing-file migration, and rollback.
- Final Manual matrix: 2/2; payload equivalence, layout, clean-target policy,
  and archive re-extraction passed. Embedded runtime is 38 files; manual archives
  have 42 and 46 files. Test locations are under `dist/work/tests/hotkeys-2.3.0-*`.
- Runtime/custom-key, follow-language, controller, resolution, and performance
  acceptance: pending. Prior flight testing is preliminary evidence only.
- No publication or GitHub push is authorized by this request.

### Rejected packaging attempt and correction

The first full Setup matrix rejected the previous AUTO-edited public
`visibility.ini`: 33 CRLF lines plus one bare LF. This was a packaged-default
format error, not a hotkey or map failure. Normalized only the source default
file's line endings to LF, without changing setting values. Rebuilt Setup and
all three local candidates; reran both matrices on the final bytes below.
Do not reuse intermediate Setup C8ADF71A or its ZIPs. Corrected the installer
profile's descriptive height defaults to match the unchanged all-on source INI.

### Final local candidate identities (not published)

Directory: `dist/work/candidates/hotkeys-2.3.0`.

| Artifact | SHA-256 |
| --- | --- |
| Setup EXE | `742DA76C6C4D3D34E395D8AA89845838FB2268F85936C3B82356E23B1598E8EB` |
| Installer ZIP | `4DD3CCB9C46457ADDC116933F8949003BCB22BDDD8F8CCBAEB2FD24E4ED4FE53` |
| Manual-No-UE4SS ZIP | `388D5D449C9F6D37539B75780E6E79C387C10E97EFBC247BDFEEB00AF1A4A53F` |
| Manual-With-UE4SS ZIP | `08A93F438D86FC62D82E78B57D9553DC736DF3C61EE68D4DC811730C5A7EFF71` |
| Embedded runtime ZIP | `792A2CC9872E36E9652864B8EA48FBE5C2493351C162961637138F6B933A77F6` |

### Owner-authorized local deployment

After tests, the owner explicitly requested installation for local testing.
`Deploy-NativePrototype.ps1 -Diagnostics Preserve` passed its predeployment
Core/static/receipt gates and deployed the exact DLL listed above. Game was
closed; no game launch or termination occurred. Exactly one enabled Radar entry
remains in mods.txt, with no predecessor entry.

Rollback backup:
`dist/work/deployment/deploy-backups/20260907-002756-628-native-only-deploy`.
Its previous DLL is the unpublished 2.2.2
`DF65072DB2E0D05179CC140D3FBE5BAEEDCA4CF5AD7E60F1C1CA4CE20B9E8688`.

Installed configuration:

- Manual Simplified Chinese (`language=zh-hans`) preserved byte-for-byte:
  `89B856ED295F72E1EA104141112A905FEF2BDCFA12FBC9B41CDDD4CBD4B17A40`.
- Local debug=true preserved byte-for-byte:
  `E888884C590B6F97EE061DF9EDB85CD83738BD1BED171D8713DCEF1A55DAF10D`.
- New hotkeys file uses F6/F7/F8 defaults, source/installed hash:
  `B9672AC71C65567A3F27B7E9D059A861D3D63B7FB49A4ABA1E915FE36C01AEC0`.
- Public diagnostics remain false and public language remains AUTO; local
  overrides did not enter candidate packages. Published 2.2.1 ZIP hashes still
  match the historical A7F4065F / 5ED7B073 / D54AF550 receipts.

The embedded release metadata records the build-time, pre-authorization
deployment status. This later exact-byte deployment receipt supersedes that
snapshot; neither establishes gameplay acceptance. The separate AutoPickup
worktree changes were preserved and not edited by this task.

## Owner regression checklist

1. With defaults, verify settings / enable / disable on F6/F7/F8 as before.
2. Close the game, configure INSERT/HOME/PAGEUP, restart. Verify each action,
   including disable after enable, and that the old default keys no longer
   trigger those actions. Avoid bindings already used by the game or Pickup.
3. Change a category and language in Settings, restart, verify keys survive.
   AUTO should refresh at an actual Settings open; explicit language stays fixed.
4. Invalid/duplicate/missing config must leave F6/F7/F8 usable after restart.
5. Open the world map while mounted and flying; test zoom/pan/reopen and confirm
   no reappearance of drift, flashing, or missing icons.
6. CM-04 passed two virtual-controller Start/Hero return cycles including
   Hero Skill with debug on. Repeat debug-off, input crossover and remaining
   submenu/lifecycle routes before claiming full controller acceptance.
