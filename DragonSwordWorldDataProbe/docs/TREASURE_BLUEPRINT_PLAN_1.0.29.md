# 1.0.29 Treasure Blueprint actor discovery

The previous `DsAnimationProp` hypothesis is removed from the normal pipeline
because the 1.0.28 capture successfully enumerated hundreds of instances but
matched zero Treasure records and recorded zero Treasure interaction events.

The new chain follows game data directly:

```text
DPropDataTable
-> TreasureBoxPropDataMap
-> DTableData_TreasureBoxProp.Data
-> DPropTreasureBoxData
-> BlueprintPath
-> generated Blueprint class
-> loaded instance / ObjectDump class members
```

This avoids guessing the inheritance family of a Treasure actor.

The runtime probe records every TreasureBox prop definition and its BlueprintPath,
derives `<BlueprintBase>_C`, and asks `FindFirstOf` only for those exact class
names. It does not load assets and does not invoke candidate state functions.

After game exit, the exact Blueprint names are matched against the full
UE4SS ObjectDump. Functions and properties on those classes are ranked for the
next opened-state transition test.

## Assault note

The user reports one of the 40 assault missions was completed historically before
the 1.0.28 capture, yet all 40 returned false/false. Therefore
`CUnexpectedMissionInStandAlone` is not a simple persistent completion flag.
The low-frequency transition sampler remains enabled only to observe what changes
while the user performs a mission during the capture session.
