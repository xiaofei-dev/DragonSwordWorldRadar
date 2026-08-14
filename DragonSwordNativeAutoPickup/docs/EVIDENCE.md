# Evidence

## 0.8 live rejection and 0.9 native-detour correction

- The installed 0.8 session recorded `visibility_events=0`, `release_attempts=0`, and `release_invocations=0`; automatic pickup code never ran.
- The executable contains direct native call xrefs to the visibility implementation at image RVA `0x61B3AC0`. Those calls bypass UE4SS's reflected UFunction hook even though hook registration reports success.
- Version 0.9 uses the already pinned PolyHook2 dependency to detour the native implementation after validating the exact game hash and its first 16 bytes. The detour calls the original first and records only the component pointer and Active edge.
- A new active edge queues one foreground-only F scan-code press/release from EngineTick. This tests the user's simpler hypothesis through the game's normal input handling without reconstructing the internal interaction RPC.
- The pinned native build succeeds with SHA-256 `645E1D68171D88FE533F7457F3394F61EC13D888B803846FA0614DEFF260B843`. The same hash was deployed at `2026-08-13T16:20:39Z`. This is build/deployment evidence only; it is not runtime accepted.

## 0.8.0 native pickup-button path

- The current `DSClient.hpp` dump declares `UDDropItemButtonUserWidget::UpdateButtonVisibilityByComponent(const UDInteractableComponent*, bool)` and the zero-parameter `OnReleasedDropItemButton()` handler.
- Static disassembly of Steam build `24693558` localized the native release handler and showed that it resolves game-owned player/context state before entering the interaction/UI/inventory flow. The press handler primarily changes pressed visual state and is not the selected action.
- This provides a bounded game-owned candidate signal and a higher-level action boundary. Version 0.8 therefore removes the active global object bootstrap, Actor registry, radius/player-chain gates, Pawn target writes, and direct `Server_RunInteractV2` call.
- The pinned native build succeeds with artifact SHA-256 `2C4B7A81E6AA87B6CF0002EA98F2D45FF7C920C3B160A0B9540C92ED13A9ECD1`; core tests also pass. The same hash was deployed to the standard `Win64\ue4ss\Mods` location at `2026-08-13T15:53:45Z`. This is build/deployment evidence only; no automatic pickup, mounted support, travel safety, or normal-exit claim is made yet.
- The superseded `src/ue4ss/main.cpp` is retained as historical evidence but is no longer linked by CMake.

## 0.7.5 F9 cancellation and 0.7.6 qualified-release correction

- Version 0.7.5 passed the current game fingerprint and armed at `2026-08-13T12:50:30Z`.
- The player chain succeeded 5/5 and discovery examined 81,920 indices, but the state returned Off one second later before the baseline completed. No candidate evaluation or action occurred.
- Input telemetry recorded 61 held-key repeat rejections and two debounce rejections. A later false release sample followed by a down sample outside the 750 ms interval could therefore manufacture a second accepted toggle.
- Version 0.7.6 requires 250 ms of continuously sampled release before rearming F9 and rejects an Off transition while discovery is incomplete. Discovery and action logic are unchanged. Runtime acceptance remains pending.

## 0.7.4 passive-only result and 0.7.5 fingerprint correction

- Steam updated DragonSword Awakening to build ID `24693558`. The installed executable is 162,562,968 bytes, modified `2026-08-13T06:09:01Z`, with SHA-256 `3DDDCEE474825310000A4CD24239AE5C8B76EF81BAC223C9F3D52565816A0CEA`.
- At `2026-08-13T11:40:51Z` and again at `11:46:01Z`, 0.7.4 recorded `trusted=false` and `PASSIVE_ONLY`. The tested UE4SS hash remained the accepted `F31188D...BE1`.
- The F9 session therefore ended as `Disabled/ContractInvalid`; `discovery_sweeps_started=0`, `player_chain_attempts=0`, `candidates_evaluated=0`, and `actions_invoked=0`. This session did not exercise the 0.7.4 discovery correction or pickup action at all.
- Version 0.7.5 adds only the exact current executable hash to the fail-closed allowlist and retains the prior tested hash. This permits a new canary test; it does not prove that the updated game retained compatible runtime semantics.

