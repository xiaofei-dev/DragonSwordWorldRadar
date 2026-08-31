# Auto Pickup Attempt Ledger

Last evidence audit: 2026-08-31.

This ledger is the mandatory starting point for every future AutoPickup change. It records what each version actually changed, which runtime gate was reached, and which route must not be presented as new without new contradictory evidence.

## Current conclusion

- Version 1.3.0 is the current corrective release. It preserves the
  game-owned selector behavior, saved-binding Enhanced Input action, mounted
  Rider, fish, drop-item, treasure exclusion, session lifecycle, and no-scan
  safety route while removing the active fixed selector RVA. It resolves one
  process capability only when the reflected `Server_RunInteractV2` virtual-
  slot/CDO path and reflected `SetInteractUIV2` direct-wrapper path agree on the
  same selector target inside validated PE32+ and x64 `.pdata`/`CHAININFO`
  bounds. The policy is
  `runtime_reflection_dual_caller_rel32_consensus_fail_closed`. It permits one
  globally pending action keyed to the exact returned interaction Component.
  Exact Actor or Component invalidation, or that exact Component leaving the
  live interactable state, confirms success. The first 650 ms timeout permits
  one game-selector-represented retry after a 100 ms cooldown; the second
  timeout quarantines only that exact Component for the current activation.
  Interaction-owner replacement clears pending/attempt state and enforces a
  new 1500 ms settle period before scanning resumes. Retry cooldown and
  quarantine are checked before live action mapping and subsystem resolution.
  Every range PAK authors the exact `SphereOverlapComp.RelativeScale3D` in 19
  reviewed type-7 child packages, covering ordinary meat, aged meat, and the
  other class-proven monster drops without root/physics/hit collision mutation.
  Native runtime range multiplication is compile-time disabled. The release
  requires one true physical F9 edge per transition, retains strong normal and
  mounted identity, and compacts action/performance logging. There is no fixed-
  RVA or game-hash address fallback. The schema-60/schema-26 corrective source
  passed a fresh exact static, core, native-artifact, installer 10/10, and
  deterministic four-package audit. Process resolution, deployment, gameplay,
  and owner smoke-test acceptance remain pending.
- Static review caught one P1 before the final package: the reflected
  `SetInteractUIV2::GetFuncPtr` is a `.pdata`-bounded direct rel32 native
  wrapper, not a virtual-dispatch thunk. The resolver model was corrected to
  require one unique terminal `E8 rel32` implementation call and separate
  implementation runtime bounds before UI selector validation. No final
  artifact or runtime-acceptance claim predates this correction.
- The previous post-audit offline release passed static, source, core,
  built-artifact, installer 10/10, deterministic ZIP, exact-entry, and checksum
  gates. DLL: 836,096 /
  `6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1`;
  unsigned Setup: 12,118,016 /
  `2E2D3F69114751FCBB14A5E972F994A619736FC54CB9C6CE9B524D787B50DDCD`;
  installer ZIP: 8,420,756 /
  `B9344F5E519A29BF8CE5F560805C0040EFA409ADADE047EE92FDA9234C47A502`;
  manual without UE4SS: 319,886 /
  `3E0C4656F64C472A86C5C498EAD04C7E38A773C3E4F003B2E93DDCDB2226E261`;
  manual with UE4SS: 8,404,482 /
  `2EFFFF81BA2CD936BD73F15F37A2C518D301A3E8E5558E138D327EC6AAFB293E`;
  range ZIP: 3,120,870 /
  `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`
  bytes / SHA-256 respectively. These hashes predate the exact-Component
  preflight and strict owner-settle correction. They are historical offline
  provenance, not the current source candidate or runtime acceptance.
- Installed runtime evidence was bounded to the latest session that actually
  enabled Auto Pickup. Dynamic selector resolution succeeded with zero selector
  faults; scan CPU was normally tens of microseconds. Five of ten actions timed
  out under the older 1000 ms/no-retry behavior, and four exact candidates were
  permanently quarantined until an Off/On cycle. The same logs showed the
  interaction owner changing while `UWorld` remained stable. This confirms an
  action confirmation/lifecycle regression, not a fixed-RVA, game-hash, range-
  PAK, or UE4SS failure.
- Review found the first corrective source still allowed generic scheduler
  backoff to overwrite the 1500 ms owner settle deadline, keyed retry records to
  the Actor instead of the exact returned Component, and repeated avoidable live
  action-resolution work before checking retry cooldown. The current source
  fixes only those evidenced boundaries and keeps the dual-reflection selector,
  diagnostic-only game hash, fail-closed ambiguity handling, and one bounded
  retry.
- The installed legitimate 1.3.0 tuning DLL is 913,920 bytes with SHA-256
  `09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B`.
  Installer ownership recognizes that exact version/plugin/Lua tuple for Repair
  while mutated and unproven same-name payloads remain rejected.
