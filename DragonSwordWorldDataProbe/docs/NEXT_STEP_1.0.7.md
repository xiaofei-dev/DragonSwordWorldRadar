# 1.0.7 next-step test

## Mole
ObjectDump confirmed:

`DETUtil.CIsClearMiniGameInStandAlone(WorldContextObject, ID) -> bool`

1.0.7 queries only IDs `12001..12040`.
It performs `12001` first as a canary. The remaining IDs are not called until that query returns a boolean in the same game process.

Output:

`runtime/reports/mole-completion-state.tsv`

Expected final semantics if validated:

- false -> unfinished -> show marker
- true -> completed -> hide marker

The probe does not call RefreshMiniGameState and does not modify game state.

## Assault
The latest evidence showed 16-byte blocked compact records. For Kind/Place/Respawn,
byte-wise XOR with `0x1B` produced structurally plausible one-block Oodle records.

1.0.7 does not hardcode `0x1B`: for this exact 16-byte layout it derives the mask
from byte #1, decodes the standard compact fields, then verifies the full PAK
DataEntry has identical compressed/uncompressed sizes and compression method
before decompression.

No adjacent entry is substituted and no 0..255 mask scan is used by this fallback.
