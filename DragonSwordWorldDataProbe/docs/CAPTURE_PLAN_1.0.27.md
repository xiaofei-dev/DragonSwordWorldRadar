# 1.0.27 capture plan

## Treasure

The candidate `ReturnContentsPropState` has no observed parameters or return
property, so this build does **not** call it.

Instead, it combines:

1. a targeted `FindAllOf("DsAnimationProp")` snapshot;
2. static matching against the 1693 treasure catalog;
3. `InteractTypeValue == TreasureBox/4`;
4. exact observation hooks on naturally occurring Prop/interaction functions;
5. state-delta and disappearance tracking.

Captured actor fields include:

- ObjectID
- DsGuid
- bShouldSaveAndLoadState
- IsDisposableProp
- AnimInfoTableID / SpawnWorldTime
- XYZ
- InteractTypeValue / InteractableValue / IsShowUI / PropIconID
- matched treasure save_id and UIDName

The strongest evidence of an opened-state path is:

- an exact/unique SaveID match;
- a Treasure interaction/state hook;
- a property change or actor disappearance immediately afterward;
- agreement with the existing `tb_treasure_box` bit truth.

## Assault

The static 40-target catalog remains complete.

For this test build, the known read-only function is sampled repeatedly:

`CUnexpectedMissionInStandAlone(PlaceID, false/true)`

The first matrix is the baseline. Later history contains only changed PlaceIDs.
Run one task after the baseline has been established, then wait after completion.

## Test sequence

1. Enter the world and wait 20 seconds.
2. Stand near an unopened ordinary treasure for 15 seconds.
3. Open it and wait 20 seconds.
4. Start/perform one assault task.
5. Wait at least 20 seconds after completion.
6. Exit the game, wait for post-exit collection, then run diagnostics.
