# Compact Radar controller-menu investigation (2.3.0)

## CM-01 - reproduced, missing native menu evidence (2026-09-07)

- Baseline installed DLL SHA-256:
  `503957BC3D92F8E8CDBC010836ACDC6E6104F8174D88F9FAA296ED5EDCDA11EA`.
- A virtual Xbox 360 controller delivered verified XInput button pulses with
  neutral release. Controller button hints appeared in the game.
- UTC 08:20:28 Start opened the in-game main menu: native minimap background
  disappeared but Radar icons and clock remained. UTC 08:20:54 A opened the
  Hero information page: Radar remained. B returned to the main menu with the
  same leakage. Owner independently confirmed both cases.
- Log session began UTC 08:14:50.423. The renderer resumed `Attached` (state 2)
  at UTC 08:20:08.966, with 29 markers. Profiles during the subsequent menu
  and Hero-page interval continued reporting compact refresh work. The drop
  count stayed at its earlier value 5. No renderer fault was recorded.
- `COMPACT_MENU_STATE` did not record native menu identities. Its sole early
  record is not an exact sample of the later visible menu. Do not present
  absent events as a complete trace of every cursor/pause flag.
- Existing suppression observes cursor, pause, map visibility, position and
  activity, not the native in-game menu. Do not change world-map placement,
  ownership, readiness, retained layout, WM-07 flight compensation, or data.

## CM-02 - bounded debug-only native UI probe (in progress)

- Inspect only the current local player's `UIManager`, its current root, and
  at most 64 enum entries in each of PanelUI, PanelStack, PanelUIDefault,
  PopupUI. Reuse the shared 250 ms control edge; no global widget enumeration,
  input hooks, writes to game UI state, or work on the 16 ms position path.
- This intermediate probe logs changes but does not alter suppression. It is
  not a fix or a release candidate. Public debug-off remains inert.
- Static USMAP inspection confirms `DPanelMain` contains gameplay HUD and
  minimap widgets. `Main`/`Main_PC` must not be blindly treated as blocking
  menus. Panel arrays may contain defaults/caches; establish live semantics.
- Compare gameplay, controller Start, Hero page, B to main menu, B to gameplay,
  and mouse-opened equivalents. Require reliable native open/close evidence
  before choosing the runtime classifier. Only the compact host visibility
  may change; retain that host through ordinary menus.
- Native build, probe deployment, runtime observations, final policy tests,
  and final exact-byte gameplay results must be recorded separately below.

### CM-02 build receipt - not deployed, repair not implemented

- Final Windows PowerShell build UTC: `2026-09-07T08:44:35Z`.
- DLL SHA-256:
  `228B89DBEBA70FD5D502760CB6530574A0FA248C6F126184B6B3FCDCEEE10A64`;
  size 1,120,768 bytes.
- Compiled-source SHA-256:
  `2C3E45A844E2B259A571D0C2FE6EDA105F7BED970E46C972B67A6C94ACC4B67C`.
- Release-tool SHA-256:
  `41D2556A79997132D5709A2460FEF8986359ECB156A1827A51F8A942A46BE116`.
- Core 2/2, F6 overlays, compact source gate, world-map canary, PostRender
  safety, release hygiene, and source-bound build receipt passed. These are
  diagnostic-build checks, not gameplay or installer acceptance.
- The intermediate PowerShell 7 build `31DF42EE...5DEB3D3` was not deployed;
  cross-shell release-tool ordering invalidated its receipt for the standard
  validation host. The final full rebuild above supersedes it.
- Installed DLL still matches the CM-01 baseline. Game is closed; no game
  files, user settings, controller state, or other Mods were changed. Awaiting
  owner approval to deploy the probe and launch for controlled sampling.
- Baseline log preserved locally under the ignored virtual-gamepad evidence
  directory, SHA-256
  `851096D5F1507BDD27A59076AF99AE7F88574DA1233FF6F2EC85D229100668CE`.
- Do not package the probe as a fix. Next: collect native UI changes during
  the exact controller sequence, implement only a proven current-menu signal,
  replace the temporary diagnostic, and rerun close/reopen/mixed-input tests.

### CM-02 deployment receipt - installed, runtime sampling pending

