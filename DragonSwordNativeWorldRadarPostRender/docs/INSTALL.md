# Installation

## Release identity

Native World Radar 2.2.1 uses the ExperimentalNested UE4SS directory contract.
The game executable and existing UE4SS loader/proxy are validated structurally
rather than through fixed compatibility hashes, so a compatible game or UE4SS
update does not require rebuilding the Mod.

| File | SHA-256 |
| --- | --- |
| `ue4ss/UE4SS.dll` | `F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1` |
| `dwmapi.dll` | `30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B` |
| `ue4ss/DS-5.3.2-0+UE5-1c1a1497.usmap` | `0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47` |
| tested `ue4ss/UE4SS-settings.ini` | `4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74` |

The hashes above identify only the embedded bootstrap/conversion payload and
verify its packaging integrity. They are not an allowlist for an already
installed structurally complete ExperimentalNested loader or proxy. Runtime
reflection/schema gates remain fail closed when a feature is incompatible.

Setup still requires the exact selected
`DS/Binaries/Win64/DSClient-Win64-Shipping.exe` path and validates a bounded,
executable AMD64 PE32+ application image. Its current SHA-256 is recorded only
in the confirmed plan and install record. F7 performs a runtime/save
resynchronization; it is not an installer and does not rebuild static catalogs.

## Recommended installation

1. Close DragonSword Awakening.
2. Fully extract the installer archive.
3. Verify the Setup executable against its `.sha256` sidecar.
4. Run `DragonSwordNativeWorldRadarPostRender-Setup-2.2.1.exe`.
5. Select `DSClient-Win64-Shipping.exe` from `DS/Binaries/Win64`.
6. Setup inspects the selected path and automatically presents `Install`,
   `Update`, or `Repair`. `Uninstall` remains disabled unless an active Radar
   installation passes strict same-product ownership validation.
7. Review the read-only plan shown by Setup.
8. Confirm the plan only when the detected location and action are correct.
9. Record the backup path only when Setup performed a UE4SS conversion. A
   normal installation or Update / Repair intentionally retains no backup.
10. After success, Setup refreshes the detected state and remains open. Close it,
    start the game in an open-world area, and press F7.

Setup is unsigned and requests elevation because the game is commonly installed
under a protected library. It never launches or terminates the game.

## UE4SS behavior

Setup has three paths:

- **Structurally complete ExperimentalNested already installed:** keep its
  loader, proxy, settings, and active Mods paths without a loader/proxy hash
  comparison. Install Radar or automatically select Update / Repair for a
  recognized existing Radar.
- **UE4SS absent:** ask for confirmation, install the embedded hash-verified
  pinned Experimental runtime, then install Radar.
- **Another, incomplete, unknown, root, or dual UE4SS layout:** show a dedicated
  conversion warning. Only after confirmation, create and byte-verify a complete
  original-layout backup, migrate existing Mods and load-control state, remove
  the old active layout, and install the pinned Experimental runtime.

Conversion copies existing Mod files and Mod-owned configuration into
`DS/Binaries/Win64/ue4ss/Mods`. It preserves valid unrelated `mods.txt` entries
and normalizes the Radar entry. After the complete backup is verified and the
migration succeeds, the old active UE4SS and source Mods layouts are removed;
they remain only in the backup. If two source layouts contain different bytes
for the same relative Mod path, Setup stops before mutation rather than guessing.

Loader-level settings are not blindly carried between incompatible UE4SS
builds. Setup installs the tested Experimental settings. Replaced loader files
and settings are preserved in the transaction backup.

Third-party native DLL Mods may require builds compiled for the pinned
Experimental ABI. Setup preserves them as requested but cannot prove their ABI
compatibility. Review those Mods if the game fails during startup.

## Target layout

```text
DS/Binaries/Win64/
  DSClient-Win64-Shipping.exe
  dwmapi.dll
  ue4ss/
    UE4SS.dll
    UE4SS-settings.ini
    DS-5.3.2-0+UE5-1c1a1497.usmap
    Mods/
      mods.txt
      DragonSwordNativeWorldRadarPostRender/
```

The supported default load authority is
`ue4ss/Mods/mods.txt`. An exact already-compatible Experimental installation may
use its validated `ModsFolderPath` and `ControllingModsTxt` overrides. During
conversion, Setup uses the tested nested layout.

## Load control and conflicts

The final controlling file contains exactly one enabled entry:

```text
DragonSwordNativeWorldRadarPostRender : 1
```

Recognized predecessor entries owned by this project are removed. Unrelated
entries, comments, encoding, and line endings are preserved. Setup rejects an
active external `DragonSwordWorldRadar` renderer rather than deleting or
disabling it.

