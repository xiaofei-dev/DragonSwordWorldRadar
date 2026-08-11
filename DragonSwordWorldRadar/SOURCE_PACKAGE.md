# Source package status

This archive contains the **complete repository snapshot** for
**DragonSwordWorldRadar 0.4.0-dev47-minimapdiagnostics1**. It follows
the same project structure as the owner-provided source archive; it is not the
installable Mod folder with binaries removed.

## Included

- existing `.git` history and `dev` branch metadata from the owner-provided repository;
- `DragonSwordWorldRadar.sln`;
- `src/ue4ss`, `src/overlay`, `src/installer`, `src/host`, and `src/tools`;
- `build`, `docs`, `resources`, `metadata`, licenses, and vendor dependencies;
- historical `dist` output from the supplied repository;
- the exact tested installable archive:
  `dist/DragonSwordWorldRadar-v0.4.0-dev47-minimapdiagnostics1.zip`;
- its extracted release staging folder under `dist/`.

Runtime logs and generated user state are not included.

## Repository-to-release mapping

```text
src/ue4ss/*          -> scripts/*
src/host/*           -> host/*
src/installer/Install.ps1 -> installer/Install.ps1
src/installer/Core/* -> src/installer/*
src/overlay/*        -> src/overlay/*
src/tools/*          -> tools/*
resources/defaults/* -> data/defaults/*
```

The repository README is copied directly to release `README.txt`. The current
source-to-release map contains 81 byte-identical mappings generated from the
dev11 staging tree. Preserved 1.8 evidence remains baseline evidence only and
is not used to claim byte identity for changed dev11 files.

## Current architecture

- one protocol-v5 scalar Motion/control Bridge;
- 35 fixed ASCII fields and two alternating slots;
- no active JSON Static Bridge;
- 9 UE4SS Lua modules;
- 36 Overlay C# files;
- 15 Installer Core C# files;
- 50 ms minimap/world-map motion sampling;
- 250 ms low-frequency control with one fresh compact-HUD widget sample per second;
- 20 XY and 10 Z publication thresholds;
- Overlay-owned treasure and nine-Boss catalogs with save filtering.
- Overlay-owned 34-record Mole/Fly catalog with bounded completion filtering.
- Dev38 remains the zero-clock Assault-isolation package. Dev39 is the separate follow-up with one protected, serialized scalar capture and wall-elapsed local advancement; it must not be tested until dev38 performance is accepted.
- Save refresh is F7-gated, debounced, below-normal priority, keyed once per isolated temporary snapshot connection, and cached independently per `.db`/`.bak` session fingerprint.

The retired files are intentionally deleted in the working tree:

```text
src/ue4ss/boss_tracker.lua
src/overlay/Bridge/StaticStateBridgeReader.cs
src/overlay/Models/RadarState.cs
```

Successor implementation files may remain untracked relative to the included
baseline commit until owner review. `metadata/source-manifest.json` is the
authoritative complete current source inventory.

## Validation and release build

Run from Windows PowerShell 5.1:

```powershell
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
```

`Compile-Source.ps1` and installation use the same Windows PowerShell 5.1
CodeDOM compiler path. Linux-side preparation validates source/release byte
mapping, Lua syntax, C#/PowerShell structure, metadata, Git object integrity,
manifest hashes, package paths, CRC, and bundled binary hashes. Windows
compilation and in-game behavior remain authoritative.
