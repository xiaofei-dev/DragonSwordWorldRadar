# Project Context

## Purpose

`DragonSwordPickupRangeExpansion` is the optional, removable resource layer for
native interaction range. Its production release provides 3x, 5x, 10x, 15x,
and 20x choices and is architecturally separate from `DragonSwordNativeAutoPickup`.

## Current state

- Artifacts: `DS_PickupRangeX3_P.pak`, `DS_PickupRangeX5_P.pak`,
  `DS_PickupRangeX10_P.pak`, `DS_PickupRangeX15_P.pak`, and
  `DS_PickupRangeX20_P.pak`; install exactly one.
- Reviewed targets: 69 package pairs / 138 PAK entries.
- Coverage: 42 explicit `NormalGather` packages, 3 production
  `DsAnimationProp` gather packages with an inherited interaction contract,
  and 5 `/Script/DS.DsInteractableAnimal` packages.
- Structured drop coverage: 19 class-proven type-7 packages, changing only
  `SphereOverlapComp.RelativeScale3D`.
- Excluded from PAK patching: every treasure/type-4 package, visual assets,
  root/movement collision, and drop-item physics/hit components.
- Historical artifact: `DS_PickupRangeX3Canary_P.pak` is preserved byte-for-byte
  as development evidence and is not a production choice.
- Static state: all five production variants are built and independently
  unpacked; x5 also retains its earlier semantic-reparse evidence.
- Bundle state: a deterministic standalone ZIP contains the five production
  choices plus an English README.
- Deployment state: no deployment is performed by the multi-variant build.
- Runtime state: `NOT_VALIDATED` until the owner tests native `F` with `F9` Off.

## Contracts

`metadata/targets-x5.json` is the legacy-named reviewed source and exclusion
inventory. Its 5x values remain an immutable review baseline; production 3x,
5x, 10x, 15x, and 20x values are derived from each recorded original scale.
`metadata/variants.json` defines the supported multiplier policy and pins the
historical canary hashes. `build/Build-Release.ps1` verifies exact source hashes,
applies one dedicated capsule-scale patch per target, enforces exact PAK
inventory, performs a second content-equivalent build, and verifies independent
unpacked bytes. `build/Build-VariantBundle.ps1` builds all five choices,
revalidates their inventories, and produces a deterministic standalone ZIP.
`build/Verify-Semantics.ps1` accepts an explicit supported multiplier and
validates the patched component scale, class contract, target count, and zero
treasure content when semantic exports are available.
`build/Deploy-Local.ps1` refuses deployment while the game is running, rejects
conflicting installed PAKs, backs up/removes x3, installs x5, and verifies the
installed hash and entry set.

## Cross-project behavior

The selected PAK changes native prompt range for 50 authored gather/animal
targets plus 19 type-7 drop targets without Auto Pickup. The production build
uses a pinned structured asset writer and exact UE5.3 mapping file to add only
`SphereOverlapComp.RelativeScale3D` to the drop packages. Round-trip binary
equality, exact source hashes, and protected physics/hit components are gated
for every target. Auto Pickup 1.3.0 does not multiply range at runtime; the PAK
is the sole owner and works independently.

## Safety boundary

Never modify an original `pakchunk*.pak`, never ship extracted game assets in
Git, never patch a treasure asset, and never treat static/package validation as
gameplay acceptance.
