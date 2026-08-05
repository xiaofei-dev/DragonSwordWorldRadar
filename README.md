# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar framework for **DragonSword Awakening**. Version 0.3.2c closes out the first complete layer: treasure detection on the minimap and world map.

## Current treasure layer

- Minimap and world-map markers.
- 16 ms world-map transform sampling.
- XYZ nearest-treasure selection.
- Type labels and colors, including blue pressure/statue puzzles.
- Encrypted save filtering, duplicate-ID handling, aliases, and editable coordinate exclusions.
- One hidden PowerShell process; no custom executable is shipped.

## Installation

1. Install UE4SS.
2. Place the release folder at `Mods/DragonSwordWorldRadar`.
3. Close the game and run `Install.cmd` once.
4. Start the game normally. Press **F7** to enable and **F8** to disable.

The installer generates `data/generated/treasures.lua` directly from the user's local game PAK. The former DragonSwordTreasureMap mod is not required.

## Game updates

Installation stores a fingerprint of the game executable and the generated-data PAK in `metadata/install-state.json`. On a later game launch, the hidden watcher compares the current game files with that fingerprint. If they changed, DragonSwordWorldRadar displays a reinstall prompt and does not start with stale generated data. Run `Install.cmd` again to regenerate all registered datasets.

## Repository layout

- `src/ue4ss` — game-thread sampling and bridge logic.
- `src/overlay` — WinForms renderer and save-state filtering.
- `src/installer/Core` — reusable local game-data extraction pipeline.
- `src/installer/Install.ps1` — one-click installer and updater.
- `src/host` — hidden watcher and in-memory overlay host.
- `src/tools` — diagnostics collector.
- `resources/defaults` — user-editable default configuration data.
- `vendor/sqlcipher` — SQLCipher runtime used for local save filtering.
- `build` — deterministic release packaging and verification.

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and [`docs/DATA_PROVIDERS.md`](docs/DATA_PROVIDERS.md).

## Development status

Treasure support is implemented. Boss, groundhog, and assault/event layers are reserved in the configuration and should be added as independent data providers and renderer layers rather than modifying the treasure provider.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.
