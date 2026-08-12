# DragonSwordWorldRadar Project Context

## Purpose

DragonSwordWorldRadar is the production mod for displaying DragonSword Awakening treasures, world bosses, Assault targets, and Mole/Fly activities. This directory is the authoritative implementation. Use this document as the first cross-chat orientation file, then inspect `README.md`, `metadata/release.json`, the current Git diff, and runtime evidence before changing behavior.

## Current baseline

- Version: `0.4.0-dev74-minigamecatalog2`
- Standard UE4SS runtime root: `DS/Binaries/Win64/ue4ss`; install the Mod at `DS/Binaries/Win64/ue4ss/Mods/DragonSwordWorldRadar`
- Runtime architecture: UE4SS Lua producer, one protocol-v6 scalar bridge with a strict inline diagnostic-mode enum, one hidden game-bound host, and one C# WinForms overlay
- Runtime rollback baseline: packaged dev15; post-dev15 composition-region, retained-map, treasure cache-key, and visibility-index changes are removed
- Active moving cadence: 250 ms fresh current-player samples, unchanged 50 ms compact Lua scalar flushing, 33 ms compact in-memory Overlay prediction/presentation, and 8 ms producer/presentation only while the expanded map is visible
- Window state: foreground/minimize/visibility APIs sampled at 250 ms, with immediate sampling on mode transitions
- Timer A/B: `high_resolution_timer = false` by default; restart-required `true` requests `timeBeginPeriod(1)`
- Debug layer evidence: separate treasure, unified Boss/Assault encounter, Mole/Fly, and world-status timing/count aggregates
- Normal-mode hot path: `debug_logging = false` prevents F7 trace activation, eager trace-field tables, Overlay performance bookkeeping, and the diagnostic-only encounter-task SQL query; use logging remains enabled
- Nearest unopened treasure selection: dev15 query-buffer/map-100 ownership with only its interval changed from 250 to 1000 ms; maximum 80. One allocation-free pass retains the two closest visible treasures for height arrows; only the closest marker is enlarged and labeled.
- Compact layout: 360-reference-pixel width and 400-reference-pixel height, with a 138-pixel time/phase group raised six pixels into the minimap square's transparent lower corner
- Marker rendering: exact dev15 direct retained-geometry Boss/Mole vector drawing
- Static datasets: generated locally by the installer from the current game PAK
- Treasure completion: fail-closed until the first valid encrypted-save snapshot; the first snapshot immediately rebuilds the index and publishes only records not confirmed open
- Accepted game-data rules: fixed chest 10220122 is not ignored and upgrades remove only its obsolete ignore/explanation; duplicate/offset 11003 remains ignored in favor of authoritative 14016
- Boss and Assault availability: one immutable 49-record world-encounter catalog and one save-database `tb_actor_respawn` query/cache/availability path. A fingerprint-pinned inference policy resolves optional conditions against fresh exact identities/cycles; generated conditions fail closed when their required scalar is unavailable.
- Mole/Fly visibility: exactly 34 install-generated records; each marker is hidden when the same-ID `DT_MiniGame_G5_11001-11034` reward bit is opened in the existing external `tb_treasure_box` snapshot
- World status: one isolated `DGameSingleton.TimeOfDay` baseline attempt after the F7 stability gate, followed by local wall-time extrapolation; no retry, recalibration, or recurring provider read.
- Player motion: one fresh Engine-to-current-Pawn scalar sample per 250 ms control callback; no Pawn/Controller/GameInstance/Viewport wrapper is retained. Overlay prediction runs at 33 ms, remains bounded to 250 ms, and freezes after 500 ms stale age.
- Performance acceptance: evidence-driven against current F8 and same-session diagnostic F5/F6/F7 tests; no historical package cadence is an acceptance baseline. F5/F6 are registered only when `debug_logging = true`. Compact bridge slots are consumed only when notified dirty, by a 250 ms healthy dual-slot fallback, or by a 50 ms polling fallback when notifications are unavailable. Watcher callbacks never parse. Expanded-map presentation remains 8 ms with notifications disabled; hidden bounded candidates are visibility-tested at 250 ms, and an active read failure drops every wrapper before arming one next-control-sample bounded rescan.
- Encounter production layer: Overlay startup combines exactly nine generated Bosses and forty validated current-PAK Assault targets, fixes the 49-ID `tb_actor_respawn` filter once, and never reloads catalogs or resets the save cache from F7.
- Save refresh: enabled-only two-second metadata checks, four-second stable-change debounce, and one shared 45-second complete-snapshot window. Continuous autosaves replace only the pending fingerprint; at most one below-normal SQLCipher worker opens per window and reads the latest stable Treasure-rich snapshot. Independent `.db`/`.bak` fingerprint reuse can eliminate that read when unchanged. Publication remains request-scoped as a fail-closed boundary, and debug output distinguishes requested Treasure publication from executed Treasure queries.
- Controls: normal play registers only F7 for normal 33 ms compact presentation and F8 for whole-mod disable. When `debug_logging = true`, F5 additionally suppresses Overlay painting while preserving producer/bridge/control work, and F6 freezes rendered motion while retaining producer/bridge work with 250 ms control reads.
- Lifecycle gate: no UE4SS LoadMap hook is registered; Pawn/root loss invalidates the epoch, performs zero UObject work for three seconds, then releases one normal player-location sample; every failed sample rearms the complete cooldown and no World-identity query is used
- Runtime error recovery: confirmed async exceptions or five consecutive one-second observations of a stalled 250 ms control heartbeat perform one token/epoch-invalidating automatic F8-to-F7 restart through the existing four-sample activation gate. Existing active presentation loops provide the scalar watchdog, so no fourth `LoopAsync` is registered; temporary missing world-map state is explicitly excluded.
- Reference object policy: Engine and `DLayerMiniMap` are cached exactly like the supplied 1.6.1 mod and resolved only when missing/invalid; no task/cutscene HUD visibility UObject query is performed

