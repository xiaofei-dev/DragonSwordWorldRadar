# Installation

## Recommended one-click installer

1. Close DragonSword: Awakening.
2. Extract `DragonSwordAutoPickup-v1.3.1-Installer.zip`.
3. Run `DragonSwordNativeAutoPickup-Setup-1.3.1.exe`.
4. Confirm the automatically detected `DSClient-Win64-Shipping.exe`, or use
   Browse if Steam discovery is unavailable.
5. Select the fallback interaction key and optional range.
6. Review and confirm any UE4SS conversion warning.
7. Select the action offered for the inspected installation: Install, Upgrade,
   Repair, or Uninstall.
8. After Install, Upgrade, or Repair, launch the game and press F9 to enable
   Auto Pickup.

Auto Pickup starts Off. Returning to the main menu or loading a different save
or World disables it again; press the configured toggle after the playable
World finishes loading.

One physical toggle-key press causes one transition even when the key is held.
The record is armed before injection and remains live after the injection call
returns until matching dispatch, existing exact confirmation, timeout, or reset.
A matching exact `Server_RunInteractV2` post-dispatch releases the global in-
flight slot and starts a 750 ms re-entry delay for that Component.
This is dispatch evidence only, not a claim that the selected target was picked
up. If neither dispatch nor exact confirmation arrives within the 750 ms
fallback window, the game
selector may present the same Component for one retry after 200 ms. A second
no-dispatch result applies a 1500 ms self-expiring backoff; it does not require
an Off/On cycle to recover. Manual interaction remains available throughout.

## Exact release package identity

The following exact-Component/owner-settle artifacts use a 750 ms fallback
window and 200 ms retry delay, but predate the dispatch-observer repair. They
remain historical package identities, together with the structured drop PAKs
and filename-owned range replacement policy:

- native DLL: 919,552 bytes /
  `10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`;
- unsigned Setup: 13,001,728 bytes /
  `2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`;
- installer ZIP: 8,556,743 bytes /
  `70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`;
- manual without UE4SS ZIP: 362,076 bytes /
  `9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`;
- manual with UE4SS ZIP: 8,446,667 bytes /
  `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`;
- standalone range ZIP: 3,919,600 bytes /
  `504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`.

Matching these values proves only the preceding package identity, not the new
candidate. The observer repair remains `RUNTIME_PENDING`; its exact artifact,
deployment, in-process dispatch behavior, gameplay, and owner smoke testing
remain pending.

## Installer actions

- **Install** is available when Auto Pickup is absent.
- **Upgrade** is available only for one recognized owned Auto Pickup
  installation. It updates the Mod in place and preserves `config.ini`.
- **Repair** is available for the current recognized owned Auto Pickup version.
  It refreshes installer-owned files in place and preserves `config.ini`.
- **Uninstall** is available only for one recognized owned installation. It
  removes the Mod, its authoritative `mods.txt` entry, and approved owned range
  PAKs. It preserves UE4SS and unrelated Mods.

An exact-runtime Upgrade uses temporary transactional rollback data and does
not retain a persistent conversion backup. Unknown same-name Mod directories
block mutation instead of being overwritten or deleted. Exact supported
range-PAK filenames and the legacy canary filename are product-owned and are
replaced or removed without a historical hash allowlist; the newly written
embedded PAK is still hash-verified.

New installations write ownership schema 2 into `INSTALL-RECORD.txt`. The
record binds the stable product ID and exact installed DLL, Lua, and notices
hashes. Future installers validate that self-contained record rather than
requiring every prior DLL hash to be manually copied into a new release.
Changing a recorded immutable file without updating it through Setup still
fails closed. Legacy schema-1 installs use the finite historical allowlist only
for one migration Repair, which rewrites the record as schema 2.

New installations write ownership schema 2 into `INSTALL-RECORD.txt`. The
record binds the stable product ID and exact installed DLL, Lua, and notices
hashes. Future installers validate that self-contained record rather than
requiring every prior DLL hash to be manually copied into a new release.
Changing a recorded immutable file without updating it through Setup still
fails closed. Legacy schema-1 installs use the finite historical allowlist only
for one migration Repair, which rewrites the record as schema 2.

The immediately preceding owned 1.3.0 DLL
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
is an explicit Repair input when its version, DLL, and Lua hashes all match.
The first 200 ms Setup omitted that historical contract and must not be used;
the corrected Setup identity is the one listed above. Unknown or modified
same-name payloads remain blocked with zero mutation.

## UE4SS conversion

