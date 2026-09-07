# One-click installer

`DragonSwordNativeAutoPickup-Setup-1.3.1.exe` is a .NET Framework 4.8 WinForms
installer. It embeds the 1.3.1 ExperimentalNested native plugin, public
configuration, passive Lua entry point, the exact tested UE4SS compatibility
runtime, and optional 3x, 5x, 10x, 15x, and 20x range PAKs.

The current 1.3.1 installer preserves the corrective single-pending, 750 ms
confirmation-window, expiring-backoff, true-F9-edge, and dual-path runtime-
selector policies. It also embeds the balanced 15x/20x PAKs. Exact Setup and
archive hashes are recorded only after the canonical release build.
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
7. Review the detected state and choose Install, Update, Repair, or Uninstall.
8. Confirm the selected keys and range. Any required UE4SS conversion is shown
   in the same confirmation. Cancel leaves files unchanged and retains edits.

Update and Repair load the installed keys into editable selectors. Change
them there to reconfigure an existing installation; only those two key values
are edited in `config.ini`. All other settings remain intact. Unchanged valid
keys preserve the exact file bytes. A different game path reloads its own
settings. The shared interaction contract is documented in
`../../docs/DRAGONSWORD_MOD_INSTALLER_STANDARD.md`, section 2.1 (workspace root).
Pickup still owns its exact native ABI and runtime compatibility decisions.

Auto Pickup starts disabled after every game launch. Returning to the main menu
or initializing a new save or World disables it again. Press the configured
toggle after a playable World loads.

## Supported runtime and conversion policy

Version 1.3.1 supports one native ABI only:

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
- Update is exposed only for one recognized older owned installation.
- Repair is exposed for the current exact owned version. On the exact runtime,
  Update and Repair apply confirmed key edits while preserving all other
  `config.ini` contents, use temporary rollback data, and retain no persistent
  conversion backup. Keys must be standalone names; a fallback of AUTO and
  inline comments on key assignments are rejected, matching native parsing.
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

The 15x and 20x variants retain their full gather/animal range. Their 19
short-lived item-drop targets intentionally use the stable 10x overlap range.

Setup reconciles only the exact supported and legacy filenames after backup or
within an owned transaction. Historical hashes are not ownership gates; the
newly written embedded PAK is still hash-verified. Range PAKs modify authored
native interaction volumes independently of the Auto Pickup DLL. The native
Mod does not add or stack a range multiplier.

## Build and validation

The final 1.3.1 release produces exactly four public archives: installer, manual
without UE4SS, manual with UE4SS, and the standalone range PAK bundle. Static,
source, core, built-artifact, deterministic ZIP, exact-entry, and checksum gates
passed. Build and isolated installer tests verify the release marker, launch-Off
and forced-Off lifecycle, public
Debug-Off configuration, exact UE4SS and PAK hashes, PE architecture,
Install/Upgrade/Repair/Uninstall ownership states, rollback, and embedded resources.

The isolated installer state matrix passes 11/11 fixtures, including
owned install/upgrade/uninstall behavior, unknown same-name rejection,
confirmed conversion with Mod migration, and all six mutually exclusive range
states. The matrix includes an explicit recorded schema-2 1.3.0 to 1.3.1
Upgrade that preserves configuration, UE4SS, another Mod, and mods.txt while
replacing the same-name 15x PAK. Final archive/content checks pass in the
canonical release pipeline.

The exact preceding owned 1.3.0 DLL
`38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1`
is an explicit Repair input when its version and Lua hash also match. Modified
or foreign same-name files remain blocked with zero mutation.

These checks prove installer and package behavior only. They do not prove that
the exact 1.3.1 DLL resolves its selector capability or functions inside the
game, and they do not replace deployment or owner smoke-test acceptance.