The current source has passed Windows PowerShell 5.1 source verification, Overlay and installer compilation, the refactor harness, and release packaging. These gates prove build integrity, not visible in-game behavior.

## Ownership and integration boundaries

- Production behavior belongs in this directory.
- Research observations from `../DragonSwordWorldDataProbe/` are evidence, not automatic production truth.
- Local `../DragonSwordWorldRadar-Dev/` and `../DragonSwordWorldRadar-Mole/` trees are intentionally excluded from Git. Never publish or force-add them.
- Integrate isolated feature work by porting only the accepted feature-specific code into this project. Reuse the existing host, watcher, bridge, overlay lifecycle, and configuration ownership.
- Preserve fail-closed behavior when save data, generated catalogs, bridge records, or game-build identity are missing or invalid.

## Important entry points

- `src/ue4ss/main.lua`: UE4SS bootstrap and bridge production
- `src/ue4ss/world_map.lua`: world-map state production
- `src/overlay/Program.cs`: Overlay process entry point
- `src/overlay/UI/RadarForm.cs`: UI lifecycle and rendering coordination
- `src/overlay/SaveData/TreasureSaveState.cs`: save refresh and published completion state
- `src/ue4ss/mole_completion.lua`: query-free safety provider that publishes the validated 34-record candidate mask
- `src/overlay/Data/MoleRewardVisibilityIndex.cs`: raw same-ID reward-bit filter, isolated from treasure ignore/alias overrides
- `src/ue4ss/world_environment.lua`: retained inactive research implementation; production `main.lua` must not import it
- `src/overlay/Data/WorldMoleCatalog.cs`: immutable, map-indexed 34-record catalog
- `src/overlay/Rendering/WorldStatusRenderer.cs`: retained-resource time/phase renderer
- `src/installer/Install.ps1`: installed configuration, compilation, dataset generation, and watcher setup
- `build/Verify-Source.ps1`: structural and safety verification
- `build/Build-Release.ps1`: full release gate and package generation
- `build/Test-ReleasePackage.ps1`: independent ZIP, path, manifest, and staging-byte audit
- `metadata/release.json`: version and release contract

## Validation workflow

Run from this directory with Windows PowerShell 5.1:

```powershell
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
& .\build\Test-ReleasePackage.ps1
```

After deployment, verify the installed source hashes, installer log, UE4SS log, host/Overlay logs, F7/F8 behavior, map transitions, treasure visibility, and clean game-exit teardown. Never treat compilation alone as gameplay acceptance.

## Publication rules

- Keep code, metadata, documentation, identifiers, and commit messages in English.
- Do not commit `dist/`, `runtime/`, logs, diagnostics ZIPs, generated catalogs, user configuration, extracted game packages, databases, dumps, or credentials.
- Do not modify or repack original game PAK files.
- Do not commit, push, publish a release, or deploy unless explicitly authorized.
