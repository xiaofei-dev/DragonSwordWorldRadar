# Workspace Layout

## Authoritative projects

- Auto Pickup source: `G:\my_projects\game_mods\DragonSword\DragonSwordNativeAutoPickup`
- Range PAK source and reviewed assets:
  `G:\my_projects\game_mods\DragonSword\DragonSwordPickupRangeExpansion`
- Pinned third-party build dependencies: `DragonSwordNativeAutoPickup\.sdk`

No `DragonSwordOpenSource` copy, alternate source checkout, or timestamped
project directory is part of the release workflow.

## Generated work

All disposable Auto Pickup build state lives under `out/`:

- `out/core`
- `out/native/ExperimentalNested`
- `out/installer/1.3.0`
- `out/tests/installer`
- `out/staging`

These directories may be deleted whenever no build or Setup process is using
them. Build scripts must not create timestamped or suffixed build roots.

The Range PAK project uses its ignored `runtime/`, `staging/`, and `dist/`
directories. `runtime/source` is the retained exact source-asset collection;
other Range runtime children are disposable.

## Immutable local release

The only publishable local output for version 1.3.0 is:

`DragonSwordNativeAutoPickup\dist\releases\1.3.0`

It contains exactly the installer ZIP, manual-without-UE4SS ZIP,
manual-with-UE4SS ZIP, standalone range ZIP, and release JSON. Do not publish
files from `out/`, the Range project's individual PAK `dist/`, or an installed
game directory.

## Installed game layout

The only supported installed Mod directory is:

`DS\Binaries\Win64\ue4ss\Mods\DragonSwordNativeAutoPickup`

The authoritative load entry is the single `DragonSwordNativeAutoPickup : 1`
line in `Win64\ue4ss\Mods\mods.txt`. Optional range uses at most one exact
`DS_PickupRangeX{3,5,10,15,20}_P.pak` under `DS\Content\Paks\~mods`.

Setup is the sole deployment, Repair, range-switching, and Uninstall authority.
Direct-copy deployment scripts and installed-directory files are not release
sources.