The installer supports the tested UE4SS v3.0.1 Beta #0 commit `1c1a1497`
ExperimentalNested runtime. When another or mixed UE4SS installation exists,
Setup performs the following transaction after explicit confirmation:

1. Copies the active Win64 UE4SS files, settings, logs, Mods, and configuration
   into a timestamped `UE4SS-<version>-<date>-Backup` directory.
2. Verifies every copied backup file by SHA-256.
3. Removes the old active UE4SS layout.
4. Installs the tested ExperimentalNested runtime.
5. Migrates unrelated Mods, `mods.txt`, and Mod configuration.
6. Installs and enables DragonSwordNativeAutoPickup.

If a backup directory already exists, Setup uses `_2`, `_3`, and later
suffixes. A failed transaction attempts to restore every recorded change.
The selected game executable hash is logged for diagnostics only and never
blocks installation.

The hash also never selects a native selector address. After the exact UE4SS
hash and loaded-path gate, the reflected `Server_RunInteractV2` exec thunk must
resolve through one virtual slot and the interactable CDO to a bounded native
implementation. The complete reflected `SetInteractUIV2` exec wrapper must
contain one unique terminal `E8 rel32` call to its separately bounded native
implementation. Both implementations must expose their structural selector
contracts and identify the same selector address. A compatible relocated build
may resolve; a missing, ambiguous, or inconsistent contract leaves Auto Pickup
Off. There is no fixed-RVA fallback and no guarantee for an arbitrary future
recompile.

## Manual without UE4SS

Use `DragonSwordAutoPickup-v1.3.1-Manual-No-UE4SS.zip` only when the exact
compatible ExperimentalNested runtime is already installed.

Copy only its `ue4ss/Mods/DragonSwordNativeAutoPickup` folder into the active
`Win64/ue4ss/Mods` directory, then add this line to the active
`ue4ss/Mods/mods.txt`:

```text
DragonSwordNativeAutoPickup : 1
```

The package does not include UE4SS or range PAKs. It includes a one-line
`ue4ss/Mods/mods.txt` only for installations where that file is absent. Never
overwrite an existing `mods.txt`; preserve all other Mod entries and merge the
Auto Pickup line.

## Manual with UE4SS

`DragonSwordAutoPickup-v1.3.1-Manual-With-UE4SS.zip` contains the complete
tested runtime and enabled Auto Pickup Mod. Close the game and extract the ZIP
directly into the directory containing `DSClient-Win64-Shipping.exe`.

Back up an existing UE4SS installation before using this direct-paste package.
Use the one-click installer when automatic backup and migration are required.

## Optional interaction range

Setup offers Original, 3x, 5x, 10x, 15x, and 20x range. Original installs no
range PAK. Install only one expanded variant. Range PAKs work independently of
Auto Pickup and can be changed or removed without changing the native Mod.
The 15x and 20x variants are aggressive and may increase prompt competition in
dense areas.

Each expanded PAK contains 69 reviewed targets: 50 gather/animal interaction
capsules and 19 class-proven type-7 monster-drop overlap spheres. This includes
ordinary meat, aged meat, coins, nuts, crystals, minerals, grain, and the other
reviewed F-pickable drops. Treasure/type-4 assets are excluded. The native Mod
does not multiply drop range at runtime.

For manual installation, use
`DragonSwordPickupRangeExpansion-v1.3.1.zip` from the same release directory.
The 15x and 20x choices retain their full gather/animal range while their 19
short-lived item-drop targets use the stable 10x overlap range.
It contains all five expanded choices plus its own step-by-step README and
checksums.

## Removal

Preferred: close the game, run the 1.3.1 installer, select the game executable,
and choose **Uninstall**.

Manual removal: close the game, remove
`Win64/ue4ss/Mods/DragonSwordNativeAutoPickup`, then remove only the line for
`DragonSwordNativeAutoPickup` from `Win64/ue4ss/Mods/mods.txt`. Preserve the
whole UE4SS runtime, `Win64/dwmapi.dll`, every unrelated Mod folder, and every
other line in `mods.txt`; other Mods may depend on them.

To remove a separately installed range PAK, close the game and open
`DS/Content/Paks/~mods`. Delete only the one file that was installed:

- `DS_PickupRangeX3_P.pak`
- `DS_PickupRangeX5_P.pak`
- `DS_PickupRangeX10_P.pak`
- `DS_PickupRangeX15_P.pak`
- `DS_PickupRangeX20_P.pak`

Keep the `~mods` folder and every other `.pak` file. Removing the selected file
restores the original interaction range after the next game start. If ownership
is uncertain, leave the file in place and use the one-click installer to check
it.
