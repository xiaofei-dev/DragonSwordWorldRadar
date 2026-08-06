# Changelog

## 0.4.0-dev9-performance1.1

- Reduced compact-motion bridge writes through cumulative visual-delta filtering while retaining a one-second liveness heartbeat.
- Changed active minimap and world-map sampling to 24 ms without changing the fixed 21-field bridge protocol.
- Added adaptive Overlay polling at 24/50/75/125 ms for active, world-idle, radar-idle, and disabled states.
- Prevented unchanged bridge slots, heartbeat-only motion frames, and visually equivalent static frames from causing repeated reads, parsing, invalidation, or paint.
- Batched non-nearest treasure markers into retained type-specific `GraphicsPath` instances.
- Reused a validated game process ID across geometry, lifetime, and save polling.
- Added scheduler/write-suppression counters to Lua and Overlay diagnostics.
- Fixed the Windows PowerShell 5.1 CodeDOM local-variable shadowing error in `MotionVisualSnapshot.Update`.
- In-game diagnostics confirmed normal operation, adaptive timer transitions, zero bridge/write failures, and materially lower idle/active work.

# Changelog

## 0.4.0-dev8-refactor2

- Fixed the refactor1 Windows PowerShell 5.1 `Add-Type` compiler blocker.
- Added mandatory real-compiler and regression-harness gates to release builds.
- Added SHA-256 package integrity validation and exact immutable-tree checks before installation.
- Made datasets, preserved config, overrides, metadata, `mods.txt`, and startup shortcut one staged, rollback-protected transaction; watcher readiness is the commit point.
- Isolated treasure and boss save-state failures, retained last-known-good snapshots, and added bounded partial-module retry.
- Added bounded bridge/catalog reads, PAK/decoder guards, canonical ZIP validation, and per-record rendering isolation.
- No intended feature, marker-style, bridge-protocol, schema, hotkey, or sampling-interval change.

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
