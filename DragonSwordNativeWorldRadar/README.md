# DragonSwordNativeWorldRadar

`DragonSwordNativeWorldRadar` is an isolated proof-of-concept for the medium native Radar architecture. It does not replace or modify `DragonSwordWorldRadar`.

## What this build proves

- A native UE4SS C++ provider can publish the current player position without a Lua polling loop.
- The existing external WinForms renderer can consume the same compact protocol from an independent runtime directory.
- Gameplay instances are resolved fresh for each 250 ms sample and are never retained across samples.
- World transitions fail closed, increment an epoch, hide the Overlay, and resume only after cooldown plus a valid location sample.
- The provider performs one bounded `FindFirstOf("Engine")` at initialization and no repeated UObject-array search.

## Current scope

This first package supports the minimap Radar path: treasures, Bosses, Assault targets, Moles, height indicators, and existing save-state filtering. World-map projection and live game-time/weather collection remain intentionally disabled until the native provider passes gameplay stability and performance testing.

The validation hotkey is **F10**. It is separate from the current Radar's F7/F8 controls. `enabled_on_launch=false` is the safe default.

Do not enable this package at the same time as `DragonSwordWorldRadar`; both would draw Radar markers even though their processes and bridge files are isolated.

## Build and validation

The project pins the same RE-UE4SS v3.0.1 toolchain used by the native AutoPickup project. Run:

```powershell
.\tools\Verify-Source.ps1
.\tools\Verify-Overlay.ps1
.\tools\Verify-Protocol.ps1
.\tools\Build-Native.ps1 <pinned toolchain parameters>
.\tools\Stage-Package.ps1 -DllPath .\build-native\main.dll -OutputDirectory .\dist\DragonSwordNativeWorldRadar-Package
```

The generated Lua marker catalogs under `assets/data/generated/` are local,
install-derived inputs and are intentionally excluded from Git. Regenerate or
copy the current-game catalogs before staging; package validation fails closed
when a required catalog is absent.

Static validation and a clean package do not establish in-game acceptance. The required runtime sequence is documented in `docs/ACCEPTANCE_CHECKLIST.md`.
