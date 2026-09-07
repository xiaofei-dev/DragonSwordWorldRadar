# Safety audit: 1.2 to 1.3 dispatch recovery

## Status and evidence limit

Status: `DISPATCH_OBSERVER_CANDIDATE_RUNTIME_PENDING`.

This audit compares the retained 1.2 runtime evidence, surviving source lineage,
1.3 source and tests, design documents, and local runtime logs. There is no
retained exact 1.2 source commit that can be restored and compared line by line.
Accordingly, this document does not claim a complete textual diff or authorize
a wholesale rollback to 1.2 behavior. Every decision below is tied to a
specific retained contract or observed failure mode.

Historical runtime acceptance and preceding 1.3 artifact hashes do not accept
the repair described here. The final repaired DLL is now built, hash-recorded,
and locally deployed, but it must still be exercised before gameplay acceptance
can change from `RUNTIME_PENDING`.

## Findings

The Debug-on failure was narrow: a diagnostic selected-distance read crossed a
gameplay boundary after Enhanced Input injection. Removing that post-injection
UObject/game call restored action effects. This proves that detailed logging
must remain observational and scalar-only. It does not prove that all 1.3 safety
changes were unnecessary.

The later non-Debug failure had a separate scheduling cause:

1. one pending record globally blocked every scan;
2. weak-identity/state confirmation was not a reliable prompt signal for every
   supported interaction type;
3. a second timeout quarantined that exact Component for the complete F9
   activation; and
4. when the single-result selector returned that Component again, the scheduler
   entered repeated 500 ms deferred scans instead of promptly considering later
   work.

The 1.2 evidence rejects a broad rollback. Its observational supersede and
250 ms same-target limiter admitted action storms: one bounded session recorded
250 injections and one identity received 61. Its time-only F9 debounce also did
not prove one transition per physical press.

## Retained 1.3 safety boundaries

- Runtime selector resolution remains dual-path, instruction-bounded,
  consensus-required, and fail-closed. There is no fixed RVA, game-hash address
  table, or historical-address fallback.
- The exact supported ExperimentalNested UE4SS hash and loaded path are checked
  before gameplay reflection.
- Fresh World, session generation, foreground window, LocalPlayer, controller,
  current Pawn, mounted Rider, interaction owner, receiver, target relation, and
  target-type checks remain in force.
- Interaction-owner change clears action records and retains the 1500 ms settle
  boundary.
- F9 remains a true physical press edge; key repeat cannot produce another
  transition before release.
- Reflected action metadata remains an exact bounded allowlist, including the
  explicitly supported object-pointer storage form. Unknown storage fails
  closed.
- The action record is armed before injection. Re-entrant or overlapping
  automatic injections remain forbidden.
- Diagnostics remain asynchronous, bounded, scalar-only on sensitive paths,
  and disabled by default. Debug mode must not change target selection,
  scheduling, input, confirmation, or recovery behavior.
- Treasure remains excluded. No UObject/Actor scan, collision mutation, direct
  pickup RPC, Windows synthetic input, recurring worker, or broad
  `ProcessEvent` observer is introduced.

## Replaced or withdrawn behavior

| Previous behavior | Disposition | Reason |
|---|---|---|
| Exact weak-identity/component-state change as the only early terminal signal | Replaced as the primary release signal | Absence of that signal does not prove that Enhanced Input failed to reach the game's interaction dispatch for every supported type. |
| 650 ms fallback window | Withdrawn | Restore the conservative 750 ms fallback while the successful path is released by direct dispatch evidence. |
| 100 ms first-timeout retry delay | Withdrawn | Restore the conservative 200 ms retry delay; fast success no longer depends on shortening failure timing. |
| Second-timeout quarantine for the full F9 activation | Removed | It can permanently exclude a live Component and starve later selector results. |
| 500 ms quarantine-driven scan deferral | Removed | It amplifies single-result selector head-of-line blocking. |
| Debug selected-distance UObject/game call after injection | Removed and prohibited | Diagnostics must not perturb the injected input path. |

## Dispatch observer contract