- Owner explicitly approved installation on 2026-09-07. Deployment used
  `Deploy-NativePrototype.ps1 -Diagnostics Preserve` in Windows PowerShell.
- Installed version is 2.3.0. Installed DLL independently matches the final
  CM-02 build: SHA-256
  `228B89DBEBA70FD5D502760CB6530574A0FA248C6F126184B6B3FCDCEEE10A64`.
- Deployment reran Core 2/2, F6 localization overlays, compact source gate,
  world-map canary, PostRender safety, release hygiene, and source-bound
  native receipt checks; all passed.
- Byte-for-byte checks confirmed hotkeys.ini, visibility.ini, diagnostics.ini,
  and treasure_overrides.txt unchanged. F6/F7/F8, manual Simplified Chinese,
  and debug_logging=true are preserved. mods.txt is unchanged, with exactly
  one enabled Radar entry. No other mod was deployed.
- Complete previous Radar tree and mods.txt were backed up under
  `dist/work/deployment/deploy-backups/20260907-014832-593-native-only-deploy`.
  The backup contains the CM-01 DLL and the previous 48,267-byte runtime log.
  Live runtime logs were cleared by the clean-stage replacement; the old log
  remains recoverable from the backup and separate baseline evidence copy.
- Game was closed before and after deployment. No game launch, controller
  input, live probe sampling, or visual acceptance was performed in this step.
  Suppression behavior is still unchanged: this is an installed diagnostic
  build, not a completed controller-menu fix.

### CM-02 runtime observation - array-only signal rejected

- Owner launched and enabled the exact CM-02 DLL. Process 3032 started at
  local 01:53:31 on 2026-09-07. Installed hash remained `228B89DB...EE10A64`.
- UTC 08:57:02.769 verified virtual Xbox Start opened the native main menu.
  Screenshot showed controller A/B hints, missing native minimap background,
  and leaked Radar icons/clock. PanelUI stayed [3], PanelStack stayed empty,
  and the same default/root values were retained. This rejects those arrays
  alone as the Start-menu discriminator; the diagnostic never changed policy.
- UTC 08:57:20.449 A was delivered, but physical Escape interrupted Computer
  Use before the resulting page could be observed. Later cursor changes and
  PopupUI=[48] are not accepted as a controlled Hero-page trace. Virtual
  controller was neutralized and detached at UTC 08:57:31.105.
- Log preserved under ignored local evidence directory
  `.tmp/virtual-gamepad-0.1.2/controller-menu-cm02-20260907/interrupted.Native.log`
  relative to the repository root, 38,507 bytes, SHA-256
  `1D1A4A0917301DBC0ADCE9E1C57CA7ABFE9D66C965D70E94AF6C44156FCFB8A8`.
  Owner reports that leakage remains.

## CM-03 - exact native minimap/menu widget probe (not a repair)

- Static schema shows DPanelMain contains both DLayerMainMenu and DLayerMiniMap.
  Main is also the ordinary gameplay HUD; a nonempty PanelUI remains insufficient.
- Extend debug-only sampling on the existing 250 ms edge: start at the exact
  current-world DLayerMiniMap weak candidate, follow at most 8 ownership links
  to a main panel that points back to that same minimap, and record the menu,
  minimap overlays, and at most 24 paint-parent nodes with cycle/budget checks.
- Slot parents require Slot.Content identity. Nested WidgetTree bridges require
  both RootWidget identity and owner.WidgetTree identity. Record local
  Visibility/RenderOpacity rather than claiming that a local flag alone proves
  effective visibility. Unknown ancestry is explicit, never guessed visible.
- Class labels are capped at 64 characters. Only copied diagnostic strings
  survive a call; no new runtime UObject retention, global scan, input hook,
  native UI write, suppression change, or world-map renderer change is added.
- Next acceptance must compare gameplay, controller Start, Hero, returns,
  mouse equivalents, and mixed input. A final policy must use proven native
  state and preserve the retained compact host; this probe cannot ship as a fix.

### CM-03 build receipt - passed, not installed

- Final build UTC `2026-09-07T09:08:54.8520393Z`; DLL 1,127,424 bytes,
  SHA-256 `6FF206504516A663B96410C7D1F8BB0A55B0271CA81848A7D5BF86D6D55DFB71`.