## 0.7.3 live failure and 0.7.4 discovery correction

- The decisive 0.7.3 session armed at `2026-08-12T19:58:04Z`. Its player chain succeeded 20/20 times, and the bounded bootstrap completed after examining 309,586 UObject indices in 2,856 ms, but `discovery_candidates_found=0`, `candidates_evaluated=0`, and `actions_invoked=0`.
- This localizes that failure before eligibility and action invocation. It does not disprove the accepted `Server_RunInteractV2` call chain because that chain was never reached.
- The 0.6.0 diagnostic previously captured one real derived `Drop_Item_BoarMeat_BP_C` by classifying the UObject first. Version 0.7.3 instead called `GetWorld()` during the global-array walk. Version 0.7.4 restores the known-success class-first scan and revalidates World ownership later from the weak identity. This is evidence-guided, but runtime has not yet proved that the premature World filter was the sole cause.
- The 0.7.3 log also recorded multiple Off/Armed/Off transitions after the long bootstrap. Version 0.7.4 adds a 750 ms monotonic debounce while retaining press/release edge detection; the owner should still press F9 exactly once for the next test.
- Actor lifecycle callbacks produced no candidate in the brief armed interval. Version 0.7.4 retains them and adds a tail-only scan for UObject indices allocated after the completed bound. It does not rescan earlier indices or restore global UObject create/delete listeners.
- The 0.7.4 artifact is compiled and statically verified only. Gameplay pickup, travel, mounted behavior, and exit safety remain unaccepted.

## 0.7.2 live failure and 0.7.3 bounded Actor registry correction

- In the decisive 0.7.2 F9 session, the player chain succeeded 132/132 times while 66 `GetAllActorsOfClass` wrapper calls returned zero candidates and no action gate ran.
- The pinned SDK wrapper returns `void` and silently exits when its GameplayStatics CDO or reflected UFunction is unavailable. The 0.7.2 adapter nevertheless marked every non-throwing call successful, so `actor_query_failures=0` did not prove that ProcessEvent executed.
- The action contract, radius, state, component, and target-field gates therefore remain unchanged. Version 0.7.3 replaces only discovery: the exact 0.6 bounded scanner covers already-existing actors, then native Actor BeginPlay/EndPlay callbacks maintain weak identities.
- This does not restore the rejected high-volume UObject create/delete listener. The bootstrap has a fixed snapshot bound, advances at most 16,384 indices with a 2 ms soft budget per pulse, and stops after completion.
- That first 0.7.3 build exposed additional static blockers and was superseded before deployment: incomplete-bootstrap action, cross-pulse uniqueness, overflow truncation, unproven mounted invocation, target-field ownership, and exit-resident worker lifetime.
- The repaired candidate blocks actions until bootstrap completion, performs complete same-pulse selection, latches overflow/thread/budget/guarded failures closed, limits invocation to the evidence-backed on-foot mode, preserves game-owned target fields, retains once-per-identity attempt history across EndPlay, owns no background logger/fingerprint task, and pins its DLL against an unsafe host unload path. Its final artifact identity is recorded in `metadata/build-fingerprints.json`. It remains not deployed and not gameplay accepted.

## 0.7.1 live failure and superseded 0.7.2 query attempt

- In the latest 0.7.1 session, F9 armed successfully and the player chain succeeded 1,915/1,915 times, but the one-shot sweep found zero candidates and more than 273,000 create callbacks still captured zero `DropItemActor` instances.
- This proves the active failure occurred before action gating: the global UObject lifecycle callback was not a reliable completed-Actor discovery signal for the tested drops.
- Version 0.7.2 removed both the one-shot global UObject sweep and all global UObject lifecycle listeners, then queried through the pinned SDK's `UGameplayStatics::GetAllActorsOfClass` wrapper every 250 ms. Runtime evidence later proved that the adapter's success attribution was invalid, so this discovery route is superseded by 0.7.3.

