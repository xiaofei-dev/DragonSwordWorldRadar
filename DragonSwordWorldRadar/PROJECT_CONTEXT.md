# DragonSwordWorldRadar Project Context

## Purpose

DragonSwordWorldRadar is the production mod for displaying DragonSword Awakening treasures, world bosses, Assault targets, and Mole/Fly activities. This directory is the authoritative implementation. Use this document as the first cross-chat orientation file, then inspect `README.md`, `metadata/release.json`, the current Git diff, and runtime evidence before changing behavior.

## Current baseline

- Version: `0.4.0-dev59-ue4ssroot1`
- Standard UE4SS runtime root: `DS/Binaries/Win64/ue4ss`; install the Mod at `DS/Binaries/Win64/ue4ss/Mods/DragonSwordWorldRadar`
- Runtime architecture: UE4SS Lua producer, one protocol-v5 scalar bridge, one hidden game-bound host, and one C# WinForms overlay
- Runtime rollback baseline: packaged dev15; post-dev15 composition-region, retained-map, treasure cache-key, and visibility-index changes are removed
- Active moving cadence: coherent 50 ms Lua radar/world-map producers and Overlay active timer; no adaptive/stationary detector
- Window state: foreground/minimize/visibility APIs sampled at 250 ms, with immediate sampling on mode transitions
- Timer A/B: `high_resolution_timer = false` by default; restart-required `true` requests `timeBeginPeriod(1)`
- Debug layer evidence: separate treasure, unified Boss/Assault encounter, Mole/Fly, and world-status timing/count aggregates
- Nearest unopened treasure selection: dev15 query-buffer/map-100 ownership with only its interval changed from 250 to 1000 ms; maximum 80
- Compact layout: 360-reference-pixel width and 400-reference-pixel height, with a 138-pixel time/phase group raised six pixels into the minimap square's transparent lower corner
- Marker rendering: exact dev15 direct retained-geometry Boss/Mole vector drawing
- Static datasets: generated locally by the installer from the current game PAK
- Treasure completion: fail-closed until the first valid encrypted-save snapshot; the first snapshot immediately rebuilds the index and publishes only records not confirmed open
- Accepted game-data rules: fixed chest 10220122 is not ignored and upgrades remove only its obsolete ignore/explanation; duplicate/offset 11003 remains ignored in favor of authoritative 14016
- Boss and Assault availability: one immutable 49-record world-encounter catalog and one save-database `tb_actor_respawn` query/cache/availability path. A fingerprint-pinned inference policy resolves optional conditions against fresh exact identities/cycles; generated conditions fail closed when their required scalar is unavailable.
- Mole/Fly visibility: exactly 34 install-generated records; each marker is hidden when the same-ID `DT_MiniGame_G5_11001-11034` reward bit is opened in the existing external `tb_treasure_box` snapshot
- World status: disabled exactly as in dev38; production performs zero time UObject reads.
- Player motion: one fresh Engine-to-current-Pawn scalar sample per 250 ms control callback; no Pawn/Controller/GameInstance/Viewport wrapper is retained. Overlay prediction remains 50 ms, bounded to 250 ms and frozen after 500 ms stale age.
- Performance baseline: packaged dev30 is the owner-accepted smoothness reference. The candidate retains later fresh-current-Pawn travel safety, so its 20 Hz Engine-to-Pawn traversal is a known steady-state deviation; debug timing separates that traversal from `K2_GetActorLocation`.
- Encounter production layer: Overlay startup combines exactly nine generated Bosses and forty validated current-PAK Assault targets, fixes the 49-ID `tb_actor_respawn` filter once, and never reloads catalogs or resets the save cache from F7.
- Save refresh: enabled-only two-second metadata checks, four-second stable-change debounce, independent in-memory `.db`/`.bak` fingerprint reuse, one SQLCipher key setup per isolated temporary snapshot connection, below-normal worker priority, and debug phase timings
- Controls: F7 enables configured marker/status layers and save work; F8 is a temporary whole-mod runtime disable for clean FPS A/B comparison and does not rewrite configuration
- Lifecycle gate: no UE4SS LoadMap hook is registered; Pawn/root loss invalidates the epoch, performs zero UObject work for three seconds, then releases one normal player-location sample; every failed sample rearms the complete cooldown and no World-identity query is used
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
- `metadata/release.json`: version and release contract

## Validation workflow

Run from this directory with Windows PowerShell 5.1:

```powershell
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
```

After deployment, verify the installed source hashes, installer log, UE4SS log, host/Overlay logs, F7/F8 behavior, map transitions, treasure visibility, and clean game-exit teardown. Never treat compilation alone as gameplay acceptance.

## Publication rules

- Keep code, metadata, documentation, identifiers, and commit messages in English.
- Do not commit `dist/`, `runtime/`, logs, diagnostics ZIPs, generated catalogs, user configuration, extracted game packages, databases, dumps, or credentials.
- Do not modify or repack original game PAK files.
- Do not commit, push, publish a release, or deploy unless explicitly authorized.
