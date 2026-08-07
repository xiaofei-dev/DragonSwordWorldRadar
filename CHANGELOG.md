# Changelog

## 0.4.0-dev9-performance1.3-mapinstant-hiddenhost1

- Removed the 1000 ms world-map reveal delay and all associated warm-up state; map entry and exit now restore the Overlay in the same timer cycle.
- Kept hide-before-resize, added `SWP_NOCOPYBITS`, and synchronously repaints the newly sized hidden surface before `SW_SHOWNOACTIVATE`. A paint-sequence check skips the second full-screen invalidate only when hidden prepaint actually completed. Debug mode records `MAP_SURFACE_PREPARED` timing. This is a zero-timer attempt to suppress stale layered-window pixels without retaining a full-screen surface outside map mode.
- Restored the proven resident hidden WScript launcher used by the stable baseline. Game-time Lua now writes only `runtime\launch.request` and no longer invokes `cmd.exe`, `wscript.exe`, or PowerShell, eliminating the transient console-window path.
- The installer recreates and validates the hidden user Startup watcher; the game-bound PowerShell host remains hidden and exits with the exact game process.
- Classified the expected pre-login save-key-not-ready state as Debug-only instead of writing a normal-use stack trace.
- The diagnostic game executable is 162,551,704 bytes; normal startup no longer reads it before the known RVAs have had 30 seconds to resolve.
- Launch request stamps are digits-only and newline-free; WScript validates them before command construction, fixing the observed broken WatcherHost log lines and reducing parameter-injection surface.
- Changed save-key discovery to try cached/current/legacy RVAs first. A full game-EXE signature scan is now a one-time worker-thread fallback after 30 seconds; transient owner/key-not-ready states neither trigger it early nor disable it, and the Overlay UI thread is never blocked by that scan.
- No bridge fields, treasure/Boss semantics, save schema, marker style, F7/F8 behavior, or active map-producer intervals changed.

## 0.4.0-dev9-performance1.2-mapfix-debug1-cleanup1

- Removed Overlay runtime polling for `debug_logging`/`diagnostic_verbose`; the setting is parsed once at process startup, so normal Timer ticks and debug-log guards perform no config-file stat/read work.
- Removed runtime Boss RespawnCycle PAK enumeration, binary-to-text decoding, regex parsing, the 30-second debug rescan path, and the associated locks/state.
- Preserved Boss availability semantics with the verified fixed rule-106 schedule: daily reset at 09:00 local time.
- Removed the save-worker call that existed only to refresh the optional Boss rule scanner.
- Added source-verifier assertions that reject reintroduction of Debug hot-reload polling and runtime Boss PAK scanning.
- No bridge schema, save-table interpretation, marker behavior, F7/F8 behavior, or map lifecycle logic changed.

## 0.4.0-dev9-performance1.2-mapfix-debug1

- Removed the always-full-client radar window introduced after `performance1.1`; radar mode again uses an actual small top-right layered window.
- Changed world-map lifecycle handling to hide the Overlay before resizing, keep it hidden for 1000 ms after every map entry, and restore the small radar immediately on the first confirmed map-close read.
- Removed the 96 ms always-on map UObject detector, three-miss exit confirmation, and 250 ms stale re-entry block from the 24 ms radar motion path.
- Restricted 24 ms world-map UObject sampling to the period in which the world map is actually active, and reused the active transform in the 250 ms static publisher.
- Made Lua game-thread queue/callback gates recover after scheduling or callback exceptions instead of remaining permanently pending.
- Split Lua and Overlay output into low-volume `Use` logs and opt-in `Debug` logs. Normal mode no longer computes or writes periodic performance diagnostics.
- Expanded debug diagnostics with producer queue/update latency, 50/100/250 ms stall counts, motion sample/write rates, Overlay timer/paint gaps, normalized process CPU, working set, visibility, and geometry. `overlayPaintFps` is explicitly labeled as an Overlay metric rather than game Present FPS.
- Reduced optional Boss respawn-rule PAK discovery to one scan per Overlay process in normal mode; periodic rescans remain available only in debug mode. PAK enumeration failures now preserve the daily 09:00 fallback and cannot abort save-state refresh.
- Added transient game-process and game-window failure recovery, log-session rotation, and exact source-verifier rules for the repaired map lifecycle.
- Hardened Overlay launch retry and serialized, before/after-consistent debug-config reads across UI/save worker threads; the normal timer checks for debug-mode changes only once per second.
- Removed the no-op Boss tracker call from the 250 ms static-state build.
- Excluded generated `runtime`/patch-deployment backups from the source-only unexpected-EXE scan while retaining SHA-256 enforcement for the single bundled `ooz.exe`.
- Static validation was completed in Linux. Windows PowerShell 5.1 `Add-Type` compilation and in-game FPS/transition validation remain mandatory deployment checks; `Install.cmd` must print `OVERLAY_COMPILE_OK`.

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
