# DragonSword Native Auto Pickup Project Context

## Current milestone

Version `0.6.0-dropitem-closed-loop-diagnostic` is a deployed read-only evidence build. It removes the failed 0.5.2-0.5.4 `Vitality_Leave_01_C` target assumptions and restores the only target-discovery route that previously captured real derived `DropItemActor` instances. Deployment is hash-verified; runtime evidence remains pending.

## Confirmed evidence

- 0.3.8 captured three derived `DropItemActor` instances and one candidate passed every state and distance gate at 2.441 m.
- `Server_InputInteractKeyAction(13, candidate, Pawn)` did not collect that item and is rejected.
- The 0.5.1 manual trace observed two `Server_RunInteractV2` calls for a `Vitality_Leave_01_C` target, but it did not prove that ordinary `DropItemActor` loot uses the same contract.
- `ExecuteTargetComponent` is transient manual-interaction context and cannot be polled as persistent target discovery.
- UObject deletion is not a universal pickup-success condition.

## 0.6.0 invariants

- F9 starts one 60-second read-only diagnostic window. No automatic pickup action is present.
- F9 and `on_update` remain scalar-only. UObject access is EngineTick-owned.
- One budgeted UObject-index sweep plus armed lifecycle capture finds non-template `DropItemActor` instances and derived classes.
- A target is locked only when exactly one eligible ordinary drop is inside the configured radius in the current player World.
- Hooks observe overlap, player interaction, `AniPickUp`, and `SetDestroy` calls. Pre/post records are retained only when receiver, parameters, or target fields correlate to the locked weak identity.
- The diagnostic never writes receiver target fields, persists a replay contract, calls an interaction UFunction, or treats deletion alone as success.
- World transition cancels the window, clears all weak identities, and schedules hook/listener cleanup.

## Acceptance boundary

Static checks and compilation prove only that the diagnostic is bounded and mutation-free. An owner test must lock one real ordinary drop, manually collect that same item, and preserve both logs. Only that exact trace may define a later automatic interaction contract.

Do not deploy, commit, or push without explicit authorization. Radar and DataProbe remain outside this change.
