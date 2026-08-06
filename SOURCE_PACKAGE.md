# Tested source baseline

This archive is the repository-layout source snapshot corresponding to **DragonSwordWorldRadar 0.4.0-dev9-performance1.1**, the version installed and tested in game by the project owner.

- Tested release archive: `DragonSwordWorldRadar-v0.4.0-dev9-performance1.1.zip`
- Tested release SHA-256: `96be8ea8a54bcc2d17550ab7480c5c76917f2e86714cf7a986bfb6dac61c84ac`
- Functional baseline: `0.4.0-dev8-refactor2`
- Performance baseline: `0.4.0-dev9-performance1.1`
- Overlay source files: 31
- Installer Core source files: 13
- UE4SS Lua modules: 7
- Motion protocol: fixed 21 fields

The release files were mapped back to their repository locations. Text files are normalized to UTF-8/LF in this archive; `.gitattributes` restores the intended Windows line endings for C#, PowerShell, CMD, and VBS files on checkout. Binary dependencies are byte-identical to the tested release.

## Updating an existing repository

Copy the **contents** of this directory into the repository root while preserving the existing `.git` directory. Then remove obsolete tracked files that are absent from this snapshot. In particular, do not retain experimental probes, backup source files, old generated manifests, or previous `dist/` output.

A safe Git workflow is:

```powershell
git status --short
# Copy this snapshot over the repository root.
git add -A
git diff --cached --stat
```

## Validation and build

Run from Windows PowerShell 5.1:

```powershell
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
```

`Compile-Source.ps1` uses the same Windows PowerShell 5.1 `Add-Type` compiler path as installation and the runtime host. `Build-Release.ps1` performs all three gates before producing `dist/DragonSwordWorldRadar-v0.4.0-dev9-performance1.1.zip`.

`metadata/source-release-map.json` records the exact tested-release mapping and hashes. It is a baseline provenance record, not a restriction on later source edits.

## Source package verification correction

The source-package verifier checks producer timing constants in `src/ue4ss/main.lua` and performance counter names in `src/ue4ss/diagnostics.lua`. The runtime behavior and release payload are unchanged; this corrects only the repository verification target for `motion_write_skips` and `motion_write_hz`.
