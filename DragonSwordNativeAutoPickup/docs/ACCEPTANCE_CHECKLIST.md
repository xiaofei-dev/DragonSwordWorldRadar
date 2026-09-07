# Acceptance Checklist

## Feature-completeness gates

- [x] One runtime-resolved native selector call per due scan and the accepted
  live Enhanced Input action for an accepted result.
- [x] Public launch state Off and configurable toggle key with F9 as default.
- [x] One physical F9 press causes at most one transition; repeat keydown events
  remain latched out until release.
- [x] Fresh current Pawn and mounted Rider resolution without cross-World
  gameplay UObject retention.
- [x] NormalGather type 2, Animal type 5, and class-proven DropItemActor type 7
  supported; TreasureBox type 4 excluded.
- [x] Saved semantic `INTERACT` keyboard binding resolved once per enable, with
  a concrete fail-closed fallback and troubleshooting override.
- [x] One global in-flight action blocks every later automatic invocation only
  while that injection remains armed.
- [x] One post observer is registered for the exact reflected
  `Server_RunInteractV2` UFunction. The record is armed before injection and
  remains live after return until dispatch, existing exact confirmation,
  timeout, or reset. While armed, the observer performs only raw receiver
  comparison and atomic marker publication: no logging, reflection, UObject
  read, game call, or action-state mutation.
- [x] EngineTick consumes a matching dispatch marker, releases the global slot,
  records `target_match_unproven=1` and `pickup_success_claim=0`, and applies a
  750 ms same-Component re-entry delay.
- [x] One selector-represented retry is allowed 200 ms after the first
  no-dispatch fallback timeout; the second result applies a 1500 ms expiring
  backoff, never activation-long quarantine. Neither delay writes a global
  scan deadline, so another selector result may proceed immediately.
- [x] Same-`UWorld` interaction-owner replacement clears pending/retry state and
  applies the normal world-settle delay.
- [x] World-settle delay and bounded failure backoff retained.
- [x] No UObject or Actor scan, overlap hook, broad `ProcessEvent` hook,
  root/physics/hit collision
  mutation, direct pickup RPC, Windows synthetic input, recurring Lua scheduler,
  or worker thread.
- [x] Each range PAK authors 19 exact type-7
  `DropItemActor.SphereOverlapComp` scales; 15x/20x cap these short-lived drop
  targets at 10x while keeping their 50 gather/animal targets at the selected
  multiplier. Native runtime range multiplication is compile-time disabled.
- [x] Owner runtime evidence confirms on-foot automatic pickup, mounted Rider
  pickup, fish pickup, and bounded performance through the 1.6.10 baseline.

These gates establish the 1.3.1 source and range policy. Historical range
experiments and rejected action routes must not be treated as unfinished work.
The earlier pre-compatibility and pre-observer 1.3.0 artifacts remain
historical. The exact 1.3.1 offline package and build identity now pass static
gates. Exact-package deployment, selector/observer runtime behavior, gameplay,
and owner smoke testing remain separate and pending.

## 1.3.1 high-range drop-list patch gates

- [x] Preserve native target selection, input, scheduling, retry, confirmation,
  and F9 behavior; only release identity may change in the DLL.
- [x] Reuse the exact 1.3.0 3x, 5x, and 10x PAK bytes and hashes.
- [x] Keep all 50 authored gather/animal targets at 15x or 20x in the matching
  high-range variant.
- [x] Require all 19 drop targets and 38 packed drop entries in each 15x/20x
  PAK to match the reviewed 10x output exactly.
- [x] Preserve 69 targets, 138 PAK entries, zero treasure targets, and protected
  physics/hit components in every variant.
- [x] Preserve all seven explicit 1.3.0 ownership tuples and add a recorded
  schema-2 1.3.0 to 1.3.1 Upgrade fixture.
- [ ] Test the exact 1.3.1 10x, 15x, and 20x PAKs after separate game restarts,
  including fast mounted travel through dense animal and destructible drops.
- [ ] Confirm no stuck prompt list, normal automatic recovery, manual F,
  treasure exclusion, World travel, and clean exit.

## 1.3.0 source contract

- [x] Main-menu and every new save or World initialization force automatic
  pickup Off and clear session-only interaction state.
- [x] Re-enabling requires the configured toggle after a playable World loads.
- [x] The tested ExperimentalNested UE4SS ABI remains unchanged.
- [x] Strong LocalPlayer/controller/current-Pawn identity is required for both
  normal and mounted play.
- [x] Compact action diagnostics replace the 1.2.0 per-injection seven-line
  User-log chain; detailed performance output remains interval-aggregated.
- [x] The game executable hash remains diagnostic-only.
- [x] No active selector path contains a fixed RVA, hash-to-address table, or
  historical-address fallback.
- [x] The exact UE4SS hash and loaded-path gate runs before gameplay reflection
  or selector resolution.
