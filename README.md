# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar framework for **DragonSword Awakening**. Version 0.4.0-dev7-stable6 is a runtime-bridge and save-state stabilization build based on the v0.3.2c treasure baseline.

## Stable4 changes

- Rebuilt from the original validated dev7 `RadarForm.cs`, not from stable1-stable3.
- The overlay remains visible and active after F7 until F8, including while the game is in the background.
- Treasure `DrawEllipse` and batched `GraphicsPath` outlines remain, but each outline uses the same color as its marker.
- Marker colors are centralized in `TreasureMarkerPalette`.
- Host and watcher processes use `%TEMP%` as their working directory.
- `Install.cmd` validates `config.lua` and compiles the exact overlay source set before reporting success.
- No experimental collection hook is loaded by the main Radar.

## Implemented layers

### Treasures

- Minimap and world-map markers generated from the local game PAK.
- Minimap movement sampled at 16 ms and consumed by the WinForms overlay through compact double-buffered motion records.
- World-map transforms sampled at 8 ms only while the world map is open.
- Static marker/configuration state uses separate 250 ms alternating JSON slots.
- XYZ nearest-treasure selection, type colors, height pointer, overrides, aliases, and SQLCipher save filtering.
- Player-height reference offset is `-150`.

### World bosses

- Nine fixed world-boss locations generated from `SectionMonsterData.xml`.
- One enlarged marker modeled on the game's field-boss icon, rendered below treasure markers.
- Visibility is derived from `tb_actor_respawn`, not streamed `Character` objects.
- Save snapshots include the active `.db` and `.bak` files plus WAL/SHM/journal sidecars.
- Readable RespawnCycleData override PAKs are detected dynamically; arbitrary relative-minute rules are supported.
- The original local daily 09:00 reset remains the fallback when no readable override is present.
- A visible marker means the Boss is eligible to spawn. The game may still require a teleport or area reload before the Actor is instantiated.

No `FindAllOf("Character")` scan is used.

## Runtime isolation

The main Radar contains no experimental Boss, treasure, or sudden-mission collection hooks. Exploration and future layer discovery are isolated in the separate `DragonSwordWorldDataProbe` Mod.

## Installation

1. Install UE4SS.
2. Place the release folder at `Mods/DragonSwordWorldRadar`.
3. Close the game and run `Install.cmd`.
4. Start the game normally.
5. Press **F7** to enable and **F8** to disable.

The installer changes only the `DragonSwordWorldRadar` entry in `mods.txt` and preserves other Mod entries.

## Game updates

Installation stores a fingerprint of the game executable and generated-data PAK in `metadata/install-state.json`. If the game changes, the watcher requests reinstallation rather than running stale datasets.

## Repository layout

- `src/ue4ss` — game-thread sampling and compact/static bridge producers.
- `src/overlay` — WinForms renderer, bridge consumers, and save-state filtering.
- `src/installer/Core` — local PAK data-provider pipeline.
- `src/host` — hidden watcher and in-memory overlay host.
- `src/tools` — diagnostics collector.
- `resources/defaults` — preserved user-facing default data.
- `vendor/sqlcipher` — SQLCipher runtime for local save reads.
- `vendor/ooz` — installation-time game-data decoder.
- `build` — source verification and release packaging.

See `docs/ARCHITECTURE.md` and `docs/DATA_PROVIDERS.md`.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.

Stable6 graphics-only fix:
- Treasure fills retain their semantic type colors.
- Treasure outlines now use the shared neutral dark marker outline.
- World-map paths use winding fill and projected-pixel deduplication.
- Boss visuals, bridge timing, save logic, and runtime behavior are unchanged.
