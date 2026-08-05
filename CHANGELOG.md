# Changelog

## 0.4.0-dev7-stable6

- Frozen dev7 stability baseline after in-game validation of F7/F8, minimap and world-map double buffering.
- Restored the complete original dev7 rendering implementation before applying narrowly scoped visibility and marker changes.
- Overlay remains active after F7 until F8; foreground, overlap, and minimized-window visibility polling are removed.
- Host and watcher processes use a temporary working directory rather than holding the Mod folder as their current directory.
- Main Radar contains no experimental Boss, treasure, or sudden-mission collection hooks.
- Treasure and Boss marker colors share `RadarMarkerStyle`; Boss uses the accepted warm-red visual.
- Treasure outlines use the shared dark outline. World-map paths use `FillMode.Winding` and projected-pixel deduplication.
- Installer validates/repairs `config.lua` and compiles the exact complete Overlay source set before installation succeeds.

## 0.4.0-dev5

- World-map refresh now runs at 8 ms while dragging or zooming and returns to 16 ms after 250 ms of stable input.

- Reduced radar motion sampling from 16 ms to 32 ms while retaining the 16 ms world-map producer.
- Producer loops now terminate completely while disabled and restart only on F7.
- World-boss Character discovery runs at most every 5 seconds and only near a known spawn.
- Installer now changes only the DragonSwordWorldRadar entry in mods.txt.

- Added a local-PAK `BossDataProvider` that generates exactly nine world-boss records.
- Added independent UE4SS world-boss runtime tracking and bridge payloads.
- Added a larger unified marker modeled on the game's field-boss map icon to the minimap and world map.
- Added death/disappearance hiding and actor-respawn restoration for previously observed bosses.
- Added independent `show_bosses` and `show_treasures` rendering controls.
- Changed the treasure player-height reference offset from `-120` to `-150`.
- Fixed release packaging to include the validated bundled `tools/ooz.exe`.
- Updated source verification to allow only the declared `ooz.exe` tool binary.

## 0.3.2c

- Established the standalone DragonSwordWorldRadar repository and release layout.
- Removed the installation dependency on DragonSwordTreasureMap 1.6.1.
- Added one-click local treasure-data extraction and generation.
- Added a reusable `IDataProvider` pipeline for future radar layers.
- Added game-version fingerprinting and launch-time reinstall prompts.
- Preserved the completed treasure layer.
