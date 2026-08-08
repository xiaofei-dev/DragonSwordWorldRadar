# 1.0.33 Treasure static Blueprint route

The runtime blocker `TreasureBoxPropDataMap.Data -> nil` is bypassed rather than
forcing a raw TMap wrapper into the game process.

The PAK directory already proves exact static entries:

- `/PropTreasureBoxData.xml` — encoded offset 10540
- `/SectionTreasureBoxData.xml` — encoded offset 11964

Both are now extracted after game exit through the same deterministic decoder
that successfully recovered UnexpectedMission tables and RespawnCycleData.

The parser reads the real `BlueprintPath` values from PropTreasureBoxData and
derives exact GeneratedClass short names. Those names are then matched against
the full UE4SS ObjectDump for class functions/properties.

This is safer than runtime UScriptStruct memory access and should identify the
actual Treasure actor Blueprint family needed for a future native/opened-state
or 25m actor-presence implementation.

## Respawn 105

The RespawnCycle parser now selects direct XML child elements with XPath and
reads namespace-prefixed attributes by `LocalName`, matching the actual XML
structure observed in the latest diagnostic.
