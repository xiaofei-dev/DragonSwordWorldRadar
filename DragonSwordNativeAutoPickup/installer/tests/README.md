# Installer integration test

`tools/Test-Installer110.ps1` loads the built 1.3.0 Setup executable as a .NET
Framework assembly and calls its real internal inspection and mutation methods
against isolated temporary `DS/Binaries/Win64` trees. It never deploys to the
actual game installation.

## Input

The script accepts a local `DSClient-Win64-Shipping.exe` as a structural test
fixture. The executable hash is diagnostic only and does not block the test or
installation.

The built installer must exist at:

```text
out/installer/1.3.0/DragonSwordNativeAutoPickup-Setup-1.3.0.exe
```

## Current matrix

The isolated 1.3.0 matrix must pass 10/10 fixtures:

1. lifecycle and 15x/20x source contracts are present, the exact deployed
   1.2.0 payload is recognized for upgrade, while mutated and unproven hashes
   are rejected;
2. exact-runtime fresh installation is backup-free and exposes owned actions;
3. exact-runtime Repair preserves `config.ini` without a persistent backup;
4. exact recognized range-PAK filenames are replaced regardless of prior hash;
5. owned Uninstall removes only product-owned files and preserves UE4SS and
   unrelated Mods;
6. absent Uninstall is disabled and rejected without mutation;
7. unknown same-name Uninstall is disabled and rejected;
8. Original/3x/5x/10x/15x/20x switching leaves no stale range alternative;
9. confirmed conversion creates a verified backup and migrates unrelated Mods;
10. conversion backup name collisions use stable numeric suffixes.

Final archive contents pass in the canonical release pipeline. Deployment, UAC
UI, file-picker behavior, Authenticode trust, and gameplay are separate gates.

## Evidence boundary

Passing this matrix proves only isolated installer behavior for the exercised
fixtures. It does not prove the exact public archives or in-game pickup,
lifecycle, stability, or performance behavior.
