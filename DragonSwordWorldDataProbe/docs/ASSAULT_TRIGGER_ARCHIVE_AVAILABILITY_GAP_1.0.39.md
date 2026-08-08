# Assault Trigger Archive and Availability Gap 1.0.39

## Confirmed trigger semantics

`CUnexpectedMissionInStandAlone` represents trigger-area or active-context state. It is not completion state and it is not global marker availability. The runtime sampler is therefore archived and disabled.

## Exact static condition audit

All 40 build-pinned UnexpectedMission Kind rows use `AcceptConditionType=MONSTER_ALIVE`. All 40 exact target actors use `SpawnConditionID=0`, `RespawnCycleID=105`, and `ServerDeathCheck=true`.

No per-mission Kind, Place, or exact target-actor row contains a separate weather or time condition. Each `MONSTER_ALIVE` condition instead names the mission's unique target actor in `AcceptConditionValue3`. In this offline game, weather or time can gate the mission indirectly by controlling whether that target exists. `UnexpectedMissionWorldData` also contains one world row with `MapID=100`, `MissionCnt_Min=1`, `MissionCnt_Max=1`, and `DaySwitchID=1`.

This supports a local world-selection or rotation layer, but does not prove that DaySwitchID 1 means a specific weather, clock interval, or reset boundary.

## Current capability

The project can identify every Assault marker and its exact target actor. It can also identify the common DAILY respawn cycle and the world-level DaySwitch reference. `MONSTER_ALIVE` is a per-mission target condition, not one shared global boolean. The remaining unknown is whether its underlying target-existence lookup covers unloaded world-partition regions or only the currently loaded area.

## Remaining evidence

Closure requires a bounded comparison of one weather/time-bound target while its region is loaded and unloaded. The same target UID must be checked without using a global Actor scan. The exact DaySwitchID 1 definition remains useful secondary evidence, but weather/time does not need a separate Radar rule if the proven target-existence source already incorporates it. Until global coverage is proven, an unloaded target must remain unknown rather than unavailable.
