# Project Context

## Purpose

Evaluate the medium native architecture for DragonSword Radar without changing the current production-development Radar.

## Ownership boundary

- C++ owns player-coordinate sampling, activation, transition epochs, cooldown, and protocol publication.
- The external WinForms Overlay owns rendering, save-state filtering, and marker catalogs.
- The current `DragonSwordWorldRadar` directory and installed Mod are read-only inputs only; they are not deployment targets for this project.

## Acceptance state

- Source policy: pending/verified by `tools/Verify-Source.ps1`.
- Native compilation: must use the pinned RE-UE4SS v3.0.1 toolchain.
- Overlay compilation and protocol compatibility: verified by project scripts.
- Gameplay stability, visual correctness, and performance: not validated until explicitly deployed and tested.

## Intentional exclusions

- No continuous UObject scan.
- No UObject create/delete listener.
- No actor Tick hook.
- No LoadMap hook.
- No retained Controller or Pawn pointer.
- No world-map projection or live time/weather provider in version 0.1.0.
