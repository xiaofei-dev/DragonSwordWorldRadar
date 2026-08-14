# Architecture

## Active control flow

```text
startup
  -> verify exact game and UE4SS hashes
  -> resolve image base + RVA 0x61B3AC0
  -> verify the first 16 native bytes
  -> install PolyHook2 x64 detour and require a trampoline

native UpdateButtonVisibilityByComponent(widget, component, active)
  -> call the original game function first
  -> active=true with a new component edge: queue one scalar input request
  -> active=false: clear the component and pending request

UE4SS program-loop update
  -> flush bounded logs
  -> poll only F9 and apply a 250 ms qualified-release edge
  -> queue only a scalar toggle

native EngineTick post callback
  -> apply the queued F9 toggle
  -> require On, a pending component edge, cooldown elapsed, and game foreground
  -> send one F scan-code keydown/keyup pair through Win32 SendInput

InitGameState pre callback
  -> turn Off
  -> clear the component edge and pending input
```

## Lifecycle and performance

The active path owns no object scan, Actor registry, player chain, spatial search, Lua scheduler, worker thread, or bridge. The dormant cost is one native function detour plus atomic checks. Input work occurs only on a new game-provided visibility edge.

Normal hot-uninstall removes the detour before unregistering global callbacks. The module remains pinned for process lifetime because late UE4SS teardown may no longer expose a safe Unreal registry. In that unsafe process-exit branch the instance and callback generation become inert while the still-mapped detour continues to call the original trampoline.

## Deliberate exclusions

The active build does not use UE4SS reflected UFunction hooks, `ProcessEvent`, UObject/Actor scans, `DropItemActor` discovery, Pawn target fields, direct interaction RPCs, KeyAction guesses, or periodic candidate polling.
