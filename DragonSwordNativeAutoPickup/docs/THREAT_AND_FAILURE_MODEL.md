# Threat and Failure Model

| Hazard | Mitigation |
|---|---|
| Shared Lua game-thread queue conflict | Marker-only Lua schedules no work; natural native UFunction supplies the pulse. |
| Wrong controller/player | Exact DS classes plus bidirectional `Controller.Pawn` / `Player.Controller` identity. |
| Natural pulse absent | Active mode remains false; no scan or Tick fallback. |
| Excess hook frequency | Native `steady_clock` throttle rejects callbacks until 150 ms elapses. |
| Wrong interactable | Exact DropItemActor owner, owned component, and values 2 and 7. |
| Stale actor after travel | FWeakObjectPtr resolution only on an accepted pulse; transition clears the queue. |
| Newly created actor not initialized | Missing component/state returns RetryLater, not permanent rejection. |
| Distant item | Squared world-unit distance is checked against configured meters. |
| Excess candidate work | Queue, candidates per pulse, retries, backoff, and action rate are bounded. |
| Unknown build | Exact fingerprint gate keeps active false. |
| Access violation | Class capture, pulse gates, property reads, location, and invocation fail closed under SEH. |
| Hook after unload | UFunction hook unregisters; callbacks reject stale generations. |

Runtime acceptance still requires input-pulse evidence, item/exclusion tests, Radar compatibility, travel, dungeon, menu, cutscene, long-session, crash, and frametime testing.
