# Assault implementation research — 1.0.10

## Strongest static source

ObjectDump proves the live game owns:

- `DUnexpectedMissionTable`
- `UnexpectedMissionPlaceDataMap`
- `UnexpectedMissionKindDataMap`
- a live instance under `GameDBTableManager_Singleton.DUnexpectedMissionTable`

The nested table structure is also explicit:

- Place table: outer map keyed by `MapID`; inner map contains `DUnexpectedMissionPlaceData`
- Kind table: outer map keyed by `GroupID`; inner map contains `DUnexpectedMissionKindData`

1.0.10 therefore snapshots these maps directly, once per game session, rather
than making Kind/Place availability depend exclusively on compact PAK decoding.

## Place -> Kind relationship

`DUnexpectedMissionKindData` is wrapped by `GroupID`.
Each Kind row has `MissionKindWeight`.
Place has a single `MissionKindData`.

The structural model is therefore:

`Place.MissionKindData -> Kind.GroupID -> one or more weighted Kind rows`

The previous direct `MissionKindData -> Kind.ID` join is retained only as a
fallback diagnostic candidate.

## Dynamic condition function

ObjectDump also proves:

`DETUtil.CUnexpectedMissionInStandAlone(WorldContextObject, MissionID, ISAll) -> bool`

1.0.10 records both `ISAll=false` and `ISAll=true` for the real Place IDs.
The booleans are deliberately stored as `raw_uninterpreted`; they are not yet
treated as active/completed until correlated against one real in-game assault.

## Final intended Radar chain

1. Runtime/static Place + Kind catalog.
2. Place selects Kind group.
3. Kind `MissionType/MissionValue*` resolves objective actor/group.
4. `AcceptConditionType/Value*` resolves weather/time/condition rules.
5. Exact actor identity resolves to SectionMonster/position.
6. Existing `tb_actor_respawn` tracker determines alive/cooldown once the actor
   is known.
7. `CUnexpectedMissionInStandAlone` may become the activation gate after its
   raw booleans are correlated with a known active/inactive mission.

No task UI is required for the intended final implementation.