- The current 750 ms confirmation-window / 200 ms retry-cooldown DLL is 919,552
  bytes / SHA-256
  `10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
  unsigned Setup is 13,001,728 /
  `07A8169881428F4CCD7F1773359762EBBEFFEC0C7DC3609671764715312B1656`.
  Installer, manual-without-UE4SS, manual-with-UE4SS, and range ZIPs are
  8,556,729 / `949268440029DA91DE7376AD7C45205B8BCB736D6435C4779FA7D5D198240D82`,
  362,055 / `16526C54A91A30804CEC96421D73597135CA781841BDE83142EB545E4ABA632A`,
  8,446,647 / `BECBB9E5F8BEBDCF5F28F7FC0CDD7C404FC09FC0977E6F85C4D64825DC37E98C`,
  and 3,919,600 /
  `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`
  bytes / SHA-256. These are offline artifacts only and were not deployed.
- The exact 1.2.0 release was built, packaged, and deployed. A bounded session
  recorded 250 injections, 17 confirmations, 30 timeouts, and 38 supersedes;
  one target received 61 injections. Both reviewed sessions also returned to
  Enabled after an intervening debounce rejection. These results reject 1.2.0
  observational confirmation, 250 ms same-target suppression, and rate-only
  F9 debounce as the next release policy without rejecting the accepted
  selector or Enhanced Input action.

- No version from `0.1.0` through `1.6.5` has visibly collected an ordinary ground drop automatically without the owner pressing the normal interaction key.
- The first 1.6.0 owner session reached the native selector four times, but every result failed `selector_actor_not_drop_item`; no RPC ran. This is a newly attributed runtime gate, not a selector or hotkey failure.
- The 1.6.1 attribution session made eight selector attempts. Every attempt selected the same `Herb_03_C` with `EInteractTypeValue::NormalGather` (`2`); all eight stopped before RPC. The owner subsequently clarified that F-collected normal gatherables, including herbs and bird eggs, are intended targets. The session therefore proved valid candidate discovery and exposed an overly narrow DropItem-only policy gate; it did not test the RPC action.
- Version 1.6.2 invoked the zero-parameter `Server_RunInteractV2` wrapper three times after valid NormalGather pairs; every target remained live and no pickup occurred. That direct RPC wrapper is rejected at the action-effect boundary.
- Version 1.6.5 then proved a later boundary: two valid `Eggplant_01_C` type-2 pairs, two correct foreground-process checks, and two complete 2/2 Windows F insertions still produced no pickup. Its final counters were `key_events=2`, `selector_pairs=2`, `input_attempts=2`, `input_records=4`, and `timeouts=2`.
- Version 1.6.6 reached two valid NormalGather pairs but stopped before injection at `active_action_not_live_on_physical_f`; it is `RUNTIME_REJECTED_AT_ACTION_RESOLUTION_GATE_NO_INJECTION`.
- Version 1.6.7 is `RUNTIME_ACCEPTED_ONE_PICKUP_PER_F9`: two separate `Herb_03_C` targets visibly collected, two exact Actor weak identities invalidated, and final counters showed two injections, two confirmations, and zero action failures/timeouts/selector faults.
- Version 1.6.8 preserves that accepted action and adds only an F9-toggle scheduler with one selector call per scan, 250 ms empty-range backoff, per-target confirmation, fresh current-Pawn resolution for walking/mounted play, and 1.5 second World-settle recovery. It is built but not deployed or runtime accepted.
- Version `1.5.0` repeated the reflected `UpdateButtonVisibilityByComponent` target-source route already runtime-rejected in `0.8.0`. Both versions registered the hook but received zero visibility callbacks.
- The manual interaction chain is not missing evidence. Versions `0.5.1` and `0.6.0` already captured the current player interaction receiver, exact target object/component relation, and zero-parameter `Server_RunInteractV2` during owner pickup.
- The 2026-08-21 offline native audit proved that `Server_RunInteractV2` does not trust a prefilled `ExecuteTargetObject` / `ExecuteTargetComponent` pair. Its implementation calls the game's own candidate selector, overwrites both fields from the selected pair, validates them, and then continues the interaction. Every retained active candidate incorrectly treated the transient fields or an external scan/UI signal as the authoritative target source.
- Build, package, deployment, hook registration, action logging, and a non-crashing session are not gameplay acceptance.

## 2026-08-21 offline native call-graph audit

Audit scope: read-only static analysis of Steam build `24831799`, executable SHA-256 `85E0F6BAFF78940C53451A282559A9378F6541E24B1CC81CD8CAF1556204A52E`, image base `0x140000000`, and the matching 2026-08-12 UE4SS CXX header dump. No runtime source was changed, built, deployed, injected, or exercised.

### Confirmed class and field identity

- The SDK places `DInteractableComponent.InteractableDataContainer` at `+0x100`, `ExecuteTargetObject` at `+0x228`, and `ExecuteTargetComponent` at `+0x230`.
- `FDsInteractableDataContainer.InteractableDataItemArray` is at container offset `+0x118`; therefore the live array pointer/count are at component offsets `+0x218` / `+0x220`. Each `FDsInteractableDataItem` is `0x20` bytes and contains weak `InteractionActor` and `OverlapComponent` identities.
- Constructor RVA `0x42A71C0` installs vtable RVA `0x7A877E8` at `0x42A71F4`, initializes the container at `+0x100`, and clears the two target fields at `0x42A72A4` / `0x42A72AB`. This ties the field layout and vtable to the SDK class rather than to a guessed neighboring type.
- The same vtable contains the neighboring generated methods at slots `+0x4A8`, `+0x4B0`, `+0x4B8`, `+0x4C0`, `+0x4C8`, and `+0x4D0`. Slot `+0x4C0` contains thunk RVA `0x42C84D0`, which jumps to `0x42C7F80`. The reflected exec thunk at `0x2E99620` also dispatches through `[vtable + 0x4C0]`. This proves that `0x42C7F80` is the native `Server_RunInteractV2` implementation.

### Confirmed target selection and writes

- `Server_RunInteractV2` calls selector RVA `0x42C0160` at `0x42C8037` with `this`, a 16-byte output pair, and a zero third argument.
- The selector reads the live `InteractableDataItemArray` at `0x42C02C9` / `0x42C02D0`, resolves weak actor/component entries, applies game-owned validity/state/range/priority logic, and writes the selected actor/component pair to its output at `0x42C09F5` / `0x42C09F8`.
- The server implementation copies that pair to `ExecuteTargetObject` at `0x42C804D` and `ExecuteTargetComponent` at `0x42C8065`, then rejects null/stale targets and continues through the interaction implementation.
- The action cleanup path clears the fields at `0x42C0C1D` / `0x42C0C2D`. A function-boundary audit of the confirmed `DInteractableComponent` code region found constructor clears, these server writes, and cleanup clears; it did not find another authoritative runtime writer before the RPC.
- `SetInteractUIV2` at `0x42C8AF0` also calls the selector, but it does not own the target-field writes. Calling or hooking a UI wrapper is therefore not equivalent to using the game-owned selector.

### Confirmed client/manual call chain

- The UTF-16 `Server_RunInteractV2` name initializes the matching static `FName`; generated client RPC stub RVA `0x40EF940` loads that name, resolves the UFunction, and dispatches it through the UObject ProcessEvent slot.
- Shared helper RVA `0x46B3640` first calls virtual slot `+0x18` on the caller-provided interaction interface to obtain the current `DInteractableComponent`. If non-null, it tail-jumps to the generated RPC stub.
- Direct callers of that helper were found at RVAs `0x442EEAD`, `0x5D3FDF4`, `0x616EB61`, and `0x61C14B5`. The `0x616EB61` call is inside the native `OnReleasedDropItemButton` implementation; exact semantic names for the other three native callers are not established by static evidence. All four converge on the same zero-parameter RPC stub.
- The exact keyboard-F caller cannot be named from the stripped executable alone, but the receiver acquisition, RPC stub, server implementation, selector, target writes, and cleanup are now closed as one native chain. No additional owner manual-pickup trace is required to rediscover it.

### Corrected failure model

The earlier assumption was backwards: `ExecuteTargetObject` and `ExecuteTargetComponent` are observable transient results, not authoritative inputs to `Server_RunInteractV2`. The implementation reselects and overwrites them from `InteractableDataItemArray`. Therefore:

- versions `0.5.2`-`0.5.4`, `0.7.x`, `1.3`, `1.4`, and `1.5` cannot validate this route by pre-reading, preserving, or filling those fields;
- versions `0.8` / `1.5` remain rejected because the reflected visibility callback is bypassed;
- versions `0.9` / `1.0` remain rejected because their visibility state never became active;
- versions `1.2` / `1.3` remain rejected because widget release is not the target selector and did not reliably populate a usable pair; and
- global UObject/Actor discovery is unnecessary for the game-owned candidate set already maintained by overlap state.

Retained `0.7.3`-`0.7.6` DLLs contain `ExecuteTargetObject` and `ExecuteTargetComponent` strings and no `InteractableDataItemArray` string; their documentation also preserves target-field gates. They therefore did not implement the selector-first route. Exact `0.7.0` source is still unavailable, but no retained source, binary, or log shows a selector call or a target-field-free RPC attempt.

### PRE_CHANGE_NOVELTY_GATE result

```text
Proposed route: One bounded, fingerprint-pinned selector-first canary; no automatic loop yet.
Target source: The game-owned native selector at RVA 0x42C0160 on the current player's DInteractableComponent.
Candidate discovery: Call the selector once on the game thread, then validate its returned Actor/Component pair as a live same-World ordinary DropItemActor relation.
Action: Call zero-parameter Server_RunInteractV2 once on the same receiver without reading, writing, or preserving ExecuteTargetObject / ExecuteTargetComponent.
Confirmation: Exact returned weak identities plus correlated target EndPlay / visible collection; ProcessEvent return alone is not success.
Prior versions compared: 0.1-0.6, retained 0.7.3-0.7.6 DLLs and 0.7.1/0.7.2 logs, 0.8-1.5, all retained Git sources.
New evidence: Exact selector, container offsets, vtable slot, generated RPC stub, server field writes, cleanup, and direct caller relationship for the current executable.
Why each prior rejection no longer applies: The route uses neither reflected/native visibility, widget release as target source, transient target-field writes, external Actor/UObject discovery, nor synthetic input. It asks the same game-owned selector used by the server implementation and lets the implementation reselect independently.
Expected one-version observable: One F9 press near an ordinary drop logs a non-null selector pair of type 2/7, invokes one RPC, and the same drop reaches EndPlay / disappears without owner F input. A null/non-drop pair stops before the RPC.
Rollback artifact: None yet; this audit changed documentation only. Any future candidate must preserve the currently installed rejected artifact before deployment.
```

The owner explicitly approved implementation after this declaration. Version 1.6 below is the resulting bounded candidate. That approval covered source and local build, not deployment, and is not runtime acceptance.
## 1.6.0 selector-first implementation record

Status: `RUNTIME_REJECTED_AT_ACTOR_CLASS_GATE`; compiled, deployed, installed-hash verified, and exercised by the owner.

- Active source: `src/ue4ss/main_1_6.cpp`; rejected 1.5 source remains retained separately.
- One F9 edge publishes one scalar request. No automatic pickup loop was added.
- The game-thread attempt resolves the fresh player receiver and calls selector RVA `0x42C0160` once with a 16-byte Actor/component output and third argument `0`.
- The pair must be complete, ordinary `DropItemActor` / `DInteractableComponent`, exact Outer-related, state `2/7`, and share the current Pawn World.
- The active source contains no names or access for either transient target field.
- Zero-parameter `Server_RunInteractV2` is called once only after validation.
- Confirmation retains one weak Actor identity for at most 1.5 seconds; visible collection remains the acceptance requirement.
- Exact game/UE4SS fingerprints fail closed. UI hooks, UObject/Actor scans, synthetic input, native detours, Lua scheduling, workers, mounted support, and deployment were not combined with this hypothesis.
- Strict native compilation passed. Local artifact: `334848` bytes, SHA-256 `B40CD0435C6454C51FB64BA52E23BD1E3CE43110FDE149D7462B1382A03A7FB3`.

Observed owner result: four F9 edges called the selector without fault, but all four returned an Actor that failed the `DropItemActor` gate. No pair passed validation, no RPC ran, and no item was collected.

## 1.6.1 selector-pair attribution record

Status: `RUNTIME_ATTRIBUTED_VALID_NORMAL_GATHER_REJECTED_BY_POLICY`; installed hash verified and behavior remained fail-closed.

- The pickup route, selector call, class/relation/World/state gates, and zero-parameter RPC call site are unchanged from 1.6.0.
- One non-null selector result now logs the exact Actor, component, component Outer, receiver identity/source, relation flag, `InteractableValue`, and `InteractTypeValue` before the existing rejection gates.
- The log is emitted only after an F9 edge; idle behavior, object-scan count, target-field access, and RPC count remain unchanged.
- Strict native build: `336384` bytes, SHA-256 `76865A93B8249BB10C23972FCA836246DD5A0C2C3117DE7445254A3390C57C9F`.
- The owner result was eight F9 edges returning `Herb_03_C.InteractComponent`, exact Outer relation, state `InteractableValue=2`, and `InteractTypeValue=2` (`NormalGather`). The active policy rejected it because the Actor was not a `DropItemActor`; `rpc_invocations=0`.
- Owner scope correction: `NormalGather` objects collected with F are intended, so the rejection was a Mod policy error rather than evidence of an unrelated selector target. No conclusion about RPC behavior is permitted from this session.

## 1.6.2 pickup-like selector canary declaration

Status: `RUNTIME_REJECTED_AT_ACTION_EFFECT_GATE`.

```text
Proposed route: Preserve the bounded selector-first one-shot route and correct only its pickup-like target policy.
Target source: The unchanged game-owned native selector at RVA 0x42C0160 on the fresh current-player DInteractableComponent.
Candidate discovery: One selector call returns one complete Actor/component pair; no UObject/Actor scan, UI hook, or synthetic input.
Action: Call zero-parameter Server_RunInteractV2 once after the corrected target policy passes.
Confirmation: Existing bounded weak-Actor probe plus owner-visible collection; NormalGather may remain live and can therefore log UNCONFIRMED despite visible success.
Prior versions compared: 0.1 through 1.6.1 and every retained route in the do-not-repeat index.
New evidence: Eight 1.6.1 attempts returned an exact, same-Outer, state-2 Herb_03_C NormalGather pair, and the owner confirmed that this is an intended F-collected target.
Why each prior rejection no longer applies: Target discovery already succeeded. This change does not revive scans, visibility hooks, KeyAction 13, widget release, synthetic F, or transient target-field writes; it changes only the policy that rejected a valid selector result.
Expected one-version observable: One F9 beside a type-2 normal gatherable logs one validated normal_gather pair, invokes exactly one RPC, and visibly collects the target without owner F input. Type 7 remains accepted only with DropItemActor class proof; all other types fail closed.
Rollback artifact: Preserve installed 1.6.1 before deploying 1.6.2.
```

Implementation delta:

- `NormalGather` (`2`) is accepted without requiring the Actor to derive from `DropItemActor`.
- `DropItemActor` (`7`) still requires `Actor.IsA(DropItemActor)`.
- Both paths retain complete-pair, `DInteractableComponent`, exact Outer, current-World, `InteractableValue=2`, and receiver-separation gates.
- Every other interaction type remains rejected. Bird eggs are covered if the game reports them as `NormalGather`; otherwise 1.6.2 logs their actual type and rejects it pending evidence.
- F9 remains one-shot. Persistent automation is deliberately not mixed into this runtime hypothesis.
- Owner runtime result: five F9 requests, three valid `Herb_03_C` NormalGather pairs, three `Server_RunInteractV2` ProcessEvent returns, two confirmation-pending rejections, three `actor_still_live` timeouts, and no visible collection.
- Reuse rule: keep the selector and type gates, but do not present another direct RPC wrapper/retry as a new action hypothesis.

## 1.6.3 selector-validated foreground-F canary declaration

Status: `NOT_EXERCISED_AT_SELECTOR_EMPTY`.

```text
Proposed route: Keep the proven one-shot selector and target policy, replace only the rejected direct-RPC action with the game's normal foreground F input path.
Target source: The unchanged native selector at RVA 0x42C0160 on the fresh current-player DInteractableComponent.
Candidate discovery: One complete selector pair; no UI target source, UObject/Actor scan, or target-field access.
Action: After validation, require current-process foreground PID, reject ConsoleWindowClass and an already-down F key, then send one scan-code F keydown+keyup pair.
Partial-input safety: If Windows reports one of two records inserted, attempt one standalone scan-code keyup recovery and fail the canary.
Prior versions compared: 0.9/1.0 compiled the same Windows helper but logged input_requests=0,input_sent=0; 1.6.2 reached a valid selector pair but its direct RPC had no effect.
New evidence: The selector is now proven to return an intended target, while the direct RPC is runtime rejected. Therefore this is the first selector-validated actual-dispatch test, not the first SendInput implementation.
Expected one-version observable: One F9 logs one validated pair, matched foreground state, F-up state, a mapped scan code, and requested_records=2/sent_records=2, followed by visible collection without owner F input.
Evidence ceiling: SendInput return 2 and GetLastError 0 prove OS queue insertion only, never game consumption or pickup.
Rollback artifact: `runtime/rollback/installed-1.6.2-20260822-212747Z`; deployment completed at `2026-08-22T21:27:47Z` with all four staged/installed hashes equal.
```

Owner runtime result: one F9 request produced one native selector call and `selector_no_candidate`. No pair passed validation, no foreground/input gate ran, and `input_attempts=0`, `input_pairs_sent=0`, and `input_records_sent=0`. This is not action rejection; the foreground-F hypothesis was not exercised.

## 1.6.4 bounded selector-window foreground-F canary declaration

Historical final status: `RUNTIME_REJECTED_AT_HOTKEY_INGRESS`.

```text
Proposed route: Preserve the complete 1.6.3 policy and action; replace only its single selector sample with one fixed bounded selector window after F9 release.
Target source: The unchanged game-owned native selector at RVA 0x42C0160 on a fresh current-player DInteractableComponent for every sample.
Candidate discovery: F9 press arms and release starts one game-thread window; sample at 50 ms intervals, at most 12 selector calls, and no longer than 750 ms. Retry only exact null/null selector_no_candidate.
Action: The first valid pair permits one unchanged 1.6.3 foreground scan-code F down+up dispatch attempt and no further selector sample; partial 1/2 insertion retains one key-up recovery.
Confirmation: Existing bounded exact weak identities plus owner-visible collection.
Prior versions compared: 0.9/1.0 contained but never requested synthetic F; 1.6.2 rejected direct RPC at effect; 1.6.3 tested one selector sample and ended empty before input.
New evidence: The only 1.6.3 owner attempt ended at selector_no_candidate. It did not exercise its action, so a narrowly bounded timing window can test whether candidate availability missed that one instant without changing discovery semantics or action.
Why each prior rejection no longer applies: The route does not revive visibility detectors, scans, UI target sources, direct RPC, transient target fields, or workers. It changes only timing around the already proven selector and retains every safety gate.
Expected one-version observable: WINDOW_STARTED reports 50/12/750 limits; each sample is attributed as no_candidate, fatal, or hit. A timing hit produces exactly one F-pair attempt and visible collection; otherwise the window exhausts or cancels within its fixed limit.
Rollback artifact: `runtime/rollback/installed-1.6.3-20260823-051420Z`; deployment completed at `2026-08-23T05:14:21Z` with all four staged/installed hashes equal.
```

Owner runtime result: The trusted/alive session at `2026-08-23T05:17:27Z` produced zero `CANARY_ARMED`, zero `CANARY_REQUESTED`, `canary_requests=0`, and `windows_started=0`. The selector and action were never exercised.

## 1.6.5 UE4SS F9-keydown ingress declaration

Status: `RUNTIME_REJECTED_AT_FOREGROUND_F_ACTION_EFFECT_GATE`.

```text
Proposed route: Preserve the complete 1.6.4 window, selector, validation, action, confirmation, and lifecycle route; replace only local press/release polling ingress.
Target source: Unchanged game-owned native selector RVA 0x42C0160 after one accepted request.
Candidate discovery: Protected CppUserModBase F9 keydown registration publishes one atomic event; on_update coalesces/debounces and publishes at most one unchanged bounded-window request.
Action: Unchanged first-valid-pair foreground scan-code F down+up pair with existing partial-release recovery.
Confirmation: Unchanged bounded weak identities plus owner-visible collection.
Prior versions compared: 1.6.4 required local polling to observe both press and release and produced zero arm/request/window records despite a trusted live session.
New evidence: The 1.6.4 failure is exactly before request publication. Moving ingress to UE4SS's owned keydown edge tests that gate without changing selector or action behavior.
Expected one-version observable: One F9 press logs KEY_EVENT_RECEIVED, CANARY_REQUESTED source=UE4SS_F9_keydown, and one WINDOW_STARTED. Later selector/action telemetry remains attributable to the unchanged route.
Rollback artifact: `runtime/rollback/installed-1.6.4-20260823-055158Z`; deployment completed at `2026-08-23T05:51:58.4187997Z` with exactly four staged/installed files equal.
```

Owner runtime result: two accepted UE4SS F9 events produced two complete `Eggplant_01_C` type-2 NormalGather selector pairs. Both attempts matched the correct foreground game process, and both `SendInput` calls requested two F records and inserted 2/2. Neither target was collected; both remained live through confirmation timeout. Final counters were `key_events=2`, `selector_pairs=2`, `input_attempts=2`, `input_records=4`, and `timeouts=2`. The foreground Windows F route is rejected at its effect boundary.

## 1.6.6 Enhanced Input Active-action canary declaration

Status: `RUNTIME_REJECTED_AT_ACTION_RESOLUTION_GATE_NO_INJECTION`.

```text
Proposed route: Preserve the proven F9 ingress, bounded selector, target policy, foreground gate, confirmation, and lifecycle controls; replace only the rejected Windows F action with one current-mapping-validated Enhanced Input action injection.
Target source: The unchanged game-owned native selector at RVA 0x42C0160 on the fresh current-player DInteractableComponent.
Candidate discovery: One UE4SS F9 event opens one 50 ms / 12-call / 750 ms game-thread selector window. Only exact null/null selector_no_candidate retries; there is no idle or global object scan.
Action: Resolve the current Controller's EnhancedPlayerInput, PlayerInputComponent, DefaultInputActionDataAsset, and Active action; require the exact Active pointer to equal a current non-ignored physical-F EnhancedActionMappings entry; resolve the current LocalPlayer EnhancedInput subsystem; call InjectInputVectorForAction once with Active and FVector(1,0,0), using empty modifier and trigger arrays.
Confirmation: Existing bounded weak target identities plus owner-visible collection; mapping/subsystem/injection telemetry alone is not success.
Prior versions compared: 0.3.8 exercised KeyAction 13; 0.9/1.0 compiled synthetic Windows F behind inactive visibility sources; 1.6.2 exercised the direct RPC wrapper; 1.6.3-1.6.5 used selector-validated foreground Windows F. None used the current LocalPlayer Enhanced Input subsystem to inject the exact current physical-F-mapped Active action.
New evidence: Version 1.6.5 proved candidate discovery, foreground ownership, and complete OS input insertion while the game still ignored the action. Current headers and live source expose the Active action mapping and reflected Enhanced Input injection boundary.
Why each prior rejection no longer applies: The action bypasses the runtime-rejected Windows message/input boundary, does not retry the direct RPC or KeyAction 13, and derives the action only from the current live physical-F mapping and current LocalPlayer subsystem. It does not use old visibility detectors, target-field writes, object scans, or cross-World UObject caches.
Expected one-version observable: One valid F9 window logs successful action/mapping and subsystem resolution, exactly one one-shot Enhanced Input injection, and visible pickup without owner F input. Any mapping or subsystem mismatch fails closed without injection.
Rollback artifact: `runtime/rollback/installed-1.6.5-20260823-070655`; deployment completed at `2026-08-23T07:06:55.9378673Z`.
```

Strict `/W4 /WX` compilation passed. The deployed 1.6.6 DLL SHA-256 is `8556A2ED797BDFE7B027E034C9CAD928DD7CC6AFABDEAD2FD86A22EB8139F3E8`. The owner session later produced two valid `Herb_04_C` pairs but both stopped at `active_action_not_live_on_physical_f`; `enhanced_input_attempts=0` and `enhanced_input_injections=0`. This rejects the guessed Active/raw-bool action-resolution implementation, not the still-unreached injection API.

## 1.6.7 live physical-F action canary declaration

Status: `BUILT_DEPLOYED_PENDING_OWNER_RUNTIME`.

```text
Proposed route: Preserve the complete 1.6.6 F9, selector, target, foreground, injection, confirmation, and lifecycle route; replace only the rejected action-resolution implementation.
Target source: Unchanged game-owned native selector RVA 0x42C0160 on the fresh current-player DInteractableComponent.
Candidate discovery: Unchanged one 50 ms / 12-call / 750 ms selector window; only selector_no_candidate retries.
Action: Traverse current EnhancedActionMappings with FScriptArrayHelper_InContainer; reflect Action, Key.KeyName, and packed bShouldBeIgnored; require exactly one non-ignored physical-F mapping and inject that exact live action once through the current LocalPlayer subsystem.
Confirmation: Existing bounded weak identities plus owner-visible collection; mapping/subsystem/injection telemetry alone is not success.
Prior versions compared: 1.6.6 used the same injection function but never reached it because it guessed IADA_Default.Active and misread the packed bool storage byte.
New evidence: The owner runtime proved one physical-F mapping exists while the old implementation reported it ignored and made zero injection attempts. The local CXX dump exposes bShouldBeIgnored as a bitfield whose whole storage byte contains unrelated flags.
Why the prior rejection no longer applies: 1.6.7 reads the reflected bool mask and derives the action directly from the unique current physical-F entry; it does not require IA_Active or inspect a raw flag byte as a boolean.
Expected one-version observable: One valid F9 window logs resolved=true, exactly one physical-F mapping, its reflected masks and exact action, one subsystem resolution, and at most one injection; visible pickup remains mandatory.
Rollback artifact: runtime/rollback/installed-1.6.6-20260823-075912; deployment completed at 2026-08-23T07:59:12.9207179Z.
```

Strict `/W4 /WX` compilation passed. The deployed 1.6.7 DLL is 362,496 bytes with SHA-256 `0F511EDD486DBD3D6AFE00C3BF6DA024D2C03E50333FF12DC9C8A766CD42ABC5`. Independent review found no remaining P0/P1 source issue. This is a safe bounded canary, not accepted automatic pickup.

## PRE_CHANGE_NOVELTY_GATE

Before editing runtime source, the developer must:

1. Read this entire ledger and the latest installed user/debug logs.
2. Name the proposed target source, candidate-discovery route, action route, and confirmation signal.
3. List every prior version that used any equivalent route.
4. State the new evidence that invalidates each prior rejection. A new wrapper, retry, cache, timer, or safety guard does not make the underlying route new.
5. If an exact historical delta cannot be recovered, mark it `UNKNOWN`; do not claim novelty.
6. Change one runtime hypothesis per version and define the expected observable result before implementation.
7. Do not build or deploy a runtime candidate unless this declaration proves that the route is new or that the earlier blocking evidence has materially changed.

Required declaration:

```text
Proposed route:
Target source:
Candidate discovery:
Action:
Confirmation:
Prior versions compared:
New evidence:
Why each prior rejection no longer applies:
Expected one-version observable:
Rollback artifact:
```

If `New evidence` or `Why each prior rejection no longer applies` is empty, the runtime change is blocked.

## Status vocabulary

| Status | Meaning |
| --- | --- |
| `STATIC_ONLY` | Source/build evidence exists; the route was not exercised in game. |
| `OBSERVATION_ONLY` | Runtime evidence was collected without an automatic pickup action. |
| `NOT_EXERCISED` | The intended action was blocked before its functional boundary. |
| `RUNTIME_REJECTED` | Runtime evidence disproved the tested route for the tested build/state. |
| `EVIDENCE_ACCEPTED` | A diagnostic fact was confirmed; this is not automatic-pickup acceptance. |
| `DUPLICATE_REJECTED_ROUTE` | The version repeated a previously rejected underlying route. |
| `UNKNOWN_DELTA` | The exact version delta is not recoverable from retained artifacts. |

## Version-by-version record

### 0.1.x to 0.3.x: baseline, pulses, lifecycle discovery, and KeyAction 13

| Version | Change or attempted route | Runtime result | Status and reuse rule |
| --- | --- | --- | --- |
| `0.1.0-observation` | Passive event-driven source for `DropItemActor` create/delete observation and manual interaction evidence. No pickup invocation. | Adapter was initially uncompiled because the pinned SDK checkout lacked UEPseudo. | `STATIC_ONLY`. Foundational safety design only. |
| `0.1.1-observation` | Restored the exact pinned SDK graph and compiled the passive adapter; retained weak identities, epoch/generation gates, F9, and no action. | Source, core tests, package layout, and native compile passed. No gameplay action existed. | `STATIC_ONLY`. Compilation is not a pickup attempt. |
| `0.2.0-active-canary` | Hooked `DInteractableComponent.OnBeginOverlap` and `Server_InputInteractKeyAction`; required two exact manual calibration matches before KeyAction 13 could be replayed. | Compiled but retained evidence does not establish deployment or an automatic action. | `NOT_EXERCISED`. Do not present class-specific reflected overlap/manual-hook calibration as new. |
| `0.3.0-hybrid-canary` | Lua queued a game-thread pulse every 150 ms by calling `IsLocalPlayerController`; C++ kept weak lifecycle candidates and attempted `Server_InputInteractKeyAction(13, candidate, Pawn)`. | In the F9-then-Radar-F7 crash session, AutoPickup had zero candidates and zero actions. The recurring Lua queue was removed because it overlapped the Radar queue. | `RUNTIME_REJECTED` as a scheduler architecture; action remained untested in this version. |
| `0.3.1-native-pulse-canary` | Removed the recurring Lua scheduler and post-hooked natural `PlayerController.ServerRecvClientInputFrame` as the native pulse. | Built/staged. The same pulse was exercised after relocation in 0.3.2. | `NOT_EXERCISED` independently; evaluate with 0.3.2. |
| `0.3.2-relocated-runtime-canary` | Kept 0.3.1 behavior but moved all paths to `Win64/ue4ss/Mods` and rebuilt against that exact runtime. | F9 and lifecycle capture worked, but `raw_hook_callbacks=0`, `accepted_pulses=0`, `world_ready=false`, and `actions=0`. | `RUNTIME_REJECTED`. `ServerRecvClientInputFrame` is not a usable pulse in the tested game flow. |
| `0.3.3-engine-tick-runtime-canary` | Replaced only the silent input-frame driver with UE4SS EngineTick post callback. | Recorded 5,376 EngineTicks and one valid player-chain resolution, but zero exact base-class captures after more than 347,000 create callbacks. | `RUNTIME_REJECTED` at exact-class discovery, not at the action. |
| `0.3.4-derived-drop-runtime-canary` | Replaced exact base-class comparison with `IsA` so Blueprint-derived `DropItemActor` objects could be captured. | Derived capture became possible, but normal-exit tests exposed registered UObject delete listeners surviving UObject-array shutdown. | Pickup remained unaccepted; listener lifetime was unsafe. |
| `0.3.5-shutdown-safe-runtime-canary` | Unregistered both UObject lifecycle listeners from `OnUObjectArrayShutdown`; pickup route remained EngineTick + lifecycle + KeyAction 13. | Addressed the confirmed exit assertion. No visible automatic pickup was accepted. | Lifecycle fix may be reused; it does not validate the action route. |
| `0.3.6-gate-attribution-runtime-canary` | Added stable gate-reason counters and bounded diagnostics; did not change discovery or action semantics. | `trusted=true`, `armed=true`, `world_ready=true`; two derived candidates, 2,273 candidate-nonempty pulses, 2,274 player-chain successes, and zero actions/failures. | `OBSERVATION_ONLY`. Do not repeat it as a functional fix. |
| `0.3.7-player-chain-attribution-runtime-canary` | Added detailed player-chain reason/timing attribution and F9 telemetry around the same route. | No retained evidence shows visible pickup or a newly exercised action boundary. | `OBSERVATION_ONLY`. Diagnostic extension only. |
| `0.3.8-mounted-pending-manual-contract-runtime-canary` | Added alternate/controller-bound Pawn support, action confirmation counters, and interaction capture. Invoked `Server_InputInteractKeyAction(13, candidate, Pawn)` on an eligible target. | Player chain succeeded 429/429; one candidate passed all gates at 2.441 m; the call executed, but the item remained and no delete confirmation occurred. Always-on listeners were also high-volume. | `RUNTIME_REJECTED` for KeyAction 13 as the tested pickup action. Mounted identity itself was observed. |

### 0.4.x to 0.6.x: runtime calibration, transient targets, and the accepted manual trace

| Version | Change or attempted route | Runtime result | Status and reuse rule |
| --- | --- | --- | --- |
| `0.4.0-calibrated-replay-bounded-discovery` | One bounded UObject/function calibration sweep; temporary whitelisted UFunction hooks; required an exact manual-call/deletion correlation before replay. No SendInput. | Swept 293,165 objects in 2,739 ms. Eight callbacks were rejected before contract capture; no contract/action. Exit later crashed while unregistering stale UFunction hooks during UObject shutdown. | `RUNTIME_REJECTED` as implemented. Never unregister saved UFunction hooks after UObject-array shutdown begins. |
| `0.4.1-calibration-path-discovery` | Discovered derived whitelisted UFunctions and changed candidate correlation to exact `InteractComponent` identity. | Registered 24 hooks. Manual pickup called only `Server_RunInteractV2` twice, both rejected as `candidate_relation_missing`; no contract/action. | `RUNTIME_REJECTED` at candidate correlation. |
| `0.4.1-calibration-path-discovery-exit-safe` | Exit-safe repair of 0.4.1 teardown; interaction hypothesis unchanged. | Static/lifecycle correction only; no distinct pickup acceptance is retained. | Reuse only the lifecycle correction, not the rejected correlation route. |
| `0.4.2-player-component-contract` | Broadened local-entry/receiver discovery around the current player component. | Fourteen `Server_RunInteractV2` calls were observed, all `candidate_relation_missing`; no contract or action. | `RUNTIME_REJECTED` at candidate relation. |
| `0.4.3-receiver-signature-probe` | Observation-only probe for exact receiver/Outer relation and reflected `CallActivePlayer` signature. | Confirmed two bool parameters: `IsActive`, `IsCutScene`. It performed no action. | `EVIDENCE_ACCEPTED` for signature only. Function meaning/pickup behavior was not proven. |
| `0.5.0-call-active-player` | Proposed `CallActivePlayer(true, false)` as the action while retaining candidate/range gates. | F9/player chain worked, but the only object was the `DropItemActor` class default object about 1.2 km away. `gate_eligible=0`; `CallActivePlayer` was never invoked. | `NOT_EXERCISED`. Do not call this action rejected or validated; do not reuse its template-object discovery. |
| `0.5.0-call-active-player-hotkey-poll` | Hotkey polling repair around the same candidate/action hypothesis. | No distinct action evidence; underlying gate remained blocked. | `NOT_EXERCISED`. Hotkey changes do not make the route new. |
| `0.5.1-observation` | Disabled mutation and logged template/world identity plus raw manual interaction receiver, targets, controller, and Pawn relations. | Proved the only discovered base object was the CDO. Manual pickup called zero-parameter `Server_RunInteractV2` on the player's interaction receiver; one trace targeted `Vitality_Leave_01_C.InteractComponent`. | `EVIDENCE_ACCEPTED`, not an auto-pickup implementation. |
| `0.5.2-vitality-leave-single-target-canary` | Used game-selected `ExecuteTargetComponent`; accepted only exact `Vitality_Leave_01_C`; invoked `Server_RunInteractV2` once per target transition. No object discovery loop. | Repeated tests reached F9/player chain but `gate_eligible=0` and `actions_invoked=0`. | `RUNTIME_REJECTED` as a persistent/general target source. |
| `0.5.3-vitality-instance-target-canary` | Added one bounded sweep, exact-instance/lifecycle matching, and a bounded manual interaction trace to the 0.5.2 route. | Still produced no eligible target or automatic action. | `RUNTIME_REJECTED`. Bounded sweep did not repair the transient-target assumption. |
| `0.5.4-vitality-instance-name-canary` | Relaxed the instance match to the `Vitality_Leave_01_C_` name prefix; action remained `Server_RunInteractV2`. | Still produced no eligible target or automatic action. | `RUNTIME_REJECTED`. Name-prefix matching is not a valid general ordinary-drop source. |
| `0.6.0-dropitem-closed-loop-diagnostic` | Read-only bounded capture of one real derived ordinary drop and correlation with the owner's normal manual pickup. | Locked `Drop_Item_BoarMeat_BP_C` at 1.250 m. Manual pickup confirmed current player receiver, exact drop/owned component targets, and zero-parameter `Server_RunInteractV2`. | `EVIDENCE_ACCEPTED`. Do not request the same manual trace again unless a game build or proposed call chain materially changes. |

### 0.7.x to 1.1: discovery rewrites, prompt visibility, and synthetic input

| Version | Change or attempted route | Runtime result | Status and reuse rule |
| --- | --- | --- | --- |
| `0.7.0` | First active source authorized from the 0.6 ordinary-drop receiver/target/RPC evidence. | No retained artifact cleanly isolates its exact delta or shows visible pickup. | `UNKNOWN_DELTA`. Treat any claimed recreation as non-novel until compared against adjacent 0.6/0.7.1 evidence. |
| `0.7.1` | One-shot global UObject sweep plus high-volume UObject create callbacks to discover existing/new `DropItemActor` objects. | Player chain 1,915/1,915; sweep and more than 273,000 create callbacks found zero candidates; no action. | `RUNTIME_REJECTED` for this discovery route. |
| `0.7.2-world-actor-query` | Removed global UObject listeners and called `UGameplayStatics.GetAllActorsOfClass` every 250 ms. | Sixty-six wrapper calls returned zero candidates. The wrapper could silently skip ProcessEvent, so its success attribution was invalid. | `RUNTIME_REJECTED` as implemented. Do not trust non-throwing wrapper return as execution proof. |
| `0.7.3-bounded-actor-registry` | Bounded current-UObject bootstrap plus Actor BeginPlay/EndPlay weak registry; later safety repair blocked incomplete/unsafe actions and pinned process lifetime. | Player chain 20/20; examined 309,586 indices in 2,856 ms; found zero candidates; no action. | `RUNTIME_REJECTED` at discovery for the tested ordering. |
| `0.7.4-corrected-drop-discovery` | Restored class-first classification before World checks, added tail-only scan and F9 debounce. | Game fingerprint changed, so the build stayed passive: zero sweeps, player-chain attempts, candidates, or actions. | `NOT_EXERCISED`. The correction was not runtime tested. |
| `0.7.5-current-build-fingerprint` | Added the exact new game fingerprint; behavior otherwise unchanged. | Armed and player chain 5/5; examined 81,920 indices before an unintended second toggle turned it Off. No candidate/action. | `NOT_EXERCISED` to completion. |
| `0.7.6-qualified-f9-release` | Required a continuously sampled 250 ms F9 release before rearm and blocked Off while discovery was incomplete. | Discovery/action unchanged; no retained visible-pickup acceptance. | Hotkey repair only. |
| `0.8.0-native-pickup-button-path` | Reflected hook of `DDropItemButtonUserWidget.UpdateButtonVisibilityByComponent`; selected `OnReleasedDropItemButton` as the higher-level action. | Hook registration succeeded, but `visibility_events=0`, `release_attempts=0`, and `release_invocations=0`. Static xrefs showed native callers bypassing the reflected UFunction hook. | `RUNTIME_REJECTED`. This exact target-source family was later repeated by 1.5. |
| `0.9.0-native-visibility-f-input` | Native detour of the component visibility implementation at RVA `0x61B3AC0`; an `Active=true` edge would send foreground F input. | Detour ran 809 times, all `Active=false`; no input request or input delivery. | `RUNTIME_REJECTED` at detector semantics. Synthetic F was not functionally tested by this version. |
| `1.0.0-dual-native-visibility-f-input` | Added the sibling icon visibility native detour while retaining the component detour and F input. | Both routes produced only `Active=false`; no input request. | `RUNTIME_REJECTED` at both visibility detectors. |
| `1.1.0-lifecycle-drop-f-input` | Replaced visibility detection with lifecycle-tracked live drops; sent F while a tracked drop was within 500 Unreal units of a fresh Pawn. No global scan. | A new game fingerprint forced passive-only operation before runtime callbacks registered. | `NOT_EXERCISED` on the new build. |

### 1.2 to 1.5: widget release, bounded scan, and the repeated reflected prompt hook

| Version | Change or attempted route | Runtime result | Status and reuse rule |
| --- | --- | --- | --- |
| `1.2.0-current-build-release-canary` | One qualified F9 found a same-World drop-button widget and invoked `OnReleasedDropItemButton`; no automatic loop. | The release returned without fault but did not visibly collect the item. Target fields populated in one session and stayed null in another. | `RUNTIME_REJECTED` as a complete action. |
| `1.3.0-validated-target-rpc-canary` | Invoked widget release, then required game-populated exact targets before calling zero-parameter `Server_RunInteractV2`. | Decisive test returned from release with both targets null; rejected `target_object_null`; RPC not invoked. | `RUNTIME_REJECTED` at target source. It does not disprove the RPC. |
| `1.4.0-world-drop-rpc-canary` | One F9 started a bounded class-first current-UObject snapshot, chose nearest same-World drop, temporarily filled null targets, then would call the RPC. | Examined 353,908 objects in 171 ms. The sole counted `DropItemActor` was the class default object counted before template exclusion; `eligible=0`; no RPC. | `RUNTIME_REJECTED` for the tested snapshot discovery state. |
| `1.5.0-prompt-target-auto-pickup` | Reflected `UpdateButtonVisibilityByComponent` hook captured weak prompt/component identities, then would validate and call `Server_RunInteractV2` on EngineTick. | Latest accepted logs show F9 On/Off working but `visibility_events=0`, `action_attempts=0`, and `rpc_invocations=0` across repeated sessions. | `DUPLICATE_REJECTED_ROUTE`: it repeats the already rejected 0.8 reflected target source. |

### 1.6: native selector-first route and policy attribution

| Version | Change or attempted route | Runtime result | Status and reuse rule |
| --- | --- | --- | --- |
| `1.6.0-selector-first-canary` | First retained direct call to the game-owned selector followed by the zero-parameter RPC after a DropItem-only gate. | Four selector calls returned non-DropItem Actors and stopped before RPC. | `RUNTIME_REJECTED_AT_POLICY_GATE`; selector and hotkey worked, action remained untested. |
| `1.6.1-selector-pair-attribution` | Added exact Actor/component/Outer/state attribution before the unchanged policy gates. | Eight calls returned the same valid Herb_03_C NormalGather pair; the overly narrow policy rejected it, RPC zero. | `EVIDENCE_ACCEPTED` for candidate discovery; owner scope correction invalidated the non-target interpretation. |
| `1.6.2-pickup-like-selector-canary` | Preserved the selector/one-shot route, accepted type 2 plus class-proven type 7, then directly invoked zero-parameter `Server_RunInteractV2`. | Five F9 requests produced three valid `Herb_03_C` NormalGather pairs and three RPC returns; two requests were rejected while confirmation was pending. All three confirmations timed out with the Actor live, and the owner observed no pickup. | `RUNTIME_REJECTED_AT_ACTION_EFFECT_GATE`; selector, receiver, relation, state, World, and type gates passed, but direct ProcessEvent produced no visible action. Do not repeat this RPC wrapper as a fix. |
| `1.6.3-selector-validated-foreground-f-canary` | Preserved every proven selector/type gate, removed direct RPC, then after explicit F9 would validate foreground PID/class and F-up state before one scan-code F down+up `SendInput` pair. | One F9 request made one selector call and returned `selector_no_candidate`; zero pairs and zero input attempts. | `NOT_EXERCISED_AT_SELECTOR_EMPTY`; the foreground-F action was never reached and must not be called rejected. |
| `1.6.4-bounded-selector-window-foreground-f-canary` | Preserved 1.6.3 exactly but sampled its selector every 50 ms within max 12 calls / hard 750 ms after F9 release. | Deployed successfully, but the trusted/alive `2026-08-23T05:17:27Z` session logged zero arm, request, canary-request, and window-start counters. | `RUNTIME_REJECTED_AT_HOTKEY_INGRESS`; selector and action remained untested. |
| `1.6.5-ue4ss-f9-keydown-bounded-selector-window` | Replaced only 1.6.4 local F9 press/release polling with protected UE4SS F9 keydown registration, atomic callback publication, and on_update coalescing/debounce. | Two valid `Eggplant_01_C` type-2 pairs passed foreground checks; two `SendInput` calls inserted 2/2 records; both targets remained live and no pickup occurred. Final counters: `key_events=2 selector_pairs=2 input_attempts=2 input_records=4 timeouts=2`. | `RUNTIME_REJECTED_AT_FOREGROUND_F_ACTION_EFFECT_GATE`; do not repeat selector-validated foreground Windows F as a new action. |
| `1.6.6-ue4ss-f9-enhanced-input-action-canary` | Requires guessed `IADA_Default.Active` to equal a current non-ignored physical-F mapping before one Enhanced Input injection. | Two valid `Herb_04_C` pairs and foreground checks passed, but both stopped at `active_action_not_live_on_physical_f`; zero injection attempts. | `RUNTIME_REJECTED_AT_ACTION_RESOLUTION_GATE_NO_INJECTION`; do not reuse the guessed Active/raw-bool implementation. |
| `1.6.7-ue4ss-f9-live-physical-f-action-canary` | Allocator-aware reflection selects the exact action from the unique non-ignored current physical-F mapping, then permits one injection. | Two owner F9 tests collected two different `Herb_03_C` targets; two injections and two exact weak-identity confirmations, with zero failures/timeouts/faults. | `RUNTIME_ACCEPTED_ONE_PICKUP_PER_F9`; accepted action baseline. |
| `1.6.8-ue4ss-f9-native-selector-automation` | Preserved 1.6.7 action and added F9 On/Off scheduling, one selector call per scan, adaptive backoff, sequential confirmation, and fresh current-Pawn resolution. | Owner testing proved persistent on-foot automatic collection. Mounted context changed to `DsVHC_WingCat_C` and its receiver, after which selector scans remained empty; 250 ms empty scans plus globally blocking 1.5 s confirmation/500 ms timeout backoff were too slow for fast traversal. | `RUNTIME_ACCEPTED_ON_FOOT_REJECTED_MOUNTED_AND_LATENCY`; retain the action, not the mounted receiver or blocking schedule. |
| `1.6.9-ue4ss-f9-mounted-fast-selector-automation` | Uses `ADsVehicleCharacter.Rider` as the mounted interaction owner, scans once per 33 ms, makes confirmation observational, suppresses only the same scalar target identity for 250 ms, and removes per-empty-scan logging. | Strict `/W4 /WX` build and core tests passed; 363,520-byte DLL SHA-256 `CD1A800346369BD3735380974BC7ED7426EFFA677A70D08309E0E8344F87A3B6`. Deployed with four exact staged/installed hash matches; 1.6.8 rollback preserved. | `BUILT_DEPLOYED_PENDING_OWNER_RUNTIME`; mounted and fast-traversal acceptance pending. |
| `1.6.10-ue4ss-f9-fish-perf-diagnostics` | Adds selector-returned Animal type 5 for runtime-observed trout/salmon, explicitly excludes TreasureBox type 4, deduplicates selector observations, aggregates engine-cadence plus per-stage average/max timings every 10 seconds, and batches each User/Debug flush into at most one file open per audience. | Owner testing collected and confirmed two trout through the mounted Rider route. The accepted performance session covered 1,739 scans with roughly 18-22 microseconds average, 378 microseconds maximum, zero slow scans, selector faults, automatic disables, object scans, or logger drops. | `RUNTIME_ACCEPTED_AUTOMATIC_MOUNTED_FISH_AND_PERFORMANCE`; exact future artifacts still require smoke testing. |
| `1.7.0-ue4ss-native-auto-pickup` | Preserves 1.6.10 gameplay logic and changes only release identity, public Debug default, documentation, verification boundaries, packaging, and local deployment tooling. | Strict build/package/deployment evidence is recorded independently. The public package stays Debug Off while the owner installation may use an exact config-only Debug override. | `RELEASE_CANDIDATE`; NormalGather, fish, chest exclusion, travel, and clean exit remain smoke gates for the exact rebuilt DLL. |
| `1.7.1-ue4ss-native-auto-pickup-range-x2-canary` | Kept the accepted selector/F-action route and attempted to lease the current player or Rider `InteractCollision` at exactly `2.0x`. | Runtime reported `interaction_collision_unavailable_or_world_mismatch`, `range_applies=0`, and native prompt distance only. | `RUNTIME_REJECTED_AT_RANGE_PRODUCER`; the receiver lease, configuration, transition state machine, counters, and tests were removed. |
| `1.7.2-ue4ss-native-auto-pickup-target-range-x2-canary` | Seeded supported target components and attempted to expand each target `InteractCollision`. | Runtime examined 332,258 UObject slots in about 19.129 seconds but produced zero applies and zero leases. | `RUNTIME_REJECTED_AT_RANGE_PRODUCER`; seed, constructor hook, retry queue, target lease, and metrics removed. |
| `1.7.3-ue4ss-native-auto-pickup-receiver-shape-x2-canary` | Captures weak identities from exact Begin/End overlap hooks, requires a matching receiver/OverlappedComp/OtherActor/OtherComp pair, then validates and leases the proven player-owned Box/Sphere/Capsule shape at exact 2x. | The owner session captured 172 Begin and 172 End events with 168 exact pairs, but every apply stopped at the broad root guard: `receiver_shape_applies=0`, `receiver_shape_rejects=105`, `shape_leases=0`; the observed producer belonged to `DsPC_Dana_C`. | `RUNTIME_REJECTED_AT_ROOT_PRODUCER`; never remove the root guard or resize the Character root capsule. Reuse only the exact overlap-pair evidence. |
| `1.7.4-ue4ss-native-auto-pickup-dedicated-range-x3-canary` | Proposed enabling `bCreateInteractCollision`, changing `BoxExtent`, and calling `Server_RegenerateInteractComponent` to request a dedicated 3x box. | Deeper disassembly rejected the premise before build: the reflected `Server_RegenerateInteractComponent` chain changes `InteractableValue` and schedules `SetInteractSwitchValue`; it does not access `bCreateInteractCollision`, `BoxExtent`, `BoxOffset`, or `InteractCollision`. RVA `0x42C2E00` is a lifecycle override and is unsafe to call manually on an already registered component. No 1.7.4 DLL or package was built or deployed. | `STATIC_BINARY_REJECTED_BEFORE_BUILD`. Never call RVA `0x42C2E00` manually and never claim the reflected regeneration function creates the collision box. The exact 1.7.3 source was restored from its 2,886-line pre-experiment capture and rebuilt successfully before the next route was started. |
| `1.7.5-ue4ss-native-auto-pickup-overlap-proxy-x3-canary` | Implemented the selector-correlated live-Begin proof and scalar-only exact-End promotion, then creates one plugin-owned deferred non-root `UCapsuleComponent` at exact 3x root radius/half-height. The inert proxy copies every root/main `ECR_Overlap` channel while all others remain `Ignore`, receives the exact Begin/End delegates through an exclusive verified handoff, and uses normal overlap events without synthetic activation migration. Cleanup performs bounded proxy-End/root-Begin reconciliation and issues `K2_DestroyComponent` as its final proxy operation with no later dereference. | Strict source checks, pinned build, deterministic four-file package, archive reproduction, and local deployment passed. DLL: `515584` bytes / `ECDE66501E490FD7954DDE6CFFAC6D8CF42CBCA89BD4B04A4FE02866A932DA0A`; archive: `230855` bytes / `15BAE11D78A5A4F5273B8C71AD24AE9ED7E1E48999B78F60114954CA242F2157`. Deployment completed at `2026-08-24T18:05:22.1331341Z` with a config-only local Debug override; public Debug remains Off. Rollback: `runtime/rollback/installed-release-20260824-110522`. No owner gameplay result exists yet. | `BUILT_PACKAGED_DEPLOYED_RUNTIME_PENDING`. The route does not resize the root, use the rejected lifecycle/regeneration path, scan UObject, poll targets, or invoke a new pickup action. Runtime acceptance still requires exact 3x proxy readback, root-delegate absence while active, visibly earlier native F availability, verified F9 restoration, walking/mounted/fish coverage, clean travel/exit, no blocking responses or post-destroy dereference, and no sustained performance regression. |
| `1.8.0-ue4ss-native-auto-pickup` | Restores the exact 1.7.0/accepted 1.6.10 no-range selector action and removes every 1.7.x runtime range subsystem. | Pinned build, core tests, no-range source gates, exact four-file package, and deployment passed. DLL: `373248` bytes / `8CBF018A7069162B03BA5083B2F665C20A92EACC91B0BDFD6BDADF92061F559A`. Optional 5x range is a separate PAK. | `DEPLOYED_RUNTIME_PENDING`; exact-artifact on-foot, Rider, fish, drop, treasure, travel, exit, and performance smoke tests remain. |
| `1.0.0` | Promotes the no-range route as the first public candidate, starts Off, derives its toggle ingress from `toggle_hotkey` (F9 default), uses authoritative `mods.txt`, and supports stable/nested ABIs plus an optional independent 5x PAK through the installer. | Core/configured-key gates and the 19-case offline installer matrix pass. Stable DLL: `573952` bytes / `FE78B221EAA234D564F996F96800049BCD7B2A856265FFC209E86660AE3837A1`; experimental DLL: `382464` bytes / `B25CD88FE953EA6F9893DC82E6760E3B14B3125F52B6B71A69F2C0CDD9422E37`; unsigned installer: `7155712` bytes / `AF47F1E234CB7DDDC1F413BF0F4BFB857619746EF094FEFBCC63E1FFBB46195B`. Final archive, deployment, and owner gameplay evidence remain pending. | `DUAL_ABI_AND_INSTALLER_BUILT_RUNTIME_NOT_VALIDATED`; build/install success does not imply gameplay acceptance. |
| `1.0.1` | Replaces physical-F selection with a one-shot saved semantic `INTERACT` (`ActionInputType=91`) resolver. `interaction_key=AUTO` is primary; an installer-selected concrete `interaction_key_fallback` defaults to F and is used only if semantic detection fails. A concrete `interaction_key` remains a troubleshooting override. | Core/source/package gates and the 20-case installer matrix pass. Stable DLL: `614912` bytes / `1D587F38E3CBA96577D7CB8374364DC7AB04BFFE77A0983B866ACED54AA097A4`; experimental DLL: `405504` bytes / `2DCB8301B6438AA283D4751DC2533AB1DE429B5C53DC8F3539F27F9A453E1068`; unsigned installer: `8473088` bytes / `D2CA3C8D38BB20540F1D4B4BA4CBF25E8B909FA2A36E7948CF2D4629AF8450DF`. Three deterministic release ZIPs were built; none was deployed. | `THREE_RELEASE_ZIPS_BUILT_RUNTIME_NOT_VALIDATED`; E/K rebinding, controller behavior, fallback use, Radar coexistence, and gameplay performance require owner testing. |
| `1.1.0` | Narrows public support to the tested ExperimentalNested UE4SS ABI and adds confirmed full-layout conversion with verified backup, old-layout removal, Mod migration, and backup-name collision suffixes. | Built, packaged, installed locally, and recorded with DLL SHA-256 `A5CE724E1F04F40A5346BDD8ECD9C7D2FF4854FCE33942E66149256D89A9A90D`; the inspected install uses F9, AUTO/fallback F, Debug Off, and the independent 10x range PAK. | `INSTALLED_FEATURE_COMPLETE_BASELINE_DERIVATIVE`; installation identity is not the 1.1.1 artifact. |
| `1.1.1` | Removes the game executable SHA-256 as an install/runtime activation gate while keeping it as diagnostics. Preserves the accepted selector, action, mounted Rider, fish/drop policy, semantic keyboard binding, and single tested UE4SS ABI. | Strict ExperimentalNested build and offline release gates pass. DLL: `404992` bytes / `3A2BA3B251910439E7625863EC4981A34AA692F62B136B51054349CDC4E29A92`; the four historical release artifacts are listed in `docs/EVIDENCE.md`. Not deployed. | `FEATURE_COMPLETE_RELEASE_BUILT_NOT_RUNTIME_RETESTED`; remaining work is exact-package smoke testing, not a new pickup implementation. |
| `1.2.0` | Preserves the accepted pickup route, forces Off on main-menu/save/World initialization, generation-binds playable hotkey acceptance, rechecks active Pawn World identity before the selector, revalidates foreground immediately before confirmation/injection, adds owned Install/Upgrade/Uninstall states, and expands the independent PAK choices to Original/3x/5x/10x/15x/20x. | Built, packaged, and deployed with DLL `413696` bytes / `9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6`. The reviewed diagnostic session recorded 250 injections, 17 confirmations, 30 timeouts, 38 supersedes, and one identity with 61 injections. Two F9 sequences returned to Enabled after a debounce rejection. | `RUNTIME_REJECTED_ACTION_STORM_AND_RATE_ONLY_F9_EDGE`; retain the selector/action route, not observational confirmation, 250 ms same-target retry, or debounce-only F9. |
| `1.3.0` | Preserves the game-owned selector and live Enhanced Input behavior while adding one global pending action, exact weak-invalidation success, exact-candidate timeout quarantine for the current activation with no retry of that identity, true physical F9 edge semantics, strong normal/mounted identity, compact bounded diagnostics, and fail-closed runtime selector resolution. The Server path resolves a reflected virtual slot through the interactable CDO; the UI path bounds the complete reflected exec wrapper, requires one terminal `E8 rel32` implementation call, and bounds that implementation separately. Both structural selector paths must identify the same address. No fixed RVA or game-hash address table remains. | The pre-compatibility DLL `422912` / `BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188` and Setup `11704832` / `570E9E81FA31DFE242999C8352311DF6E566D0B18DAEC2E103734C2A4D3447DB` remain historical. Static review corrected the UI-wrapper P1 before final packaging. The post-audit final offline release passed static/source/core/artifact gates, installer 10/10, and deterministic ZIP exact-entry/checksum gates. Final DLL: `836096` / `6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1`; Setup: `12118016` / `2E2D3F69114751FCBB14A5E972F994A619736FC54CB9C6CE9B524D787B50DDCD`; installer ZIP: `8420756` / `B9344F5E519A29BF8CE5F560805C0040EFA409ADADE047EE92FDA9234C47A502`; manual no UE4SS: `319886` / `3E0C4656F64C472A86C5C498EAD04C7E38A773C3E4F003B2E93DDCDB2226E261`; manual with UE4SS: `8404482` / `2EFFFF81BA2CD936BD73F15F37A2C518D301A3E8E5558E138D327EC6AAFB293E`; range ZIP: `3120870` / `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`. | `POST_AUDIT_OFFLINE_RELEASE_PASSED_RUNTIME_PENDING`; in-process selector resolution, deployment, gameplay, and owner smoke tests remain pending. Compatible code-contract relocation may resolve, arbitrary recompiles are not guaranteed, and failure remains Off with no historical-address fallback. |

## Route-level do-not-repeat index

| Route family | Versions | Evidence boundary |
| --- | --- | --- |
| Reflected `UpdateButtonVisibilityByComponent` target source | 0.8, 1.5 | Registered successfully but produced zero callbacks; native direct calls bypass the UFunction hook. |
| Native component/icon visibility as pickup availability | 0.9, 1.0 | Detours executed but every observed update was `Active=false`. |
| Visibility-gated synthetic F dispatch | 0.9, 1.0 | The input helper was compiled, but accepted logs had `input_requests=0,input_sent=0`; Windows input was never emitted. Reuse is permitted only behind a proven source and must not be described as a new helper. |
| Selector-validated foreground Windows F dispatch | 1.6.3-1.6.5 | Version 1.6.5 reached two valid pairs and inserted both requested 2/2 F records into the correct foreground process, but both targets remained live and no pickup occurred. This action family is runtime rejected. |
| `Server_InputInteractKeyAction(13, candidate, Pawn)` | 0.2 calibration concept; 0.3.x active path | 0.3.8 invoked it on an eligible target; the item remained. |
| Widget release as complete action or target source | 0.8 concept; 1.2, 1.3 | Release did not collect; target population was inconsistent and decisively null in 1.3. |
| Transient `ExecuteTargetComponent` / Vitality name as general discovery | 0.5.2-0.5.4 | Never produced an eligible automatic action. |
| Global UObject lifecycle discovery | 0.3.x, 0.7.1 | High callback volume, incomplete live-Actor coverage, and teardown risk. |
| `GetAllActorsOfClass` reflected wrapper polling | 0.7.2 | Zero candidates and false success attribution. |
| Bounded UObject snapshots as the assumed live-drop source | 0.4.x, 0.5.3, 0.6, 0.7.3/0.7.4, 1.4 | Useful for diagnostics in 0.6, but repeated automatic candidates often found only templates or zero live drops. |
| Repeating the same manual pickup trace | 0.5.1, 0.6 | Receiver, exact target pair, and zero-parameter RPC were already captured. |
| Direct zero-parameter `Server_RunInteractV2` after a valid selector pair | 1.6.2 | Three valid NormalGather attempts returned from ProcessEvent without visible pickup. A returned RPC wrapper is not action acceptance. |

## 1.3.0 exact-package startup regression and reflection-fix candidate

The first deployed final 1.3.0 DLL was 836,096 bytes with SHA-256
`6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1`.
At `2026-08-29T19:41:08Z`, it passed the pinned UE4SS fingerprint and recorded
the game hash under `game_hash_policy=diagnostic_only`, but disabled itself at
the reflection contract before any hotkey, scan, or selector work. The bounded
session had `key_events_received=0`, `automatic_scans=0`, and
`selector_attempts=0`. Every coarse object-presence field printed `true`, so
the failure was a hidden exact size, flag, property-class, or layout predicate.

This evidence rejects the range PAK, game-hash policy, pinned UE4SS payload,
and dynamic selector resolver as causes of that startup event: the resolver was
never executed. The unnecessary hard gate was the exact parameter schema of
`SetInteractUIV2`, even though the runtime never invokes that UFunction or
reads its parameters. The correction retains that schema as diagnostic data
and keeps every action-bearing reflected contract required. Selector safety is
unchanged: both runtime-resolved machine-code paths remain PE/runtime-function
bounded, independently validated, consensus-only, and fail closed with no
fixed RVA or hash-address table.

The candidate adds one named `required_failure` plus actual parameter sizes,
flags, classes, offsets, element sizes, array dimensions, and return-property
state to `REFLECTION_CONTRACT_DETAIL`. It was built from a clean pinned SDK
clone and passed source/core/artifact checks. DLL: 912,896 bytes / SHA-256
`4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F`.
The isolated unsigned Setup is 12,196,352 bytes / SHA-256
`4BEC3DCA4211E577A47E5DAC8CEBF106D4CB3FDED292CE06A164BA68438826C7`
and passed the current installer lifecycle matrix 10/10.

The installer now mirrors Radar state naming without weakening ownership:
current exact 1.3.0 is `Repair`, an older recognized release is `Upgrade`, and
an absent Mod is `Install`. Repair preserves `config.ini`, replaces only the
strictly owned Mod payload, safely reconciles the selected owned range PAK, and
keeps owned Uninstall available. The retained pre-compatibility and released
final 1.3.0 DLL hashes are explicit exact ownership contracts. Modified,
foreign, duplicate, or ambiguous same-name directories remain blocked. The
current installed reflection-fix candidate was inspected read-only as
`CanRepair=True`, `CanUpgrade=False`, `CanUninstall=True`, version `1.3.0`, and
owned 20x range; no game file was changed by that inspection.

The reusable clean SDK checkout is retained at
`build-sdk-clean-reflection-20260829-01` on commit
`1c1a1497f942c707f47ba668db75b25e86f6c08a`; its worktree is clean. Use it for
this candidate family instead of `.sdk/RE-UE4SS`, whose pre-existing
`deps/first/patternsleuth_bind/Cargo.lock` modification is unrelated. The first
local submodule initialization stopped because Git file transport was disabled;
rerunning only that initialization with `git -c protocol.file.allow=always`
completed it. This is a build-environment setup fact, not a source or runtime
failure, and should not be rediscovered by rebuilding from the dirty SDK.

One obsolete 20-case installer test was also attempted and stopped before all
fixtures because it still reflects the removed internal
`InstallerEngine.Install` method. The release pipeline uses
`Test-Installer110.ps1`; that current 10-case matrix passed. Do not treat the
obsolete harness failure as an installer-product failure or rerun it for 1.3.0.

Status: `DIAGNOSTIC_CANDIDATE_BUILT_NOT_DEPLOYED_RUNTIME_PENDING`. Do not call
the regression fixed until the exact candidate records
`REFLECTION_CONTRACT_DETAIL`, reaches `SELECTOR_RESOLVED`, accepts one physical
F9 press in a playable World, and visibly collects a supported target. If it
still fails, use the newly named clause and actual metadata to change only the
incorrect predicate.

The owner runtime session then reached `SELECTOR_RESOLVED`, resolved the saved
semantic `INTERACT` keyboard binding to `F`, accepted F9, and returned supported
type-2 selector pairs. It stopped before the first Enhanced Input injection at
`enhanced_action_mapping_field_metadata_mismatch`; counters remained
`enhanced_input_attempts=0`, `enhanced_input_injections=0`, and
`selector_faults=0`. Source comparison isolated the newly narrowed
`EnhancedActionKeyMapping.Action` check: the accepted baseline used the object
property family, while the corrective source required only exact
`FObjectProperty`. UE5 can reflect a `TObjectPtr<UInputAction>` field as exact
`FObjectPtrProperty`, which derives from `FObjectProperty` and provides the same
UE4SS object-property accessor contract.

The compatibility correction does not restore the broad family cast. It
explicitly allows only exact `FObjectProperty` or exact `FObjectPtrProperty`,
uses `GetObjectPropertyValue`, and retains the resolved `InputAction` class,
layout, alignment, mapping uniqueness, foreground, World, lifecycle, and
fail-closed checks. Weak, soft, interface, and unknown object storage remain
rejected. The former composite failure is split into named Action, Key,
ignored-flag, KeyName, and key-struct failures; bounded `ACTION_TRACE` records
the accepted Action storage kind. No direct RPC, Windows `SendInput`, guessed
Action, fixed selector RVA, or game-hash authorization was introduced.

The final offline release build now passes the strict native build, core tests,
built-artifact marker gate, current installer lifecycle matrix 10/10, exact ZIP
entry and checksum coverage, and deterministic archive reconstruction. Final
release DLL: 913,920 bytes / SHA-256
`01A6E1358FBFE0B9DB35B4ECAAB2F1F08425265F47D6B1873872AA56D9E002AC`.
Final unsigned Setup: 12,197,376 bytes / SHA-256
`5E9B2672879596805644AAA5A5DD2E9B574BBA93A421D38E69AF1FB9A2EA3487`.
The installer explicitly recognizes the installed 1.3.0 reflection candidate
`4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F`
as owned, so the final Setup exposes Repair instead of blocking that transition
as an unknown same-name directory. Modified or unproven payloads remain blocked.

Status: `ACTION_PROPERTY_COMPATIBILITY_OFFLINE_RELEASE_PASSED_RUNTIME_PENDING`.
Offline build/package success does not establish gameplay acceptance; the exact
artifact must still resolve the selector, report an accepted Action storage
kind, inject once, and visibly collect a supported target.

The owner then installed that exact 913,920-byte artifact and confirmed visible
pickup. The retained session dynamically resolved selector RVA `0x42C0160`
through Server/UI consensus, loaded saved `INTERACT=F`, accepted the physical F9
edge, and recorded `action_property_storage=ObjectPtrProperty`. It completed 11
Enhanced Input injections with zero injection failures and zero selector faults;
7 reached exact weak-identity confirmation. Four mounted normal-gather targets
remained Actor-live until the 1500 ms terminal timeout, so the one-global-pending
rule blocked all later targets during each window even though selection and
injection were inexpensive. Scan timing was approximately 25-32 microseconds
with no slow scans. This identifies the perceived latency as confirmation wait,
not scan frequency, selector resolution, saved-F binding, range PAK, or CPU cost.

The conservative follow-up changes only `kActionConfirmationWindow` from 1500
ms to 1000 ms. It does not change the 33 ms active/idle scan interval, permit a
second pending action, retry a timed-out candidate, remove quarantine, relax the
Action storage allowlist, or add a fixed selector address. The strict native
build, core tests, built-artifact gate, installer matrix 10/10, and deterministic
four-archive release gates pass. Tuning DLL: 913,920 bytes / SHA-256
`09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B`;
unsigned Setup: 12,197,888 bytes / SHA-256
`999ADF4D527A00947F56E747A40318DCF494B99D5710DF3CEE0CEAE8BEAB5EC2`.
Status: `CONFIRMATION_WINDOW_TUNING_OFFLINE_RELEASE_PASSED_RUNTIME_PENDING`.

## 2026-08-29 lifecycle-confirmation and inherited-drop range correction

Owner observation: Auto Pickup could visibly collect after the Action-property
repair, but later appeared to stop and caused intermittent pauses. The retained
process log resolved the selector dynamically at RVA `0x42C0160`, recorded zero
selector faults, and kept scan work in the tens of microseconds. The terminal
failure was one Actor-live timeout permanently excluding that exact candidate
for the activation. The same log showed travel replacing the interaction owner
while preserving the `UWorld` identity, so the old World-only reset did not
clear the stale attempt state.

Rejected explanations: the selected 20x PAK, UE4SS ABI, game hash, and fixed
selector address were not causal in that session. The active selector was
resolved at runtime, the hash was diagnostic-only, and neither the selector nor
scan timing showed a fault. Repeated injection and broader object enumeration
remain rejected solutions.

Corrective policy:

- retain one global pending action and dynamic dual-path selector consensus;
- confirm exact actor/component invalidation or the exact pending component
  leaving `InteractableValue=2` after guarded Outer/type/World validation;
- after the first 1500 ms timeout, record a 200 ms cooldown and admit at most
  one retry only when the game selector presents the same exact weak identity;
- quarantine that identity only after the second timeout;
- when the interaction-owner weak identity changes inside the same `UWorld`,
  reset pending/retry state and apply the existing 1500 ms settle delay; and
- retain the fail-closed action-state capacity and physical F9 edge.

Range evidence: extracted ordinary meat, aged meat, animal meat, coins, nuts,
crystals, and other type-7 child packages all inherit
`DropItemActor.SphereOverlapComp`. Their serialized child components expose
physics and hit spheres, not an unambiguous authored pickup radius. Patching
those child assets would risk item physics. The accepted narrow bridge detects
exactly one owned 3x/5x/10x/15x/20x PAK filename and, at DropItemActor BeginPlay,
uses exact reflected `FObjectProperty`, `FFloatProperty`, and `FBoolProperty`
contracts to call `SphereComponent:SetSphereRadius` on that inherited overlap
sphere. It performs no UObject/Actor scan and never resizes the physics or hit
spheres. No PAK requires no callback; multiple PAKs or reflection drift disables
only drop-range extension.

Evidence completed: strict MSVC compilation against the existing pinned build
configuration and all core tests pass. The canonical clean-dependency build is
currently blocked because the pinned RE-UE4SS checkout already contains a
modified `deps/first/patternsleuth_bind/Cargo.lock`; that third-party file was
not cleaned or overwritten. Release packaging, deployment, and gameplay
acceptance remain pending.

## Current gate

Version 1.6.10 remains the accepted selector/action, mounted-fish, and bounded-
scan baseline. The deployed 1.2.0 release proves that route still executes but
rejects its action scheduling and toggle-edge policy because it produced action
storms and repeat-driven state transitions. Version 1.3.0 is the current
corrective line: one pending action, exact invalidation/state confirmation, one
selector-represented retry after the first timeout, second-timeout
exact-candidate quarantine, same-World interaction-owner reset, true physical
F9 edge, strong normal/mounted identity, compact diagnostics, and
the fail-closed asymmetric dual-path selector resolver is implemented. The
resolver uses validated PE32+ and x64 runtime-function bounds, requires
the `Server_RunInteractV2` virtual-slot/CDO path and the complete
`SetInteractUIV2` direct-wrapper/terminal-implementation path to reach selector
address consensus, and has no fixed RVA or game-hash address fallback. It
supports compatible code-contract relocation, not arbitrary recompiles.
Historical exact offline build/package provenance and the installer 10/10 plus
deterministic ZIP gates are complete for earlier 1.3.0 artifacts. The current
200 ms retry-cooldown source has also passed a fresh clean-dependency native
build, artifact verification, installer matrix 10/10, and deterministic package
gates. Deployment, gameplay, performance, and owner smoke-test acceptance for
that exact artifact remain pending.
Versions 1.7.1 through 1.7.5 are historical general range experiments and must
not be reintroduced. The reviewed authored range remains in the optional PAK;
only inherited `DropItemActor.SphereOverlapComp` receives the narrow
BeginPlay/exact-reflection bridge described above. Retrying root resize, the rejected
`0x42C2E00` lifecycle call, overlap proxies, guessed `IA_Active`, raw bool-byte
tests, direct pickup RPC, foreground Windows `SendInput`, KeyAction 13,
inactive visibility sources, target/global UObject scans, or continuous
injection is not new evidence. An additional game build is valid only if the
runtime contracts resolve uniquely; otherwise automation must remain Off.
Automatic saved-gamepad binding selection and installer signing are optional
future enhancements, not unfinished core features.

## 2026-08-30 bounded retry-cooldown tuning

Owner testing reported that the 1500 ms confirmation window, 500 ms retry
cooldown, and one-retry lifecycle appeared stable, but requested slightly
faster recovery. Prior runtime evidence showed that reducing the confirmation
window to 1000 ms caused false timeouts, so the confirmed 1500 ms upper bound
remains unchanged. Only `kAutomaticActionRetryDelay` changes from 500 ms to
200 ms. Normal success is unaffected because exact invalidation or the exact
Component leaving the interactable state confirms immediately; only a first
unconfirmed timeout reaches this cooldown. The same exact selector-presented
Component still receives at most one retry, and a second timeout still
quarantines it for the current activation. Dynamic selector consensus,
diagnostic-only game hash, owner/context reset, range bridging, and all
fail-closed gates remain unchanged. Static, native, installer, and package
gates passed using the clean pinned dependency worktree. Native DLL: 925,696
bytes / `5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08`;
unsigned Setup: 12,210,176 bytes /
`3EF1D37E771123CC8DCC192EC18CB89F62EE5823F245E78E39B7BF3CA0570898`.
Installer, manual-without-UE4SS, manual-with-UE4SS, and range ZIP SHA-256:
`6B333A0ECDF9A7AF125E614DFC7A64BF03610BD83D1B5A4AB5FF080DE3EB1149`,
`70AAFB9B4B49FEAF3ED698FBC32D37FC78F4690AD76598BEED2DFB5CEA1DB2DB`,
`FF39DD1339CC9C2CF39F1578CDD70C0F606250150EF30EC83C55E2B647CD4014`,
and `90D8C6296A8D1E6607B4E6532AC3DBC6E6524062DD55B1F5F8A32D568C0754CB`.
Deployment and gameplay acceptance remain separate and were not performed.

Installer follow-up: the first 200 ms Setup omitted the immediately preceding
final action-lifecycle DLL hash
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
from its historical ownership contracts, so an otherwise legitimate 1.3.0
installation could be reported as unverifiable. The corrected Setup adds only
that exact version/DLL/Lua tuple and an explicit regression assertion. Repair
may replace that owned payload while preserving configuration; modified or
foreign same-name files remain fail-closed and are never deleted.

## 2026-08-30 PRE_CHANGE_NOVELTY_GATE: confirmation-window tuning

New owner evidence from the exact installed 200 ms retry-cooldown build shows
that the first pickup succeeds, while the next target is delayed. The retained
runtime log resolves the selector with zero faults and records nine injections,
nine unconfirmed terminal results, and approximately 1500-1530 ms per pending
window. Scan, selector, and injection work remain in the microsecond range, so
the dominant delay is the confirmation window rather than enumeration or CPU
cost.

The proposed delta changes only `kActionConfirmationWindow` from 1500 ms to
750 ms. It retains one global pending action, the 33 ms scan cadence, the 200 ms
retry cooldown, at most two exact-Component attempts, second-timeout quarantine,
dynamic selector consensus, and every fail-closed lifecycle and ownership gate.
A same-target failure remains bounded to approximately 750 + 200 + 750 ms;
different selector-presented targets can advance after the first 750 ms timeout.
The earlier 1000 ms candidate was rejected because one timeout caused permanent
quarantine. That rejected policy is not being reintroduced: the current bounded
retry and second-timeout quarantine remain intact. Expected runtime evidence is
shorter pending gaps without overlapping or continuous injections. Native,
installer, package, deployment, and gameplay acceptance remain pending.

## 2026-08-30 POST_CHANGE_OFFLINE_ACCEPTANCE: 750 ms and structured drop PAKs

The scoped timing change is implemented: the confirmation window is 750 ms,
while the 33 ms scan cadence, one global pending action, 200 ms retry cooldown,
two-attempt maximum, and second-timeout exact-Component quarantine remain
unchanged. A failed same-target action is therefore bounded to approximately
1700 ms; successful invalidation/state evidence can confirm earlier.

All five range variants now contain 50 reviewed gather/animal packages plus 19
structured `/Script/DS.DropItemActor` child packages (69 targets / 138 entries).
The exact `SphereOverlapComp.RelativeScale3D` is authored without changing
`CapsulePhysicsComp` or `SphereHitComp`; treasure assets are absent. Native
runtime range multiplication is compile-time disabled.

The installer now treats the five supported range filenames and legacy canary
filename as product-owned replacement/removal targets without consulting a
historical PAK hash. The selected embedded PAK remains hash-verified after write.
The final installer matrix passed 10/10 isolated fixtures.

Final local artifact identities are DLL 919,552 /
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
unsigned Setup 13,000,704 /
`3027460CE5BC5EDA7F6DC8735245E14455A7851E9D990C1D027119269699E6AB`;
installer ZIP 8,556,264 /
`4263919A5AC55389D9C100DD93BBBB808025E48E878ACC6F7875A0BB65A17634`;
manual-without-UE4SS 362,055 /
`16526C54A91A30804CEC96421D73597135CA781841BDE83142EB545E4ABA632A`;
manual-with-UE4SS 8,446,647 /
`BECBB9E5F8BEBDCF5F28F7FC0CDD7C404FC09FC0977E6F85C4D64825DC37E98C`;
and range ZIP 3,919,600 /
`504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.
Static, core, native-artifact, PAK semantic/roundtrip, installer, deterministic
ZIP, exact-entry, and checksum gates passed. Nothing was deployed or launched;
gameplay and owner smoke-test acceptance remain `NOT_VALIDATED`.

