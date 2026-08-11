# Evidence

## Confirmed static evidence

- `/Script/Engine.PlayerController:ServerRecvClientInputFrame` exists with an input-frame integer and byte-array data.
- `/Script/DS.DropItemActor` owns a `DInteractableComponent`.
- `DInteractableComponent` exposes `InteractableValue`, `InteractTypeValue`, and `Server_InputInteractKeyAction(TargetActor, InActor, KeyAction)`.
- `EInteractTypeValue::DropItemActor == 7`; TreasureBox is 4; object-dump structure identifies `EActionKeyType::DROPITEM == 13`.
- Required DS classes are `/Script/DS.DsPlayerCharacter` and `/Script/DS.DsPlayerController`.
- Pinned UE4SS documents `RegisterAActorTick` as extremely performance-sensitive, applies it around every Actor tick, and exposes no unregister API; this design excludes it.

## Crash evidence and rationale

During the confirmed F9-then-Radar-F7 crash, AutoPickup had zero candidates/actions. Radar's second activation logged the queue request but its callback never entered before UE4SS raised C++ exception `0xe06d7363`. The recurring AutoPickup Lua game-thread scheduler was therefore removed rather than tuned. The replacement uses no shared recurring Lua queue.

## Build evidence

The relocated-runtime adapter compiles against RE-UE4SS `1c1a1497f942c707f47ba668db75b25e86f6c08a`, UEPseudo `b2e876da82b17254c04304746341c8fde0ddb37c`, and the pinned MSVC/Rust toolchain used by the installed `Win64/ue4ss` runtime. This confirms API compilation; it does not prove the natural function fires in every gameplay scene or that the crash is resolved in game.

## Unverified runtime evidence

- Input-frame callback and accepted-pulse rates in each scene.
- F9/F7 cross-mod compatibility and crash resolution.
- Correct eligible-only pickup, transition recovery, performance, and long-session behavior.

No runtime acceptance claim is made before owner gameplay validation.
