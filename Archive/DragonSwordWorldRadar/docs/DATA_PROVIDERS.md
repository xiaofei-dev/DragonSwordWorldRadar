# Data provider framework

Local game-data generation is implemented through `DragonSwordWorldRadar.Installer.IDataProvider`.

```csharp
public interface IDataProvider
{
    string Id { get; }
    DataSetResult Generate(InstallationContext context);
}
```

`InstallationPipeline` registers four providers.

## Treasure provider

`TreasureDataProvider` extracts `SectionTreasureBoxData.xml` and generates `data/generated/treasures.lua`, including map, save ID, XYZ, UIDName, and GroupID fields used by the renderer and override system.

## Boss provider

`BossDataProvider` extracts the local field-boss data and generates exactly nine world-boss records in `data/generated/bosses.lua`. At Overlay startup these records are combined with the validated Assault records into one immutable 49-record encounter target set for `tb_actor_respawn`.

## Mole/Fly provider

`MoleDataProvider` extracts and cross-validates exactly 34 `MiniGame_Fly` records (`11001-11034`) and generates `data/generated/moles.lua`. Stable IDs and mask bits are installer-owned; runtime completion is read separately through the bounded, fail-closed `mole_completion.lua` provider.

## Assault provider

`AssaultDataProvider` extracts the current PAK's UnexpectedMission place/kind, SectionMonster, and RevealCycle tables. It validates exactly 40 map-100 targets and exact Place/CID/UID/UIDName joins. `metadata/assault-inference-policy.xml` is a versioned, Radar-fingerprint-pinned condition input: each selector must resolve exactly against the fresh target set and each cycle must resolve against the freshly extracted cycle table. Confirmed cycle rows and inferred target bindings retain distinct provenance. The generated optional condition is attached to the same unified encounter model used by Bosses. A policy fingerprint, identity, condition type, hour, or provenance mismatch fails installation rather than guessing; unsupported weather semantics remain excluded and fail closed.

The install writes the locked pre-generation fingerprint into each dataset entry and generated Assault record, recomputes identity after generation, and rejects any pre/post drift before publishing `datasets.json`.

## Runtime-only world time

Production world time is unavailable. The retained research provider is not imported or called because both periodic and delayed one-shot `DGameSingleton.TimeOfDay` access reproduced an uncatchable native UE4SS crash. The renderer fails closed, conditioned Assault logic receives unavailable time, and weather remains unavailable and unqueried.

## Adding another layer

1. Add an `IDataProvider` under `src/installer/Core/Providers`.
2. Extract or scan the required local game data.
3. Write a deterministic file under `data/generated`.
4. Return a `DataSetResult` with record count and source metadata.
5. Register the provider in `InstallationPipeline`.
6. Add an isolated UE4SS producer module and Overlay renderer/state module.

Generated game data is local-only. The PAK AES key is detected from the user's executable and Oodle-compatible decompression uses the bundled `vendor/ooz/ooz.exe`.
## Save owner-pointer runtime binding

The installer scans the exact current `DSClient-Win64-Shipping.exe` with the existing owner-reference pattern while the normal executable/PAK fingerprint is locked before and after generation. Exactly one PE-bounded match is required. It writes `data/generated/save_owner_pointer.cfg` with schema version, complete game fingerprint, executable length, RVA, and provenance only. The SQLCipher key and absolute process addresses are never persisted. Runtime recomputes the same current fingerprint before trying this RVA, then retains the known-RVA candidates and one delayed pattern-scan fallback for update compatibility.
