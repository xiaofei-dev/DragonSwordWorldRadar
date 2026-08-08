# 1.0.31 safe runtime rollback

1.0.30 introduced two new in-game runtime probes:

- `assault_quest_trigger_correlation`
- `assault_environment_snapshot`

The user reported a game crash before a diagnostic proving which probe was
responsible. Both are therefore removed from the active in-game schedule.

The code is preserved but not loaded by `probe_order`.

The active runtime returns to the last known-stable pair:

- `treasure_blueprint_catalog`
- `assault_condition_transition`

The following research remains enabled only after the game exits:

- exact RespawnCycleData.xml extraction for cycle 105;
- Quest/Trigger/Register/Weather/DaySwitch ObjectDump analysis;
- Treasure Blueprint class analysis;
- transition summary.

This isolates runtime crash risk from static research while preserving all data.
