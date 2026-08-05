# Data provider framework

Local game-data generation is implemented through `DragonSwordWorldRadar.Installer.IDataProvider`.

```csharp
public interface IDataProvider
{
    string Id { get; }
    DataSetResult Generate(InstallationContext context);
}
```

`InstallationPipeline` owns the provider registry. Version 0.3.2c registers `TreasureDataProvider` only.

## Adding a future layer

For a boss layer:

1. Add `BossDataProvider : IDataProvider` under `src/installer/Core/Providers`.
2. Extract or scan the required local game data.
3. Write a deterministic file such as `data/generated/bosses.lua`.
4. Return a `DataSetResult` with record count and source metadata.
5. Register the provider in `InstallationPipeline`.
6. Add a stable UE4SS module shim and an independent renderer layer.

The installer automatically records every returned dataset in `metadata/datasets.json`. Game-version invalidation applies to the entire provider pipeline, so a reinstall regenerates treasures, bosses, and future datasets together.

## Extraction constraints

- Extracted game data is generated locally and is not included in releases.
- The PAK AES key is detected from the user's local executable.
- Oodle decompression uses the open-source `ooz.exe` decoder.
- No extractor executable is shipped.
