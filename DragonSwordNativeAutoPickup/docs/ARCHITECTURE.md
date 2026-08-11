# Architecture

## Event flow

```text
natural PlayerController:ServerRecvClientInputFrame post hook
  -> count raw callback
  -> exact DsPlayerController
  -> native steady-clock 150 ms throttle
  -> fresh Controller.Pawn + exact DsPlayerCharacter
  -> require Player.Controller == Controller
  -> guarded player location
  -> active = F9 armed + trusted fingerprint + fresh accepted pulse
  -> resolve up to 8 due DropItemActor weak candidates
  -> exact state/owner/radius gates
  -> at most one guarded KeyAction 13 call

UObject create/delete listeners
  -> exact DropItemActor class pointer comparison only
  -> add/remove FWeakObjectPtr under mutex

InitGameStatePre
  -> active=false, world_ready=false, clear weak candidates and pulse timers
```

## Lifetime and thread rules

The marker-only Lua file schedules nothing. The natural input-frame UFunction supplies the game-thread context. C++ retains static class/UFunction metadata and weak object identities; weak candidates are resolved only during an accepted pulse. Create/delete callbacks never inspect gameplay properties or invoke reflected functions.

Reflected property reads and ProcessEvent calls are enclosed by MSVC SEH fail-closed boundaries. Hook/global callbacks also use generation checks to reject stale mod instances.

## Performance and compatibility

The input-frame hook is class-specific and throttled to 150 ms before Pawn/property work. Diagnostics separate raw hook callbacks, accepted pulses, throttle rejects, and gate rejects. UE4SS `RegisterAActorTick` is deliberately excluded because it runs around every Actor tick, is documented as extremely performance-sensitive, and has no matching unregister API in the pinned SDK. No Tick fallback is permitted; missing natural pulses fail closed.
