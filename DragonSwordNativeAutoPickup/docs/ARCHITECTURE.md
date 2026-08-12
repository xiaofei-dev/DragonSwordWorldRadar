# Architecture

## Read-only control flow

```text
on_update / Windows F9 scalar poll
  -> one physical rising-edge request
  -> no UObject access

EngineTick post callback (150 ms throttle)
  -> consume F9 request
  -> start a 60-second diagnostic window
  -> bounded UObject-index sweep for non-template DropItemActor instances
  -> lifecycle capture for new DropItemActor instances while the window is active
  -> resolve current Engine -> LocalPlayer -> Controller -> Pawn -> Location
  -> require current World, InteractComponent ownership, state values, and radius
  -> lock only one unambiguous weak candidate identity
  -> observe correlated manual interaction calls
  -> never invoke pickup
```

## Correlation

The locked identity includes UObject index and serial for the drop owner and its `InteractComponent`. A hook record is retained only when at least one of these is true:

- receiver or receiver Outer equals the locked owner/component;
- an object parameter equals the locked owner/component;
- receiver `ExecuteTargetObject` or `ExecuteTargetComponent` equals the locked owner/component.

Each retained call records pre/post phase, receiver, parameters, selected object fields, locked component state, World relation, and bounded related object properties.

## Observed functions

- `DInteractableComponent`: `OnBeginOverlap`, `OnEndOverlap`, `CallActivePlayer`, `Server_RunInteractV2`, input-action helpers, UI helpers, and return-state helpers.
- `DsPlayerController`: local interaction press/release and related client/server state helpers.
- `DropItemActor`: `AniPickUp` and `SetDestroy`.

Every hook is observation-only. All replay-eligible flags are false.

## Lifecycle safety

F9 Off, timeout, world transition, and shutdown invalidate the active state and remove hooks/listeners from the safe EngineTick control path. UObject-array shutdown abandons saved UFunction pointers rather than dereferencing them.
