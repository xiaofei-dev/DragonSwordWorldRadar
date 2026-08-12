# Evidence

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
- The current source fix invalidates callback generation, disables activity, clears only the Mod's local hook records, and removes UObject listeners using the UE4SS shutdown pattern. The destructor skips all UE4SS registry mutation after that shutdown signal.
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