## Configuration and diagnostics

The installed user-owned configuration files are:

- `config/visibility.ini`
- `config/diagnostics.ini`
- `data/defaults/treasure_overrides.txt`

Recognized existing values are preserved byte-for-byte during an update after
strict syntax and size validation. All bundled generated catalogs are replaced
from the current Setup payload. Public diagnostics
default to disabled. To capture a diagnostic session, close the game, set
`debug_logging=true` in the `[diagnostics]` section of `diagnostics.ini`, then
restart. The file is read only at native startup and is not hot reloaded. The
exact legacy one-line `event_log_enabled=true|false` form remains accepted for
preserved older installations. The log is written under `runtime/logs` and is
never included in a release package.

Fresh visibility configuration uses readable `[radar]`, `[map]`, `[modes]`,
`[height_arrows]`, and `[interface]` sections. Height-arrow defaults are
Treasure ON, Area Quest ON, and Mole ON. A valid existing configuration keeps
its choices. The interface stores one of 11 explicit languages. A legacy AUTO
value migrates on the next actual F6 opening or F7 activation by resolving
`DGameUserSettings.LanguageText`, then Kismet and English, and persisting the
matching explicit language; AUTO is not displayed. Explicit choices persist and
remain authoritative. Strictly valid older formats remain upgrade-readable;
the next real F6 change atomically writes the current complete format. Setup
rejects malformed, oversized, duplicate, unknown, mixed, or incomplete content
before mutation.

F6 may open while Radar is Off, On, or Faulted. Bug Report and Close are
separate top-bar controls. Read-only status text uses a thin state-colored strip;
Enable, Disable, or Retry is a separate action that keeps the page open. Enable
still requires a loaded playable world, and Bug Report opens the fixed Nexus
Posts page. These runtime controls do not run Setup or
change installed files beyond the normal bounded visibility-config save.

## Transaction, backup, and rollback

Before mutation, Setup revalidates the confirmed plan. Every path uses a
bounded transaction journal for automatic rollback. Successful normal installs
and Update / Repair operations delete that temporary journal and retain no
persistent backup. A conversion backup is
created directly below `Win64` and named for the detected UE4SS layout and UTC
date, for example:

```text
DS/Binaries/Win64/UE4SS-v3.0.1-StableRoot-20260827-093816-606-Backup/
```

The contents begin at the original `Win64` level rather than recreating the
`DS/Binaries/Win64` parent chain:

```text
UE4SS-v3.0.1-StableRoot-<timestamp>-Backup/
  UE4SS.dll
  UE4SS-settings.ini
  UE4SS.log
  dwmapi.dll
  Mods/
  ue4ss/                 # present when the original layout used this folder
  CXXHeaderDump/         # present when it existed
```

That tree preserves the user's original active UE4SS folder structure, complete
Mods directories, `mods.txt`, Mod configuration, loader settings, logs, mapping,
and header dump. `COMPLETE-UE4SS-BACKUP.txt` lists the SHA-256-verified files.
Short-name rollback copies are stored under `.rollback` so long Mod-relative
paths do not exceed the Windows path limit. No old active layout is removed
until the complete snapshot passes verification.

Writes are journaled. The installed Experimental structure, mapping, Radar payload,
and load authority are verified before commit. A failure triggers reverse-order
rollback. The installer test gate includes an injected late-failure case that
must restore the original fixture tree exactly.

After verification and commit, Setup shows a success message, refreshes the
detected installation state, and remains open. A newly installed current release
therefore changes the main action to `Repair` and enables `Uninstall` only when
the active installation still passes strict ownership validation.

Do not delete backups until the converted environment and all retained Mods have
been accepted in game.

## Manual installation without UE4SS

Use `DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-No-UE4SS.zip` only
when a structurally compatible ExperimentalNested UE4SS runtime is already
installed. Fully extract the archive and close the game. Copy only
`ue4ss/Mods/DragonSwordNativeWorldRadarPostRender` into the existing
`DS/Binaries/Win64/ue4ss/Mods` directory. The archive does not contain a loader,
proxy, or settings file.

If the target `ue4ss/Mods/mods.txt` does not exist, copy the included one-line
file to that location. If it exists, never overwrite it; preserve every other
entry and merge exactly:

```text
DragonSwordNativeWorldRadarPostRender : 1
```

Preserve unrelated entries, comments, encoding, and line endings. Never replace
an existing active file with the packaged one-line file. This package performs no layout or
path-override detection; use Setup for an unknown or configured layout. It
never supports StableRoot.

## Manual installation with UE4SS

