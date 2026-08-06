# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar framework for **DragonSword Awakening**. Version `0.4.0-dev9-performance1.1` keeps the validated treasure and world-boss behavior of `0.4.0-dev8-refactor2` while optimizing the measured file-bridge, polling, and WinForms rendering hot paths.

## Performance1 focus

- The compact motion protocol remains exactly 21 fields. Minimap and world-map movement are sampled every 24 ms. A slot is written only after a visible movement/transform delta or after a 1000 ms liveness heartbeat.
- Player XY writes use a cumulative 20 game-unit threshold, player Z uses 10 units, and world-map transforms suppress sub-pixel pan/zoom jitter. Because the comparison is against the last published frame, small changes accumulate and are never permanently lost.
- The Overlay checks each alternating slot's timestamp and length before opening it. Unchanged motion and static files therefore incur metadata checks only, not repeated file reads and parsing.
- Overlay refresh is adaptive: 24 ms during visual motion, 50 ms for an idle world map, 75 ms for an idle minimap, and 125 ms while disabled. Activity resumes immediately when a newer visual frame is delivered.
- Heartbeat-only motion frames and visually equivalent static states update liveness without invalidating or repainting the form.
- Non-nearest treasure markers are grouped into four retained type-specific `GraphicsPath` batches. All four batches share one smoothing-state transition per paint. The nearest marker, height indicator, labels, and boss markers retain their previous behavior.
- Game lifetime, geometry, and save-state polling reuse a validated tracked process ID before falling back to process enumeration.
- Static marker/configuration state remains double-buffered and is generated every 250 ms.

The bridge schema, generated data schema, save-table interpretation, marker colors, F7/F8 controls, height behavior, and boss filtering semantics are unchanged.

## Diagnostics

`Collect-Diagnostics.cmd` now exposes both sides of the scheduler:

- Lua `LUA_PERF`: `motion_samples`, `motion_write_skips`, `motion_skip_percent`, and `motion_write_hz`.
- Overlay performance: total/static/motion frames, redraws, suppressed frames, invalidates, paints, refresh time, and paint time.

On an idle minimap, expected behavior is approximately one motion heartbeat write per second, a 75 ms Overlay timer, and no heartbeat-triggered paint. Actual values depend on movement, map interaction, storage, antivirus scanning, and system load.

## Implemented layers

### Treasures

- Minimap and world-map markers generated from the local game PAK.
- XYZ nearest-treasure selection, semantic type colors, height pointer, overrides, aliases, and SQLCipher save filtering.
- Player-height reference offset is `-150`.

### World bosses

- Nine fixed world-boss locations generated from local game data.
- One enlarged marker modeled on the game's field-boss icon.
- Eligibility is derived from `tb_actor_respawn`, not streamed `Character` objects.
- Save snapshots include active `.db`/`.bak` files and WAL/SHM/journal sidecars.
- Readable RespawnCycleData rule-106 override PAKs are detected dynamically; the local daily 09:00 reset remains the fallback.

No `FindAllOf("Character")` scan or experimental HP/death hook is used.

## Reliability

- Installation compiles the exact complete Overlay source set under Windows PowerShell 5.1 before installing the watcher. A compile failure stops installation.
- Static and motion bridges use alternating files and sequence validation. Invalid or partial frames never replace the last complete published frame.
- Save, catalog, visibility-index, boss, diagnostics, timer, and renderer failures are isolated and rate-limited.
- Catalog, override, and per-map visibility replacements publish only after complete consistency checks.
- A motion stream older than 2500 ms falls back to the latest complete static state.

## Installation

1. Install UE4SS.
2. Extract the complete release folder to `Mods/DragonSwordWorldRadar`.
3. Close the game and run `Install.cmd`.
4. Confirm the installer prints `OVERLAY_COMPILE_OK`.
5. Start the game normally.
6. Press **F7** to enable and **F8** to disable.

Do not run `Install.cmd` from inside a ZIP preview. The installer preserves a valid `scripts/config.lua`, `data/treasure_overrides.txt`, and unrelated entries in `mods.txt`.

## Game updates

Installation stores a fingerprint of the game executable and generated-data PAK in `metadata/install-state.json`. If the game changes, the watcher requests reinstallation instead of running stale datasets.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.
