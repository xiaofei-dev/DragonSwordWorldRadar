# Source package status

This archive contains the **complete repository snapshot** for
**DragonSwordWorldRadar 0.4.0-dev74-minigamecatalog2**. It follows
the same project structure as the owner-provided source archive; it is not the
installable Mod folder with binaries removed.

## Included

- existing `.git` history and `dev` branch metadata from the owner-provided repository;
- `DragonSwordWorldRadar.sln`;
- `src/ue4ss`, `src/overlay`, `src/installer`, `src/host`, and `src/tools`;
- `build`, `docs`, `resources`, `metadata`, licenses, and vendor dependencies;
- historical `dist` output from the supplied repository;
- the exact tested installable archive:
  `dist/DragonSwordWorldRadar-v0.4.0-dev74-minigamecatalog2.zip`;
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
source-to-release map is regenerated from the current staging tree and records
byte identity for every mapped source/release file. Historical release evidence
is not used to claim identity for the current package.

## Current architecture

- one protocol-v6 scalar Motion/control Bridge;
- 38 fixed ASCII fields and two alternating slots;
- no active JSON Static Bridge;
- 9 UE4SS Lua modules;
- 41 Overlay C# files;
- 18 Installer Core C# files;
- 250 ms fresh current-player sampling and a scalar-only 50 ms compact flush loop;
- 33 ms compact in-memory Overlay prediction/presentation with dirty-slot bridge ingestion, a 250 ms healthy safety scan, and a 50 ms watcher-unavailable fallback;
- 8 ms transform production/presentation only while the expanded map is visible;
- 250 ms low-frequency control with one protected minimap-scale sample per second;
- 20 XY and 10 Z publication thresholds;
- Overlay-owned treasure and nine-Boss catalogs with save filtering.
- Overlay-owned 34-record Mole/Fly catalog with bounded completion filtering.
- One isolated world-clock scalar is attempted after the F7 stability gate and advances locally without retry or recalibration.
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
CodeDOM compiler path. The Windows release gate validates source/release byte
mapping, C#/PowerShell structure, metadata, manifest hashes, ZIP paths,
decompression/CRC reads, exact ZIP-to-staging bytes, and bundled binary sets.
In-game behavior and frame-time evidence remain authoritative.
