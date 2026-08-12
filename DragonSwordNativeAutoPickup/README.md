# DragonSword Native Auto Pickup

`DragonSwordNativeAutoPickup` is an isolated native UE4SS research Mod for owner-authorized ordinary ground-loot pickup. Version `0.6.0-dropitem-closed-loop-diagnostic` does not perform automatic pickup. It captures the missing exact manual interaction contract for one real `DropItemActor`.

## Why this diagnostic exists

Earlier builds proved target discovery and action selection separately but never for the same ordinary drop:

- 0.3.8 found real derived `DropItemActor` candidates, but its KeyAction call did not pick them up.
- 0.5.1 traced a manual `Vitality_Leave_01_C` interaction, not a proven ordinary `DropItemActor` interaction.
- 0.5.2-0.5.4 incorrectly treated transient target fields or the Vitality instance name as persistent general discovery and found no usable target.

The 0.6.0 build closes that evidence gap without guessing another action.

## Current behavior

- F9 starts or stops one bounded 60-second read-only window.
- A budgeted sweep and lifecycle filter capture real non-template `DropItemActor` instances.
- The current player chain and candidate locations are resolved on EngineTick.
- Exactly one eligible candidate inside 4.5 m is locked by UObject index and serial.
- Correlated pre/post calls are logged for overlap, player interaction, `AniPickUp`, and `SetDestroy` paths.
- World transition cancels the diagnostic and clears all weak references.
- There is no automatic interaction call, target-field mutation, runtime contract file, or deletion-based success claim.

## Owner capture

1. Start on foot in stable open-world play.
2. Leave exactly one ordinary ground drop within 4.5 m.
3. Press and release F9 once.
4. Wait for `DIAGNOSTIC_TARGET_LOCKED`.
5. Manually collect that same item once through the normal game interaction.
6. Wait several seconds or until the 60-second window ends.
7. Exit normally and preserve both runtime logs.

Do not test travel, dungeons, or mounted pickup in this capture. Compilation and packaging do not establish a pickup implementation.
