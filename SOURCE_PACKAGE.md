# Source package status

This archive contains the **complete repository snapshot** for
**DragonSwordWorldRadar 0.4.0-dev9-performance1.8-singlebridge1**. It follows
the same project structure as the owner-provided source archive; it is not the
installable Mod folder with binaries removed.

## Included

- existing `.git` history and `dev` branch metadata from the owner-provided repository;
- `DragonSwordWorldRadar.sln`;
- `src/ue4ss`, `src/overlay`, `src/installer`, `src/host`, and `src/tools`;
- `build`, `docs`, `resources`, `metadata`, licenses, and vendor dependencies;
- historical `dist` output from the supplied repository;
- the exact tested installable archive:
  `dist/DragonSwordWorldRadar-v0.4.0-dev9-performance1.8-singlebridge1.zip`;
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

The repository README contains source/build guidance in addition to runtime
instructions, so it intentionally differs from the tested release's
`README.txt`. All mapped production code and runtime script files are
byte-identical to the tested 1.8 release.

## Current architecture

- one protocol-v2 Motion/control Bridge;
- 27 fixed ASCII fields and two alternating slots;
- no active JSON Static Bridge;
- 6 UE4SS Lua modules;
- 31 Overlay C# files;
- 13 Installer Core C# files;
- 24 ms minimap/world-map motion sampling;
- 250 ms low-frequency control sampling;
- 20 XY and 10 Z publication thresholds;
- Overlay-owned treasure and nine-Boss catalogs with save filtering.

The retired files are intentionally deleted in the working tree:

```text
src/ue4ss/boss_tracker.lua
src/overlay/Bridge/StaticStateBridgeReader.cs
src/overlay/Models/RadarState.cs
```

The new files are intentionally untracked relative to the older included Git
commit until reviewed and committed:

```text
src/overlay/Data/WorldBossCatalog.cs
src/overlay/Models/OverlayModels.cs
```

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
