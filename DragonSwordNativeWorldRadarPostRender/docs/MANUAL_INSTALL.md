# DragonSword Native World Radar - Manual Installation

This guide accompanies the two Native World Radar 3.0.0 manual archives.
Verify the included checksums before copying files. Exact local release evidence
and historical archive identities are recorded in the source repository's
`docs/RELEASE_STATUS.md`.

The two manual packages target only ExperimentalNested UE4SS. They contain no
CMD, BAT, PowerShell, or executable installer and perform no automatic path,
version, layout, ownership, backup, merge, rollback, or uninstall operation.

## Manual installation without UE4SS

Use `DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-No-UE4SS.zip` only
when a compatible, structurally complete ExperimentalNested UE4SS runtime is
already installed. If uncertain, use Setup for inspection and conversion.

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

6. Launch the game, load your save in an open-world area, wait until you can
   control the character, and press the Enable key (default F7).

Never replace an existing `mods.txt` with the one-line file from this ZIP. That
would remove the enablement entries for other installed Mods.

## Manual installation with UE4SS

Use
`DragonSwordNativeWorldRadarPostRender-v3.0.0-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip`
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
   For an existing Radar installation, also preserve the user files listed in
   the next section before copying. Setup Update / Repair is preferred.
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
binary and bundled catalogs while preserving validated user copies of the
following files, except for hotkeys explicitly changed and confirmed in Setup:

- `config\visibility.ini`
- `config\hotkeys.ini`
- `config\diagnostics.ini`
- `data\defaults\treasure_overrides.txt`

If a manual update is unavoidable, back up those files and the active
`mods.txt`, copy only the new Mod folder, restore the user files, and confirm
that exactly one enabled Radar line remains.

The visibility file also stores five compact-only height controls, independent
Scene switches, Scene range/count/distance settings and the UI language preference.
Clean defaults enable Treasure, Area Quest, Mini-games, Boss and Assault height
guidance. All three Scene categories default On with 600 m / 24 and Auto focus labels.
Valid existing choices survive migration; missing new settings take these defaults.
The selector contains 11 explicit languages and Use game language. Automatic
language detection refreshes when Settings opens or Enable is pressed,
retaining the last valid detection on failure. A saved manual choice stays fixed.

Esc cancels an active confirmation. Otherwise it closes the complete settings
page, including its language popup or focused slider, and keeps the last values.
The same press's repeat/release
messages remain consumed; release Esc and press again for normal game behavior.

### Custom keys

Prefer the Setup package if you want a graphical key selector: close the game,
run Setup, change Settings / Enable / Disable keys, then confirm **Update** or
**Repair**. Existing bindings are loaded and other user settings are preserved.

Close the game and edit
`DS\Binaries\Win64\ue4ss\Mods\DragonSwordNativeWorldRadarPostRender\config\hotkeys.ini`:

```ini
[hotkeys]
settings_hotkey=INSERT
enable_hotkey=HOME
disable_hotkey=PAGEUP
```

Restart to apply. Defaults remain F6/F7/F8; instructions here refer to those
default actions. Use three distinct keys from F1-F24, A-Z, 0-9, NUM0-NUM9,
HOME, END, PAGEUP, PAGEDOWN, INSERT, DELETE, SPACE (case-insensitive). Modifier
combinations are unsupported. Avoid keys already used by the game or another
Mod. Invalid/missing runtime configuration uses all defaults. F6 settings never
overwrite this file. Back it up before manually copying an update.

F6 remains available while Radar is Off, On, or Faulted. The status action is
Enable, Disable, or Retry; Enable still requires a loaded playable world.
Guide beside Close explains treasure colors, Radar/Map icons, height arrows
and distance modes in all eleven languages. Guide and Settings retain separate
scroll positions while the menu is open. The header and footer remain visible
when the body scrolls on shorter displays.

The All row changes the supported Radar or Map categories together. Map changes
refresh an already-open world map. Mini-games includes flying, marmot and wave
activities; green chests mark mini-game rewards. Reset to defaults also selects
Auto focus; updates preserve your saved distance mode.

Reset to defaults, Vote for this mod and Feedback sit in the bottom row and ask
for confirmation. Reset restores display settings, preserving module power and
startup hotkeys. Vote opens the Mod's Nexus page for Mod of the Month voting;
Feedback opens Posts. Complete the vote or post on Nexus.

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
