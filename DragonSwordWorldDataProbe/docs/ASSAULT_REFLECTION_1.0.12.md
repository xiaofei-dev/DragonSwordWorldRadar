# 1.0.12 runtime table fix

The 1.0.11 diagnostic proved:
- the live `DUnexpectedMissionTable` singleton exists;
- both top-level properties exist;
- the failure was specifically the assumption that the outer generated struct
  exposes a member named `Data`.

1.0.12 queries the actual `StructProperty` metadata, calls `GetStruct()`, enumerates
its members with `ForEachProperty`, and selects the member whose runtime value is
an iterable `TMap`.

It then supports either:
- outer map directly containing Place/Kind records; or
- outer map containing the known Wrap structs whose inner map is `Data`.

`assault-runtime-schema.tsv` records what the game actually exposes.

PAK extraction is not required for this path.
