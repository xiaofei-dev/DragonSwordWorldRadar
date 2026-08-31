# Manual installation for the tested ExperimentalNested UE4SS layout

This archive is the manual-install alternative to the one-click Setup program.
It is intentionally limited to the exact experimental UE4SS layout used by the
author during development.

## Supported environment

- DragonSword: Awakening for Windows with an executable named
  `DSClient-Win64-Shipping.exe`
- Reference tested game SHA-256, retained for diagnostics only:
  `85E0F6BAFF78940C53451A282559A9378F6541E24B1CC81CD8CAF1556204A52E`
- `Win64/ue4ss/UE4SS.dll` SHA-256:
  `F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1`
- `Win64/dwmapi.dll` SHA-256:
  `30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B`
- Active Mods directory: `Win64/ue4ss/Mods`

The game hash is diagnostic-only: it never selects or authorizes a selector
address. F9 activation still requires runtime compatibility resolution. The
reflected `Server_RunInteractV2` exec thunk must resolve through one virtual
slot and the interactable CDO to a bounded native implementation. The complete
reflected `SetInteractUIV2` exec wrapper must contain one unique terminal
`E8 rel32` call to its separately bounded native implementation. Both
implementations must identify the same selector address through their structural
contracts. A compatible relocated build may resolve, while a missing,
ambiguous, or inconsistent contract leaves Auto Pickup Off. There is no fixed-
RVA fallback and no guarantee for an arbitrary future recompile.

Do not use this manual package with a different UE4SS binary or Mods layout.
Use the one-click installer when conversion, exact layout checks, backup, and
migration are required.

## Install

1. Close DragonSword: Awakening completely.
2. Open the game directory ending in `DS/Binaries/Win64`.
3. Copy only `ue4ss/Mods/DragonSwordNativeAutoPickup` from the ZIP into
   `Win64/ue4ss/Mods`.
4. Open the controlling `Win64/ue4ss/Mods/mods.txt`.
5. If the file already exists, do not overwrite it. Preserve every existing
   line and add or replace exactly this line:

   ```text
   DragonSwordNativeAutoPickup : 1
   ```

6. If `mods.txt` does not exist, copy the included one-line
   `ue4ss/Mods/mods.txt` to that exact location.
7. Launch the game. Auto Pickup starts disabled. Press `F9` once to enable it
   and press `F9` again to disable it.

Returning to the main menu or initializing another save or World forces Auto
Pickup Off. Press the configured toggle again after the playable World loads.
Holding F9 does not repeat the transition; release is required before another
press is accepted. If an automatic action times out, the same exact returned
Component may be retried once after 100 ms only when the game selector presents
it again. A second timeout quarantines that Component for the current activation; other
candidates may still proceed. Use separate press/release cycles to go Off and
then On to clear the attempt records.

The archive includes a one-line `ue4ss/Mods/mods.txt` for installations that do
not already have one. Never copy it over an existing `mods.txt`; doing so would
remove the enablement entries for other installed Mods.

## Configuration

Edit:

`Win64/ue4ss/Mods/DragonSwordNativeAutoPickup/config.ini`

- `toggle_hotkey` changes the Auto Pickup toggle key.
- `interaction_key=AUTO` reads the saved semantic `INTERACT` keyboard binding
  once whenever AutoPickup is enabled.
- `interaction_key_fallback=F` is used only if that saved binding cannot be
  resolved. Change it to the exact Unreal key name currently mapped to
  interaction, such as `E`, `K`, or `Gamepad_FaceButton_Bottom`.
- A concrete `interaction_key` remains available as a troubleshooting override
  that bypasses AUTO.
- Public debug logging is disabled by default.

AUTO currently selects the saved keyboard binding. A concrete Unreal gamepad
key can be configured as an override when required; automatic selection of the
saved gamepad binding is not a current public guarantee.

The selector and Enhanced Input route is owner-accepted through the historical
1.6.10 baseline. Version 1.3.0 corrects the deployed 1.2.0 action-storm and F9
repeat policies and replaces the fixed selector RVA with fail-closed dual-
path runtime resolution. The current manual-without-UE4SS ZIP is 362,076 bytes /
`9E4A7177C2D28AB2C823B1FCBB92473445CDD21FE5FEB3CF9C8A88E91080274B`;
the current manual-with-UE4SS ZIP is 8,446,667 bytes /
`55BE6AA64FE035B0F9F76BBD6479E3F715AA1A5A35BA2B2B4C2B1F277E628C6D`.
Both contain the 919,552-byte 750 ms confirmation-window / 200 ms retry-
cooldown native DLL with SHA-256
`10F5F4D575C07FF90A7C692E6A91906B6EA5B01E50D1671F82CBB40E8DB174B2`.
Static, source, core, built-artifact, deterministic ZIP, exact-entry, and
checksum gates passed. A local 1.3.0 diagnostic installation later produced
in-process evidence, and the owner reported gameplay acceptance on 2026-08-31.
The installed DLL was not independently hash-matched to this manual archive.

## Optional interaction-range PAKs

Original, 3x, 5x, 10x, 15x, and 20x are the complete range-selection set in
the one-click installer. Manual Auto Pickup archives do not include range PAKs.
For manual installation, use the separately published
`DragonSwordPickupRangeExpansion-v1.3.0.zip` and install exactly one option at
a time under `DS/Content/Paks/~mods`. Range works independently of the Auto
Pickup toggle; 15x and 20x are aggressive options for separate testing in
dense areas.

## Uninstall

1. Close the game.
2. Remove only `Win64/ue4ss/Mods/DragonSwordNativeAutoPickup`.
3. Remove only the `DragonSwordNativeAutoPickup` line from the controlling
   `Win64/ue4ss/Mods/mods.txt`. Keep every other line; do not delete or replace
   the whole file.
4. Keep `Win64/dwmapi.dll`, the `Win64/ue4ss` runtime, and every unrelated Mod
   folder. Other installed Mods may use them.

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