- Compiled-source SHA-256
  `9FF5FC612CADAF0AAB7A4F9889B0763C436B495EEF167D2123D78CFA8AD8B3BA`.
  Release-tools SHA-256 remains
  `41D2556A79997132D5709A2460FEF8986359ECB156A1827A51F8A942A46BE116`.
- Native /W4 /WX build, source-bound receipt, F6 localization overlays,
  Core 2/2, compact/world-map/PostRender source gates, release hygiene, and
  additional debug-only/read-only/bounded probe checks passed.
- Initial compile rejected a UObject/UWorld pointer comparison. Corrected
  the probe parameter to UWorld using the existing caller cast convention;
  the full rebuild above passed. Initial sandbox Core invocation could not
  write its existing CMake cache; approved normal-context rerun passed 2/2.
- Installed DLL was independently rechecked and remains CM-02
  `228B89DBEBA70FD5D502760CB6530574A0FA248C6F126184B6B3FCDCEEE10A64`.
  No deployment, game launch, input automation, package replacement, or GitHub
  mutation occurred in this step. CM-03 runtime evidence is still missing.

### CM-03 deployment receipt - installed, runtime sampling pending

- Owner explicitly approved installation. On 2026-09-07 Windows PowerShell
  `Deploy-NativePrototype.ps1 -Diagnostics Preserve` exited successfully after
  rerunning F6 overlays, Core 2/2, compact/world-map/PostRender source gates,
  release hygiene, and the source-bound native receipt.
- Independently verified installed DLL SHA-256:
  `6FF206504516A663B96410C7D1F8BB0A55B0271CA81848A7D5BF86D6D55DFB71`.
- Hotkeys, visibility/language, diagnostics, and treasure overrides match their
  pre-install bytes. F6/F7/F8, manual zh-hans, and debug_logging=true remain.
  mods.txt is byte-for-byte unchanged with exactly one enabled Radar entry.
- Rollback backup:
  `dist/work/deployment/deploy-backups/20260907-021154-612-native-only-deploy`.
  Its DLL matches CM-02 `228B89DB...EE10A64`; its 38,507-byte log matches
  `1D1A4A0917301DBC0ADCE9E1C57CA7ABFE9D66C965D70E94AF6C44156FCFB8A8`.
  Clean-stage installation replaced the live Radar tree; the previous version
  and logs remain recoverable in this full backup.
- Game was closed before and after installation. No game launch, controller
  input, or new runtime observation occurred. This remains a diagnostic build
  with unchanged suppression, not an accepted controller-menu repair.

### CM-03 runtime evidence - hidden paint ancestor confirmed

- Owner requested continued testing on 2026-09-07. Game PID 6592 started at
  local 02:14:12. Fresh probe events confirmed CM-03 was loaded; the installed
  DLL was independently rechecked as
  `6FF206504516A663B96410C7D1F8BB0A55B0271CA81848A7D5BF86D6D55DFB71`.
- Used the existing foreground-PID-guarded virtual Xbox helper. Every button
  press and neutral release was verified through XInput. Computer Use inspected
  each resulting page before another input; no progression buttons were used.

