# 1.0.18

The compact XOR16 metadata is now treated as established evidence, not guessed
again. The remaining uncertainty is where the corresponding full DataEntry header
begins and which field rejected the previous strict comparison.

For each Kind/Place record, 1.0.18 writes `xor16_debug/<table>.json` containing:
- raw 16 bytes;
- XOR mask and decoded bytes;
- compact PAK offset / compressed / uncompressed size;
- full DataEntry fields at the direct offset;
- exact-match result;
- exact nearby-header matches within +/-384 bytes;
- chosen header, decompression attempt, and XML identity result.

A nearby header is never accepted based on proximity alone: it must match the
compact compressed size, uncompressed size, compression index, and encryption flag,
and there must be exactly one match.
