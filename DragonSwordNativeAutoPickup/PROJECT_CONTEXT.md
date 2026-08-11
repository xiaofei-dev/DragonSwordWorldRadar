# DragonSword Native Auto Pickup Project Context

## Purpose

This is an independent native C++ auto-pickup canary with a marker-only Lua entry point. It does not share runtime state or game-thread scheduling with Radar or DataProbe.

## Current milestone

`0.3.2-relocated-runtime-canary` retains the queue-free native pulse and updates all runtime, fingerprint, package, and deployment paths for `Win64/ue4ss/Mods`. It builds against the exact relocated UE4SS revision; static and native build success do not prove runtime behavior.

## Non-negotiable rules

- No recurring Lua work, UObject scan, `FindAllOf`, global ProcessEvent hook, ActorTick/ReceiveTick hook, or LoadMap hook.
- Lifecycle callbacks may only perform exact class comparison and weak capture/delete bookkeeping.
- Require exact DropItemActor owner, component ownership, values 2 and 7, exact DS controller/player, bidirectional Pawn/Controller identity, distance, and KeyAction 13.
- Native input-frame processing is throttled to 150 ms; queue, due candidates, retries, backoff, and actions are bounded.
- Unknown fingerprints, missing pulse metadata, absent natural pulses, stale weak pointers, transitions, or access faults fail closed.
- F9 cannot bypass fingerprint or fresh accepted-pulse gates.
- Runtime label is `OWNER_AUTHORIZED_RUNTIME_CANARY`; acceptance remains false until owner testing.

## Handoff

Read `README.md`, `docs/ARCHITECTURE.md`, `docs/THREAT_AND_FAILURE_MODEL.md`, and `metadata/interaction-contract.json`. Run `tools/Verify-Source.ps1` before packaging or runtime work.
