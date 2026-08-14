# Threat and Failure Model

| Hazard | Mitigation |
|---|---|
| A game update moves or changes the native function | Exact executable hash, RVA, and 16-byte prefix must all match; otherwise the Mod remains passive. |
| The detour suppresses normal game behavior | The original trampoline is called first with the untouched native arguments. |
| F9 produces repeated On/Off transitions | A physical press requires 250 ms of qualified release before rearming. |
| Synthetic F reaches another application | Input is allowed only when the foreground window belongs to the current game process. |
| A key remains held | One `SendInput` call contains both scan-code keydown and keyup records. |
| One component creates a tight input loop | Only a new active edge queues input; cooldown is 250 ms and pending is consumed once. |
| Cooldown drops a legitimate new edge | Pending remains queued until the cooldown expires. |
| Stale state survives World travel | InitGameState forces Off and clears component/pending state. |
| Discovery causes frame loss | There is no object/Actor scan, player query, radius calculation, or periodic candidate search. |
| Windows input is ignored by the game | Logs separate native detection from `F_INPUT_SENT`; visible pickup remains the acceptance test. |
| Late host teardown leaves a patched native function | The DLL is process-lifetime pinned; unsafe shutdown invalidates Mod state while the mapped detour continues calling its trampoline. |
| Build success is called gameplay success | Detection, input delivery, collection, mounted play, travel, and exit remain owner runtime gates. |