## 0.6.0 ordinary-drop closed-loop result accepted for 0.7.0 source

- F9 armed the read-only diagnostic and locked one real derived `DropItemActor`, `Drop_Item_BoarMeat_BP_C`, at 1.250 m.
- The owner manually collected that exact item. The correlated trace captured `/Script/DS.DInteractableComponent:Server_RunInteractV2` on the current Pawn's `InteractableComponent` receiver with zero parameters.
- The receiver property FNV-1a hash was `400295510`, which resolves to `InteractableComponent`.
- `ExecuteTargetObject` and `ExecuteTargetComponent` on that receiver matched the locked drop owner and its owned `InteractComponent` at the call.
- This closes the earlier ordinary-drop action gap and authorizes the narrowly scoped active source in 0.7.0. It does not establish runtime acceptance, mounted support, multi-target selection, or any other interaction function.

## Runtime evidence through 0.3.8

- 0.3.8 was trusted and active. Its exact player chain succeeded 429/429 times, including 396 expected-character and 33 controller-bound alternate-Pawn pulses. Mounted identity support is therefore demonstrated for that build.
- Three derived DropItemActor candidates were captured. Object 333287 passed all exact gates at 2.441 m.
- `Server_InputInteractKeyAction` was invoked with KeyAction 13, current candidate, and current Pawn, but the item remained after 1,503 ms. `confirmed_by_delete=0`; ProcessEvent invocation did not produce pickup.
- No manual capture reached that exact function. A normal manual pickup therefore uses another path.
- Approximately 801,000 create and 521,000 delete callbacks in a short session proved that always-on lifecycle listeners are materially expensive.
- Rate limiting did not create a true F9 edge; later OS repeats still toggled.

## Static entry evidence

The installed DSClient name pool around `DInteractableComponent` contains `CallActivePlayer`, `SetInteractUIV2`, `ClientSetInteractUIV2`, `Server_RunInteractV2`, `Server_InputInteractKeyAction`, `Server_InputInteractKeyAfterAction`, and `ServerReturnInteract_EnableState`. Controller-related pools contain `Interact`, `OnPressInteractionButton`, `OnReleaseInteractionButton`, `Client_InteractActionSetting`, `Client_InteractActionSetting_Restore`, and `ServerReturnInteractState`, with fields including `InteractActor`, `InteractionActor`, `InteractActorUID`, `PickupInteractComponent`, `CurrentPC`, and `BeforePC`.

Names alone do not prove ownership, signature, call direction, or replay safety. Version 0.4.0 resolves exact UFunction metadata at runtime only during calibration, logs bounded scalar metadata, and refuses automatic mutation unless a recreatable local entry is positively correlated with exact deletion. It never uses SendInput.

## Owner calibration evidence from 0.4.0

- The exact trusted 0.4.0 artifact started and completed its activation sweep in 2,739 ms over 293,165 object indices. The maximum measured batch was 2,633 microseconds.
- One base DropItemActor remained in the candidate set while the global delete-callback counter increased by one during manual pickup. The raw log did not contain an exact candidate index+serial correlation, so this is not evidence that the discovered object was the removed item. The earlier stronger statement is retracted.
- Eight temporary-hook callbacks were observed, but all were rejected before contract capture: `calibration_calls=0` and `contracts_validated_by_delete=0`. No contract file was produced and automatic pickup never became active.
- Static metadata registration found component function IDs 0-6 and only controller-side ID 12. The likely local controller entry was not available at the hard-coded native path. The previous direct component-Outer correlation also provided no accepted call.
- Version 0.4.1 responds narrowly: the same one-time bounded sweep discovers whitelisted derived UFunctions by exact owner hierarchy, component correlation uses exact `InteractComponent` identity, and diagnostics attribute rejection by function and reason. This is an evidence-driven correction, not runtime acceptance.

