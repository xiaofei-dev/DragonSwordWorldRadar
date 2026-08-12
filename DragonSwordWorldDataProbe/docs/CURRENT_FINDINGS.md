# Current Findings

## Assault closure status

- The build-pinned static catalog contains 40 exact Assault targets with Place, Kind, CID, UID, UIDName, coordinates, and actor joins.
- All 40 missions use `DEFEAT_MONSTER` and `MONSTER_ALIVE`; each condition names one unique target actor.
- All 40 target actors use `RespawnCycleID=105`, `ServerDeathCheck=true`, and direct `SpawnConditionID=0`.
- A controlled completion confirmed that `tb_actor_respawn` records the defeated Assault target, including its death time and respawn type. This is sufficient for the production death/respawn state path.
- RespawnCycle row 105 is statically confirmed as `DAILY`. Its exact reset boundary is not required to begin production integration because the existing save tracker already exposes the current persisted row state.
- `CUnexpectedMissionInStandAlone` is trigger-area state, not task completion or global availability, and must not be used by production.

## Special availability condition

- `RevealCycleData.xml` is now deterministically extracted and verified for the current game build.
- RevealCycle row 10001 is confirmed as a cemetery-skeleton schedule with reveal hour 23 and hide hour 6.
- CID 143 was absent at 22:51:13 and present at 23:01:46 while the sampled weather state did not change.
- The CID 143 to RevealCycle 10001 mapping is a strong inference based on identity and the observed reveal boundary. It is not a confirmed direct key binding.
- No evidence currently supports a separate weather rule for the 40 Assault targets. Weather must remain excluded from production availability logic.

## Production-ready interpretation

The production Radar can proceed with this fail-closed model:

1. Use the static 40-target catalog for identity and location.
2. Use `tb_actor_respawn` for defeated/respawn state.
3. Apply the 23:00-06:00 schedule only to CID 143, with its inferred provenance preserved in data or documentation.
4. Do not use Actor enumeration, lifecycle hooks, mission-trigger booleans, or weather labels.

One gameplay validation remains advisable after implementation: verify CID 143 is hidden before 23:00, shown after 23:00, and hidden after 06:00. This is production validation, not a reason to continue DataProbe collection.

## Frozen areas

- Boss research is complete and must not be expanded.
- Treasure runtime research is archived because continuous Lua Actor enumeration did not meet the performance target.
- Automatic `MonsterSpawnBase` enumeration is prohibited because the successful diagnostic was followed by a UE4SS-path native crash.
