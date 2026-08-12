# Assault Spawn Condition Diagnostics

## Objective

Bind known Assault target identities to their fixed `MonsterSpawnBase` actors and read the exact static-table references that control spawning. This route replaces moving-monster proximity inference as the primary diagnostic.

## Proven schema

The build-pinned object dump proves these `MonsterSpawnBase` properties:

- `TableKey_Monster`
- `TableKey_LevelSettings`
- `TableKey_SpawnCondition`
- `TableKey_RespawnCycle`
- `TableKey_RevealCycle`
- `RespawnCycleData`

All five table references use `/Script/DS.TableKey`. The structure contains one `IntProperty` named `Key`, so the runtime can read an exact integer ID without stringifying userdata or invoking a discovered game function.

`DRevealCycleInfo` contains `ID`, `Memo`, `RevealIngametime`, and `HideIngametime`. Resolving a nonzero reveal-cycle key to those row values remains a separate static-data step.

## Runtime method

`assault_spawn_condition_diagnostics.lua` performs one exact `FindAllOf("MonsterSpawnBase")` discovery pass. It calculates the nearest of three build-pinned target coordinates and binds a spawner only within 1,500 Unreal units. Bound spawners are cached and sampled through property reads. If any binding is missing or invalid, discovery retries no more than once per 30 seconds.

The three targets are:

- PlaceID 104 / CID 143: observed night-time candidate.
- PlaceID 109 / CID 148: second GroupID-zero comparison target.
- PlaceID 120 / CID 106: ordinary Quaku control.

The module emits:

- `runtime/reports/assault-spawn-condition-discovery.tsv`: all loaded `MonsterSpawnBase` discovery candidates and nearest known target.
- `runtime/reports/assault-spawn-condition-current.tsv`: current bound-target values.
- `runtime/reports/assault-spawn-condition-history.tsv`: baselines and value transitions only.

## Evidence interpretation

- **Confirmed:** a spawner was coordinate-bound and its `TableKey.Key` value was read.
- **Inferred:** a nonzero reveal-cycle key probably controls the observed time window.
- **Unknown:** the reveal/hide times until the key is resolved against `RevealCycleData` or a controlled transition confirms the semantics.

Zero is preserved as an observed key. An unreadable property is emitted as empty and is not coerced to zero.

## Safety boundary

The module performs no generic Actor or Character enumeration, no container enumeration, no hooks, no lifecycle callbacks, no unknown function calls, and no game-state mutation. `MonsterSpawnBase` enumeration is diagnostic and does not prove unloaded world-partition coverage.