- [x] The loaded PE32+ `.text`, x64 `.pdata`, and bounded `CHAININFO` records
  bound decoded native implementation fragments.
- [x] The reflected `Server_RunInteractV2` exec thunk must identify one unique
  virtual slot; the interactable CDO entry and bounded native implementation
  must expose one structurally valid selector call.
- [x] The same exact reflected `Server_RunInteractV2` UFunction owns the sole
  per-function post observer; shutdown unregisters only this Mod's hook IDs and
  never calls a global unregister-all operation.
- [x] The complete reflected `SetInteractUIV2` exec wrapper must contain one
  unique terminal `E8 rel32` native-implementation call; that bounded
  implementation must expose one structurally valid UI selector call.
- [x] The asymmetric Server virtual path and UI direct-wrapper path must resolve
  the same selector address.
- [x] Resolution failure, ambiguity, caller disagreement, executable-page
  failure, or a guarded selector fault keeps automation Off for the process.
- [x] Original, 3x, 5x, 10x, 15x, and 20x are the complete mutually exclusive
  range-selection set.
- [x] The standalone PAK owns 50 gather/animal targets plus 19 class-proven
  type-7 drop targets (69 targets / 138 entries), including ordinary and aged
  meat; treasure/type-4 assets remain excluded.
- [x] Engine/active/post-invocation due is 25 ms and idle cadence is 33 ms;
  dispatch consumption does not add another 25 ms wait and its EngineTick may
  continue scanning.
  The no-dispatch fallback window is 750 ms, retry delay is 200 ms,
  same-Component dispatch re-entry is 750 ms, and terminal no-dispatch backoff
  is 1500 ms.
- [x] The release contract contains exactly four package classes: installer,
  manual without UE4SS, manual with UE4SS, and standalone range PAKs.

## 1.3.0 offline release gates

The following must be checked only after the corresponding commands and
isolated fixtures pass:

- [x] ExperimentalNested native build passes strict compilation and exact
  artifact verification.
- [x] Public configuration starts Off and uses Debug Off.
- [x] Install, Upgrade, Repair, and Uninstall state inspection passes absent, owned,
  and unknown same-name fixtures.
- [x] The exact preceding `38DA6C...` 1.3.0 DLL/Lua/version tuple exposes
  Repair, while a changed DLL or Lua hash remains blocked with zero mutation.
- [x] Exact-runtime Upgrade preserves `config.ini`, uses temporary rollback,
  and creates no persistent conversion backup.
- [x] Uninstall removes only owned Auto Pickup files, its `mods.txt` entry, and
  owned approved range PAKs while preserving UE4SS and unrelated Mods.
- [x] Conversion requires confirmation and passes complete verified backup,
  old-layout removal, unrelated-Mod migration, and backup-name collision
  suffix fixtures.
- [x] All six range choices pass mutual-exclusion and embedded-hash checks.
- [x] `mods.txt` is authoritative and legacy `enabled.txt` is absent.
- [x] All four release archives pass structure, English-only, provenance, and
  checksum audits.
- [x] Third-party Mods, including `ZeroKarya_PartySwitch`, are excluded.
- [x] Synthetic resolver fixtures cover relocated layouts, duplicate or
  embedded patterns, caller disagreement, truncated instructions, invalid jump
  chains, and fail-closed ambiguity.
- [x] PE runtime fixtures cover malformed PE32+ headers, invalid `.text`,
  malformed runtime-function tables, chained fragments, loops, and bounds.
- [x] Static review caught and corrected the P1 assumption that
  `SetInteractUIV2::GetFuncPtr` was a virtual-dispatch thunk before any final
  package was produced.

Final offline artifact evidence:

- [x] Native DLL: 919,552 bytes /
  `10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`.
- [x] Unsigned Setup: 13,001,728 bytes /
  `2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`.
- [x] Installer ZIP: 8,556,743 bytes /
  `70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`.
- [x] Manual without UE4SS ZIP: 362,076 bytes /
  `9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`.
- [x] Manual with UE4SS ZIP: 8,446,667 bytes /
  `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`.