The adapter registers one post hook for the exact reflected
`/Script/DS.DInteractableComponent:Server_RunInteractV2` UFunction. It does not
register a global `ProcessEvent` callback.

The record is armed before injection and remains correlatable after the
injection function returns until matching dispatch, existing exact weak/state
confirmation, timeout, or context reset. The callback is relevant only while
that record is armed. Its entire gameplay-sensitive responsibility is:

1. read the armed scalar action token and raw receiver address;
2. compare the raw callback receiver address with that armed address; and
3. publish the matching action token through an atomic marker.

The callback must not log, format text, perform reflection, dereference a
UObject, call a game function, resolve weak identities, change scheduler/action
state, or allocate recovery records. Shutdown unregisters only this Mod's hook
IDs from this exact UFunction; it must never unregister hooks owned by another
Mod.

EngineTick is the sole consumer. A marker matching the current armed action
releases the global in-flight slot and records
`PICKUP_DISPATCH_OBSERVED ... target_match_unproven=1 pickup_success_claim=0`.
The marker proves only that the injected interaction action reached
`Server_RunInteractV2` on the armed receiver. The game can reselect internally,
so it is not evidence that the selector-returned Component was picked up or
that inventory changed.

Existing exact Actor/Component invalidation or exact Component state-change
evidence remains an alternate terminal path inside the 750 ms fallback window.
The marker is consumed on the next actual EngineTick pulse, which normally
arrives roughly 31-35 ms after injection in the reviewed runtime. The existing
25 ms active/post due is therefore already satisfied. Consumption must not add
another 25 ms delay; the same EngineTick may continue scanning after it safely
clears the in-flight record.

## Timing and recovery contract

| Parameter | Current source contract |
|---|---:|
| Engine pulse | 25 ms |
| Active scan | 25 ms |
| Idle scan | 33 ms |
| Post-invocation due | 25 ms, not re-applied after marker consumption |
| No-dispatch fallback window | 750 ms |
| First no-dispatch retry delay | 200 ms |
| Maximum no-dispatch attempts | 2 |
| Same-Component delay after matching dispatch | 750 ms |
| Backoff after second no-dispatch result | 1500 ms, self-expiring |
| Activation-long timeout quarantine | disabled |

Different selector-presented candidates may proceed as soon as EngineTick
consumes a matching dispatch marker. The same Component remains temporarily
ineligible for 750 ms, limiting immediate duplicate injection without restoring
the rejected 1.2 250 ms storm policy. If no marker arrives, one retry is allowed
after 200 ms; after its second no-dispatch result, the exact Component becomes
eligible again automatically after 1500 ms. F9 Off/On is not required for
recovery. The 200 ms retry delay and 1500 ms recovery delay exist only in that
Component's record; the scheduler does not convert either into a global scan
pause, so a different selector result may proceed on the timeout EngineTick.

## Required acceptance evidence

Source/static checks must prove the exact hook registration/unregistration,
callback restrictions, pre-injection arming, token correlation, bounded retry,
expiring record cleanup, no global hook removal, and absence of activation-long
quarantine or the quarantine-specific 500 ms loop.

Runtime acceptance must use the exact repaired DLL and record at least:

- installed DLL hash and build provenance;
- one `PICKUP_ACTION_INVOKED` correlated to one
  `PICKUP_DISPATCH_OBSERVED` by action/request/tick fields;
- explicit `target_match_unproven=1 pickup_success_claim=0`;
- a different target advancing after dispatch while the first Component remains
  in its 750 ms re-entry delay;
- one controlled no-dispatch first timeout, exactly one retry after 200 ms, and
  a second-result 1500 ms self-expiring backoff;
- automatic recovery without an F9 Off/On reset;
- manual F availability, normal gather, fish/drop, mounted Rider, treasure
  exclusion, World travel, clean exit, and bounded performance; and
- equivalent behavior with `debug_logging=false` and `debug_logging=true`, with
  no Debug-only UObject/game call after injection.

Until those observations exist, the correct status is `RUNTIME_PENDING`, not
gameplay accepted.
