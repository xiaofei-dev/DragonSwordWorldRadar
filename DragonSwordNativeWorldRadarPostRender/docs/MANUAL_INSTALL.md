# DragonSword Native World Radar 2.2.1 - Manual Installation

The two manual packages target only ExperimentalNested UE4SS. They contain no
CMD, BAT, PowerShell, or executable installer and perform no automatic path,
version, layout, ownership, backup, merge, rollback, or uninstall operation.

## Manual installation without UE4SS

Use `DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-No-UE4SS.zip` only
when the tested ExperimentalNested UE4SS runtime is already installed.

1. Close DragonSword Awakening.
2. Open the game folder ending in `DS\Binaries\Win64`. It contains
   `DSClient-Win64-Shipping.exe`.
3. Confirm that `Win64\ue4ss\UE4SS.dll` already exists.
4. From this ZIP, copy only
   `ue4ss\Mods\DragonSwordNativeWorldRadarPostRender` into
   `Win64\ue4ss\Mods\`.
5. Enable the Mod in `Win64\ue4ss\Mods\mods.txt`:
   - If that file does not exist, copy the included `ue4ss\Mods\mods.txt` to
     that exact location.
   - If it already exists, especially when other Mods are installed, do not
     overwrite it. Keep every existing line and add or replace exactly one
     Radar line:

     ```text
     DragonSwordNativeWorldRadarPostRender : 1
     ```

6. Launch the game, load a playable open world, and press F7 once.

Never replace an existing `mods.txt` with the one-line file from this ZIP. That
would remove the enablement entries for other installed Mods.

## Manual installation with UE4SS

Use
`DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`
for a clean target with no existing UE4SS. It contains the tested
ExperimentalNested UE4SS v3.0.1 Beta #0 commit `1c1a1497`.

### Clean installation with no existing UE4SS

1. Close DragonSword Awakening.
2. Open the game folder ending in `DS\Binaries\Win64`. It contains
   `DSClient-Win64-Shipping.exe`.
3. Extract every file and folder from this ZIP directly into that `Win64`
   folder.
4. Launch the game, load a playable open world, and press F7 once.

### Existing UE4SS or other installed Mods

1. Close the game and back up the existing `Win64\ue4ss` folder and
   `Win64\dwmapi.dll`.
2. Do not extract the whole ZIP over the existing installation. If the current
   runtime is already the exact supported ExperimentalNested version, copy
   only `ue4ss\Mods\DragonSwordNativeWorldRadarPostRender` into
   `Win64\ue4ss\Mods\`.
3. Open the existing `Win64\ue4ss\Mods\mods.txt`. Do not overwrite this file.
   Keep every line for other Mods and add or replace exactly one line:

   ```text
   DragonSwordNativeWorldRadarPostRender : 1
   ```

4. Launch the game, load a playable open world, and press F7 once.

If the installed UE4SS version or layout is different or uncertain, use Setup
instead. Setup performs compatibility checks, transactional conversion,
load-control merge, and owned-Mod update or repair.

## Update and user configuration

Use Setup Update / Repair for an existing Radar installation. It refreshes the
binary and bundled catalogs while preserving validated user copies of:

- `config\visibility.ini`
- `config\diagnostics.ini`
- `data\defaults\treasure_overrides.txt`

If a manual update is unavoidable, back up those files and the active
`mods.txt`, copy only the new Mod folder, restore the user files, and confirm
that exactly one enabled Radar line remains.

The 2.2.x visibility file also stores the three compact-only height-arrow
switches and the UI language preference. Clean defaults enable Treasure, Area
Quest, and Mole height guidance. A valid existing configuration keeps its
choices. The selector contains only 11 explicit languages. A legacy AUTO value
migrates on the next actual F6 opening or F7 activation through
`DGameUserSettings.LanguageText`, Kismet, and English to one persisted explicit
language; AUTO is not displayed. A saved explicit language remains authoritative.

F6 remains available while Radar is Off, On, or Faulted. The status action is
Enable, Disable, or Retry; Enable still requires a loaded playable world. The
Bug Report action opens the fixed Nexus Posts page.

## Remove the Radar

1. Close DragonSword Awakening.
2. Delete only
   `Win64\ue4ss\Mods\DragonSwordNativeWorldRadarPostRender`.
3. Open `Win64\ue4ss\Mods\mods.txt`.
4. Remove only the line for `DragonSwordNativeWorldRadarPostRender`. Keep every
   line for other Mods. Do not delete or replace the whole `mods.txt`.
5. Keep `Win64\dwmapi.dll`, the `Win64\ue4ss` runtime, and unrelated Mod
   folders. Other installed Mods may use them.

Neither manual package supports StableRoot. Static archive tests prove package
identity and copy topology only; they do not prove fresh in-game marker
behavior or frame time.