## Pinned SDK evidence

The adapter compiles against RE-UE4SS `1c1a1497f942c707f47ba668db75b25e86f6c08a` and UEPseudo `b2e876da82b17254c04304746341c8fde0ddb37c`. The SDK provides exact `UObjectGlobals::RegisterHook/UnregisterHook`, global-index `FUObjectArray::GetNumElements/IndexToObject`, listener add/remove, and unregisterable EngineTick/world-reset callbacks. UE4SS keydown/on_update execute on the program loop, so UObject control operations are explicitly deferred to EngineTick.

## 0.4.1 live calibration result

- The 2026-08-11 16:23 session discovered two base DropItemActors and continuously resolved the player chain successfully.
- The bounded function sweep examined 51,009 UFunctions and registered 24 exact whitelisted hooks.
- Manual pickup called only `/Script/DS.DInteractableComponent:Server_RunInteractV2` twice. Both calls were rejected as `candidate_relation_missing`; `calibration_calls=0`, `contracts_validated_by_delete=0`, and `actions=0`.
- This proves 0.4.1 could never enter automatic mode. Its exact DropItemActor `InteractComponent` assumption did not describe the live call receiver.

## Unverified for 0.4.2

- Which newly discovered bounded local entry, if any, a normal manual ordinary-drop pickup exposes.
- Whether the resulting persisted contract replays successfully in normal and mounted play.
- Discovery completion time and per-batch cost in the live object population.
- Travel, menu, dungeon, cutscene, long-session, coexistence, and clean-exit behavior.

Build success and static evidence do not establish runtime acceptance.

## Confirmed 0.4.0 exit teardown failure

- The latest owner exit test crashed after 68 seconds while Unreal reported `IsRequestingExit=true`.
- The resolved stack is `NativeAutoPickup::OnUObjectArrayShutdown` -> `NativeAutoPickup::unregister_calibration_hooks` -> UE4SS `UObjectGlobals::UnregisterHook`.
- UE4SS `UnregisterHook(UFunction*, ids)` immediately dereferences the supplied UFunction. That saved object is not safe once UObject-array shutdown has begun.
- The 0.6 diagnostic fixed that path by reacting to UObject-array shutdown, but it required high-volume UObject listeners. Version 0.7.3 deliberately does not restore those listeners. It resolves `RtlDllShutdownInProgress` once at startup, invalidates the instance/generation first at teardown, and abandons local hook records without touching UE4SS/UObject registries when either process shutdown is in progress or Unreal's exported initialization state is already false. Normal hot-uninstall still unregisters every callback. This removes the known unsafe teardown call without adding a polling path; normal process exit remains a required runtime test rather than a static claim.
- Schema-12 current-Controller/Pawn property receiver capture and `Server_RunInteractV2` replay still require gameplay acceptance.

## 0.4.2 live result and 0.4.3 probe

- After discovery completed, `Server_RunInteractV2` reached 14 calls but every call still reported `candidate_relation_missing`; no calibration call, contract, action, or tracked deletion was produced.
- All player pulses were attributed to the alternate/controller-bound Pawn mode during that mounted session.
- The repeated no-delete call is not accepted as the pickup entry. Version 0.4.3 logs its exact receiver/Outer relation once and records exact reflected parameter names/types/sizes for `CallActivePlayer`; it performs no additional action.

## 0.4.3 live signature and failed 0.5.0 action candidate

- Runtime reflection reported `CallActivePlayer` parameter bytes `2` with ordered fields `IsActive:BoolProperty:1` and `IsCutScene:BoolProperty:1`.
- `Server_RunInteractV2` remained unrelated and is no longer selected as the production action.
- Version 0.5.0 installed `CallActivePlayer(true,false)` from signature evidence alone. The later owner test showed that this was premature: no nearby candidate became eligible and `CallActivePlayer` was never invoked.
- The fixed adapter compiled with the pinned SDK and passed source/core gates, but that did not establish object identification or gameplay behavior.

