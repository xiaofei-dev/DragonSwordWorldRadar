# Acceptance Checklist

## Static and build acceptance for 0.6.0

- [x] Version is `0.6.0-dropitem-closed-loop-diagnostic`.
- [x] F9 and `on_update` perform no UObject work.
- [x] The diagnostic filters non-template `DropItemActor` instances and derived classes.
- [x] Discovery is budgeted and limited to one 60-second owner-triggered window.
- [x] Exactly one eligible current-World candidate is required before locking.
- [x] Hook output is filtered to the locked owner/component relation.
- [x] Both pre and post state are recorded.
- [x] Overlap, player-interaction, `AniPickUp`, and `SetDestroy` paths are observed.
- [x] No target property is written and no pickup UFunction is invoked.
- [x] No replay contract is loaded, persisted, validated, or quarantined.
- [x] Travel cancels the window and clears all weak identities.
- [x] Exact pinned `/W4 /WX` native build passes.
- [x] Source, core, and staged-package verification pass after metadata closure.

## Owner evidence capture

- [ ] `DIAGNOSTIC_ARMED` occurs after one physical F9 press.
- [ ] Discovery completes without an exception or runaway callback count.
- [ ] Exactly one nearby ordinary drop produces `DIAGNOSTIC_TARGET_LOCKED`.
- [ ] The owner manually collects that same item exactly once.
- [ ] Correlated pre/post `TRACE_INTERACTION_CALL` records identify the actual manual sequence.
- [ ] The result records the real state transition, whether or not UObject deletion occurs.
- [ ] Logs contain no action-invocation event; the 0.6 source has no automatic action path.
- [ ] F9 Off, timeout, normal exit, and world cancellation are crash-free.

No automatic pickup implementation is accepted until the exact same-object trace is reviewed.