- [x] Standalone range ZIP: 3,919,600 bytes /
  `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.

The checked resolver items above establish source and deterministic fixture
behavior only. They do not prove that either the reflected Server virtual path
or the reflected UI direct-wrapper path resolves in a particular game process.

## Dispatch-observer local candidate

- [x] Pass source, manifest, package-layout, mods.txt, core, native, and
  built-artifact gates.
- [x] Record and deploy 927,744-byte DLL
  `AC86CF2FA26047CF713B567C1CA63D4AD424C86A3FF9C020B80CAD07B4211F5D`
  while the game is closed.
- [x] Verify the installed DLL matches ownership schema 2 while preserving
  `config.ini` and `mods.txt` byte-for-byte.
- [ ] Launch and complete the runtime items under Exact 1.3.0 release smoke
  test before accepting the candidate.

## 1.3.0 reflection-fix candidate gates

- [x] Record the failed deployed artifact hash and prove that UE4SS identity
  passed while key, scan, and selector-attempt counters remained zero.
- [x] Report every required reflection predicate by a stable failure name and
  log the actual bounded parameter metadata.
- [x] Keep the unused UI parameter schema diagnostic-only while preserving all
  action-bearing reflection gates and dual-path machine-code consensus.
- [x] Preserve no fixed RVA, no game-hash address table, diagnostic-only game
  hash, exact pinned UE4SS, and fail-closed ambiguity/fault behavior.
- [x] Build against a clean exact pinned SDK checkout.
- [x] Pass source/core/native-artifact checks and current installer matrix
  10/10.
- [x] Expose Repair for the current exact owned version, Upgrade for older
  recognized versions, Install when absent, and strictly owned Uninstall only.
- [x] Confirm Repair preserves `config.ini`, uses no persistent backup on the
  exact runtime, keeps unknown same-name Mod targets fail closed with zero
  mutation, and reconciles exact supported range-PAK filenames by name.
- [ ] Deploy the exact candidate DLL
  `4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F`.
- [ ] Capture `REFLECTION_CONTRACT_DETAIL` and either a conclusive
  `SELECTOR_RESOLVED` or named fail-closed selector result.
- [ ] Confirm one physical F9 transition and one visible supported pickup.

## Native status-card visual smoke test

- [x] Keep status timing in a pure model with unit coverage for queued
  Starting-to-result transitions, smoothstep midpoint, expiry, and travel clear.
- [x] Require native UMG, hit-test-invisible widgets, guarded UI failure, and no
  pickup/input/hook ownership in the static source gate.
- [x] Build and verify the exact ExperimentalNested native DLL with strict
  warnings enabled.
- [ ] In gameplay, confirm the top-center translucent card is readable over
  bright terrain and dark interiors at the owner's normal resolution.
- [ ] Confirm `STARTING...` resolves to `ENABLED` or `NOT READY`, F9 Off shows
  `DISABLED`, and every card fades away without leaving a stale widget.
- [ ] Confirm mouse, keyboard, controller, mounted play, World travel, and clean
  exit are unchanged while the card is visible and after it expires.
- [ ] Review the exact deployed candidate before treating the visual design as
  accepted; build and hash verification alone are not visual acceptance.

## Exact 1.3.0 release smoke test

The following unchecked items are release QA, not feature-development tasks:

- [ ] Deploy the exact 1.3.0 package and verify its installed DLL hash.
- [ ] On the reference game build, confirm one `SELECTOR_RESOLVED` record proves
  Server virtual-path/UI direct-wrapper consensus and that no
  `SELECTOR_UNAVAILABLE` or `SELECTOR_FAULTED` record follows.
- [ ] If another game build is available, confirm it resolves by compatible
  code contracts rather than by hash. If it does not resolve, confirm F9 cannot
  enable automation and no selector invocation occurs.
- [ ] Confirm launch-Off and one transition per physical F9 press, including a
  held-key/operating-system-repeat test, on foot and while mounted.
- [ ] Confirm returning to the main menu disables Auto Pickup and a new load
  requires F9 again.
- [ ] Confirm one normal gather, one fish or monster drop, and treasure
  exclusion.
- [ ] Confirm one matching `PICKUP_DISPATCH_OBSERVED` releases the global
  in-flight slot without claiming target-level pickup success, then confirm a
  different candidate can proceed while the first Component remains inside its
  750 ms re-entry delay.
- [ ] Force one controlled no-dispatch fallback timeout and confirm a single
  selector-represented retry after 200 ms; force the second no-dispatch result
  and confirm a 1500 ms self-expiring Component backoff, manual F availability,
  continued processing of other candidates, and automatic recovery without an
  Off/On cycle.
- [ ] Collect ordinary meat, aged meat, and one other monster drop from beyond
  original range with exactly one expanded PAK, then confirm Original/no-PAK
  behavior and that only one supported range filename is installed.
- [ ] Confirm World travel, clean exit, and no sustained frame-time regression.
- [ ] Confirm Install, Upgrade, Repair, and Uninstall from the exact public
  installer.
- [ ] Confirm 15x and 20x independently in dense areas if those options are
  included in public support.

## Optional enhancements

No enhancement below is required for the completed current scope:

- automatic selection of the saved gamepad binding instead of a concrete
  configured gamepad key;
- Authenticode signing for the one-click installer.

The final pre-observer 1.3.0 installer matrix passed 10/10 isolated fixtures and
its exact artifact/archive identities remain historical evidence. They do not
validate the dispatch-observer repair. Every unchecked row above remains open.

Status: `DISPATCH_OBSERVER_CANDIDATE = RUNTIME_PENDING`.

Status: `INSTALLED_ARTIFACT_HASH = NOT_APPLICABLE_UNTIL_DEPLOYED`.