| UTC action time | Action and observed page | Native evidence / Radar result |
| --- | --- | --- |
| Before 09:20:13 | Gameplay, mounted and stationary | Native minimap and Radar visible; Right and paint ancestor p2 visibility 4, opacity 1; PanelUI=[3], PanelStack=[] |
| 09:20:13.529 | Start opens main menu | Radar icons/clock leak; native minimap disappears; Right/p2 fades to opacity 0 then visibility 1 (Collapsed), seq 239-242; local minimap stays visibility 0, opacity 1 |
| 09:20:38.707 | A opens Hero | PanelUI=[33,11], PanelStack=[11]; Right/p2 remains Collapsed and main panel fades to opacity 0; a later cursor=true edge (seq 253) hides Radar, so the delayed first screenshot is not pure-controller leakage evidence |
| 09:21:13.393 | B returns to main menu | Radar leaks again; PanelUI=[3], PanelStack=[]; cursor=false and compact_suppressed=false; Right/p2 remains Collapsed, opacity 0 |
| 09:21:38.611 | B returns to gameplay | Native minimap returns; Right/p2 becomes visibility 4 and fades back to opacity 1, seq 267-271 |
| 09:22:02.135 | Start repeats main-menu opening | Same visible Radar leakage and hidden native minimap |
| 09:22:21.920 | A repeats Hero entry | Immediate and later screenshots show controller hints and persistent Radar icons/clock over Hero; seq 281 has cursor=false, paused=false, compact_suppressed=false; seq 285 has Right/p2 Collapsed and main opacity 0 |
| 09:23:32.678 | Mouse click on the already selected Information page | Same Hero page, Radar disappears; seq 296-297 changes to state 3 / cursor=true / compact_suppressed=true; native hidden ancestry is unchanged |
| 09:23:52.078 | Mouse clicks Hero close | Returns to main menu with no leaked Radar; seq 304 has PanelUI=[3], cursor=true; Right/p2 still Collapsed |
| 09:24:20.000 | D-pad Right switches the same menu back to controller input | Team selection only, no page entered; Radar icons/clock reappear; seq 310-311 switches state 2 / cursor=false / compact_suppressed=false while native hidden ancestry stays unchanged |
| 09:24:40.308 | B returns to gameplay | Native minimap and Radar visible again; seq 320 restores Right/p2 visibility 4 and opacity 1 |
| 09:24:59.933 | Quit helper | Virtual controller neutralized and detached; game left running in gameplay |

