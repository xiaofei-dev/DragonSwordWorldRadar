# 1.0.9

Mole:
- 12001 is the only canary.
- 12002-12040 are queried in one read-only Lua bulk step.
- The TSV is written once after the bulk pass.
- No shell/external process is used.

Assault:
- Legacy AssaultStaticProbe inner mutex removed.
- ModuleHost global mutex is the only collection serializer.
- Cache schema bumped, forcing one fresh extraction with the XOR16 Kind/Place decoder.
