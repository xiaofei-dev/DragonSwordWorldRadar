# One-click installer

`DragonSwordNativeAutoPickup-Setup-1.3.0.exe` is a .NET Framework 4.8 WinForms
installer. It embeds the 1.3.0 ExperimentalNested native plugin, public
configuration, passive Lua entry point, the exact tested UE4SS compatibility
runtime, and optional 3x, 5x, 10x, 15x, and 20x range PAKs.

The current 1.3.0 installer embeds the corrective single-pending, 750 ms
confirmation-window, timeout-quarantine, true-F9-edge, and dual-path runtime-
selector policies. The corrected unsigned Setup is 13,001,728 bytes with
SHA-256
`2BF6109E93606175610374F08ED2D91E813043BF204736AA3260E23966F53997`;
its installer ZIP is 8,556,743 bytes with SHA-256
`70A498FCBE8C3F37E2ADFFBCA1509DCAB23065B7E521C426F80C76D14310C124`.
The isolated installer state matrix remains separate from selector runtime,
installed artifact identity, and owner acceptance. A local 1.3.0 diagnostic
installation produced runtime evidence and the owner reported gameplay
acceptance on 2026-08-31; its installed DLL hash was not independently recorded.
The installer is a single unsigned executable with no required sidecar files.
Windows must provide .NET Framework 4.8.

Owned installs use `INSTALL-RECORD.txt` ownership schema 2. Its stable product
ID and recorded DLL/Lua/notices hashes make the immediately preceding valid
installation self-describing, so future releases do not need a new manual DLL
allowlist entry. Immutable-file tampering and unknown same-name directories
remain fail-closed. Historical hash tuples exist only to migrate legacy schema-1
records once.

## User flow

1. Close DragonSword: Awakening.
2. Run Setup as administrator.
3. Select `DSClient-Win64-Shipping.exe` from `DS/Binaries/Win64`.
4. Choose a toggle key. F9 is the default.
5. Choose a concrete fallback interaction key. F is the default and is used
   only when automatic semantic `INTERACT` keyboard-binding resolution fails.
6. Choose Original, 3x, 5x, 10x, 15x, or 20x native interaction range.
7. Review the detected state and choose Install, Upgrade, Repair, or Uninstall.
8. Confirm a UE4SS conversion warning if conversion is required.

Auto Pickup starts disabled after every game launch. Returning to the main menu
or initializing a new save or World disables it again. Press the configured
toggle after a playable World loads.

## Supported runtime and conversion policy

Version 1.3.0 supports one native ABI only:

- UE4SS v3.0.1 Beta #0 commit `1c1a1497`;
- ExperimentalNested layout with `Win64/ue4ss/UE4SS.dll`;
- active Mods root `Win64/ue4ss/Mods`.

The selected executable must be named `DSClient-Win64-Shipping.exe`. Its hash
is recorded in diagnostics only; it never selects or authorizes a selector
address. After the exact UE4SS hash and loaded-path gate, the loaded PE32+ image
and x64 `.pdata`/`CHAININFO` bounds are validated. The reflected
`Server_RunInteractV2` exec thunk resolves through one virtual slot and the
interactable CDO. The complete reflected `SetInteractUIV2` exec wrapper must
contain one unique terminal `E8 rel32` call to its separately bounded native
implementation. Both implementations must satisfy their selector structural
contracts and identify the same selector address. Missing, ambiguous, or
inconsistent evidence leaves Auto Pickup Off. There is no fixed-RVA fallback.
Compatible code-contract relocation can resolve, but arbitrary recompiles are
not guaranteed.

If UE4SS is absent, Setup installs the tested runtime. If a different or mixed
layout exists, Setup requires confirmation and then:

1. copies the complete active Win64-relative UE4SS layout, logs, settings,
   Mods, and configuration into a unique backup directory;
2. verifies every copied backup file by SHA-256;
3. removes the old active UE4SS layout;
4. installs the tested ExperimentalNested runtime;
5. migrates unrelated Mods, their configuration, and `mods.txt`;
6. installs and enables Auto Pickup.

Backup-name collisions use `_2`, `_3`, and later suffixes. Caught installation
failures attempt transactional file restoration. Setup rejects reparse-point
paths and writes only inside the selected game tree.

## Installation-state actions

- Install is exposed when Auto Pickup is absent.
- Upgrade is exposed only for one recognized older owned installation.
- Repair is exposed for the current exact owned version. On the exact runtime,
  Upgrade and Repair preserve `config.ini`, use temporary rollback data, and
  retain no persistent conversion backup.
- Uninstall is exposed only for one recognized owned installation. It removes
  the Mod, its authoritative `mods.txt` entry, and approved owned range PAKs.
  It preserves UE4SS and unrelated Mods.

Unknown same-name Mod content fails closed. Exact supported range-PAK filenames
and the legacy canary filename are product-owned and may be replaced or removed
regardless of their prior hash. A stale inspection identity must be rejected
before mutation.

## Mod ownership

`mods.txt` is the only load authority. Setup preserves unrelated lines,
normalizes Auto Pickup to exactly one
`DragonSwordNativeAutoPickup : 1` entry, and removes the legacy `enabled.txt`
bypass.

Third-party Mods are migrated as user content but are never embedded in the
Auto Pickup release. `ZeroKarya_PartySwitch` is explicitly excluded from source
and distribution manifests.

## Optional range PAKs

Range selection is mutually exclusive:

- Original installs no expansion PAK;
- 3x installs `DS_PickupRangeX3_P.pak`;
- 5x installs `DS_PickupRangeX5_P.pak`;
- 10x installs `DS_PickupRangeX10_P.pak`;
- 15x installs `DS_PickupRangeX15_P.pak`;
- 20x installs `DS_PickupRangeX20_P.pak`.

Setup reconciles only the exact supported and legacy filenames after backup or
within an owned transaction. Historical hashes are not ownership gates; the
newly written embedded PAK is still hash-verified. Range PAKs modify authored
native interaction volumes independently of the Auto Pickup DLL. The native
Mod does not add or stack a range multiplier.

## Build and validation

The final 1.3.0 release produced exactly four public archives: installer, manual
without UE4SS, manual with UE4SS, and the standalone range PAK bundle. Static,
source, core, built-artifact, deterministic ZIP, exact-entry, and checksum gates
passed. Build and isolated installer tests verify the release marker, launch-Off
and forced-Off lifecycle, public
Debug-Off configuration, exact UE4SS and PAK hashes, PE architecture,
Install/Upgrade/Repair/Uninstall ownership states, rollback, and embedded resources.

The isolated installer state matrix passes 10/10 fixtures, including
owned install/upgrade/uninstall behavior, unknown same-name rejection,
confirmed conversion with Mod migration, and all six mutually exclusive range
states. The tenth fixture verifies stable numeric suffixes for conversion-backup
name collisions. Final archive/content checks pass in the canonical release
pipeline.

The current native DLL is 919,552 bytes /
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`.
The manual-without-UE4SS, manual-with-UE4SS, and range ZIPs are 362,076 /
`9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`,
8,446,667 / `55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`,
and 3,919,600 /
`504C1E9524CDE63B096C88E24DBA0D5E008F9076BA24B6BC6065D60780848801`
bytes / SHA-256 respectively.

The exact preceding owned 1.3.0 DLL
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
is an explicit Repair input when its version and Lua hash also match. Modified
or foreign same-name files remain blocked with zero mutation.

These checks prove installer and package behavior only. They do not prove that
the exact 1.3.0 DLL resolves its selector capability or functions inside the
game, and they do not replace deployment or owner smoke-test acceptance.
