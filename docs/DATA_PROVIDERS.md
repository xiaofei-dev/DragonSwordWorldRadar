# Data provider framework

Local game-data generation is implemented through `DragonSwordWorldRadar.Installer.IDataProvider`.

```csharp
public interface IDataProvider
{
    string Id { get; }
    DataSetResult Generate(InstallationContext context);
}
```

`InstallationPipeline` owns the provider registry. Version 0.4.0-dev3 registers two independent providers.

## Treasure provider

`TreasureDataProvider` extracts `SectionTreasureBoxData.xml` and generates `data/generated/treasures.lua`.

## Boss provider

`BossDataProvider` extracts `FieldBossListData.xml` and `SectionMonsterData.xml`, resolves the nine supported field-boss rows by exact boss ID, validates their UID/UIDName/XYZ data, and generates `data/generated/bosses.lua`.

The generated boss catalog is static location and identity data only. Death and respawn are runtime concerns handled by `scripts/boss_tracker.lua`; no boss status is written into the treasure save-state model.

## Adding another layer

1. Add an `IDataProvider` under `src/installer/Core/Providers`.
2. Extract or scan the required local game data.
3. Write a deterministic file under `data/generated`.
4. Return a `DataSetResult` with record count and source metadata.
5. Register the provider in `InstallationPipeline`.
6. Add a stable UE4SS module shim, state tracker if needed, and an independent renderer layer.

The installer records every returned dataset in `metadata/datasets.json`. Game-version invalidation applies to the full provider pipeline.

## Extraction constraints

- Extracted game data is generated locally and is not included in releases.
- The PAK AES key is detected from the user's local executable.
- Oodle-compatible decompression uses the bundled `ooz.exe` decoder.
- No custom radar executable is built or distributed.
