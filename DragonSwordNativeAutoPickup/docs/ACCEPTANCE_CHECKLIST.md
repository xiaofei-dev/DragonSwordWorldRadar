# Acceptance Checklist

## Static and build

- [x] Marker-only Lua with zero recurring scheduling or game-thread queue calls.
- [x] Exact `ServerRecvClientInputFrame` post hook with native 150 ms throttle.
- [x] No DropItemActor scan, `FindAllOf`, global ProcessEvent, ActorTick/ReceiveTick, or LoadMap hook.
- [x] Exact Controller/Pawn classes and bidirectional identity are required.
- [x] Lifecycle callbacks perform exact class weak capture/delete bookkeeping only.
- [x] Candidate count, per-pulse work, retries, backoff, and action rate are bounded.
- [x] Reflected access and invocation have fail-closed SEH boundaries.
- [x] Diagnostics separate raw hooks, accepted pulses, throttle rejects, and gate rejects.
- [x] Native adapter compiles against pinned UE4SS 3.0.1.
- [ ] Final source verifier and package-layout checks pass after documentation closure.

## Owner runtime canary

- [ ] Trusted fingerprint logs `READY`; unknown fingerprint remains inactive.
- [ ] With Radar enabled, F9 then F7 no longer blocks Radar's game-thread activation callback or crashes.
- [ ] `raw_hook_callbacks` and `accepted_pulses` increase in normal open-world play.
- [ ] Missing-pulse scenes stay inactive without scans or Tick fallback.
- [ ] Nearby ordinary ground loot is picked up once; excluded and distant objects are untouched.
- [ ] World travel clears old candidates and requires a fresh accepted pulse.
- [ ] Menus, cutscenes, dungeons, travel, and long sessions do not crash.
- [ ] Enabled/disabled frametime remains within the agreed budget.

Unchecked runtime items are not accepted by a clean build or package.
