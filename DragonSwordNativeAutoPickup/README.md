# DragonSword Native Auto Pickup

`DragonSwordNativeAutoPickup` is an isolated native C++ UE4SS canary for ordinary ground loot in **DragonSword Awakening**. A marker-only Lua entry point schedules no work; C++ owns the natural game-thread pulse, lifecycle capture, validation, range checks, retry policy, and server interaction.

## Status

Version `0.3.2-relocated-runtime-canary` is labeled `OWNER_AUTHORIZED_RUNTIME_CANARY`. It targets the relocated `Win64/ue4ss/Mods` runtime and builds against its exact UE4SS 3.0.1 revision. Deployment is permitted for owner testing, but the build is not accepted until gameplay evidence confirms correct pickup and transition behavior.

F9 arms or disarms the canary. Active pickup additionally requires exact trusted game/UE4SS fingerprints plus a fresh accepted input-frame pulse after any world transition. Unknown fingerprints remain off.

## Runtime design

- C++ post-hooks only `/Script/Engine.PlayerController:ServerRecvClientInputFrame`, a naturally occurring game-thread UFunction. It is throttled with `steady_clock` to one accepted pulse per 150 ms.
- Every accepted pulse requires an exact `DsPlayerController`, a fresh exact `DsPlayerCharacter`, and bidirectional identity: `Controller.Pawn == Player` and `Player.Controller == Controller`.
- UObject lifecycle callbacks capture only exact `/Script/DS.DropItemActor` objects as `FWeakObjectPtr` values under a mutex. They never read gameplay properties or call `ProcessEvent`.
- On an accepted pulse, C++ resolves due weak candidates, requires exact owner/component ownership, `InteractableValue == 2`, `InteractTypeValue == 7`, radius, and invokes `Server_InputInteractKeyAction` with `KeyAction == 13`.
- Work is bounded to eight due candidates per pulse and at most one action per configured interval, with bounded retry/backoff.
- `InitGameStatePre` disables active mode and clears candidates. A fresh accepted input-frame pulse is required before reactivation.

There is no recurring Lua scheduler, `FindAllOf`, DropItemActor scan, global ProcessEvent hook, ActorTick/ReceiveTick hook, LoadMap hook, or retained Pawn/Controller wrapper. If the natural input-frame function is missing or does not fire in a scene, pickup stays inactive.

## Verification

```powershell
& .\tools\Verify-Source.ps1
& .\tools\Build-Native.ps1 <pinned toolchain parameters>
```

Do not deploy, launch the game, commit, push, or claim runtime acceptance without explicit owner authorization and gameplay evidence.
