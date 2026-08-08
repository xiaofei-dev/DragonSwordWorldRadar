# Legacy snapshots

This directory intentionally preserves historical implementations that were
previously replaced or removed.

Files end in `.disabled.txt` so neither Lua nor PowerShell module discovery can
execute them accidentally.

Current production/research modules remain under `scripts/modules` and
`tools/modules`. Historical files here are reference material only.

Notable preserved snapshots:
- Mole 1.0.7: completion query implementation that caused visible shell-window spam.
- Mole 1.0.8: safe sequential completion query.
- Mole 1.0.9: canary + bulk completion query.
- Assault 1.0.10 / 1.0.12 runtime table experiments.
- PAK 1.0.9 XOR16 extractor implementation.
