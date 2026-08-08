# 1.0.22

## Assault

1.0.21 proved both critical tables were already decoded correctly:

- `UnexpectedMissionKindData.xml`
  - direct DataEntry match: true
  - decompression attempted: true
  - XML identity: true
- `UnexpectedMissionPlaceData.xml`
  - direct DataEntry match: true
  - decompression attempted: true
  - XML identity: true

The only failure was control flow: after deterministic XOR16 success, the script
continued into the legacy uncompressed fallback, overwrote `$fallback`, then
reported the original standard-decoder exception.

1.0.22 treats a validated XOR16 result as final.

## Treasure

DETUtil discovery is now a negative result: there is no direct Treasure-specific
safe bool helper analogous to `CIsClearMiniGameInStandAlone`.

The next discovery stage is structural rather than name-only:

1. seed with TreasureBox-related structs/enums/properties;
2. resolve their ObjectDump addresses;
3. find properties/classes/structs referencing those addresses;
4. walk owners up to three hops;
5. enumerate every function owned by those discovered owners;
6. rank read/query-shaped functions;
7. never invoke them automatically.

This is intended to discover state APIs whose names do not contain `Treasure`.