`DragonSwordNativeWorldRadarPostRender-v2.2.1-Manual-With-UE4SS-v3.0.1-Beta0-g1c1a1497.zip` contains
the same Mod payload plus the pinned integrity-verified ExperimentalNested
runtime and a clean enabled `mods.txt`. It is valid only when the target has no
existing UE4SS installation. Fully extract it, close the game, and copy
every file and folder from the ZIP root into the exact `DS/Binaries/Win64` directory.
Do not merge this package over another loader, settings file, or Mods tree.

Both manual archives are script-free and perform no structural detection,
version restriction, path derivation, load-control merge, Update / Repair,
ownership validation, automatic backup, rollback, or uninstall. Generic copy
updates can overwrite `config/visibility.ini`, `config/diagnostics.ini`, and
`data/defaults/treasure_overrides.txt`; use Setup Update / Repair for an
existing Radar. See `MANUAL_INSTALL.md` for the exact clean-copy instructions.
Neither manual channel contains or restores a StableRoot payload. Both ship
fresh diagnostics disabled.

## Uninstall

Close the game, run Setup, select the exact game executable, and click
`Uninstall`. The button is disabled unless Setup detects a structurally complete
ExperimentalNested layout and proves strict ownership of the installed Radar.
When enabled, it offers a confirmed
transaction. The transaction removes only the exact
`DragonSwordNativeWorldRadarPostRender` directory and at most one valid Radar
entry from the controlling `mods.txt`. It preserves UE4SS, unrelated Mods,
`DS/Saved`, encoding, and line endings. A failure restores every recorded
mutation; a successful uninstall retains no backup.

If Setup cannot prove ownership, stop. The manual fallback is to back up the
exact product directory and controlling `mods.txt`, verify the top-level release
metadata and package manifest, then remove only that owned directory and its
single normalized load-control entry. Never remove UE4SS, unrelated Mods, or
game saves as part of Radar removal.

## Acceptance boundary

Successful compilation, package hashing, isolated installation, or rollback
testing proves only those stages. Fresh in-game startup, F7 activation, world
transition, map rendering, physical-controller behavior, all three height
controls, the responsive F6 status/actions/Bug Report page, all 11 language
glyph sets, and owner-observed
performance remain separate runtime acceptance requirements for the exact
rebuilt installer. Refreshed Core `2/2`, all static gates, release hygiene, and
the clean native `/W4 /WX` build passed for DLL
`6AEFDACC1A44EF6F387456CB31FE1A6828259EF7ACDEE1D1FE13BAE10BDFA4D5`,
bound to compiled-source SHA-256
`A98660932CC5DA1BC3E2B9D262DBFC13E0FED3935A6174B5C92E8622BBA2A1EC`.
`Build-Release.ps1` package validation passed for that exact DLL: Setup reports
  `20/20`, the manual-copy matrix reports `2/2`, payload equivalence, manual
  layout, and clean-target policy validation pass, and all three public ZIPs
  re-extract byte-identically. Local diagnostics-enabled deployment of exact
  DLL `6AEFDACC...` passed with matching source, build, and installed hashes.
  Its rollback backup is
  `dist/work/deployment/deploy-backups/20260903-003509-033-native-only-deploy`.
  The prior backup
  `dist/work/deployment/deploy-backups/20260902-234105-640-native-only-deploy`.
  belongs to the superseded intermediate 59529B2A deployment and is not current
  candidate evidence. This does not establish Setup ownership. The earlier 634D283A deployment and
  backup are historical only.

The packaged game-1.0.11 owner RVA and member offset `0x128` are fast paths,
not version locks. One FullActivation shares a total budget of at most 24
active-`.db` key validations across packaged and structural owner routes. If
the packaged candidates fail, FullActivation scans the current executable at
most once for exactly one retained structural signature. That scan counts only
targets inside the mapped image and scans executable sections through
`min(SizeOfRawData, VirtualSize)`. Structurally incompatible updates fail
closed; compatibility with every future version is not promised. Current F6
source uses measured desired-size evidence for exact font-layout TextBlocks
after first open, language changes, status changes, and language-popup display.
`GetDesiredSize` must return the known `Vector2D` structure identity. Invalid
evidence and the render-scale fallback keep authored text geometry, and this
pass changes no button hit box, map geometry,
or per-frame path. Live F6 visual behavior, gameplay, controller handling,
clean exit, and performance remain `NOT_VALIDATED` for 6AEFDACC.
Historical 2.2.0 and 2.1.1 evidence is not 2.2.1 evidence. Exact-artifact 2.2.1
gameplay acceptance remains pending.
