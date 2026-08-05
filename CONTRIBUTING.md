# Contributing

- Keep each radar feature as an independent layer and data provider.
- Do not commit locally generated game data under `data/generated`.
- Do not add custom executables to the release.
- Preserve user files during upgrades: `scripts/config.lua` and `data/treasure_overrides.txt`.
- Run `build/Verify-Source.ps1` and `build/Build-Release.ps1` before publishing.
