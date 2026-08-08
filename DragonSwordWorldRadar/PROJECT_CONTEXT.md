# DragonSwordWorldRadar Project Context

## Purpose

DragonSwordWorldRadar is the production mod for displaying DragonSword Awakening treasures and world bosses in radar and world-map overlays. This directory is the authoritative implementation. Use this document as the first cross-chat orientation file, then inspect `README.md`, `metadata/release.json`, the current Git diff, and runtime evidence before changing behavior.

## Current baseline

- Version: `0.4.0-dev9-performance1.8-singlebridge1`
- Runtime architecture: UE4SS Lua producer, one protocol-v2 file bridge, one hidden game-bound host, and one C# WinForms overlay
- Static datasets: generated locally by the installer from the current game PAK
- Treasure completion: read from the encrypted save database and gated until the first valid snapshot
- Boss availability: static catalog plus save-database `tb_actor_respawn` state
- Controls: F7 enables the Radar and F8 disables it

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
