# 1.0.15 assault dual-path capture

Primary:
`DUnexpectedMissionTable` runtime Place/Kind snapshot.

New diagnostics:
- `assault-runtime-access.tsv`: top-level/nested struct access types and addresses.
- `assault-runtime-functions.tsv`: DUnexpectedMissionTable function inventory.

Fallback:
`assault_kind_place_fast` reads only:
- UnexpectedMissionPlaceData
- UnexpectedMissionKindData

It runs only if runtime snapshots do not already contain rows.
It is pinned to build `85d072028086faac03bd39f783126254e624405cdb2784da04395d829c17a25f` and its child process is killed at 60 seconds.

The historical `pak_static` module is not modified and remains manual-only.

Catalog:
same-build bundled support tables + runtime or fast Kind/Place -> actor/position/condition correlation.
