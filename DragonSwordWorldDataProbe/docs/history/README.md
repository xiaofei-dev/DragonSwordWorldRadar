# Historical Module Archive

This directory is the stable entry point for historical DragonSwordWorldDataProbe module records. It documents preserved implementations without changing their runtime status.

## Evidence labels

- `Production accepted`: implemented in DragonSwordWorldRadar and supported by runtime evidence.
- `Confirmed research`: directly observed or deterministically joined, but not necessarily production behavior.
- `Inferred`: supported by multiple observations but not yet closed by a controlled test.
- `Unknown`: the available evidence does not establish semantics.
- `Archived`: implementation and evidence are preserved but excluded from the active runtime path.

## Archive files

- [Runtime Modules](runtime-modules.md): UE4SS Lua modules and their safety boundaries.
- [External Modules](external-modules.md): post-exit PowerShell collectors and analyzers.
- [Data and Evidence](data-and-evidence.md): build-pinned catalogs, reports, metadata, and ground truth.
- [World Time and Weather](world-time-and-weather.md): hidden clock observations, conversion rules, and weather limits.
- [Production Migrations](production-migrations.md): research that has been accepted or partially ported into DragonSwordWorldRadar.

## Current active scope

Version 1.0.55 automatically runs only `healthcheck`. Assault runtime sampling is archived after the static RevealCycle table and the production implementation boundary were established. Treasure sampling is archived, Mole and Boss research are frozen, automatic `MonsterSpawnBase` enumeration is prohibited, and no global Character or Actor scan is permitted.

Historical source presence does not imply that a module is active, safe for automatic use, or production accepted. Always check `scripts/config.lua`, `config/modules.json`, and `metadata/method-matrix.json`.