- Root-cause evidence: reading only DLayerMiniMap's local Visibility is
  insufficient. Its verified Slot.Parent paint ancestor (p2, corresponding to
  the main panel's Right container) is hidden independently. Main-menu opening
  does not require PanelUI/PanelStack, cursor, or pause changes. The independent
  Radar viewport host does not inherit this native paint-ancestor hiding.
- Candidate repair direction, NOT implemented here: use bounded, identity-
  checked native paint ancestry as an additional compact suppression input,
  with explicit unknown/stale-state handling. Preserve existing cursor/pause/
  world-map guards and retained host/pool; do not latch controller buttons,
  modify native widgets, or change world-map transforms. Opacity animation
  timing needs a deliberate policy and regression tests, not an arbitrary
  threshold justified solely by these screenshots.
- Original log copied without clearing or changing the live log:
  `.tmp/virtual-gamepad-0.1.2/controller-menu-cm03-20260907-0925/sampled.Native.log`
  relative to repository root; 175,587 bytes; SHA-256
  `B52DE8D606836896186F84557ECB1FAE6B89958C28EAF3F8CE85114DDFEC54D4`.
  Screenshots and verified input timestamps are also in this task's tool history.
- This turn changed only the test record and created the local evidence copy.
  No source, settings, installed DLL, package, other mod, or GitHub changes.
  Controller main menu, Hero, returns, and mouse/controller crossover were
  observed. Keyboard-only opening, other submenus, travel, resolution changes,
  and a future suppression repair are NOT accepted by this diagnostic test.

### CM-04 - native paint-ancestry suppression candidate (2026-09-07)

- **Request:** Continue the controller fix and put persistent AUTO first in
  the 2.3.0 language selector. Investigate the city/world-map symptom separately.
- **Status:** `SOURCE_AND_BUILD_PASS_RUNTIME_PENDING`. CM-03 remains installed;
  no game deployment, launch, input injection, public packaging or Git push in
  this turn. Do not treat the previous CM-03 replay as CM-04 acceptance.
- **Change:** Use the current weak `DLayerMiniMap_C` candidate, expected World,
  and a main-panel backlink to that exact layer. Owner search is bounded to
  eight nodes. Follow at most 24 cycle-checked, same-world paint ancestors;
  Slot.Content/Parent and WidgetTree.RootWidget/owner.WidgetTree prove edges.
  Hidden, Collapsed or finite zero opacity suppresses only Radar's viewport
  host. Positive fade values are not thresholded. Retain the host/marker pool
  and every existing cursor/world-map/pause/activity guard.
- **Failure policy:** Missing/stale identities, malformed fields, unproven
  links, cycles, depth exhaustion and guarded read exceptions yield Unknown.
  Unknown releases only the new guard; it never bypasses existing guards or
  latches an old hidden state. Candidate/pool reset clears the sample. This
  deliberately favors retaining baseline gameplay behavior over permanently
  blanking Radar on an unsupported UI schema; unknown-state menu leakage is
  still a failure requiring a new bounded evidence check, not an accepted fix.
- **Scheduling:** Existing shared 250 ms activity edge, including debug-off.
  No new input hook, controller poll, widget scan, timer or reflected 16 ms
  position-path query. `NATIVE_MINIMAP_PAINT_STATE` is change-only opt-in debug
  evidence: state 0 Unknown, 1 Visible, 2 Hidden. Native UI and world-map
  transforms are read-only/unchanged by CM-04.
- **Language:** AUTO first; save `language=auto`, never detected language.
  Manual preference persists until explicitly selecting AUTO. Actual F6 open
  (and existing F7 activation) samples game language; only AUTO follows it.
  Korean/Traditional Chinese popup rasters move to zero-based cells 3 and 5;
  all 12 click/highlight/config round trips are tested. Main text TGAs unchanged.
- **Verification:** Core 2/2 tests, 1067 state assertions; full native /W4 /WX
  build passed. F6 localized overlays, native compact (including new ancestry
  safety gate), world-map, PostRender and release-hygiene gates passed. The
  first hygiene run overlapped the active build and correctly rejected its
  partial stage; the completed-stage rerun passed. Scoped diff check passed.
- **Exact candidate:** `dist/work/build/native/main.dll`, 1,129,984 bytes,
  SHA-256 `D6C79578F3C19C033BB0A1F75EDB8382719CFDC87BB145D0D39ECB12FBDB7046`.
  Receipt `dist/work/build/native/native-build-receipt.json`:
  compiled source `64B19F310CB47FA3D0D53C886F4B8DB0E89D5EF6AB7D8CC9E2BF3376F9707E1A`;
  release tools `98B3F70A8A50B919F8F3B7BC5153A5125271B4CBB94C57EB9D44DA0A43CE4EF5`.
- **Next acceptance:** Install only after owner approval. Replay CM-03's
  gameplay -> Start -> Hero -> Start -> gameplay, twice without moving the
  mouse; then mouse/controller crossover on the same page. Require native
  hidden ancestry -> state 2 / owned host hidden and gameplay -> state 1 /
  restored. Repeat with debug off. Check keyboard menus, other submenus,
  F6/F7/F8, travel and first activation while native HUD is hidden. A 250 ms
  sampling delay plus game fade time is possible; no frame-exact claim.
  For language, test AUTO -> game-language change -> reopen F6 -> restart,
  manual -> game-language change -> reopen -> restart, then manual -> AUTO.
  Visually check all 12 cells, particularly Korean and Traditional Chinese.

### CM-04 deployment and standing local authorization - 2026-09-07

- Owner requested installation and automatic installation after future Radar
  fixes in this workflow. Run the existing receipt/test/backup deployment path
  only with the game stopped, preserve user configuration, and do not infer
  permission to kill/launch the game, modify other mods, or publish anything.
- `Deploy-NativePrototype.ps1 -Diagnostics Preserve` passed: core tests 2/2,
  F6 overlay, compact, world-map, PostRender and release-hygiene checks, staged
  and installed payload validation, and exact native-build receipt checks.
- Installed CM-04 DLL: 1,129,984 bytes, SHA-256
  `D6C79578F3C19C033BB0A1F75EDB8382719CFDC87BB145D0D39ECB12FBDB7046`.
  Independently rehashed after deployment; it matches this turn's prechecked
  source binary. No source or native binary rebuild was needed in this turn.
- Four user files match pre-deployment bytes: `config/visibility.ini`
  (`03768807...ADF0255A0`), `config/diagnostics.ini`
  (`E888884C...5DAF10D`), `config/hotkeys.ini`
  (`B9672AC7...C01AEC0`), and `data/defaults/treasure_overrides.txt`
  (`CD52EE5C...00BA43`). Current values: `language=auto`,
  `debug_logging=true`, Settings F6 / Enable F7 / Disable F8.
- Installed popup overlay hash:
  `545BD546AC3AA6D4C28CE56C66DDBCEA0E3E0A361DD8D1B461B4361FE469A8A0`.
  `mods.txt` is byte-identical to its pre-deployment backup, with one enabled
  current Radar entry and no predecessor entry.
- Full rollback backup (project-relative):
  `dist/work/deployment/deploy-backups/20260907-025731-623-native-only-deploy`.
  Its `installed-native/dlls/main.dll` matches old CM-03
  `6FF206504516A663B96410C7D1F8BB0A55B0271CA81848A7D5BF86D6D55DFB71`;
  the old live debug log is also present there. The clean installed tree starts
  a new runtime log on the next game launch.
- Game process absent before and after install. No launch, runtime input,
  gameplay acceptance, public package refresh or GitHub action occurred.

### CM-04 controller runtime replay - 2026-09-07, 10:05-10:15 UTC

- **Status:** `RUNTIME_PASS_TESTED_CONTROLLER_ROUTES_ONLY`. Owner requested
  immediate controller testing and confirmed the installed version. Used the
  already deployed CM-04 candidate above; no rebuild or deployment this turn.
- **Method:** Foreground-guarded virtual Xbox input, verified 120 ms button
  presses and neutral releases. Two gameplay -> Start -> Hero -> Start ->
  gameplay cycles; cycle two additionally visited Hero Skill with D-pad Down.
  No mouse input was used during those menu routes. The first helper safely
  detached on its idle timeout while the second Start menu was open; a fresh
  helper continued that cycle. This is not an uninterrupted-device soak test.
- **Visual result:** No compact Radar icons or clock remained on the settled
  Start, Hero Information or Hero Skill pages. Both returns restored the
  gameplay Radar; the final gameplay view contained 35 compact markers and
  the clock. Controller button hints remained visible in the tested menus.
- **Trace:** First Start press at 10:06:15.961; native paint state becomes
  Hidden (2), compact_suppressed=true at seq 510 / 10:06:16.800. Gameplay
  return restores Visible (1), compact_suppressed=false at seq 531 /
  10:06:55.513. Second Start changes to Hidden at seq 542 / 10:07:12.309;
  second gameplay return restores Visible at seq 592 / 10:11:08.176.
  Corresponding menu activity samples retain cursor=false. This exercises
  native paint ancestry suppression rather than the mouse-cursor workaround.
- **Timing boundary:** The first Start-to-Hidden sample took about 839 ms,
  including the native fade and the existing 250 ms sampling edge. Settled
  page screenshots do not prove frame-perfect hiding throughout transitions.
- **Map-close control:** Two Radar-enabled and two F8-disabled View/B cycles
  are recorded separately as WM-09 in `WORLD_MAP_ATTEMPT_LEDGER.md`.
- **Evidence:** Original live log was copied, not cleared, to repository-root
  `.tmp/virtual-gamepad-0.1.2/controller-menu-cm04-20260907-1015/sampled.Native.log`;
  405,104 bytes; SHA-256
  `CB69E05494D70B2C53E9B46D483CF411034F009ED743287B527C4D95E946CEB5`.
  Verified input timestamps and screenshots are retained in the task history.
- **End state / scope:** F7 restored Radar after the disabled control. Game
  left running in gameplay with Radar enabled; virtual controller neutralized
  and detached at 10:15:16.007 UTC. Only test records and the local log copy
  were written; no source, configuration, installed payload, save, other Mod,
  package, or GitHub changes. Debug remained enabled.
- **Still pending:** Debug-off replay, CM-04 mouse/controller crossover,
  keyboard-only menu routes, other submenus, travel, first activation while
  native HUD is hidden, city-specific activation, different resolutions,
  physical controller variants and long-session testing. F6/AUTO language and
  custom hotkeys were not exercised. These results are not full 2.3.0 release
  acceptance and do not retroactively accept CM-03.

### Tooling observations

- Build scripts require the existing `cmake-3.29.6-complete` location and
  `ImGuiColorTextEditPinned`; the default CMake location is absent.
- Existing build/SDK outputs need the approved host permission context.
- Run legacy static PowerShell gates in Windows PowerShell: PowerShell 7's
  negative split-count semantics make the unchanged height-catalog verifier
  reject a valid TSV. Do not rewrite accepted catalog bytes to fix the shell.
- The release-tool digest also differs across shells because Sort-Object orders
  Build-Native.ps1 and Build-Native-Stable.ps1 differently. Rebuild and verify
  with the same Windows PowerShell host; do not weaken receipt verification.