## 2026-08-30 OWNERSHIP_SCHEMA_2_AND_CANONICAL_LAYOUT

The recurring predecessor-hash failure is closed at its source. New Setup
records use ownership schema 2 with stable product ID
`8F4282F9-25C4-4EFC-A150-1C4A812C85B4` and exact installed DLL, Lua, and
notices hashes. Later installers validate the record and closed owned layout,
so a normal next release needs no manually copied predecessor DLL hash. Changed
immutable files still fail closed. The installed `5490BCC4...CDA08` schema-1
payload is an explicit one-time migration fixture.

The rebuilt installer matrix passed 10/10, including schema-2 fresh install,
Repair, immutable-payload tamper rejection, exact predecessor recognition, and
filename-owned range replacement. Setup is 13,001,728 /
`07A8169881428F4CCD7F1773359762EBBEFFEC0C7DC3609671764715312B1656`;
installer ZIP is 8,556,729 /
`949268440029DA91DE7376AD7C45205B8BCB736D6435C4779FA7D5D198240D82`.
The other three archive hashes are unchanged.

The new Setup inspected the actual old game install as owned and uninstallable,
then its transactional Uninstall removed the old Mod directory, 20x PAK, and
`mods.txt` authority entry. Post-uninstall state was `IsInstalled=False` and
range `Absent`. The new release remains local and not gameplay validated.

## 2026-08-31 owner-observed confirmation and retry latency tuning

The exact Debug-on runtime isolated Radar and confirmed that the diagnostic
location-read fix restored action effects: 38 Enhanced Input injections
produced four exact confirmations with zero injection, selector, action-state,
logger, or slow-tick faults. The four successful confirmations completed in
approximately 542-584 ms. Unconfirmed attempts dominated perceived pauses
because one global pending action remained occupied until the 750 ms bound and
the same exact target then waited 200 ms before its one retry.

The local tuning changes only `kActionConfirmationWindow` from 750 ms to 650 ms
and `kAutomaticActionRetryDelay` from 200 ms to 100 ms. The follow-up changes
engine/active and post-pickup cadence from 33 ms to 25 ms while retaining the
33 ms idle cadence. One global pending action, maximum two attempts, second-
timeout exact-target quarantine, selector, target policy, and input route remain
unchanged. A fully
unconfirmed same-target cycle is reduced from about 1700 ms to about 1400 ms.
The exact tuned DLL still requires owner gameplay validation.
