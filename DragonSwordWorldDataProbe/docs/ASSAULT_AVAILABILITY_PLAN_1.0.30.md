# Assault availability research — 1.0.30

Current proven facts:
- 40/40 static Place -> Kind -> exact actor chain is complete.
- CUnexpectedMissionInStandAlone is an in-zone/active-context signal, not marker availability.
- all 40 Kind rows: AcceptConditionType = MONSTER_ALIVE.
- all 40 exact actors: RespawnCycleID = 105, ServerDeathCheck = true, SpawnConditionID = 0.
- UnexpectedMissionWorldData references DaySwitchID = 1.

1.0.30 therefore collects three independent layers:

1. Quest Trigger / registration
   - DETTask_TagQuestStepLoopInStandAlone
   - DLayerEvent_GoalQuest
   - QuestID / MissionID / ISRegister / Active / State / Clear fields
   - Quest-system function signatures only

2. Respawn/cooldown
   - exact extraction of RespawnCycleData.xml
   - isolate cycle 105
   - no cooldown semantics are assumed until that row is observed

3. Environment
   - read-only Weather / Climate / Day / Time values from likely controller classes
   - ObjectDump evidence for Weather/Climate/DaySwitch
   - no weather-trigger claim until a task-specific correlation exists

Desired availability field pattern for a repeatable task:
- outside trigger area while task is available: true
- inside trigger area: true
- after completion/cooldown begins: false
