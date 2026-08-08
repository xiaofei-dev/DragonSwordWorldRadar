# 1.0.24

1.0.23 completed the static assault catalog. A stronger relation is now proven:

- Place.MissionKindData == Kind.GroupID
- Kind.MissionValue1 == SectionMonsterData.CID
- Kind.AcceptConditionValue3 == SectionMonsterData.UIDName

The combined CID + UIDName predicate yields exactly one actor for every one of
the 40 missions. This gives exact UID, XYZ, CID and RespawnCycleID.

All 40 current actors also have RespawnCycleID=105, ServerDeathCheck=true and
SpawnConditionID=0.

1.0.24 therefore stops treating the objective actor as unresolved and begins
dynamic correlation. It calls the already-known read-only
CUnexpectedMissionInStandAlone once for all 40 Place IDs with both ISAll=false
and ISAll=true. No semantic label is assigned until the returned pattern is
correlated with a known active mission.
