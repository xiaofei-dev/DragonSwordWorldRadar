# 1.0.13 assault complete-capture design

## Runtime priority
The runtime scheduler now loads only `assault_runtime_table`. Mole/Boss research
is frozen, so no unrelated probe delays Place/Kind collection.

## UE4SS reflection correction
The table reader uses:
- `UObject:Reflection():GetProperty(name)` for property metadata;
- `StructProperty:GetStruct()` + `UStruct:ForEachProperty()` for nested schema;
- `UObject:GetPropertyValue(name)` for UObject values;
- direct `UScriptStruct.Member` access for generated structs;
- `TMap:ForEach` with wrapped key/value `get()` unwrapping.

## Support-table strategy
A previous successful PAK extraction produced ten support tables on build:

`85d072028086faac03bd39f783126254e624405cdb2784da04395d829c17a25f`

The exact same build fingerprint is still present in the latest diagnostics.
Those support XMLs are bundled as reference data and are accepted only when
`static_inventory` reports that exact fingerprint.

This avoids another PAK scan while retaining:
- UnexpectedMissionWorldData
- SectionMonsterData
- SpawnMonsterGroupData
- ActorSpawnConditionData
- NPCSpawnConditionData
- MonsterCharacterData
- ActorPositionData
- link tables

Runtime Place/Kind remains authoritative for the two compact tables that were
not extractable.

## Dynamic correlation
`CUnexpectedMissionInStandAlone` is queried in bounded batches of at most 32 IDs.
If a `DLayerUnexpectedMission` instance exists, target XYZ/radius/timer is also
captured for semantic correlation.

## Intended final Radar chain
Place -> Kind GroupID -> MissionValue* -> exact SectionMonster/position ->
accept/spawn conditions -> existing `tb_actor_respawn` availability.
