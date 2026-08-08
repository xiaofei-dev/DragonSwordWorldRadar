# 1.0.25

Treasure research is now intentionally narrow: find a native read-only
`save_id -> opened/unopened` accessor or state container.

Already complete: 1693 static records, coordinates/types/IDs, and the current
`tb_treasure_box` bitset ground truth.

The new focused parser ranks world TreasureBox Actor/Component and persistent
Save/Player/WorldState owners higher, while penalizing AdventureBook/UI owners.
It emits owners, functions, and state-like properties. Unknown candidate
functions are never invoked.

The assault correlation writer is also corrected so a legitimate Lua `false`
return is serialized as `false` rather than being lost through the
`a and false or fallback` idiom.
