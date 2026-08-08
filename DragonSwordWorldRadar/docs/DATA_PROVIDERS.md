# Data provider framework

Local game-data generation is implemented through `DragonSwordWorldRadar.Installer.IDataProvider`.

```csharp
public interface IDataProvider
{
    string Id { get; }
    DataSetResult Generate(InstallationContext context);
}
```

`InstallationPipeline` registers two providers.

## Treasure provider

`TreasureDataProvider` extracts `SectionTreasureBoxData.xml` and generates `data/generated/treasures.lua`, including map, save ID, XYZ, UIDName, and GroupID fields used by the renderer and override system.

## Boss provider

`BossDataProvider` extracts the local field-boss data and generates exactly nine world-boss records in `data/generated/bosses.lua`. Runtime availability is derived separately from save data (`tb_actor_respawn`).

## Adding another layer

1. Add an `IDataProvider` under `src/installer/Core/Providers`.
2. Extract or scan the required local game data.
3. Write a deterministic file under `data/generated`.
4. Return a `DataSetResult` with record count and source metadata.
5. Register the provider in `InstallationPipeline`.
6. Add an isolated UE4SS producer module and Overlay renderer/state module.

Generated game data is local-only. The PAK AES key is detected from the user's executable and Oodle-compatible decompression uses the bundled `vendor/ooz/ooz.exe`.
