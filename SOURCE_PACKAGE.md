# Source package status

This archive contains the repaired successor **DragonSwordWorldRadar 0.4.0-dev9-performance1.3-mapinstant-hiddenhost1**.

The repository also retains exact provenance for the last owner-tested baseline:

- Historical tested release: `DragonSwordWorldRadar-v0.4.0-dev9-performance1.1.zip`
- Historical tested release SHA-256: `96be8ea8a54bcc2d17550ab7480c5c76917f2e86714cf7a986bfb6dac61c84ac`
- Functional baseline: `0.4.0-dev8-refactor2`
- Accepted performance baseline: `0.4.0-dev9-performance1.1`
- Current repaired successor: `0.4.0-dev9-performance1.3-mapinstant-hiddenhost1`
- Overlay source files: 31
- Installer Core source files: 13
- UE4SS Lua modules: 7
- Motion protocol: fixed 21 fields

`metadata/source-release-map.json` and `metadata/source-snapshot.json` remain historical baseline records. They intentionally keep the `performance1.1` version and hashes; they do not claim that the repaired successor has already been tested in game.

## Current repair scope

The current source removes the later always-full-client radar window and high-frequency map detector, restores a real small radar window outside map mode, removes the fixed map reveal delay, performs a hidden no-copy resize plus synchronous prepaint, and immediately restores radar mode on the first missing active-map read. It also separates low-volume Use logs from opt-in Debug logs, restores the stable resident hidden watcher so game-time Lua creates no shell process, removes runtime Debug-config polling and Boss PAK-rule scanning, and preserves the fixed daily 09:00 Boss cooldown behavior.

The current repair received Linux-side static validation, including JSON/XML parsing, Lua syntax checks, C#/PowerShell lexical and delimiter scans, source-inventory checks, semantic lifecycle assertions, unexpected-binary checks, and bundled binary hashes. This environment cannot execute Windows PowerShell 5.1 `Add-Type` or the game.

Therefore:

- No in-game FPS improvement is claimed by this source package alone.
- No in-game map-transition pass is claimed.
- No Windows PowerShell compile pass is claimed here.
- The authoritative deployment gate remains `Install.cmd`; it must print `OVERLAY_COMPILE_OK` before installation is accepted.

## Updating an existing repository

Copy the **contents** of this directory into the repository root while preserving the existing `.git` directory. Remove obsolete tracked files that are absent from this snapshot. Do not retain experimental probes, backup source files, previous `dist/` output, or generated runtime state as source.

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

`Compile-Source.ps1` uses the same Windows PowerShell 5.1 `Add-Type` compiler path as installation and the runtime host. `Build-Release.ps1` performs all validation gates before producing `dist/DragonSwordWorldRadar-v0.4.0-dev9-performance1.3-mapinstant-hiddenhost1.zip`.

The source verifier now rejects the obsolete 96 ms always-on map detector, map-exit hysteresis, fixed-fullscreen radar helpers, timed map warm-up state, game-time shell launch, runtime Debug-config polling, and runtime Boss PAK scanning. It requires actual small radar geometry, hidden no-copy transition repaint, the resident hidden watcher, split logs, debug-only performance counters, recoverable async pending gates, and the fixed daily 09:00 Boss rule.
