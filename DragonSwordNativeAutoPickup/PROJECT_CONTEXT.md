# DragonSword Native Auto Pickup Project Context

## Current milestone

Version `0.9.0-native-visibility-f-input` was compiled, statically verified, and deployed to the standard `Win64\ue4ss\Mods` location at `2026-08-13T16:20:39Z`, but it is not runtime accepted. The installed 0.8 build registered a reflected UFunction hook successfully, yet its decisive log contained zero visibility events and zero release attempts. Static xrefs then confirmed that the game calls the implementation directly in native C++, bypassing UE4SS `RegisterHook`.

## 0.9 design

The current Steam executable is pinned by SHA-256, image-relative function RVA `0x61B3AC0`, and the first 16 native bytes. PolyHook2 detours that native function and always calls its trampoline first. A new `Active=true` component edge queues one synthetic F scan-code press/release. The input is sent only from the next EngineTick while DragonSword owns the foreground window and the 250 ms input cooldown has elapsed.

## Active invariants

- F9 polling uses a 250 ms qualified-release edge, preventing the repeated On/Off transitions observed in 0.8.
- No reflection UFunction hook, UObject/Actor scan, Pawn query, spatial selection, target write, or direct interaction RPC exists in the active source.
- An exact game hash, RVA signature, and successful detour trampoline are all required before activation.
- The original native function runs before the Mod records the visibility edge.
- F is sent as one scan-code keydown/keyup pair only while the game is foreground.
- A hidden event and InitGameState clear the pending component; InitGameState also turns the Mod Off.
- Logging distinguishes detection, input request, input delivery, foreground rejection, cooldown, and failure.

## Acceptance boundary

Compilation and static validation do not prove that Windows input reaches the game's interaction binding. Owner testing must first show `native_visibility_events > 0`, then `F_INPUT_SENT`, then visible collection. If detection succeeds but pickup does not, the remaining failure is specifically the Windows input boundary rather than discovery.

Do not deploy, commit, or push without explicit authorization. Radar and DataProbe remain outside this change.
