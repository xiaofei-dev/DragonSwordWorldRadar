# Stable6 source snapshot

This archive is the clean source-tree snapshot corresponding to **DragonSwordWorldRadar 0.4.0-dev7-stable6**.

- Stable release archive SHA-256: `81c3f3528c3950e8d980d5b7485f2933b8b0f00b65324d01563e2b3083cc66b8`
- Baseline: original complete local repository supplied by the project owner.
- Generated `dist/` output and `.git/` internals are deliberately excluded.
- Stable6 runtime files were mapped back to their repository source locations.
- The main Radar contains no experimental data-probe hooks.
- `DragonSwordWorldDataProbe` remains a separate project and is not included here.

## Copying into an existing clone

Copy the contents of this directory into the repository root. Preserve the existing `.git` directory. Delete obsolete files that are absent from this snapshot, especially `src/ue4ss/treasure_event_probe.lua`.

## Build

```powershell
& .\build\Verify-Source.ps1
& .\build\Build-Release.ps1
```

The authoritative Windows compile gate is performed by the packaged `Install.cmd`, which compiles the complete Overlay source set before installation completes.
