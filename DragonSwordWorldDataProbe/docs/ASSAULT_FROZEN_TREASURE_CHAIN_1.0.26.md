# 1.0.26

## Assault

Assault static research is frozen.

The current build has a complete 40/40 chain:

Place -> Kind -> CID + UIDName -> exact SectionMonster -> UID / XYZ / RespawnCycleID.

`CUnexpectedMissionInStandAlone` is also proven to accept all 40 IDs and return
booleans. Available samples were 40/40 false/false; a positive active-event
sample was never observed, so the exact true-state semantics remain unlabelled.

The code and data are preserved, but assault is removed from the normal pipeline.

## Treasure

Research is narrowed to:

TreasureBox Actor
-> DInteractableComponent / Prop state
-> persistent opened state
-> treasure save_id.

Runtime inventory is limited to explicit focused classes and uses no global Actor
scan, hooks, or unknown function calls.

Post-exit ObjectDump analysis ranks only this Actor/interaction/persistence chain
and penalizes UI/AdventureBook/WorldMap owners.

The existing tb_treasure_box bitset remains the authoritative validation source
and fallback until a native accessor is proven.
