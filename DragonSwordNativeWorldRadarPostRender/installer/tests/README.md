# Installer integration tests

`InstallerExperimentalConversion.Tests.ps1` is the isolated filesystem
acceptance matrix for the unsigned C# Setup executable. It loads Setup as an
assembly and drives the internal engine through reflection. It never opens the
WinForms UI, launches or terminates the game, deploys to the selected game, or
writes to any supplied fixture binary.

## Required inputs

The runner requires:

- the built Setup executable;
- a structurally valid DragonSword shipping executable fixture;
- a structurally valid ExperimentalNested `UE4SS.dll` fixture;
- a structurally valid root `dwmapi.dll` fixture; and
- a caller-owned disposable working directory.

The binaries are read-only fixture sources. Their size, hash, attributes, and
timestamps are checked before and after the matrix. Each case copies them into
a unique fake `DS/Binaries/Win64` tree and cleans that fixture afterward. Never
use the real game directory or a release staging directory as the working
directory.

```powershell
.\tools\Test-Installer.ps1 `
  -InstallerExe 'C:\candidate\DragonSwordNativeWorldRadarPostRender-Setup-2.2.1.exe' `
  -SupportedGameExecutable 'G:\fixtures\DSClient-Win64-Shipping.exe' `
  -ExperimentalUE4SSDll 'G:\fixtures\UE4SS.dll' `
  -ExperimentalDwmapiDll 'G:\fixtures\dwmapi.dll' `
  -WorkingDirectory 'G:\dsnwr-installer-tests'
```

The historical 2.2.0 runner reported 20 passed, 0 failed, and 0 skipped. The
manual-copy runner separately reports 2 passed, 0 failed, and 0 skipped.
Historical 2.1.1 results and hashes are not 2.2.0 evidence. The 2.2.1 installer
matrix remains pending until the new Setup executable is built and tested.

The fixture hashes do not form compatibility allowlists. Existing game,
loader, and proxy compatibility is based on the exact path/layout plus bounded
AMD64 PE32+ structural checks. Setup still verifies its embedded
bootstrap/conversion resources and Radar payload by SHA-256 because those
hashes prove Setup integrity, not the version of an existing installation.

## Required 20-case matrix

The checked-in matrix covers:

1. clean ExperimentalNested installation;
2. clean absence with embedded ExperimentalNested bootstrap;
3. root/incomplete layout conversion with complete retained backup;
4. existing Mod and `mods.txt` migration during conversion;
5. legacy root settings migration;
6. conflicting active external renderer rejection without mutation;
7. disabled external renderer preservation;
8. stale confirmed-plan rejection without mutation;
9. recognized same-product install handling;
10. injected late failure with byte-exact rollback;
11. strict unknown same-name ownership rejection;
12. payload tamper rejection;
13. reparse-path rejection;
14. structurally valid changed game-image acceptance;
15. malformed game-image rejection;
16. structurally valid changed UE4SS loader/proxy hash acceptance; and
17. Update / Repair of an older owned Radar while preserving all three
    user-owned files, refreshing immutable catalogs, and leaving no persistent
    backup;
18. confirmed uninstall removes only strictly owned Radar state;
19. stale uninstall confirmation is rejected without mutation; and
20. an injected uninstall failure restores Radar and `mods.txt` exactly.

The runner requires exactly:

```text
expected=20
passed=20
failed=0
skipped=0
release_gate=PASSED
sources_unchanged=true
fixtures_cleaned=true
```

Any other result blocks staging. This is static filesystem evidence only; it
does not prove deployment, gameplay behavior, or frame-time acceptance.