## 0.5.0 failure diagnosis and 0.5.1 evidence plan

- In the latest 0.5.0 session, F9 entered `ArmedReady`, `active=true`, and the player chain succeeded 433/433 times with a 31 microsecond average.
- The activation sweep examined 305,382 objects but found only one base-class candidate. Its state was `InteractableValue=2`, `InteractTypeValue=7`; all 109 evaluations rejected it at roughly 1,235 to 1,212 meters.
- `gate_eligible=0`, `actions=0`, and `actions_invoked=0`. Therefore this session says nothing about whether `CallActivePlayer` performs pickup.
- The lone object's low index, base class, persistent default state, and world-origin-like distance strongly suggest a class default/template object. This remains an inference because 0.5.0 did not log flags, full name, Outer, or World.
- Manual-pickup evidence still points to `Server_RunInteractV2`, but the receiver relationship and exact removed object are unresolved. None of the inspected reference Mods supplies a proven pickup call chain.
- Version 0.5.1 is observation-only. It logs candidate template/world identity, the raw interaction call before correlation rejection, bounded receiver/controller/Pawn object relationships, and pre-delete watched identities. It disables built-in contracts, replay, contract persistence, and automatic mutation.

## Accepted 0.5.1 manual interaction trace

- The only discovered `/Script/DS.DropItemActor` was `/Script/DS.Default__DropItemActor` with object flags `0x00000031`, `cdo=true`, `archetype=true`, `/Script/DS` as its package Outer, no World, and location `(0,0,0)`. The old candidate architecture therefore observed a template rather than live ground loot.
- One manual ordinary pickup produced two observed `/Script/DS.DInteractableComponent:Server_RunInteractV2` pre-hook entries on the player's own `InteractionComponent`. The second entry exposed `receiver.ExecuteTargetComponent` pointing to `Vitality_Leave_01_C.InteractComponent`.
- The player's `InteractableComponent` property pointed to that receiver. This establishes the local receiver relationship used by 0.5.2.
- No watched UObject delete matched the pickup. Deletion is not a valid sole success rule for this item.
- The diagnostic player chain succeeded 236/236 times with a 23 microsecond average and 64 microsecond maximum in the decisive interval. The one-time sweep examined 289,992 objects and is removed from the 0.5.2 active path.

## 0.5.2 active canary boundary

- The game-selected `ExecuteTargetComponent` replaces object scanning as target discovery.
- Only non-template current-World targets whose Outer class name is exactly `Vitality_Leave_01_C` are eligible.
- `Server_RunInteractV2` is invoked once on the current on-foot player interaction receiver, then weak-identity debounced until the game changes or clears its selected target.
- Mounted receiver resolution and all other item classes remain unaccepted. Visible pickup and clean runtime behavior require owner testing.

## 0.5.2-0.5.4 rejection and 0.6.0 correction

- Repeated owner tests showed that F9 and the player chain worked, while the automatic action path still invoked nothing. The last measured failed session resolved the player chain 433/433 times but had `gate_eligible=0` and `actions_invoked=0`.
- The only discovered object in that session was a base `DropItemActor` about 1.2 km from the player. Later evidence identified the base object as template-like; it was not a valid nearby ordinary drop.
- `ExecuteTargetComponent` is transient manual-interaction context. Treating `Vitality_Leave_01_C` or that field as general persistent DropItemActor discovery did not produce a reliable automatic pickup implementation.
- The accepted 0.5.1 trace proves `Server_RunInteractV2` only for its observed Vitality interaction. It does not prove the action used by ordinary derived `DropItemActor` instances.
- Version 0.6.0 therefore removes every automatic action invocation, target-field write, replay/persistence route, and automatic-success claim. It performs one bounded read-only capture that must correlate the same real DropItemActor owner/component with the owner's normal manual pickup call sequence.
- Static verification and the pinned native build are complete. Runtime interaction evidence is still pending and no automatic pickup behavior is claimed.
